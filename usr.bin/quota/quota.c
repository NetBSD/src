/*	$NetBSD: quota.c,v 1.33.2.1 2011/01/20 14:25:05 bouyer Exp $	*/

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
__RCSID("$NetBSD: quota.c,v 1.33.2.1 2011/01/20 14:25:05 bouyer Exp $");
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
#include <sys/queue.h>
#include <prop/proplib.h>
#include <sys/quota.h>

#include <ufs/ufs/quota2_prop.h>
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

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpcsvc/rquota.h>

const char *qfextension[] = INITQFNAMES;

struct quotause {
	struct	quotause *next;
	long	flags;
	struct	quota2_entry q2e;
	char	fsname[MAXPATHLEN + 1];
};
#define	FOUND	0x01

int	alldigits(char *);
int	callaurpc(char *, int, int, int, xdrproc_t, void *, xdrproc_t, void *);
int	main(int, char **);
int	getnfsquota(struct statvfs *, struct fstab *, struct quotause *,
	    long, int);
struct quotause	*getprivs(long id, int quotatype);
int	getufsquota(struct statvfs *, struct quotause *, long, int);
void	heading(int, u_long, const char *, const char *);
void	showgid(gid_t);
void	showgrpname(const char *);
void	showquotas(int, u_long, const char *);
void	showuid(uid_t);
void	showusrname(const char *);
const char *intprt(uint64_t, int);
const char *timeprt(time_t seconds);
void	usage(void);

int	qflag = 0;
int	vflag = 0;
int	hflag = 0;
int	dflag = 0;
int	Dflag = 0;
uid_t	myuid;

int
main(argc, argv)
	int argc;
	char *argv[];
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
		if (myuid != 0) {
			printf("quota: -d: permission denied\n");
			exit(1);
		}
#endif
		if (uflag)
			showquotas(USRQUOTA, 0, "");
		if (gflag)
			showquotas(GRPQUOTA, 0, "");
		exit(0);
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
		exit(0);
	}
	if (uflag && gflag)
		usage();
	if (uflag) {
		for (; argc > 0; argc--, argv++) {
			if (alldigits(*argv))
				showuid(atoi(*argv));
			else
				showusrname(*argv);
		}
		exit(0);
	}
	if (gflag) {
		for (; argc > 0; argc--, argv++) {
			if (alldigits(*argv))
				showgid(atoi(*argv));
			else
				showgrpname(*argv);
		}
		exit(0);
	}
	/* NOTREACHED */
	return (0);
}

void
usage()
{

	fprintf(stderr, "%s\n%s\n%s\n%s\n",
	    "usage: quota [-Dhguqv]",
	    "\tquota [-Dhqv] -u username ...",
	    "\tquota [-Dhqv] -g groupname ...",
	    "\tquota -d [-Dhguqv]");
	exit(1);
}

/*
 * Print out quotas for a specified user identifier.
 */
void
showuid(uid)
	uid_t uid;
{
	struct passwd *pwd = getpwuid(uid);
	const char *name;

	if (pwd == NULL)
		name = "(no account)";
	else
		name = pwd->pw_name;
	if (uid != myuid && myuid != 0) {
		printf("quota: %s (uid %d): permission denied\n", name, uid);
		return;
	}
	showquotas(USRQUOTA, uid, name);
}

/*
 * Print out quotas for a specified user name.
 */
void
showusrname(name)
	const char *name;
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
	showquotas(USRQUOTA, pwd->pw_uid, name);
}

/*
 * Print out quotas for a specified group identifier.
 */
void
showgid(gid)
	gid_t gid;
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
	showquotas(GRPQUOTA, gid, name);
}

/*
 * Print out quotas for a specified group name.
 */
void
showgrpname(name)
	const char *name;
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
	showquotas(GRPQUOTA, grp->gr_gid, name);
}

void
showquotas(type, id, name)
	int type;
	u_long id;
	const char *name;
{
	struct quotause *qup;
	struct quotause *quplist;
	const char *msgi, *msgb, *nam;
	int lines = 0;
	static time_t now;

	if (now == 0)
		time(&now);
	quplist = getprivs(id, type);
	for (qup = quplist; qup; qup = qup->next) {
		if (!vflag &&
		    qup->q2e.q2e_val[Q2V_BLOCK].q2v_softlimit == UQUAD_MAX &&
		    qup->q2e.q2e_val[Q2V_BLOCK].q2v_hardlimit == UQUAD_MAX &&
		    qup->q2e.q2e_val[Q2V_FILE].q2v_softlimit == UQUAD_MAX &&
		    qup->q2e.q2e_val[Q2V_FILE].q2v_hardlimit == UQUAD_MAX)
			continue;
		msgi = NULL;
		if (qup->q2e.q2e_val[Q2V_FILE].q2v_hardlimit &&
		    qup->q2e.q2e_val[Q2V_FILE].q2v_cur >=
		    qup->q2e.q2e_val[Q2V_FILE].q2v_hardlimit)
			msgi = "File limit reached on";
		else if (qup->q2e.q2e_val[Q2V_FILE].q2v_softlimit &&
		    qup->q2e.q2e_val[Q2V_FILE].q2v_cur >=
		    qup->q2e.q2e_val[Q2V_FILE].q2v_softlimit) {
			if (qup->q2e.q2e_val[Q2V_FILE].q2v_time > now)
				msgi = "In file grace period on";
			else
				msgi = "Over file quota on";
		}
		msgb = NULL;
		if (qup->q2e.q2e_val[Q2V_BLOCK].q2v_hardlimit &&
		    qup->q2e.q2e_val[Q2V_BLOCK].q2v_cur >=
		    qup->q2e.q2e_val[Q2V_BLOCK].q2v_hardlimit)
			msgb = "Block limit reached on";
		else if (qup->q2e.q2e_val[Q2V_BLOCK].q2v_softlimit &&
		    qup->q2e.q2e_val[Q2V_BLOCK].q2v_cur >=
		    qup->q2e.q2e_val[Q2V_BLOCK].q2v_softlimit) {
			if (qup->q2e.q2e_val[Q2V_BLOCK].q2v_time > now)
				msgb = "In block grace period on";
			else
				msgb = "Over block quota on";
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
		if (vflag || dflag ||
		    qup->q2e.q2e_val[Q2V_BLOCK].q2v_cur ||
		    qup->q2e.q2e_val[Q2V_FILE].q2v_cur) {
			if (lines++ == 0)
				heading(type, id, name, "");
			nam = qup->fsname;
			if (strlen(qup->fsname) > 4) {
				printf("%s\n", qup->fsname);
				nam = "";
			} 
			printf("%12s%9s%c%8s%9s%8s"
			    , nam
			    , intprt(qup->q2e.q2e_val[Q2V_BLOCK].q2v_cur
				,HN_B)
			    , (msgb == NULL) ? ' ' : '*'
			    , intprt(qup->q2e.q2e_val[Q2V_BLOCK].q2v_softlimit
				, HN_B)
			    , intprt(qup->q2e.q2e_val[Q2V_BLOCK].q2v_hardlimit
				, HN_B)
			    , (msgb == NULL) ? ""
			        : timeprt(qup->q2e.q2e_val[Q2V_BLOCK].q2v_time));
			printf("%8s%c%7s%8s%8s\n"
			    , intprt(qup->q2e.q2e_val[Q2V_FILE].q2v_cur, 0)
			    , (msgi == NULL) ? ' ' : '*'
			    , intprt(qup->q2e.q2e_val[Q2V_FILE].q2v_softlimit
				, 0)
			    , intprt(qup->q2e.q2e_val[Q2V_FILE].q2v_hardlimit
				, 0)
			    , (msgi == NULL) ? ""
			        : timeprt(qup->q2e.q2e_val[Q2V_FILE].q2v_time)
			);
			continue;
		}
	}
	if (!qflag && lines == 0)
		heading(type, id, name, "none");
}

void
heading(type, id, name, tag)
	int type;
	u_long id;
	const char *name, *tag;
{
	if (dflag)
		printf("Default %s disk quotas: %s\n",
		    qfextension[type], tag);
	else
		printf("Disk quotas for %s %s (%cid %ld): %s\n",
		    qfextension[type], name, *qfextension[type],
		    (u_long)id, tag);

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
 * convert 64bit value to a printable string
 */
const char *
intprt(uint64_t val, int flags)
{
	static char buf[21];

	if (val == UQUAD_MAX)
		return("-");

	if (flags & HN_B)
		val = dbtob(val);
	
	if (hflag) {
		humanize_number(buf, 6, val, "", HN_AUTOSCALE, flags);
		return buf;
	}
	if (flags & HN_B) {
		/* traditionnal display: blocks are in kilobytes */
		val = val / 1024;
	}
	snprintf(buf, sizeof(buf), "%" PRIu64, val);
	return buf;
}

/*
 * Calculate the grace period and return a printable string for it.
 */
const char *
timeprt(time_t seconds)
{
	time_t hours, minutes;
	static char buf[20];
	static time_t now;

	if (now == 0)
		time(&now);
	if (now > seconds)
		return ("none");
	seconds -= now;
	minutes = (seconds + 30) / 60;
	hours = (minutes + 30) / 60;
	if (hours >= 36) {
		(void)snprintf(buf, sizeof buf, "%ddays",
		    (int)((hours + 12) / 24));
		return (buf);
	}
	if (minutes >= 60) {
		(void)snprintf(buf, sizeof buf, "%2d:%d",
		    (int)(minutes / 60), (int)(minutes % 60));
		return (buf);
	}
	(void)snprintf(buf, sizeof buf, "%2d", (int)minutes);
	return (buf);
}

/*
 * Collect the requested quota information.
 */
struct quotause *
getprivs(id, quotatype)
	long id;
	int quotatype;
{
	struct quotause *qup, *quptail;
	struct quotause *quphead;
	struct statvfs *fst;
	int nfst, i;

	qup = quphead = quptail = NULL;

	nfst = getmntinfo(&fst, MNT_WAIT);
	if (nfst == 0)
		errx(2, "no filesystems mounted!");
	setfsent();
	for (i = 0; i < nfst; i++) {
		if (qup == NULL) {
			if ((qup =
			    (struct quotause *)malloc(sizeof *qup)) == NULL)
				errx(2, "out of memory");
		}
		if (strncmp(fst[i].f_fstypename, "nfs", 
		    sizeof(fst[i].f_fstypename)) == 0) {
			if (getnfsquota(&fst[i], NULL, qup, id, quotatype) == 0)
				continue;
		} else if (strncmp(fst[i].f_fstypename, "ffs",
		    sizeof(fst[i].f_fstypename)) == 0 &&
		    (fst[i].f_flag &ST_QUOTA) != 0) {
			if (getufsquota(&fst[i], qup, id, quotatype) == 0)
				continue;
		} else
			continue;
		(void)strncpy(qup->fsname, fst[i].f_mntonname,
		    sizeof(qup->fsname) - 1);
		if (quphead == NULL)
			quphead = qup;
		else
			quptail->next = qup;
		quptail = qup;
		quptail->next = 0;
		qup = NULL;
	}
	if (qup)
		free(qup);
	endfsent();
	return (quphead);
}


int
getufsquota(struct statvfs *fst, struct quotause *qup, long id, int type)
{
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	struct plistref pref;
	int error;
	int8_t error8;
	bool ret;

	dict = quota2_prop_create();
	cmds = prop_array_create();
	datas = prop_array_create();
	data = prop_dictionary_create();

	if (dict == NULL || cmds == NULL || datas == NULL || data == NULL)
		errx(1, "can't allocate proplist");

	if (dflag)
		ret = prop_dictionary_set_cstring(data, "id", "default");
	else
		ret = prop_dictionary_set_uint32(data, "id", id);
	if (!ret)
		err(1, "prop_dictionary_set(id)");
		
	if (!prop_array_add(datas, data))
		err(1, "prop_array_add(data)");
	prop_object_release(data);
	if (!quota2_prop_add_command(cmds, "get", qfextension[type], datas))
		err(1, "prop_add_command");
	if (!prop_dictionary_set(dict, "commands", cmds))
		err(1, "prop_dictionary_set(command)");
	if (Dflag)
		printf("message to kernel:\n%s\n",
		    prop_dictionary_externalize(dict));

	if (!prop_dictionary_send_syscall(dict, &pref))
		err(1, "prop_dictionary_send_syscall");
	prop_object_release(dict);

	if (quotactl(fst->f_mntonname, &pref) != 0)
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
		if (error8 != ENOENT && error8 != ENODEV) {
			if (dflag)
				fprintf(stderr, "get default %s quota: %s\n",
				    qfextension[type], strerror(error8));
			else 
				fprintf(stderr, "get %s quota for %ld: %s\n",
				    qfextension[type], id, strerror(error8));
		}
		prop_object_release(dict);
		return (0);
	}
	datas = prop_dictionary_get(cmd, "data");
	if (datas == NULL)
		err(1, "prop_dict_get(datas)");

	/* only one data, no need to iter */
	if (prop_array_count(datas) == 0) {
		/* no quota for this user/group */
		prop_object_release(dict);
		return (0);
	}
	
	data = prop_array_get(datas, 0);
	if (data == NULL)
		err(1, "prop_array_get(data)");

	error = quota2_dict_get_q2e_usage(data, &qup->q2e);
	if (error) {
		errx(1, "quota2_dict_get_q2e_usage: %s\n",
		    strerror(error));
	}
	return (1);
}

int
getnfsquota(fst, fs, qup, id, quotatype)
	struct statvfs *fst;
	struct fstab *fs; 
	struct quotause *qup;
	long id;
	int quotatype;
{
	struct getquota_args gq_args;
	struct ext_getquota_args ext_gq_args;
	struct getquota_rslt gq_rslt;
	struct quota2_entry *q2e = &qup->q2e;
	struct timeval tv;
	char *cp;
	int ret;

	if (fst->f_flag & MNT_LOCAL)
		return (0);

	/*
	 * must be some form of "hostname:/path"
	 */
	cp = strchr(fst->f_mntfromname, ':');
	if (cp == NULL) {
		warnx("cannot find hostname for %s", fst->f_mntfromname);
		return (0);
	}
 
	*cp = '\0';
	if (*(cp+1) != '/') {
		*cp = ':';
		return (0);
	}

	ext_gq_args.gqa_pathp = cp + 1;
	ext_gq_args.gqa_id = id;
	ext_gq_args.gqa_type =
	    (quotatype == USRQUOTA) ? RQUOTA_USRQUOTA : RQUOTA_GRPQUOTA;
	ret = callaurpc(fst->f_mntfromname, RQUOTAPROG, EXT_RQUOTAVERS,
	    RQUOTAPROC_GETQUOTA, xdr_ext_getquota_args, &ext_gq_args,
	    xdr_getquota_rslt, &gq_rslt);
	if (ret == RPC_PROGVERSMISMATCH) {
		if (quotatype != USRQUOTA) {
			*cp = ':';
			return (0);
		}
		/* try RQUOTAVERS */
		gq_args.gqa_pathp = cp + 1;
		gq_args.gqa_uid = id;
		ret = callaurpc(fst->f_mntfromname, RQUOTAPROG, RQUOTAVERS,
		    RQUOTAPROC_GETQUOTA, xdr_getquota_args, &gq_args,
			    xdr_getquota_rslt, &gq_rslt);
	}
	if (ret != RPC_SUCCESS) {
		*cp = ':';
		return (0);
	}

	switch (gq_rslt.status) {
	case Q_NOQUOTA:
		break;
	case Q_EPERM:
		warnx("quota permission error, host: %s", fst->f_mntfromname);
		break;
	case Q_OK:
		gettimeofday(&tv, NULL);
			/* blocks*/
		q2e->q2e_val[Q2V_BLOCK].q2v_hardlimit =
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_bhardlimit *
		    (gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsize / DEV_BSIZE);
		q2e->q2e_val[Q2V_BLOCK].q2v_softlimit =
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsoftlimit *
		    (gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsize / DEV_BSIZE);
		q2e->q2e_val[Q2V_BLOCK].q2v_cur =
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_curblocks *
		    (gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsize / DEV_BSIZE);
			/* inodes */
		q2e->q2e_val[Q2V_FILE].q2v_hardlimit =
			gq_rslt.getquota_rslt_u.gqr_rquota.rq_fhardlimit;
		q2e->q2e_val[Q2V_FILE].q2v_softlimit =
			gq_rslt.getquota_rslt_u.gqr_rquota.rq_fsoftlimit;
		q2e->q2e_val[Q2V_FILE].q2v_cur =
			gq_rslt.getquota_rslt_u.gqr_rquota.rq_curfiles;
			/* grace times */
		q2e->q2e_val[Q2V_BLOCK].q2v_time =
		    tv.tv_sec + gq_rslt.getquota_rslt_u.gqr_rquota.rq_btimeleft;
		q2e->q2e_val[Q2V_FILE].q2v_time =
		    tv.tv_sec + gq_rslt.getquota_rslt_u.gqr_rquota.rq_ftimeleft;
		*cp = ':';
		return (1);
	default:
		warnx("bad rpc result, host: %s", fst->f_mntfromname);
		break;
	}
	*cp = ':';
	return (0);
}
 
int
callaurpc(host, prognum, versnum, procnum, inproc, in, outproc, out)
	char *host;
	int prognum, versnum, procnum;
	xdrproc_t inproc;
	void *in;
	xdrproc_t outproc;
	void *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct hostent *hp;
	struct timeval timeout, tottimeout;
 
	CLIENT *client = NULL;
	int sock = RPC_ANYSOCK;
 
	if ((hp = gethostbyname(host)) == NULL)
		return ((int) RPC_UNKNOWNHOST);
	timeout.tv_usec = 0;
	timeout.tv_sec = 6;
	memmove(&server_addr.sin_addr, hp->h_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port =  0;

	if ((client = clntudp_create(&server_addr, prognum,
	    versnum, timeout, &sock)) == NULL)
		return ((int) rpc_createerr.cf_stat);

	client->cl_auth = authunix_create_default();
	tottimeout.tv_sec = 25;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(client, procnum, inproc, in,
	    outproc, out, tottimeout);
 
	return ((int) clnt_stat);
}

int
alldigits(s)
	char *s;
{
	int c;

	c = *s++;
	do {
		if (!isdigit(c))
			return (0);
	} while ((c = *s++) != 0);
	return (1);
}
