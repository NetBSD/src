/*	$NetBSD: ukfs.h,v 1.9.8.2 2008/07/18 16:37:57 simonb Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Finnish Cultural Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_RUMPFS_UKFS_H_
#define _SYS_RUMPFS_UKFS_H_

#include <sys/types.h>
#include <sys/stat.h>

#include <stdbool.h>
#include <stdint.h>

struct vnode;
struct ukfs;

#include "rump.h"

int		ukfs_init(void);
struct ukfs	*ukfs_mount(const char *, const char *, const char *,
			  int, void *, size_t);
void		ukfs_release(struct ukfs *, int);

int		ukfs_ll_namei(struct ukfs *, uint32_t, uint32_t, const char *,
			      struct vnode **, struct vnode **,
			      struct componentname **);
void		ukfs_ll_rele(struct vnode *);

int		ukfs_getdents(struct ukfs *, const char *, off_t *,
			      uint8_t *, size_t);
ssize_t		ukfs_read(struct ukfs *, const char *, off_t,
			      uint8_t *, size_t);
ssize_t		ukfs_write(struct ukfs *, const char *, off_t,
			       uint8_t *, size_t);
ssize_t		ukfs_readlink(struct ukfs *, const char *, char *, size_t);

int		ukfs_create(struct ukfs *, const char *, mode_t);
int		ukfs_mkdir(struct ukfs *, const char *, mode_t);
int		ukfs_mknod(struct ukfs *, const char *, mode_t, dev_t);
int		ukfs_mkfifo(struct ukfs *, const char *, mode_t);
int		ukfs_symlink(struct ukfs *, const char *, const char *);

int		ukfs_remove(struct ukfs *, const char *);
int		ukfs_rmdir(struct ukfs *, const char *);

int		ukfs_link(struct ukfs *, const char *, const char *);
int		ukfs_rename(struct ukfs *, const char *, const char *);

int		ukfs_chdir(struct ukfs *, const char *);

int		ukfs_stat(struct ukfs *, const char *, struct stat *);
int		ukfs_lstat(struct ukfs *, const char *, struct stat *);

int		ukfs_chmod(struct ukfs *, const char *, mode_t);
int		ukfs_lchmod(struct ukfs *, const char *, mode_t);
int		ukfs_chown(struct ukfs *, const char *, uid_t, gid_t);
int		ukfs_lchown(struct ukfs *, const char *, uid_t, gid_t);
int		ukfs_chflags(struct ukfs *, const char *, u_long);
int		ukfs_lchflags(struct ukfs *, const char *, u_long);

int		ukfs_utimes(struct ukfs *, const char *, 
			    const struct timeval *);
int		ukfs_lutimes(struct ukfs *, const char *, 
			     const struct timeval *);

struct mount	*ukfs_getmp(struct ukfs *);
struct vnode	*ukfs_getrvp(struct ukfs *);

#define UKFS_UIOINIT(uio, iov, buf, bufsize, offset, rw)		\
do {									\
	iov.iov_base = buf;						\
	iov.iov_len = bufsize;						\
	uio.uio_iov = &(iov);						\
	uio.uio_iovcnt = 1;						\
	uio.uio_offset = offset;					\
	uio.uio_resid = bufsize;					\
	uio.uio_rw = rw;						\
	uio.uio_vmspace = UIO_VMSPACE_SYS;				\
} while (/*CONSTCOND*/0)


#endif /* _SYS_RUMPFS_UKFS_H_ */
