/*	$NetBSD: repquota.c,v 1.36 2012/01/09 15:40:47 dholland Exp $	*/

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
__RCSID("$NetBSD: repquota.c,v 1.36 2012/01/09 15:40:47 dholland Exp $");
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

#include <quota/quota.h>
#include <ufs/ufs/quota1.h>
#include <quota.h>

#include "printquota.h"
#include "quotautil.h"

struct fileusage {
	struct	fileusage *fu_next;
	struct	quotaval fu_qv[QUOTA_NLIMITS];
	uint32_t	fu_id;
	char	fu_name[1];
	/* actually bigger */
};
#define FUHASH 1024	/* must be power of two */
static struct fileusage *fuhead[QUOTA_NCLASS][FUHASH];
static uint32_t highid[QUOTA_NCLASS];	/* highest addid()'ed identifier per class */
int valid[QUOTA_NCLASS];
static struct quotaval defaultqv[QUOTA_NCLASS][QUOTA_NLIMITS];

static int	vflag = 0;		/* verbose */
static int	aflag = 0;		/* all file systems */
static int	Dflag = 0;		/* debug */
static int	hflag = 0;		/* humanize */
static int	xflag = 0;		/* export */


static struct fileusage *addid(uint32_t, int, const char *);
static struct fileusage *lookup(uint32_t, int);
static struct fileusage *qremove(uint32_t, int);
static int	repquota(struct quotahandle *, int);
static int	repquota2(struct quotahandle *, int);
static int	repquota1(struct quotahandle *, int);
static void	usage(void) __attribute__((__noreturn__));
static void	printquotas(int, struct quotahandle *);
static void	exportquotas(void);

int
main(int argc, char **argv)
{
	int gflag = 0, uflag = 0, errs = 0;
	long i, argnum, done = 0;
	int ch;
	struct statvfs *fst;
	int nfst;
	struct quotahandle *qh;

	while ((ch = getopt(argc, argv, "Daguhvx")) != -1) {
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
		case 'D':
			Dflag++;
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
	if (repquota2(qh, idtype) != 0)
		return repquota1(qh, idtype);
	return 0;
}

static int
repquota2(struct quotahandle *qh, int idtype)
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

static int
repquota1(struct quotahandle *qh, int idtype)
{
	char qfpathname[MAXPATHLEN];
	struct fstab *fs;
	struct fileusage *fup;
	FILE *qf;
	uint32_t id;
	struct dqblk dqbuf;
	time_t bgrace = MAX_DQ_TIME, igrace = MAX_DQ_TIME;
	int type = ufsclass2qtype(idtype);
	const char *mountpoint;

	mountpoint = quota_getmountpoint(qh);

	setfsent();
	while ((fs = getfsent()) != NULL) {
		if (strcmp(fs->fs_vfstype, "ffs") == 0 &&
		   strcmp(fs->fs_file, mountpoint) == 0)
			break;
	}
	endfsent();
	if (fs == NULL) {
		warnx("%s not found in fstab", mountpoint);
		return 1;
	}
	if (!hasquota(qfpathname, sizeof(qfpathname), fs, type))
		return 0;
		
	if ((qf = fopen(qfpathname, "r")) == NULL) {
		warn("Cannot open `%s'", qfpathname);
		return 1;
	}
	for (id = 0; ; id++) {
		fread(&dqbuf, sizeof(struct dqblk), 1, qf);
		if (feof(qf))
			break;
		if (id == 0) {
			if (dqbuf.dqb_btime > 0)
				bgrace = dqbuf.dqb_btime;
			if (dqbuf.dqb_itime > 0)
				igrace = dqbuf.dqb_itime;
		}
		if (dqbuf.dqb_curinodes == 0 && dqbuf.dqb_curblocks == 0 &&
		    dqbuf.dqb_bsoftlimit == 0 && dqbuf.dqb_bhardlimit == 0 &&
		    dqbuf.dqb_isoftlimit == 0 && dqbuf.dqb_ihardlimit == 0)
			continue;
		if ((fup = lookup(id, idtype)) == 0)
			fup = addid(id, idtype, (char *)0);
		dqblk_to_quotaval(&dqbuf, fup->fu_qv);
		fup->fu_qv[QUOTA_LIMIT_BLOCK].qv_grace = bgrace;
		fup->fu_qv[QUOTA_LIMIT_FILE].qv_grace = igrace;
	}
	defaultqv[idtype][QUOTA_LIMIT_BLOCK].qv_grace = bgrace;
	defaultqv[idtype][QUOTA_LIMIT_FILE].qv_grace = igrace;
	defaultqv[idtype][QUOTA_LIMIT_BLOCK].qv_softlimit = 
	    defaultqv[idtype][QUOTA_LIMIT_BLOCK].qv_hardlimit = 
	    defaultqv[idtype][QUOTA_LIMIT_FILE].qv_softlimit = 
	    defaultqv[idtype][QUOTA_LIMIT_FILE].qv_hardlimit = QUOTA_NOLIMIT;
	fclose(qf);
	valid[idtype] = 1;
	if (xflag == 0)
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
	const char *timemsg[QUOTA_NLIMITS];
	char overchar[QUOTA_NLIMITS];
	time_t now;
	char b0[2][20], b1[20], b2[20], b3[20];

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
		    ufs_quota_class_names[idtype], quota_getmountpoint(qh),
		    quota_getmountdevice(qh), quota_getimplname(qh));
	printf("                        Block limits               "
	    "File limits\n");
	printf(idtype == QUOTA_IDTYPE_USER ? "User " : "Group");
	printf("            used     soft     hard  grace      used"
	    "soft    hard  grace\n");
	for (id = 0; id <= highid[idtype]; id++) {
		fup = qremove(id, idtype);
		q = fup->fu_qv;
		if (fup == 0)
			continue;
		for (i = 0; i < QUOTA_NLIMITS; i++) {
			switch (QL_STATUS(quota_check_limit(q[i].qv_usage, 1,
			    q[i].qv_softlimit, q[i].qv_hardlimit,
			    q[i].qv_expiretime, now))) {
			case QL_S_DENY_HARD:
			case QL_S_DENY_GRACE:
			case QL_S_ALLOW_SOFT:
				timemsg[i] = timeprt(b0[i], 8, now,
				    q[i].qv_expiretime);
				overchar[i] = '+';
				break;
			default:
				if (vflag && q[i].qv_grace != QUOTA_NOTIME) {
					timemsg[i] = timeprt(b0[i], 8, 0,
							     q[i].qv_grace);
				} else {
					timemsg[i] = "";
				}
				overchar[i] = '-';
				break;
			}
		}

		if (q[QUOTA_LIMIT_BLOCK].qv_usage == 0 &&
		    q[QUOTA_LIMIT_FILE].qv_usage == 0 && vflag == 0 &&
		    overchar[QUOTA_LIMIT_BLOCK] == '-' &&
		    overchar[QUOTA_LIMIT_FILE] == '-')
			continue;
		if (strlen(fup->fu_name) > 9)
			printf("%s ", fup->fu_name);
		else
			printf("%-10s", fup->fu_name);
		printf("%c%c%9s%9s%9s%7s",
			overchar[QUOTA_LIMIT_BLOCK], overchar[QUOTA_LIMIT_FILE],
			intprt(b1, 10, q[QUOTA_LIMIT_BLOCK].qv_usage,
			  HN_B, hflag),
			intprt(b2, 10, q[QUOTA_LIMIT_BLOCK].qv_softlimit,
			  HN_B, hflag),
			intprt(b3, 10, q[QUOTA_LIMIT_BLOCK].qv_hardlimit,
			  HN_B, hflag),
			timemsg[QUOTA_LIMIT_BLOCK]);
		printf("  %8s%8s%8s%7s\n",
			intprt(b1, 9, q[QUOTA_LIMIT_FILE].qv_usage, 0, hflag),
			intprt(b2, 9, q[QUOTA_LIMIT_FILE].qv_softlimit,
			  0, hflag),
			intprt(b3, 9, q[QUOTA_LIMIT_FILE].qv_hardlimit,
			  0, hflag),
			timemsg[QUOTA_LIMIT_FILE]);
		free(fup);
	}
}

static void
exportquotas(void)
{
	uint32_t id;
	struct fileusage *fup;
	prop_dictionary_t dict, data;
	prop_array_t cmds, datas;
	int idtype;
	uint64_t *valuesp[QUOTA_NLIMITS];

	dict = quota_prop_create();
	cmds = prop_array_create();

	if (dict == NULL || cmds == NULL) {
		errx(1, "can't allocate proplist");
	}


	for (idtype = 0; idtype < QUOTA_NCLASS; idtype++) {
		if (valid[idtype] == 0)
			continue;
		datas = prop_array_create();
		if (datas == NULL)
			errx(1, "can't allocate proplist");
		valuesp[QUOTA_LIMIT_BLOCK] =
		    &defaultqv[idtype][QUOTA_LIMIT_BLOCK].qv_hardlimit;
		valuesp[QUOTA_LIMIT_FILE] =
		    &defaultqv[idtype][QUOTA_LIMIT_FILE].qv_hardlimit;
		data = quota64toprop(0, 1, valuesp,
		    ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
		    ufs_quota_limit_names, QUOTA_NLIMITS);
		if (data == NULL)
			err(1, "quota64toprop(default)");
		if (!prop_array_add_and_rel(datas, data))
			err(1, "prop_array_add(data)");

		for (id = 0; id <= highid[idtype]; id++) {
			fup = qremove(id, idtype);
			if (fup == 0)
				continue;
			valuesp[QUOTA_LIMIT_BLOCK] =
			    &fup->fu_qv[QUOTA_LIMIT_BLOCK].qv_hardlimit;
			valuesp[QUOTA_LIMIT_FILE] =
			    &fup->fu_qv[QUOTA_LIMIT_FILE].qv_hardlimit;
			data = quota64toprop(id, 0, valuesp,
			    ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
			    ufs_quota_limit_names, QUOTA_NLIMITS);
			if (data == NULL)
				err(1, "quota64toprop(id)");
			if (!prop_array_add_and_rel(datas, data))
				err(1, "prop_array_add(data)");
			free(fup);
		}

		if (!quota_prop_add_command(cmds, "set",
		    ufs_quota_class_names[idtype], datas))
			err(1, "prop_add_command");
	}

	if (!prop_dictionary_set(dict, "commands", cmds))
		err(1, "prop_dictionary_set(command)");

	printf("%s\n", prop_dictionary_externalize(dict));
	return;
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
			errx(1, "Unknown quota ID type %d\n", idtype);
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
	fup->fu_qv[QUOTA_LIMIT_BLOCK] = defaultqv[idtype][QUOTA_LIMIT_BLOCK];
	fup->fu_qv[QUOTA_LIMIT_FILE] = defaultqv[idtype][QUOTA_LIMIT_FILE];
	return fup;
}
