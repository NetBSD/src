/*	$NetBSD: quota.c,v 1.50 2014/07/13 01:46:04 dholland Exp $	*/

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
static char sccsid[] = "@(#)quota.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: quota.c,v 1.50 2014/07/13 01:46:04 dholland Exp $");
#endif
#endif /* not lint */

/*
 * Disk quota reporting program.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/socket.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fstab.h>
#include <grp.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <quota.h>

#include "printquota.h"

struct quotause {
	struct	quotause *next;
	uid_t	id;
	struct	quotaval *qvs;
	unsigned numqvs;
	char	fsname[MAXPATHLEN + 1];
	struct	quotahandle *qh;
};

static int	anyusage(struct quotaval *, unsigned);
static int	anyover(struct quotaval *, unsigned, time_t);
static const char *getovermsg(struct quotaval *, const char *, time_t);
static struct quotause	*getprivs(id_t, int);
static void	heading(int, const char *, id_t, const char *, const char *);
static int	isover(struct quotaval *qv, time_t now);
static void	printqv(struct quotaval *, int, time_t);
static void	showgid(gid_t);
static void	showgrpname(const char *);
static void	showonequota(int, const char *, id_t, const char *,
			     struct quotause *);
static void	showquotas(int, const char *, id_t, const char *);
static void	showuid(uid_t);
static void	showusrname(const char *);
static int	unlimited(struct quotaval *qvs, unsigned numqvs);
static void	usage(void) __dead;

static int	qflag = 0;
static int	vflag = 0;
static int	hflag = 0;
static int	dflag = 0;
static uid_t	myuid;
static int needheading;

int
main(int argc, char *argv[])
{
	int ngroups; 
	gid_t mygid, gidset[NGROUPS];
	int i, gflag = 0, uflag = 0;
	int ch;

	myuid = getuid();
	while ((ch = getopt(argc, argv, "dhugvq")) != -1) {
		switch(ch) {
		case 'g':
			gflag++;
			break;
		case 'u':
			uflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'q':
			qflag++;
			break;
		case 'h':
			hflag++;
			break;
		case 'd':
			dflag++;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (!uflag && !gflag)
		uflag++;
	if (dflag) {
#if 0
		if (myuid != 0)
			errx(1, "-d: permission denied");
#endif
		if (uflag)
			showquotas(QUOTA_IDTYPE_USER, "user", 0, "");
		if (gflag)
			showquotas(QUOTA_IDTYPE_GROUP, "group", 0, "");
		return 0;
	}
	if (argc == 0) {
		if (uflag)
			showuid(myuid);
		if (gflag) {
			if (dflag)
				showgid(0);
			else {
				mygid = getgid();
				ngroups = getgroups(NGROUPS, gidset);
				if (ngroups < 0)
					err(1, "getgroups");
				showgid(mygid);
				for (i = 0; i < ngroups; i++)
					if (gidset[i] != mygid)
						showgid(gidset[i]);
			}
		}
		return 0;
	}
	if (uflag && gflag)
		usage();
	if (uflag) {
		for (; argc > 0; argc--, argv++) {
			if (alldigits(*argv))
				showuid((uid_t)atoi(*argv));
			else
				showusrname(*argv);
		}
		return 0;
	}
	if (gflag) {
		for (; argc > 0; argc--, argv++) {
			if (alldigits(*argv))
				showgid((gid_t)atoi(*argv));
			else
				showgrpname(*argv);
		}
		return 0;
	}
	/* NOTREACHED */
	return 0;
}

static void
usage(void)
{
	const char *p = getprogname();
	fprintf(stderr, "Usage: %s [-Dhguqv]\n"
	    "\t%s [-Dhqv] -u username ...\n"
	    "\t%s [-Dhqv] -g groupname ...\n"
	    "\t%s -d [-Dhguqv]\n", p, p, p, p);
	exit(1);
}

/*
 * Print out quotas for a specified user identifier.
 */
static void
showuid(uid_t uid)
{
	struct passwd *pwd = getpwuid(uid);
	const char *name;

	if (pwd == NULL)
		name = "(no account)";
	else
		name = pwd->pw_name;
	if (uid != myuid && myuid != 0) {
		warnx("%s (uid %d): permission denied", name, uid);
		return;
	}
	showquotas(QUOTA_IDTYPE_USER, "user", uid, name);
}

/*
 * Print out quotas for a specified user name.
 */
static void
showusrname(const char *name)
{
	struct passwd *pwd = getpwnam(name);

	if (pwd == NULL) {
		warnx("%s: unknown user", name);
		return;
	}
	if (pwd->pw_uid != myuid && myuid != 0) {
		warnx("%s (uid %d): permission denied", name, pwd->pw_uid);
		return;
	}
	showquotas(QUOTA_IDTYPE_USER, "user", pwd->pw_uid, name);
}

/*
 * Print out quotas for a specified group identifier.
 */
static void
showgid(gid_t gid)
{
	struct group *grp = getgrgid(gid);
	int ngroups;
	gid_t mygid, gidset[NGROUPS];
	int i;
	const char *name;

	if (grp == NULL)
		name = "(no entry)";
	else
		name = grp->gr_name;
	mygid = getgid();
	ngroups = getgroups(NGROUPS, gidset);
	if (ngroups < 0) {
		warn("getgroups");
		return;
	}
	if (gid != mygid) {
		for (i = 0; i < ngroups; i++)
			if (gid == gidset[i])
				break;
		if (i >= ngroups && myuid != 0) {
			warnx("%s (gid %d): permission denied", name, gid);
			return;
		}
	}
	showquotas(QUOTA_IDTYPE_GROUP, "group", gid, name);
}

/*
 * Print out quotas for a specified group name.
 */
static void
showgrpname(const char *name)
{
	struct group *grp = getgrnam(name);
	int ngroups;
	gid_t mygid, gidset[NGROUPS];
	int i;

	if (grp == NULL) {
		warnx("%s: unknown group", name);
		return;
	}
	mygid = getgid();
	ngroups = getgroups(NGROUPS, gidset);
	if (ngroups < 0) {
		warn("getgroups");
		return;
	}
	if (grp->gr_gid != mygid) {
		for (i = 0; i < ngroups; i++)
			if (grp->gr_gid == gidset[i])
				break;
		if (i >= ngroups && myuid != 0) {
			warnx("%s (gid %d): permission denied",
			    name, grp->gr_gid);
			return;
		}
	}
	showquotas(QUOTA_IDTYPE_GROUP, "group", grp->gr_gid, name);
}

static void
showquotas(int idtype, const char *idtypename, id_t id, const char *idname)
{
	struct quotause *qup;
	struct quotause *quplist;

	needheading = 1;

	quplist = getprivs(id, idtype);
	for (qup = quplist; qup; qup = qup->next) {
		showonequota(idtype, idtypename, id, idname, qup);
	}
	if (!qflag) {
		/* In case nothing printed, issue a header saying "none" */
		heading(idtype, idtypename, id, idname, "none");
	}
}

static void
showonequota(int idtype, const char *idtypename, id_t id, const char *idname,
	     struct quotause *qup)
{
	static time_t now;
	struct quotaval *qvs;
	unsigned numqvs, i;
	const char *msg;

	qvs = qup->qvs;
	numqvs = qup->numqvs;

	if (now == 0) {
		time(&now);
	}

	if (!vflag && unlimited(qvs, numqvs)) {
		return;
	}

	if (qflag) {
		for (i=0; i<numqvs; i++) {
			msg = getovermsg(&qvs[i],
					 quota_idtype_getname(qup->qh, i),
					 now);
			if (msg != NULL) {
				heading(idtype, idtypename, id, idname, "");
				printf("\t%s %s\n", msg, qup->fsname);
			}
		}
		return;
	}

	/*
	 * XXX this behavior appears to be demanded by the ATF tests,
	 * although it seems to be at variance with the preexisting
	 * logic in quota.c.
	 */
	if (unlimited(qvs, numqvs) && !anyusage(qvs, numqvs)) {
		return;
	}

	/*
	 * XXX: anyover can in fact be true if anyusage is not true,
	 * if there's a quota of zero set on some volume. This is
	 * because the check we do checks if adding one more thing
	 * will go over. That is reasonable, I suppose, but arguably
	 * the resulting behavior with usage 0 is a bug. (Also, what
	 * reason do we have to believe that the reported grace expire
	 * time is valid if we aren't in fact over yet?)
	 */

	if (vflag || dflag || anyover(qvs, numqvs, now) || 
	    anyusage(qvs, numqvs)) {
		heading(idtype, idtypename, id, idname, "");
		if (strlen(qup->fsname) > 4) {
			printf("%s\n", qup->fsname);
			printf("%12s", "");
		} else {
			printf("%12s", qup->fsname);
		}

		for (i=0; i<numqvs; i++) {
			printqv(&qvs[i],
				quota_objtype_isbytes(qup->qh, i), now);
		}
		printf("\n");
	}
}

static void
heading(int idtype, const char *idtypename, id_t id, const char *idname,
	const char *tag)
{
	if (needheading == 0)
		return;
	needheading = 0;

	if (dflag)
		printf("Default %s disk quotas: %s\n", idtypename, tag);
	else
		printf("Disk quotas for %s %s (%cid %u): %s\n",
		       idtypename, idname, idtypename[0], id, tag);

	if (!qflag && tag[0] == '\0') {
		printf("%12s%9s %8s%9s%8s%8s %7s%8s%8s\n"
		    , "Filesystem"
		    , "blocks"
		    , "quota"
		    , "limit"
		    , "grace"
		    , "files"
		    , "quota"
		    , "limit"
		    , "grace"
		);
	}
}

static void
printqv(struct quotaval *qv, int isbytes, time_t now)
{
	char buf[20];
	const char *str;
	int intprtflags, over, width;

	/*
	 * The assorted finagling of width is to match the previous
	 * open-coded formatting for exactly two quota object types,
	 * which was chosen to make the default report fit in 80
	 * columns.
	 */

	width = isbytes ? 9 : 8;
	intprtflags = isbytes ? HN_B : 0;
	over = isover(qv, now);

	str = intprt(buf, width, qv->qv_usage, intprtflags, hflag);
	printf("%*s", width, str);

	printf("%c", over ? '*' : ' ');

	str = intprt(buf, width, qv->qv_softlimit, intprtflags, hflag);
	printf("%*s", width-1, str);

	str = intprt(buf, width, qv->qv_hardlimit, intprtflags, hflag);
	printf("%*s", width, str);

	if (over) {
		str = timeprt(buf, 9, now, qv->qv_expiretime);
	} else if (vflag && qv->qv_grace != QUOTA_NOTIME) {
		str = timeprt(buf, 9, 0, qv->qv_grace);
	} else {
		str = "";
	}
	printf("%8s", str);
}

/*
 * Collect the requested quota information.
 */
static struct quotause *
getprivs(id_t id, int idtype)
{
	struct quotause *qup, *quptail;
	struct quotause *quphead;
	struct statvfs *fst;
	struct quotakey qk;
	int nfst, i;
	unsigned j;

	qup = quphead = quptail = NULL;

	nfst = getmntinfo(&fst, MNT_WAIT);
	if (nfst == 0)
		errx(2, "no filesystems mounted!");
	for (i = 0; i < nfst; i++) {
		if (qup == NULL) {
			if ((qup = malloc(sizeof *qup)) == NULL)
				err(1, "Out of memory");
		}
		qup->qh = quota_open(fst[i].f_mntonname);
		if (qup->qh == NULL) {
			if (errno == EOPNOTSUPP || errno == ENXIO) {
				continue;
			}
			err(1, "%s: quota_open", fst[i].f_mntonname);
		}
		qup->numqvs = quota_getnumidtypes(qup->qh);
		qup->qvs = malloc(qup->numqvs * sizeof(qup->qvs[0]));
		if (qup->qvs == NULL) {
			err(1, "Out of memory");
		}
		qk.qk_idtype = idtype;
		if (dflag) {
			qk.qk_id = QUOTA_DEFAULTID;
		} else {
			qk.qk_id = id;
		}
		for (j=0; j<qup->numqvs; j++) {
			qk.qk_objtype = j;
			if (quota_get(qup->qh, &qk, &qup->qvs[j]) < 0) {
				if (errno != ENOENT && errno != ENODEV) {
					warn("%s: quota_get (objtype %u)",
					     fst[i].f_mntonname, j);
				}
				quotaval_clear(&qup->qvs[j]);
			}
		}
		(void)strlcpy(qup->fsname, fst[i].f_mntonname,
		    sizeof(qup->fsname));
		if (quphead == NULL)
			quphead = qup;
		else
			quptail->next = qup;
		quptail = qup;
		quptail->next = 0;
		qup = NULL;
	}
	free(qup);
	return quphead;
}

static int
unlimited(struct quotaval *qvs, unsigned numqvs)
{
	unsigned i;

	for (i=0; i<numqvs; i++) {
		if (qvs[i].qv_softlimit != QUOTA_NOLIMIT ||
		    qvs[i].qv_hardlimit != QUOTA_NOLIMIT) {
			return 0;
		}
	}
	return 1;
}

static int
anyusage(struct quotaval *qvs, unsigned numqvs)
{
	unsigned i;

	for (i=0; i<numqvs; i++) {
		if (qvs[i].qv_usage > 0) {
			return 1;
		}
	}
	return 0;
}

static int
anyover(struct quotaval *qvs, unsigned numqvs, time_t now)
{
	unsigned i;

	for (i=0; i<numqvs; i++) {
		if (isover(&qvs[i], now)) {
			return 1;
		}
	}
	return 0;
}

static int
isover(struct quotaval *qv, time_t now)
{
	return (qv->qv_usage >= qv->qv_hardlimit ||
		qv->qv_usage >= qv->qv_softlimit);
}

static const char *
getovermsg(struct quotaval *qv, const char *what, time_t now)
{
	static char buf[64];

	if (qv->qv_usage >= qv->qv_hardlimit) {
		snprintf(buf, sizeof(buf), "%c%s limit reached on",
			 toupper((unsigned char)what[0]), what+1);
		return buf;
	}

	if (qv->qv_usage < qv->qv_softlimit) {
		/* Ok */
		return NULL;
	}

	if (now > qv->qv_expiretime) {
		snprintf(buf, sizeof(buf), "Over %s quota on", what);
		return buf;
	}

	snprintf(buf, sizeof(buf), "In %s grace period on", what);
	return buf;
}
