/*	$NetBSD: mount_nfs.c,v 1.37 2003/03/22 11:15:52 jdolecek Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__COPYRIGHT("@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_nfs.c	8.11 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: mount_nfs.c,v 1.37 2003/03/22 11:15:52 jdolecek Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <syslog.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>

#ifdef ISO
#include <netiso/iso.h>
#endif

#ifdef NFSKERB
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>
#endif

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#define _KERNEL
#include <nfs/nfs.h>
#undef _KERNEL
#include <nfs/nqnfs.h>
#include <nfs/nfsmount.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <mntopts.h>

#define	ALTF_BG		0x1
#define ALTF_CONN	0x2
#define ALTF_DUMBTIMR	0x4
#define ALTF_INTR	0x8
#define ALTF_KERB	0x10
#define ALTF_NFSV3	0x20
#define ALTF_RDIRPLUS	0x40
#define	ALTF_MNTUDP	0x80
#define ALTF_NORESPORT	0x100
#define ALTF_SEQPACKET	0x200
#define ALTF_NQNFS	0x400
#define ALTF_SOFT	0x800
#define ALTF_TCP	0x1000
#define ALTF_NFSV2	0x2000

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_FORCE,
	MOPT_UPDATE,
	MOPT_GETARGS,
	{ "bg", 0, ALTF_BG, 1 },
	{ "conn", 0, ALTF_CONN, 1 },
	{ "dumbtimer", 0, ALTF_DUMBTIMR, 1 },
	{ "intr", 0, ALTF_INTR, 1 },
#ifdef NFSKERB
	{ "kerb", 0, ALTF_KERB, 1 },
#endif
	{ "nfsv3", 0, ALTF_NFSV3, 1 },
	{ "rdirplus", 0, ALTF_RDIRPLUS, 1 },
	{ "mntudp", 0, ALTF_MNTUDP, 1 },
	{ "noresport", 0, ALTF_NORESPORT, 1 },
#ifdef ISO
	{ "seqpacket", 0, ALTF_SEQPACKET, 1 },
#endif
	{ "nqnfs", 0, ALTF_NQNFS, 1 },
	{ "soft", 0, ALTF_SOFT, 1 },
	{ "tcp", 0, ALTF_TCP, 1 },
	{ "nfsv2", 0, ALTF_NFSV2, 1 },
	{ NULL }
};

struct nfs_args nfsdefargs = {
	NFS_ARGSVERSION,
	(struct sockaddr *)0,
	sizeof (struct sockaddr_in),
	SOCK_DGRAM,
	0,
	(u_char *)0,
	0,
	NFSMNT_NFSV3|NFSMNT_NOCONN|NFSMNT_RESVPORT,
	NFS_WSIZE,
	NFS_RSIZE,
	NFS_READDIRSIZE,
	10,
	NFS_RETRANS,
	NFS_MAXGRPS,
	NFS_DEFRAHEAD,
	NQ_DEFLEASE,
	NQ_DEADTHRESH,
	(char *)0,
};

struct nfhret {
	u_long		stat;
	long		vers;
	long		auth;
	long		fhsize;
	u_char		nfh[NFSX_V3FHMAX];
};
#define	DEF_RETRY	10000
#define	BGRND	1
#define	ISBGRND	2
int retrycnt;
int opflags = 0;
int nfsproto = IPPROTO_UDP;
int force2 = 0;
int force3 = 0;
int mnttcp_ok = 1;

#ifdef NFSKERB
static char inst[INST_SZ];
static char realm[REALM_SZ];
static struct {
	u_long		kind;
	KTEXT_ST	kt;
} ktick;
static struct nfsrpc_nickverf kverf;
static struct nfsrpc_fullblock kin, kout;
static NFSKERBKEY_T kivec;
static CREDENTIALS kcr;
static struct timeval ktv;
static NFSKERBKEYSCHED_T kerb_keysched;
#endif

static void	shownfsargs __P((const struct nfs_args *));
static int	getnfsargs __P((char *, struct nfs_args *));
#ifdef ISO
static struct	iso_addr *iso_addr __P((const char *));
#endif
int	main __P((int, char *[]));
int	mount_nfs __P((int argc, char **argv));
/* void	set_rpc_maxgrouplist __P((int)); */
static void	usage __P((void));
static int	xdr_dir __P((XDR *, char *));
static int	xdr_fh __P((XDR *, struct nfhret *));

#ifndef MOUNT_NOMAIN
int
main(argc, argv)
	int argc;
	char **argv;
{
	return mount_nfs(argc, argv);
}
#endif

int
mount_nfs(argc, argv)
	int argc;
	char *argv[];
{
	int c, retval;
	struct nfs_args *nfsargsp;
	struct nfs_args nfsargs;
	struct nfsd_cargs ncd;
	struct sockaddr_storage sa;
	int mntflags, altflags, i, nfssvc_flag, num;
	char *name, *p, *spec, *ospec;
#ifdef NFSKERB
	uid_t last_ruid;

	last_ruid = -1;
	if (krb_get_lrealm(realm, 0) != KSUCCESS)
	    (void)strcpy(realm, KRB_REALM);
	if (sizeof (struct nfsrpc_nickverf) != RPCX_NICKVERF ||
	    sizeof (struct nfsrpc_fullblock) != RPCX_FULLBLOCK ||
	    ((char *)&ktick.kt) - ((char *)&ktick) != NFSX_UNSIGNED ||
	    ((char *)ktick.kt.dat) - ((char *)&ktick) != 2 * NFSX_UNSIGNED)
		warnx("Yikes! NFSKERB structs not packed!!\n");
#endif
	retrycnt = DEF_RETRY;

	mntflags = 0;
	altflags = 0;
	nfsargs = nfsdefargs;
	nfsargsp = &nfsargs;
	while ((c = getopt(argc, argv,
	    "23a:bcCdD:g:I:iKL:lm:o:PpqR:r:sTt:w:x:UX")) != -1)
		switch (c) {
		case '3':
			if (force2)
				errx(1, "-2 and -3 are mutually exclusive");
			force3 = 1;
			break;
		case '2':
			if (force3)
				errx(1, "-2 and -3 are mutually exclusive");
			force2 = 1;
			nfsargsp->flags &= ~NFSMNT_NFSV3;
			break;
		case 'a':
			num = strtol(optarg, &p, 10);
			if (*p || num < 0)
				errx(1, "illegal -a value -- %s", optarg);
			nfsargsp->readahead = num;
			nfsargsp->flags |= NFSMNT_READAHEAD;
			break;
		case 'b':
			opflags |= BGRND;
			break;
		case 'c':
			nfsargsp->flags |= NFSMNT_NOCONN;
			break;
		case 'C':
			nfsargsp->flags &= ~NFSMNT_NOCONN;
			break;
		case 'D':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -D value -- %s", optarg);
			nfsargsp->deadthresh = num;
			nfsargsp->flags |= NFSMNT_DEADTHRESH;
			break;
		case 'd':
			nfsargsp->flags |= NFSMNT_DUMBTIMR;
			break;
#if 0 /* XXXX */
		case 'g':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -g value -- %s", optarg);
			set_rpc_maxgrouplist(num);
			nfsargsp->maxgrouplist = num;
			nfsargsp->flags |= NFSMNT_MAXGRPS;
			break;
#endif
		case 'I':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -I value -- %s", optarg);
			nfsargsp->readdirsize = num;
			nfsargsp->flags |= NFSMNT_READDIRSIZE;
			break;
		case 'i':
			nfsargsp->flags |= NFSMNT_INT;
			break;
#ifdef NFSKERB
		case 'K':
			nfsargsp->flags |= NFSMNT_KERB;
			break;
#endif
		case 'L':
			num = strtol(optarg, &p, 10);
			if (*p || num < 2)
				errx(1, "illegal -L value -- %s", optarg);
			nfsargsp->leaseterm = num;
			nfsargsp->flags |= NFSMNT_LEASETERM;
			break;
		case 'l':
			nfsargsp->flags |= NFSMNT_RDIRPLUS;
			break;
#ifdef NFSKERB
		case 'm':
			(void)strncpy(realm, optarg, REALM_SZ - 1);
			realm[REALM_SZ - 1] = '\0';
			break;
#endif
		case 'o':
			getmntopts(optarg, mopts, &mntflags, &altflags);
			if (altflags & ALTF_BG)
				opflags |= BGRND;
			if (altflags & ALTF_CONN)
				nfsargsp->flags &= ~NFSMNT_NOCONN;
			if (altflags & ALTF_DUMBTIMR)
				nfsargsp->flags |= NFSMNT_DUMBTIMR;
			if (altflags & ALTF_INTR)
				nfsargsp->flags |= NFSMNT_INT;
#ifdef NFSKERB
			if (altflags & ALTF_KERB)
				nfsargsp->flags |= NFSMNT_KERB;
#endif
			if (altflags & ALTF_NFSV3) {
				if (force2)
					errx(1, "conflicting version options");
				force3 = 1;
			}
			if (altflags & ALTF_NFSV2) {
				if (force3)
					errx(1, "conflicting version options");
				force2 = 1;
				nfsargsp->flags &= ~NFSMNT_NFSV3;
			}
			if (altflags & ALTF_RDIRPLUS)
				nfsargsp->flags |= NFSMNT_RDIRPLUS;
			if (altflags & ALTF_MNTUDP)
				mnttcp_ok = 0;
			if (altflags & ALTF_NORESPORT)
				nfsargsp->flags &= ~NFSMNT_RESVPORT;
#ifdef ISO
			if (altflags & ALTF_SEQPACKET)
				nfsargsp->sotype = SOCK_SEQPACKET;
#endif
			if (altflags & ALTF_NQNFS) {
				if (force2)
					errx(1, "nqnfs only available with v3");
				force3 = 1;
				nfsargsp->flags |= NFSMNT_NQNFS;
			}
			if (altflags & ALTF_SOFT)
				nfsargsp->flags |= NFSMNT_SOFT;
			if (altflags & ALTF_TCP) {
				nfsargsp->sotype = SOCK_STREAM;
				nfsproto = IPPROTO_TCP;
			}
			altflags = 0;
			break;
		case 'P':
			nfsargsp->flags |= NFSMNT_RESVPORT;
			break;
		case 'p':
			nfsargsp->flags &= ~NFSMNT_RESVPORT;
			break;
		case 'q':
			if (force2)
				errx(1, "nqnfs only available with v3");
			force3 = 1;
			nfsargsp->flags |= NFSMNT_NQNFS;
			break;
		case 'R':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -R value -- %s", optarg);
			retrycnt = num;
			break;
		case 'r':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -r value -- %s", optarg);
			nfsargsp->rsize = num;
			nfsargsp->flags |= NFSMNT_RSIZE;
			break;
#ifdef ISO
		case 'S':
			nfsargsp->sotype = SOCK_SEQPACKET;
			break;
#endif
		case 's':
			nfsargsp->flags |= NFSMNT_SOFT;
			break;
		case 'T':
			nfsargsp->sotype = SOCK_STREAM;
			nfsproto = IPPROTO_TCP;
			break;
		case 't':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -t value -- %s", optarg);
			nfsargsp->timeo = num;
			nfsargsp->flags |= NFSMNT_TIMEO;
			break;
		case 'w':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -w value -- %s", optarg);
			nfsargsp->wsize = num;
			nfsargsp->flags |= NFSMNT_WSIZE;
			break;
		case 'x':
			num = strtol(optarg, &p, 10);
			if (*p || num <= 0)
				errx(1, "illegal -x value -- %s", optarg);
			nfsargsp->retrans = num;
			nfsargsp->flags |= NFSMNT_RETRANS;
			break;
		case 'X':
			nfsargsp->flags |= NFSMNT_XLATECOOKIE;
			break;
		case 'U':
			mnttcp_ok = 0;
			break;
		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	spec = *argv++;
	name = *argv;
	if ((ospec = strdup(spec)) == NULL) {
		err(1, "strdup");
	}

	if ((mntflags & MNT_GETARGS) != 0) {
		memset(&sa, 0, sizeof(sa));
		nfsargsp->addr = (struct sockaddr *)&sa;
		nfsargsp->addrlen = sizeof(sa);
	} else {
		if (!getnfsargs(spec, nfsargsp))
			exit(1);
	}
	if ((retval = mount(MOUNT_NFS, name, mntflags, nfsargsp))) {
		/* Did we just default to v3 on a v2-only kernel?
		 * If so, default to v2 & try again */
		if ((errno == EPROGMISMATCH) && !force3) {
			nfsargsp->flags &= ~NFSMNT_NFSV3;
			retval = mount(MOUNT_NFS, name, mntflags, nfsargsp);
		}
	}
	if (retval)
		err(1, "%s on %s", ospec, name);
	if (mntflags & MNT_GETARGS) {
		shownfsargs(nfsargsp);
		return (0);
	}
		
	if (nfsargsp->flags & (NFSMNT_NQNFS | NFSMNT_KERB)) {
		if ((opflags & ISBGRND) == 0) {
			if ((i = fork()) != 0) {
				if (i == -1)
					err(1, "nqnfs 1");
				exit(0);
			}
			(void) setsid();
			(void) close(STDIN_FILENO);
			(void) close(STDOUT_FILENO);
			(void) close(STDERR_FILENO);
			(void) chdir("/");
		}
		openlog("mount_nfs", LOG_PID, LOG_DAEMON);
		nfssvc_flag = NFSSVC_MNTD;
		ncd.ncd_dirp = name;
		while (nfssvc(nfssvc_flag, (caddr_t)&ncd) < 0) {
			if (errno != ENEEDAUTH) {
				syslog(LOG_ERR, "nfssvc err %m");
				continue;
			}
			nfssvc_flag =
			    NFSSVC_MNTD | NFSSVC_GOTAUTH | NFSSVC_AUTHINFAIL;
#ifdef NFSKERB
			/*
			 * Set up as ncd_authuid for the kerberos call.
			 * Must set ruid to ncd_authuid and reset the
			 * ticket name iff ncd_authuid is not the same
			 * as last time, so that the right ticket file
			 * is found.
			 * Get the Kerberos credential structure so that
			 * we have the seesion key and get a ticket for
			 * this uid.
			 * For more info see the IETF Draft "Authentication
			 * in ONC RPC".
			 */
			if (ncd.ncd_authuid != last_ruid) {
				krb_set_tkt_string("");
				last_ruid = ncd.ncd_authuid;
			}
			setreuid(ncd.ncd_authuid, 0);
			kret = krb_get_cred(NFS_KERBSRV, inst, realm, &kcr);
			if (kret == RET_NOTKT) {
		            kret = get_ad_tkt(NFS_KERBSRV, inst, realm,
				DEFAULT_TKT_LIFE);
			    if (kret == KSUCCESS)
				kret = krb_get_cred(NFS_KERBSRV, inst, realm,
				    &kcr);
			}
			if (kret == KSUCCESS)
			    kret = krb_mk_req(&ktick.kt, NFS_KERBSRV, inst,
				realm, 0);

			/*
			 * Fill in the AKN_FULLNAME authenticator and verfier.
			 * Along with the Kerberos ticket, we need to build
			 * the timestamp verifier and encrypt it in CBC mode.
			 */
			if (kret == KSUCCESS &&
			    ktick.kt.length <= (RPCAUTH_MAXSIZ-3*NFSX_UNSIGNED)
			    && gettimeofday(&ktv, (struct timezone *)0) == 0) {
			    ncd.ncd_authtype = RPCAUTH_KERB4;
			    ncd.ncd_authstr = (u_char *)&ktick;
			    ncd.ncd_authlen = nfsm_rndup(ktick.kt.length) +
				3 * NFSX_UNSIGNED;
			    ncd.ncd_verfstr = (u_char *)&kverf;
			    ncd.ncd_verflen = sizeof (kverf);
			    memmove(ncd.ncd_key, kcr.session,
				sizeof (kcr.session));
			    kin.t1 = htonl(ktv.tv_sec);
			    kin.t2 = htonl(ktv.tv_usec);
			    kin.w1 = htonl(NFS_KERBTTL);
			    kin.w2 = htonl(NFS_KERBTTL - 1);
			    memset((caddr_t)kivec, 0, sizeof (kivec));

			    /*
			     * Encrypt kin in CBC mode using the session
			     * key in kcr.
			     */
			    XXX

			    /*
			     * Finally, fill the timestamp verifier into the
			     * authenticator and verifier.
			     */
			    ktick.kind = htonl(RPCAKN_FULLNAME);
			    kverf.kind = htonl(RPCAKN_FULLNAME);
			    NFS_KERBW1(ktick.kt) = kout.w1;
			    ktick.kt.length = htonl(ktick.kt.length);
			    kverf.verf.t1 = kout.t1;
			    kverf.verf.t2 = kout.t2;
			    kverf.verf.w2 = kout.w2;
			    nfssvc_flag = NFSSVC_MNTD | NFSSVC_GOTAUTH;
			}
			setreuid(0, 0);
#endif /* NFSKERB */
		}
	}
	exit(0);
}

static void
shownfsargs(nfsargsp)
	const struct nfs_args *nfsargsp;
{
	char fbuf[2048];
	char host[NI_MAXHOST], serv[NI_MAXSERV];
	int error;

	(void)snprintb(fbuf, sizeof(fbuf), NFSMNT_BITS, nfsargsp->flags);
	if (nfsargsp->addr != NULL) {
		error = getnameinfo(nfsargsp->addr, nfsargsp->addrlen, host,
		    sizeof(host), serv, sizeof(serv),
		    NI_NUMERICHOST | NI_NUMERICSERV);
		if (error != 0)
			warnx("getnameinfo: %s", gai_strerror(error));
	} else
		error = -1;

	printf("version=%d", nfsargsp->version);
	if (error == 0)
		printf(", addr=%s, port=%s, addrlen=%d",
		    host, serv, nfsargsp->addrlen);
	printf(", sotype=%d, proto=%d, fhsize=%d, "
	    "flags=%s, wsize=%d, rsize=%d, readdirsize=%d, timeo=%d, "
	    "retrans=%d, maxgrouplist=%d, readahead=%d, leaseterm=%d, "
	    "deadthresh=%d\n",
	    nfsargsp->sotype,
	    nfsargsp->proto,
	    nfsargsp->fhsize,
	    fbuf,
	    nfsargsp->wsize,
	    nfsargsp->rsize,
	    nfsargsp->readdirsize,
	    nfsargsp->timeo,
	    nfsargsp->retrans,
	    nfsargsp->maxgrouplist,
	    nfsargsp->readahead,
	    nfsargsp->leaseterm,
	    nfsargsp->deadthresh);
}

static int
getnfsargs(spec, nfsargsp)
	char *spec;
	struct nfs_args *nfsargsp;
{
	CLIENT *clp;
	struct addrinfo hints, *ai_nfs, *ai;
	int ecode;
	char host[NI_MAXHOST], serv[NI_MAXSERV];
	static struct netbuf nfs_nb;
	static struct sockaddr_storage nfs_ss;
	struct netconfig *nconf;
	char *netid;
#ifdef ISO
	static struct sockaddr_iso isoaddr;
	struct iso_addr *isop;
	int isoflag = 0;
#endif
	struct timeval pertry, try;
	enum clnt_stat clnt_stat;
	int i, nfsvers, mntvers, orgcnt;
	char *hostp, *delimp;
#ifdef NFSKERB
	char *cp;
#endif
	static struct nfhret nfhret;
	static char nam[MNAMELEN + 1];

	strncpy(nam, spec, MNAMELEN);
	nam[MNAMELEN] = '\0';
	if ((delimp = strchr(spec, '@')) != NULL) {
		hostp = delimp + 1;
	} else if ((delimp = strrchr(spec, ':')) != NULL) {
		hostp = spec;
		spec = delimp + 1;
	} else {
		warnx("no <host>:<dirpath> or <dirpath>@<host> spec");
		return (0);
	}
	*delimp = '\0';
	/*
	 * DUMB!! Until the mount protocol works on iso transport, we must
	 * supply both an iso and an inet address for the host.
	 */
#ifdef ISO
	if (!strncmp(hostp, "iso=", 4)) {
		u_short isoport;

		hostp += 4;
		isoflag++;
		if ((delimp = strchr(hostp, '+')) == NULL) {
			warnx("no iso+inet address");
			return (0);
		}
		*delimp = '\0';
		if ((isop = iso_addr(hostp)) == NULL) {
			warnx("bad ISO address");
			return (0);
		}
		memset(&isoaddr, 0, sizeof (isoaddr));
		memcpy(&isoaddr.siso_addr, isop, sizeof (struct iso_addr));
		isoaddr.siso_len = sizeof (isoaddr);
		isoaddr.siso_family = AF_ISO;
		isoaddr.siso_tlen = 2;
		isoport = htons(NFS_PORT);
		memcpy(TSEL(&isoaddr), &isoport, isoaddr.siso_tlen);
		hostp = delimp + 1;
	}
#endif /* ISO */

	/*
	 * Handle an internet host address and reverse resolve it if
	 * doing Kerberos.
	 */
	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_socktype = nfsargsp->sotype;
	if (getaddrinfo(hostp, "nfs", &hints, &ai_nfs) == 0) {
		if ((nfsargsp->flags & NFSMNT_KERB)) {
			hints.ai_flags = 0;
			if (getnameinfo(ai->ai_addr, ai->ai_addrlen, host,
			    sizeof host, serv, sizeof serv, 0) != 0) {
				warnx("can't reverse resolve net address for "
				    "host \"%s\": %s", hostp,
				    gai_strerror(ecode));
				return (0);
			}
			hostp = host;
		}
	} else {
		hints.ai_flags = 0;
		if ((ecode = getaddrinfo(hostp, "nfs", &hints, &ai_nfs)) != 0) {
			warnx("can't get net id for host \"%s\": %s", hostp,
			    gai_strerror(ecode));
			return (0);
		}
	}
#ifdef NFSKERB
	if (nfsargsp->flags & NFSMNT_KERB) {
		strncpy(inst, hp->h_name, INST_SZ);
		inst[INST_SZ - 1] = '\0';
		if (cp = strchr(inst, '.'))
			*cp = '\0';
	}
#endif /* NFSKERB */

	if (force2) {
		nfsvers = NFS_VER2;
		mntvers = RPCMNT_VER1;
	} else {
		nfsvers = NFS_VER3;
		mntvers = RPCMNT_VER3;
	}
	orgcnt = retrycnt;
	nfhret.stat = EACCES;	/* Mark not yet successful */

    for (ai = ai_nfs; ai; ai = ai->ai_next) {
	/*
	 * XXX. Nead a generic (family, type, proto) -> nconf interface.
	 * __rpc_*2nconf exist, maybe they should be exported.
	 */
	if (nfsargsp->sotype == SOCK_STREAM) {
		if (ai->ai_family == AF_INET6)	
			netid = "tcp6";
		else
			netid = "tcp";
	} else {
		if (ai->ai_family == AF_INET6)
			netid = "udp6";
		else
			netid = "udp";
	}

	nconf = getnetconfigent(netid);

tryagain:
	retrycnt = orgcnt;

	while (retrycnt > 0) {
		nfs_nb.buf = &nfs_ss;
		nfs_nb.maxlen = sizeof nfs_ss;
		if (!rpcb_getaddr(RPCPROG_NFS, nfsvers, nconf, &nfs_nb, hostp)){
			if (rpc_createerr.cf_stat == RPC_SYSTEMERROR) {
				nfhret.stat = rpc_createerr.cf_error.re_errno;
				break;
			}
			if (rpc_createerr.cf_stat == RPC_UNKNOWNPROTO) {
				nfhret.stat = EPROTONOSUPPORT;
				break;
			}
			if ((opflags & ISBGRND) == 0)
				clnt_pcreateerror(
				    "mount_nfs: rpcbind to nfs on server");
		} else {
			pertry.tv_sec = 10;
			pertry.tv_usec = 0;
			/*
			 * XXX relies on clnt_tcp_create to bind to a reserved
			 * socket.
			 */
			clp = clnt_tp_create(hostp, RPCPROG_MNT, mntvers,
			     mnttcp_ok ? nconf : getnetconfigent("udp"));
			if (clp == NULL) {
				if ((opflags & ISBGRND) == 0) {
					clnt_pcreateerror(
					    "Cannot MNT RPC (mountd)");
				}
			} else {
				CLNT_CONTROL(clp, CLSET_RETRY_TIMEOUT,
				    (char *)&pertry);
				clp->cl_auth = authsys_create_default();
				try.tv_sec = 10;
				try.tv_usec = 0;
				if (nfsargsp->flags & NFSMNT_KERB)
				    nfhret.auth = RPCAUTH_KERB4;
				else
				    nfhret.auth = RPCAUTH_UNIX;
				nfhret.vers = mntvers;
				clnt_stat = clnt_call(clp, RPCMNT_MOUNT,
				    xdr_dir, spec, xdr_fh, &nfhret, try);
				switch (clnt_stat) {
				case RPC_PROGVERSMISMATCH:
					if (nfsvers == NFS_VER3 && !force3) {
						nfsvers = NFS_VER2;
						mntvers = RPCMNT_VER1;
						nfsargsp->flags &=
							~NFSMNT_NFSV3;
						goto tryagain;
					} else {
						errx(1, "%s", clnt_sperror(clp,
							"MNT RPC"));
					}
				case RPC_SUCCESS:
					auth_destroy(clp->cl_auth);
					clnt_destroy(clp);
					retrycnt = 0;
					break;
				default:
					/* XXX should give up on some errors */
					if ((opflags & ISBGRND) == 0)
						warnx("%s", clnt_sperror(clp,
						    "bad MNT RPC"));
					break;
				}
			}
		}
		if (--retrycnt > 0) {
			if (opflags & BGRND) {
				opflags &= ~BGRND;
				if ((i = fork()) != 0) {
					if (i == -1)
						err(1, "nqnfs 2");
					exit(0);
				}
				(void) setsid();
				(void) close(STDIN_FILENO);
				(void) close(STDOUT_FILENO);
				(void) close(STDERR_FILENO);
				(void) chdir("/");
				opflags |= ISBGRND;
			}
			sleep(60);
		}
	}
	if (nfhret.stat == 0)
		break;
    }
	freeaddrinfo(ai_nfs);
	if (nfhret.stat) {
		if (opflags & ISBGRND)
			exit(1);
		errno = nfhret.stat;
		warnx("can't access %s: %s", spec, strerror(nfhret.stat));
		return (0);
	}
#ifdef ISO
	if (isoflag) {
		nfsargsp->addr = (struct sockaddr *) &isoaddr;
		nfsargsp->addrlen = sizeof (isoaddr);
	} else
#endif /* ISO */
	{
		nfsargsp->addr = (struct sockaddr *) nfs_nb.buf;
		nfsargsp->addrlen = nfs_nb.len;
	}
	nfsargsp->fh = nfhret.nfh;
	nfsargsp->fhsize = nfhret.fhsize;
	nfsargsp->hostname = nam;
	return (1);
}

/*
 * xdr routines for mount rpc's
 */
static int
xdr_dir(xdrsp, dirp)
	XDR *xdrsp;
	char *dirp;
{
	return (xdr_string(xdrsp, &dirp, RPCMNT_PATHLEN));
}

static int
xdr_fh(xdrsp, np)
	XDR *xdrsp;
	struct nfhret *np;
{
	int i;
	long auth, authcnt, authfnd = 0;

	if (!xdr_u_long(xdrsp, &np->stat))
		return (0);
	if (np->stat)
		return (1);
	switch (np->vers) {
	case 1:
		np->fhsize = NFSX_V2FH;
		return (xdr_opaque(xdrsp, (caddr_t)np->nfh, NFSX_V2FH));
	case 3:
		if (!xdr_long(xdrsp, &np->fhsize))
			return (0);
		if (np->fhsize <= 0 || np->fhsize > NFSX_V3FHMAX)
			return (0);
		if (!xdr_opaque(xdrsp, (caddr_t)np->nfh, np->fhsize))
			return (0);
		if (!xdr_long(xdrsp, &authcnt))
			return (0);
		for (i = 0; i < authcnt; i++) {
			if (!xdr_long(xdrsp, &auth))
				return (0);
			if (auth == np->auth)
				authfnd++;
		}
		/*
		 * Some servers, such as DEC's OSF/1 return a nil authenticator
		 * list to indicate RPCAUTH_UNIX.
		 */
		if (!authfnd && (authcnt > 0 || np->auth != RPCAUTH_UNIX))
			np->stat = EAUTH;
		return (1);
	};
	return (0);
}

static void
usage()
{
	(void)fprintf(stderr, "usage: mount_nfs %s\n%s\n%s\n%s\n%s\n",
"[-23bcCdiKlpPqsTUX] [-a maxreadahead] [-D deadthresh]",
"\t[-g maxgroups] [-I readdirsize] [-L leaseterm] [-m realm]",
"\t[-o options] [-R retrycnt] [-r readsize] [-t timeout]",
"\t[-w writesize] [-x retrans]",
"\trhost:path node");
	exit(1);
}
