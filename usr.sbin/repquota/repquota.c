/*	$NetBSD: repquota.c,v 1.30 2011/03/24 17:05:47 bouyer Exp $	*/

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
__RCSID("$NetBSD: repquota.c,v 1.30 2011/03/24 17:05:47 bouyer Exp $");
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

#include <quota/quotaprop.h>
#include <quota/quota.h>
#include <ufs/ufs/quota1.h>
#include <sys/quota.h>

#include "printquota.h"
#include "quotautil.h"

struct fileusage {
	struct	fileusage *fu_next;
	struct	ufs_quota_entry fu_qe[QUOTA_NLIMITS];
	uint32_t	fu_id;
	char	fu_name[1];
	/* actually bigger */
};
#define FUHASH 1024	/* must be power of two */
static struct fileusage *fuhead[QUOTA_NCLASS][FUHASH];
static uint32_t highid[QUOTA_NCLASS];	/* highest addid()'ed identifier per class */
int valid[QUOTA_NCLASS];
static struct ufs_quota_entry defaultqe[QUOTA_NCLASS][QUOTA_NLIMITS];

static int	vflag = 0;		/* verbose */
static int	aflag = 0;		/* all file systems */
static int	Dflag = 0;		/* debug */
static int	hflag = 0;		/* humanize */
static int	xflag = 0;		/* export */


static struct fileusage *addid(uint32_t, int, const char *);
static struct fileusage *lookup(uint32_t, int);
static struct fileusage *qremove(uint32_t, int);
static int	repquota(const struct statvfs *, int);
static int	repquota2(const struct statvfs *, int);
static int	repquota1(const struct statvfs *, int);
static void	usage(void) __attribute__((__noreturn__));
static void	printquotas(int, const struct statvfs *, int);
static void	exportquotas(void);

int
main(int argc, char **argv)
{
	int gflag = 0, uflag = 0, errs = 0;
	long i, argnum, done = 0;
	int ch;
	struct statvfs *fst;
	int nfst;

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
		if (aflag) {
			if (gflag)
				errs += repquota(&fst[i], QUOTA_CLASS_GROUP);
			if (uflag)
				errs += repquota(&fst[i], QUOTA_CLASS_USER);
			continue;
		}
		if ((argnum = oneof(fst[i].f_mntonname, argv, argc)) >= 0 ||
		    (argnum = oneof(fst[i].f_mntfromname, argv, argc)) >= 0) {
			done |= 1U << argnum;
			if (gflag)
				errs += repquota(&fst[i], QUOTA_CLASS_GROUP);
			if (uflag)
				errs += repquota(&fst[i], QUOTA_CLASS_USER);
		}
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
repquota(const struct statvfs *vfs, int class)
{
	if (repquota2(vfs, class) != 0)
		return repquota1(vfs, class);
	return 0;
}

static int
repquota2(const struct statvfs *vfs, int class)
{
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	struct plistref pref;
	int8_t error8, version = 0;
	prop_object_iterator_t cmditer, dataiter;
	struct ufs_quota_entry *qep;
	struct fileusage *fup;
	const char *strid;
	uint32_t id;
	uint64_t *values[QUOTA_NLIMITS];

	dict = quota_prop_create();
	cmds = prop_array_create();
	datas = prop_array_create();

	if (dict == NULL || cmds == NULL || datas == NULL)
		errx(1, "can't allocate proplist");
	if (!quota_prop_add_command(cmds, "getall",
	    ufs_quota_class_names[class], datas))
		err(1, "prop_add_command");
	if (!quota_prop_add_command(cmds, "get version",
	    ufs_quota_class_names[class], prop_array_create()))
		err(1, "prop_add_command");
	if (!prop_dictionary_set(dict, "commands", cmds))
		err(1, "prop_dictionary_set(command)");
	if (Dflag)
		printf("message to kernel:\n%s\n",
		    prop_dictionary_externalize(dict));
	if (!prop_dictionary_send_syscall(dict, &pref))
		err(1, "prop_dictionary_send_syscall");
	prop_object_release(dict);

	if (quotactl(vfs->f_mntonname, &pref) != 0)
		err(1, "quotactl");

	if ((errno = prop_dictionary_recv_syscall(&pref, &dict)) != 0) {
		err(1, "prop_dictionary_recv_syscall");
	}
	if (Dflag)
		printf("reply from kernel:\n%s\n",
		    prop_dictionary_externalize(dict));
	if ((errno = quota_get_cmds(dict, &cmds)) != 0) {
		err(1, "quota_get_cmds");
	}
	cmditer = prop_array_iterator(cmds);
	if (cmditer == NULL)
		err(1, "prop_array_iterator(cmds)");

	while ((cmd = prop_object_iterator_next(cmditer)) != NULL) {
		const char *cmdstr;
		if (!prop_dictionary_get_cstring_nocopy(cmd, "command",
		    &cmdstr))
			err(1, "prop_get(command)");

		if (!prop_dictionary_get_int8(cmd, "return", &error8))
			err(1, "prop_get(return)");

		if (error8) {
			prop_object_release(dict);
			if (error8 != EOPNOTSUPP) {
				errno = error8;
				warn("get %s quotas",
				    ufs_quota_class_names[class]);
			}
			return error8;
		}
		datas = prop_dictionary_get(cmd, "data");
		if (datas == NULL)
			err(1, "prop_dict_get(datas)");

		if (strcmp("get version", cmdstr) == 0) {
			data = prop_array_get(datas, 0);
			if (data == NULL)
				err(1, "prop_array_get(version)");
			if (!prop_dictionary_get_int8(data, "version",
			    &version))
				err(1, "prop_get_int8(version)");
			continue;
		}
		dataiter = prop_array_iterator(datas);
		if (dataiter == NULL)
			err(1, "prop_array_iterator");

		valid[class] = 1;
		while ((data = prop_object_iterator_next(dataiter)) != NULL) {
			strid = NULL;
			if (!prop_dictionary_get_uint32(data, "id", &id)) {
				if (!prop_dictionary_get_cstring_nocopy(data,
				    "id", &strid))
					errx(1, "can't find id in quota entry");
				if (strcmp(strid, "default") != 0) {
					errx(1,
					    "wrong id string %s in quota entry",
					    strid);
				}
				qep = defaultqe[class];
			} else {
				if ((fup = lookup(id, class)) == 0)
					fup = addid(id, class, (char *)0);
				qep = fup->fu_qe;
			}
			values[QUOTA_LIMIT_BLOCK] =
			    &qep[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit;
			values[QUOTA_LIMIT_FILE] =
			    &qep[QUOTA_LIMIT_FILE].ufsqe_hardlimit;
				
			errno = proptoquota64(data, values,
			    ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
			    ufs_quota_limit_names, QUOTA_NLIMITS);
			if (errno)
				err(1, "proptoquota64");
		}
		prop_object_iterator_release(dataiter);
	}
	prop_object_iterator_release(cmditer);
	prop_object_release(dict);
	if (xflag == 0)
		printquotas(class, vfs, version);
	return 0;
}

static int
repquota1(const struct statvfs *vfs, int class)
{
	char qfpathname[MAXPATHLEN];
	struct fstab *fs;
	struct fileusage *fup;
	FILE *qf;
	uint32_t id;
	struct dqblk dqbuf;
	time_t bgrace = MAX_DQ_TIME, igrace = MAX_DQ_TIME;
	int type = ufsclass2qtype(class);

	setfsent();
	while ((fs = getfsent()) != NULL) {
		if (strcmp(fs->fs_vfstype, "ffs") == 0 &&
		   strcmp(fs->fs_file, vfs->f_mntonname) == 0)
			break;
	}
	endfsent();
	if (fs == NULL) {
		warnx("%s not found in fstab", vfs->f_mntonname);
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
		if ((fup = lookup(id, class)) == 0)
			fup = addid(id, class, (char *)0);
		dqblk2ufsqe(&dqbuf, fup->fu_qe);
		fup->fu_qe[QUOTA_LIMIT_BLOCK].ufsqe_grace = bgrace;
		fup->fu_qe[QUOTA_LIMIT_FILE].ufsqe_grace = igrace;
	}
	defaultqe[class][QUOTA_LIMIT_BLOCK].ufsqe_grace = bgrace;
	defaultqe[class][QUOTA_LIMIT_FILE].ufsqe_grace = igrace;
	defaultqe[class][QUOTA_LIMIT_BLOCK].ufsqe_softlimit = 
	    defaultqe[class][QUOTA_LIMIT_BLOCK].ufsqe_hardlimit = 
	    defaultqe[class][QUOTA_LIMIT_FILE].ufsqe_softlimit = 
	    defaultqe[class][QUOTA_LIMIT_FILE].ufsqe_hardlimit = UQUAD_MAX;
	fclose(qf);
	valid[class] = 1;
	if (xflag == 0)
		printquotas(class, vfs, 1);
	return 0;
}

static void
printquotas(int class, const struct statvfs *vfs, int version)
{
	static int multiple = 0;
	uint32_t id;
	int i;
	struct fileusage *fup;
	struct ufs_quota_entry *q;
	const char *timemsg[QUOTA_NLIMITS];
	char overchar[QUOTA_NLIMITS];
	time_t now;
	char b0[2][20], b1[20], b2[20], b3[20];

	switch(class) {
	case  QUOTA_CLASS_GROUP:
		{
		struct group *gr;
		setgrent();
		while ((gr = getgrent()) != 0)
			(void)addid(gr->gr_gid, QUOTA_CLASS_GROUP, gr->gr_name);
		endgrent();
		break;
		}
	case QUOTA_CLASS_USER:
		{
		struct passwd *pw;
		setpwent();
		while ((pw = getpwent()) != 0)
			(void)addid(pw->pw_uid, QUOTA_CLASS_USER, pw->pw_name);
		endpwent();
		break;
		}
	default:
		errx(1, "unknown quota class %d", class);
	}

	time(&now);

	if (multiple++)
		printf("\n");
	if (vflag)
		printf("*** Report for %s quotas on %s (%s, version %d)\n",
		    ufs_quota_class_names[class], vfs->f_mntonname,
		    vfs->f_mntfromname, version);
	printf("                        Block limits               "
	    "File limits\n");
	printf(class == QUOTA_CLASS_USER ? "User " : "Group");
	printf("            used     soft     hard  grace      used"
	    "soft    hard  grace\n");
	for (id = 0; id <= highid[class]; id++) {
		fup = qremove(id, class);
		q = fup->fu_qe;
		if (fup == 0)
			continue;
		for (i = 0; i < QUOTA_NLIMITS; i++) {
			switch (QL_STATUS(quota_check_limit(q[i].ufsqe_cur, 1,
			    q[i].ufsqe_softlimit, q[i].ufsqe_hardlimit,
			    q[i].ufsqe_time, now))) {
			case QL_S_DENY_HARD:
			case QL_S_DENY_GRACE:
			case QL_S_ALLOW_SOFT:
				timemsg[i] = timeprt(b0[i], 8, now,
				    q[i].ufsqe_time);
				overchar[i] = '+';
				break;
			default:
				timemsg[i] =  (vflag && version == 2) ?
				    timeprt(b0[i], 8, 0, q[i].ufsqe_grace) : "";
				overchar[i] = '-';
				break;
			}
		}

		if (q[QUOTA_LIMIT_BLOCK].ufsqe_cur == 0 &&
		    q[QUOTA_LIMIT_FILE].ufsqe_cur == 0 && vflag == 0 &&
		    overchar[QUOTA_LIMIT_BLOCK] == '-' &&
		    overchar[QUOTA_LIMIT_FILE] == '-')
			continue;
		if (strlen(fup->fu_name) > 9)
			printf("%s ", fup->fu_name);
		else
			printf("%-10s", fup->fu_name);
		printf("%c%c%9s%9s%9s%7s",
			overchar[QUOTA_LIMIT_BLOCK], overchar[QUOTA_LIMIT_FILE],
			intprt(b1, 10, q[QUOTA_LIMIT_BLOCK].ufsqe_cur,
			  HN_B, hflag),
			intprt(b2, 10, q[QUOTA_LIMIT_BLOCK].ufsqe_softlimit,
			  HN_B, hflag),
			intprt(b3, 10, q[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit,
			  HN_B, hflag),
			timemsg[QUOTA_LIMIT_BLOCK]);
		printf("  %8s%8s%8s%7s\n",
			intprt(b1, 9, q[QUOTA_LIMIT_FILE].ufsqe_cur, 0, hflag),
			intprt(b2, 9, q[QUOTA_LIMIT_FILE].ufsqe_softlimit,
			  0, hflag),
			intprt(b3, 9, q[QUOTA_LIMIT_FILE].ufsqe_hardlimit,
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
	int class;
	uint64_t *valuesp[QUOTA_NLIMITS];

	dict = quota_prop_create();
	cmds = prop_array_create();

	if (dict == NULL || cmds == NULL) {
		errx(1, "can't allocate proplist");
	}


	for (class = 0; class < QUOTA_NCLASS; class++) {
		if (valid[class] == 0)
			continue;
		datas = prop_array_create();
		if (datas == NULL)
			errx(1, "can't allocate proplist");
		valuesp[QUOTA_LIMIT_BLOCK] =
		    &defaultqe[class][QUOTA_LIMIT_BLOCK].ufsqe_hardlimit;
		valuesp[QUOTA_LIMIT_FILE] =
		    &defaultqe[class][QUOTA_LIMIT_FILE].ufsqe_hardlimit;
		data = quota64toprop(0, 1, valuesp,
		    ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
		    ufs_quota_limit_names, QUOTA_NLIMITS);
		if (data == NULL)
			err(1, "quota64toprop(default)");
		if (!prop_array_add_and_rel(datas, data))
			err(1, "prop_array_add(data)");

		for (id = 0; id <= highid[class]; id++) {
			fup = qremove(id, class);
			if (fup == 0)
				continue;
			valuesp[QUOTA_LIMIT_BLOCK] =
			    &fup->fu_qe[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit;
			valuesp[QUOTA_LIMIT_FILE] =
			    &fup->fu_qe[QUOTA_LIMIT_FILE].ufsqe_hardlimit;
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
		    ufs_quota_class_names[class], datas))
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
 * Lookup an id of a specific class.
 */
struct fileusage *
lookup(uint32_t id, int class)
{
	struct fileusage *fup;

	for (fup = fuhead[class][id & (FUHASH-1)]; fup != 0; fup = fup->fu_next)
		if (fup->fu_id == id)
			return fup;
	return NULL;
}
/*
 * Lookup and remove an id of a specific class.
 */
static struct fileusage *
qremove(uint32_t id, int class)
{
	struct fileusage *fup, **fupp;

	for (fupp = &fuhead[class][id & (FUHASH-1)]; *fupp != 0;) {
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
addid(uint32_t id, int class, const char *name)
{
	struct fileusage *fup, **fhp;
	struct group *gr = NULL;
	struct passwd *pw = NULL;
	size_t len;

	if ((fup = lookup(id, class)) != NULL) {
		return fup;
	}
	if (name == NULL) {
		switch(class) {
		case  QUOTA_CLASS_GROUP:
			gr = getgrgid(id);
			
			if (gr != NULL)
				name = gr->gr_name;
			break;
		case QUOTA_CLASS_USER:
			pw = getpwuid(id);
			if (pw)
				name = pw->pw_name;
			break;
		default:
			errx(1, "unknown quota class %d\n", class);
		}
	}

	if (name)
		len = strlen(name);
	else
		len = 10;
	if ((fup = calloc(1, sizeof(*fup) + len)) == NULL)
		err(1, "out of memory for fileusage structures");
	fhp = &fuhead[class][id & (FUHASH - 1)];
	fup->fu_next = *fhp;
	*fhp = fup;
	fup->fu_id = id;
	if (id > highid[class])
		highid[class] = id;
	if (name) {
		memmove(fup->fu_name, name, len + 1);
	} else {
		snprintf(fup->fu_name, len + 1, "%u", id);
	}
	fup->fu_qe[QUOTA_LIMIT_BLOCK] = defaultqe[class][QUOTA_LIMIT_BLOCK];
	fup->fu_qe[QUOTA_LIMIT_FILE] = defaultqe[class][QUOTA_LIMIT_FILE];
	return fup;
}
