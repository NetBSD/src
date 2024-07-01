/* $NetBSD: exfatfs_vnops.h,v 1.1.2.2 2024/07/01 22:15:21 perseant Exp $ */

/*-
 * Copyright (c) 2022, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#ifndef EXFATFS_VNOPS_H_
#define EXFATFS_VNOPS_H_

int exfatfs_access(void *);
int exfatfs_activate(struct xfinode *, bool);
int exfatfs_advlock(void *);
int exfatfs_bmap(void *);
int exfatfs_close(void *);
int exfatfs_create(void *);
int exfatfs_deactivate(struct xfinode *, bool);
int exfatfs_findempty(struct vnode *, struct xfinode *);
int exfatfs_fsync(void *);
int exfatfs_getattr(void *);
int exfatfs_inactive(void *);
void exfatfs_itimes(struct xfinode *, const struct timespec *,
		    const struct timespec *, const struct timespec *);
int exfatfs_lookup(void *);
int exfatfs_mkdir(void *);
int exfatfs_pathconf(void *);
int exfatfs_print(void *);
int exfatfs_read(void *);
int exfatfs_readdir(void *);
int exfatfs_readlink(void *);
int exfatfs_reclaim(void *);
int exfatfs_rekey(struct vnode *, struct exfatfs_dirent_key *);
int exfatfs_remove(void *);
int exfatfs_rename(void *);
int exfatfs_rmdir(void *);
int exfatfs_setattr(void *);
int exfatfs_strategy(void *);
int exfatfs_symlink(void *);
int exfatfs_update(struct vnode *, const struct timespec *,
		   const struct timespec *, int);
int exfatfs_write(void *);
int exfatfs_writeback(struct xfinode *);
int exfatfs_writeback2(struct xfinode *, struct xfinode *);

#endif /* EXFATFS_VNOPS_H_ */
