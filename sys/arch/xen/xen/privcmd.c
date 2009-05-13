/* $NetBSD: privcmd.c,v 1.35.4.1 2009/05/13 17:18:50 jym Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
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
__KERNEL_RCSID(0, "$NetBSD: privcmd.c,v 1.35.4.1 2009/05/13 17:18:50 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/proc.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/kernfs/kernfs.h>

#include <uvm/uvm.h>
#include <uvm/uvm_fault.h>
#include <uvm/uvm_fault_i.h>

#include <xen/kernfs_machdep.h>
#include <xen/xenio.h>

#define	PRIVCMD_MODE	(S_IRUSR)

/* Magic value is used to mark invalid pages.
 * This must be a value within the page-offset.
 * Page-aligned values including 0x0 are used by the guest.
 */ 
#define INVALID_PAGE	0xfff

struct privcmd_object {
	struct uvm_object uobj;
	paddr_t *maddr; /* array of machine address to map */
	int	npages;
	int	domid;
};

int privcmd_nobjects = 0;

static void privpgop_reference(struct uvm_object *);
static void privpgop_detach(struct uvm_object *);
static int privpgop_fault(struct uvm_faultinfo *, vaddr_t , struct vm_page **,
			 int, int, vm_prot_t, int);
static int privcmd_map_obj(struct vm_map *, vaddr_t, paddr_t *, int, int);


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
	paddr_t *maddr;

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
#if defined(XEN3) && !defined(XEN_COMPAT_030001)
			"shll $5,%%eax ;"
			"addl $hypercall_page,%%eax ;"
			"call *%%eax ;"
#else
			TRAP_INSTR "; "
#endif
			"popl %%edi; popl %%esi; popl %%edx;"
			"popl %%ecx; popl %%ebx"
			: "=a" (error) : "0" (ap->a_data) : "memory" );
#endif /* __i386__ */
#if defined(__x86_64__)
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
#endif /* __x86_64__ */
		if (ap->a_command == IOCTL_PRIVCMD_HYPERCALL) {
			if (error >= 0) {
				hc->retval = error;
				error = 0;
			} else {
				/* error occured, return the errno */
				error = privcmd_xen2bsd_errno(error);
				hc->retval = 0;
			}
		} else {
			error = privcmd_xen2bsd_errno(error);
		}
		break;
	}
#ifndef XEN3
	case IOCTL_PRIVCMD_INITDOMAIN_EVTCHN_OLD:
		{
		extern int initdom_ctrlif_domcontroller_port;
		error = initdom_ctrlif_domcontroller_port;
		}
		break;
	case IOCTL_PRIVCMD_INITDOMAIN_EVTCHN:
		{
		extern int initdom_ctrlif_domcontroller_port;
		*(int *)ap->a_data = initdom_ctrlif_domcontroller_port;
		}
		error = 0;
		break;
#endif /* XEN3 */
	case IOCTL_PRIVCMD_MMAP:
	{
		int i, j;
		privcmd_mmap_t *mcmd = ap->a_data;
		privcmd_mmap_entry_t mentry;
		vaddr_t va;
		u_long ma;
		struct vm_map *vmm = &curlwp->l_proc->p_vmspace->vm_map;

		for (i = 0; i < mcmd->num; i++) {
			error = copyin(&mcmd->entry[i], &mentry, sizeof(mentry));
			if (error)
				return error;
			if (mentry.npages == 0)
				return EINVAL;
			if (mentry.va > VM_MAXUSER_ADDRESS)
				return EINVAL;
#if 0
			if (mentry.va + (mentry.npages << PGSHIFT) >
			    mrentry->vm_end)
				return EINVAL;
#endif
			maddr = kmem_alloc(sizeof(paddr_t) * mentry.npages,
			    KM_SLEEP);
			if (maddr == NULL)
				return ENOMEM;
			va = mentry.va & ~PAGE_MASK;
			ma = mentry.mfn <<  PGSHIFT; /* XXX ??? */
			for (j = 0; j < mentry.npages; j++) {
				maddr[j] = ma;
				ma += PAGE_SIZE;
			}
			error  = privcmd_map_obj(vmm, va, maddr,
			    mentry.npages, mcmd->dom);
			if (error)
				return error;
		}
		break;
	}
	case IOCTL_PRIVCMD_MMAPBATCH:
	{
		int i;
		privcmd_mmapbatch_t* pmb = ap->a_data;
		vaddr_t va0, va;
		u_long mfn, ma;
		struct vm_map *vmm;
		struct vm_map_entry *entry;
		vm_prot_t prot;
		pmap_t pmap;
		vaddr_t trymap;

		vmm = &curlwp->l_proc->p_vmspace->vm_map;
		pmap = vm_map_pmap(vmm);
		va0 = pmb->addr & ~PAGE_MASK;

		if (pmb->num == 0)
			return EINVAL;
		if (va0 > VM_MAXUSER_ADDRESS)
			return EINVAL;
		if (((VM_MAXUSER_ADDRESS - va0) >> PGSHIFT) < pmb->num)
			return EINVAL;

		vm_map_lock_read(vmm);
		if (!uvm_map_lookup_entry(vmm, va0, &entry)) {
			vm_map_unlock_read(vmm);
			return EINVAL;
		}
		prot = entry->protection;
		vm_map_unlock_read(vmm);
		
		maddr = kmem_alloc(sizeof(paddr_t) * pmb->num, KM_SLEEP);
		if (maddr == NULL)
			return ENOMEM;
		/* get a page of KVA to check mappins */
		trymap = uvm_km_alloc(kernel_map, PAGE_SIZE, PAGE_SIZE,
		    UVM_KMF_VAONLY);
		if (trymap == 0) {
			kmem_free(maddr, sizeof(paddr_t) * pmb->num);
			return ENOMEM;
		}

		for(i = 0; i < pmb->num; ++i) {
			va = va0 + (i * PAGE_SIZE);
			error = copyin(&pmb->arr[i], &mfn, sizeof(mfn));
			if (error != 0) {
				kmem_free(maddr, sizeof(paddr_t) * pmb->num);
				uvm_km_free(kernel_map, trymap, PAGE_SIZE,
				    UVM_KMF_VAONLY);
				return error;
			}
			ma = mfn << PGSHIFT;
			if (pmap_enter_ma(pmap_kernel(), trymap, ma, 0,
			    prot, PMAP_CANFAIL, pmb->dom)) {
				mfn |= 0xF0000000;
				copyout(&mfn, &pmb->arr[i], sizeof(mfn));
				maddr[i] = INVALID_PAGE;
			} else {
				pmap_remove(pmap_kernel(), trymap,
				    trymap + PAGE_SIZE);
				maddr[i] = ma;
			}
		}
		error = privcmd_map_obj(vmm, va0, maddr, pmb->num, pmb->dom);
		uvm_km_free(kernel_map, trymap, PAGE_SIZE, UVM_KMF_VAONLY);

		if (error != 0)
			return error;

		break;
	}
#ifndef XEN3
	case IOCTL_PRIVCMD_GET_MACH2PHYS_START_MFN:
		{
		unsigned long *mfn_start = ap->a_data;
		*mfn_start = HYPERVISOR_shared_info->arch.mfn_to_pfn_start;
		error = 0;
		}
		break;
#endif /* !XEN3 */
	default:
		error = EINVAL;
	}
	
	return error;
}

static struct uvm_pagerops privpgops = {
  .pgo_reference = privpgop_reference,
  .pgo_detach = privpgop_detach,
  .pgo_fault = privpgop_fault,
};

static void
privpgop_reference(struct uvm_object *uobj)
{
	mutex_enter(&uobj->vmobjlock);
	uobj->uo_refs++;
	mutex_exit(&uobj->vmobjlock);
}

static void
privpgop_detach(struct uvm_object *uobj)
{
	struct privcmd_object *pobj = (struct privcmd_object *)uobj;
	mutex_enter(&uobj->vmobjlock);
	if (uobj->uo_refs > 1) {
		uobj->uo_refs--;
		mutex_exit(&uobj->vmobjlock);
		return;
	}
	mutex_exit(&uobj->vmobjlock);
	kmem_free(pobj->maddr, sizeof(paddr_t) * pobj->npages);
	UVM_OBJ_DESTROY(uobj);
	kmem_free(pobj, sizeof(struct privcmd_object));
	privcmd_nobjects--;
}

static int
privpgop_fault(struct uvm_faultinfo *ufi, vaddr_t vaddr, struct vm_page **pps,
    int npages, int centeridx, vm_prot_t access_type, int flags)
{
	struct vm_map_entry *entry = ufi->entry;
	struct uvm_object *uobj = entry->object.uvm_obj;
	struct privcmd_object *pobj = (struct privcmd_object*)uobj;
	int maddr_i;
	int i, error = 0;

	/* compute offset from start of map */
	maddr_i = (entry->offset + (vaddr - entry->start)) >> PAGE_SHIFT;
	if (maddr_i + npages > pobj->npages)
		return EINVAL;
	for (i = 0; i < npages; i++, maddr_i++, vaddr+= PAGE_SIZE) {
		if ((flags & PGO_ALLPAGES) == 0 && i != centeridx)
			continue;
		if (pps[i] == PGO_DONTCARE)
			continue;
		if (pobj->maddr[maddr_i] == INVALID_PAGE) {
			/* this has already been flagged as error */
			uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap,
			    uobj, NULL);
			pmap_update(ufi->orig_map->pmap);
			return EFAULT;
		}
		error = pmap_enter_ma(ufi->orig_map->pmap, vaddr,
		    pobj->maddr[maddr_i], 0, ufi->entry->protection,
		    PMAP_CANFAIL | ufi->entry->protection,
		    pobj->domid);
		if (error == ENOMEM) {
			uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap,
			    uobj, NULL);
			pmap_update(ufi->orig_map->pmap);
			uvm_wait("udv_fault");
			return (ERESTART);
		}
		if (error) {
			/* XXX for proper ptp accountings */
			pmap_remove(ufi->orig_map->pmap, vaddr, 
			    vaddr + PAGE_SIZE);
		}
	}
	uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, uobj, NULL);
	pmap_update(ufi->orig_map->pmap);
	return (error);
}

static int
privcmd_map_obj(struct vm_map *map, vaddr_t start, paddr_t *maddr,
		int npages, int domid)
{
	struct privcmd_object *obj;
	int error;
	uvm_flag_t uvmflag;
	vaddr_t newstart = start;
	vm_prot_t prot;
	off_t size = ((off_t)npages << PGSHIFT);

	vm_map_lock_read(map);
	/* get protections. This also check for validity of mapping */
	if (uvm_map_checkprot(map, start, start + size - 1, VM_PROT_WRITE))
		prot = VM_PROT_READ | VM_PROT_WRITE;
	else if (uvm_map_checkprot(map, start, start + size - 1, VM_PROT_READ))
		prot = VM_PROT_READ;
	else {
		printf("uvm_map_checkprot 0x%lx -> 0x%lx "
		    "failed\n",
		    start, (unsigned long)(start + size - 1));
		vm_map_unlock_read(map);
		kmem_free(maddr, sizeof(paddr_t) * npages);
		return EINVAL;
	}
	vm_map_unlock_read(map);
	/* remove current entries */
	uvm_unmap1(map, start, start + size, 0);

	obj = kmem_alloc(sizeof(struct privcmd_object), KM_SLEEP);
	if (obj == NULL) {
		kmem_free(maddr, sizeof(paddr_t) * npages);
		return ENOMEM;
	}

	privcmd_nobjects++;
	UVM_OBJ_INIT(&obj->uobj, &privpgops, 1);
	mutex_enter(&obj->uobj.vmobjlock);
	obj->maddr = maddr;
	obj->npages = npages;
	obj->domid = domid;
	mutex_exit(&obj->uobj.vmobjlock);
	uvmflag = UVM_MAPFLAG(prot, prot, UVM_INH_NONE, UVM_ADV_NORMAL,
	    UVM_FLAG_FIXED | UVM_FLAG_NOMERGE);
	error = uvm_map(map, &newstart, size, &obj->uobj, 0, 0, uvmflag);

	if (error) {
		if (obj)
			obj->uobj.pgops->pgo_detach(&obj->uobj);
		return error;
	}
	if (newstart != start) {
		printf("uvm_map didn't give us back our vm space\n");
		return EINVAL;
	}
	return 0;
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

	KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
	KERNFS_INITENTRY(dkt, DT_REG, "privcmd", NULL, kfst, VREG,
	    PRIVCMD_MODE);
	kernfs_addentry(kernxen_pkt, dkt);
}
