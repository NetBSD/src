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
 * from @(#) Header: rpc.c,v 1.12 93/09/28 08:31:56 leres Exp  (LBL)
 *   $Id: rpc.c,v 1.4 1994/06/14 00:31:51 glass Exp $
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsv2.h>
#undef NFSX_FATTR
#include <errno.h>

#include "salibc.h"

#include "netboot.h"
#include "netif.h"
#include "bootbootp.h"

/* XXX defines we can't easily get from system includes */
#define	PMAPPORT		111
#define	PMAPPROG		100000
#define	PMAPVERS		2
#define	PMAPPROC_GETPORT	3

#define	RPC_MSG_VERSION		2
#define MSG_ACCEPTED		0
#define CALL			0
#define REPLY			1

/* Null rpc auth info */
struct auth_info {
	int	rp_atype;		/* zero (really AUTH_NULL) */
	u_long	rp_alen;		/* zero (size of auth struct) */
};

/* Generic rpc call header */
struct rpc_call {
	u_long	rp_xid;			/* request transaction id */
	int	rp_direction;		/* call direction */
	u_long	rp_rpcvers;		/* rpc version (2) */
	u_long	rp_prog;		/* program */
	u_long	rp_vers;		/* version */
	u_long	rp_proc;		/* procedure */
	struct	auth_info rp_auth;	/* AUTH_NULL */
	struct	auth_info rp_verf;	/* AUTH_NULL */
};

/* Generic rpc reply header */
struct rpc_reply {
	u_long	rp_xid;			/* request transaction id */
	int	rp_direction;		/* call direction */
	int	rp_stat;		/* accept status */
	u_long	rp_prog;		/* program (unused) */
	u_long	rp_vers;		/* version (unused) */
	u_long	rp_proc;		/* procedure (unused) */
};

struct nfs_call_data {
	u_char	fh[NFS_FHSIZE];
	u_long	off;
	u_long	len;
	u_long	xxx;			/* XXX what's this for? */
};

/* Data part of nfs rpc reply (also the largest thing we receive) */
struct nfs_reply_data {
	u_long	errno;
#ifndef NFSX_FATTR
	struct	nfsv2_fattr fa;
#else
	u_char	fa[NFSX_FATTR(0)];
#endif
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
static	int callrpc __P((struct iodesc *, u_long, u_long, u_long,
	    void *, int, void *, int));
static	int recvrpc __P((struct iodesc *, void *, int));
static	u_short getport __P((struct iodesc *, u_long, u_long));

/* Make a rpc call; return length of answer */
static int
callrpc(d, prog, vers, proc, sdata, slen, rdata, rlen)
	register struct iodesc *d;
	register u_long prog, vers, proc;
	register void *sdata;
	register int slen;
	register void *rdata;
	register int rlen;
{
	register int cc;
	register struct rpc_call *rpc;
	struct {
		u_char	header[HEADER_SIZE];
		struct	rpc_call wrpc;
		u_char	data[sizeof(struct nfs_reply_data)]; /* XXX */
	} wbuf;
	struct {
		u_char header[HEADER_SIZE];
		struct	rpc_reply rrpc;
		union {
			u_long	errno;
			u_char	data[sizeof(struct nfs_reply_data)];
		} ru;
	} rbuf;

#ifdef DEBUG
	if (debug)
	    printf("callrpc: called\n");
#endif
	if (rlen > sizeof(rbuf.ru.data))
		panic("callrpc: huge read (%d > %d)",
		    rlen, sizeof(rbuf.ru.data));

	d->destport = getport(d, prog, vers);

	rpc = &wbuf.wrpc;

	bzero(rpc, sizeof(*rpc));

	rpc->rp_xid = d->xid;
	rpc->rp_rpcvers = htonl(RPC_MSG_VERSION);
	rpc->rp_prog = htonl(prog);
	rpc->rp_vers = htonl(vers);
	rpc->rp_proc = htonl(proc);
	bcopy(sdata, wbuf.data, slen);

	cc = sendrecv(d, sendudp, rpc, sizeof(*rpc) + slen, recvrpc,
	    ((u_char *)&rbuf.rrpc) - HEADER_SIZE, sizeof(rbuf) - HEADER_SIZE);

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
static int
recvrpc(d, pkt, len)
	register struct iodesc *d;
	register void *pkt;
	int len;
{
	register struct rpc_reply *rpc;

#ifdef DEBUG
	if (debug)
	    printf("recvrpc: called\n");
#endif
	rpc = (struct rpc_reply *)checkudp(d, pkt, &len);
	if (rpc == NULL || len < sizeof(*rpc)) {
		errno = 0;
		return (-1);
	}

	NTOHL(rpc->rp_direction);
	NTOHL(rpc->rp_stat);

	if (rpc->rp_xid != d->xid || rpc->rp_direction != REPLY ||
	    rpc->rp_stat != MSG_ACCEPTED) {
		errno = 0;
		return (-1);
	}

	/* Bump xid so next request will be unique */
	++d->xid;

	/* Return data count (thus indicating success) */
	return (len - sizeof(*rpc));
}

/* Request a port number from the port mapper */
static u_short
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

#ifdef DEBUG
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
	    &sdata, sizeof(sdata), &port, sizeof(port)) < 0)
		panic("getport: %m");

	/* Cache answer */
	pl->addr = d->destip;
	pl->prog = prog;
	pl->vers = vers;
	pl->port = port;
	++pmap_num;

	return ((u_short)port);
}

/* Fetch file handle */
void
getnfsfh(d, path, fhp)
	register struct iodesc *d;
	char *path;
	u_char *fhp;
{
	register int len;
	struct {
		u_long	len;
		char	path[FNAME_SIZE];
	} sdata;
	struct {
		u_long	errno;
		u_char	fh[NFS_FHSIZE];
	} rdata;

#ifdef DEBUG
	if (debug)
	    printf("getnfsfh: called\n");
#endif
	bzero(&sdata, sizeof(sdata));
	len = strlen(path);
	if (len > sizeof(sdata.path))
		len = sizeof(sdata.path);
	bcopy(path, sdata.path, len);
	sdata.len = htonl(len);
	len = sizeof(sdata) - sizeof(sdata.path) + roundup(len, sizeof(long));

	if (callrpc(d, RPCPROG_MNT, RPCMNT_VER1, RPCMNT_MOUNT,
	    &sdata, len, &rdata, sizeof(rdata)) < 0)
		panic("getnfsfh: %s: %m", path);

	bcopy(rdata.fh, fhp, sizeof(rdata.fh));
}

/* Fetch file timestamp and size */
void
getnfsinfo(d, tp, sp, fp)
	register struct iodesc *d;
	register time_t *tp;
	register u_long *sp, *fp;
{
	register int rlen;
	register u_long t;
	struct {
		u_long	errno;
		struct	nfsv2_fattr fa;
	} rdata;

#ifdef DEBUG
 	if (debug)
 	    printf("getnfsinfo: called\n");
#endif
	rlen = sizeof(rdata);
#if 0
#ifdef NFSX_FATTR
#if NFSX_FATTR(1) > NFSX_FATTR(0)
	/* nqnfs makes this more painful than it needs to be */
	rlen -= NFSX_FATTR(1) - NFSX_FATTR(0);
#endif
#endif
#endif
	if (callrpc(d, NFS_PROG, NFS_VER2, NFSPROC_GETATTR,
	    d->fh, NFS_FHSIZE, &rdata, rlen) < 0)
		panic("getnfsinfo: %m");

	if (tp) {
		*tp = ntohl(rdata.fa.fa_nfsmtime.nfs_sec);
		t = ntohl(rdata.fa.fa_nfsatime.nfs_sec);
		if (*tp < t)
			*tp = t;
	}
	if (sp)
		*sp = ntohl(rdata.fa.fa_nfssize);
	if (fp)
		*fp = ntohl(rdata.fa.fa_type);
}

/* Lookup a file. Optionally return timestamp and size */
int
lookupfh(d, name, fhp, tp, sp, fp)
	register struct iodesc *d;
	register char *name;
	register u_char *fhp;
	register time_t *tp;
	register u_long *sp, *fp;
{
	register int len, rlen;
	struct {
		u_char	fh[NFS_FHSIZE];
		u_long	len;
		char	name[FNAME_SIZE];
	} sdata;
	struct {
		u_long	errno;
		u_char	fh[NFS_FHSIZE];
		struct	nfsv2_fattr fa;
	} rdata;

#ifdef DEBUG
	if (debug)
	    printf("lookupfh: called\n");
#endif

	bzero(&sdata, sizeof(sdata));
	bcopy(d->fh, sdata.fh, sizeof(sdata.fh));
	len = strlen(name);
	if (len > sizeof(sdata.name))
		len = sizeof(sdata.name);
	bcopy(name, sdata.name, len);
	sdata.len = htonl(len);
	len = sizeof(sdata) - sizeof(sdata.name) + roundup(len, sizeof(long));

	rlen = sizeof(rdata);
#if 0
#ifdef NFSX_FATTR
#if NFSX_FATTR(1) > NFSX_FATTR(0)
	/* nqnfs makes this more painful than it needs to be */
	rlen -= NFSX_FATTR(1) - NFSX_FATTR(0);
#endif
#endif
#endif
	if (callrpc(d, NFS_PROG, NFS_VER2, NFSPROC_LOOKUP,
	    &sdata, len, &rdata, rlen) < 0)
		return (-1);

	bcopy(rdata.fh, fhp, sizeof(rdata.fh));
	if (tp)
		*tp = ntohl(rdata.fa.fa_nfsctime.nfs_sec);
	if (sp)
		*sp = ntohl(rdata.fa.fa_nfssize);
	if (fp)
		*fp = ntohl(rdata.fa.fa_type);
	return (0);
}

static	int sendreaddata __P((struct iodesc *, void *, int));
static	int recvreaddata __P((struct iodesc *, void *, int));

/* max number of nfs reads pending */
#define NFS_COUNT 10

static struct nfsstate {
	u_long	off;
	u_long	len;
	int	done;
	void	*addr;
	u_long	xid;
} nfsstate[NFS_COUNT];

static u_long nfscc;

static int
sendreaddata(d, pkt, len)
	register struct iodesc *d;
	register void *pkt;
	register int len;
{
	register int i;
	register u_long cc;
	register struct rpc_call *rpc;
	register struct nfs_call_data *nfs;
	register struct nfsstate *ns;

#ifdef DEBUG
	if (debug)
	    printf("sendreaddata: called\n");
#endif

	if (len != sizeof(*rpc) + sizeof(*nfs))
		panic("sendreaddata: bad buffer (%d != %d)",
		    len, sizeof(*rpc) + sizeof(*nfs));
	rpc = pkt;
	nfs = (struct nfs_call_data *)(rpc + 1);
	for (i = 0, ns = nfsstate; i < NFS_COUNT; ++i, ++ns) {
		if (ns->done)
			continue;

		rpc->rp_xid = ns->xid;
		nfs->off = htonl(ns->off);
		nfs->len = htonl(ns->len);
		cc = sendudp(d, rpc, len);

		if (cc != len)
			panic("sendreaddata: short write (%d != %d)", cc, len);
	}
	/* XXX we may have actually sent a lot more bytes... */

	return (len);
}

/* Returns char count if done else -1 (and errno == 0) */
static int
recvreaddata(d, pkt, len)
	register struct iodesc *d;
	register void *pkt;
	int len;
{
	register int i;
	register struct rpc_reply *rpc;
	register struct nfs_reply_data *nfs;
	register struct nfsstate *ns;

#ifdef DEBUG
	if (debug)
	    printf("recvreaddata: called\n");
#endif
	rpc = (struct rpc_reply *)checkudp(d, pkt, &len);
	if (rpc == NULL || len < sizeof(*rpc)) {
		errno = 0;
		return (-1);
	}
	len -= sizeof(*rpc);

	NTOHL(rpc->rp_direction);
	NTOHL(rpc->rp_stat);

	if (rpc->rp_direction != REPLY || rpc->rp_stat != MSG_ACCEPTED) {
		errno = 0;
		return (-1);
	}

	for (i = 0, ns = nfsstate; i < NFS_COUNT; ++i, ++ns)
		if (rpc->rp_xid == ns->xid)
			break;
	if (i >= NFS_COUNT) {
		errno = 0;
		return (-1);
	}

	if (ns->done) {
		errno = 0;
		return (-1);
	}
	nfs = (struct nfs_reply_data *)(rpc + 1);
	if (len < sizeof(nfs->errno))
		panic("recvreaddata: bad read %d", len);
	if (nfs->errno) {
		errno = ntohl(nfs->errno);
		return (-1);
	}
	if (len < sizeof(*nfs) - sizeof(nfs->data))
		panic("recvreaddata: less than nfs sized %d", len);
	len -= sizeof(*nfs) - sizeof(nfs->data);

	if (len < nfs->count)
		panic("recvreaddata: short read (%d < %d)", len, nfs->count);
	len = nfs->count;
	if (len > ns->len)
		panic("recvreaddata: huge read (%d > %d)", len, ns->len);

	bcopy(nfs->data, ns->addr, len);
	ns->done = 1;
	nfscc += len;

	if (len < ns->len) {
		/* If first packet assume no more data to read */
		if (i == 0)
			return (0);

		/* Short read, assume we are at EOF */
		++i;
		++ns;
		while (i < NFS_COUNT) {
			ns->done = 1;
			++i;
			++ns;
		}
	}

	for (i = 0, ns = nfsstate; i < NFS_COUNT; ++i, ++ns)
		if (!ns->done) {
			errno = 0;
			return (-1);
		}

	/* Return data count (thus indicating success) */
	return (nfscc);
}

/* Read data from a file */
int
readdata(d, off, addr, len)
	register struct iodesc *d;
	register u_long off;
	register void *addr;
	register u_long len;
{
	register int i, cc;
	register struct rpc_call *rpc;
	register struct nfsstate *ns;
	struct {
		u_char	header[HEADER_SIZE];
		struct	rpc_call rpc;
		struct	nfs_call_data nfs;
	} sdata;
	struct {
		u_char	header[HEADER_SIZE];
		struct	rpc_call rpc;
		struct	nfs_reply_data nfs;
	} rdata;

#ifdef DEBUG
	if (debug)
	    printf("readdata: called\n");
#endif
	if (len == 0)
		return (0);
	d->destport = getport(d, NFS_PROG, NFS_VER2);


	bzero(&sdata, sizeof(sdata));

	for (i = 0, ns = nfsstate; i < NFS_COUNT; ++i, ++ns) {
		if (len <= 0) {
			ns->done = 1;
			continue;
		}
		ns->done = 0;

		ns->xid = d->xid;
		++d->xid;

		ns->off = off;
		ns->len = len;
		if (ns->len > NFSREAD_SIZE)
			ns->len = NFSREAD_SIZE;
#ifdef notdef
/* XXX to align or not align? It doesn't seem to speed things up... */
		if ((ns->off % NFSREAD_SIZE) != 0)
			ns->len -= off % NFSREAD_SIZE;
#endif

		off += ns->len;
		len -= ns->len;

		ns->addr = addr;
		addr += NFSREAD_SIZE;
	}

	rpc = &sdata.rpc;
	rpc->rp_rpcvers = htonl(RPC_MSG_VERSION);
	rpc->rp_prog = htonl(NFS_PROG);
	rpc->rp_vers = htonl(NFS_VER2);
	rpc->rp_proc = htonl(NFSPROC_READ);
	bcopy(d->fh, sdata.nfs.fh, sizeof(sdata.nfs.fh));

	nfscc = 0;
	cc = sendrecv(d, sendreaddata,
	    &sdata.rpc,
	    sizeof(struct rpc_call) + sizeof(struct nfs_call_data),
	    recvreaddata,
	    ((u_char *)&rdata.rpc) - HEADER_SIZE, HEADER_SIZE +
	    sizeof(struct rpc_call) + sizeof(struct nfs_reply_data));
	return (cc);
}

void readseg(d, off, size, addr)
	register struct iodesc * d;
	register u_long off, size, addr;
{
	register int i, cc;
	i = 0;
	while (size > 0) {
		cc = readdata(d, off, (void *)addr, size);
		if (cc <= 0) {
			/* XXX maybe should retry on certain errors */
			if (cc < 0)
				panic("\nreadseg: read: %s",
				    strerror(errno));
			panic("\nreadseg: hit EOF unexpectantly");
		}
		off += cc;
		addr += cc;
		size -= cc;
		i = ~i;
		printf("%c\010", i ? '-' : '=');
	}
}
