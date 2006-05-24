/*	$NetBSD: specdev.h,v 1.29.8.1 2006/05/24 10:58:55 yamt Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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
#ifndef _MISCFS_SPECFS_SPECDEV_H_
#define _MISCFS_SPECFS_SPECDEV_H_

/*
 * This structure defines the information maintained about
 * special devices. It is allocated in checkalias and freed
 * in vgone.
 */
struct spec_cow_entry {
	SLIST_ENTRY(spec_cow_entry) ce_list;
	int (*ce_func)(void *, struct buf *);
	void *ce_cookie;
};

struct specinfo {
	struct	vnode **si_hashchain;
	struct	vnode *si_specnext;
	struct	mount *si_mountpoint;
	dev_t	si_rdev;
	struct	lockf *si_lockf;
	struct simplelock si_cow_slock;
	SLIST_HEAD(, spec_cow_entry) si_cow_head;
	int si_cow_req;
	int si_cow_count;
};
/*
 * Exported shorthand
 */
#define v_rdev		v_specinfo->si_rdev
#define v_hashchain	v_specinfo->si_hashchain
#define v_specnext	v_specinfo->si_specnext
#define v_speclockf	v_specinfo->si_lockf
#define v_specmountpoint v_specinfo->si_mountpoint
#define v_spec_cow_slock v_specinfo->si_cow_slock
#define v_spec_cow_head	v_specinfo->si_cow_head
#define v_spec_cow_req	v_specinfo->si_cow_req
#define v_spec_cow_count v_specinfo->si_cow_count

#define SPEC_COW_LOCK(si, s) \
	do { \
		(s) = splbio(); \
		simple_lock(&(si)->si_cow_slock) ; \
	} while (/*CONSTCOND*/0)

#define SPEC_COW_UNLOCK(si, s) \
	do { \
		simple_unlock(&(si)->si_cow_slock) ; \
		splx((s)); \
	} while (/*CONSTCOND*/0)

/*
 * Special device management
 */
#define	SPECHSZ	64
#if	((SPECHSZ&(SPECHSZ-1)) == 0)
#define	SPECHASH(rdev)	(((rdev>>5)+(rdev))&(SPECHSZ-1))
#else
#define	SPECHASH(rdev)	(((unsigned)((rdev>>5)+(rdev)))%SPECHSZ)
#endif

extern	struct vnode *speclisth[SPECHSZ];

/*
 * Prototypes for special file operations on vnodes.
 */
extern	int (**spec_vnodeop_p)(void *);
struct	nameidata;
struct	componentname;
struct	flock;
struct	buf;
struct	uio;

int	spec_lookup(void *);
#define	spec_create	genfs_badop
#define	spec_mknod	genfs_badop
int	spec_open(void *);
int	spec_close(void *);
#define	spec_access	genfs_ebadf
#define	spec_getattr	genfs_ebadf
#define	spec_setattr	genfs_ebadf
int	spec_read(void *);
int	spec_write(void *);
#define	spec_lease_check genfs_nullop
#define spec_fcntl	genfs_fcntl
int	spec_ioctl(void *);
int	spec_poll(void *);
int	spec_kqfilter(void *);
#define spec_revoke	genfs_revoke
#define	spec_mmap	genfs_mmap
int	spec_fsync(void *);
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
int	spec_inactive(void *);
#define	spec_lock	genfs_nolock
#define	spec_unlock	genfs_nounlock
int	spec_bmap(void *);
int	spec_strategy(void *);
int	spec_print(void *);
#define	spec_islocked	genfs_noislocked
int	spec_pathconf(void *);
int	spec_advlock(void *);
#define	spec_bwrite	vn_bwrite
#define	spec_getpages	genfs_getpages
#define	spec_putpages	genfs_putpages
int	spec_size(void *);

#endif /* _MISCFS_SPECFS_SPECDEV_H_ */
