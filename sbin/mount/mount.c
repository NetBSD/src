/*	$NetBSD: mount.c,v 1.98 2013/01/24 17:53:49 christos Exp $	*/

/*
 * Copyright (c) 1980, 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1989, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount.c	8.25 (Berkeley) 5/8/95";
#else
__RCSID("$NetBSD: mount.c,v 1.98 2013/01/24 17:53:49 christos Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/wait.h>

#include <fs/puffs/puffs_msgif.h>

#include <err.h>
#include <errno.h>
#include <fstab.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#define MOUNTNAMES
#include <fcntl.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>

#include "pathnames.h"
#include "mountprog.h"

static int	debug, verbose;

static void	catopt(char **, const char *);
static const char *
		getfslab(const char *str);
static struct statvfs *
		getmntpt(const char *);
static int 	getmntargs(struct statvfs *, char *, size_t);
static int	hasopt(const char *, const char *);
static void	mangle(char *, int *, const char ** volatile *, int *);
static int	mountfs(const char *, const char *, const char *,
		    int, const char *, const char *, int, char *, size_t);
static void	prmount(struct statvfs *);
__dead static void	usage(void);


/* Map from mount otions to printable formats. */
static const struct opt {
	int o_opt;
	int o_silent;
	const char *o_name;
} optnames[] = {
	__MNT_FLAGS
};

static const char ffs_fstype[] = "ffs";

int
main(int argc, char *argv[])
{
	const char *mntfromname, *mntonname, **vfslist, *vfstype;
	struct fstab *fs;
	struct statvfs *mntbuf;
#if 0
	FILE *mountdfp;
#endif
	int all, ch, forceall, i, init_flags, mntsize, rval;
	char *options;
	const char *mountopts, *fstypename;
	char canonical_path_buf[MAXPATHLEN], buf[MAXPATHLEN];
	char *canonical_path;

	/* started as "mount" */
	all = forceall = init_flags = 0;
	options = NULL;
	vfslist = NULL;
	vfstype = ffs_fstype;
	while ((ch = getopt(argc, argv, "Aadfo:rwt:uv")) != -1)
		switch (ch) {
		case 'A':
			all = forceall = 1;
			break;
		case 'a':
			all = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 'f':
			init_flags |= MNT_FORCE;
			break;
		case 'o':
			if (*optarg)
				catopt(&options, optarg);
			break;
		case 'r':
			init_flags |= MNT_RDONLY;
			break;
		case 't':
			if (vfslist != NULL)
				errx(EXIT_FAILURE,
				    "Only one -t option may be specified.");
			vfslist = makevfslist(optarg);
			vfstype = optarg;
			break;
		case 'u':
			init_flags |= MNT_UPDATE;
			break;
		case 'v':
			verbose++;
			break;
		case 'w':
			init_flags &= ~MNT_RDONLY;
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

#define	BADTYPE(type)							\
	(strcmp(type, FSTAB_RO) &&					\
	    strcmp(type, FSTAB_RW) && strcmp(type, FSTAB_RQ))

	rval = 0;
	switch (argc) {
	case 0:
		if (all)
			while ((fs = getfsent()) != NULL) {
				if (BADTYPE(fs->fs_type))
					continue;
				if (checkvfsname(fs->fs_vfstype, vfslist))
					continue;
				if (hasopt(fs->fs_mntops, "noauto"))
					continue;
				if (strcmp(fs->fs_spec, "from_mount") == 0) {
					if ((mntbuf = getmntpt(fs->fs_file))
					    == NULL)
						errx(EXIT_FAILURE,
						    "Unknown file system %s",
						    fs->fs_file);
					mntfromname = mntbuf->f_mntfromname;
				} else
					mntfromname = fs->fs_spec;
				mntfromname = getfsspecname(buf, sizeof(buf),
				    mntfromname);
				if (mntfromname == NULL)
					err(EXIT_FAILURE, "%s", buf);
				if (mountfs(fs->fs_vfstype, mntfromname,
				    fs->fs_file, init_flags, options,
				    fs->fs_mntops, !forceall, NULL, 0))
					rval = 1;
			}
		else {
			if ((mntsize = getmntinfo(&mntbuf, MNT_NOWAIT)) == 0)
				err(EXIT_FAILURE, "getmntinfo");
			for (i = 0; i < mntsize; i++) {
				if (checkvfsname(mntbuf[i].f_fstypename,
				    vfslist))
					continue;
				prmount(&mntbuf[i]);
			}
		}
		return rval;
	case 1:
		if (vfslist != NULL) {
			usage();
			/* NOTREACHED */
		}

		/*
		 * Create a canonical version of the device or mount path
		 * passed to us.  It's ok for this to fail.  It's also ok
		 * for the result to be exactly the same as the original.
		 */
		canonical_path = realpath(*argv, canonical_path_buf);

		if (init_flags & MNT_UPDATE) {
			/*
			 * Try looking up the canonical path first,
			 * then try exactly what the user entered.
			 */
			if ((canonical_path == NULL ||
			    (mntbuf = getmntpt(canonical_path)) == NULL) &&
			    (mntbuf = getmntpt(*argv)) == NULL) {
out:
				errx(EXIT_FAILURE,
				    "Unknown special file or file system `%s'",
				    *argv);
			}
			mntfromname = mntbuf->f_mntfromname;
			if ((fs = getfsfile(mntbuf->f_mntonname)) != NULL) {
				if (strcmp(fs->fs_spec, "from_mount") != 0)
					mntfromname = fs->fs_spec;
				/* ignore the fstab file options.  */
				fs->fs_mntops = NULL;
			}
			mntonname  = mntbuf->f_mntonname;
			fstypename = mntbuf->f_fstypename;
			mountopts  = NULL;
		} else {
			/*
			 * Try looking up the canonical path first,
			 * then try exactly what the user entered.
			 */
			if (canonical_path == NULL ||
			    ((fs = getfsfile(canonical_path)) == NULL &&
			     (fs = getfsspec(canonical_path)) == NULL))
			{
				if ((fs = getfsfile(*argv)) == NULL &&
				    (fs = getfsspec(*argv)) == NULL) {
					goto out;
				}
			}
			if (BADTYPE(fs->fs_type))
				errx(EXIT_FAILURE,
				    "Unknown file system type for `%s'",
				    *argv);
			if (strcmp(fs->fs_spec, "from_mount") == 0) {
				if ((canonical_path == NULL ||
				    (mntbuf = getmntpt(canonical_path))
				     == NULL) &&
				    (mntbuf = getmntpt(*argv)) == NULL)
					goto out;
				mntfromname = mntbuf->f_mntfromname;
			} else
				mntfromname = fs->fs_spec;
			mntonname   = fs->fs_file;
			fstypename  = fs->fs_vfstype;
			mountopts   = fs->fs_mntops;
		}
		mntfromname = getfsspecname(buf, sizeof(buf), mntfromname);
		if (mntfromname == NULL)
			err(EXIT_FAILURE, "%s", buf);
		rval = mountfs(fstypename, mntfromname,
		    mntonname, init_flags, options, mountopts, 0, NULL, 0);
		break;
	case 2:
		/*
		 * If -t flag has not been specified, and spec contains either
		 * a ':' or a '@' then assume that an NFS filesystem is being
		 * specified ala Sun.
		 */
		mntfromname = getfsspecname(buf, sizeof(buf), argv[0]);
		if (mntfromname == NULL)
			err(EXIT_FAILURE, "%s", buf);
		if (vfslist == NULL) {
			if (strpbrk(argv[0], ":@") != NULL) {
				fprintf(stderr, "WARNING: autoselecting nfs "
				    "based on : or @ in the device name is "
				    "deprecated!\n"
				    "WARNING: This behaviour will be removed "
				    "in a future release\n");
				vfstype = "nfs";
			} else {
				vfstype = getfslab(mntfromname);
				if (vfstype == NULL)
					vfstype = ffs_fstype;
			}
		}
		rval = mountfs(vfstype, mntfromname, argv[1], init_flags,
		    options, NULL, 0, NULL, 0);
		break;
	default:
		usage();
	}

#if 0	/* disabled because it interferes the service. */
	/*
	 * If the mount was successfully, and done by root, tell mountd the
	 * good news.  Pid checks are probably unnecessary, but don't hurt.
	 */
	if (rval == 0 && getuid() == 0 &&
	    (mountdfp = fopen(_PATH_MOUNTDPID, "r")) != NULL) {
		int pid;

		if (fscanf(mountdfp, "%d", &pid) == 1 &&
		    pid > 0 && kill(pid, SIGHUP) == -1 && errno != ESRCH)
			err(EXIT_FAILURE, "signal mountd");
		(void)fclose(mountdfp);
	}
#endif

	return rval;
}

int
hasopt(const char *mntopts, const char *option)
{
	int negative, found;
	char *opt, *optbuf;

	if (option[0] == 'n' && option[1] == 'o') {
		negative = 1;
		option += 2;
	} else
		negative = 0;
	optbuf = estrdup(mntopts);
	found = 0;
	for (opt = optbuf; (opt = strtok(opt, ",")) != NULL; opt = NULL) {
		if (opt[0] == 'n' && opt[1] == 'o') {
			if (!strcasecmp(opt + 2, option))
				found = negative;
		} else if (!strcasecmp(opt, option))
			found = !negative;
	}
	free(optbuf);
	return (found);
}

static int
mountfs(const char *vfstype, const char *spec, const char *name, 
    int flags, const char *options, const char *mntopts,
    int skipmounted, char *buf, size_t buflen)
{
	/* List of directories containing mount_xxx subcommands. */
	static const char *edirs[] = {
#ifdef RESCUEDIR
		RESCUEDIR,
#endif
		_PATH_SBIN,
		_PATH_USRSBIN,
		NULL
	};
	const char ** volatile argv, **edir;
	struct statvfs *sfp, sf;
	pid_t pid;
	int pfd[2];
	int argc, numfs, i, status, maxargc;
	char *optbuf, execname[MAXPATHLEN + 1], execbase[MAXPATHLEN],
	    mntpath[MAXPATHLEN];
	volatile int getargs;

	if (realpath(name, mntpath) == NULL) {
		warn("realpath `%s'", name);
		return (1);
	}

	name = mntpath;

	optbuf = NULL;
	if (mntopts)
		catopt(&optbuf, mntopts);

	if (options) {
		catopt(&optbuf, options);
		getargs = strstr(options, "getargs") != NULL;
	} else
		getargs = 0;

	if (!mntopts && !options)
		catopt(&optbuf, "rw");

	if (getargs == 0 && strcmp(name, "/") == 0 && !hasopt(optbuf, "union"))
		flags |= MNT_UPDATE;
	else if (skipmounted) {
		if ((numfs = getmntinfo(&sfp, MNT_WAIT)) == 0) {
			warn("getmntinfo");
			return (1);
		}
		for(i = 0; i < numfs; i++) {
			const char *mountedtype = sfp[i].f_fstypename;
			size_t cmplen = sizeof(sfp[i].f_fstypename);

			/* remove "puffs|" from comparisons, if present */
#define TYPESIZE (sizeof(PUFFS_TYPEPREFIX)-1)
			if (strncmp(mountedtype,
			    PUFFS_TYPEPREFIX, TYPESIZE) == 0) {
				mountedtype += TYPESIZE;
				cmplen -= TYPESIZE;
			}

			/*
			 * XXX can't check f_mntfromname,
			 * thanks to mfs, union, etc.
			 */
			if (strncmp(name, sfp[i].f_mntonname, MNAMELEN) == 0 &&
			    strncmp(vfstype, mountedtype, cmplen) == 0) {
				if (verbose)
					(void)printf("%s on %s type %.*s: "
					    "%s\n",
					    sfp[i].f_mntfromname,
					    sfp[i].f_mntonname,
					    (int)sizeof(sfp[i].f_fstypename),
					    sfp[i].f_fstypename,
					    "already mounted");
				return (0);
			}
		}
	}
	if (flags & MNT_FORCE)
		catopt(&optbuf, "force");
	if (flags & MNT_RDONLY)
		catopt(&optbuf, "ro");

	if (flags & MNT_UPDATE) {
		catopt(&optbuf, "update");
		/* Figure out the fstype only if we defaulted to ffs */
		if (vfstype == ffs_fstype && statvfs(name, &sf) != -1)
			vfstype = sf.f_fstypename;
	}

	maxargc = 64;
	argv = ecalloc(maxargc, sizeof(*argv));

	if (getargs &&
	    strncmp(vfstype, PUFFS_TYPEPREFIX, sizeof(PUFFS_TYPEPREFIX)-1) == 0)
		(void)snprintf(execbase, sizeof(execbase), "mount_puffs");
	else if (hasopt(optbuf, "rump"))
		(void)snprintf(execbase, sizeof(execbase), "rump_%s", vfstype);
	else
		(void)snprintf(execbase, sizeof(execbase), "mount_%s", vfstype);
	argc = 0;
	argv[argc++] = execbase;
	if (optbuf)
		mangle(optbuf, &argc, &argv, &maxargc);
	argv[argc++] = spec;
	argv[argc++] = name;
	argv[argc] = NULL;

	if ((verbose && buf == NULL) || debug) {
		(void)printf("exec:");
		for (i = 0; i < argc; i++)
			(void)printf(" %s", argv[i]);
		(void)printf("\n");
	}

	if (buf) {
		if (pipe(pfd) == -1)
			warn("Cannot create pipe");
	}

	switch (pid = vfork()) {
	case -1:				/* Error. */
		warn("vfork");
		if (optbuf)
			free(optbuf);
		free(argv);
		return (1);

	case 0:					/* Child. */
		if (debug)
			_exit(0);

		if (buf) {
			(void)close(pfd[0]);
			(void)close(STDOUT_FILENO);
			if (dup2(pfd[1], STDOUT_FILENO) == -1)
				warn("Cannot open fd to mount program");
		}

		/* Go find an executable. */
		edir = edirs;
		do {
			(void)snprintf(execname,
			    sizeof(execname), "%s/%s", *edir, execbase);
			(void)execv(execname, __UNCONST(argv));
			if (errno != ENOENT)
				warn("exec %s for %s", execname, name);
		} while (*++edir != NULL);

		if (errno == ENOENT)
			warnx("%s not found for %s", execbase, name);
		_exit(1);
		/* NOTREACHED */

	default:				/* Parent. */
		if (optbuf)
			free(optbuf);
		free(argv);

		if (buf || getargs) {
			char tbuf[1024], *ptr;
			int nread;

			if (buf == NULL) {
				ptr = tbuf;
				buflen = sizeof(tbuf) - 1;
			} else {
				ptr = buf;
				buflen--;
			}
			(void)close(pfd[1]);
			(void)signal(SIGPIPE, SIG_IGN);
			while ((nread = read(pfd[0], ptr, buflen)) > 0) {
				buflen -= nread;
				ptr += nread;
			}
			*ptr = '\0';
			if (buflen == 0) {
				while (read(pfd[0], &nread, sizeof(nread)) > 0)
					continue;
			}
			if (buf == NULL)
				(void)fprintf(stdout, "%s", tbuf);
		}

		if (waitpid(pid, &status, 0) == -1) {
			warn("waitpid");
			return (1);
		}

		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) != 0)
				return (WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
			warnx("%s: %s", name, strsignal(WTERMSIG(status)));
			return (1);
		}

		if (buf == NULL) {
			if (verbose) {
				if (statvfs(name, &sf) == -1) {
					warn("statvfs %s", name);
					return (1);
				}
				prmount(&sf);
			}
		}
		break;
	}

	return (0);
}

static void
prmount(struct statvfs *sfp)
{
	int flags;
	const struct opt *o;
	struct passwd *pw;
	int f;

	(void)printf("%s on %s type %.*s", sfp->f_mntfromname,
	    sfp->f_mntonname, (int)sizeof(sfp->f_fstypename),
	    sfp->f_fstypename);

	flags = sfp->f_flag & MNT_VISFLAGMASK;
	for (f = 0, o = optnames; flags && o < 
	    &optnames[sizeof(optnames)/sizeof(optnames[0])]; o++)
		if (flags & o->o_opt) {
			if (!o->o_silent || verbose)
				(void)printf("%s%s", !f++ ? " (" : ", ",
				    o->o_name);
			flags &= ~o->o_opt;
		}
	if (flags)
		(void)printf("%sunknown flag%s %#x", !f++ ? " (" : ", ",
		    flags & (flags - 1) ? "s" : "", flags);
	if (sfp->f_owner) {
		(void)printf("%smounted by ", !f++ ? " (" : ", ");
		if ((pw = getpwuid(sfp->f_owner)) != NULL)
			(void)printf("%s", pw->pw_name);
		else
			(void)printf("%d", sfp->f_owner);
	}
	if (verbose)
		(void)printf("%sfsid: 0x%x/0x%x",
		    !f++ ? " (" /* ) */: ", ",
		    sfp->f_fsidx.__fsid_val[0], sfp->f_fsidx.__fsid_val[1]);

	if (verbose) {
		(void)printf("%s", !f++ ? " (" : ", ");
		(void)printf("reads: sync %" PRIu64 " async %" PRIu64 "",
		    sfp->f_syncreads, sfp->f_asyncreads);
		(void)printf(", writes: sync %" PRIu64 " async %" PRIu64 "",
		    sfp->f_syncwrites, sfp->f_asyncwrites);
		if (verbose > 1) {
			char buf[2048];

			if (getmntargs(sfp, buf, sizeof(buf)))
				printf(", [%s: %s]", sfp->f_fstypename, buf);
		}
		printf(")\n");
	} else
		(void)printf("%s", f ? ")\n" : "\n");
}

static int
getmntargs(struct statvfs *sfs, char *buf, size_t buflen)
{

	if (mountfs(sfs->f_fstypename, sfs->f_mntfromname, sfs->f_mntonname, 0,
	    "getargs", NULL, 0, buf, buflen))
		return (0);
	else {
		if (*buf == '\0')
			return (0);
		if ((buf = strchr(buf, '\n')) != NULL)
			*buf = '\0';
		return (1);
	}
}

static struct statvfs *
getmntpt(const char *name)
{
	struct statvfs *mntbuf;
	int i, mntsize;

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (i = 0; i < mntsize; i++)
		if (strcmp(mntbuf[i].f_mntfromname, name) == 0 ||
		    strcmp(mntbuf[i].f_mntonname, name) == 0)
			return (&mntbuf[i]);
	return (NULL);
}

static void
catopt(char **sp, const char *o)
{
	char *s, *n;

	s = *sp;
	if (s) {
		easprintf(&n, "%s,%s", s, o);
		free(s);
		s = n;
	} else
		s = estrdup(o);
	*sp = s;
}

static void
mangle(char *options, int *argcp, const char ** volatile *argvp, int *maxargcp)
{
	char *p, *s;
	int argc, maxargc;
	const char **argv, **nargv;

	argc = *argcp;
	argv = *argvp;
	maxargc = *maxargcp;

	for (s = options; (p = strsep(&s, ",")) != NULL;) {
		/* Always leave space for one more argument and the NULL. */
		if (argc >= maxargc - 4) {
			nargv = erealloc(argv, (maxargc << 1) * sizeof(nargv));
			argv = nargv;
			maxargc <<= 1;
		}
		if (*p != '\0') {
			if (*p == '-') {
				argv[argc++] = p;
				p = strchr(p, '=');
				if (p) {
					*p = '\0';
					argv[argc++] = p+1;
				}
			} else if (strcmp(p, "rw") != 0) {
				argv[argc++] = "-o";
				argv[argc++] = p;
			}
		}
	}

	*argcp = argc;
	*argvp = argv;
	*maxargcp = maxargc;
}

/* Deduce the filesystem type from the disk label. */
static const char *
getfslab(const char *str)
{
	static struct dkwedge_info dkw;
	struct disklabel dl;
	int fd;
	int part;
	const char *vfstype;
	u_char fstype;
	char buf[MAXPATHLEN + 1];
	char *sp, *ep;

	if ((fd = open(str, O_RDONLY)) == -1) {
		/*
		 * Iff we get EBUSY try the raw device. Since mount always uses
		 * the block device we know we are never passed a raw device.
		 */
		if (errno != EBUSY)
			err(EXIT_FAILURE, "cannot open `%s'", str);
		strlcpy(buf, str, MAXPATHLEN);
		if ((sp = strrchr(buf, '/')) != NULL)
			++sp;
		else
			sp = buf;
		for (ep = sp + strlen(sp) + 1;  ep > sp; ep--)
			*ep = *(ep - 1);
		*sp = 'r';

		/* Silently fail here - mount call can display error */
		if ((fd = open(buf, O_RDONLY)) == -1)
			return (NULL);
	}

	/* Check to see if this is a wedge. */
	if (ioctl(fd, DIOCGWEDGEINFO, &dkw) == 0) {
		/* Yup, this is easy. */
		(void) close(fd);
		return (dkw.dkw_ptype);
	}

	if (ioctl(fd, DIOCGDINFO, &dl) == -1) {
		(void) close(fd);
		return (NULL);
	}

	(void) close(fd);

	part = str[strlen(str) - 1] - 'a';

	if (part < 0 || part >= dl.d_npartitions)
		return (NULL);

	/* Return NULL for unknown types - caller can fall back to ffs */
	if ((fstype = dl.d_partitions[part].p_fstype) >= FSMAXMOUNTNAMES)
		vfstype = NULL;
	else
		vfstype = mountnames[fstype];

	return (vfstype);
}

static void
usage(void)
{

	(void)fprintf(stderr, "Usage: %s [-Aadfruvw] [-t type]\n"
	    "\t%s [-dfruvw] special | node\n"
	    "\t%s [-dfruvw] [-o options] [-t type] special node\n",
	    getprogname(), getprogname(), getprogname());
	exit(1);
}
