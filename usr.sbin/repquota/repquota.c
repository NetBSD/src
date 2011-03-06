/*	$NetBSD: repquota.c,v 1.28 2011/03/06 23:25:42 christos Exp $	*/

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
__RCSID("$NetBSD: repquota.c,v 1.28 2011/03/06 23:25:42 christos Exp $");
#endif
#endif /* not lint */

/*
 * Quota report
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <prop/proplib.h>
#include <sys/quota.h>

#include <errno.h>
#include <err.h>
#include <fstab.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ufs/ufs/quota2_prop.h>
#include <ufs/ufs/quota1.h>

#include "printquota.h"
#include "quotautil.h"

struct fileusage {
	struct	fileusage *fu_next;
	struct	quota2_entry fu_q2e;
	uint32_t	fu_id;
	char	fu_name[1];
	/* actually bigger */
};
#define FUHASH 1024	/* must be power of two */
static struct fileusage *fuhead[MAXQUOTAS][FUHASH];
static uint32_t highid[MAXQUOTAS];	/* highest addid()'ed identifier per type */
int valid[MAXQUOTAS];
static struct quota2_entry defaultq2e[MAXQUOTAS];

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
				errs += repquota(&fst[i], GRPQUOTA);
			if (uflag)
				errs += repquota(&fst[i], USRQUOTA);
			continue;
		}
		if ((argnum = oneof(fst[i].f_mntonname, argv, argc)) >= 0 ||
		    (argnum = oneof(fst[i].f_mntfromname, argv, argc)) >= 0) {
			done |= 1U << argnum;
			if (gflag)
				errs += repquota(&fst[i], GRPQUOTA);
			if (uflag)
				errs += repquota(&fst[i], USRQUOTA);
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
repquota(const struct statvfs *vfs, int type)
{
	if (repquota2(vfs, type) != 0)
		return repquota1(vfs, type);
	return 0;
}

static int
repquota2(const struct statvfs *vfs, int type)
{
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	struct plistref pref;
	int8_t error8, version = 0;
	prop_object_iterator_t cmditer, dataiter;
	struct quota2_entry *q2ep;
	struct fileusage *fup;
	const char *strid;
	uint32_t id;

	dict = quota2_prop_create();
	cmds = prop_array_create();
	datas = prop_array_create();

	if (dict == NULL || cmds == NULL || datas == NULL)
		errx(1, "can't allocate proplist");
	if (!quota2_prop_add_command(cmds, "getall", qfextension[type], datas))
		err(1, "prop_add_command");
	if (!quota2_prop_add_command(cmds, "get version", qfextension[type],
	     prop_array_create()))
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
	if ((errno = quota2_get_cmds(dict, &cmds)) != 0) {
		err(1, "quota2_get_cmds");
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
				warn("get %s quotas", qfextension[type]);
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

		valid[type] = 1;
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
				q2ep = &defaultq2e[type];
			} else {
				if ((fup = lookup(id, type)) == 0)
					fup = addid(id, type, (char *)0);
				q2ep = &fup->fu_q2e;
				q2ep->q2e_uid = id;
			}
				
			errno = quota2_dict_get_q2e_usage(data, q2ep);
			if (errno)
				err(1, "quota2_dict_get_q2e_usage");
		}
		prop_object_iterator_release(dataiter);
	}
	prop_object_iterator_release(cmditer);
	prop_object_release(dict);
	if (xflag == 0)
		printquotas(type, vfs, version);
	return 0;
}

static int
repquota1(const struct statvfs *vfs, int type)
{
	char qfpathname[MAXPATHLEN];
	struct fstab *fs;
	struct fileusage *fup;
	FILE *qf;
	uint32_t id;
	struct dqblk dqbuf;
	time_t bgrace = MAX_DQ_TIME, igrace = MAX_DQ_TIME;

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
		if ((fup = lookup(id, type)) == 0)
			fup = addid(id, type, (char *)0);
		dqblk2q2e(&dqbuf, &fup->fu_q2e);
		fup->fu_q2e.q2e_val[QL_BLOCK].q2v_grace = bgrace;
		fup->fu_q2e.q2e_val[QL_FILE].q2v_grace = igrace;
	}
	defaultq2e[type].q2e_val[QL_BLOCK].q2v_grace = bgrace;
	defaultq2e[type].q2e_val[QL_FILE].q2v_grace = igrace;
	defaultq2e[type].q2e_val[QL_BLOCK].q2v_softlimit = 
	    defaultq2e[type].q2e_val[QL_BLOCK].q2v_hardlimit = 
	    defaultq2e[type].q2e_val[QL_FILE].q2v_softlimit = 
	    defaultq2e[type].q2e_val[QL_FILE].q2v_hardlimit = UQUAD_MAX;
	fclose(qf);
	valid[type] = 1;
	if (xflag == 0)
		printquotas(type, vfs, 1);
	return 0;
}

static void
printquotas(int type, const struct statvfs *vfs, int version)
{
	static int multiple = 0;
	uint32_t id;
	int i;
	struct fileusage *fup;
	struct quota2_val *q;
	const char *timemsg[N_QL];
	char overchar[N_QL];
	time_t now;
	char b0[20], b1[20], b2[20], b3[20];

	switch(type) {
	case  GRPQUOTA:
		{
		struct group *gr;
		setgrent();
		while ((gr = getgrent()) != 0)
			(void)addid(gr->gr_gid, GRPQUOTA, gr->gr_name);
		endgrent();
		break;
		}
	case USRQUOTA:
		{
		struct passwd *pw;
		setpwent();
		while ((pw = getpwent()) != 0)
			(void)addid(pw->pw_uid, USRQUOTA, pw->pw_name);
		endpwent();
		break;
		}
	default:
		errx(1, "unknown quota type %d", type);
	}

	time(&now);

	if (multiple++)
		printf("\n");
	if (vflag)
		printf("*** Report for %s quotas on %s (%s, version %d)\n",
		    qfextension[type], vfs->f_mntonname, vfs->f_mntfromname,
		    version);
	printf("                        Block limits               "
	    "File limits\n");
	printf(type == USRQUOTA ? "User " : "Group");
	printf("            used     soft     hard  grace      used"
	    "soft    hard  grace\n");
	for (id = 0; id <= highid[type]; id++) {
		fup = qremove(id, type);
		q = fup->fu_q2e.q2e_val;
		if (fup == 0)
			continue;
		for (i = 0; i < N_QL; i++) {
			switch (QL_STATUS(quota2_check_limit(&q[i], 1, now))) {
			case QL_S_DENY_HARD:
			case QL_S_DENY_GRACE:
			case QL_S_ALLOW_SOFT:
				timemsg[i] = timeprt(b0, 8, now, q[i].q2v_time);
				overchar[i] = '+';
				break;
			default:
				timemsg[i] =  (vflag && version == 2) ?
				    timeprt(b0, 8, 0, q[i].q2v_grace) : "";
				overchar[i] = '-';
				break;
			}
		}

		if (q[QL_BLOCK].q2v_cur == 0 &&
		    q[QL_FILE].q2v_cur == 0 && vflag == 0 &&
		    overchar[QL_BLOCK] == '-' && overchar[QL_FILE] == '-')
			continue;
		if (strlen(fup->fu_name) > 9)
			printf("%s ", fup->fu_name);
		else
			printf("%-10s", fup->fu_name);
		printf("%c%c%9s%9s%9s%7s",
			overchar[QL_BLOCK], overchar[QL_FILE],
			intprt(b1, 10, q[QL_BLOCK].q2v_cur, HN_B, hflag),
			intprt(b2, 10, q[QL_BLOCK].q2v_softlimit, HN_B, hflag),
			intprt(b3, 10, q[QL_BLOCK].q2v_hardlimit, HN_B, hflag),
			timemsg[QL_BLOCK]);
		printf("  %8s%8s%8s%7s\n",
			intprt(b1, 9, q[QL_FILE].q2v_cur, 0, hflag),
			intprt(b2, 9, q[QL_FILE].q2v_softlimit, 0, hflag),
			intprt(b3, 9, q[QL_FILE].q2v_hardlimit, 0, hflag),
			timemsg[QL_FILE]);
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
	int type;

	dict = quota2_prop_create();
	cmds = prop_array_create();

	if (dict == NULL || cmds == NULL) {
		errx(1, "can't allocate proplist");
	}


	for (type = 0; type < MAXQUOTAS; type++) {
		if (valid[type] == 0)
			continue;
		datas = prop_array_create();
		if (datas == NULL)
			errx(1, "can't allocate proplist");
		data = q2etoprop(&defaultq2e[type], 1);
		if (data == NULL)
			err(1, "q2etoprop(default)");
		if (!prop_array_add_and_rel(datas, data))
			err(1, "prop_array_add(data)");

		for (id = 0; id <= highid[type]; id++) {
			fup = qremove(id, type);
			if (fup == 0)
				continue;
			fup->fu_q2e.q2e_uid = id;
			data = q2etoprop(&fup->fu_q2e, 0);
			if (data == NULL)
				err(1, "q2etoprop(default)");
			if (!prop_array_add_and_rel(datas, data))
				err(1, "prop_array_add(data)");
			free(fup);
		}

		if (!quota2_prop_add_command(cmds, "set",
		    qfextension[type], datas))
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
 * Lookup an id of a specific type.
 */
struct fileusage *
lookup(uint32_t id, int type)
{
	struct fileusage *fup;

	for (fup = fuhead[type][id & (FUHASH-1)]; fup != 0; fup = fup->fu_next)
		if (fup->fu_id == id)
			return fup;
	return NULL;
}
/*
 * Lookup and remove an id of a specific type.
 */
static struct fileusage *
qremove(uint32_t id, int type)
{
	struct fileusage *fup, **fupp;

	for (fupp = &fuhead[type][id & (FUHASH-1)]; *fupp != 0;) {
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
addid(uint32_t id, int type, const char *name)
{
	struct fileusage *fup, **fhp;
	struct group *gr = NULL;
	struct passwd *pw = NULL;
	size_t len;

	if ((fup = lookup(id, type)) != NULL) {
		return fup;
	}
	if (name == NULL) {
		switch(type) {
		case  GRPQUOTA:
			gr = getgrgid(id);
			
			if (gr != NULL)
				name = gr->gr_name;
			break;
		case USRQUOTA:
			pw = getpwuid(id);
			if (pw)
				name = pw->pw_name;
			break;
		default:
			errx(1, "unknown quota type %d\n", type);
		}
	}

	if (name)
		len = strlen(name);
	else
		len = 10;
	if ((fup = calloc(1, sizeof(*fup) + len)) == NULL)
		err(1, "out of memory for fileusage structures");
	fhp = &fuhead[type][id & (FUHASH - 1)];
	fup->fu_next = *fhp;
	*fhp = fup;
	fup->fu_id = id;
	if (id > highid[type])
		highid[type] = id;
	if (name) {
		memmove(fup->fu_name, name, len + 1);
	} else {
		snprintf(fup->fu_name, len + 1, "%u", id);
	}
	fup->fu_q2e = defaultq2e[type];
	return fup;
}
