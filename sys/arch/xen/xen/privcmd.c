/* $NetBSD: privcmd.c,v 1.66 2022/09/01 15:32:16 bouyer Exp $ */

/*-
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
__KERNEL_RCSID(0, "$NetBSD: privcmd.c,v 1.66 2022/09/01 15:32:16 bouyer Exp $");

#include "opt_xen.h"

#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/proc.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/kernfs/kernfs.h>

#include <uvm/uvm.h>
#include <uvm/uvm_fault.h>
#include <uvm/uvm_fault_i.h>

#include <xen/kernfs_machdep.h>
#include <xen/hypervisor.h>
#include <xen/xen.h>
#include <xen/xenio.h>
#include <xen/xenmem.h>
#include <xen/xenpmap.h>
#include <xen/granttables.h>

#define	PRIVCMD_MODE	(S_IRUSR)

/* Magic value is used to mark invalid pages.
 * This must be a value within the page-offset.
 * Page-aligned values including 0x0 are used by the guest.
 */
#define INVALID_PAGE	0xfff

typedef enum _privcmd_type {
	PTYPE_PRIVCMD,
	PTYPE_PRIVCMD_PHYSMAP,
	PTYPE_GNTDEV_REF,
	PTYPE_GNTDEV_ALLOC
} privcmd_type;

struct privcmd_object_privcmd {
	paddr_t base_paddr; /* base address of physical space */
        paddr_t *maddr; /* array of machine address to map */
        int     domid;
        bool    no_translate;
};

struct privcmd_object_gntref {
	paddr_t base_paddr; /* base address of physical space */
        struct ioctl_gntdev_grant_notify notify;
	struct gnttab_map_grant_ref ops[1]; /* variable length */
};

struct privcmd_object_gntalloc {
        vaddr_t	gntva;	/* granted area mapped in kernel */
        uint16_t domid;
        uint16_t flags;
        struct ioctl_gntdev_grant_notify notify;
	uint32_t gref_ids[1]; /* variable length */
};

struct privcmd_object {
	struct uvm_object uobj;
	privcmd_type type;
	int	npages;
	union {
		struct privcmd_object_privcmd pc;
		struct privcmd_object_gntref gr;
		struct privcmd_object_gntalloc ga;
	} u;
};

#define PGO_GNTREF_LEN(count) \
    (sizeof(struct privcmd_object) + \
	sizeof(struct gnttab_map_grant_ref) * ((count) - 1))

#define PGO_GNTA_LEN(count) \
    (sizeof(struct privcmd_object) + \
	sizeof(uint32_t) * ((count) - 1))

int privcmd_nobjects = 0;

static void privpgop_reference(struct uvm_object *);
static void privpgop_detach(struct uvm_object *);
static int privpgop_fault(struct uvm_faultinfo *, vaddr_t , struct vm_page **,
			  int, int, vm_prot_t, int);
static int privcmd_map_obj(struct vm_map *, vaddr_t,
			   struct privcmd_object *, vm_prot_t);


static int
privcmd_xen2bsd_errno(int error)
{
	/*
	 * Xen uses System V error codes.
	 * In order to keep bloat as minimal as possible,
	 * only convert what really impact us.
	 */

	switch(-error) {
	case 0:
		return 0;
	case 1:
		return EPERM;
	case 2:
		return ENOENT;
	case 3:
		return ESRCH;
	case 4:
		return EINTR;
	case 5:
		return EIO;
	case 6:
		return ENXIO;
	case 7:
		return E2BIG;
	case 8:
		return ENOEXEC;
	case 9:
		return EBADF;
	case 10:
		return ECHILD;
	case 11:
		return EAGAIN;
	case 12:
		return ENOMEM;
	case 13:
		return EACCES;
	case 14:
		return EFAULT;
	case 15:
		return ENOTBLK;
	case 16:
		return EBUSY;
	case 17:
		return EEXIST;
	case 18:
		return EXDEV;
	case 19:
		return ENODEV;
	case 20:
		return ENOTDIR;
	case 21:
		return EISDIR;
	case 22:
		return EINVAL;
	case 23:
		return ENFILE;
	case 24:
		return EMFILE;
	case 25:
		return ENOTTY;
	case 26:
		return ETXTBSY;
	case 27:
		return EFBIG;
	case 28:
		return ENOSPC;
	case 29:
		return ESPIPE;
	case 30:
		return EROFS;
	case 31:
		return EMLINK;
	case 32:
		return EPIPE;
	case 33:
		return EDOM;
	case 34:
		return ERANGE;
	case 35:
		return EDEADLK;
	case 36:
		return ENAMETOOLONG;
	case 37:
		return ENOLCK;
	case 38:
		return ENOSYS;
	case 39:
		return ENOTEMPTY;
	case 40:
		return ELOOP;
	case 42:
		return ENOMSG;
	case 43:
		return EIDRM;
	case 60:
		return ENOSTR;
	case 61:
		return ENODATA;
	case 62:
		return ETIME;
	case 63:
		return ENOSR;
	case 66:
		return EREMOTE;
	case 74:
		return EBADMSG;
	case 75:
		return EOVERFLOW;
	case 84:
		return EILSEQ;
	case 87:
		return EUSERS;
	case 88:
		return ENOTSOCK;
	case 89:
		return EDESTADDRREQ;
	case 90:
		return EMSGSIZE;
	case 91:
		return EPROTOTYPE;
	case 92:
		return ENOPROTOOPT;
	case 93:
		return EPROTONOSUPPORT;
	case 94:
		return ESOCKTNOSUPPORT;
	case 95:
		return EOPNOTSUPP;
	case 96:
		return EPFNOSUPPORT;
	case 97:
		return EAFNOSUPPORT;
	case 98:
		return EADDRINUSE;
	case 99:
		return EADDRNOTAVAIL;
	case 100:
		return ENETDOWN;
	case 101:
		return ENETUNREACH;
	case 102:
		return ENETRESET;
	case 103:
		return ECONNABORTED;
	case 104:
		return ECONNRESET;
	case 105:
		return ENOBUFS;
	case 106:
		return EISCONN;
	case 107:
		return ENOTCONN;
	case 108:
		return ESHUTDOWN;
	case 109:
		return ETOOMANYREFS;
	case 110:
		return ETIMEDOUT;
	case 111:
		return ECONNREFUSED;
	case 112:
		return EHOSTDOWN;
	case 113:
		return EHOSTUNREACH;
	case 114:
		return EALREADY;
	case 115:
		return EINPROGRESS;
	case 116:
		return ESTALE;
	case 122:
		return EDQUOT;
	default:
		printf("unknown xen error code %d\n", -error);
		return -error;
	}
}

static vm_prot_t
privcmd_get_map_prot(struct vm_map *map, vaddr_t start, off_t size)
{
	vm_prot_t prot;

	vm_map_lock_read(map);
	/* get protections. This also check for validity of mapping */
	if (uvm_map_checkprot(map, start, start + size - 1, VM_PROT_WRITE))
		prot = VM_PROT_READ | VM_PROT_WRITE;
	else if (uvm_map_checkprot(map, start, start + size - 1, VM_PROT_READ))
		prot = VM_PROT_READ;
	else {
		printf("privcmd_get_map_prot 0x%lx -> 0x%lx "
		    "failed\n",
		    start, (unsigned long)(start + size - 1));
		prot = UVM_PROT_NONE;
	}
	vm_map_unlock_read(map);
	return prot;
}

static int
privcmd_mmap(struct vop_ioctl_args *ap)
{
#ifndef XENPV
	printf("IOCTL_PRIVCMD_MMAP not supported\n");
	return EINVAL;
#else
	int i, j;
	privcmd_mmap_t *mcmd = ap->a_data;
	privcmd_mmap_entry_t mentry;
	vaddr_t va;
	paddr_t ma;
	struct vm_map *vmm = &curlwp->l_proc->p_vmspace->vm_map;
	paddr_t *maddr;
	struct privcmd_object *obj;
	vm_prot_t prot;
	int error;

	for (i = 0; i < mcmd->num; i++) {
		error = copyin(&mcmd->entry[i], &mentry, sizeof(mentry));
		if (error)
			return EINVAL;
		if (mentry.npages == 0)
			return EINVAL;
		if (mentry.va > VM_MAXUSER_ADDRESS)
			return EINVAL;
		va = mentry.va & ~PAGE_MASK;
		prot = privcmd_get_map_prot(vmm, va, mentry.npages * PAGE_SIZE);
		if (prot == UVM_PROT_NONE)
			return EINVAL;
		maddr = kmem_alloc(sizeof(paddr_t) * mentry.npages,
		    KM_SLEEP);
		ma = ((paddr_t)mentry.mfn) <<  PGSHIFT;
		for (j = 0; j < mentry.npages; j++) {
			maddr[j] = ma;
			ma += PAGE_SIZE;
		}
		obj = kmem_alloc(sizeof(*obj), KM_SLEEP);
		obj->type = PTYPE_PRIVCMD;
		obj->u.pc.maddr = maddr;
		obj->u.pc.no_translate = false;
		obj->npages = mentry.npages;
		obj->u.pc.domid = mcmd->dom;
		error  = privcmd_map_obj(vmm, va, obj, prot);
		if (error)
			return error;
	}
	return 0;
#endif
}

static int
privcmd_mmapbatch(struct vop_ioctl_args *ap)
{
#ifndef XENPV
	printf("IOCTL_PRIVCMD_MMAPBATCH not supported\n");
	return EINVAL;
#else
	int i;
	privcmd_mmapbatch_t* pmb = ap->a_data;
	vaddr_t va0;
	u_long mfn;
	paddr_t ma;
	struct vm_map *vmm;
	vaddr_t trymap;
	paddr_t *maddr;
	struct privcmd_object *obj;
	vm_prot_t prot;
	int error;

	vmm = &curlwp->l_proc->p_vmspace->vm_map;
	va0 = pmb->addr & ~PAGE_MASK;

	if (pmb->num == 0)
		return EINVAL;
	if (va0 > VM_MAXUSER_ADDRESS)
		return EINVAL;
	if (((VM_MAXUSER_ADDRESS - va0) >> PGSHIFT) < pmb->num)
		return EINVAL;

	prot = privcmd_get_map_prot(vmm, va0, PAGE_SIZE);
	if (prot == UVM_PROT_NONE)
		return EINVAL;
	
	maddr = kmem_alloc(sizeof(paddr_t) * pmb->num, KM_SLEEP);
	/* get a page of KVA to check mappins */
	trymap = uvm_km_alloc(kernel_map, PAGE_SIZE, PAGE_SIZE,
	    UVM_KMF_VAONLY);
	if (trymap == 0) {
		kmem_free(maddr, sizeof(paddr_t) * pmb->num);
		return ENOMEM;
	}

	obj = kmem_alloc(sizeof(*obj), KM_SLEEP);
	obj->type = PTYPE_PRIVCMD;
	obj->u.pc.maddr = maddr;
	obj->u.pc.no_translate = false;
	obj->npages = pmb->num;
	obj->u.pc.domid = pmb->dom;

	for(i = 0; i < pmb->num; ++i) {
		error = copyin(&pmb->arr[i], &mfn, sizeof(mfn));
		if (error != 0) {
			/* XXX: mappings */
			pmap_update(pmap_kernel());
			kmem_free(maddr, sizeof(paddr_t) * pmb->num);
			uvm_km_free(kernel_map, trymap, PAGE_SIZE,
			    UVM_KMF_VAONLY);
			return error;
		}
		ma = ((paddr_t)mfn) << PGSHIFT;
		if ((error = pmap_enter_ma(pmap_kernel(), trymap, ma, 0,
		    prot, PMAP_CANFAIL | prot, pmb->dom))) {
			mfn |= 0xF0000000;
			copyout(&mfn, &pmb->arr[i], sizeof(mfn));
			maddr[i] = INVALID_PAGE;
		} else {
			pmap_remove(pmap_kernel(), trymap,
			    trymap + PAGE_SIZE);
			maddr[i] = ma;
		}
	}
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, trymap, PAGE_SIZE, UVM_KMF_VAONLY);

	error = privcmd_map_obj(vmm, va0, obj, prot);

	return error;
#endif
}

static int
privcmd_mmapbatch_v2(struct vop_ioctl_args *ap)
{
	int i;
	privcmd_mmapbatch_v2_t* pmb = ap->a_data;
	vaddr_t va0;
	u_long mfn;
	struct vm_map *vmm;
	paddr_t *maddr;
	struct privcmd_object *obj;
	vm_prot_t prot;
	int error;
	paddr_t base_paddr = 0;

	vmm = &curlwp->l_proc->p_vmspace->vm_map;
	va0 = pmb->addr & ~PAGE_MASK;

	if (pmb->num == 0)
		return EINVAL;
	if (va0 > VM_MAXUSER_ADDRESS)
		return EINVAL;
	if (((VM_MAXUSER_ADDRESS - va0) >> PGSHIFT) < pmb->num)
		return EINVAL;

	prot = privcmd_get_map_prot(vmm, va0, PAGE_SIZE);
	if (prot == UVM_PROT_NONE)
		return EINVAL;
	
#ifndef XENPV
	KASSERT(xen_feature(XENFEAT_auto_translated_physmap));
	base_paddr = xenmem_alloc_pa(pmb->num * PAGE_SIZE, PAGE_SIZE, true);
	KASSERT(base_paddr != 0);
#endif
	maddr = kmem_alloc(sizeof(paddr_t) * pmb->num, KM_SLEEP);
	obj = kmem_alloc(sizeof(*obj), KM_SLEEP);
	obj->type = PTYPE_PRIVCMD_PHYSMAP;
	obj->u.pc.maddr = maddr;
	obj->u.pc.base_paddr = base_paddr;
	obj->u.pc.no_translate = false;
	obj->npages = pmb->num;
	obj->u.pc.domid = pmb->dom;

	for(i = 0; i < pmb->num; ++i) {
		error = copyin(&pmb->arr[i], &mfn, sizeof(mfn));
		if (error != 0) {
			kmem_free(maddr, sizeof(paddr_t) * pmb->num);
			kmem_free(obj, sizeof(*obj));
#ifndef XENPV
			xenmem_free_pa(base_paddr, pmb->num * PAGE_SIZE);
#endif
			return error;
		}
#ifdef XENPV
		maddr[i] = ((paddr_t)mfn) << PGSHIFT;
#else
		maddr[i] = mfn; /* TMP argument for XENMEM_add_to_physmap */
#endif

	}
	error = privcmd_map_obj(vmm, va0, obj, prot);
	if (error)
		return error;

	/*
	 * map the range in user process now.
	 * If Xenr return -ENOENT, retry (paging in progress)
	 */
	for(i = 0; i < pmb->num; i++, va0 += PAGE_SIZE) {
		int err, cerr;
#ifdef XENPV
		for (int j = 0 ; j < 10; j++) {
			err = pmap_enter_ma(vmm->pmap, va0, maddr[i], 0, 
			    prot, PMAP_CANFAIL | prot,
			    pmb->dom);
			if (err != -2) /* Xen ENOENT */
				break;
			if (kpause("xnoent", 1, mstohz(100), NULL))
				break;
		}
		if (err) {
			maddr[i] = INVALID_PAGE;
		}
#else /* XENPV */
		xen_add_to_physmap_batch_t add;
		u_long idx;
		xen_pfn_t gpfn;
		int err2;
		memset(&add, 0, sizeof(add));

		add.domid = DOMID_SELF;
		add.space = XENMAPSPACE_gmfn_foreign;
		add.size = 1;
		add.foreign_domid = pmb->dom;
		idx = maddr[i];
		set_xen_guest_handle(add.idxs, &idx);
		maddr[i] = INVALID_PAGE;
		gpfn = (base_paddr >> PGSHIFT) + i;
		set_xen_guest_handle(add.gpfns, &gpfn);
		err2 = 0;
		set_xen_guest_handle(add.errs, &err2);
		err = HYPERVISOR_memory_op(XENMEM_add_to_physmap_batch, &add);
		if (err < 0) {
			printf("privcmd_mmapbatch_v2: XENMEM_add_to_physmap_batch failed %d\n", err);
			privpgop_detach(&obj->uobj);
			return privcmd_xen2bsd_errno(err);
		}
		err = err2;
		if (err == 0) 
			maddr[i] = base_paddr + i * PAGE_SIZE;
#endif /* XENPV */

		cerr = copyout(&err, &pmb->err[i], sizeof(pmb->err[i]));
		if (cerr) {
			privpgop_detach(&obj->uobj);
			return cerr;
		}
	}
	return 0;
}

static int
privcmd_mmap_resource(struct vop_ioctl_args *ap)
{
	int i;
	privcmd_mmap_resource_t* pmr = ap->a_data;
	vaddr_t va0;
	struct vm_map *vmm;
	struct privcmd_object *obj;
	vm_prot_t prot;
	int error;
	struct xen_mem_acquire_resource op;
	xen_pfn_t *pfns;
	paddr_t *maddr;
	paddr_t base_paddr = 0;

	vmm = &curlwp->l_proc->p_vmspace->vm_map;
	va0 = pmr->addr & ~PAGE_MASK;

	if (pmr->num == 0)
		return EINVAL;
	if (va0 > VM_MAXUSER_ADDRESS)
		return EINVAL;
	if (((VM_MAXUSER_ADDRESS - va0) >> PGSHIFT) < pmr->num)
		return EINVAL;

	prot = privcmd_get_map_prot(vmm, va0, PAGE_SIZE);
	if (prot == UVM_PROT_NONE)
		return EINVAL;
	
	pfns = kmem_alloc(sizeof(xen_pfn_t) * pmr->num, KM_SLEEP);
#ifndef XENPV
	KASSERT(xen_feature(XENFEAT_auto_translated_physmap));
	base_paddr = xenmem_alloc_pa(pmr->num * PAGE_SIZE, PAGE_SIZE, true);
	KASSERT(base_paddr != 0);
	for (i = 0; i < pmr->num; i++) {
		pfns[i] = (base_paddr >> PGSHIFT) + i;
	}
#else
	KASSERT(!xen_feature(XENFEAT_auto_translated_physmap));
#endif
	
	memset(&op, 0, sizeof(op));
	op.domid = pmr->dom;
	op.type = pmr->type;
	op.id = pmr->id;
	op.frame = pmr->idx;
	op.nr_frames = pmr->num;
	set_xen_guest_handle(op.frame_list, pfns);

	error = HYPERVISOR_memory_op(XENMEM_acquire_resource, &op);
	if (error) {
		printf("%s: XENMEM_acquire_resource failed: %d\n",
		    __func__, error);
		return privcmd_xen2bsd_errno(error);
	}
	maddr = kmem_alloc(sizeof(paddr_t) * pmr->num, KM_SLEEP);
	for (i = 0; i < pmr->num; i++) {
		maddr[i] = pfns[i] << PGSHIFT;
	}
	kmem_free(pfns, sizeof(xen_pfn_t) * pmr->num);

	obj = kmem_alloc(sizeof(*obj), KM_SLEEP);
	obj->type = PTYPE_PRIVCMD_PHYSMAP;
	obj->u.pc.base_paddr = base_paddr;
	obj->u.pc.maddr = maddr;
	obj->u.pc.no_translate = true;
	obj->npages = pmr->num;
	obj->u.pc.domid = (op.flags & XENMEM_rsrc_acq_caller_owned) ?
	    DOMID_SELF : pmr->dom;

	error = privcmd_map_obj(vmm, va0, obj, prot);
	return error;
}

static int
privcmd_map_gref(struct vop_ioctl_args *ap)
{
	struct ioctl_gntdev_mmap_grant_ref *mgr = ap->a_data;
	struct vm_map *vmm = &curlwp->l_proc->p_vmspace->vm_map;
	struct privcmd_object *obj;
	vaddr_t va0 = (vaddr_t)mgr->va & ~PAGE_MASK;
	vm_prot_t prot;
	int error;

	if (mgr->count == 0)
		return EINVAL;
	if (va0 > VM_MAXUSER_ADDRESS)
		return EINVAL;
	if (((VM_MAXUSER_ADDRESS - va0) >> PGSHIFT) < mgr->count)
		return EINVAL;
	if (mgr->notify.offset < 0 || mgr->notify.offset > mgr->count)
		return EINVAL;

	prot = privcmd_get_map_prot(vmm, va0, PAGE_SIZE);
	if (prot == UVM_PROT_NONE)
		return EINVAL;
	
	obj = kmem_alloc(PGO_GNTREF_LEN(mgr->count), KM_SLEEP);

	obj->type  = PTYPE_GNTDEV_REF;
	obj->npages = mgr->count;
	memcpy(&obj->u.gr.notify, &mgr->notify,
	    sizeof(obj->u.gr.notify));
#ifndef XENPV
	KASSERT(xen_feature(XENFEAT_auto_translated_physmap));
	obj->u.gr.base_paddr = xenmem_alloc_pa(obj->npages * PAGE_SIZE,
	    PAGE_SIZE, true);
	KASSERT(obj->u.gr.base_paddr != 0);
#else
	obj->u.gr.base_paddr = 0;
#endif /* !XENPV */

	for (int i = 0; i < obj->npages; ++i) {
		struct ioctl_gntdev_grant_ref gref;
		error = copyin(&mgr->refs[i], &gref, sizeof(gref));
		if (error != 0) {
			goto err1;
		}
#ifdef XENPV
		obj->u.gr.ops[i].host_addr = 0;
		obj->u.gr.ops[i].flags = GNTMAP_host_map |
		    GNTMAP_application_map | GNTMAP_contains_pte;
#else /* XENPV */
		obj->u.gr.ops[i].host_addr = 
		    obj->u.gr.base_paddr + PAGE_SIZE * i;
		obj->u.gr.ops[i].flags = GNTMAP_host_map;
#endif /* XENPV */
		obj->u.gr.ops[i].dev_bus_addr = 0;
		obj->u.gr.ops[i].ref = gref.ref;
		obj->u.gr.ops[i].dom = gref.domid;
		obj->u.gr.ops[i].handle = -1;
		if (prot == UVM_PROT_READ)
			obj->u.gr.ops[i].flags |= GNTMAP_readonly;
	}
	error = privcmd_map_obj(vmm, va0, obj, prot);
	return error;
err1:
#ifndef XENPV
	xenmem_free_pa(obj->u.gr.base_paddr, obj->npages * PAGE_SIZE);
#endif
	kmem_free(obj, PGO_GNTREF_LEN(obj->npages));
	return error;
}

static int
privcmd_alloc_gref(struct vop_ioctl_args *ap)
{
	struct ioctl_gntdev_alloc_grant_ref *mga = ap->a_data;
	struct vm_map *vmm = &curlwp->l_proc->p_vmspace->vm_map;
	struct privcmd_object *obj;
	vaddr_t va0 = (vaddr_t)mga->va & ~PAGE_MASK;
	vm_prot_t prot;
	int error, ret;

	if (mga->count == 0)
		return EINVAL;
	if (va0 > VM_MAXUSER_ADDRESS)
		return EINVAL;
	if (((VM_MAXUSER_ADDRESS - va0) >> PGSHIFT) < mga->count)
		return EINVAL;
	if (mga->notify.offset < 0 || mga->notify.offset > mga->count)
		return EINVAL;

	prot = privcmd_get_map_prot(vmm, va0, PAGE_SIZE);
	if (prot == UVM_PROT_NONE)
		return EINVAL;
	
	obj = kmem_alloc(PGO_GNTA_LEN(mga->count), KM_SLEEP);

	obj->type  = PTYPE_GNTDEV_ALLOC;
	obj->npages = mga->count;
	obj->u.ga.domid = mga->domid;
	memcpy(&obj->u.ga.notify, &mga->notify,
	    sizeof(obj->u.ga.notify));
	obj->u.ga.gntva = uvm_km_alloc(kernel_map,
	    PAGE_SIZE * obj->npages, PAGE_SIZE, UVM_KMF_WIRED | UVM_KMF_ZERO);
	if (obj->u.ga.gntva == 0) {
		error = ENOMEM;
		goto err1;
	}

	for (int i = 0; i < obj->npages; ++i) {
		paddr_t ma;
		vaddr_t va = obj->u.ga.gntva + i * PAGE_SIZE;
		grant_ref_t id;
		bool ro = ((mga->flags & GNTDEV_ALLOC_FLAG_WRITABLE) == 0);
		(void)pmap_extract_ma(pmap_kernel(), va, &ma);
		if ((ret = xengnt_grant_access(mga->domid, ma, ro, &id)) != 0) {
			printf("%s: xengnt_grant_access failed: %d\n",
			    __func__, ret);
			for (int j = 0; j < i; j++) {
				xengnt_revoke_access(obj->u.ga.gref_ids[j]);
				error = ret;
				goto err2;
			}
		}
		obj->u.ga.gref_ids[i] = id;
	}

	error = copyout(&obj->u.ga.gref_ids[0], mga->gref_ids,
	    sizeof(uint32_t) * obj->npages);
	if (error) {
		for (int i = 0; i < obj->npages; ++i) {
			xengnt_revoke_access(obj->u.ga.gref_ids[i]);
		}
		goto err2;
	}

	error = privcmd_map_obj(vmm, va0, obj, prot);
	return error;

err2:
	uvm_km_free(kernel_map, obj->u.ga.gntva,
	    PAGE_SIZE * obj->npages, UVM_KMF_WIRED);
err1:
	kmem_free(obj, PGO_GNTA_LEN(obj->npages));
	return error;
}

static int
privcmd_ioctl(void *v)
{
	struct vop_ioctl_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		u_long a_command;
		void *a_data;
		int a_fflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	int error = 0;

	switch (ap->a_command) {
	case IOCTL_PRIVCMD_HYPERCALL:
	case IOCTL_PRIVCMD_HYPERCALL_OLD:
	/*
	 * oprivcmd_hypercall_t is privcmd_hypercall_t without the last entry
	 */
	{
		privcmd_hypercall_t *hc = ap->a_data;
		if (hc->op >= (PAGE_SIZE >> 5))
			return EINVAL;
		error = -EOPNOTSUPP;
#if defined(__i386__)
		__asm volatile (
			"pushl %%ebx; pushl %%ecx; pushl %%edx;"
			"pushl %%esi; pushl %%edi; "
			"movl  4(%%eax),%%ebx ;"
			"movl  8(%%eax),%%ecx ;"
			"movl 12(%%eax),%%edx ;"
			"movl 16(%%eax),%%esi ;"
			"movl 20(%%eax),%%edi ;"
			"movl   (%%eax),%%eax ;"
			"shll $5,%%eax ;"
			"addl $hypercall_page,%%eax ;"
			"call *%%eax ;"
			"popl %%edi; popl %%esi; popl %%edx;"
			"popl %%ecx; popl %%ebx"
			: "=a" (error) : "0" (ap->a_data) : "memory" );
#endif /* __i386__ */
#if defined(__x86_64__)
#ifndef XENPV
		/* hypervisor can't access user memory if SMAP is enabled */
		smap_disable();
#endif
		{
		long i1, i2, i3;
		__asm volatile (
			"movq %8,%%r10; movq %9,%%r8;"
			"shll $5,%%eax ;"
			"addq $hypercall_page,%%rax ;"
			"call *%%rax"
			: "=a" (error), "=D" (i1),
			  "=S" (i2), "=d" (i3)
			: "0" ((unsigned int)hc->op),
			  "1" (hc->arg[0]),
			  "2" (hc->arg[1]),
			  "3" (hc->arg[2]),
			  "g" (hc->arg[3]),
			  "g" (hc->arg[4])
			: "r8", "r10", "memory" );
		}
#ifndef XENPV
		smap_enable();
#endif
#endif /* __x86_64__ */
		if (ap->a_command == IOCTL_PRIVCMD_HYPERCALL) {
			if (error >= 0) {
				hc->retval = error;
				error = 0;
			} else {
				/* error occurred, return the errno */
				error = privcmd_xen2bsd_errno(error);
				hc->retval = 0;
			}
		} else {
			error = privcmd_xen2bsd_errno(error);
		}
		break;
	}
	case IOCTL_PRIVCMD_MMAP:
		return privcmd_mmap(ap);

	case IOCTL_PRIVCMD_MMAPBATCH:
		return privcmd_mmapbatch(ap);

	case IOCTL_PRIVCMD_MMAPBATCH_V2:
		return privcmd_mmapbatch_v2(ap);

	case IOCTL_PRIVCMD_MMAP_RESOURCE:
		return privcmd_mmap_resource(ap);

	case IOCTL_GNTDEV_MMAP_GRANT_REF:
		return privcmd_map_gref(ap);

	case IOCTL_GNTDEV_ALLOC_GRANT_REF:
		return privcmd_alloc_gref(ap);
	default:
		error = EINVAL;
	}
	
	return error;
}

static const struct uvm_pagerops privpgops = {
  .pgo_reference = privpgop_reference,
  .pgo_detach = privpgop_detach,
  .pgo_fault = privpgop_fault,
};

static void
privpgop_reference(struct uvm_object *uobj)
{
	rw_enter(uobj->vmobjlock, RW_WRITER);
	uobj->uo_refs++;
	rw_exit(uobj->vmobjlock);
}

static void
privcmd_notify(struct ioctl_gntdev_grant_notify *notify, vaddr_t va,
    struct gnttab_map_grant_ref *gmops)
{
	if (notify->action & UNMAP_NOTIFY_SEND_EVENT) {
		hypervisor_notify_via_evtchn(notify->event_channel_port);
	}
	if ((notify->action & UNMAP_NOTIFY_CLEAR_BYTE) == 0) {
		notify->action = 0;
		return;
	}
	if (va == 0) {
		struct gnttab_map_grant_ref op;
		struct gnttab_unmap_grant_ref uop;
		int i = notify->offset / PAGE_SIZE;
		int o = notify->offset % PAGE_SIZE;
		int err;
#ifndef XENPV
		paddr_t base_paddr;
		base_paddr = xenmem_alloc_pa(PAGE_SIZE, PAGE_SIZE, true);
#endif

		KASSERT(gmops != NULL);
		va = uvm_km_alloc(kernel_map, PAGE_SIZE, PAGE_SIZE,
		    UVM_KMF_VAONLY | UVM_KMF_WAITVA);
#ifndef XENPV
		op.host_addr = base_paddr;
#else
		op.host_addr = va;
#endif
		op.dev_bus_addr = 0;
		op.ref = gmops[i].ref;
		op.dom = gmops[i].dom;
		op.handle = -1;
		op.flags = GNTMAP_host_map;
		err = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &op, 1);
		if (err == 0 && op.status == GNTST_okay) {
#ifndef XENPV
			pmap_kenter_pa(va, base_paddr,
			    VM_PROT_READ | VM_PROT_WRITE, 0);
#endif
			char *n = (void *)(va + o);
			*n = 0;
#ifndef XENPV
			pmap_kremove(va, PAGE_SIZE);
			uop.host_addr = base_paddr;
#else
			uop.host_addr = va;
#endif
			uop.handle = op.handle;
			uop.dev_bus_addr = 0;
			(void)HYPERVISOR_grant_table_op(
			    GNTTABOP_unmap_grant_ref, &uop, 1);
		}
		uvm_km_free(kernel_map, va, PAGE_SIZE, UVM_KMF_VAONLY);
#ifndef XENPV
		xenmem_free_pa(base_paddr, PAGE_SIZE);
#endif
	} else {
		KASSERT(gmops == NULL);
		char *n = (void *)(va + notify->offset);
		*n = 0;
	}
	notify->action = 0;
}

static void
privpgop_detach(struct uvm_object *uobj)
{
	struct privcmd_object *pobj = (struct privcmd_object *)uobj;

	rw_enter(uobj->vmobjlock, RW_WRITER);
	KASSERT(uobj->uo_refs > 0);
	if (uobj->uo_refs > 1) {
		uobj->uo_refs--;
		rw_exit(uobj->vmobjlock);
		return;
	}
	rw_exit(uobj->vmobjlock);
	switch (pobj->type) {
	case PTYPE_PRIVCMD_PHYSMAP:
#ifndef XENPV
		for (int i = 0; i < pobj->npages; i++) {
			if (pobj->u.pc.maddr[i] != INVALID_PAGE) {
				struct xen_remove_from_physmap rm;
				rm.domid = DOMID_SELF;
				rm.gpfn = pobj->u.pc.maddr[i] >> PGSHIFT;
				HYPERVISOR_memory_op(
				    XENMEM_remove_from_physmap, &rm);
			}
		}
		xenmem_free_pa(pobj->u.pc.base_paddr, pobj->npages * PAGE_SIZE);
#endif
		/* FALLTHROUGH */
	case PTYPE_PRIVCMD:
		kmem_free(pobj->u.pc.maddr, sizeof(paddr_t) * pobj->npages);
		uvm_obj_destroy(uobj, true);
		kmem_free(pobj, sizeof(struct privcmd_object));
		break;
	case PTYPE_GNTDEV_REF:
	{
		privcmd_notify(&pobj->u.gr.notify, 0, pobj->u.gr.ops);
#ifndef XENPV
		KASSERT(pobj->u.gr.base_paddr != 0);
		for (int i = 0; i < pobj->npages; i++) {
			struct xen_remove_from_physmap rm;
			rm.domid = DOMID_SELF;
			rm.gpfn = (pobj->u.gr.base_paddr << PGSHIFT) + i;
			HYPERVISOR_memory_op(XENMEM_remove_from_physmap, &rm);
		}
		xenmem_free_pa(pobj->u.gr.base_paddr, pobj->npages * PAGE_SIZE);
#endif
		kmem_free(pobj, PGO_GNTREF_LEN(pobj->npages));
		break;
	}
	case PTYPE_GNTDEV_ALLOC:
		privcmd_notify(&pobj->u.ga.notify, pobj->u.ga.gntva, NULL);
		for (int i = 0; i < pobj->npages; ++i) {
			xengnt_revoke_access(pobj->u.ga.gref_ids[i]);
		}
		uvm_km_free(kernel_map, pobj->u.ga.gntva,
		    PAGE_SIZE * pobj->npages, UVM_KMF_WIRED);
		kmem_free(pobj, PGO_GNTA_LEN(pobj->npages));
	}
	privcmd_nobjects--;
}

static int
privpgop_fault(struct uvm_faultinfo *ufi, vaddr_t vaddr, struct vm_page **pps,
    int npages, int centeridx, vm_prot_t access_type, int flags)
{
	struct vm_map_entry *entry = ufi->entry;
	struct uvm_object *uobj = entry->object.uvm_obj;
	struct privcmd_object *pobj = (struct privcmd_object*)uobj;
	int maddr_i, i, error = 0;

	/* compute offset from start of map */
	maddr_i = (entry->offset + (vaddr - entry->start)) >> PAGE_SHIFT;
	if (maddr_i + npages > pobj->npages) {
		return EINVAL;
	}
	for (i = 0; i < npages; i++, maddr_i++, vaddr+= PAGE_SIZE) {
		if ((flags & PGO_ALLPAGES) == 0 && i != centeridx)
			continue;
		if (pps[i] == PGO_DONTCARE)
			continue;
		switch(pobj->type) {
		case PTYPE_PRIVCMD:
		case PTYPE_PRIVCMD_PHYSMAP:
		{
			u_int pm_flags = PMAP_CANFAIL | ufi->entry->protection;
#ifdef XENPV
			if (pobj->u.pc.no_translate)
				pm_flags |= PMAP_MD_XEN_NOTR;
#endif
			if (pobj->u.pc.maddr[maddr_i] == INVALID_PAGE) {
				/* This has already been flagged as error. */
				error = EFAULT;
				goto out;
			}
			error = pmap_enter_ma(ufi->orig_map->pmap, vaddr,
			    pobj->u.pc.maddr[maddr_i], 0,
			    ufi->entry->protection, pm_flags,
			    pobj->u.pc.domid);
			if (error == ENOMEM) {
				goto out;
			}
			if (error) {
				pobj->u.pc.maddr[maddr_i] = INVALID_PAGE;
				error = EFAULT;
			}
			break;
		}
		case PTYPE_GNTDEV_REF:
		{
			struct pmap *pmap = ufi->orig_map->pmap;
			if (pmap_enter_gnt(pmap, vaddr, entry->start, pobj->npages, &pobj->u.gr.ops[0]) != GNTST_okay) {
				error = EFAULT;
				goto out;
			}
			break;
		}
		case PTYPE_GNTDEV_ALLOC:
		{
			paddr_t pa;
			if (!pmap_extract(pmap_kernel(),
			    pobj->u.ga.gntva + maddr_i * PAGE_SIZE, &pa)) {
				error = EFAULT;
				goto out;
			}
			error = pmap_enter(ufi->orig_map->pmap, vaddr, pa,
			    ufi->entry->protection,
			    PMAP_CANFAIL | ufi->entry->protection);
			if (error == ENOMEM) {
				goto out;
			}
			break;
		}
		}
		if (error) {
			/* XXX for proper ptp accountings */
			pmap_remove(ufi->orig_map->pmap, vaddr,
			    vaddr + PAGE_SIZE);
		}
	}
out:
	pmap_update(ufi->orig_map->pmap);
	uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, uobj);
	return error;
}

static int
privcmd_map_obj(struct vm_map *map, vaddr_t start, struct privcmd_object *obj,
    vm_prot_t prot)
{
	int error;
	uvm_flag_t uvmflag;
	vaddr_t newstart = start;
	off_t size = ((off_t)obj->npages << PGSHIFT);

	privcmd_nobjects++;
	uvm_obj_init(&obj->uobj, &privpgops, true, 1);
	uvmflag = UVM_MAPFLAG(prot, prot, UVM_INH_NONE, UVM_ADV_NORMAL,
	    UVM_FLAG_FIXED | UVM_FLAG_UNMAP | UVM_FLAG_NOMERGE);
	error = uvm_map(map, &newstart, size, &obj->uobj, 0, 0, uvmflag);

	if (error)
		obj->uobj.pgops->pgo_detach(&obj->uobj);
	return error;
}

static const struct kernfs_fileop privcmd_fileops[] = {
  { .kf_fileop = KERNFS_FILEOP_IOCTL, .kf_vop = privcmd_ioctl },
};

void
xenprivcmd_init(void)
{
	kernfs_entry_t *dkt;
	kfstype kfst;

	if (!xendomain_is_privileged())
		return;

	kfst = KERNFS_ALLOCTYPE(privcmd_fileops);

	KERNFS_ALLOCENTRY(dkt, KM_SLEEP);
	KERNFS_INITENTRY(dkt, DT_REG, "privcmd", NULL, kfst, VREG,
	    PRIVCMD_MODE);
	kernfs_addentry(kernxen_pkt, dkt);
}
