/*	$NetBSD: ffs_vnops.c,v 1.16.2.3 1999/02/25 03:58:29 chs Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)ffs_vnops.c	8.15 (Berkeley) 5/14/95
 */

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_uvm.h"
#include "opt_uvmhist.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/signalvar.h>

#include <vm/vm.h>

#if defined(UVM)
#include <uvm/uvm_extern.h>
#endif

#include <miscfs/fifofs/fifo.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufsmount.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

/* Global vfs data structures for ufs. */
int (**ffs_vnodeop_p) __P((void *));
struct vnodeopv_entry_desc ffs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, ufs_lookup },		/* lookup */
	{ &vop_create_desc, ufs_create },		/* create */
	{ &vop_whiteout_desc, ufs_whiteout },		/* whiteout */
	{ &vop_mknod_desc, ufs_mknod },			/* mknod */
	{ &vop_open_desc, ufs_open },			/* open */
	{ &vop_close_desc, ufs_close },			/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, ufs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ffs_read },			/* read */
	{ &vop_write_desc, ffs_write },			/* write */
	{ &vop_lease_desc, ufs_lease_check },		/* lease */
	{ &vop_ioctl_desc, ufs_ioctl },			/* ioctl */
	{ &vop_poll_desc, ufs_poll },			/* poll */
	{ &vop_revoke_desc, ufs_revoke },		/* revoke */
	{ &vop_mmap_desc, ufs_mmap },			/* mmap */
	{ &vop_fsync_desc, ffs_fsync },			/* fsync */
	{ &vop_seek_desc, ufs_seek },			/* seek */
	{ &vop_remove_desc, ufs_remove },		/* remove */
	{ &vop_link_desc, ufs_link },			/* link */
	{ &vop_rename_desc, ufs_rename },		/* rename */
	{ &vop_mkdir_desc, ufs_mkdir },			/* mkdir */
	{ &vop_rmdir_desc, ufs_rmdir },			/* rmdir */
	{ &vop_symlink_desc, ufs_symlink },		/* symlink */
	{ &vop_readdir_desc, ufs_readdir },		/* readdir */
	{ &vop_readlink_desc, ufs_readlink },		/* readlink */
	{ &vop_abortop_desc, ufs_abortop },		/* abortop */
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ffs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, ufs_bmap },			/* bmap */
	{ &vop_strategy_desc, ufs_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_pathconf_desc, ufs_pathconf },		/* pathconf */
	{ &vop_advlock_desc, ufs_advlock },		/* advlock */
	{ &vop_blkatoff_desc, ffs_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, ffs_valloc },		/* valloc */
	{ &vop_reallocblks_desc, ffs_reallocblks },	/* reallocblks */
	{ &vop_vfree_desc, ffs_vfree },			/* vfree */
	{ &vop_truncate_desc, ffs_truncate },		/* truncate */
	{ &vop_update_desc, ffs_update },		/* update */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
#ifdef UBC
	{ &vop_getpages_desc, ffs_getpages },		/* getpages */
	{ &vop_putpages_desc, ffs_putpages },		/* putpages */
#endif
	{ NULL, NULL }
};
struct vnodeopv_desc ffs_vnodeop_opv_desc =
	{ &ffs_vnodeop_p, ffs_vnodeop_entries };

int (**ffs_specop_p) __P((void *));
struct vnodeopv_entry_desc ffs_specop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, spec_lookup },		/* lookup */
	{ &vop_create_desc, spec_create },		/* create */
	{ &vop_mknod_desc, spec_mknod },		/* mknod */
	{ &vop_open_desc, spec_open },			/* open */
	{ &vop_close_desc, ufsspec_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, ufs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ufsspec_read },		/* read */
	{ &vop_write_desc, ufsspec_write },		/* write */
	{ &vop_lease_desc, spec_lease_check },		/* lease */
	{ &vop_ioctl_desc, spec_ioctl },		/* ioctl */
	{ &vop_poll_desc, spec_poll },			/* poll */
	{ &vop_revoke_desc, spec_revoke },		/* revoke */
	{ &vop_mmap_desc, spec_mmap },			/* mmap */
	{ &vop_fsync_desc, ffs_fsync },			/* fsync */
	{ &vop_seek_desc, spec_seek },			/* seek */
	{ &vop_remove_desc, spec_remove },		/* remove */
	{ &vop_link_desc, spec_link },			/* link */
	{ &vop_rename_desc, spec_rename },		/* rename */
	{ &vop_mkdir_desc, spec_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, spec_rmdir },		/* rmdir */
	{ &vop_symlink_desc, spec_symlink },		/* symlink */
	{ &vop_readdir_desc, spec_readdir },		/* readdir */
	{ &vop_readlink_desc, spec_readlink },		/* readlink */
	{ &vop_abortop_desc, spec_abortop },		/* abortop */
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ffs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, spec_bmap },			/* bmap */
	{ &vop_strategy_desc, spec_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_pathconf_desc, spec_pathconf },		/* pathconf */
	{ &vop_advlock_desc, spec_advlock },		/* advlock */
	{ &vop_blkatoff_desc, spec_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, spec_valloc },		/* valloc */
	{ &vop_reallocblks_desc, spec_reallocblks },	/* reallocblks */
	{ &vop_vfree_desc, ffs_vfree },			/* vfree */
	{ &vop_truncate_desc, spec_truncate },		/* truncate */
	{ &vop_update_desc, ffs_update },		/* update */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ (struct vnodeop_desc*)NULL, (int(*) __P((void *)))NULL }
};
struct vnodeopv_desc ffs_specop_opv_desc =
	{ &ffs_specop_p, ffs_specop_entries };

int (**ffs_fifoop_p) __P((void *));
struct vnodeopv_entry_desc ffs_fifoop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, fifo_lookup },		/* lookup */
	{ &vop_create_desc, fifo_create },		/* create */
	{ &vop_mknod_desc, fifo_mknod },		/* mknod */
	{ &vop_open_desc, fifo_open },			/* open */
	{ &vop_close_desc, ufsfifo_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, ufs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ufsfifo_read },		/* read */
	{ &vop_write_desc, ufsfifo_write },		/* write */
	{ &vop_lease_desc, fifo_lease_check },		/* lease */
	{ &vop_ioctl_desc, fifo_ioctl },		/* ioctl */
	{ &vop_poll_desc, fifo_poll },			/* poll */
	{ &vop_revoke_desc, fifo_revoke },		/* revoke */
	{ &vop_mmap_desc, fifo_mmap },			/* mmap */
	{ &vop_fsync_desc, ffs_fsync },			/* fsync */
	{ &vop_seek_desc, fifo_seek },			/* seek */
	{ &vop_remove_desc, fifo_remove },		/* remove */
	{ &vop_link_desc, fifo_link },			/* link */
	{ &vop_rename_desc, fifo_rename },		/* rename */
	{ &vop_mkdir_desc, fifo_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, fifo_rmdir },		/* rmdir */
	{ &vop_symlink_desc, fifo_symlink },		/* symlink */
	{ &vop_readdir_desc, fifo_readdir },		/* readdir */
	{ &vop_readlink_desc, fifo_readlink },		/* readlink */
	{ &vop_abortop_desc, fifo_abortop },		/* abortop */
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ffs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, fifo_bmap },			/* bmap */
	{ &vop_strategy_desc, fifo_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_pathconf_desc, fifo_pathconf },		/* pathconf */
	{ &vop_advlock_desc, fifo_advlock },		/* advlock */
	{ &vop_blkatoff_desc, fifo_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, fifo_valloc },		/* valloc */
	{ &vop_reallocblks_desc, fifo_reallocblks },	/* reallocblks */
	{ &vop_vfree_desc, ffs_vfree },			/* vfree */
	{ &vop_truncate_desc, fifo_truncate },		/* truncate */
	{ &vop_update_desc, ffs_update },		/* update */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ (struct vnodeop_desc*)NULL, (int(*) __P((void *)))NULL }
};
struct vnodeopv_desc ffs_fifoop_opv_desc =
	{ &ffs_fifoop_p, ffs_fifoop_entries };

int doclusterread = 1;
int doclusterwrite = 1;

#include <ufs/ufs/ufs_readwrite.c>

/*
 * Reclaim an inode so that it can be used for other purposes.
 */
int
ffs_reclaim(v)
	void *v;
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;
	register struct vnode *vp = ap->a_vp;
	int error;

	if ((error = ufs_reclaim(vp, ap->a_p)) != 0)
		return (error);

	/*
	 * XXX MFS ends up here, too, to free an inode.  Should we create
	 * XXX a separate pool for MFS inodes?
	 */
	pool_put(&ffs_inode_pool, vp->v_data);
	vp->v_data = NULL;
	return (0);
}

#ifdef UBC
#include <uvm/uvm.h>

int
ffs_getpages(v)
	void *v;
{
	struct vop_getpages_args /* {
		struct vnode *a_vp;
		vaddr_t a_offset;
		vm_page_t *a_m;
		int *a_count;
		int a_centeridx;
		vm_prot_t a_access_type;
		int a_advice;
		int a_flags;
	} */ *ap = v;

	int i, error, npages;
	vm_page_t m;
	vaddr_t kva;
	struct buf tmpbuf, *bp;
	struct vnode *vp = ap->a_vp;
	struct uvm_object *uobj = &vp->v_uvm.u_obj;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;
	UVMHIST_FUNC("ffs_getpages"); UVMHIST_CALLED(ubchist);

#ifdef DIAGNOSTIC
	if (ap->a_centeridx < 0 || ap->a_centeridx > *ap->a_count) {
		panic("ffs_getpages: centeridx %d out of range",
		      ap->a_centeridx);
	}
#endif

	if (ap->a_flags & PGO_LOCKED) {
		uvn_findpages(uobj, ap->a_offset, ap->a_count, ap->a_m,
			      UFP_NOWAIT|UFP_NOALLOC);

		/* XXX PGO_ALLPAGES? */
		return ap->a_m[ap->a_centeridx] == NULL ?
			VM_PAGER_UNLOCK : VM_PAGER_OK;
	}

	/* vnode is VOP_LOCKed, uobj is locked */

	/*
	 * XXX
	 * first see if center page already exists.
	 * this is needed because ubc_fault() doesn't
	 * do a PGO_LOCKED call first.
	 * if the page exists, return it.
	 */

	npages = 1;
	uvn_findpages(uobj, ap->a_offset + (ap->a_centeridx << PAGE_SHIFT),
		      &npages, &ap->a_m[ap->a_centeridx], UFP_NOALLOC);
	if (npages == 1) {
		simple_unlock(&uobj->vmobjlock);
		return VM_PAGER_OK;
	}

	npages = 1;
	uvn_findpages(uobj, ap->a_offset, &npages, ap->a_m, 0);
	m = ap->a_m[ap->a_centeridx];

	simple_unlock(&uobj->vmobjlock);

	if ((m->flags & PG_FAKE) == 0) {
		UVMHIST_LOG(ubchist, "page was valid",0,0,0,0);
		return VM_PAGER_OK;
	}

	bp = &tmpbuf;
	bzero(bp, sizeof *bp);

	bp->b_lblkno = lblkno(fs, m->offset);
	if (m->blkno == 0) {
		error = VOP_BMAP(vp, bp->b_lblkno, NULL, &bp->b_blkno, NULL);
		if (error) {
UVMHIST_LOG(ubchist, "VOP_BMAP error %d", error,0,0,0);
			return (error);
		}
UVMHIST_LOG(ubchist, "VOP_BMAP gave 0x%x", bp->b_blkno,0,0,0);
		if (bp->b_blkno == (daddr_t)-1) {

			/*
			 * XXX pre-allocate blocks here someday?
			 * ... only for write accesses.
			 * for read accesses we can skip the block allocation
			 * if we can arrange to be called again
			 * when a subsequent write access occurs.
			 * needs support in uvm_fault().
			 */
			uvm_pagezero(m);
			m->flags &= ~(PG_FAKE);
			pmap_clear_modify(PMAP_PGARG(m));
			pmap_clear_reference(PMAP_PGARG(m));
			return VM_PAGER_OK;
		}
	}

	/* adjust physical blkno for partial blocks */
	bp->b_blkno += (m->offset - lblktosize(fs, bp->b_lblkno)) >> DEV_BSHIFT;
	m->blkno = bp->b_blkno;

UVMHIST_LOG(ubchist, "vp %p lblkno 0x%x blkno 0x%x",
	    vp, bp->b_lblkno, bp->b_blkno, 0);

	/*
	 * don't bother reading the pages if we're just going to
	 * overwrite them.
	 */
	if (ap->a_flags & PGO_OVERWRITE) {
		for (i = 0; i < *ap->a_count; i++) {

			/* XXX for now, zero the pages */
			if (ap->a_m[i]->flags & PG_FAKE) {
				uvm_pagezero(ap->a_m[i]);
				ap->a_m[i]->flags &= ~(PG_FAKE);
			}
		}
		return VM_PAGER_OK;
	}

	kva = uvm_pagermapin(ap->a_m, *ap->a_count, NULL, M_WAITOK);
	if (kva == 0) {
		/* XXX unbusy pages */
		return VM_PAGER_AGAIN;
	}

	bp->b_bufsize = PAGE_SIZE;
	bp->b_data = (void *)kva;
	bp->b_bcount = bp->b_lblkno < 0 ? bp->b_bufsize :
		min(bp->b_bufsize,
		    fragroundup(fs, vp->v_uvm.u_size - m->offset));
	bp->b_vp = vp;
	bp->b_flags = B_BUSY|B_READ;

UVMHIST_LOG(ubchist, "bp %p vp %p blkno 0x%x", bp, vp, bp->b_lblkno, 0);

	VOP_STRATEGY(bp);
	error = biowait(bp);

UVMHIST_LOG(ubchist, "error %d", error,0,0,0);

	uvm_pagermapout(kva, *ap->a_count);

	if (error) {
		simple_lock(&uobj->vmobjlock);
		if (m->flags & PG_WANTED)
			wakeup(m);
		m->flags &= ~(PG_WANTED|PG_BUSY);
		UVM_PAGE_OWN(m, NULL);
		uvm_lock_pageq();
		uvm_pagefree(m);
		uvm_unlock_pageq();
		simple_unlock(&uobj->vmobjlock);
	} else { 
		m->flags &= ~(PG_FAKE);
		pmap_clear_modify(PMAP_PGARG(m));
	}

	return error ? VM_PAGER_ERROR : VM_PAGER_OK;
}

/*
 * Vnode op for VM putpages.
 */
int
ffs_putpages(v)
	void *v;
{
	struct vop_putpages_args /* {
		struct vnode *a_vp;
		struct vm_page **a_m;
		int a_count;
		int a_sync;
		int *a_rtvals;
	} */ *ap = v;

	int s, error;
	vaddr_t kva, offset;
	struct vm_page *pg;
	struct buf tmpbuf, *bp;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;
	struct ucred *cred = curproc->p_ucred;	/* XXX curproc */
	UVMHIST_FUNC("ffs_putpages"); UVMHIST_CALLED(ubchist);

	pg = ap->a_m[0];

#ifdef DIAGNOSTIC
	if (ap->a_count > 1) {
		int i;

		for (i = 1; i < ap->a_count; i++) {
			if (ap->a_m[i]->blkno !=
			    pg->blkno + i * (PAGE_SIZE >> DEV_BSHIFT)) {
				panic("ffs_putpages: pg %p %p not clusterable",
				      pg, ap->a_m[i]);
			}
		}
	}
#endif

	kva = uvm_pagermapin(ap->a_m, ap->a_count, NULL, M_WAITOK);
	if (kva == 0) {
		return VM_PAGER_AGAIN;
	}

	if (ap->a_sync) {
		bp = &tmpbuf;
		bzero(bp, sizeof *bp);
	}
	else {
		struct uvm_aiobuf *abp = pool_get(uvm_aiobuf_pool, PR_WAITOK);
		abp->aio.aiodone = uvm_aio_aiodone;
		abp->aio.kva = kva;
		abp->aio.npages = ap->a_count;
		abp->aio.pd_ptr = abp;
		bp = &abp->buf;
		bzero(bp, sizeof *bp);
		bp->b_flags = B_CALL|B_ASYNC;
		bp->b_iodone = uvm_aio_biodone;
	}

	offset = pg->offset;
	bp->b_bufsize = ap->a_count << PAGE_SHIFT;
	bp->b_data = (void *)kva;
	bp->b_lblkno = lblkno(fs, offset);
	bp->b_bcount = min(bp->b_bufsize,
			   fragroundup(fs, vp->v_uvm.u_size - offset));
	bp->b_flags |= B_BUSY|B_WRITE;
	bp->b_vp = vp;

	/*
	 * if the blkno wasn't set in the page, do a bmap to find it.
	 * this should only happen if the block is unallocated.
	 * we'll always want to set the page's blkno if it's allocated
	 * to help prevent memory deadlocks.  we probably even want to
	 * allocate blocks in VOP_GETPAGES() for this same reason.
	 * 
	 * once we switch to doing that, the page should always have a blkno.
	 * but for now, we'll handle allocating blocks here too.
	 * 
	 * XXX this doesn't handle blocksizes smaller than PAGE_SIZE.
	 */

	bp->b_blkno = pg->blkno;
	if (bp->b_blkno == 0) {
		int bsize;

#ifdef DEBUG
		error = VOP_BMAP(vp, bp->b_lblkno, NULL, &bp->b_blkno, NULL);
		if (error) {
			goto out;
		}
		if (bp->b_blkno != (daddr_t)-1) {
			panic("ffs_putpage: page %p has blkno 0, not %d\n",
			      pg, bp->b_blkno);
		}
#endif
		bsize = blksize(fs, ip, lblkno(fs, offset));
		error = ffs_balloc(ip, bp->b_lblkno, bsize, cred, NULL,
				   &bp->b_blkno, 0, NULL);
		if (error) {
			UVMHIST_LOG(ubchist, "ffs_balloc -> %d", error, 0,0,0);
			goto out;
		}

		/* adjust physical blkno for partial blocks */
		bp->b_blkno += ((offset - lblktosize(fs, bp->b_lblkno))
				>> DEV_BSHIFT);
		pg->blkno = bp->b_blkno;
	}

	UVMHIST_LOG(ubchist, "pg %p vp %p lblkno 0x%x blkno 0x%x",
		    pg, vp, bp->b_lblkno, bp->b_blkno);

	s = splbio();
	vp->v_numoutput++;
	splx(s);

	VOP_STRATEGY(bp);
	if (!ap->a_sync) {
		return VM_PAGER_PEND;
	}

	error = biowait(bp);

out:
UVMHIST_LOG(ubchist, "error %d", error,0,0,0);
	uvm_pagermapout(kva, ap->a_count);

	return error ? VM_PAGER_ERROR : VM_PAGER_OK;
}
#endif
