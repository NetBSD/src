/*	$NetBSD: getnfsargs.c,v 1.18 2017/02/05 00:24:24 christos Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1992, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_nfs.c	8.11 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: getnfsargs.c,v 1.18 2017/02/05 00:24:24 christos Exp $");
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

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>

#include <arpa/inet.h>

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

#include "mount_nfs.h"

int retrycnt = DEF_RETRY; 
int opflags = 0;
int force2 = 0;
int force3 = 0;
int mnttcp_ok = 1;
int port = 0;

struct nfhret {
	u_long		stat;
	long		vers;
	long		auth;
	long		fhsize;
	u_char		nfh[NFSX_V3FHMAX];
};

static int	xdr_dir(XDR *, char *);
static int	xdr_fh(XDR *, struct nfhret *);

#ifndef MOUNTNFS_RETRYRPC
#define MOUNTNFS_RETRYRPC 60
#endif

int
getnfsargs(char *spec, struct nfs_args *nfsargsp)
{
	CLIENT *clp;
	struct addrinfo hints, *ai_nfs, *ai;
	int ecode;
	static struct netbuf nfs_nb;
	static struct sockaddr_storage nfs_ss;
	struct netconfig *nconf;
	const char *netid;
	struct timeval pertry, try;
	enum clnt_stat clnt_stat;
	int i, nfsvers, mntvers;
	int retryleft;
	char *hostp, *delimp;
	static struct nfhret nfhret;
	static char nam[MNAMELEN + 1];

	strlcpy(nam, spec, sizeof(nam));
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
	 * Handle an internet host address.
	 */
	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_socktype = nfsargsp->sotype;
	if (getaddrinfo(hostp, "nfs", &hints, &ai_nfs) != 0) {
		hints.ai_flags = 0;
		if ((ecode = getaddrinfo(hostp, "nfs", &hints, &ai_nfs)) != 0) {
			warnx("can't get net id for host \"%s\": %s", hostp,
			    gai_strerror(ecode));
			return (0);
		}
	}

	if ((nfsargsp->flags & NFSMNT_NFSV3) != 0) {
		nfsvers = NFS_VER3;
		mntvers = RPCMNT_VER3;
	} else {
		nfsvers = NFS_VER2;
		mntvers = RPCMNT_VER1;
	}
	nfhret.stat = EACCES;	/* Mark not yet successful */

    for (ai = ai_nfs; ai; ai = ai->ai_next) {
	/*
	 * XXX. Need a generic (family, type, proto) -> nconf interface.
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
	retryleft = retrycnt;

	while (retryleft > 0) {
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
			if ((opflags & ISBGRND) == 0) {
				char buf[64];

				snprintf(buf, sizeof(buf),
				    "%s: rpcbind to nfs on server",
				    getprogname());
				clnt_pcreateerror(buf);
			}
		} else {
			pertry.tv_sec = 30;
			pertry.tv_usec = 0;
			/*
			 * XXX relies on clnt_tcp_create to bind to a reserved
			 * socket.
			 */
			clp = clnt_tp_create(hostp, RPCPROG_MNT, mntvers,
			     mnttcp_ok ? nconf : getnetconfigent(netid));
			if (clp == NULL) {
				if ((opflags & ISBGRND) == 0) {
					clnt_pcreateerror(
					    "Cannot MNT RPC (mountd)");
				}
			} else {
				CLNT_CONTROL(clp, CLSET_RETRY_TIMEOUT,
				    (char *)&pertry);
				clp->cl_auth = authsys_create_default();
				try.tv_sec = 30;
				try.tv_usec = 0;
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
					retryleft = 0;
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
		if (--retryleft > 0) {
			if (opflags & BGRND) {
				opflags &= ~BGRND;
				if ((i = fork()) != 0) {
					if (i == -1)
						err(1, "fork");
					exit(0);
				}
				(void) setsid();
				(void) close(STDIN_FILENO);
				(void) close(STDOUT_FILENO);
				(void) close(STDERR_FILENO);
				(void) chdir("/");
				opflags |= ISBGRND;
			}
			sleep(MOUNTNFS_RETRYRPC);
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
	} else {
		nfsargsp->addr = (struct sockaddr *) nfs_nb.buf;
		nfsargsp->addrlen = nfs_nb.len;
		if (port != 0) {
			struct sockaddr *sa = nfsargsp->addr;
			switch (sa->sa_family) {
			case AF_INET:
				((struct sockaddr_in *)sa)->sin_port = port;
				break;
#ifdef INET6
			case AF_INET6:
				((struct sockaddr_in6 *)sa)->sin6_port = port;
				break;
#endif
			default:
				errx(1, "Unsupported socket family %d",
				    sa->sa_family);
			}
		}
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
xdr_dir(XDR *xdrsp, char *dirp)
{
	return (xdr_string(xdrsp, &dirp, RPCMNT_PATHLEN));
}

static int
xdr_fh(XDR *xdrsp, struct nfhret *np)
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
