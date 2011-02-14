/*	$NetBSD: quota.c,v 1.33.2.8 2011/02/14 20:35:24 bouyer Exp $	*/

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
__RCSID("$NetBSD: quota.c,v 1.33.2.8 2011/02/14 20:35:24 bouyer Exp $");
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

#include <ufs/ufs/quota2.h>
#include <ufs/ufs/quota1.h>

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpcsvc/rquota.h>

#include <printquota.h>
#include <getvfsquota.h>

struct quotause {
	struct	quotause *next;
	long	flags;
	struct	quota2_entry q2e;
	char	fsname[MAXPATHLEN + 1];
};
#define	FOUND	0x01
#define	QUOTA2	0x02

int	alldigits(char *);
int	callaurpc(char *, int, int, int, xdrproc_t, void *, xdrproc_t, void *);
int	main(int, char **);
int	getnfsquota(struct statvfs *, struct fstab *, struct quotause *,
	    long, int);
struct quotause	*getprivs(long id, int quotatype);
void	heading(int, u_long, const char *, const char *);
void	showgid(gid_t);
void	showgrpname(const char *);
void	showquotas(int, u_long, const char *);
void	showuid(uid_t);
void	showusrname(const char *);
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
	const char *msgi, *msgb, *nam, *timemsg;
	int lines = 0;
	static time_t now;

	if (now == 0)
		time(&now);
	quplist = getprivs(id, type);
	for (qup = quplist; qup; qup = qup->next) {
		int ql_stat;
		if (!vflag &&
		    qup->q2e.q2e_val[QL_BLOCK].q2v_softlimit == UQUAD_MAX &&
		    qup->q2e.q2e_val[QL_BLOCK].q2v_hardlimit == UQUAD_MAX &&
		    qup->q2e.q2e_val[QL_FILE].q2v_softlimit == UQUAD_MAX &&
		    qup->q2e.q2e_val[QL_FILE].q2v_hardlimit == UQUAD_MAX)
			continue;
		ql_stat = quota2_check_limit(&qup->q2e.q2e_val[QL_FILE],
		    1, now);
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
		ql_stat = quota2_check_limit(&qup->q2e.q2e_val[QL_BLOCK],
		    1, now);
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
		if (vflag || dflag ||
		    qup->q2e.q2e_val[QL_BLOCK].q2v_cur ||
		    qup->q2e.q2e_val[QL_FILE].q2v_cur) {
			if (lines++ == 0)
				heading(type, id, name, "");
			nam = qup->fsname;
			if (strlen(qup->fsname) > 4) {
				printf("%s\n", qup->fsname);
				nam = "";
			} 
			if (msgb)
				timemsg = timeprt(now, 
				    qup->q2e.q2e_val[QL_BLOCK].q2v_time, 8);
			else if ((qup->flags & QUOTA2) != 0 && vflag)
				timemsg = timeprt(0,
				    qup->q2e.q2e_val[QL_BLOCK].q2v_grace, 8);
			else
				timemsg = "";
				
			printf("%12s%9s%c%8s%9s%8s"
			    , nam
			    , intprt(qup->q2e.q2e_val[QL_BLOCK].q2v_cur
				,HN_B, hflag, 8)
			    , (msgb == NULL) ? ' ' : '*'
			    , intprt(qup->q2e.q2e_val[QL_BLOCK].q2v_softlimit
				, HN_B, hflag, 8)
			    , intprt(qup->q2e.q2e_val[QL_BLOCK].q2v_hardlimit
				, HN_B, hflag, 8)
			    , timemsg);

			if (msgi)
				timemsg = timeprt(now, 
				    qup->q2e.q2e_val[QL_FILE].q2v_time, 8);
			else if ((qup->flags & QUOTA2) != 0 && vflag)
				timemsg = timeprt(0,
				    qup->q2e.q2e_val[QL_FILE].q2v_grace, 8);
			else
				timemsg = "";
				
			printf("%8s%c%7s%8s%8s\n"
			    , intprt(qup->q2e.q2e_val[QL_FILE].q2v_cur
				, 0, hflag, 7)
			    , (msgi == NULL) ? ' ' : '*'
			    , intprt(qup->q2e.q2e_val[QL_FILE].q2v_softlimit
				, 0, hflag, 7)
			    , intprt(qup->q2e.q2e_val[QL_FILE].q2v_hardlimit
				, 0, hflag, 7)
			    , timemsg);
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
	int8_t version;

	qup = quphead = quptail = NULL;

	nfst = getmntinfo(&fst, MNT_WAIT);
	if (nfst == 0)
		errx(2, "no filesystems mounted!");
	for (i = 0; i < nfst; i++) {
		if (qup == NULL) {
			if ((qup =
			    (struct quotause *)malloc(sizeof *qup)) == NULL)
				errx(2, "out of memory");
		}
		if (strncmp(fst[i].f_fstypename, "nfs", 
		    sizeof(fst[i].f_fstypename)) == 0) {
			version = 0;
			if (getnfsquota(&fst[i], NULL, qup, id, quotatype) == 0)
				continue;
		} else if (strncmp(fst[i].f_fstypename, "ffs",
		    sizeof(fst[i].f_fstypename)) == 0 &&
		    (fst[i].f_flag & ST_QUOTA) != 0) {
			if (getvfsquota(fst[i].f_mntonname, &qup->q2e, &version,
			    id, quotatype, dflag, Dflag) == 0)
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
	if (qup)
		free(qup);
	return (quphead);
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
	struct dqblk dqblk;
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
		q2e->q2e_val[QL_BLOCK].q2v_hardlimit =
		dqblk.dqb_bhardlimit =
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_bhardlimit *
		    (gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsize / DEV_BSIZE);
		dqblk.dqb_bsoftlimit =
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsoftlimit *
		    (gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsize / DEV_BSIZE);
		dqblk.dqb_curblocks =
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_curblocks *
		    (gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsize / DEV_BSIZE);
			/* inodes */
		dqblk.dqb_ihardlimit =
			gq_rslt.getquota_rslt_u.gqr_rquota.rq_fhardlimit;
		dqblk.dqb_isoftlimit =
			gq_rslt.getquota_rslt_u.gqr_rquota.rq_fsoftlimit;
		dqblk.dqb_curinodes =
			gq_rslt.getquota_rslt_u.gqr_rquota.rq_curfiles;
			/* grace times */
		dqblk.dqb_btime =
		    tv.tv_sec + gq_rslt.getquota_rslt_u.gqr_rquota.rq_btimeleft;
		dqblk.dqb_itime =
		    tv.tv_sec + gq_rslt.getquota_rslt_u.gqr_rquota.rq_ftimeleft;
		dqblk2q2e(&dqblk, q2e);
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
