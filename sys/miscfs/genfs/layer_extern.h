/*	$NetBSD: layer_extern.h,v 1.2 2000/03/13 23:52:40 soren Exp $	*/

/*
 * Copyright (c) 1999 National Aeronautics & Space Administration
 * All rights reserved.
 *
 * This software was written by William Studenmund of the
 * Numerical Aerospace Similation Facility, NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the National Aeronautics & Space Administration
 *    nor the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NATIONAL AERONAUTICS & SPACE ADMINISTRATION
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ADMINISTRATION OR CONTRIB-
 * UTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1992, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 *	California, Berkeley and its contributors.
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
 */

/*
 * Routines defined by layerfs
 */

/* misc routines in layer_subr.c */
void	layerfs_init __P((void));
int	layer_node_alloc __P((struct mount *, struct vnode *, struct vnode **));
int	layer_node_create __P((struct mount *, struct vnode *, struct vnode **));
struct vnode *
	layer_node_find __P((struct mount *, struct vnode *));
#define LOG2_SIZEVNODE	7		/* log2(sizeof struct vnode) */
#define LAYER_NHASH(lmp, vp) \
	(&((lmp)->layerm_node_hashtbl[(((u_long)vp)>>LOG2_SIZEVNODE) & \
		(lmp)->layerm_node_hash]))

/* vfs routines */
int	layerfs_start __P((struct mount *, int, struct proc *));
int	layerfs_root __P((struct mount *, struct vnode **));
int	layerfs_quotactl __P((struct mount *, int, uid_t, caddr_t,
			     struct proc *));
int	layerfs_statfs __P((struct mount *, struct statfs *, struct proc *));
int	layerfs_sync __P((struct mount *, int, struct ucred *, struct proc *));
int	layerfs_vget __P((struct mount *, ino_t, struct vnode **));
int	layerfs_fhtovp __P((struct mount *, struct fid *, struct vnode **));
int	layerfs_checkexp __P((struct mount *, struct mbuf *, int *,
			   struct ucred **));
int	layerfs_vptofh __P((struct vnode *, struct fid *));
int	layerfs_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
			   struct proc *));

/* VOP routines */
int	layer_bypass __P((void *));
int	layer_getattr __P((void *));
int	layer_inactive __P((void *));
int	layer_reclaim __P((void *));
int	layer_print __P((void *));
int	layer_strategy __P((void *));
int	layer_bwrite __P((void *));
int	layer_bmap __P((void *));
int	layer_lock __P((void *));
int	layer_unlock __P((void *));
int	layer_islocked __P((void *));
int	layer_fsync __P((void *));
int	layer_lookup __P((void *));
int	layer_setattr __P((void *));
int	layer_access __P((void *));
int	layer_open __P((void *));
