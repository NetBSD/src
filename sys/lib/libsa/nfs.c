/*	$NetBSD: nfs.c,v 1.6 1995/02/20 11:04:12 mycroft Exp $	*/

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
#include <nfs/xdr_subs.h>

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
	struct	nfsv2_fattr fa;
	u_long	count;
	u_char	data[1200];
};
#define NFSREAD_SIZE sizeof(((struct nfs_reply_data *)0)->data)

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
	} wbuf;
	struct {
		u_long	errno;
		u_char	fh[NFS_FHSIZE];
	} rbuf;
	size_t cc;
	
#ifdef NFS_DEBUG
	if (debug)
		printf("getmountfh: called\n");
#endif

	bzero(&wbuf, sizeof(wbuf));
	len = strlen(path);
	if (len > sizeof(wbuf.path))
		len = sizeof(wbuf.path);
	bcopy(path, wbuf.path, len);
	wbuf.len = htonl(len);
	len = sizeof(wbuf) - sizeof(wbuf.path) + roundup(len, sizeof(long));

	if ((cc = callrpc(d, RPCPROG_MNT, RPCMNT_VER1, RPCMNT_MOUNT,
	    &wbuf, len, &rbuf, sizeof(rbuf))) == -1)
		    return (-1);
	if (cc < sizeof(rbuf.errno))
		panic("getmountfh: callrpc small read");
	if (rbuf.errno) {
		errno = ntohl(rbuf.errno);
		return (-1);
	}
	bcopy(rbuf.fh, fhp, sizeof(rbuf.fh));
	return (0);
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
	} rbuf;
	size_t cc;

#ifdef NFS_DEBUG
 	if (debug)
 	    printf("getnfsinfo: called\n");
#endif
	rlen = sizeof(rbuf);
#if NFSX_FATTR(1) > NFSX_FATTR(0)
	/* nqnfs makes this more painful than it needs to be */
	rlen -= NFSX_FATTR(1) - NFSX_FATTR(0);
#endif
	if ((cc = callrpc(d->iodesc, NFS_PROG, NFS_VER2, NFSPROC_GETATTR,
	    d->fh, NFS_FHSIZE, &rbuf, rlen)) == -1)
		return (-1);
	if (cc < sizeof(rbuf.errno))
		panic("getnfsinfo: callrpc small read");
	if (rbuf.errno) {
		errno = ntohl(rbuf.errno);
		return (-1);
	}
	if (tp) {
		*tp = ntohl(rbuf.fa.fa_nfsmtime.nfs_sec);
		t = ntohl(rbuf.fa.fa_nfsatime.nfs_sec);
		if (*tp < t)
			*tp = t;
	}
	if (sp)
		*sp = ntohl(rbuf.fa.fa_nfssize);
	if (fp)
		*fp = ntohl(rbuf.fa.fa_type);
	if (mp)
		*mp = ntohl(rbuf.fa.fa_mode);
	if (up)
		*up = ntohl(rbuf.fa.fa_uid);
	if (gp)
		*gp = ntohl(rbuf.fa.fa_gid);
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
	} wbuf;
	struct {
		u_long	errno;
		u_char	fh[NFS_FHSIZE];
		struct	nfsv2_fattr fa;
	} rbuf;
	size_t cc;
	
#ifdef NFS_DEBUG
	if (debug)
		printf("lookupfh: called\n");
#endif

	bzero(&wbuf, sizeof(wbuf));
	bcopy(d->fh, wbuf.fh, sizeof(wbuf.fh));
	len = strlen(name);
	if (len > sizeof(wbuf.name))
		len = sizeof(wbuf.name);
	bcopy(name, wbuf.name, len);
	wbuf.len = htonl(len);
	len = sizeof(wbuf) - sizeof(wbuf.name) + roundup(len, sizeof(long));

	rlen = sizeof(rbuf);
#ifdef NFSX_FATTR
#if NFSX_FATTR(1) > NFSX_FATTR(0)
	/* nqnfs makes this more painful than it needs to be */
	rlen -= NFSX_FATTR(1) - NFSX_FATTR(0);
#endif
#endif

	if ((cc = callrpc(d->iodesc, NFS_PROG, NFS_VER2, NFSPROC_LOOKUP,
	    &wbuf, len, &rbuf, rlen)) == -1)
		return (-1);
	if (cc < sizeof(rbuf.errno))
		panic("lookupfh: callrpc small read");
	if (rbuf.errno) {
		errno = ntohl(rbuf.errno);
		return (-1);
	}
	bcopy(rbuf.fh, fhp, sizeof(rbuf.fh));
	if (tp)
		*tp = ntohl(rbuf.fa.fa_nfsctime.nfs_sec);
	if (sp)
		*sp = ntohl(rbuf.fa.fa_nfssize);
	if (fp)
		*fp = ntohl(rbuf.fa.fa_type);
	return (0);
}

/* Read data from a file */
static size_t
readdata(d, off, addr, len)
	register struct nfs_iodesc *d;
	register off_t off;
	register void *addr;
	register size_t len;
{
	size_t cc;
	struct nfs_call_data wbuf;
	struct nfs_reply_data rbuf;

	bcopy(d->fh, wbuf.fh, NFS_FHSIZE);
	wbuf.off = txdr_unsigned(off);
	if (len > NFSREAD_SIZE)
		len = NFSREAD_SIZE;
	wbuf.len = txdr_unsigned(len);
	wbuf.xxx = txdr_unsigned(0);

	cc = callrpc(d->iodesc, NFS_PROG, NFS_VER2, NFSPROC_READ,
	    &wbuf, sizeof(wbuf),
	    &rbuf, sizeof(rbuf) - NFSREAD_SIZE + len);
	if (cc == -1 || cc < sizeof(rbuf) - NFSREAD_SIZE)
		return (-1);

	cc -= sizeof(rbuf) - NFSREAD_SIZE;
	bcopy(rbuf.data, addr, cc);
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
	register size_t cc;
	
#ifdef NFS_DEBUG
	if (debug)
		printf("nfs_read: size=%d off=%d\n", size, (int)fp->off);
#endif
	while (size > 0) {
		cc = readdata(fp->iodesc, fp->off, (void *)addr, size);
		/* XXX maybe should retry on certain errors */
		if (cc == -1) {
#ifdef NFS_DEBUG
			if (debug)
				printf("nfs_read: read: %s", strerror(errno));
#endif
			return (-1);
		}
		if (cc == 0) {
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
