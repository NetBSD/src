/*	$NetBSD: nfs_boot.c,v 1.33.4.2 1997/09/01 21:02:52 thorpej Exp $	*/

/*-
 * Copyright (c) 1995, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Support for NFS diskless booting, specifically getting information
 * about where to mount root from, what pathnames, etc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <net/if.h>
#include <net/route.h>

#include <net/if_ether.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/if_inarp.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsdiskless.h>
#include <nfs/krpc.h>
#include <nfs/xdr_subs.h>
#include <nfs/nfs_var.h>

#include "ether.h"
#if NETHER == 0
/* XXX - Do we still need this?  How can I tell? -gwr */

int nfs_boot_init(nd, procp)
	struct nfs_diskless *nd;
	struct proc *procp;
{
	printf("nfs_boot: NETHER == 0\n");
	return (ENXIO);
}

int
nfs_boot_getfh(ndm)
	struct nfs_dlmount *ndm;
{
	return (ENXIO);
}

#else /* NETHER */

/*
 * There are two implementations of NFS diskless boot.
 * One implementation uses BOOTP (RFC951, RFC1048),
 * the other uses Sun RPC/bootparams.  See the files:
 *    nfs_bootp.c:   BOOTP (RFC951, RFC1048)
 *    nfs_bootsun.c: Sun RPC/bootparams
 *
 * The variable nfs_boot_rfc951 selects which to use.
 * This is defined as BSS so machine-dependent code
 * may provide a data definition to override this.
 */
int nfs_boot_rfc951; /* 0: BOOTP. 1: RARP/SUNRPC */

/* mountd RPC */
static int md_mount __P((struct sockaddr_in *mdsin, char *path,
	struct nfs_args *argp));

static void nfs_boot_defrt __P((struct in_addr *));


/*
 * Called with an empty nfs_diskless struct to be filled in.
 * Find an interface, determine its ip address (etc.) and
 * save all the boot parameters in the nfs_diskless struct.
 */
int
nfs_boot_init(nd, procp)
	struct nfs_diskless *nd;
	struct proc *procp;
{
	struct ifnet *ifp;
	int error;

	/*
	 * Find the network interface.
	 */
	ifp = ifunit(root_device->dv_xname);
	if (ifp == NULL) {
		printf("nfs_boot: '%s' not found\n",
		       root_device->dv_xname);
		return (ENXIO);
	}
	if (ifp->if_type != IFT_ETHER) {
		printf("nfs_boot: unknown I/F type %d\n", ifp->if_type);
		return (ENODEV);	/* Op. not supported by device */
	}

	if (nfs_boot_rfc951) {
		printf("nfs_boot: trying BOOTP/DHCP\n");
		error = nfs_bootdhcp(ifp, nd, procp);
	} else {
		printf("nfs_boot: trying RARP (and RPC/bootparam)\n");
		error = nfs_bootparam(ifp, nd, procp);
	}
	if (error)
		return (error);

	/*
	 * If the gateway address is set, add a default route.
	 * (The mountd RPCs may go across a gateway.)
	 */
	if (nd->nd_gwip.s_addr)
		nfs_boot_defrt(&nd->nd_gwip);

	return (0);
}

/*
 * Install a default route to the passed IP address.
 */
static void
nfs_boot_defrt(gw_ip)
	struct in_addr *gw_ip;
{
	struct sockaddr dst, gw, mask;
	struct sockaddr_in *sin;
	int error;

	/* Destination: (default) */
	bzero((caddr_t)&dst, sizeof(dst));
	dst.sa_len = sizeof(dst);
	dst.sa_family = AF_INET;
	/* Gateway: */
	bzero((caddr_t)&gw, sizeof(gw));
	sin = (struct sockaddr_in *)&gw;
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = gw_ip->s_addr;
	/* Mask: (zero length) */
	/* XXX - Just pass a null pointer? */
	bzero(&mask, sizeof(mask));

	/* add, dest, gw, mask, flags, 0 */
	error = rtrequest(RTM_ADD, &dst, &gw, &mask,
					  (RTF_UP | RTF_GATEWAY | RTF_STATIC), NULL);
	if (error) {
		printf("nfs_boot: add route, error=%d\n", error);
		error = 0;
	}
}

/*
 * Get an initial NFS file handle using Sun RPC/mountd.
 * Separate function because we used to call it twice.
 * (once for root and once for swap)
 */
int
nfs_boot_getfh(ndm)
	struct nfs_dlmount *ndm;	/* output */
{
	struct nfs_args *args;
	struct sockaddr_in *sin;
	char *pathname;
	int error;
	u_int16_t port;

	args = &ndm->ndm_args;

	/* Initialize mount args. */
	bzero((caddr_t) args, sizeof(*args));
	args->addr     = &ndm->ndm_saddr;
	args->addrlen  = args->addr->sa_len;
#ifdef NFS_BOOT_TCP
	args->sotype   = SOCK_STREAM;
#else
	args->sotype   = SOCK_DGRAM;
#endif
	args->fh       = ndm->ndm_fh;
	args->hostname = ndm->ndm_host;
	args->flags    = NFSMNT_RESVPORT | NFSMNT_NFSV3;

#ifdef	NFS_BOOT_OPTIONS
	args->flags    |= NFS_BOOT_OPTIONS;
#endif
#ifdef	NFS_BOOT_RWSIZE
	/*
	 * Reduce rsize,wsize for interfaces that consistently
	 * drop fragments of long UDP messages.  (i.e. wd8003).
	 * You can always change these later via remount.
	 */
	args->flags   |= NFSMNT_WSIZE | NFSMNT_RSIZE;
	args->wsize    = NFS_BOOT_RWSIZE;
	args->rsize    = NFS_BOOT_RWSIZE;
#endif

	/*
	 * Find the pathname part of the "server:pathname"
	 * string left in ndm->ndm_host by nfs_boot_init.
	 */
	pathname = strchr(ndm->ndm_host, ':');
	if (pathname == 0) {
		printf("nfs_boot: getfh - no pathname\n");
		return (EIO);
	}
	pathname++;

	/*
	 * Get file handle using RPC to mountd/mount
	 */
	sin = (struct sockaddr_in *)&ndm->ndm_saddr;
	error = md_mount(sin, pathname, args);
	if (error) {
		printf("nfs_boot: mountd `%s', error=%d\n",
		       ndm->ndm_host, error);
		return (error);
	}

	/* Set port number for NFS use. */
	/* XXX: NFS port is always 2049, right? */
#ifdef NFS_BOOT_TCP
retry:
#endif
	error = krpc_portmap(sin, NFS_PROG,
		    (args->flags & NFSMNT_NFSV3) ? NFS_VER3 : NFS_VER2,
		    (args->sotype == SOCK_STREAM) ? IPPROTO_TCP : IPPROTO_UDP,
		    &port);
	if (port == htons(0))
		error = EIO;
	if (error) {
#ifdef NFS_BOOT_TCP
		if (args->sotype == SOCK_STREAM) {
			args->sotype = SOCK_DGRAM;
			goto retry;
		}
#endif
		printf("nfs_boot: portmap NFS, error=%d\n", error);
		return (error);
	}
	sin->sin_port = port;
	return (0);
}


/*
 * RPC: mountd/mount
 * Given a server pathname, get an NFS file handle.
 * Also, sets sin->sin_port to the NFS service port.
 */
static int
md_mount(mdsin, path, argp)
	struct sockaddr_in *mdsin;		/* mountd server address */
	char *path;
	struct nfs_args *argp;
{
	/* The RPC structures */
	struct rdata {
		u_int32_t errno;
		union {
			u_int8_t  v2fh[NFSX_V2FH];
			struct {
				u_int32_t fhlen;
				u_int8_t  fh[1];
			} v3fh;
		} fh;
	} *rdata;
	struct mbuf *m;
	u_int8_t *fh;
	int minlen, error;
	int mntver;

	mntver = (argp->flags & NFSMNT_NFSV3) ? 3 : 2;
	do {
		/*
		 * Get port number for MOUNTD.
		 */
		error = krpc_portmap(mdsin, RPCPROG_MNT, mntver,
		                    IPPROTO_UDP, &mdsin->sin_port);
		if (error)
			continue;

		/* This mbuf is consumed by krpc_call. */
		m = xdr_string_encode(path, strlen(path));
		if (m == NULL)
			return ENOMEM;

		/* Do RPC to mountd. */
		error = krpc_call(mdsin, RPCPROG_MNT, mntver,
		                  RPCMNT_MOUNT, &m, NULL);
		if (error != EPROGMISMATCH)
			break;
		/* Try lower version of mountd. */
	} while (--mntver >= 1);
	if (error) {
		printf("nfs_boot: mountd error=%d\n", error);
		return error;
	}
	if (mntver != 3)
		argp->flags &= ~NFSMNT_NFSV3;

	/* The reply might have only the errno. */
	if (m->m_len < 4)
		goto bad;
	/* Have at least errno, so check that. */
	rdata = mtod(m, struct rdata *);
	error = fxdr_unsigned(u_int32_t, rdata->errno);
	if (error)
		goto out;

	/* Have errno==0, so the fh must be there. */
	if (mntver == 3) {
		argp->fhsize   = fxdr_unsigned(u_int32_t, rdata->fh.v3fh.fhlen);
		if (argp->fhsize > NFSX_V3FHMAX)
			goto bad;
		minlen = 2 * sizeof(u_int32_t) + argp->fhsize;
	} else {
		argp->fhsize   = NFSX_V2FH;
		minlen = sizeof(u_int32_t) + argp->fhsize;
	}

	if (m->m_len < minlen) {
		m = m_pullup(m, minlen);
		if (m == NULL)
			return(EBADRPC);
		rdata = mtod(m, struct rdata *);
	}

	fh = (mntver == 3) ?
		rdata->fh.v3fh.fh : rdata->fh.v2fh;
	bcopy(fh, argp->fh, argp->fhsize);

	goto out;

bad:
	error = EBADRPC;

out:
	m_freem(m);
	return error;
}

#endif /* NETHER */
