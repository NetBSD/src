/*	$NetBSD: nfs.c,v 1.8 1995/07/03 04:56:33 gwr Exp $	*/

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

/* Define our own NFS attributes without NQNFS stuff. */
struct nfsv2_fattrs {
	n_long	fa_type;
	n_long	fa_mode;
	n_long	fa_nlink;
	n_long	fa_uid;
	n_long	fa_gid;
	n_long	fa_size;
	n_long	fa_blocksize;
	n_long	fa_rdev;
	n_long	fa_blocks;
	n_long	fa_fsid;
	n_long	fa_fileid;
	struct nfsv2_time fa_atime;
	struct nfsv2_time fa_mtime;
	struct nfsv2_time fa_ctime;
};


struct nfs_read_args {
	u_char	fh[NFS_FHSIZE];
	n_long	off;
	n_long	len;
	n_long	xxx;			/* XXX what's this for? */
};

/* Data part of nfs rpc reply (also the largest thing we receive) */
#define NFSREAD_SIZE 1024
struct nfs_read_repl {
	n_long	errno;
	struct	nfsv2_fattrs fa;
	n_long	count;
	u_char	data[NFSREAD_SIZE];
};

struct nfs_iodesc {
	struct	iodesc	*iodesc;
	off_t	off;
	u_char	fh[NFS_FHSIZE];
	struct nfsv2_fattrs fa;	/* all in network order */
};

struct nfs_iodesc nfs_root_node;


/* Fetch the root file handle (call mount daemon) */
int
nfs_getrootfh(d, path, fhp)
	register struct iodesc *d;
	char *path;
	u_char *fhp;
{
	register int len;
	struct args {
		n_long	len;
		char	path[FNAME_SIZE];
	} *args;
	struct repl {
		n_long	errno;
		u_char	fh[NFS_FHSIZE];
	} *repl;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		struct args d;
	} sdata;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		struct repl d;
	} rdata;
	size_t cc;
	
#ifdef NFS_DEBUG
	if (debug)
		printf("nfs_getrootfh: %s\n", path);
#endif

	args = &sdata.d;
	repl = &rdata.d;

	bzero(args, sizeof(*args));
	len = strlen(path);
	if (len > sizeof(args->path))
		len = sizeof(args->path);
	args->len = htonl(len);
	bcopy(path, args->path, len);
	len = 4 + roundup(len, 4);

	cc = rpc_call(d, RPCPROG_MNT, RPCMNT_VER1, RPCMNT_MOUNT,
	    args, len, repl, sizeof(*repl));
	if (cc < 4)
		return (-1);
	if (repl->errno) {
		errno = ntohl(repl->errno);
		return (-1);
	}
	bcopy(repl->fh, fhp, sizeof(repl->fh));
	return (0);
}

/* Lookup a file.  Return handle and attributes. */
int
nfs_lookupfh(d, name, newfd)
	struct nfs_iodesc *d;
	char *name;
	struct nfs_iodesc *newfd;
{
	register int len, rlen;
	struct args {
		u_char	fh[NFS_FHSIZE];
		n_long	len;
		char	name[FNAME_SIZE];
	} *args;
	struct repl {
		n_long	errno;
		u_char	fh[NFS_FHSIZE];
		struct	nfsv2_fattrs fa;
	} *repl;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		struct args d;
	} sdata;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		struct repl d;
	} rdata;
	size_t cc;
	
#ifdef NFS_DEBUG
	if (debug)
		printf("lookupfh: called\n");
#endif

	args = &sdata.d;
	repl = &rdata.d;

	bzero(args, sizeof(*args));
	bcopy(d->fh, args->fh, sizeof(args->fh));
	len = strlen(name);
	if (len > sizeof(args->name))
		len = sizeof(args->name);
	bcopy(name, args->name, len);
	args->len = htonl(len);
	len = 4 + roundup(len, 4);
	len += NFS_FHSIZE;

	rlen = sizeof(*repl);

	cc = rpc_call(d->iodesc, NFS_PROG, NFS_VER2, NFSPROC_LOOKUP,
	    args, len, repl, rlen);
	if (cc < 4)
		return (EIO);
	if (repl->errno) {
		/* XXX - saerrno.h should match errno.h and RPC! */
		printf("nfs_lookup: \"%s\" error=%d\n",
			   name, ntohl(repl->errno));
		return (ENOENT);
	}
	bcopy( repl->fh, &newfd->fh, sizeof(newfd->fh));
	bcopy(&repl->fa, &newfd->fa, sizeof(newfd->fa));
	return (0);
}

/* Read data from a file */
size_t
nfs_readdata(d, off, addr, len)
	struct nfs_iodesc *d;
	off_t off;
	void *addr;
	size_t len;
{
	struct nfs_read_args *args;
	struct nfs_read_repl *repl;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		struct nfs_read_args d;
	} sdata;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		struct nfs_read_repl d;
	} rdata;
	size_t cc;
	int hlen, rlen, x;

	args = &sdata.d;
	repl = &rdata.d;

	bcopy(d->fh, args->fh, NFS_FHSIZE);
	args->off = txdr_unsigned(off);
	if (len > NFSREAD_SIZE)
		len = NFSREAD_SIZE;
	args->len = txdr_unsigned(len);
	args->xxx = txdr_unsigned(0);
	hlen = sizeof(*repl) - NFSREAD_SIZE;

	cc = rpc_call(d->iodesc, NFS_PROG, NFS_VER2, NFSPROC_READ,
	    args, sizeof(*args),
	    repl, sizeof(*repl));
	if (cc < hlen)
		return (-1);
	if (repl->errno) {
		errno = ntohl(repl->errno);
		return (-1);
	}
	rlen = cc - hlen;
	x = ntohl(repl->count);
	if (rlen < x) {
		printf("nfsread: short packet, %d < %d\n", rlen, x);
		return(-1);
	}
	bcopy(repl->data, addr, x);
	return (x);
}

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
	struct nfsv2_fattrs *fa;

	if (!(desc = socktodesc(sock))) {
		errno = EINVAL;
		return(-1);
	}

	/* Bind to a reserved port. */
	desc->myport = htons(--rpc_port);
	desc->destip = ip;
	if (nfs_getrootfh(desc, path, nfs_root_node.fh))
		return (-1);
	nfs_root_node.iodesc = desc;
	/* Fake up attributes for the root dir. */
	fa = &nfs_root_node.fa;
	fa->fa_type  = htonl(NFDIR);
	fa->fa_mode  = htonl(0755);
	fa->fa_nlink = htonl(2);

#ifdef NFS_DEBUG
	if (debug)
		printf("nfs_mount: got fh for %s\n", path);
#endif

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
	struct nfs_iodesc *newfd;
	int rc = 0;

#ifdef NFS_DEBUG
 	if (debug)
 	    printf("nfs_open: %s\n", path);
#endif
	if (nfs_root_node.iodesc == NULL) {
		errno = EIO;
		printf("nfs_open: must mount first.\n");
		return(-1);
	}

	/* allocate file system specific data structure */
	newfd = alloc(sizeof(*newfd));
	newfd->iodesc = nfs_root_node.iodesc;
	newfd->off = 0;

	/* lookup a file handle */
	rc = nfs_lookupfh(&nfs_root_node, path, newfd);
	if (rc) {
#ifdef NFS_DEBUG
		if (debug)
			printf("nfs_open: %s lookupfh failed: %s\n",
				path, strerror(errno));
#endif
		free(newfd, sizeof(*newfd));
		return(rc);
	}
	f->f_fsdata = (void *)newfd;

#ifdef NFS_DEBUG
	if (debug)
		printf("nfs_open: \"%s\", type=%d size=%d\n", path,
			   ntohl(newfd->fa.fa_type),
			   ntohl(newfd->fa.fa_size));
#endif

	return(rc);
}

int
nfs_close(f)
	struct open_file *f;
{
	register struct nfs_iodesc *fp = (struct nfs_iodesc *)f->f_fsdata;

#ifdef NFS_DEBUG
	if (debug)
		printf("nfs_close: fp=0x%x\n", fp);
#endif

	if (fp)
		free(fp, sizeof(struct nfs_iodesc));
	f->f_fsdata = (void *)0;
	
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
		twiddle();
		cc = nfs_readdata(fp, fp->off, (void *)addr, size);
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
	register struct nfs_iodesc *d = (struct nfs_iodesc *)f->f_fsdata;
	n_long size = ntohl(d->fa.fa_size);

	switch (where) {
	case SEEK_SET:
		d->off = offset;
		break;
	case SEEK_CUR:
		d->off += offset;
		break;
	case SEEK_END:
		d->off = size - offset;
		break;
	default:
		return (-1);
	}

	return (d->off);
}

/* NFNON=0, NFREG=1, NFDIR=2, NFBLK=3, NFCHR=4, NFLNK=5 */
int nfs_stat_types[8] = {
	0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK, 0 };

int
nfs_stat(f, sb)
	struct open_file *f;
	struct stat *sb;
{
	struct nfs_iodesc *fp = (struct nfs_iodesc *)f->f_fsdata;
	register n_long ftype, mode;

	ftype = ntohl(fp->fa.fa_type);
	mode  = ntohl(fp->fa.fa_mode);
	mode |= nfs_stat_types[ftype & 7];

	sb->st_mode  = mode;
	sb->st_nlink = ntohl(fp->fa.fa_nlink);
	sb->st_uid   = ntohl(fp->fa.fa_uid);
	sb->st_gid   = ntohl(fp->fa.fa_gid);
	sb->st_size  = ntohl(fp->fa.fa_size);

	return (0);
}
