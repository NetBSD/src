/*      $NetBSD: edquota.c,v 1.51 2012/08/14 04:48:42 dholland Exp $ */
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
__RCSID("$NetBSD: edquota.c,v 1.51 2012/08/14 04:48:42 dholland Exp $");
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

#include <quota.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fstab.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "printquota.h"

#include "pathnames.h"

/*
 * XXX. Ideally we shouldn't compile this in, but it'll take some
 * reworking to avoid it and it'll be ok for now.
 */
#define EDQUOTA_NUMOBJTYPES	2

#if 0
static const char *quotagroup = QUOTAGROUP;
#endif

#define MAX_TMPSTR	(100+MAXPATHLEN)

enum sources {
	SRC_EDITED,	/* values came from user */
	SRC_QUOTA,	/* values came from a specific quota entry */
	SRC_DEFAULT,	/* values were copied from the default quota entry */
	SRC_CLEAR,	/* values arose by zeroing out a quota entry */
};

struct quotause {
	struct	quotause *next;
	unsigned found:1,	/* found after running editor */
		xgrace:1,	/* grace periods are per-id */
		isdefault:1;

	struct	quotaval qv[EDQUOTA_NUMOBJTYPES];
	enum sources source[EDQUOTA_NUMOBJTYPES];
	char	fsname[MAXPATHLEN + 1];
	char	implementation[32];
};

struct quotalist {
	struct quotause *head;
	struct quotause *tail;
	char *idtypename;
};

static void	usage(void) __dead;

static int Hflag = 0;

/* more compact form of constants */
#define QO_BLK QUOTA_OBJTYPE_BLOCKS
#define QO_FL  QUOTA_OBJTYPE_FILES

////////////////////////////////////////////////////////////
// support code

/*
 * This routine converts a name for a particular quota class to
 * an identifier. This routine must agree with the kernel routine
 * getinoquota as to the interpretation of quota classes.
 */
static int
getidbyname(const char *name, int idtype)
{
	struct passwd *pw;
	struct group *gr;

	if (alldigits(name))
		return atoi(name);
	switch (idtype) {
	case QUOTA_IDTYPE_USER:
		if ((pw = getpwnam(name)) != NULL)
			return pw->pw_uid;
		warnx("%s: no such user", name);
		break;
	case QUOTA_IDTYPE_GROUP:
		if ((gr = getgrnam(name)) != NULL)
			return gr->gr_gid;
		warnx("%s: no such group", name);
		break;
	default:
		warnx("%d: unknown quota type", idtype);
		break;
	}
	sleep(1);
	return -1;
}

/*
 * check if a source is "real" (reflects actual data) or not
 */
static bool
source_is_real(enum sources source)
{
	switch (source) {
	    case SRC_EDITED:
	    case SRC_QUOTA:
		return true;
	    case SRC_DEFAULT:
	    case SRC_CLEAR:
		return false;
	}
	assert(!"encountered invalid source");
	return false;
}

/*
 * some simple string tools
 */

static /*const*/ char *
skipws(/*const*/ char *s)
{
	while (*s == ' ' || *s == '\t') {
		s++;
	}
	return s;
}

static /*const*/ char *
skipword(/*const*/ char *s)
{
	while (*s != '\0' && *s != '\n' && *s != ' ' && *s != '\t') {
		s++;
	}
	return s;
}

////////////////////////////////////////////////////////////
// quotause operations

/*
 * Create an empty quotause structure.
 */
static struct quotause *
quotause_create(void)
{
	struct quotause *qup;
	unsigned i;

	qup = malloc(sizeof(*qup));
	if (qup == NULL) {
		err(1, "malloc");
	}

	qup->next = NULL;
	qup->found = 0;
	qup->xgrace = 0;
	qup->isdefault = 0;
	for (i=0; i<EDQUOTA_NUMOBJTYPES; i++) {
		quotaval_clear(&qup->qv[i]);
		qup->source[i] = SRC_CLEAR;
	}
	qup->fsname[0] = '\0';

	return qup;
}

/*
 * Free a quotause structure.
 */
static void
quotause_destroy(struct quotause *qup)
{
	free(qup);
}

////////////////////////////////////////////////////////////
// quotalist operations

/*
 * Create a quotause list.
 */
static struct quotalist *
quotalist_create(void)
{
	struct quotalist *qlist;

	qlist = malloc(sizeof(*qlist));
	if (qlist == NULL) {
		err(1, "malloc");
	}

	qlist->head = NULL;
	qlist->tail = NULL;
	qlist->idtypename = NULL;

	return qlist;
}

/*
 * Free a list of quotause structures.
 */
static void
quotalist_destroy(struct quotalist *qlist)
{
	struct quotause *qup, *nextqup;

	for (qup = qlist->head; qup; qup = nextqup) {
		nextqup = qup->next;
		quotause_destroy(qup);
	}
	free(qlist->idtypename);
	free(qlist);
}

#if 0
static bool
quotalist_empty(struct quotalist *qlist)
{
	return qlist->head == NULL;
}
#endif

static void
quotalist_append(struct quotalist *qlist, struct quotause *qup)
{
	/* should not already be on a list */
	assert(qup->next == NULL);

	if (qlist->head == NULL) {
		qlist->head = qup;
	} else {
		qlist->tail->next = qup;
	}
	qlist->tail = qup;
}

////////////////////////////////////////////////////////////
// ffs quota v1

#if 0
static void
putprivs1(uint32_t id, int idtype, struct quotause *qup)
{
	struct dqblk dqblk;
	int fd;

	quotavals_to_dqblk(&qup->qv[QUOTA_LIMIT_BLOCK],
			   &qup->qv[QUOTA_LIMIT_FILE],
			   &dqblk);
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

static struct quotause *
getprivs1(long id, int idtype, const char *filesys)
{
	struct fstab *fs;
	char qfpathname[MAXPATHLEN], xbuf[MAXPATHLEN];
	const char *fsspec;
	struct quotause *qup;
	struct dqblk dqblk;
	int fd;

	setfsent();
	while ((fs = getfsent()) != NULL) {
		if (strcmp(fs->fs_vfstype, "ffs"))
			continue;
		fsspec = getfsspecname(xbuf, sizeof(xbuf), fs->fs_spec);
		if (fsspec == NULL) {
			warn("%s", xbuf);
			continue;
		}
		if (strcmp(fsspec, filesys) == 0 ||
		    strcmp(fs->fs_file, filesys) == 0)
			break;
	}
	if (fs == NULL)
		return NULL;

	if (!hasquota(qfpathname, sizeof(qfpathname), fs,
	    quota_idtype_to_ufs(idtype)))
		return NULL;

	qup = quotause_create();
	strcpy(qup->fsname, fs->fs_file);
	if ((fd = open(qfpathname, O_RDONLY)) < 0) {
		fd = open(qfpathname, O_RDWR|O_CREAT, 0640);
		if (fd < 0 && errno != ENOENT) {
			warnx("open `%s'", qfpathname);
			quotause_destroy(qup);
			return NULL;
		}
		warnx("Creating quota file %s", qfpathname);
		sleep(3);
		(void)fchown(fd, getuid(),
		    getidbyname(quotagroup, QUOTA_CLASS_GROUP));
		(void)fchmod(fd, 0640);
	}
	(void)lseek(fd, (off_t)(id * sizeof(struct dqblk)),
	    SEEK_SET);
	switch (read(fd, &dqblk, sizeof(struct dqblk))) {
	case 0:			/* EOF */
		/*
		 * Convert implicit 0 quota (EOF)
		 * into an explicit one (zero'ed dqblk)
		 */
		memset(&dqblk, 0, sizeof(struct dqblk));
		break;

	case sizeof(struct dqblk):	/* OK */
		break;

	default:		/* ERROR */
		warn("read error in `%s'", qfpathname);
		close(fd);
		quotause_destroy(qup);
		return NULL;
	}
	close(fd);
	qup->qfname = qfpathname;
	endfsent();
	dqblk_to_quotavals(&dqblk,
			   &qup->qv[QUOTA_LIMIT_BLOCK],
			   &qup->qv[QUOTA_LIMIT_FILE]);
	return qup;
}
#endif

////////////////////////////////////////////////////////////
// generic quota interface

static int
dogetprivs2(struct quotahandle *qh, int idtype, id_t id, int defaultq,
	    int objtype, struct quotause *qup)
{
	struct quotakey qk;

	qk.qk_idtype = idtype;
	qk.qk_id = defaultq ? QUOTA_DEFAULTID : id;
	qk.qk_objtype = objtype;
	if (quota_get(qh, &qk, &qup->qv[objtype]) == 0) {
		/* succeeded */
		qup->source[objtype] = SRC_QUOTA;
		return 0;
	}
	if (errno != ENOENT) {
		/* serious failure */
		return -1;
	}

	/* no entry, get default entry */
	qk.qk_id = QUOTA_DEFAULTID;
	if (quota_get(qh, &qk, &qup->qv[objtype]) == 0) {
		/* succeeded */
		qup->source[objtype] = SRC_DEFAULT;
		return 0;
	}
	if (errno != ENOENT) {
		return -1;
	}

	/* use a zeroed-out entry */
	quotaval_clear(&qup->qv[objtype]);
	qup->source[objtype] = SRC_CLEAR;
	return 0;
}

static struct quotause *
getprivs2(long id, int idtype, const char *filesys, int defaultq,
	  char **idtypename_p)
{
	struct quotause *qup;
	struct quotahandle *qh;
	const char *impl;
	unsigned restrictions;
	const char *idtypename;
	int serrno;

	qup = quotause_create();
	strcpy(qup->fsname, filesys);
	if (defaultq)
		qup->isdefault = 1;

	qh = quota_open(filesys);
	if (qh == NULL) {
		serrno = errno;
		quotause_destroy(qup);
		errno = serrno;
		return NULL;
	}

	impl = quota_getimplname(qh);
	if (impl == NULL) {
		impl = "???";
	}
	strlcpy(qup->implementation, impl, sizeof(qup->implementation));

	restrictions = quota_getrestrictions(qh);
	if ((restrictions & QUOTA_RESTRICT_UNIFORMGRACE) == 0) {
		qup->xgrace = 1;
	}

	if (*idtypename_p == NULL) {
		idtypename = quota_idtype_getname(qh, idtype);
		*idtypename_p = strdup(idtypename);
		if (*idtypename_p == NULL) {
			errx(1, "Out of memory");
		}
	}

	if (dogetprivs2(qh, idtype, id, defaultq, QUOTA_OBJTYPE_BLOCKS, qup)) {
		serrno = errno;
		quota_close(qh);
		quotause_destroy(qup);
		errno = serrno;
		return NULL;
	}

	if (dogetprivs2(qh, idtype, id, defaultq, QUOTA_OBJTYPE_FILES, qup)) {
		serrno = errno;
		quota_close(qh);
		quotause_destroy(qup);
		errno = serrno;
		return NULL;
	}

	quota_close(qh);

	return qup;
}

static void
putprivs2(uint32_t id, int idtype, struct quotause *qup)
{
	struct quotahandle *qh;
	struct quotakey qk;
	char idname[32];

	if (qup->isdefault) {
		snprintf(idname, sizeof(idname), "%s default",
			 idtype == QUOTA_IDTYPE_USER ? "user" : "group");
		id = QUOTA_DEFAULTID;
	} else {
		snprintf(idname, sizeof(idname), "%s %u",
			 idtype == QUOTA_IDTYPE_USER ? "uid" : "gid", id);
	}

	qh = quota_open(qup->fsname);
	if (qh == NULL) {
		err(1, "%s: quota_open", qup->fsname);
	}

	if (source_is_real(qup->source[QUOTA_OBJTYPE_BLOCKS])) {
		qk.qk_idtype = idtype;
		qk.qk_id = id;
		qk.qk_objtype = QUOTA_OBJTYPE_BLOCKS;
		if (quota_put(qh, &qk, &qup->qv[QO_BLK])) {
			err(1, "%s: quota_put (%s blocks)", qup->fsname,
			    idname);
		}
	}

	if (source_is_real(qup->source[QUOTA_OBJTYPE_FILES])) {
		qk.qk_idtype = idtype;
		qk.qk_id = id;
		qk.qk_objtype = QUOTA_OBJTYPE_FILES;
		if (quota_put(qh, &qk, &qup->qv[QO_FL])) {
			err(1, "%s: quota_put (%s files)", qup->fsname,
			    idname);
		}
	}

	quota_close(qh);
}

////////////////////////////////////////////////////////////
// quota format switch

/*
 * Collect the requested quota information.
 */
static struct quotalist *
getprivs(long id, int defaultq, int idtype, const char *filesys)
{
	struct statvfs *fst;
	int nfst, i;
	struct quotalist *qlist;
	struct quotause *qup;
	int seenany = 0;

	qlist = quotalist_create();

	nfst = getmntinfo(&fst, MNT_WAIT);
	if (nfst == 0)
		errx(1, "no filesystems mounted!");

	for (i = 0; i < nfst; i++) {
		if ((fst[i].f_flag & ST_QUOTA) == 0)
			continue;
		seenany = 1;
		if (filesys &&
		    strcmp(fst[i].f_mntonname, filesys) != 0 &&
		    strcmp(fst[i].f_mntfromname, filesys) != 0)
			continue;
		qup = getprivs2(id, idtype, fst[i].f_mntonname, defaultq,
				&qlist->idtypename);
		if (qup == NULL) {
			warn("Reading quotas failed for id %ld", id);
			continue;
		}

		quotalist_append(qlist, qup);
	}

	if (!seenany) {
		errx(1, "No mounted filesystems have quota support");
	}

#if 0
	if (filesys && quotalist_empty(qlist)) {
		if (defaultq)
			errx(1, "no default quota for version 1");
		/* if we get there, filesys is not mounted. try the old way */
		qup = getprivs1(id, idtype, filesys);
		if (qup == NULL) {
			warnx("getprivs1 failed");
			return qlist;
		}
		quotalist_append(qlist, qup);
	}
#endif
	return qlist;
}

/*
 * Store the requested quota information.
 */
static void
putprivs(uint32_t id, int idtype, struct quotalist *qlist)
{
	struct quotause *qup;

        for (qup = qlist->head; qup; qup = qup->next) {
#if 0
		if (qup->qfname != NULL)
			putprivs1(id, idtype, qup);
		else
#endif
			putprivs2(id, idtype, qup);
	}
}

static void
clearpriv(int argc, char **argv, const char *filesys, int idtype)
{
	struct statvfs *fst;
	int nfst, i;
	int id;
	id_t *ids;
	unsigned nids, maxids, j;
	struct quotahandle *qh;
	struct quotakey qk;
	char idname[32];

	maxids = 4;
	nids = 0;
	ids = malloc(maxids * sizeof(ids[0]));
	if (ids == NULL) {
		err(1, "malloc");
	}

	for ( ; argc > 0; argc--, argv++) {
		if ((id = getidbyname(*argv, idtype)) == -1)
			continue;

		if (nids + 1 > maxids) {
			maxids *= 2;
			ids = realloc(ids, maxids * sizeof(ids[0]));
			if (ids == NULL) {
				err(1, "realloc");
			}
		}
		ids[nids++] = id;
	}

	/* now loop over quota-enabled filesystems */
	nfst = getmntinfo(&fst, MNT_WAIT);
	if (nfst == 0)
		errx(1, "no filesystems mounted!");

	for (i = 0; i < nfst; i++) {
		if ((fst[i].f_flag & ST_QUOTA) == 0)
			continue;
		if (filesys && strcmp(fst[i].f_mntonname, filesys) != 0 &&
		    strcmp(fst[i].f_mntfromname, filesys) != 0)
			continue;

		qh = quota_open(fst[i].f_mntonname);
		if (qh == NULL) {
			err(1, "%s: quota_open", fst[i].f_mntonname);
		}

		for (j = 0; j < nids; j++) {
			snprintf(idname, sizeof(idname), "%s %u",
				 idtype == QUOTA_IDTYPE_USER ? 
				 "uid" : "gid", ids[j]);

			qk.qk_idtype = idtype;
			qk.qk_id = ids[j];
			qk.qk_objtype = QUOTA_OBJTYPE_BLOCKS;
			if (quota_delete(qh, &qk)) {
				err(1, "%s: quota_delete (%s blocks)",
				    fst[i].f_mntonname, idname);
			}

			qk.qk_idtype = idtype;
			qk.qk_id = ids[j];
			qk.qk_objtype = QUOTA_OBJTYPE_FILES;
			if (quota_delete(qh, &qk)) {
				if (errno == ENOENT) {
 					/*
					 * XXX ignore this case; due
					 * to a weakness in the quota
					 * proplib interface it can
					 * appear spuriously.
					 */
				} else {
					err(1, "%s: quota_delete (%s files)",
					    fst[i].f_mntonname, idname);
				}
			}
		}

		quota_close(qh);
	}

	free(ids);
}

////////////////////////////////////////////////////////////
// editor

/*
 * Take a list of privileges and get it edited.
 */
static int
editit(const char *ltmpfile)
{
	pid_t pid;
	int lst;
	char p[MAX_TMPSTR];
	const char *ed;
	sigset_t s, os;

	sigemptyset(&s);
	sigaddset(&s, SIGINT);
	sigaddset(&s, SIGQUIT);
	sigaddset(&s, SIGHUP);
	if (sigprocmask(SIG_BLOCK, &s, &os) == -1)
		err(1, "sigprocmask");
top:
	switch ((pid = fork())) {
	case -1:
		if (errno == EPROCLIM) {
			warnx("You have too many processes");
			return 0;
		}
		if (errno == EAGAIN) {
			sleep(1);
			goto top;
		}
		warn("fork");
		return 0;
	case 0:
		if (sigprocmask(SIG_SETMASK, &os, NULL) == -1)
			err(1, "sigprocmask");
		setgid(getgid());
		setuid(getuid());
		if ((ed = getenv("EDITOR")) == (char *)0)
			ed = _PATH_VI;
		if (strlen(ed) + strlen(ltmpfile) + 2 >= MAX_TMPSTR) {
			errx(1, "%s", "editor or filename too long");
		}
		snprintf(p, sizeof(p), "%s %s", ed, ltmpfile);
		execlp(_PATH_BSHELL, _PATH_BSHELL, "-c", p, NULL);
		err(1, "%s", ed);
	default:
		if (waitpid(pid, &lst, 0) == -1)
			err(1, "waitpid");
		if (sigprocmask(SIG_SETMASK, &os, NULL) == -1)
			err(1, "sigprocmask");
		if (!WIFEXITED(lst) || WEXITSTATUS(lst) != 0)
			return 0;
		return 1;
	}
}

/*
 * Convert a quotause list to an ASCII file.
 */
static int
writeprivs(struct quotalist *qlist, int outfd, const char *name,
    int idtype, const char *idtypename)
{
	struct quotause *qup;
	FILE *fd;
	char b0[32], b1[32], b2[32], b3[32];
	const char *comm;

	(void)ftruncate(outfd, 0);
	(void)lseek(outfd, (off_t)0, SEEK_SET);
	if ((fd = fdopen(dup(outfd), "w")) == NULL)
		errx(1, "fdopen");
	if (name == NULL) {
		fprintf(fd, "Default %s quotas:\n", idtypename);
	} else {
		fprintf(fd, "Quotas for %s %s:\n", idtypename, name);
	}
	for (qup = qlist->head; qup; qup = qup->next) {
		struct quotaval *q = qup->qv;

		fprintf(fd, "%s (%s):\n", qup->fsname, qup->implementation);

		comm = source_is_real(qup->source[QO_BLK]) ? "" : "#";
		fprintf(fd, "\tblocks:%s\n",
			Hflag ? "" : " (sizes in 1K-blocks)");
		fprintf(fd, "\t\t%susage: %s\n", comm,
			intprt(b1, 21, q[QO_BLK].qv_usage,
			       HN_NOSPACE | HN_B, Hflag));
		fprintf(fd, "\t\t%slimits: soft %s, hard %s\n", comm,
			intprt(b2, 21, q[QO_BLK].qv_softlimit,
			       HN_NOSPACE | HN_B, Hflag),
			intprt(b3, 21, q[QO_BLK].qv_hardlimit,
			       HN_NOSPACE | HN_B, Hflag));
		if (qup->xgrace || qup->isdefault) {
			fprintf(fd, "\t\t%sgrace: %s\n", comm,
				timepprt(b0, 21, q[QO_BLK].qv_grace, Hflag));
		}

		comm = source_is_real(qup->source[QO_FL]) ? "" : "#";
		fprintf(fd, "\tinodes:\n");
		fprintf(fd, "\t\t%susage: %s\n", comm,
			intprt(b1, 21, q[QO_FL].qv_usage,
			       HN_NOSPACE, Hflag));
		fprintf(fd, "\t\t%slimits: soft %s, hard %s\n", comm,
			intprt(b2, 21, q[QO_FL].qv_softlimit,
			       HN_NOSPACE, Hflag),
			intprt(b3, 21, q[QO_FL].qv_hardlimit,
			       HN_NOSPACE, Hflag));
		if (qup->xgrace || qup->isdefault) {
			fprintf(fd, "\t\t%sgrace: %s\n", comm,
				timepprt(b0, 21, q[QO_FL].qv_grace, Hflag));
		}
	}
	fclose(fd);
	return 1;
}

/*
 * Merge changes to an ASCII file into a quotause list.
 */
static int
readprivs(struct quotalist *qlist, int infd, int dflag)
{
	FILE *fd;			/* file */
	unsigned lineno;		/* line number in file */
	char line[BUFSIZ];		/* input buffer */
	size_t len;			/* length of input buffer */
	bool seenheader;		/* true if past the file header */
	struct quotause *qup;		/* current filesystem */
	bool haveobjtype;		/* true if objtype is valid */
	unsigned objtype;		/* current object type */
	int objtypeflags;		/* humanize flags */
	/* XXX make following const later (requires non-local cleanup) */
	/*const*/ char *text, *text2, *t; /* scratch values */
	char b0[32], b1[32];
	uint64_t soft, hard, current;
	time_t grace;
	struct quotaval *qv;

	lineno = 0;
	seenheader = false;
	qup = NULL;
	haveobjtype = false;
	objtype = QUOTA_OBJTYPE_BLOCKS; /* for gcc 4.5 */
	objtypeflags = HN_B;

	(void)lseek(infd, (off_t)0, SEEK_SET);
	fd = fdopen(dup(infd), "r");
	if (fd == NULL) {
		warn("Can't re-read temp file");
		return -1;
	}

	/*
	 * Read back the same format we wrote.
	 */

	while (fgets(line, sizeof(line), fd)) {
		lineno++;
		if (!seenheader) {
			if ((!strncmp(line, "Default ", 8) && dflag) ||
			    (!strncmp(line, "Quotas for ", 11) && !dflag)) {
				/* ok. */
				seenheader = 1;
				continue;
			}
			warnx("Header line missing");
			goto fail;
		}
		len = strlen(line);
		if (len == 0) {
			/* ? */
			continue;
		}
		if (line[len - 1] != '\n') {
			warnx("Line %u too long", lineno);
			goto fail;
		}
		line[--len] = '\0';
		if (line[len - 1] == '\r') {
			line[--len] = '\0';
		}
		if (len == 0) {
			/* blank line */
			continue;
		}

		/*
		 * If the line has:     it is:
                 *      two tabs        values
		 *      one tab         the next objtype
		 *      no tabs         the next filesystem
		 */
		if (line[0] == '\t' && line[1] == '\t') {
			if (qup == NULL) {
				warnx("Line %u: values with no filesystem",
				      lineno);
				goto fail;
			}
			if (!haveobjtype) {
				warnx("Line %u: values with no object type",
				      lineno);
				goto fail;
			}
			qv = &qup->qv[objtype];

			text = line + 2;
			if (*text == '#') {
				/* commented out; ignore */
				continue;
			}
			else if (!strncmp(text, "usage:", 6)) {

				/* usage: %llu */
				text += 6;
				t = skipws(text);
				if (intrd(t, &current, objtypeflags) != 0) {
					warnx("Line %u: Bad number %s",
					      lineno, t);
					goto fail;
				}

				/*
				 * Because the humanization can lead
				 * to roundoff, check if the two
				 * values produce the same humanized
				 * string, rather than if they're the
				 * same number. Sigh.
				 */
				intprt(b0, 21, current,
				       HN_NOSPACE | objtypeflags, Hflag);
				intprt(b1, 21, qv->qv_usage,
				       HN_NOSPACE | objtypeflags, Hflag);
				if (strcmp(b0, b1)) {
					warnx("Line %u: cannot change usage",
					      lineno);
				}
				continue;

			} else if (!strncmp(text, "limits:", 7)) {

				/* limits: soft %llu, hard %llu */
				text += 7;
				text2 = strchr(text, ',');
				if (text2 == NULL) {
					warnx("Line %u: expected comma",
					      lineno);
					goto fail;
				}
				*text2 = '\0';
				text2++;

				t = skipws(text);
				t = skipword(t);
				t = skipws(t);
				if (intrd(t, &soft, objtypeflags) != 0) {
					warnx("Line %u: Bad number %s",
					      lineno, t);
					goto fail;
				}
				t = skipws(text2);
				t = skipword(t);
				t = skipws(t);
				if (intrd(t, &hard, objtypeflags) != 0) {
					warnx("Line %u: Bad number %s",
					      lineno, t);
					goto fail;
				}

				/*
				 * Cause time limit to be reset when the quota
				 * is next used if previously had no soft limit
				 * or were under it, but now have a soft limit
				 * and are over it.
				 */
				if (qv->qv_usage && qv->qv_usage >= soft &&
				    (qv->qv_softlimit == 0 ||
				     qv->qv_usage < qv->qv_softlimit)) {
					qv->qv_expiretime = 0;
				}
				if (soft != qv->qv_softlimit ||
				    hard != qv->qv_hardlimit) {
					qv->qv_softlimit = soft;
					qv->qv_hardlimit = hard;
					qup->source[objtype] = SRC_EDITED;
				}
				qup->found = 1;

			} else if (!strncmp(text, "grace:", 6)) {

				text += 6;
				/* grace: %llu */
				t = skipws(text);
				if (timeprd(t, &grace) != 0) {
					warnx("Line %u: Bad number %s",
					      lineno, t);
					goto fail;
				}
				if (qup->isdefault || qup->xgrace) {
					if (grace != qv->qv_grace) {
						qv->qv_grace = grace;
						qup->source[objtype] =
							SRC_EDITED;
					}
					qup->found = 1;
				} else {
					warnx("Line %u: Cannot set individual "
					      "grace time on this filesystem",
					      lineno);
					goto fail;
				}

			} else {
				warnx("Line %u: Unknown/unexpected value line",
				      lineno);
				goto fail;
			}
		} else if (line[0] == '\t') {
			text = line + 1;
			if (*text == '#') {
				/* commented out; ignore */
				continue;
			}
			else if (!strncmp(text, "blocks:", 7)) {
				objtype = QUOTA_OBJTYPE_BLOCKS;
				objtypeflags = HN_B;
				haveobjtype = true;
			} else if (!strncmp(text, "inodes:", 7)) {
				objtype = QUOTA_OBJTYPE_FILES;
				objtypeflags = 0;
				haveobjtype = true;
			} else {
				warnx("Line %u: Unknown/unexpected object "
				      "type (%s)", lineno, text);
				goto fail;
			}
		} else {
			t = strchr(line, ' ');
			if (t == NULL) {
				t = strchr(line, ':');
				if (t == NULL) {
					t = line + len;
				}
			}
			*t = '\0';

			if (*line == '#') {
				/* commented out; ignore */
				continue;
			}

			for (qup = qlist->head; qup; qup = qup->next) {
				if (!strcmp(line, qup->fsname))
					break;
			}
			if (qup == NULL) {
				warnx("Line %u: Filesystem %s invalid or has "
				      "no quota support", lineno, line);
				goto fail;
			}
			haveobjtype = false;
		}
	}

	fclose(fd);

	/*
	 * Disable quotas for any filesystems that we didn't see,
	 * because they must have been deleted in the editor.
	 *
	 * XXX this should be improved so it results in
	 * quota_delete(), not just writing out a blank quotaval.
	 */
	for (qup = qlist->head; qup; qup = qup->next) {
		if (qup->found) {
			qup->found = 0;
			continue;
		}

		if (source_is_real(qup->source[QUOTA_OBJTYPE_BLOCKS])) {
			quotaval_clear(&qup->qv[QUOTA_OBJTYPE_BLOCKS]);
			qup->source[QUOTA_OBJTYPE_BLOCKS] = SRC_EDITED;
		}

		if (source_is_real(qup->source[QUOTA_OBJTYPE_FILES])) {
			quotaval_clear(&qup->qv[QUOTA_OBJTYPE_FILES]);
			qup->source[QUOTA_OBJTYPE_FILES] = SRC_EDITED;
		}
	}
	return 0;

fail:
	sleep(3);
	fclose(fd);
	return -1;
}

////////////////////////////////////////////////////////////
// actions

static void
replicate(const char *fs, int idtype, const char *protoname,
	  char **names, int numnames)
{
	long protoid, id;
	struct quotalist *protoprivs;
	struct quotause *qup;
	int i;

	if ((protoid = getidbyname(protoname, idtype)) == -1)
		exit(1);
	protoprivs = getprivs(protoid, 0, idtype, fs);
	for (qup = protoprivs->head; qup; qup = qup->next) {
		qup->qv[QO_BLK].qv_expiretime = 0;
		qup->qv[QO_FL].qv_expiretime = 0;
		qup->source[QO_BLK] = SRC_EDITED;
		qup->source[QO_FL] = SRC_EDITED;
	}
	for (i=0; i<numnames; i++) {
		id = getidbyname(names[i], idtype);
		if (id == -1)
			continue;
		putprivs(id, idtype, protoprivs);
	}
	/* XXX */
	/* quotalist_destroy(protoprivs); */
}

static void
assign(const char *fs, int idtype,
       char *soft, char *hard, char *grace,
       char **names, int numnames)
{
	struct quotalist *curprivs;
	struct quotause *lqup;
	u_int64_t softb, hardb, softi, hardi;
	time_t  graceb, gracei;
	char *str;
	long id;
	int dflag;
	int i;

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
	for (i=0; i<numnames; i++) {
		if (names[i] == NULL) {
			id = 0;
			dflag = 1;
		} else {
			id = getidbyname(names[i], idtype);
			if (id == -1)
				continue;
			dflag = 0;
		}

		curprivs = getprivs(id, dflag, idtype, fs);
		for (lqup = curprivs->head; lqup; lqup = lqup->next) {
			struct quotaval *q = lqup->qv;
			if (soft) {
				if (!dflag && softb &&
				    q[QO_BLK].qv_usage >= softb &&
				    (q[QO_BLK].qv_softlimit == 0 ||
				     q[QO_BLK].qv_usage <
				     q[QO_BLK].qv_softlimit))
					q[QO_BLK].qv_expiretime = 0;
				if (!dflag && softi &&
				    q[QO_FL].qv_usage >= softb &&
				    (q[QO_FL].qv_softlimit == 0 ||
				     q[QO_FL].qv_usage <
				     q[QO_FL].qv_softlimit))
					q[QO_FL].qv_expiretime = 0;
				q[QO_BLK].qv_softlimit = softb;
				q[QO_FL].qv_softlimit = softi;
			}
			if (hard) {
				q[QO_BLK].qv_hardlimit = hardb;
				q[QO_FL].qv_hardlimit = hardi;
			}
			if (grace) {
				q[QO_BLK].qv_grace = graceb;
				q[QO_FL].qv_grace = gracei;
			}
			lqup->source[QO_BLK] = SRC_EDITED;
			lqup->source[QO_FL] = SRC_EDITED;
		}
		putprivs(id, idtype, curprivs);
		quotalist_destroy(curprivs);
	}
}

static void
clear(const char *fs, int idtype, char **names, int numnames)
{
	clearpriv(numnames, names, fs, idtype);
}

static void
editone(const char *fs, int idtype, const char *name,
	int tmpfd, const char *tmppath)
{
	struct quotalist *curprivs;
	long id;
	int dflag;

	if (name == NULL) {
		id = 0;
		dflag = 1;
	} else {
		id = getidbyname(name, idtype);
		if (id == -1)
			return;
		dflag = 0;
	}
	curprivs = getprivs(id, dflag, idtype, fs);

	if (writeprivs(curprivs, tmpfd, name, idtype,
		       curprivs->idtypename) == 0)
		goto fail;

	if (editit(tmppath) == 0)
		goto fail;

	if (readprivs(curprivs, tmpfd, dflag) < 0)
		goto fail;

	putprivs(id, idtype, curprivs);
fail:
	quotalist_destroy(curprivs);
}

static void
edit(const char *fs, int idtype, char **names, int numnames)
{
	char tmppath[] = _PATH_TMPFILE;
	int tmpfd, i;

	tmpfd = mkstemp(tmppath);
	fchown(tmpfd, getuid(), getgid());

	for (i=0; i<numnames; i++) {
		editone(fs, idtype, names[i], tmpfd, tmppath);
	}

	close(tmpfd);
	unlink(tmppath);
}

////////////////////////////////////////////////////////////
// main

static void
usage(void)
{
	const char *p = getprogname();
	fprintf(stderr,
	    "Usage: %s [-D] [-H] [-u] [-p <username>] [-f <filesystem>] "
		"-d | <username> ...\n"
	    "\t%s [-D] [-H] -g [-p <groupname>] [-f <filesystem>] "
		"-d | <groupname> ...\n"
	    "\t%s [-D] [-u] [-f <filesystem>] [-s b#/i#] [-h b#/i#] [-t t#/t#] "
		"-d | <username> ...\n"
	    "\t%s [-D] -g [-f <filesystem>] [-s b#/i#] [-h b#/i#] [-t t#/t#] "
		"-d | <groupname> ...\n"
	    "\t%s [-D] [-H] [-u] -c [-f <filesystem>] username ...\n"
	    "\t%s [-D] [-H] -g -c [-f <filesystem>] groupname ...\n",
	    p, p, p, p, p, p);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int idtype;
	char *protoname;
	char *soft = NULL, *hard = NULL, *grace = NULL;
	char *fs = NULL;
	int ch;
	int pflag = 0;
	int cflag = 0;
	int dflag = 0;

	if (argc < 2)
		usage();
	if (getuid())
		errx(1, "permission denied");
	protoname = NULL;
	idtype = QUOTA_IDTYPE_USER;
	while ((ch = getopt(argc, argv, "Hcdugp:s:h:t:f:")) != -1) {
		switch(ch) {
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
			idtype = QUOTA_IDTYPE_GROUP;
			break;
		case 'u':
			idtype = QUOTA_IDTYPE_USER;
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
		replicate(fs, idtype, protoname, argv, argc);
	} else if (soft || hard || grace) {
		if (cflag)
			usage();
		if (dflag) {
			/* use argv[argc], which is null, to mean 'default' */
			argc++;
		}
		assign(fs, idtype, soft, hard, grace, argv, argc);
	} else if (cflag) {
		if (dflag)
			usage();
		clear(fs, idtype, argv, argc);
	} else {
		if (dflag) {
			/* use argv[argc], which is null, to mean 'default' */
			argc++;
		}
		edit(fs, idtype, argv, argc);
	}
	return 0;
}
