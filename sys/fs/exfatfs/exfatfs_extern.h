/*	$NetBSD: exfatfs_extern.h,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
#include <sys/types.h>
#include <sys/vnode.h>
#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_inode.h>
#include <fs/exfatfs/exfatfs_mount.h>

int exfatfs_bmap_shared(struct vnode *vp, daddr_t bn, struct vnode **vpp,
			daddr_t *bnp, int *runp);
int exfatfs_mountfs_shared(struct vnode *devvp, struct exfatfs_mount *, unsigned secsize, struct exfatfs **fsp);
struct xfinode *exfatfs_newxfinode(struct exfatfs *, uint32_t, uint32_t);
struct exfatfs_dirent *exfatfs_newdirent(void);
int exfatfs_get_file_name(struct xfinode *, uint16_t *, int *, int);
int exfatfs_set_file_name(struct xfinode *, uint16_t *, int);
void exfatfs_freexfinode(struct xfinode *);
void exfatfs_freedirent(struct exfatfs_dirent *);
int exfatfs_scandir(struct vnode *dvp, off_t startoff, off_t *endoff,
		    unsigned (*emptyfunc)(void *, off_t, off_t),
		    unsigned (*validfunc)(void *, struct xfinode *, off_t),
		    void *arg);
#define SCANDIR_STOP     0x00000001
#define SCANDIR_DONTFREE 0x00000002
