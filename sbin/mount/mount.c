/*	$NetBSD: mount.c,v 1.59 2002/05/21 23:51:19 nathanw Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1989, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount.c	8.25 (Berkeley) 5/8/95";
#else
__RCSID("$NetBSD: mount.c,v 1.59 2002/05/21 23:51:19 nathanw Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fstab.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MOUNTNAMES
#include <fcntl.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>

#include "pathnames.h"
#include "vfslist.h"

static int	debug, verbose;

static void	catopt __P((char **, const char *));
static const char *
		getfslab __P((const char *str));
static struct statfs *
		getmntpt __P((const char *));
static int	hasopt __P((const char *, const char *));
static void	mangle __P((char *, int *, const char ***, int *));
static int	mountfs __P((const char *, const char *, const char *,
		    int, const char *, const char *, int));
static void	prmount __P((struct statfs *));
static void	usage __P((void));

#ifndef NO_MOUNT_PROGS
void	checkname __P((int, char *[]));
#endif
int	main __P((int, char *[]));

/* Map from mount otions to printable formats. */
static const struct opt {
	int o_opt;
	int o_silent;
	const char *o_name;
} optnames[] = {
	__MNT_FLAGS
};

static char ffs_fstype[] = "ffs";

int
main(argc, argv)
	int argc;
	char *argv[];
{
	const char *mntfromname, *mntonname, **vfslist, *vfstype;
	struct fstab *fs;
	struct statfs *mntbuf;
	FILE *mountdfp;
	int all, ch, forceall, i, init_flags, mntsize, rval;
	char *options;
	const char *mountopts, *fstypename;

#ifndef NO_MOUNT_PROGS
	/* if called as specific mount, call it's main mount routine */
	checkname(argc, argv);
#endif

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
				errx(1,
				    "only one -t option may be specified.");
			vfslist = makevfslist(optarg);
			vfstype = optarg;
			break;
		case 'u':
			init_flags |= MNT_UPDATE;
			break;
		case 'v':
			verbose = 1;
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
				if (mountfs(fs->fs_vfstype, fs->fs_spec,
				    fs->fs_file, init_flags, options,
				    fs->fs_mntops, !forceall))
					rval = 1;
			}
		else {
			if ((mntsize = getmntinfo(&mntbuf, MNT_NOWAIT)) == 0)
				err(1, "getmntinfo");
			for (i = 0; i < mntsize; i++) {
				if (checkvfsname(mntbuf[i].f_fstypename,
				    vfslist))
					continue;
				prmount(&mntbuf[i]);
			}
		}
		exit(rval);
		/* NOTREACHED */
	case 1:
		if (vfslist != NULL) {
			usage();
			/* NOTREACHED */
		}

		if (init_flags & MNT_UPDATE) {
			if ((mntbuf = getmntpt(*argv)) == NULL)
				errx(1,
				    "unknown special file or file system %s.",
				    *argv);
			if ((fs = getfsfile(mntbuf->f_mntonname)) != NULL) {
				mntfromname = fs->fs_spec;
				/* ignore the fstab file options.  */
				fs->fs_mntops = NULL;
			} else
				mntfromname = mntbuf->f_mntfromname;
			mntonname  = mntbuf->f_mntonname;
			fstypename = mntbuf->f_fstypename;
			mountopts  = NULL;
		} else {
			if ((fs = getfsfile(*argv)) == NULL &&
			    (fs = getfsspec(*argv)) == NULL)
				errx(1,
				    "%s: unknown special file or file system.",
				    *argv);
			if (BADTYPE(fs->fs_type))
				errx(1, "%s has unknown file system type.",
				    *argv);
			mntfromname = fs->fs_spec;
			mntonname   = fs->fs_file;
			fstypename  = fs->fs_vfstype;
			mountopts   = fs->fs_mntops;
		}
		rval = mountfs(fstypename, mntfromname,
		    mntonname, init_flags, options, mountopts, 0);
		break;
	case 2:
		/*
		 * If -t flag has not been specified, and spec contains either
		 * a ':' or a '@' then assume that an NFS filesystem is being
		 * specified ala Sun.
		 */
		if (vfslist == NULL) {
			if (strpbrk(argv[0], ":@") != NULL)
				vfstype = "nfs";
			else {
				vfstype = getfslab(argv[0]);
				if (vfstype == NULL)
					vfstype = ffs_fstype;
			}
		}
		rval = mountfs(vfstype,
		    argv[0], argv[1], init_flags, options, NULL, 0);
		break;
	default:
		usage();
		/* NOTREACHED */
	}

	/*
	 * If the mount was successfully, and done by root, tell mountd the
	 * good news.  Pid checks are probably unnecessary, but don't hurt.
	 */
	if (rval == 0 && getuid() == 0 &&
	    (mountdfp = fopen(_PATH_MOUNTDPID, "r")) != NULL) {
		int pid;

		if (fscanf(mountdfp, "%d", &pid) == 1 &&
		    pid > 0 && kill(pid, SIGHUP) == -1 && errno != ESRCH)
			err(1, "signal mountd");
		(void)fclose(mountdfp);
	}

	exit(rval);
	/* NOTREACHED */
}

int
hasopt(mntopts, option)
	const char *mntopts, *option;
{
	int negative, found;
	char *opt, *optbuf;

	if (option[0] == 'n' && option[1] == 'o') {
		negative = 1;
		option += 2;
	} else
		negative = 0;
	optbuf = strdup(mntopts);
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
mountfs(vfstype, spec, name, flags, options, mntopts, skipmounted)
	const char *vfstype, *spec, *name, *options, *mntopts;
	int flags, skipmounted;
{
	/* List of directories containing mount_xxx subcommands. */
	static const char *edirs[] = {
		_PATH_SBIN,
		_PATH_USRSBIN,
		NULL
	};
	const char **argv, **edir;
	struct statfs *sfp, sf;
	pid_t pid;
	int argc, numfs, i, status, maxargc;
	char *optbuf, execname[MAXPATHLEN + 1], execbase[MAXPATHLEN],
	    mntpath[MAXPATHLEN];

#ifdef __GNUC__
	(void) &name;
	(void) &optbuf;
	(void) &vfstype;
#endif

	if (realpath(name, mntpath) == NULL) {
		warn("realpath %s", name);
		return (1);
	}

	name = mntpath;

	optbuf = NULL;
	if (mntopts)
		catopt(&optbuf, mntopts);
	if (options)
		catopt(&optbuf, options);
	if (!mntopts && !options)
		catopt(&optbuf, "rw");

	if (!strcmp(name, "/"))
		flags |= MNT_UPDATE;
	else if (skipmounted) {
		if ((numfs = getmntinfo(&sfp, MNT_WAIT)) == 0) {
			warn("getmntinfo");
			return (1);
		}
		for(i = 0; i < numfs; i++) {
			/*
			 * XXX can't check f_mntfromname,
			 * thanks to mfs, union, etc.
			 */
			if (strncmp(name, sfp[i].f_mntonname, MNAMELEN) == 0 &&
			    strncmp(vfstype, sfp[i].f_fstypename,
				MFSNAMELEN) == 0) {
				if (verbose)
					(void)printf("%s on %s type %.*s: "
					    "%s\n",
					    sfp[i].f_mntfromname,
					    sfp[i].f_mntonname,
					    MFSNAMELEN,
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
		if (vfstype == ffs_fstype && statfs(name, &sf) != -1)
			vfstype = sf.f_fstypename;
	}

	maxargc = 64;
	argv = malloc(sizeof(char *) * maxargc);

	(void) snprintf(execbase, sizeof(execbase), "mount_%s", vfstype);
	argc = 0;
	argv[argc++] = execbase;
	if (optbuf)
		mangle(optbuf, &argc, &argv, &maxargc);
	argv[argc++] = spec;
	argv[argc++] = name;
	argv[argc] = NULL;

	if (verbose) {
		(void)printf("exec:");
		for (i = 0; i < argc; i++)
			(void)printf(" %s", argv[i]);
		(void)printf("\n");
	}

	switch (pid = vfork()) {
	case -1:				/* Error. */
		warn("vfork");
		if (optbuf)
			free(optbuf);
		return (1);

	case 0:					/* Child. */
		if (debug)
			_exit(0);

		/* Go find an executable. */
		edir = edirs;
		do {
			(void)snprintf(execname,
			    sizeof(execname), "%s/%s", *edir, execbase);
			(void)execv(execname, (char * const *)argv);
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

		if (waitpid(pid, &status, 0) < 0) {
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

		if (verbose) {
			if (statfs(name, &sf) < 0) {
				warn("statfs %s", name);
				return (1);
			}
			prmount(&sf);
		}
		break;
	}

	return (0);
}

static void
prmount(sfp)
	struct statfs *sfp;
{
	int flags;
	const struct opt *o;
	struct passwd *pw;
	int f;

	(void)printf("%s on %s type %.*s", sfp->f_mntfromname,
	    sfp->f_mntonname, MFSNAMELEN, sfp->f_fstypename);

	flags = sfp->f_flags & MNT_VISFLAGMASK;
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
		(void)printf("%swrites: sync %ld async %ld)\n",
		    !f++ ? " (" : ", ", sfp->f_syncwrites, sfp->f_asyncwrites);
	else
		(void)printf("%s", f ? ")\n" : "\n");
}

static struct statfs *
getmntpt(name)
	const char *name;
{
	struct statfs *mntbuf;
	int i, mntsize;

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (i = 0; i < mntsize; i++)
		if (strcmp(mntbuf[i].f_mntfromname, name) == 0 ||
		    strcmp(mntbuf[i].f_mntonname, name) == 0)
			return (&mntbuf[i]);
	return (NULL);
}

static void
catopt(sp, o)
	char **sp;
	const char *o;
{
	char *s;
	size_t i, j;

	s = *sp;
	if (s) {
		i = strlen(s);
		j = i + 1 + strlen(o) + 1;
		s = realloc(s, j);
		(void)snprintf(s + i, j, ",%s", o);
	} else
		s = strdup(o);
	*sp = s;
}

static void
mangle(options, argcp, argvp, maxargcp)
	char *options;
	int *argcp, *maxargcp;
	const char ***argvp;
{
	char *p, *s;
	int argc, maxargc;
	const char **argv;

	argc = *argcp;
	argv = *argvp;
	maxargc = *maxargcp;

	for (s = options; (p = strsep(&s, ",")) != NULL;) {
		/* Always leave space for one more argument and the NULL. */
		if (argc >= maxargc - 4) {
			maxargc <<= 1;
			argv = realloc(argv, maxargc * sizeof(char *));
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
getfslab(str)
	const char *str;
{
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
			err(1, "cannot open `%s'", str);
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
usage()
{

	(void)fprintf(stderr,
	    "usage: mount %s\n       mount %s\n       mount %s\n",
	    "[-Aadfruvw] [-t type]",
	    "[-dfruvw] special | node",
	    "[-dfruvw] [-o options] [-t type] special node");
	exit(1);
	/* NOTREACHED */
}
