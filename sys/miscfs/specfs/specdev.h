/*	$NetBSD: specdev.h,v 1.18 1999/11/15 18:49:10 fvdl Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)specdev.h	8.6 (Berkeley) 5/21/95
 */

/*
 * This structure defines the information maintained about
 * special devices. It is allocated in checkalias and freed
 * in vgone.
 */
struct specinfo {
	struct	vnode **si_hashchain;
	struct	vnode *si_specnext;
	struct	mount *si_mountpoint;
	dev_t	si_rdev;
	struct	lockf *si_lockf;
};
/*
 * Exported shorthand
 */
#define v_rdev		v_specinfo->si_rdev
#define v_hashchain	v_specinfo->si_hashchain
#define v_specnext	v_specinfo->si_specnext
#define v_speclockf	v_specinfo->si_lockf
#define v_specmountpoint v_specinfo->si_mountpoint

/*
 * Special device management
 */
#define	SPECHSZ	64
#if	((SPECHSZ&(SPECHSZ-1)) == 0)
#define	SPECHASH(rdev)	(((rdev>>5)+(rdev))&(SPECHSZ-1))
#else
#define	SPECHASH(rdev)	(((unsigned)((rdev>>5)+(rdev)))%SPECHSZ)
#endif

struct vnode *speclisth[SPECHSZ];

/*
 * Prototypes for special file operations on vnodes.
 */
extern	int (**spec_vnodeop_p) __P((void *));
struct	nameidata;
struct	componentname;
struct	ucred;
struct	flock;
struct	buf;
struct	uio;

int	spec_lookup	__P((void *));
#define	spec_create	genfs_badop
#define	spec_mknod	genfs_badop
int	spec_open	__P((void *));
int	spec_close	__P((void *));
#define	spec_access	genfs_ebadf
#define	spec_getattr	genfs_ebadf
#define	spec_setattr	genfs_ebadf
int	spec_read	__P((void *));
int	spec_write	__P((void *));
#define	spec_lease_check genfs_nullop
int	spec_ioctl	__P((void *));
int	spec_poll	__P((void *));
#define spec_revoke	genfs_revoke
#define	spec_mmap	genfs_badop
int	spec_fsync	__P((void *));
#define	spec_seek	genfs_nullop		/* XXX should query device */
#define	spec_remove	genfs_badop
#define	spec_link	genfs_badop
#define	spec_rename	genfs_badop
#define	spec_mkdir	genfs_badop
#define	spec_rmdir	genfs_badop
#define	spec_symlink	genfs_badop
#define	spec_readdir	genfs_badop
#define	spec_readlink	genfs_badop
#define	spec_abortop	genfs_badop
#define	spec_reclaim	genfs_nullop
int	spec_inactive	__P((void *));
#define	spec_lock	genfs_nolock
#define	spec_unlock	genfs_nounlock
int	spec_bmap	__P((void *));
int	spec_strategy	__P((void *));
int	spec_print	__P((void *));
#define	spec_islocked	genfs_noislocked
int	spec_pathconf	__P((void *));
int	spec_advlock	__P((void *));
#define	spec_blkatoff	genfs_badop
#define	spec_valloc	genfs_badop
#define	spec_reallocblks genfs_badop
#define	spec_vfree	genfs_badop
#define	spec_truncate	genfs_nullop
#define	spec_update	genfs_nullop
#define	spec_bwrite	vn_bwrite
