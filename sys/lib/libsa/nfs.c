/*	$NetBSD: nfs.c,v 1.50.24.1 2024/09/20 11:31:32 martin Exp $	*/

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

/*
 * XXX Does not currently implement:
 * XXX
 * XXX LIBSA_NO_FS_CLOSE
 * XXX LIBSA_NO_FS_SEEK
 * XXX LIBSA_NO_FS_WRITE
 * XXX LIBSA_NO_FS_SYMLINK (does this even make sense?)
 * XXX LIBSA_FS_SINGLECOMPONENT (does this even make sense?)
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#ifdef _STANDALONE
#include <lib/libkern/libkern.h>
#else
#include <string.h>
#endif

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "rpcv2.h"
#include "nfsv2.h"
#include "nfsv3.h"

#include "stand.h"
#include "net.h"
#include "nfs.h"
#include "rpc.h"

/* Storage for any filehandle (including length for V3) */
#define NFS_FHSTORE (NFS_FHSIZE < NFS_V3FHSIZE ? NFS_V3FHSIZE + 4: NFS_FHSIZE)

/* Data part of nfs rpc reply (also the largest thing we receive) */
#define NFSREAD_SIZE 1024

#ifndef NFS_NOSYMLINK
struct nfs_readlnk_repl {
	n_long	errno;
	n_long	len;
	char	path[NFS_MAXPATHLEN];
};
#endif

static inline uint64_t
getnquad(n_long x[2]) {
	return (uint64_t)ntohl(x[0]) << 32 | ntohl(x[1]);
}

static inline void
setnquad(n_long x[2], uint64_t v)
{
	x[0] = htonl((n_long)(v >> 32));
	x[1] = htonl((n_long)(v & 0xffffffff));
}

struct nfs_iodesc {
	struct	iodesc	*iodesc;
	off_t	off;
	int	version;
	u_char	fh[NFS_FHSTORE];
	union {
		/* all in network order */
		struct nfsv2_fattr v2;
		struct nfsv3_fattr v3;
	} u_fa;
};

static inline size_t
fhstore(int version, u_char *fh)
{
	size_t len;

	switch (version) {
	case NFS_VER2:
		len = NFS_FHSIZE;
		break;
	case NFS_VER3:
		len = fh[0] << 24 | fh[1] << 16 | fh[2] << 8 | fh[3];
		if (len > NFS_V3FHSIZE)
			len = NFS_V3FHSIZE;
		len = 4 + roundup(len, 4);
		break;
	default:
		len = 0;
		break;
	}

	return len;
}

static inline size_t
fhcopy(int version, u_char *src, u_char *dst)
{
	size_t len = fhstore(version, src);
	memcpy(dst, src, len);
	return len;
}

#define setfh(d, s) fhcopy((d)->version, (s), (d)->fh)
#define getfh(d, s) fhcopy((d)->version, (d)->fh, (s))


struct nfs_iodesc nfs_root_node;

int	nfs_getrootfh(struct iodesc *, char *, u_char *, int *);
int	nfs_lookupfh(struct nfs_iodesc *, const char *, int,
	    struct nfs_iodesc *);
int	nfs_readlink(struct nfs_iodesc *, char *);
ssize_t	nfs_readdata(struct nfs_iodesc *, off_t, void *, size_t);

/*
 * Fetch the root file handle (call mount daemon)
 * On error, return non-zero and set errno.
 */
int
nfs_getrootfh(struct iodesc *d, char *path, u_char *fhp, int *versionp)
{
	int len;
	struct args {
		n_long	len;
		char	path[FNAME_SIZE];
	} *args;
	struct repl {
		n_long	errno;
		u_char	fh[NFS_FHSTORE];
	} *repl;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		struct args d;
	} sdata;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		struct repl d;
	} rdata;
	ssize_t cc;

#ifdef NFS_DEBUG
	if (debug)
		printf("%s: %s\n", __func__, path);
#endif

	args = &sdata.d;
	repl = &rdata.d;

	(void)memset(args, 0, sizeof(*args));
	len = strlen(path);
	if ((size_t)len > sizeof(args->path))
		len = sizeof(args->path);
	args->len = htonl(len);
	(void)memcpy(args->path, path, len);
	len = 4 + roundup(len, 4);

	*versionp = NFS_VER3;
	cc = rpc_call(d, RPCPROG_MNT, RPCMNT_VER3, RPCMNT_MOUNT,
	    args, len, repl, sizeof(*repl));
	if (cc == -1 || cc < 4 || repl->errno) {
		*versionp = NFS_VER2;
		cc = rpc_call(d, RPCPROG_MNT, RPCMNT_VER1, RPCMNT_MOUNT,
		    args, len, repl, sizeof(*repl));
	}
	if (cc == -1) {
		/* errno was set by rpc_call */
		return -1;
	}
	if (cc < 4) {
		errno = EBADRPC;
		return -1;
	}
	if (repl->errno) {
		errno = ntohl(repl->errno);
		return -1;
	}
	fhcopy(*versionp, repl->fh, fhp);
	return 0;
}

/*
 * Lookup a file.  Store handle and attributes.
 * Return zero or error number.
 */
int
nfs_lookupfh(struct nfs_iodesc *d, const char *name, int len,
	struct nfs_iodesc *newfd)
{
	struct argsv2 {
		u_char	fh[NFS_FHSIZE];
		n_long	len;
		char	name[FNAME_SIZE];
	} *argsv2;
	struct argsv3 {
		u_char	fh[NFS_FHSTORE];
		n_long	len;
		char	name[FNAME_SIZE];
	} *argsv3;
	struct replv2 {
		n_long	errno;
		u_char	fh[NFS_FHSIZE];
		struct	nfsv2_fattr fa;
	} *replv2;
	struct replv3 {
		n_long	errno;
		u_char	fh[NFS_FHSTORE];
		n_long	fattrflag;
		struct	nfsv3_fattr fa;
		n_long	dattrflag;
		struct	nfsv3_fattr da;
	} *replv3;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		union {
			struct argsv2 v2;
			struct argsv3 v3;
		} u_d;
	} sdata;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		union {
			struct replv2 v2;
			struct replv3 v3;
		} u_d;
	} rdata;
	ssize_t cc;
	size_t alen;

#ifdef NFS_DEBUG
	if (debug)
		printf("%s: called\n", __func__);
#endif

	argsv2 = &sdata.u_d.v2;
	argsv3 = &sdata.u_d.v3;
	replv2 = &rdata.u_d.v2;
	replv3 = &rdata.u_d.v3;

	switch (d->version) {
	case NFS_VER2:
		(void)memset(argsv2, 0, sizeof(*argsv2));
		getfh(d, argsv2->fh);
		if ((size_t)len > sizeof(argsv2->name))
			len = sizeof(argsv2->name);
		(void)memcpy(argsv2->name, name, len);
		argsv2->len = htonl(len);

		/* padded name, name length */
		len = roundup(len, 4) + 4;
		/* filehandle size */
		alen = fhstore(d->version, argsv2->fh);

		cc = rpc_call(d->iodesc, NFS_PROG, NFS_VER2, NFSPROC_LOOKUP,
		    argsv2, alen+len, replv2, sizeof(*replv2));
		break;
	case NFS_VER3:
		(void)memset(argsv3, 0, sizeof(*argsv3));
		getfh(d, argsv3->fh);
		if ((size_t)len > sizeof(argsv3->name))
			len = sizeof(argsv3->name);
		(void)memcpy(argsv3->name, name, len);
		argsv3->len = htonl(len);

		/* padded name, name length */
		len = roundup(len, 4) + 4;
		/* filehandle size */
		alen = fhstore(d->version, argsv3->fh);

		/* adjust for variable sized file handle */
		memmove(argsv3->fh + alen, &argsv3->len, len);

		cc = rpc_call(d->iodesc, NFS_PROG, NFS_VER3, NFSV3PROC_LOOKUP,
		    argsv3, alen+len, replv3, sizeof(*replv3));
		break;
	default:
		return ENOSYS;
	}

	if (cc == -1)
		return errno;		/* XXX - from rpc_call */
	if (cc < 4)
		return EIO;

	switch (d->version) {
	case NFS_VER2:
		if (replv2->errno) {
			/* saerrno.h now matches NFS error numbers. */
			return ntohl(replv2->errno);
		}

		setfh(newfd, replv2->fh);
		(void)memcpy(&newfd->u_fa.v2, &replv2->fa,
		    sizeof(newfd->u_fa.v2));
		break;
	case NFS_VER3:
		if (replv3->errno) {
			/* saerrno.h now matches NFS error numbers. */
			return ntohl(replv3->errno);
		}

		setfh(newfd, replv3->fh);

		if (replv3->fattrflag) {
			(void)memcpy(&newfd->u_fa.v3, &replv3->fa,
			    sizeof(newfd->u_fa.v3));
		}
		break;
	}
	return 0;
}

#ifndef NFS_NOSYMLINK
/*
 * Get the destination of a symbolic link.
 */
int
nfs_readlink(struct nfs_iodesc *d, char *buf)
{
	struct {
		n_long	h[RPC_HEADER_WORDS];
		u_char	fh[NFS_FHSTORE];
	} sdata;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		struct nfs_readlnk_repl d;
	} rdata;
	ssize_t cc;

#ifdef NFS_DEBUG
	if (debug)
		printf("%s: called\n", __func__);
#endif

	getfh(d, sdata.fh);
	cc = rpc_call(d->iodesc, NFS_PROG, d->version, NFSPROC_READLINK,
		      sdata.fh, fhstore(d->version, sdata.fh),
		      &rdata.d, sizeof(rdata.d));
	if (cc == -1)
		return errno;

	if (cc < 4)
		return EIO;

	if (rdata.d.errno)
		return ntohl(rdata.d.errno);

	rdata.d.len = ntohl(rdata.d.len);
	if (rdata.d.len > NFS_MAXPATHLEN)
		return ENAMETOOLONG;

	(void)memcpy(buf, rdata.d.path, rdata.d.len);
	buf[rdata.d.len] = 0;
	return 0;
}
#endif

/*
 * Read data from a file.
 * Return transfer count or -1 (and set errno)
 */
ssize_t
nfs_readdata(struct nfs_iodesc *d, off_t off, void *addr, size_t len)
{
	struct argsv2 {
		u_char	fh[NFS_FHSIZE];
		n_long	off;
		n_long	len;
		n_long	xxx;			/* XXX what's this for? */
	} *argsv2;
	struct argsv3 {
		u_char	fh[NFS_FHSTORE];
		n_long	off[2];
		n_long	len;
	} *argsv3;
	struct replv2 {
		n_long	errno;
		struct	nfsv2_fattr fa;
		n_long	count;
		u_char	data[NFSREAD_SIZE];
	} *replv2;
	struct replv3 {
		n_long	errno;
		n_long	attrflag;
		struct	nfsv3_fattr fa;
		n_long	count;
		n_long	eof;
		n_long	length;
		u_char	data[NFSREAD_SIZE];
	} *replv3;
	struct replv3_noattr {
		n_long	errno;
		n_long	attrflag;
		n_long	count;
		n_long	eof;
		n_long	length;
		u_char	data[NFSREAD_SIZE];
	} *replv3no;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		union {
			struct argsv2 v2;
			struct argsv3 v3;
		} u_d;
	} sdata;
	struct {
		n_long	h[RPC_HEADER_WORDS];
		union {
			struct replv2 v2;
			struct replv3 v3;
		} u_d;
	} rdata;
	ssize_t cc;
	long x;
	size_t hlen, rlen, alen;
	u_char *data;

	argsv2 = &sdata.u_d.v2;
	argsv3 = &sdata.u_d.v3;
	replv2 = &rdata.u_d.v2;
	replv3 = &rdata.u_d.v3;

	if (len > NFSREAD_SIZE)
		len = NFSREAD_SIZE;

	switch (d->version) {
	case NFS_VER2:
		getfh(d, argsv2->fh);
		argsv2->off = htonl((n_long)off);
		argsv2->len = htonl((n_long)len);
		argsv2->xxx = htonl((n_long)0);
		hlen = sizeof(*replv2) - NFSREAD_SIZE;
		cc = rpc_call(d->iodesc, NFS_PROG, d->version, NFSPROC_READ,
		    argsv2, sizeof(*argsv2),
		    replv2, sizeof(*replv2));
		break;
	case NFS_VER3:
		getfh(d, argsv3->fh);
		setnquad(argsv3->off, (uint64_t)off);
		argsv3->len = htonl((n_long)len);
		hlen = sizeof(*replv3) - NFSREAD_SIZE;

		/* adjust for variable sized file handle */
		alen = sizeof(*argsv3) - offsetof(struct argsv3, off);
		memmove(argsv3->fh + fhstore(d->version, argsv3->fh),
		    &argsv3->off, alen);
		alen += fhstore(d->version, argsv3->fh);

		cc = rpc_call(d->iodesc, NFS_PROG, d->version, NFSPROC_READ,
		    argsv3, alen,
		    replv3, sizeof(*replv3));
		break;
	default:
		errno = ENOSYS;
		return -1;
	}

	if (cc == -1) {
		/* errno was already set by rpc_call */
		return -1;
	}
	if (cc < (ssize_t)hlen) {
		errno = EBADRPC;
		return -1;
	}

	switch (d->version) {
	case NFS_VER2:
		if (replv2->errno) {
			errno = ntohl(replv2->errno);
			return -1;
		}
		x = ntohl(replv2->count);
		data = replv2->data;
		break;
	case NFS_VER3:
		if (replv3->errno) {
			errno = ntohl(replv3->errno);
			return -1;
		}

		/* adjust for optional attributes */
		if (replv3->attrflag) {
			x = ntohl(replv3->length);
			data = replv3->data;
		} else {
			replv3no = (struct replv3_noattr *)replv3;
			x = ntohl(replv3no->length);
			data = replv3no->data;
		}
		break;
	default:
		errno = ENOSYS;
		return -1;
	}

	rlen = cc - hlen;
	if (rlen < (size_t)x) {
		printf("%s: short packet, %zu < %ld\n", __func__, rlen, x);
		errno = EBADRPC;
		return -1;
	}
	(void)memcpy(addr, data, x);
	return x;
}

/*
 * nfs_mount - mount this nfs filesystem to a host
 * On error, return non-zero and set errno.
 */
int
nfs_mount(int sock, struct in_addr ip, char *path)
{
	struct iodesc *desc;
	struct nfsv2_fattr *fa2;
	struct nfsv3_fattr *fa3;

	if (!(desc = socktodesc(sock))) {
		errno = EINVAL;
		return -1;
	}

	/* Bind to a reserved port. */
	desc->myport = htons(--rpc_port);
	desc->destip = ip;
	if (nfs_getrootfh(desc, path, nfs_root_node.fh, &nfs_root_node.version))
		return -1;
	nfs_root_node.iodesc = desc;
	/* Fake up attributes for the root dir. */
	switch (nfs_root_node.version) {
	case NFS_VER2:
		fa2 = &nfs_root_node.u_fa.v2;
		fa2->fa_type  = htonl(NFDIR);
		fa2->fa_mode  = htonl(0755);
		fa2->fa_nlink = htonl(2);
		break;
	case NFS_VER3:
		fa3 = &nfs_root_node.u_fa.v3;
		fa3->fa_type  = htonl(NFDIR);
		fa3->fa_mode  = htonl(0755);
		fa3->fa_nlink = htonl(2);
		break;
	default:
		errno = ENOSYS;
		return -1;
	}

#ifdef NFS_DEBUG
	if (debug)
		printf("%s: got fh for %s\n", __func__, path);
#endif

	return 0;
}

/*
 * Open a file.
 * return zero or error number
 */
__compactcall int
nfs_open(const char *path, struct open_file *f)
{
	struct nfs_iodesc *newfd, *currfd;
	const char *cp;
#ifndef NFS_NOSYMLINK
	const char *ncp;
	int c;
	char namebuf[NFS_MAXPATHLEN + 1];
	char linkbuf[NFS_MAXPATHLEN + 1];
	int nlinks = 0;
	n_long fa_type;
#endif
	int error = 0;

#ifdef NFS_DEBUG
 	if (debug)
		printf("%s: %s\n", __func__, path);
#endif

#ifdef LIBSA_NFS_IMPLICIT_MOUNT
	if (nfs_mount(*((int *)(f->f_devdata)), rootip, rootpath))
		return errno;
#endif

	if (nfs_root_node.iodesc == NULL) {
		printf("%s: must mount first.\n", __func__);
		return ENXIO;
	}

	currfd = &nfs_root_node;
	newfd = 0;

#ifndef NFS_NOSYMLINK
	cp = path;
	while (*cp) {
		/*
		 * Remove extra separators
		 */
		while (*cp == '/')
			cp++;

		if (*cp == '\0')
			break;
		/*
		 * Check that current node is a directory.
		 */
		switch (currfd->version) {
		case NFS_VER2:
			fa_type = currfd->u_fa.v2.fa_type;
			break;
		case NFS_VER3:
			fa_type = currfd->u_fa.v3.fa_type;
			break;
		default:
			fa_type = htonl(NFNON);
			break;
		}
		if (fa_type != htonl(NFDIR)) {
			error = ENOTDIR;
			goto out;
		}

		/* allocate file system specific data structure */
		newfd = alloc(sizeof(*newfd));
		newfd->iodesc = currfd->iodesc;
		newfd->off = 0;
		newfd->version = currfd->version;

		/*
		 * Get next component of path name.
		 */
		{
			int len = 0;

			ncp = cp;
			while ((c = *cp) != '\0' && c != '/') {
				if (++len > NFS_MAXNAMLEN) {
					error = ENOENT;
					goto out;
				}
				cp++;
			}
		}

		/* lookup a file handle */
		error = nfs_lookupfh(currfd, ncp, cp - ncp, newfd);
		if (error)
			goto out;

		/*
		 * Check for symbolic link
		 */
		switch (newfd->version) {
		case NFS_VER2:
			fa_type = newfd->u_fa.v2.fa_type;
			break;
		case NFS_VER3:
			fa_type = newfd->u_fa.v3.fa_type;
			break;
		default:
			fa_type = htonl(NFNON);
			break;
		}
		if (fa_type == htonl(NFLNK)) {
			int link_len, len;

			error = nfs_readlink(newfd, linkbuf);
			if (error)
				goto out;

			link_len = strlen(linkbuf);
			len = strlen(cp);

			if (link_len + len > MAXPATHLEN
			    || ++nlinks > MAXSYMLINKS) {
				error = ENOENT;
				goto out;
			}

			(void)memcpy(&namebuf[link_len], cp, len + 1);
			(void)memcpy(namebuf, linkbuf, link_len);

			/*
			 * If absolute pathname, restart at root.
			 * If relative pathname, restart at parent directory.
			 */
			cp = namebuf;
			if (*cp == '/') {
				if (currfd != &nfs_root_node)
					dealloc(currfd, sizeof(*currfd));
				currfd = &nfs_root_node;
			}

			dealloc(newfd, sizeof(*newfd));
			newfd = 0;

			continue;
		}

		if (currfd != &nfs_root_node)
			dealloc(currfd, sizeof(*currfd));
		currfd = newfd;
		newfd = 0;
	}

	error = 0;

out:
#else
	/* allocate file system specific data structure */
	currfd = alloc(sizeof(*currfd));
	currfd->iodesc = nfs_root_node.iodesc;
	currfd->off = 0;
	currfd->version = nfs_root_node.version;

	cp = path;
	/*
	 * Remove extra separators
	 */
	while (*cp == '/')
		cp++;

	/* XXX: Check for empty path here? */

	error = nfs_lookupfh(&nfs_root_node, cp, strlen(cp), currfd);
#endif
	if (!error) {
		f->f_fsdata = (void *)currfd;
		fsmod = "nfs";
		return 0;
	}

#ifdef NFS_DEBUG
	if (debug)
		printf("%s: %s lookupfh failed: %s\n", __func__,
		    path, strerror(error));
#endif
	if (currfd != &nfs_root_node)
		dealloc(currfd, sizeof(*currfd));
	if (newfd)
		dealloc(newfd, sizeof(*newfd));

	return error;
}

__compactcall int
nfs_close(struct open_file *f)
{
	struct nfs_iodesc *fp = (struct nfs_iodesc *)f->f_fsdata;

#ifdef NFS_DEBUG
	if (debug)
		printf("%s: fp=%p\n", __func__, fp);
#endif

	if (fp)
		dealloc(fp, sizeof(struct nfs_iodesc));
	f->f_fsdata = (void *)0;

	return 0;
}

/*
 * read a portion of a file
 */
__compactcall int
nfs_read(struct open_file *f, void *buf, size_t size, size_t *resid)
{
	struct nfs_iodesc *fp = (struct nfs_iodesc *)f->f_fsdata;
	ssize_t cc;
	char *addr = buf;

#ifdef NFS_DEBUG
	if (debug)
		printf("%s: size=%zu off=%" PRIx64 "\n", __func__, size, fp->off);
#endif
	while ((int)size > 0) {
#if !defined(LIBSA_NO_TWIDDLE)
		twiddle();
#endif
		cc = nfs_readdata(fp, fp->off, (void *)addr, size);
		/* XXX maybe should retry on certain errors */
		if (cc == -1) {
#ifdef NFS_DEBUG
			if (debug)
				printf("%s: read: %s\n", __func__, 
				    strerror(errno));
#endif
			return errno;	/* XXX - from nfs_readdata */
		}
		if (cc == 0) {
#ifdef NFS_DEBUG
			if (debug)
				printf("%s: hit EOF unexpectedly\n", __func__);
#endif
			goto ret;
		}
		fp->off += cc;
		addr += cc;
		size -= cc;
	}
ret:
	if (resid)
		*resid = size;

	return 0;
}

/*
 * Not implemented.
 */
__compactcall int
nfs_write(struct open_file *f, void *buf, size_t size, size_t *resid)
{
	return EROFS;
}

__compactcall off_t
nfs_seek(struct open_file *f, off_t offset, int where)
{
	struct nfs_iodesc *d = (struct nfs_iodesc *)f->f_fsdata;
	off_t size;

	switch (d->version) {
	case NFS_VER2:
		size = ntohl(d->u_fa.v2.fa_size);
		break;
	case NFS_VER3:
		size = getnquad(d->u_fa.v3.fa_size);
		break;
	default:
		return -1;
	}

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
		return -1;
	}

	return d->off;
}

/* NFNON=0, NFREG=1, NFDIR=2, NFBLK=3, NFCHR=4, NFLNK=5 */
const int nfs_stat_types[8] = {
	0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK, 0 };

__compactcall int
nfs_stat(struct open_file *f, struct stat *sb)
{
	struct nfs_iodesc *fp = (struct nfs_iodesc *)f->f_fsdata;
	n_long ftype, mode;

	switch (fp->version) {
	case NFS_VER2:
		ftype = ntohl(fp->u_fa.v2.fa_type);
		mode  = ntohl(fp->u_fa.v2.fa_mode);
		sb->st_nlink = ntohl(fp->u_fa.v2.fa_nlink);
		sb->st_uid   = ntohl(fp->u_fa.v2.fa_uid);
		sb->st_gid   = ntohl(fp->u_fa.v2.fa_gid);
		sb->st_size  = ntohl(fp->u_fa.v2.fa_size);
		break;
	case NFS_VER3:
		ftype = ntohl(fp->u_fa.v3.fa_type);
		mode  = ntohl(fp->u_fa.v3.fa_mode);
		sb->st_nlink = ntohl(fp->u_fa.v3.fa_nlink);
		sb->st_uid   = ntohl(fp->u_fa.v3.fa_uid);
		sb->st_gid   = ntohl(fp->u_fa.v3.fa_gid);
		sb->st_size  = getnquad(fp->u_fa.v3.fa_size);
		break;
	default:
		return -1;
	}

	mode |= nfs_stat_types[ftype & 7];
	sb->st_mode  = mode;

	return 0;
}

#if defined(LIBSA_ENABLE_LS_OP)
#include "ls.h"
__compactcall void
nfs_ls(struct open_file *f, const char *pattern)
{
	lsunsup("nfs");
}
#endif
