/*-
 *  Copyright (c) 1993 John Brezak
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: nfs.c,v 1.3 1994/08/22 21:56:08 brezak Exp $
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsv2.h>
#undef NFSX_FATTR

#include "stand.h"
#include "net.h"
#include "netif.h"
#include "nfs.h"
#include "rpc.h"

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

struct nfs_iodesc {
	off_t	off;
	size_t	size;
	u_char	*fh;
	struct	iodesc	*iodesc;
};

/* Fetch (mount) file handle */
static int
getmountfh(d, path, fhp)
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
	int cc;
	
#ifdef NFS_DEBUG
	if (debug)
	    printf("getmountfh: called\n");
#endif
	bzero(&sdata, sizeof(sdata));
	len = strlen(path);
	if (len > sizeof(sdata.path))
		len = sizeof(sdata.path);
	bcopy(path, sdata.path, len);
	sdata.len = htonl(len);
	len = sizeof(sdata) - sizeof(sdata.path) + roundup(len, sizeof(long));

	if ((cc = callrpc(d, RPCPROG_MNT, RPCMNT_VER1, RPCMNT_MOUNT,
	    &sdata, len, &rdata, sizeof(rdata))) < 0)
		    return(-1);
	if (cc < sizeof(rdata.errno))
		panic("getmountfh: callrpc small read");
	if (rdata.errno) {
		errno = ntohl(rdata.errno);
		return(-1);
	}
	bcopy(rdata.fh, fhp, sizeof(rdata.fh));
	return(0);
}

/* Fetch file timestamp and size */
static int
getnfsinfo(d, tp, sp, fp, mp, up, gp)
	register struct nfs_iodesc *d;
	register time_t *tp;
	u_long *sp, *fp;
	mode_t *mp;
	uid_t *up;
	gid_t *gp;
{
	register int rlen;
	register u_long t;
	struct {
		u_long	errno;
		struct	nfsv2_fattr fa;
	} rdata;
	int cc;

#ifdef NFS_DEBUG
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
	if ((cc = callrpc(d->iodesc, NFS_PROG, NFS_VER2, NFSPROC_GETATTR,
		          d->fh, NFS_FHSIZE, &rdata, rlen)) < 0)
		return(-1);
	if (cc < sizeof(rdata.errno))
		panic("getnfsinfo: callrpc small read");
	if (rdata.errno) {
		errno = ntohl(rdata.errno);
		return(-1);
	}
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
	if (mp)
		*mp = ntohl(rdata.fa.fa_mode);
	if (up)
		*up = ntohl(rdata.fa.fa_uid);
	if (gp)
		*gp = ntohl(rdata.fa.fa_gid);
	return(0);
}

/* Lookup a file. Optionally return timestamp and size */
static int
lookupfh(d, name, fhp, tp, sp, fp)
	struct nfs_iodesc *d;
	char *name;
	u_char *fhp;
	time_t *tp;
	u_long *sp, *fp;
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
	int cc;
	
#ifdef NFS_DEBUG
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
	if ((cc = callrpc(d->iodesc, NFS_PROG, NFS_VER2, NFSPROC_LOOKUP,
	    &sdata, len, &rdata, rlen)) < 0)
		return (-1);
	if (cc < sizeof(rdata.errno))
		panic("lookupfh: callrpc small read");
	if (rdata.errno) {
		errno = ntohl(rdata.errno);
		return(-1);
	}
	bcopy(rdata.fh, fhp, sizeof(rdata.fh));
	if (tp)
		*tp = ntohl(rdata.fa.fa_nfsctime.nfs_sec);
	if (sp)
		*sp = ntohl(rdata.fa.fa_nfssize);
	if (fp)
		*fp = ntohl(rdata.fa.fa_type);
	return (0);
}

static int
sendreaddata(d, pkt, len)
	register struct nfs_iodesc *d;
	register void *pkt;
	register int len;
{
	register int i;
	register u_long cc;
	register struct rpc_call *rpc;
	register struct nfs_call_data *nfs;
	register struct nfsstate *ns;

#ifdef NFS_DEBUG
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
		cc = sendudp(d->iodesc, rpc, len);

		if (cc != len)
			panic("sendreaddata: short write (%d != %d)", cc, len);
	}
	/* XXX we may have actually sent a lot more bytes... */

	return (len);
}

/* Returns char count if done else -1 (and errno == 0) */
static int
recvreaddata(d, pkt, len)
	register struct nfs_iodesc *d;
	register void *pkt;
	int len;
{
	register int i;
	register struct rpc_reply *rpc;
	register struct nfs_reply_data *nfs;
	register struct nfsstate *ns;

#ifdef NFS_DEBUG
	if (debug)
	    printf("recvreaddata: called\n");
#endif
	rpc = (struct rpc_reply *)checkudp(d->iodesc, pkt, &len);
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
#ifdef NFS_DEBUG
	if (debug)
	    printf("recvreaddata: ns=%x\n", (u_int)ns);
#endif
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

	
#ifdef NFS_DEBUG
	if (debug)
	    printf("recvreaddata: read %d bytes.\n", len);
#endif
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
static int
readdata(d, off, addr, len)
	register struct nfs_iodesc *d;
	register off_t off;
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

#ifdef NFS_DEBUG
	if (debug)
	    printf("readdata: addr=%x, off=%d len=%d\n", (u_int)addr, (u_int)off, len);
#endif
	if (len == 0)
		return (0);
	d->iodesc->destport = getport(d->iodesc, NFS_PROG, NFS_VER2);

	bzero(&sdata, sizeof(sdata));

	for (i = 0, ns = nfsstate; i < NFS_COUNT; ++i, ++ns) {
		if (len <= 0) {
			ns->done = 1;
			continue;
		}
		ns->done = 0;

		ns->xid = d->iodesc->xid;
		++d->iodesc->xid;

		ns->off = (u_int)off;
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
	cc = sendrecv(d->iodesc,
		      sendreaddata, &sdata.rpc,
		      sizeof(struct rpc_call) + sizeof(struct nfs_call_data),
		      recvreaddata,
			((u_char *)&rdata.rpc) - HEADER_SIZE, HEADER_SIZE +
			  sizeof(struct rpc_call) + sizeof(struct nfs_reply_data));
	return (cc);
}

static struct iodesc *mountfs;
static u_char mountfh[NFS_FHSIZE];
static time_t mounttime;

/*
 * nfs_mount - mount this nfs filesystem to a host
 */
int
nfs_mount(sock, ip, path)
	int sock;
	n_long ip;
	char *path;
{
	struct iodesc *desc;
	struct nfs_iodesc *fp;
	u_long ftype;
	
	if (!(desc = socktodesc(sock))) {
		errno = EINVAL;
		return(-1);
	}
	bcopy(&desc->myea[4], &desc->myport, 2);
	desc->destip = ip;
	getmountfh(desc, path, mountfh);

	fp = alloc(sizeof(struct nfs_iodesc));
	fp->iodesc = desc;
	fp->fh = mountfh;
	fp->off = 0;
	if (getnfsinfo(fp, &mounttime, NULL, &ftype, NULL, NULL, NULL) < 0) {
		free(fp, sizeof(struct nfs_iodesc));
		return(-1);
	}

	if (ftype != NFDIR) {
		free(fp, sizeof(struct nfs_iodesc));
	    	errno = EINVAL;
		printf("nfs_mount: bad mount ftype %d", ftype);
		return(-1);
	}
#ifdef NFS_DEBUG
	if (debug)
		printf("nfs_mount: got fh for %s, mtime=%d, ftype=%d\n",
			path, mounttime, ftype);
#endif
	mountfs = desc;
	free(fp, sizeof(struct nfs_iodesc));

	return(0);
}

/*
 * Open a file.
 */
int
nfs_open(path, f)
	char *path;
	struct open_file *f;
{
	register struct nfs_iodesc *fp;
	u_char *imagefh;
	u_long size, ftype;
	int rc = 0;

#ifdef NFS_DEBUG
 	if (debug)
 	    printf("nfs_open: %s\n", path);
#endif
	if (!mountfs) {
		errno = EIO;
		printf("nfs_open: must mount first.\n");
		return(-1);
	}

	/* allocate file system specific data structure */
	fp = alloc(sizeof(struct nfs_iodesc));
	fp->iodesc = mountfs;
	fp->fh = mountfh;
	fp->off = 0;
	
	f->f_fsdata = (void *)fp;
	imagefh = alloc(NFS_FHSIZE);
	bzero(imagefh, NFS_FHSIZE);

	/* lookup a file handle */
	rc = lookupfh(fp, path, imagefh, NULL, &size, &ftype);
	if (rc < 0) {
#ifdef NFS_DEBUG
		if (debug)
			printf("nfs_open: %s lookupfh failed: %s\n", path, strerror(errno));
#endif
		f->f_fsdata = (void *)0;
		free(fp, sizeof(struct nfs_iodesc));
		free(imagefh, NFS_FHSIZE);
		return(rc);
	}
	fp->fh = imagefh;
	
#ifdef NFS_DEBUG
	if (debug)
		printf("nfs_open: %s success, size=%d ftype=%d\n",
			path, size, ftype);
#endif
	fp->size = size;

	return(rc);
}

int
nfs_close(f)
	struct open_file *f;
{
	register struct nfs_iodesc *fp = (struct nfs_iodesc *)f->f_fsdata;

#ifdef NFS_DEBUG
	if (debug)
		printf("nfs_close: called\n");
#endif
	f->f_fsdata = (void *)0;
	if (fp == (struct nfs_iodesc *)0)
		return (0);

	free(fp->fh, NFS_FHSIZE);
	free(fp, sizeof(struct nfs_iodesc));
	
	return (0);
}

/*
 * read a portion of a file
 */
int
nfs_read(f, addr, size, resid)
	struct open_file *f;
	char *addr;
	u_int size;
	u_int *resid;	/* out */
{
	register struct nfs_iodesc *fp = (struct nfs_iodesc *)f->f_fsdata;
	register int cc;
	
#ifdef NFS_DEBUG
	if (debug)
		printf("nfs_read: size=%d off=%d\n", size, (int)fp->off);
#endif
	while (size > 0) {
		cc = readdata(fp->iodesc, fp->off, (void *)addr, size);
		if (cc <= 0) {
			/* XXX maybe should retry on certain errors */
			if (cc < 0) {
#ifdef NFS_DEBUG
				if (debug)
					printf("nfs_read: read: %s",
						strerror(errno));
#endif
				return (-1);
			}
			if (debug)
				printf("nfs_read: hit EOF unexpectantly");
			goto ret;
		}
		fp->off += cc;
		addr += cc;
		size -= cc;
	}
ret:
	if (resid)
		*resid = size;

	return (0);
}

/*
 * Not implemented.
 */
int
nfs_write(f, start, size, resid)
	struct open_file *f;
	char *start;
	u_int size;
	u_int *resid;	/* out */
{
	errno = EROFS;
	
	return (-1);
}

off_t
nfs_seek(f, offset, where)
	struct open_file *f;
	off_t offset;
	int where;
{
	register struct nfs_iodesc *fp = (struct nfs_iodesc *)f->f_fsdata;

	switch (where) {
	case SEEK_SET:
		fp->off = offset;
		break;
	case SEEK_CUR:
		fp->off += offset;
		break;
	case SEEK_END:
		fp->off = fp->size - offset;
		break;
	default:
		return (-1);
	}
	return (fp->off);
}

int
nfs_stat(f, sb)
	struct open_file *f;
	struct stat *sb;
{
	register struct nfs_iodesc *fp = (struct nfs_iodesc *)f->f_fsdata;
	mode_t mode = 0;
	u_long ftype = 0;

#ifdef NFS_DEBUG
 	if (debug)
 	    printf("nfs_stat: called\n");
#endif
	if (getnfsinfo(fp, &mounttime, &sb->st_size, &ftype, &mode, &sb->st_uid, &sb->st_gid) < 0)
		return(-1);

	/* create a mode */
	switch (ftype) {
	case NFNON:
		sb->st_mode = 0;
		break;
	case NFREG:
		sb->st_mode = S_IFREG;
		break;
	case NFDIR:
		sb->st_mode = S_IFDIR;
		break;
	case NFBLK:
		sb->st_mode = S_IFBLK;
		break;
	case NFCHR:
		sb->st_mode = S_IFCHR;
		break;
	case NFLNK:
		sb->st_mode = S_IFLNK;
		break;
	}
	sb->st_mode |= mode;

	return (0);
}
