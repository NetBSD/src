/*	$NetBSD: repquota.c,v 1.45 2015/06/16 23:04:14 christos Exp $	*/

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
static char sccsid[] = "@(#)repquota.c	8.2 (Berkeley) 11/22/94";
#else
__RCSID("$NetBSD: repquota.c,v 1.45 2015/06/16 23:04:14 christos Exp $");
#endif
#endif /* not lint */

/*
 * Quota report
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>

#include <errno.h>
#include <err.h>
#include <fstab.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <quota.h>

#include "printquota.h"

/*
 * XXX. Ideally we shouldn't compile either of these in, but it's a
 * nontrivial rework to avoid it and it'll work ok for now.
 */
#define REPQUOTA_NUMIDTYPES	2
#define REPQUOTA_NUMOBJTYPES	2

struct fileusage {
	struct	fileusage *fu_next;
	struct	quotaval fu_qv[REPQUOTA_NUMOBJTYPES];
	uint32_t	fu_id;
	char	fu_name[1];
	/* actually bigger */
};

#define FUHASH 1024	/* must be power of two */
static struct fileusage *fuhead[REPQUOTA_NUMIDTYPES][FUHASH];

/* highest addid()'ed identifier per idtype */
static uint32_t highid[REPQUOTA_NUMIDTYPES];

int valid[REPQUOTA_NUMIDTYPES];

static struct quotaval defaultqv[REPQUOTA_NUMIDTYPES][REPQUOTA_NUMOBJTYPES];

static int	vflag = 0;		/* verbose */
static int	aflag = 0;		/* all file systems */
static int	hflag = 0;		/* humanize */
static int	xflag = 0;		/* export */

/*
 * XXX this should go away and be replaced with a call to
 * quota_idtype_getname(), but that needs a quotahandle and requires
 * the same nontrivial rework as getting rid of REPQUOTA_NUMIDTYPES.
 */
static const char *const repquota_idtype_names[REPQUOTA_NUMIDTYPES] = {
	"user",
	"group",
};

static struct fileusage *addid(uint32_t, int, const char *);
static struct fileusage *lookup(uint32_t, int);
static struct fileusage *qremove(uint32_t, int);
static int	repquota(struct quotahandle *, int);
static void	usage(void) __attribute__((__noreturn__));
static void	printquotas(int, struct quotahandle *);
static void	exportquotas(void);
static int	oneof(const char *, char *[], int cnt);
static int isover(struct quotaval *qv, time_t now);

int
main(int argc, char **argv)
{
	int gflag = 0, uflag = 0, errs = 0;
	long i, argnum, done = 0;
	int ch;
	struct statvfs *fst;
	int nfst;
	struct quotahandle *qh;

	if (!strcmp(getprogname(), "quotadump")) {
		xflag = 1;
	}

	while ((ch = getopt(argc, argv, "aguhvx")) != -1) {
		switch(ch) {
		case 'a':
			aflag++;
			break;
		case 'g':
			gflag++;
			break;
		case 'u':
			uflag++;
			break;
		case 'h':
			hflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'x':
			xflag++;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (xflag && (argc != 1 || aflag))
		usage();
	if (argc == 0 && !aflag)
		usage();
	if (!gflag && !uflag) {
		if (aflag)
			gflag++;
		uflag++;
	}

	nfst = getmntinfo(&fst, MNT_WAIT);
	if (nfst == 0)
		errx(1, "no filesystems mounted!");
	for (i = 0; i < nfst; i++) {
		if ((fst[i].f_flag & ST_QUOTA) == 0)
			continue;
		/* check if we want this volume */
		if (!aflag) {
			argnum = oneof(fst[i].f_mntonname, argv, argc);
			if (argnum < 0) {
				argnum = oneof(fst[i].f_mntfromname,
					       argv, argc);
			}
			if (argnum < 0) {
				continue;
			}
			done |= 1U << argnum;
		}

		qh = quota_open(fst[i].f_mntonname);
		if (qh == NULL) {
			/* XXX: check this errno */
			if (errno == EOPNOTSUPP || errno == ENXIO) {
				continue;
			}
			warn("%s: quota_open", fst[i].f_mntonname);
			continue;
		}

		if (gflag)
			errs += repquota(qh, QUOTA_IDTYPE_GROUP);
		if (uflag)
			errs += repquota(qh, QUOTA_IDTYPE_USER);

		quota_close(qh);
	}
	if (xflag)
		exportquotas();
	for (i = 0; i < argc; i++)
		if ((done & (1U << i)) == 0)
			warnx("%s not mounted", argv[i]);
	return errs;
}

static void
usage(void)
{
	const char *p = getprogname();
	fprintf(stderr, "usage: %s [-D] [-v] [-g] [-u] -a\n"
		"\t%s [-D] [-v] [-g] [-u] filesys ...\n"
		"\t%s -x [-D] [-g] [-u] filesys\n", p, p, p);
	exit(1);
}

static int
repquota(struct quotahandle *qh, int idtype)
{
	struct quotacursor *qc;
	struct quotakey qk;
	struct quotaval qv;
	struct quotaval *qvp;
	struct fileusage *fup;

	qc = quota_opencursor(qh);
	if (qc == NULL) {
		return 1;
	}

	if (idtype == QUOTA_IDTYPE_USER) {
		quotacursor_skipidtype(qc, QUOTA_IDTYPE_GROUP);
	}
	if (idtype == QUOTA_IDTYPE_GROUP) {
		quotacursor_skipidtype(qc, QUOTA_IDTYPE_USER);
	}

	valid[idtype] = 0;
	while (!quotacursor_atend(qc)) {
		if (quotacursor_get(qc, &qk, &qv)) {
			err(1, "%s: quotacursor_get", quota_getmountpoint(qh));
		}
		if (qk.qk_idtype != idtype) {
			continue;
		}

		valid[idtype] = 1;
		if (qk.qk_id == QUOTA_DEFAULTID) {
			qvp = defaultqv[idtype];
		} else {
			if ((fup = lookup(qk.qk_id, idtype)) == 0)
				fup = addid(qk.qk_id, idtype, (char *)0);
			qvp = fup->fu_qv;
		}
		if (qk.qk_objtype == QUOTA_OBJTYPE_BLOCKS) {
			qvp[QUOTA_OBJTYPE_BLOCKS] = qv;
		} else if (qk.qk_objtype == QUOTA_OBJTYPE_FILES) {
			qvp[QUOTA_OBJTYPE_FILES] = qv;
		}
	}

	if (xflag == 0 && valid[idtype])
		printquotas(idtype, qh);

	return 0;
}

static void
printquotas(int idtype, struct quotahandle *qh)
{
	static int multiple = 0;
	uint32_t id;
	int i;
	struct fileusage *fup;
	struct quotaval *q;
	const char *timemsg[REPQUOTA_NUMOBJTYPES];
	char overchar[REPQUOTA_NUMOBJTYPES];
	time_t now;
	char b0[2][20], b1[20], b2[20], b3[20];
	int ok, objtype;
	int isbytes, width;

	switch (idtype) {
	case QUOTA_IDTYPE_GROUP:
		{
		struct group *gr;
		setgrent();
		while ((gr = getgrent()) != 0)
			(void)addid(gr->gr_gid, idtype, gr->gr_name);
		endgrent();
		break;
		}
	case QUOTA_IDTYPE_USER:
		{
		struct passwd *pw;
		setpwent();
		while ((pw = getpwent()) != 0)
			(void)addid(pw->pw_uid, idtype, pw->pw_name);
		endpwent();
		break;
		}
	default:
		errx(1, "Unknown quota ID type %d", idtype);
	}

	time(&now);

	if (multiple++)
		printf("\n");
	if (vflag)
		printf("*** Report for %s quotas on %s (%s: %s)\n",
		    repquota_idtype_names[idtype], quota_getmountpoint(qh),
		    quota_getmountdevice(qh), quota_getimplname(qh));
	printf("                        Block limits               "
	    "File limits\n");
	printf(idtype == QUOTA_IDTYPE_USER ? "User " : "Group");
	printf("            used     soft     hard  grace      used"
	    "    soft    hard  grace\n");
	for (id = 0; id <= highid[idtype]; id++) {
		fup = qremove(id, idtype);
		q = fup->fu_qv;
		if (fup == 0)
			continue;
		for (i = 0; i < REPQUOTA_NUMOBJTYPES; i++) {
			if (isover(&q[i], now)) {
				timemsg[i] = timeprt(b0[i], 8, now,
				    q[i].qv_expiretime);
				overchar[i] = '+';
			} else {
				if (vflag && q[i].qv_grace != QUOTA_NOTIME) {
					timemsg[i] = timeprt(b0[i], 8, 0,
							     q[i].qv_grace);
				} else {
					timemsg[i] = "";
				}
				overchar[i] = '-';
			}
		}

		ok = 1;
		for (objtype = 0; objtype < REPQUOTA_NUMOBJTYPES; objtype++) {
			if (q[objtype].qv_usage != 0 ||
			    overchar[objtype] != '-') {
				ok = 0;
			}
		}
		if (ok && vflag == 0)
			continue;
		if (strlen(fup->fu_name) > 9)
			printf("%s ", fup->fu_name);
		else
			printf("%-10s", fup->fu_name);
		for (objtype = 0; objtype < REPQUOTA_NUMOBJTYPES; objtype++) {
			printf("%c", overchar[objtype]);
		}
		for (objtype = 0; objtype < REPQUOTA_NUMOBJTYPES; objtype++) {
			isbytes = quota_objtype_isbytes(qh, objtype);
			width = isbytes ? 9 : 8;
			printf("%*s%*s%*s%7s",
			       width,
			       intprt(b1, width+1, q[objtype].qv_usage,
				      isbytes ? HN_B : 0, hflag),
			       width,
			       intprt(b2, width+1, q[objtype].qv_softlimit,
				      isbytes ? HN_B : 0, hflag),
			       width,
			       intprt(b3, width+1, q[objtype].qv_hardlimit,
				      isbytes ? HN_B : 0, hflag),
			       timemsg[objtype]);

			if (objtype + 1 < REPQUOTA_NUMOBJTYPES) {
				printf("  ");
			} else {
				printf("\n");
			}
		}
		free(fup);
	}
}

static void
exportquotaval(const struct quotaval *qv)
{
	if (qv->qv_hardlimit == QUOTA_NOLIMIT) {
		printf(" -");
	} else {
		printf(" %llu", (unsigned long long)qv->qv_hardlimit);
	}

	if (qv->qv_softlimit == QUOTA_NOLIMIT) {
		printf(" -");
	} else {
		printf(" %llu", (unsigned long long)qv->qv_softlimit);
	}

	printf(" %llu", (unsigned long long)qv->qv_usage);

	if (qv->qv_expiretime == QUOTA_NOTIME) {
		printf(" -");
	} else {
		printf(" %lld", (long long)qv->qv_expiretime);
	}

	if (qv->qv_grace == QUOTA_NOTIME) {
		printf(" -");
	} else {
		printf(" %lld", (long long)qv->qv_grace);
	}
}

static void
exportquotas(void)
{
	int idtype;
	id_t id;
	struct fileusage *fup;

	/* header */
	printf("@format netbsd-quota-dump v1\n");
	printf("# idtype id objtype   hard soft usage expire grace\n");

	for (idtype = 0; idtype < REPQUOTA_NUMIDTYPES; idtype++) {
		if (valid[idtype] == 0)
			continue;

		printf("%s default block  ", repquota_idtype_names[idtype]);
		exportquotaval(&defaultqv[idtype][QUOTA_OBJTYPE_BLOCKS]);
		printf("\n");
			
		printf("%s default file  ", repquota_idtype_names[idtype]);
		exportquotaval(&defaultqv[idtype][QUOTA_OBJTYPE_FILES]);
		printf("\n");

		for (id = 0; id <= highid[idtype]; id++) {
			fup = qremove(id, idtype);
			if (fup == 0)
				continue;

			printf("%s %u block  ", repquota_idtype_names[idtype],
			       id);
			exportquotaval(&fup->fu_qv[QUOTA_OBJTYPE_BLOCKS]);
			printf("\n");

			printf("%s %u file  ", repquota_idtype_names[idtype],
			       id);
			exportquotaval(&fup->fu_qv[QUOTA_OBJTYPE_FILES]);
			printf("\n");

			free(fup);
		}
	}
	printf("@end\n");
}

/*
 * Routines to manage the file usage table.
 *
 * Lookup an id of a specific id type.
 */
struct fileusage *
lookup(uint32_t id, int idtype)
{
	struct fileusage *fup;

	for (fup = fuhead[idtype][id & (FUHASH-1)]; fup != 0; fup = fup->fu_next)
		if (fup->fu_id == id)
			return fup;
	return NULL;
}
/*
 * Lookup and remove an id of a specific id type.
 */
static struct fileusage *
qremove(uint32_t id, int idtype)
{
	struct fileusage *fup, **fupp;

	for (fupp = &fuhead[idtype][id & (FUHASH-1)]; *fupp != 0;) {
		fup = *fupp;
		if (fup->fu_id == id) {
			*fupp = fup->fu_next;
			return fup;
		}
		fupp = &fup->fu_next;
	}
	return NULL;
}

/*
 * Add a new file usage id if it does not already exist.
 */
static struct fileusage *
addid(uint32_t id, int idtype, const char *name)
{
	struct fileusage *fup, **fhp;
	struct group *gr = NULL;
	struct passwd *pw = NULL;
	size_t len;

	if ((fup = lookup(id, idtype)) != NULL) {
		return fup;
	}
	if (name == NULL) {
		switch(idtype) {
		case  QUOTA_IDTYPE_GROUP:
			gr = getgrgid(id);
			
			if (gr != NULL)
				name = gr->gr_name;
			break;
		case QUOTA_IDTYPE_USER:
			pw = getpwuid(id);
			if (pw)
				name = pw->pw_name;
			break;
		default:
			errx(1, "Unknown quota ID type %d", idtype);
		}
	}

	if (name)
		len = strlen(name);
	else
		len = 10;
	if ((fup = calloc(1, sizeof(*fup) + len)) == NULL)
		err(1, "out of memory for fileusage structures");
	fhp = &fuhead[idtype][id & (FUHASH - 1)];
	fup->fu_next = *fhp;
	*fhp = fup;
	fup->fu_id = id;
	if (id > highid[idtype])
		highid[idtype] = id;
	if (name) {
		memmove(fup->fu_name, name, len + 1);
	} else {
		snprintf(fup->fu_name, len + 1, "%u", id);
	}
	/*
	 * XXX nothing guarantees the default limits have been loaded yet
	 */
	fup->fu_qv[QUOTA_OBJTYPE_BLOCKS] = defaultqv[idtype][QUOTA_OBJTYPE_BLOCKS];
	fup->fu_qv[QUOTA_OBJTYPE_FILES] = defaultqv[idtype][QUOTA_OBJTYPE_FILES];
	return fup;
}

/*
 * Check to see if target appears in list of size cnt.
 */
static int
oneof(const char *target, char *list[], int cnt)
{
	int i;

	for (i = 0; i < cnt; i++)
		if (strcmp(target, list[i]) == 0)
			return i;
	return -1;
}

static int
isover(struct quotaval *qv, time_t now)
{ 
	return (qv->qv_usage >= qv->qv_hardlimit ||
		qv->qv_usage >= qv->qv_softlimit);
} 
