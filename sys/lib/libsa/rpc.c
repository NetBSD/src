/*	$NetBSD: rpc.c,v 1.4 1995/02/20 11:04:18 mycroft Exp $	*/

/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *	California, Lawrence Berkeley Laboratory and its contributors.
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
 *
 * @(#) Header: rpc.c,v 1.12 93/09/28 08:31:56 leres Exp  (LBL)
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsv2.h>
#include <nfs/xdr_subs.h>

#include <string.h>

#include "stand.h"
#include "net.h"
#include "netif.h"
#include "rpc.h"

/* XXX Data part of nfs rpc reply (also the largest thing we receive) */
struct nfs_reply_data {
	u_long	errno;
	u_char	fa[NFSX_FATTR(0)];
	u_long	count;
	u_char	data[1200];
};
#define NFSREAD_SIZE sizeof(((struct nfs_reply_data *)0)->data)

/* Cache stuff */
#define PMAP_NUM 8			/* need at most 5 pmap entries */

static struct pmap_list {
	u_long	addr;			/* address of server */
	u_long	prog;
	u_long	vers;
	u_short	port;			/* cached port for service */
} pmap_list[PMAP_NUM] = {
	{ 0, PMAPPROG, PMAPVERS, PMAPPORT }
};
static	int pmap_num = 1;

/* Local forwards */
static	size_t recvrpc __P((struct iodesc *, void *, size_t, time_t));

/* Make a rpc call; return length of answer */
size_t
callrpc(d, prog, vers, proc, sdata, slen, rdata, rlen)
	register struct iodesc *d;
	register u_long prog, vers, proc;
	register void *sdata;
	register size_t slen;
	register void *rdata;
	register size_t rlen;
{
	register size_t cc;
	register u_long *tl;
	struct {
		u_char	header[HEADER_SIZE];
		u_long	data[96];	/* XXX */
	} wbuf;
	struct {
		u_char	header[HEADER_SIZE];
		struct	rpc_reply rrpc;
		union {
			u_long	errno;
			u_char	data[sizeof(struct nfs_reply_data)];
		} ru;
	} rbuf;

#ifdef RPC_DEBUG
	if (debug)
		printf("callrpc: called\n");
#endif
	if (rlen > sizeof(rbuf.ru.data))
		panic("callrpc: huge read (%d > %d)",
		    rlen, sizeof(rbuf.ru.data));

	d->destport = getport(d, prog, vers);

	tl = wbuf.data;

	/* Fill in RPC call structure. */
	*tl++ = txdr_unsigned(d->xid);
	*tl++ = txdr_unsigned(RPC_CALL);
	*tl++ = txdr_unsigned(RPC_VER2);
	*tl++ = txdr_unsigned(prog);
	*tl++ = txdr_unsigned(vers);
	*tl++ = txdr_unsigned(proc);

	/* Fill in authorization info. */
	if (prog != NFS_PROG) {
		*tl++ = txdr_unsigned(RPCAUTH_NULL);
		*tl++ = txdr_unsigned(0);
	} else {
		*tl++ = txdr_unsigned(RPCAUTH_UNIX);
		*tl++ = txdr_unsigned(3*NFSX_UNSIGNED);
		*tl++ = txdr_unsigned(0);	/* time */
		*tl++ = txdr_unsigned(0);	/* host name length */
		*tl++ = txdr_unsigned(0);	/* uid */
		*tl++ = txdr_unsigned(0);	/* gid */
		*tl++ = txdr_unsigned(0);	/* gid list length */
	}
	*tl++ = txdr_unsigned(RPCAUTH_NULL);
	*tl++ = txdr_unsigned(0);

	/* Fill in RPC call arguments. */
	bcopy(sdata, tl, slen);

	cc = sendrecv(d,
	    sendudp, wbuf.data,
	    (tl - wbuf.data) * NFSX_UNSIGNED + slen,
	    recvrpc, &rbuf.rrpc,
	    sizeof(struct rpc_reply) + sizeof(struct nfs_reply_data));

#ifdef RPC_DEBUG
	if (debug)
		printf("callrpc: cc=%d rlen=%d, rp_stat=%d\n", cc, rlen,
		    rbuf.rrpc.rp_stat);
#endif

	/* Bump xid so next request will be unique. */
	++d->xid;

	if (cc == -1)
		return (-1);
	if (cc < rlen) {
		/* Check for an error return */
		if (cc >= sizeof(rbuf.ru.errno) && rbuf.ru.errno != 0) {
			errno = ntohl(rbuf.ru.errno);
			return (-1);
		}
		panic("callrpc: missing data (%d < %d)", cc, rlen);
	}
	if (cc > sizeof(rbuf.ru.data))
		panic("callrpc: huge return (%d > %d)",
		    cc, sizeof(rbuf.ru.data));
	bcopy(rbuf.ru.data, rdata, cc);
	return (cc);
}

/* Returns true if packet is the one we're waiting for */
static size_t
recvrpc(d, pkt, len, tleft)
	register struct iodesc *d;
	register void *pkt;
	register size_t len;
	time_t tleft;
{
	register struct rpc_reply *rpc;

	errno = 0;
#ifdef RPC_DEBUG
	if (debug)
		printf("recvrpc: called len=%d\n", len);
#endif

	len = readudp(d, pkt, len, tleft);
	if (len == -1 || len < sizeof(struct rpc_reply))
		goto bad;

	rpc = (struct rpc_reply *)pkt;
	NTOHL(rpc->rp_xid);
	NTOHL(rpc->rp_direction);
	NTOHL(rpc->rp_stat);

	if (rpc->rp_xid != d->xid ||
	    rpc->rp_direction != RPC_REPLY ||
	    rpc->rp_stat != RPC_MSGACCEPTED) {
#ifdef RPC_DEBUG
		if (debug) {
			if (rpc->rp_xid != d->xid)
				printf("recvrpc: rp_xid %d != xid %d\n",
				    rpc->rp_xid, d->xid);
			if (rpc->rp_direction != RPC_REPLY)
				printf("recvrpc: rp_direction %d != RPC_REPLY\n",
				    rpc->rp_direction);
			if (rpc->rp_stat != RPC_MSGACCEPTED)
				printf("recvrpc: rp_stat %d != RPC_MSGACCEPTED\n",
				    rpc->rp_stat);
		}
#endif
		goto bad;
	}

	/* Return data count (thus indicating success) */
	return (len - sizeof(*rpc));

bad:
	return (-1);
}

/* Request a port number from the port mapper */
u_short
getport(d, prog, vers)
	register struct iodesc *d;
	u_long prog;
	u_long vers;
{
	register int i;
	register struct pmap_list *pl;
	u_long port;
	struct {
		u_long	prog;		/* call program */
		u_long	vers;		/* call version */
		u_long	proto;		/* call protocol */
		u_long	port;		/* call port (unused) */
	} sdata;

#ifdef RPC_DEBUG
	if (debug)
	    printf("getport: called\n");
#endif
	/* Try for cached answer first */
	for (i = 0, pl = pmap_list; i < pmap_num; ++i, ++pl)
		if ((pl->addr == d->destip || pl->addr == 0) &&
		    pl->prog == prog && pl->vers == vers)
			return (pl->port);

	/* Don't overflow cache */
	if (pmap_num > PMAP_NUM - 1)
		panic("getport: overflowed pmap_list!");

	sdata.prog = htonl(prog);
	sdata.vers = htonl(vers);
	sdata.proto = htonl(IPPROTO_UDP);
	sdata.port = 0;

	if (callrpc(d, PMAPPROG, PMAPVERS, PMAPPROC_GETPORT,
		&sdata, sizeof(sdata), &port, sizeof(port)) < 0) {
		printf("getport: %s", strerror(errno));
		return(-1);
	}

	/* Cache answer */
	pl->addr = d->destip;
	pl->prog = prog;
	pl->vers = vers;
	pl->port = port;
	++pmap_num;

	return ((u_short)port);
}
