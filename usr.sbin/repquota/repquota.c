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
__RCSID("$NetBSD: repquota.c,v 1.25.2.8 2011/02/14 20:55:36 bouyer Exp $");
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

#include <printquota.h>

const char *qfextension[] = INITQFNAMES;
const char *qfname = QUOTAFILENAME;

struct fileusage {
	struct	fileusage *fu_next;
	struct	quota2_entry fu_q2e;
	u_long	fu_id;
	char	fu_name[1];
	/* actually bigger */
};
#define FUHASH 1024	/* must be power of two */
struct fileusage *fuhead[MAXQUOTAS][FUHASH];
u_long highid[MAXQUOTAS];	/* highest addid()'ed identifier per type */
int valid[MAXQUOTAS];
struct quota2_entry defaultq2e[MAXQUOTAS];

int	vflag = 0;		/* verbose */
int	aflag = 0;		/* all file systems */
int	Dflag = 0;		/* debug */
int	hflag = 0;		/* humanize */
int	xflag = 0;		/* export */


struct fileusage *addid(u_long, int, const char *);
int	hasquota(struct fstab *, int, char **);
struct fileusage *lookup(u_long, int);
int	main(int, char **);
int	oneof(const char *, char **, int);
int	repquota(const struct statvfs *, int);
int	repquota2(const struct statvfs *, int);
int	repquota1(const struct statvfs *, int);
void	usage(void);
void	printquotas(int, const struct statvfs *, int);
void	exportquotas(void);
void	dqblk2q2e(const struct dqblk *, struct quota2_entry *);

int
main(argc, argv)
	int argc;
	char **argv;
{
	struct passwd *pw;
	struct group *gr;
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
	if (xflag && argc != 1)
		usage();
	if (argc == 0 && !aflag)
		usage();
	if (!gflag && !uflag) {
		if (aflag)
			gflag++;
		uflag++;
	}
	if (gflag) {
		setgrent();
		while ((gr = getgrent()) != 0)
			(void) addid((u_long)gr->gr_gid, GRPQUOTA, gr->gr_name);
		endgrent();
	}
	if (uflag) {
		setpwent();
		while ((pw = getpwent()) != 0)
			(void) addid((u_long)pw->pw_uid, USRQUOTA, pw->pw_name);
		endpwent();
	}

	nfst = getmntinfo(&fst, MNT_WAIT);
	if (nfst == 0)
		errx(2, "no filesystems mounted!");
	for (i = 0; i < nfst; i++) {
		if (strncmp(fst[i].f_fstypename, "ffs",
		    sizeof(fst[i].f_fstypename)) != 0 ||
		    (fst[i].f_flag & ST_QUOTA) == 0)
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
			done |= 1 << argnum;
			if (gflag)
				errs += repquota(&fst[i], GRPQUOTA);
			if (uflag)
				errs += repquota(&fst[i], USRQUOTA);
		}
	}
	if (xflag)
		exportquotas();
	for (i = 0; i < argc; i++)
		if ((done & (1 << i)) == 0)
			fprintf(stderr, "%s not mounted\n", argv[i]);
	exit(errs);
}

void
usage()
{
	fprintf(stderr, "usage:\n"
		"\trepquota [-D] [-v] [-g] [-u] -a\n"
		"\trepquota [-D] [-v] [-g] [-u] filesys ...\n"
		"\trepquota -x [-D] [-g] [-u] filesys\n");
	exit(1);
}

int
repquota(const struct statvfs *vfs, int type)
{
	if (repquota2(vfs, type) != 0)
		return (repquota1(vfs, type));
	return 0;
}

int
repquota2(const struct statvfs *vfs, int type)
{
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	struct plistref pref;
	int error;
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
				fprintf(stderr, "get %s quotas: %s\n",
				    qfextension[type], strerror(error8));
			}
			return (error8);
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
				
			error = quota2_dict_get_q2e_usage(data, q2ep);
			if (error) {
				errx(1, "quota2_dict_get_q2e_usage: %s\n",
				    strerror(error));
			}
		}
		prop_object_iterator_release(dataiter);
	}
	prop_object_iterator_release(cmditer);
	prop_object_release(dict);
	if (xflag == 0)
		printquotas(type, vfs, version);
	return (0);
}

int repquota1(const struct statvfs *vfs, int type)
{
	char *qfpathname;
	struct fstab *fs;
	struct fileusage *fup;
	FILE *qf;
	u_long id;
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
		fprintf(stderr, "%s not found in fstab\n", vfs->f_mntonname);
		return 1;
	}
	if (!hasquota(fs, type, &qfpathname))
		return 0;
		
	if ((qf = fopen(qfpathname, "r")) == NULL) {
		perror(qfpathname);
		return (1);
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
	return (0);
}

void
printquotas(int type, const struct statvfs *vfs, int version)
{
	static int multiple = 0;
	u_long id;
	int i;
	struct fileusage *fup;
	const char *timemsg[N_QL];
	char overchar[N_QL];
	static time_t now;

	if (now == 0)
		time(&now);

	if (multiple++)
		printf("\n");
	if (vflag)
		fprintf(stdout,
		    "*** Report for %s quotas on %s (%s, version %d)\n",
		    qfextension[type], vfs->f_mntonname, vfs->f_mntfromname,
		    version);
	printf("                        Block limits               File limits\n");
	printf(type == USRQUOTA ? "User " : "Group");
	printf("            used     soft     hard  grace      used    soft    hard  grace\n");
	for (id = 0; id <= highid[type]; id++) {
		fup = lookup(id, type);
		if (fup == 0)
			continue;
		for (i = 0; i < N_QL; i++) {
			switch (QL_STATUS(quota2_check_limit(
			     &fup->fu_q2e.q2e_val[i], 1, now))) {
			case QL_S_DENY_HARD:
			case QL_S_DENY_GRACE:
			case QL_S_ALLOW_SOFT:
				timemsg[i] = timeprt(now,
				    fup->fu_q2e.q2e_val[i].q2v_time, 7);
				overchar[i] = '+';
				break;
			default:
				timemsg[i] =  (vflag && version == 2) ?
				    timeprt(0,
					fup->fu_q2e.q2e_val[i].q2v_grace, 7):
				    "";
				overchar[i] = '-';
				break;
			}
		}

		if (fup->fu_q2e.q2e_val[QL_BLOCK].q2v_cur == 0 &&
		    fup->fu_q2e.q2e_val[QL_FILE].q2v_cur == 0 && vflag == 0 &&
		    overchar[QL_BLOCK] == '-' && overchar[QL_FILE] == '-')
			continue;
		if (strlen(fup->fu_name) > 9)
			printf("%s ", fup->fu_name);
		else
			printf("%-10s", fup->fu_name);
		printf("%c%c%9s%9s%9s%7s",
			overchar[QL_BLOCK], overchar[QL_FILE],
			intprt(fup->fu_q2e.q2e_val[QL_BLOCK].q2v_cur,
				HN_B, hflag, 9),
			intprt(fup->fu_q2e.q2e_val[QL_BLOCK].q2v_softlimit,
				HN_B, hflag, 9),
			intprt(fup->fu_q2e.q2e_val[QL_BLOCK].q2v_hardlimit,
				HN_B, hflag, 9),
			timemsg[QL_BLOCK]);
		printf("  %8s%8s%8s%7s\n",
			intprt(fup->fu_q2e.q2e_val[QL_FILE].q2v_cur,
				0, hflag, 8),
			intprt(fup->fu_q2e.q2e_val[QL_FILE].q2v_softlimit,
				0, hflag, 8),
			intprt(fup->fu_q2e.q2e_val[QL_FILE].q2v_hardlimit,
				0, hflag, 8),
			timemsg[QL_FILE]);
		memset(&fup->fu_q2e, 0, sizeof(fup->fu_q2e));
	}
}

void
exportquotas()
{
	u_long id;
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
			fup = lookup(id, type);
			if (fup == 0)
				continue;
			fup->fu_q2e.q2e_uid = id;
			data = q2etoprop(&fup->fu_q2e, 0);
			if (data == NULL)
				err(1, "q2etoprop(default)");
			if (!prop_array_add_and_rel(datas, data))
				err(1, "prop_array_add(data)");
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
 * Check to see if target appears in list of size cnt.
 */
int
oneof(target, list, cnt)
	const char *target;
	char *list[];
	int cnt;
{
	int i;

	for (i = 0; i < cnt; i++)
		if (strcmp(target, list[i]) == 0)
			return (i);
	return (-1);
}

/*
 * Check to see if a particular quota is to be enabled.
 */
int
hasquota(fs, type, qfnamep)
	struct fstab *fs;
	int type;
	char **qfnamep;
{
	char *opt;
	char *cp = NULL;
	static char initname, usrname[100], grpname[100];
	static char buf[BUFSIZ];

	if (!initname) {
		sprintf(usrname, "%s%s", qfextension[USRQUOTA], qfname);
		sprintf(grpname, "%s%s", qfextension[GRPQUOTA], qfname);
		initname = 1;
	}
	strcpy(buf, fs->fs_mntops);
	for (opt = strtok(buf, ","); opt; opt = strtok(NULL, ",")) {
		if ((cp = strchr(opt, '=')) != NULL)
			*cp++ = '\0';
		if (type == USRQUOTA && strcmp(opt, usrname) == 0)
			break;
		if (type == GRPQUOTA && strcmp(opt, grpname) == 0)
			break;
	}
	if (!opt)
		return (0);
	if (cp) {
		*qfnamep = cp;
		return (1);
	}
	(void) sprintf(buf, "%s/%s.%s", fs->fs_file, qfname, qfextension[type]);
	*qfnamep = buf;
	return (1);
}

/*
 * Routines to manage the file usage table.
 *
 * Lookup an id of a specific type.
 */
struct fileusage *
lookup(id, type)
	u_long id;
	int type;
{
	struct fileusage *fup;

	for (fup = fuhead[type][id & (FUHASH-1)]; fup != 0; fup = fup->fu_next)
		if (fup->fu_id == id)
			return (fup);
	return ((struct fileusage *)0);
}

/*
 * Add a new file usage id if it does not already exist.
 */
struct fileusage *
addid(id, type, name)
	u_long id;
	int type;
	const char *name;
{
	struct fileusage *fup, **fhp;
	int len;

	if ((fup = lookup(id, type)) != NULL)
		return (fup);
	if (name)
		len = strlen(name);
	else
		len = 10;
	if ((fup = (struct fileusage *)calloc(1, sizeof(*fup) + len)) == NULL) {
		fprintf(stderr, "out of memory for fileusage structures\n");
		exit(1);
	}
	fhp = &fuhead[type][id & (FUHASH - 1)];
	fup->fu_next = *fhp;
	*fhp = fup;
	fup->fu_id = id;
	if (id > highid[type])
		highid[type] = id;
	if (name) {
		memmove(fup->fu_name, name, len + 1);
	} else {
		sprintf(fup->fu_name, "%lu", (u_long)id);
	}
	return (fup);
}
