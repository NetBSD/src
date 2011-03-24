/*	$NetBSD: quota.c,v 1.37 2011/03/24 17:05:46 bouyer Exp $	*/

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
__RCSID("$NetBSD: quota.c,v 1.37 2011/03/24 17:05:46 bouyer Exp $");
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

#include <quota/quotaprop.h>
#include <quota/quota.h>

#include "printquota.h"
#include "getvfsquota.h"

struct quotause {
	struct	quotause *next;
	long	flags;
	uid_t	id;
	struct	ufs_quota_entry qe[QUOTA_NLIMITS];
	char	fsname[MAXPATHLEN + 1];
};
#define	FOUND	0x01
#define	QUOTA2	0x02

static struct quotause	*getprivs(uint32_t, int);
static void	heading(int, uint32_t, const char *, const char *);
static void	showgid(gid_t);
static void	showgrpname(const char *);
static void	showquotas(int, uint32_t, const char *);
static void	showuid(uid_t);
static void	showusrname(const char *);
static void	usage(void) __attribute__((__noreturn__));

static int	qflag = 0;
static int	vflag = 0;
static int	hflag = 0;
static int	dflag = 0;
static int	Dflag = 0;
static uid_t	myuid;

int
main(int argc, char *argv[])
{
	int ngroups; 
	gid_t mygid, gidset[NGROUPS];
	int i, gflag = 0, uflag = 0;
	int ch;

	myuid = getuid();
	while ((ch = getopt(argc, argv, "Ddhugvq")) != -1) {
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
		case 'D':
			Dflag++;
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
			showquotas(QUOTA_CLASS_USER, 0, "");
		if (gflag)
			showquotas(QUOTA_CLASS_GROUP, 0, "");
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
	showquotas(QUOTA_CLASS_USER, uid, name);
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
	showquotas(QUOTA_CLASS_USER, pwd->pw_uid, name);
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
	showquotas(QUOTA_CLASS_GROUP, gid, name);
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
	showquotas(QUOTA_CLASS_GROUP, grp->gr_gid, name);
}

static void
showquotas(int type, uint32_t id, const char *name)
{
	struct quotause *qup;
	struct quotause *quplist;
	const char *msgi, *msgb, *nam, *timemsg;
	int lines = 0;
	static time_t now;
	char b0[20], b1[20], b2[20], b3[20];

	if (now == 0)
		time(&now);
	quplist = getprivs(id, type);
	for (qup = quplist; qup; qup = qup->next) {
		int ql_stat;
		struct ufs_quota_entry *q = qup->qe;
		if (!vflag &&
		    q[QUOTA_LIMIT_BLOCK].ufsqe_softlimit == UQUAD_MAX &&
		    q[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit == UQUAD_MAX &&
		    q[QUOTA_LIMIT_FILE].ufsqe_softlimit == UQUAD_MAX &&
		    q[QUOTA_LIMIT_FILE].ufsqe_hardlimit == UQUAD_MAX)
			continue;
		ql_stat = quota_check_limit(q[QUOTA_LIMIT_FILE].ufsqe_cur, 1,
		    q[QUOTA_LIMIT_FILE].ufsqe_softlimit,
		    q[QUOTA_LIMIT_FILE].ufsqe_hardlimit,
		    q[QUOTA_LIMIT_FILE].ufsqe_time, now);
		switch(QL_STATUS(ql_stat)) {
		case QL_S_DENY_HARD:
			msgi = "File limit reached on";
			break;
		case QL_S_DENY_GRACE:
			msgi = "Over file quota on";
			break;
		case QL_S_ALLOW_SOFT:
			msgi = "In file grace period on";
			break;
		default:
			msgi = NULL;
		}
		ql_stat = quota_check_limit(q[QUOTA_LIMIT_BLOCK].ufsqe_cur, 1,
		    q[QUOTA_LIMIT_BLOCK].ufsqe_softlimit,
		    q[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit,
		    q[QUOTA_LIMIT_BLOCK].ufsqe_time, now);
		switch(QL_STATUS(ql_stat)) {
		case QL_S_DENY_HARD:
			msgb = "Block limit reached on";
			break;
		case QL_S_DENY_GRACE:
			msgb = "Over block quota on";
			break;
		case QL_S_ALLOW_SOFT:
			msgb = "In block grace period on";
			break;
		default:
			msgb = NULL;
		}
		if (qflag) {
			if ((msgi != NULL || msgb != NULL) &&
			    lines++ == 0)
				heading(type, id, name, "");
			if (msgi != NULL)
				printf("\t%s %s\n", msgi, qup->fsname);
			if (msgb != NULL)
				printf("\t%s %s\n", msgb, qup->fsname);
			continue;
		}
		if (vflag || dflag || msgi || msgb ||
		    q[QUOTA_LIMIT_BLOCK].ufsqe_cur ||
		    q[QUOTA_LIMIT_FILE].ufsqe_cur) {
			if (lines++ == 0)
				heading(type, id, name, "");
			nam = qup->fsname;
			if (strlen(qup->fsname) > 4) {
				printf("%s\n", qup->fsname);
				nam = "";
			} 
			if (msgb)
				timemsg = timeprt(b0, 9, now,
				    q[QUOTA_LIMIT_BLOCK].ufsqe_time);
			else if ((qup->flags & QUOTA2) != 0 && vflag)
				timemsg = timeprt(b0, 9, 0,
				    q[QUOTA_LIMIT_BLOCK].ufsqe_grace);
			else
				timemsg = "";
				
			printf("%12s%9s%c%8s%9s%8s",
			    nam,
			    intprt(b1, 9, q[QUOTA_LIMIT_BLOCK].ufsqe_cur,
			    HN_B, hflag),
			    (msgb == NULL) ? ' ' : '*',
			    intprt(b2, 9, q[QUOTA_LIMIT_BLOCK].ufsqe_softlimit,
			    HN_B, hflag),
			    intprt(b3, 9, q[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit,
			    HN_B, hflag),
			    timemsg);

			if (msgi)
				timemsg = timeprt(b0, 9, now, 
				    q[QUOTA_LIMIT_FILE].ufsqe_time);
			else if ((qup->flags & QUOTA2) != 0 && vflag)
				timemsg = timeprt(b0, 9, 0,
				    q[QUOTA_LIMIT_FILE].ufsqe_grace);
			else
				timemsg = "";
				
			printf("%8s%c%7s%8s%8s\n",
			    intprt(b1, 8, q[QUOTA_LIMIT_FILE].ufsqe_cur, 0,
			     hflag),
			    (msgi == NULL) ? ' ' : '*',
			    intprt(b2, 8, q[QUOTA_LIMIT_FILE].ufsqe_softlimit,
			     0, hflag),
			    intprt(b3, 8, q[QUOTA_LIMIT_FILE].ufsqe_hardlimit,
			     0, hflag),
			    timemsg);
			continue;
		}
	}
	if (!qflag && lines == 0)
		heading(type, id, name, "none");
}

static void
heading(int type, uint32_t id, const char *name, const char *tag)
{
	if (dflag)
		printf("Default %s disk quotas: %s\n",
		    ufs_quota_class_names[type], tag);
	else
		printf("Disk quotas for %s %s (%cid %u): %s\n",
		    ufs_quota_class_names[type], name,
		    *ufs_quota_class_names[type], id, tag);

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

/*
 * Collect the requested quota information.
 */
static struct quotause *
getprivs(uint32_t id, int quotatype)
{
	struct quotause *qup, *quptail;
	struct quotause *quphead;
	struct statvfs *fst;
	int nfst, i;
	int8_t version;

	qup = quphead = quptail = NULL;

	nfst = getmntinfo(&fst, MNT_WAIT);
	if (nfst == 0)
		errx(2, "no filesystems mounted!");
	for (i = 0; i < nfst; i++) {
		if (qup == NULL) {
			if ((qup = malloc(sizeof *qup)) == NULL)
				err(1, "out of memory");
		}
		if (strncmp(fst[i].f_fstypename, "nfs", 
		    sizeof(fst[i].f_fstypename)) == 0) {
			version = 0;
			if (getnfsquota(fst[i].f_mntfromname,
			    qup->qe, id, ufs_quota_class_names[quotatype]) != 1)
				continue;
		} else if ((fst[i].f_flag & ST_QUOTA) != 0) {
			if (getvfsquota(fst[i].f_mntonname, qup->qe, &version,
			    id, quotatype, dflag, Dflag) != 1)
				continue;
		} else
			continue;
		(void)strncpy(qup->fsname, fst[i].f_mntonname,
		    sizeof(qup->fsname) - 1);
		if (version == 2)
			qup->flags |= QUOTA2;
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
