/* $NetBSD: exfatfs_vfsops.h,v 1.1.2.2 2024/07/01 22:15:21 perseant Exp $ */

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

#ifndef EXFATFS_VFSOPS_H_
#define EXFATFS_VFSOPS_H_

int exfatfs_getnewvnode(struct exfatfs *, struct vnode *,
			uint32_t, uint32_t, unsigned,
			void *, struct vnode **);

#ifdef _KERNEL
void exfatfs_init(void);
int exfatfs_loadvnode(struct mount *, struct vnode *, const void *, size_t,
	const void **);
int exfatfs_mount(struct mount *, const char *, void *, size_t *);
int exfatfs_mountfs(struct vnode *, struct mount *, struct lwp *,
	struct exfatfs_args *);
int exfatfs_finish_mountfs(struct exfatfs *);
int exfatfs_mountroot(void);
/* void exfatfs_reinit(void); */
int exfatfs_root(struct mount *, int, struct vnode **);
int exfatfs_start(struct mount *, int);
int exfatfs_statvfs(struct mount *, struct statvfs *);
int exfatfs_sync(struct mount *, int, kauth_cred_t);
int exfatfs_unmount(struct mount *, int);
int exfatfs_vget(struct mount *, ino_t, int, struct vnode **);
#endif /* _KERNEL */

#endif /* EXFATFS_VFSOPS_H_ */
