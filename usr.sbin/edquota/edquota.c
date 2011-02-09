/*      $NetBSD: edquota.c,v 1.29.16.11 2011/02/09 17:48:45 bouyer Exp $ */
/*
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1990, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "from: @(#)edquota.c	8.3 (Berkeley) 4/27/95";
#else
__RCSID("$NetBSD: edquota.c,v 1.29.16.11 2011/02/09 17:48:45 bouyer Exp $");
#endif
#endif /* not lint */

/*
 * Disk quota editor.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/statvfs.h>

#include <ufs/ufs/quota2_prop.h>
#include <ufs/ufs/quota1.h>
#include <sys/quota.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fstab.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <printquota.h>
#include <getvfsquota.h>

#include "pathnames.h"

const char *qfname = QUOTAFILENAME;
const char *quotagroup = QUOTAGROUP;
char tmpfil[] = _PATH_TMP;

struct quotause {
	struct	quotause *next;
	long	flags;
	struct	quota2_entry q2e;
	char	fsname[MAXPATHLEN + 1];
	char	*qfname;
};
#define	FOUND	0x01
#define	QUOTA2	0x02
#define	DEFAULT	0x04

#define MAX_TMPSTR	(100+MAXPATHLEN)

int	main(int, char **);
void	usage(void);
int	getentry(const char *, int);
struct quotause * getprivs(long, int, const char *, int);
struct quotause * getprivs2(long, int, const char *, int);
struct quotause * getprivs1(long, int, const char *);
void	putprivs(long, int, struct quotause *);
void	putprivs2(long, int, struct quotause *);
void	putprivs1(long, int, struct quotause *);
int	editit(char *);
int	writeprivs(struct quotause *, int, char *, int);
int	readprivs(struct quotause *, int);
int	writetimes(struct quotause *, int, int);
int	readtimes(struct quotause *, int);
void	freeq(struct quotause *);
void	freeprivs(struct quotause *);
int	alldigits(const char *);
int	hasquota(struct fstab *, int, char **);
static void clearpriv(int, char **, const char *, int);

int Hflag = 0;
int Dflag = 0;
int dflag = 0;

int
main(argc, argv)
	int argc;
	char **argv;
{
	struct quotause *qup, *protoprivs, *curprivs;
	long id, protoid;
	int quotatype, tmpfd;
	char *protoname;
	char *soft = NULL, *hard = NULL, *grace = NULL;
	char *fs = NULL;
	int ch;
	int pflag = 0;
	int cflag = 0;

	if (argc < 2)
		usage();
	if (getuid())
		errx(1, "permission denied");
	protoname = NULL;
	quotatype = USRQUOTA;
	while ((ch = getopt(argc, argv, "DHcdugp:s:h:t:f:")) != -1) {
		switch(ch) {
		case 'D':
			Dflag++;
			break;
		case 'H':
			Hflag++;
			break;
		case 'c':
			cflag++;
			break;
		case 'd':
			dflag++;
			break;
		case 'p':
			protoname = optarg;
			pflag++;
			break;
		case 'g':
			quotatype = GRPQUOTA;
			break;
		case 'u':
			quotatype = USRQUOTA;
			break;
		case 's':
			soft = optarg;
			break;
		case 'h':
			hard = optarg;
			break;
		case 't':
			grace = optarg;
			break;
		case 'f':
			fs = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (pflag) {
		if (soft || hard || grace || dflag || cflag)
			usage();
		if ((protoid = getentry(protoname, quotatype)) == -1)
			exit(1);
		protoprivs = getprivs(protoid, quotatype, fs, 0);
		for (qup = protoprivs; qup; qup = qup->next) {
			qup->q2e.q2e_val[QL_BLOCK].q2v_time = 0;
			qup->q2e.q2e_val[QL_FILE].q2v_time = 0;
		}
		while (argc-- > 0) {
			if ((id = getentry(*argv++, quotatype)) < 0)
				continue;
			putprivs(id, quotatype, protoprivs);
		}
		exit(0);
	}
	if (soft || hard || grace) {
		struct quotause *lqup;
		u_int64_t softb, hardb, softi, hardi;
		time_t  graceb, gracei;
		char *str;

		if (cflag)
			usage();
		if (soft) {
			str = strsep(&soft, "/");
			if (str[0] == '\0' || soft == NULL || soft[0] == '\0')
				usage();
			    
			if (intrd(str, &softb, HN_B) != 0)
				errx(1, "%s: bad number", str);
			if (intrd(soft, &softi, 0) != 0)
				errx(1, "%s: bad number", soft);
		}
		if (hard) {
			str = strsep(&hard, "/");
			if (str[0] == '\0' || hard == NULL || hard[0] == '\0')
				usage();
			    
			if (intrd(str, &hardb, HN_B) != 0)
				errx(1, "%s: bad number", str);
			if (intrd(hard, &hardi, 0) != 0)
				errx(1, "%s: bad number", hard);
		}
		if (grace) {
			str = strsep(&grace, "/");
			if (str[0] == '\0' || grace == NULL || grace[0] == '\0')
				usage();
			    
			if (timeprd(str, &graceb) != 0)
				errx(1, "%s: bad number", str);
			if (timeprd(grace, &gracei) != 0)
				errx(1, "%s: bad number", grace);
		}
		if (dflag) {
			curprivs = getprivs(0, quotatype, fs, 1);
			for (lqup = curprivs; lqup; lqup = lqup->next) {
				if (soft) {
					lqup->q2e.q2e_val[QL_BLOCK].q2v_softlimit = softb;
					lqup->q2e.q2e_val[QL_FILE].q2v_softlimit = softi;
				}
				if (hard) {
					lqup->q2e.q2e_val[QL_BLOCK].q2v_hardlimit = hardb;
					lqup->q2e.q2e_val[QL_FILE].q2v_hardlimit = hardi;
				}
				if (grace) {
					lqup->q2e.q2e_val[QL_BLOCK].q2v_grace = graceb;
					lqup->q2e.q2e_val[QL_FILE].q2v_grace = gracei;
				}
			}
			putprivs(0, quotatype, curprivs);
			freeprivs(curprivs);
			exit(0);
		}
		for ( ; argc > 0; argc--, argv++) {
			if ((id = getentry(*argv, quotatype)) == -1)
				continue;
			curprivs = getprivs(id, quotatype, fs, 0);
			for (lqup = curprivs; lqup; lqup = lqup->next) {
				if (soft) {
					if (softb &&
					    lqup->q2e.q2e_val[QL_BLOCK].q2v_cur >= softb &&
					    (lqup->q2e.q2e_val[QL_BLOCK].q2v_softlimit == 0 ||
					    lqup->q2e.q2e_val[QL_BLOCK].q2v_cur < lqup->q2e.q2e_val[QL_BLOCK].q2v_softlimit))
						lqup->q2e.q2e_val[QL_BLOCK].q2v_time = 0;
					if (softi &&
					    lqup->q2e.q2e_val[QL_FILE].q2v_cur >= softb &&
					    (lqup->q2e.q2e_val[QL_FILE].q2v_softlimit == 0 ||
					    lqup->q2e.q2e_val[QL_FILE].q2v_cur < lqup->q2e.q2e_val[QL_FILE].q2v_softlimit))
						lqup->q2e.q2e_val[QL_FILE].q2v_time = 0;
					lqup->q2e.q2e_val[QL_BLOCK].q2v_softlimit = softb;
					lqup->q2e.q2e_val[QL_FILE].q2v_softlimit = softi;
				}
				if (hard) {
					lqup->q2e.q2e_val[QL_BLOCK].q2v_hardlimit = hardb;
					lqup->q2e.q2e_val[QL_FILE].q2v_hardlimit = hardi;
				}
				if (grace) {
					lqup->q2e.q2e_val[QL_BLOCK].q2v_grace = graceb;
					lqup->q2e.q2e_val[QL_FILE].q2v_grace = gracei;
				}
			}
			putprivs(id, quotatype, curprivs);
			freeprivs(curprivs);
		}
		exit(0);
	}
	if (cflag) {
		if (dflag)
			usage();
		clearpriv(argc, argv, fs, quotatype);
		exit(0);
	}
	tmpfd = mkstemp(tmpfil);
	fchown(tmpfd, getuid(), getgid());
	if (dflag) {
		curprivs = getprivs(0, quotatype, fs, 1);
		if (writeprivs(curprivs, tmpfd, NULL, quotatype) &&
		    editit(tmpfil) && readprivs(curprivs, tmpfd))
			putprivs(0, quotatype, curprivs);
		freeprivs(curprivs);
	}
	for ( ; argc > 0; argc--, argv++) {
		if ((id = getentry(*argv, quotatype)) == -1)
			continue;
		curprivs = getprivs(id, quotatype, fs, 0);
		if (writeprivs(curprivs, tmpfd, *argv, quotatype) == 0)
			continue;
		if (editit(tmpfil) && readprivs(curprivs, tmpfd))
			putprivs(id, quotatype, curprivs);
		freeprivs(curprivs);
	}
	close(tmpfd);
	unlink(tmpfil);
	exit(0);
}

void
usage()
{
	fprintf(stderr,
	    "usage:\n"
	    "  edquota [-D] [-H] [-u] [-p username] [-f filesystem] -d | username ...\n"
	    "  edquota [-D] [-H] -g [-p groupname] [-f filesystem] -d | groupname ...\n"
	    "  edquota [-D] [-u] [-f filesystem] [-s b#/i#] [-h b#/i#] [-t t#/t#] \\\n\t-d | username ...\n"
	    "  edquota [-D] -g [-f filesystem] [-s b#/i#] [-h b#/i#] [-t t#/t#] \\\n\t-d | groupname ...\n"
	    "  edquota [-D] [-H] [-u] -c [-f filesystem] username ...\n"
	    "  edquota [-D] [-H] -g -c [-f filesystem] groupname ...\n"
	    );
	exit(1);
}

/*
 * This routine converts a name for a particular quota type to
 * an identifier. This routine must agree with the kernel routine
 * getinoquota as to the interpretation of quota types.
 */
int
getentry(name, quotatype)
	const char *name;
	int quotatype;
{
	struct passwd *pw;
	struct group *gr;

	if (alldigits(name))
		return (atoi(name));
	switch(quotatype) {
	case USRQUOTA:
		if ((pw = getpwnam(name)) != NULL)
			return (pw->pw_uid);
		warnx("%s: no such user", name);
		break;
	case GRPQUOTA:
		if ((gr = getgrnam(name)) != NULL)
			return (gr->gr_gid);
		warnx("%s: no such group", name);
		break;
	default:
		warnx("%d: unknown quota type", quotatype);
		break;
	}
	sleep(1);
	return (-1);
}

/*
 * Collect the requested quota information.
 */
struct quotause *
getprivs(long id, int quotatype, const char *filesys, int defaultq)
{
	struct statvfs *fst;
	int nfst, i;
	struct quotause *qup, *quptail = NULL;
	struct quotause *quphead = NULL;

	nfst = getmntinfo(&fst, MNT_WAIT);
	if (nfst == 0)
		errx(2, "no filesystems mounted!");

	for (i = 0; i < nfst; i++) {
		if (strcmp(fst[i].f_fstypename, "ffs") != 0 ||
		    (fst[i].f_flag & ST_QUOTA) == 0)
			continue;
		if (filesys && strcmp(fst[i].f_mntonname, filesys) != 0 &&
		    strcmp(fst[i].f_mntfromname, filesys) != 0)
			continue;
		qup = getprivs2(id, quotatype, fst[i].f_mntonname, defaultq);
		if (qup == NULL)
			return NULL;
		if (quphead == NULL)
			quphead = qup;
		else
			quptail->next = qup;
		quptail = qup;
		qup->next = 0;
	}

	if (filesys && quphead == NULL) {
		if (defaultq)
			errx(1, "no default quota for version 1");
		/* if we get there, filesys is not mounted. try the old way */
		qup = getprivs1(id, quotatype, filesys);
		if (qup == NULL)
			return NULL;
		if (quphead == NULL)
			quphead = qup;
		else
			quptail->next = qup;
		quptail = qup;
		qup->next = 0;
	}
	return quphead;
}

struct quotause *
getprivs2(long id, int quotatype, const char *filesys, int defaultq)
{
	struct quotause *qup;
	int8_t version;

	if ((qup = (struct quotause *)malloc(sizeof(*qup))) == NULL)
		errx(2, "out of memory");
	memset(qup, 0, sizeof(*qup));
	strcpy(qup->fsname, filesys);
	if (defaultq)
		qup->flags |= DEFAULT;
	if (!getvfsquota(filesys, &qup->q2e, &version,
	    id, quotatype, defaultq, Dflag)) {
		/* no entry, get default entry */
		if (!getvfsquota(filesys, &qup->q2e, &version,
		    id, quotatype, 1, Dflag)) {
			free(qup);
			return NULL;
		}
	}
	if (version == 2)
		qup->flags |= QUOTA2;
	qup->q2e.q2e_uid = id;
	return qup;
}

struct quotause *
getprivs1(long id, int quotatype, const char *filesys)
{
	struct fstab *fs;
	char *qfpathname;
	struct quotause *qup;
	struct dqblk dqblk;
	int fd;

	setfsent();
	while ((fs = getfsent()) != NULL) {
		if (strcmp(fs->fs_vfstype, "ffs"))
			continue;
		if (strcmp(fs->fs_spec, filesys) == 0 ||
		    strcmp(fs->fs_file, filesys) == 0)
			break;
	}
	if (fs == NULL)
		return NULL;

	if (!hasquota(fs, quotatype, &qfpathname))
		return NULL;
	if ((qup = (struct quotause *)malloc(sizeof(*qup))) == NULL)
		errx(2, "out of memory");
	strcpy(qup->fsname, fs->fs_file);
	if ((fd = open(qfpathname, O_RDONLY)) < 0) {
		fd = open(qfpathname, O_RDWR|O_CREAT, 0640);
		if (fd < 0 && errno != ENOENT) {
			warnx("open `%s'", qfpathname);
			freeq(qup);
			return NULL;
		}
		warnx("Creating quota file %s", qfpathname);
		sleep(3);
		(void) fchown(fd, getuid(),
		    getentry(quotagroup, GRPQUOTA));
		(void) fchmod(fd, 0640);
	}
	(void)lseek(fd, (off_t)(id * sizeof(struct dqblk)),
	    SEEK_SET);
	switch (read(fd, &dqblk, sizeof(struct dqblk))) {
	case 0:			/* EOF */
		/*
		 * Convert implicit 0 quota (EOF)
		 * into an explicit one (zero'ed dqblk)
		 */
		memset((caddr_t)&dqblk, 0,
		    sizeof(struct dqblk));
		break;

	case sizeof(struct dqblk):	/* OK */
		break;

	default:		/* ERROR */
		warn("read error in `%s'", qfpathname);
		close(fd);
		freeq(qup);
		return NULL;
	}
	close(fd);
	qup->qfname = qfpathname;
	endfsent();
	dqblk2q2e(&dqblk, &qup->q2e);
	return (qup);
}

/*
 * Store the requested quota information.
 */
void
putprivs(long id, int quotatype, struct quotause *quplist)
{
	struct quotause *qup;

        for (qup = quplist; qup; qup = qup->next) {
		if (qup->qfname == NULL)
			putprivs2(id, quotatype, qup);
		else
			putprivs1(id, quotatype, qup);
	}
}

void
putprivs2(long id, int quotatype, struct quotause *qup)
{

	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	struct plistref pref;
	int error;
	int8_t error8;

	qup->q2e.q2e_uid = id;
	data = q2etoprop(&qup->q2e, (qup->flags & DEFAULT) ? 1 : 0);

	if (data == NULL)
		err(1, "q2etoprop(id)");

	dict = quota2_prop_create();
	cmds = prop_array_create();
	datas = prop_array_create();

	if (dict == NULL || cmds == NULL || datas == NULL) {
		errx(1, "can't allocate proplist");
	}

	if (!prop_array_add_and_rel(datas, data))
		err(1, "prop_array_add(data)");
	
	if (!quota2_prop_add_command(cmds, "set",
	    qfextension[quotatype], datas))
		err(1, "prop_add_command");
	if (!prop_dictionary_set(dict, "commands", cmds))
		err(1, "prop_dictionary_set(command)");
	if (Dflag)
		printf("message to kernel:\n%s\n",
		    prop_dictionary_externalize(dict));

	if (!prop_dictionary_send_syscall(dict, &pref))
		err(1, "prop_dictionary_send_syscall");
	prop_object_release(dict);

	if (quotactl(qup->fsname, &pref) != 0)
		err(1, "quotactl");

	if ((error = prop_dictionary_recv_syscall(&pref, &dict)) != 0) {
		errx(1, "prop_dictionary_recv_syscall: %s\n",
		    strerror(error));
	}

	if (Dflag)
		printf("reply from kernel:\n%s\n",
		    prop_dictionary_externalize(dict));

	if ((error = quota2_get_cmds(dict, &cmds)) != 0) {
		errx(1, "quota2_get_cmds: %s\n",
		    strerror(error));
	}
	/* only one command, no need to iter */
	cmd = prop_array_get(cmds, 0);
	if (cmd == NULL)
		err(1, "prop_array_get(cmd)");

	if (!prop_dictionary_get_int8(cmd, "return", &error8))
		err(1, "prop_get(return)");

	if (error8) {
		if (qup->flags & DEFAULT)
			fprintf(stderr, "set default %s quota: %s\n",
			    qfextension[quotatype], strerror(error8));
		else
			fprintf(stderr, "set %s quota for %ld: %s\n",
			    qfextension[quotatype], id, strerror(error8));
	}
	prop_object_release(dict);
}

void
putprivs1(long id, int quotatype, struct quotause *qup)
{
	struct dqblk dqblk;
	int fd;

	q2e2dqblk(&qup->q2e, &dqblk);
	assert((qup->flags & DEFAULT) == 0);

	if ((fd = open(qup->qfname, O_WRONLY)) < 0) {
		warnx("open `%s'", qup->qfname);
	} else {
		(void)lseek(fd,
		    (off_t)(id * (long)sizeof (struct dqblk)),
		    SEEK_SET);
		if (write(fd, &dqblk, sizeof (struct dqblk)) !=
		    sizeof (struct dqblk))
			warnx("writing `%s'", qup->qfname);
		close(fd);
	}
}

/*
 * Take a list of privileges and get it edited.
 */
int
editit(ltmpfile)
	char *ltmpfile;
{
	long omask;
	int pid, lst;
	char p[MAX_TMPSTR];

	omask = sigblock(sigmask(SIGINT)|sigmask(SIGQUIT)|sigmask(SIGHUP));
 top:
	if ((pid = fork()) < 0) {

		if (errno == EPROCLIM) {
			warnx("You have too many processes");
			return(0);
		}
		if (errno == EAGAIN) {
			sleep(1);
			goto top;
		}
		warn("fork");
		return (0);
	}
	if (pid == 0) {
		const char *ed;

		sigsetmask(omask);
		setgid(getgid());
		setuid(getuid());
		if ((ed = getenv("EDITOR")) == (char *)0)
			ed = _PATH_VI;
		if (strlen(ed) + strlen(ltmpfile) + 2 >= MAX_TMPSTR) {
			err (1, "%s", "editor or filename too long");
		}
		snprintf (p, MAX_TMPSTR, "%s %s", ed, ltmpfile);
		execlp(_PATH_BSHELL, _PATH_BSHELL, "-c", p, NULL);
		err(1, "%s", ed);
	}
	waitpid(pid, &lst, 0);
	sigsetmask(omask);
	if (!WIFEXITED(lst) || WEXITSTATUS(lst) != 0)
		return (0);
	return (1);
}

/*
 * Convert a quotause list to an ASCII file.
 */
int
writeprivs(quplist, outfd, name, quotatype)
	struct quotause *quplist;
	int outfd;
	char *name;
	int quotatype;
{
	struct quotause *qup;
	FILE *fd;

	ftruncate(outfd, 0);
	(void)lseek(outfd, (off_t)0, SEEK_SET);
	if ((fd = fdopen(dup(outfd), "w")) == NULL)
		errx(1, "fdopen `%s'", tmpfil);
	if (dflag) {
		fprintf(fd, "Default %s quotas:\n", qfextension[quotatype]);
	} else {
		fprintf(fd, "Quotas for %s %s:\n",
		    qfextension[quotatype], name);
	}
	for (qup = quplist; qup; qup = qup->next) {
		fprintf(fd, "%s (version %d):\n",
		     qup->fsname, (qup->flags & QUOTA2) ? 2 : 1);
		if ((qup->flags & DEFAULT) == 0 || (qup->flags & QUOTA2) != 0) {
			fprintf(fd, "\tblocks in use: %s, "
			    "limits (soft = %s, hard = %s",
			    intprt(qup->q2e.q2e_val[QL_BLOCK].q2v_cur,
				HN_NOSPACE | HN_B, Hflag, 20), 
			    intprt(qup->q2e.q2e_val[QL_BLOCK].q2v_softlimit,
				HN_NOSPACE | HN_B, Hflag, 20),
			    intprt(qup->q2e.q2e_val[QL_BLOCK].q2v_hardlimit,
				HN_NOSPACE | HN_B, Hflag, 20));
			if (qup->flags & QUOTA2)
				fprintf(fd, ", ");
		} else
			fprintf(fd, "\tblocks: (");
			
		if (qup->flags & (QUOTA2|DEFAULT)) {
		    fprintf(fd, "grace = %s",
			timepprt(qup->q2e.q2e_val[QL_BLOCK].q2v_grace,
			    Hflag, 20));
		}
		fprintf(fd, ")\n");
		if ((qup->flags & DEFAULT) == 0 || (qup->flags & QUOTA2) != 0) {
			fprintf(fd, "\tinodes in use: %s, "
			    "limits (soft = %s, hard = %s",
			    intprt(qup->q2e.q2e_val[QL_FILE].q2v_cur,
				HN_NOSPACE, Hflag, 20),
			    intprt(qup->q2e.q2e_val[QL_FILE].q2v_softlimit,
				HN_NOSPACE, Hflag, 20),
			    intprt(qup->q2e.q2e_val[QL_FILE].q2v_hardlimit,
				 HN_NOSPACE, Hflag, 20));
			if (qup->flags & QUOTA2)
				fprintf(fd, ", ");
		} else
			fprintf(fd, "\tinodes: (");

		if (qup->flags & (QUOTA2|DEFAULT)) {
		    fprintf(fd, "grace = %s",
			timepprt(qup->q2e.q2e_val[QL_FILE].q2v_grace,
			    Hflag, 20));
		}
		fprintf(fd, ")\n");
	}
	fclose(fd);
	return (1);
}

/*
 * Merge changes to an ASCII file into a quotause list.
 */
int
readprivs(quplist, infd)
	struct quotause *quplist;
	int infd;
{
	struct quotause *qup;
	FILE *fd;
	int cnt;
	char fsp[BUFSIZ];
	static char line0[BUFSIZ], line1[BUFSIZ], line2[BUFSIZ];
	static char scurb[BUFSIZ], scuri[BUFSIZ], ssoft[BUFSIZ], shard[BUFSIZ];
	static char stime[BUFSIZ];
	uint64_t softb, hardb, softi, hardi;
	time_t graceb = -1, gracei = -1;
	int version;

	(void)lseek(infd, (off_t)0, SEEK_SET);
	fd = fdopen(dup(infd), "r");
	if (fd == NULL) {
		warn("Can't re-read temp file");
		return (0);
	}
	/*
	 * Discard title line, then read pairs of lines to process.
	 */
	(void) fgets(line1, sizeof (line1), fd);
	while (fgets(line0, sizeof (line0), fd) != NULL &&
	       fgets(line1, sizeof (line2), fd) != NULL &&
	       fgets(line2, sizeof (line2), fd) != NULL) {
		if (sscanf(line0, "%s (version %d):\n", fsp, &version) != 2) {
			warnx("%s: bad format", line0);
			goto out;
		}
#define last_char(str) ((str)[strlen(str) - 1])
		if (last_char(line1) != '\n') {
			warnx("%s:%s: bad format", fsp, line1);
			goto out;
		}
		last_char(line1) = '\0';
		if (last_char(line2) != '\n') {
			warnx("%s:%s: bad format", fsp, line2);
			goto out;
		}
		last_char(line2) = '\0';
		if (dflag && version == 1) {
			if (sscanf(line1,
			    "\tblocks: (grace = %s\n", stime) != 1) {
				warnx("%s:%s: bad format", fsp, line1);
				goto out;
			}
			if (last_char(stime) != ')') {
				warnx("%s:%s: bad format", fsp, line1);
				goto out;
			}
			last_char(stime) = '\0';
			if (timeprd(stime, &graceb) != 0) {
				warnx("%s:%s: bad number", fsp, stime);
				goto out;
			}
			if (sscanf(line2,
			    "\tinodes: (grace = %s\n", stime) != 1) {
				warnx("%s:%s: bad format", fsp, line2);
				goto out;
			}
			if (last_char(stime) != ')') {
				warnx("%s:%s: bad format", fsp, line2);
				goto out;
			}
			last_char(stime) = '\0';
			if (timeprd(stime, &gracei) != 0) {
				warnx("%s:%s: bad number", fsp, stime);
				goto out;
			}
		} else {
			cnt = sscanf(line1,
			    "\tblocks in use: %s limits (soft = %s hard = %s "
			    "grace = %s", scurb, ssoft, shard, stime);
			if (cnt == 3) {
				if (version != 1 ||
				    last_char(scurb) != ',' ||
				    last_char(ssoft) != ',' ||
				    last_char(shard) != ')') {
					warnx("%s:%s: bad format %d",
					    fsp, line1, cnt);
					goto out;
				}
				stime[0] = '\0';
			} else if (cnt == 4) {
				if (version < 2 ||
				    last_char(scurb) != ',' ||
				    last_char(ssoft) != ',' ||
				    last_char(shard) != ',' ||
				    last_char(stime) != ')') {
					warnx("%s:%s: bad format %d",
					    fsp, line1, cnt);
					goto out;
				}
			} else {
				warnx("%s: %s: bad format cnt %d", fsp, line1, cnt);
				goto out;
			}
			/* drop last char which is ',' or ')' */
			last_char(scurb) = '\0';
			last_char(ssoft) = '\0';
			last_char(shard) = '\0';
			last_char(stime) = '\0';
			
			if (intrd(ssoft, &softb, HN_B) != 0) {
				warnx("%s:%s: bad number", fsp, ssoft);
				goto out;
			}
			if (intrd(shard, &hardb, HN_B) != 0) {
				warnx("%s:%s: bad number", fsp, shard);
				goto out;
			}
			if (cnt == 4) {
				if (timeprd(stime, &graceb) != 0) {
					warnx("%s:%s: bad number", fsp, stime);
					goto out;
				}
			}

			cnt = sscanf(line2,
			    "\tinodes in use: %s limits (soft = %s hard = %s "
			    "grace = %s", scuri, ssoft, shard, stime);
			if (cnt == 3) {
				if (version != 1 ||
				    last_char(scuri) != ',' ||
				    last_char(ssoft) != ',' ||
				    last_char(shard) != ')') {
					warnx("%s:%s: bad format %d",
					    fsp, line2, cnt);
					goto out;
				}
				stime[0] = '\0';
			} else if (cnt == 4) {
				if (version < 2 ||
				    last_char(scuri) != ',' ||
				    last_char(ssoft) != ',' ||
				    last_char(shard) != ',' ||
				    last_char(stime) != ')') {
					warnx("%s:%s: bad format %d",
					    fsp, line2, cnt);
					goto out;
				}
			} else {
				warnx("%s: %s: bad format", fsp, line2);
				goto out;
			}
			/* drop last char which is ',' or ')' */
			last_char(scuri) = '\0';
			last_char(ssoft) = '\0';
			last_char(shard) = '\0';
			last_char(stime) = '\0';
			if (intrd(ssoft, &softi, 0) != 0) {
				warnx("%s:%s: bad number", fsp, ssoft);
				goto out;
			}
			if (intrd(shard, &hardi, 0) != 0) {
				warnx("%s:%s: bad number", fsp, shard);
				goto out;
			}
			if (cnt == 4) {
				if (timeprd(stime, &gracei) != 0) {
					warnx("%s:%s: bad number", fsp, stime);
					goto out;
				}
			}
		}
		for (qup = quplist; qup; qup = qup->next) {
			if (strcmp(fsp, qup->fsname))
				continue;
			if (version == 1 && dflag) {
				qup->q2e.q2e_val[QL_BLOCK].q2v_grace = graceb;
				qup->q2e.q2e_val[QL_FILE].q2v_grace = gracei;
				qup->flags |= FOUND;
				continue;
			}

			if (strcmp(intprt(qup->q2e.q2e_val[QL_BLOCK].q2v_cur,
			    HN_NOSPACE | HN_B, Hflag, 20),
			    scurb) != 0 ||
			    strcmp(intprt(qup->q2e.q2e_val[QL_FILE].q2v_cur,
			    HN_NOSPACE, Hflag, 20),
			    scuri) != 0) {
				warnx("%s: cannot change current allocation",
				    fsp);
				break;
			}
			/*
			 * Cause time limit to be reset when the quota
			 * is next used if previously had no soft limit
			 * or were under it, but now have a soft limit
			 * and are over it.
			 */
			if (qup->q2e.q2e_val[QL_BLOCK].q2v_cur &&
			    qup->q2e.q2e_val[QL_BLOCK].q2v_cur >= softb &&
			    (qup->q2e.q2e_val[QL_BLOCK].q2v_softlimit == 0 ||
			     qup->q2e.q2e_val[QL_BLOCK].q2v_cur <
			     qup->q2e.q2e_val[QL_BLOCK].q2v_softlimit))
				qup->q2e.q2e_val[QL_BLOCK].q2v_time = 0;
			if (qup->q2e.q2e_val[QL_FILE].q2v_cur &&
			    qup->q2e.q2e_val[QL_FILE].q2v_cur >= softi &&
			    (qup->q2e.q2e_val[QL_FILE].q2v_softlimit == 0 ||
			     qup->q2e.q2e_val[QL_FILE].q2v_cur <
			     qup->q2e.q2e_val[QL_FILE].q2v_softlimit))
				qup->q2e.q2e_val[QL_FILE].q2v_time = 0;
			qup->q2e.q2e_val[QL_BLOCK].q2v_softlimit = softb;
			qup->q2e.q2e_val[QL_BLOCK].q2v_hardlimit = hardb;
			if (version == 2)
				qup->q2e.q2e_val[QL_BLOCK].q2v_grace = graceb;
			qup->q2e.q2e_val[QL_FILE].q2v_softlimit  = softi;
			qup->q2e.q2e_val[QL_FILE].q2v_hardlimit  = hardi;
			if (version == 2)
				qup->q2e.q2e_val[QL_FILE].q2v_grace = gracei;
			qup->flags |= FOUND;
		}
	}
out:
	fclose(fd);
	/*
	 * Disable quotas for any filesystems that have not been found.
	 */
	for (qup = quplist; qup; qup = qup->next) {
		if (qup->flags & FOUND) {
			qup->flags &= ~FOUND;
			continue;
		}
		qup->q2e.q2e_val[QL_BLOCK].q2v_softlimit = UQUAD_MAX;
		qup->q2e.q2e_val[QL_BLOCK].q2v_hardlimit = UQUAD_MAX;
		qup->q2e.q2e_val[QL_BLOCK].q2v_grace = 0;
		qup->q2e.q2e_val[QL_FILE].q2v_softlimit = UQUAD_MAX;
		qup->q2e.q2e_val[QL_FILE].q2v_hardlimit = UQUAD_MAX;
		qup->q2e.q2e_val[QL_FILE].q2v_grace = 0;
	}
	return (1);
}

/*
 * Free a quotause structure.
 */
void
freeq(struct quotause *qup)
{
	if (qup->qfname)
		free(qup->qfname);
	free(qup);
}

/*
 * Free a list of quotause structures.
 */
void
freeprivs(quplist)
	struct quotause *quplist;
{
	struct quotause *qup, *nextqup;

	for (qup = quplist; qup; qup = nextqup) {
		nextqup = qup->next;
		freeq(qup);
	}
}

/*
 * Check whether a string is completely composed of digits.
 */
int
alldigits(s)
	const char *s;
{
	int c;

	c = *s++;
	do {
		if (!isdigit(c))
			return (0);
	} while ((c = *s++) != 0);
	return (1);
}

/*
 * Check to see if a particular legacy quota is to be enabled in fstab
 */
int
hasquota(fs, type, qfnamep)
	struct fstab *fs;
	int type;
	char **qfnamep;
{
	char *opt;
	char *cp;
	static char initname, usrname[100], grpname[100];
	char *buf;

	if (!initname) {
		sprintf(usrname, "%s%s", qfextension[USRQUOTA], qfname);
		sprintf(grpname, "%s%s", qfextension[GRPQUOTA], qfname);
		initname = 1;
	}
	buf =  fs->fs_mntops;
	cp = NULL;
	for (opt = strtok(buf, ","); opt; opt = strtok(NULL, ",")) {
		if ((cp = strchr(opt, '=')) != NULL)
			*cp++ = '\0';
		if (type == USRQUOTA && strcmp(opt, usrname) == 0)
			break;
		if (type == GRPQUOTA && strcmp(opt, grpname) == 0)
			break;
	}
	if (!opt) {
		*qfnamep = NULL;
		return (0);
	}
	if (cp) {
		*qfnamep = malloc(strlen(cp) + 1);
		if (*qfnamep == NULL)
			err(1, "malloc");
		strcpy(*qfnamep, cp);
		return (1);
	}
	*qfnamep = malloc(BUFSIZ);
	if (*qfnamep == NULL)
		err(1, "malloc");
	(void) sprintf(*qfnamep, "%s/%s.%s", fs->fs_file, qfname,
	    qfextension[type]);
	return (1);
}

static void
clearpriv(int argc, char **argv, const char *filesys, int quotatype)
{
	prop_array_t cmds, datas;
	prop_dictionary_t protodict, dict, data, cmd;
	struct plistref pref;
	bool ret;
	struct statvfs *fst;
	int nfst, i, error;
	int8_t error8;
	int id;

	/* build a generic command */
	protodict = quota2_prop_create();
	cmds = prop_array_create();
	datas = prop_array_create();
	if (protodict == NULL || cmds == NULL || datas == NULL) {
		errx(1, "can't allocate proplist");
	}

	for ( ; argc > 0; argc--, argv++) {
		if ((id = getentry(*argv, quotatype)) == -1)
			continue;
		data = prop_dictionary_create();
		if (data == NULL)
			errx(1, "can't allocate proplist");

		ret = prop_dictionary_set_uint32(data, "id", id);
		if (!ret)
			err(1, "prop_dictionary_set(id)");
		if (!prop_array_add_and_rel(datas, data))
			err(1, "prop_array_add(data)");
	}
	if (!quota2_prop_add_command(cmds, "clear", qfextension[quotatype],
	    datas))
		err(1, "prop_add_command");

	if (!prop_dictionary_set(protodict, "commands", cmds))
		err(1, "prop_dictionary_set(command)");

	/* now loop over quota-enabled filesystems */
	nfst = getmntinfo(&fst, MNT_WAIT);
	if (nfst == 0)
		errx(2, "no filesystems mounted!");

	for (i = 0; i < nfst; i++) {
		if (strcmp(fst[i].f_fstypename, "ffs") != 0 ||
		    (fst[i].f_flag & ST_QUOTA) == 0)
			continue;
		if (filesys && strcmp(fst[i].f_mntonname, filesys) != 0 &&
		    strcmp(fst[i].f_mntfromname, filesys) != 0)
			continue;
		if (Dflag) {
			fprintf(stderr, "message to kernel for %s:\n%s\n",
			    fst[i].f_mntonname,
			    prop_dictionary_externalize(protodict));
		}

		if (!prop_dictionary_send_syscall(protodict, &pref))
			err(1, "prop_dictionary_send_syscall");
		if (quotactl(fst[i].f_mntonname, &pref) != 0)
			err(1, "quotactl");

		if ((error = prop_dictionary_recv_syscall(&pref, &dict)) != 0) {
			errx(1, "prop_dictionary_recv_syscall: %s\n",
			    strerror(error));
		}

		if (Dflag) {
			fprintf(stderr, "reply from kernel for %s:\n%s\n",
			    fst[i].f_mntonname,
			    prop_dictionary_externalize(dict));
		}
		if ((error = quota2_get_cmds(dict, &cmds)) != 0) {
			errx(1, "quota2_get_cmds: %s\n",
			    strerror(error));
		}
		/* only one command, no need to iter */
		cmd = prop_array_get(cmds, 0);
		if (cmd == NULL)
			err(1, "prop_array_get(cmd)");

		if (!prop_dictionary_get_int8(cmd, "return", &error8))
			err(1, "prop_get(return)");
		if (error8) {
			fprintf(stderr, "clear %s quota entries on %s: %s\n",
			    qfextension[quotatype], fst[i].f_mntonname,
			    strerror(error8));
		}
		prop_object_release(dict);
	}
	prop_object_release(protodict);
}
