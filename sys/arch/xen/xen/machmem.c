/* $NetBSD: machmem.c,v 1.1 2004/05/07 15:51:04 cl Exp $ */

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 2004 Christian Limpach.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machmem.c,v 1.1 2004/05/07 15:51:04 cl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/stat.h>

#include <uvm/uvm.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/kernfs/kernfs.h>

#include <machine/kernfs_machdep.h>
#include <machine/pmap.h>

#define	MACHMEM_MODE	(S_IRUSR)

/*  #define	MACHMEM_DEBUG */
#ifdef MACHMEM_DEBUG
#define	DPRINTF(x) printf x
#else
#define	DPRINTF(x)
#endif

/*
 * functions
 */

static void	machmem_reference(struct uvm_object *);
static void	machmem_detach(struct uvm_object *);
static int	machmem_fault(struct uvm_faultinfo *, vaddr_t,
    struct vm_page **, int, int, vm_fault_t, vm_prot_t, int);

/*
 * master pager structure
 */

static struct uvm_pagerops machmem_deviceops = {
	NULL,
	machmem_reference,
	machmem_detach,
	machmem_fault,
};

/*
 * machmem_reference
 *
 * duplicate a reference to a VM object.  Note that the reference
 * count must already be at least one (the passed in reference) so
 * there is no chance of the uvn being killed or locked out here.
 *
 * => caller must call with object unlocked.
 * => caller must be using the same accessprot as was used at attach time
 */

static void
machmem_reference(struct uvm_object *uobj)
{
	VREF((struct vnode *)uobj);
}

/*
 * machmem_detach
 *
 * remove a reference to a VM object.
 *
 * => caller must call with object unlocked and map locked.
 */

static void
machmem_detach(struct uvm_object *uobj)
{

	simple_lock(&uobj->vmobjlock);
	if (uobj->uo_refs > 1) {
		uobj->uo_refs--;
		DPRINTF(("machmem_detach %p refs %d\n", uobj, uobj->uo_refs));
		simple_unlock(&uobj->vmobjlock);
		return;
	}

	DPRINTF(("machmem_detach final %p refs %d\n", uobj, uobj->uo_refs));
	/*
	 * On process exist, close happens before munmap and we might
	 * clear the last reference => cleanup pgops
	 */
	uobj->pgops = &uvm_vnodeops;

	simple_unlock(&uobj->vmobjlock);
	vrele((struct vnode *)uobj);
}

/*
 * machmem_fault: non-standard fault routine for device "pages"
 *
 * See comments at uvm/uvm_device.c:udv_fault
 *
 */

static int
machmem_fault(struct uvm_faultinfo *ufi, vaddr_t vaddr, struct vm_page **pps,
    int npages, int centeridx, int flags, vm_fault_t fault_type,
    vm_prot_t access_type)
{
	struct vm_map_entry *entry = ufi->entry;
	struct uvm_object *uobj = entry->object.uvm_obj;
	vaddr_t curr_va;
	off_t curr_offset;
	paddr_t paddr;
	int lcv, retval;
	vm_prot_t mapprot;
	boolean_t wired;

	/*
	 * we do not allow device mappings to be mapped copy-on-write
	 * so we kill any attempt to do so here.
	 */

	if (UVM_ET_ISCOPYONWRITE(entry)) {
		uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, uobj, NULL);
		return(EIO);
	}

	/*
	 * now we must determine the offset in udv to use and the VA to
	 * use for pmap_enter.  note that we always use orig_map's pmap
	 * for pmap_enter (even if we have a submap).   since virtual
	 * addresses in a submap must match the main map, this is ok.
	 */

	/* udv offset = (offset from start of entry) + entry's offset */
	curr_offset = entry->offset + (vaddr - entry->start);
	/* pmap va = vaddr (virtual address of pps[0]) */
	curr_va = vaddr;

	wired = VM_MAPENT_ISWIRED(entry) || fault_type == VM_FAULT_WIRE ||
		fault_type == VM_FAULT_WIREMAX;

	DPRINTF(("machmem_fault va %08lx ma %08lx npages %d\n", 
	    vaddr, (unsigned long)curr_offset, npages));
	DPRINTF(("machmem_fault %p/%d va %08lx ma %08lx npages %d\n", 
	    entry, entry->wired_count, vaddr, (unsigned long)curr_offset,
	    npages));

	/*
	 * loop over the page range entering in as needed
	 */

	retval = 0;
	for (lcv = 0 ; lcv < npages ; lcv++, curr_offset += PAGE_SIZE,
	    curr_va += PAGE_SIZE) {
		if ((flags & PGO_ALLPAGES) == 0 && lcv != centeridx)
			continue;

		if (pps[lcv] == PGO_DONTCARE)
			continue;

		paddr = curr_offset;
		KDASSERT((paddr & PAGE_MASK) == 0);
		mapprot = ufi->entry->protection;
		if (pmap_enter_ma(ufi->orig_map->pmap, curr_va, paddr,
		    mapprot, PMAP_CANFAIL | mapprot |
		    (wired ? PMAP_WIRED : 0)) != 0) {
			/*
			 * pmap_enter_ma() didn't have the resource to
			 * enter this mapping.  Unlock everything,
			 * wait for the pagedaemon to free up some
			 * pages, and then tell uvm_fault() to start
			 * the fault again.
			 *
			 * XXX Needs some rethinking for the PGO_ALLPAGES
			 * XXX case.
			 */
			uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap,
			    uobj, NULL);
			/* sync what we have so far */
			pmap_update(ufi->orig_map->pmap);
			uvm_wait("udv_fault");
			return (ERESTART);
		}
	}

	uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, uobj, NULL);
	pmap_update(ufi->orig_map->pmap);
	return (retval);
}


static int
machmem_open(void *v)
{
	struct vop_open_args /* {
		struct vnode *a_vp;
		int a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	extern paddr_t pmap_mem_end;

	DPRINTF(("machmem_open %p refs %d\n", &ap->a_vp->v_uobj,
	    ap->a_vp->v_uobj.uo_refs));
	if (ap->a_vp->v_uobj.uo_refs <= 1)
		ap->a_vp->v_uobj.pgops = &machmem_deviceops;

	ap->a_vp->v_size = pmap_mem_end;

	return 0;
}

static int
machmem_close(void *v)
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;

	DPRINTF(("machmem_close %p refs %d\n", &ap->a_vp->v_uobj,
	    ap->a_vp->v_uobj.uo_refs));
	if (ap->a_vp->v_uobj.uo_refs <= 1)
		ap->a_vp->v_uobj.pgops = &uvm_vnodeops;

	return 0;
}

static const struct kernfs_fileop machmem_fileops[] = {
  { .kf_fileop = KERNFS_FILEOP_CLOSE, .kf_vop = machmem_close },
  { .kf_fileop = KERNFS_FILEOP_OPEN, .kf_vop = machmem_open },
};

void
xenmachmem_init()
{
	kernfs_entry_t *dkt;
	kfstype kfst;

	if ((xen_start_info.flags & SIF_PRIVILEGED) == 0)
		return;

	kfst = KERNFS_ALLOCTYPE(machmem_fileops);

	KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
	KERNFS_INITENTRY(dkt, DT_REG, "machmem", NULL, kfst, VREG,
	    MACHMEM_MODE);
	kernfs_addentry(kernxen_pkt, dkt);
}
