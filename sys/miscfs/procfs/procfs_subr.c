/*
 * Copyright (c) 1993 Paul Kranenburg
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
 *      This product includes software developed by Paul Kranenburg.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *
 *	$Id: procfs_subr.c,v 1.4 1993/12/18 03:58:08 mycroft Exp $
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/resourcevar.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_kern.h>
#include <sys/kinfo.h>
#include <sys/kinfo_proc.h>

#include <sys/procfs.h>
#include <miscfs/procfs/pfsnode.h>

#include <machine/vmparam.h>

/*
 * Get process address map (PIOCGMAP)
 */
int
pfs_vmmap(procp, pfsp, pmapp)
struct proc	*procp;
struct nfsnode	*pfsp;
struct procmap	*pmapp;
{
	int		error = 0;
	vm_map_t	map;
	vm_map_entry_t	entry;
	struct procmap	prmap;

	map = &procp->p_vmspace->vm_map;
	vm_map_lock(map);
	entry = map->header.next;

	while (entry != &map->header) {
		if (entry->is_a_map) {
			vm_map_t	submap = entry->object.share_map;
			vm_map_entry_t	subentry;

			vm_map_lock(submap);
			subentry = submap->header.next;
			while (subentry != &submap->header) {
				prmap.vaddr = subentry->start;
				prmap.size = subentry->end - subentry->start;
				prmap.offset = subentry->offset;
				prmap.prot = subentry->protection;
				error = copyout(&prmap, pmapp, sizeof(prmap));
				if (error)
					break;
				pmapp++;
				subentry = subentry->next;
			}
			vm_map_unlock(submap);
			if (error)
				break;
		}
		prmap.vaddr = entry->start;
		prmap.size = entry->end - entry->start;
		prmap.offset = entry->offset;
		prmap.prot = entry->protection;
		error = copyout(&prmap, pmapp, sizeof(prmap));
		if (error)
			break;
		pmapp++;
		entry = entry->next;
	}

	vm_map_unlock(map);
	return error;
}

/*
 * Count number of VM entries of process (PIOCNMAP)
 */
int
pfs_vm_nentries(procp, pfsp)
struct proc	*procp;
struct nfsnode	*pfsp;
{
	int		count = 0;
	vm_map_t	map;
	vm_map_entry_t	entry;

	map = &procp->p_vmspace->vm_map;
	vm_map_lock(map);
	entry = map->header.next;

	while (entry != &map->header) {
		if (entry->is_a_map)
			count += entry->object.share_map->nentries;
		else
			count++;
		entry = entry->next;
	}

	vm_map_unlock(map);
	return count;
}

/*
 * Map process mapped file to file descriptor (PIOCGMAPFD)
 */
int
pfs_vmfd(procp, pfsp, vmfdp, p)
struct proc	*procp;
struct pfsnode	*pfsp;
struct vmfd	*vmfdp;
struct proc	*p;
{
	int		rv;
	vm_map_t	map;
	vm_offset_t	addr;
	vm_size_t	size;
	vm_prot_t	prot, maxprot;
	vm_inherit_t	inherit;
	boolean_t	shared;
	vm_object_t	object;
	vm_offset_t	objoff;
	struct vnode	*vp;
	struct file	*fp;
	extern struct fileops	vnops;

	map = &procp->p_vmspace->vm_map;

	addr = vmfdp->vaddr;
	rv = vm_region(map, &addr, &size, &prot, &maxprot,
			&inherit, &shared, &object, &objoff);

	if (rv != KERN_SUCCESS)
		return EINVAL;

	while (object != NULL && object->pager == NULL)
		object = object->shadow;

	if (object == NULL || object->pager == NULL
			/* Nobody seems to care || !object->pager_ready */ )
		return ENOENT;

	if (object->pager->pg_type != PG_VNODE)
		return ENOENT;

	/* We have a vnode pager, allocate file descriptor */
	vp = (struct vnode *)object->pager->pg_handle;
	if (VOP_ACCESS(vp, VREAD, p->p_ucred, p)) {
		rv = EACCES;
		goto out;
	}
	rv = falloc(p, &fp, &vmfdp->fd);
	if (rv)
		goto out;

	VREF(vp);
	fp->f_type = DTYPE_VNODE;
	fp->f_ops = &vnops;
	fp->f_data = (caddr_t)vp;
	fp->f_flag = FREAD;

out:
	vm_object_unlock(object);
	return rv;
}

/*
 * Vnode op for reading/writing.
 */
/* ARGSUSED */
pfs_doio(vp, uio, ioflag, cred)
	struct vnode *vp;
	register struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	struct pfsnode	*pfsp = VTOPFS(vp);
	struct proc	*procp;
	int		error = 0;
	long		n, on;

#ifdef DEBUG
	if (pfs_debug)
		printf("pfs_doio(%s): vp 0x%x, proc %x\n",
			uio->uio_rw==UIO_READ?"R":"W", vp, uio->uio_procp);
#endif

#ifdef DIAGNOSTIC
	if (vp->v_type != VPROC)
		panic("pfs_doio vtype");
#endif
	procp = pfsp->pfs_pid?pfind(pfsp->pfs_pid):&proc0;
	if (!procp)
		return ESRCH;

	if (procp->p_flag & SSYS)
		return EACCES;

	if (uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset < 0)
		return (EINVAL);

	do { /* One page at a time */
		int		rv;
		vm_map_t	map;
		vm_offset_t	offset;
		vm_size_t	size;
		vm_prot_t	oldprot = 0, prot, maxprot;
		vm_inherit_t	inherit;
		boolean_t	shared;
		vm_object_t	object;
		vm_offset_t	objoff;
		vm_page_t	m;
		vm_offset_t	kva;

		on = uio->uio_offset - trunc_page(uio->uio_offset);
		n = MIN(PAGE_SIZE-on, uio->uio_resid);

		/* Map page into kernel space */

		map = &procp->p_vmspace->vm_map;
#if 0
	vm_map_print(map, 1);
#endif

		offset = trunc_page(uio->uio_offset);

		rv = vm_region(map, &offset, &size, &prot, &maxprot,
				&inherit, &shared, &object, &objoff);
		if (rv != KERN_SUCCESS)
			return EINVAL;

		vm_object_unlock(object);

		if (uio->uio_rw == UIO_WRITE && (prot & VM_PROT_WRITE) == 0) {
			oldprot = prot;
			prot |= VM_PROT_WRITE;
			rv = vm_protect(map, offset, PAGE_SIZE, FALSE, prot);
			if (rv != KERN_SUCCESS)
				return EPERM;
		}
		/* Just fault the page */
		rv = vm_fault(map, offset, prot, FALSE);
		if (rv != KERN_SUCCESS)
			return EFAULT;

		/* Look up again as vm_fault() may have inserted a shadow object */
		rv = vm_region(map, &offset, &size, &prot, &maxprot,
				&inherit, &shared, &object, &objoff);
		if (rv != KERN_SUCCESS)
			return EINVAL;

		/* Now find the page */
		/* XXX hope it's still there, should we have wired it? */
		m = vm_page_lookup(object, objoff);
		if (m == NULL)
			return ESRCH;

		kva = kmem_alloc_wait(kernel_map, PAGE_SIZE);

		pmap_enter(vm_map_pmap(kernel_map), kva, VM_PAGE_TO_PHYS(m),
			VM_PROT_DEFAULT, TRUE);

		error = uiomove(kva + on, (int)n, uio);

		pmap_remove(vm_map_pmap(kernel_map), kva, kva + PAGE_SIZE);
		kmem_free_wakeup(kernel_map, kva, PAGE_SIZE);
		if (oldprot) {
			rv = vm_protect(map, offset, PAGE_SIZE, FALSE, oldprot);
			if (rv != KERN_SUCCESS)
				return EPERM;
		}

	} while (error == 0 && uio->uio_resid > 0);

	return (error);
}

#if 00
int
pfs_map(procp, kva, rw, offset)
struct proc	*procp;
int		rw;
vm_offset_t	*kva, offset;
{
	int		rv;
	vm_map_t	map;
	vm_size_t	size;
	vm_prot_t	prot, maxprot;
	vm_inherit_t	inherit;
	boolean_t	shared;
	vm_object_t	object;
	vm_offset_t	objoff;
	vm_page_t	m;

	map = &procp->p_vmspace->vm_map;
#if 0
	vm_map_print(map, 1);
#endif

	offset = trunc_page(offset);

	rv = vm_region(map, &offset, &size, &prot, &maxprot,
			&inherit, &shared, &object, &objoff);
	if (rv != KERN_SUCCESS)
		return EINVAL;

	vm_object_unlock(object);

	if (rw == UIO_WRITE && (prot & VM_PROT_WRITE) == 0) {
		prot |= VM_PROT_WRITE;
		rv = vm_protect(map, offset, PAGE_SIZE, FALSE, prot);
		if (rv != KERN_SUCCESS)
			return EPERM;
	}
	/* Just fault page */
	rv = vm_fault(map, offset, prot, FALSE);
	if (rv != KERN_SUCCESS)
		return EFAULT;

	/* Look up again as vm_fault() may have inserted a shadow object */
	rv = vm_region(map, &offset, &size, &prot, &maxprot,
			&inherit, &shared, &object, &objoff);
	if (rv != KERN_SUCCESS)
		return EINVAL;

	m = vm_page_lookup(object, objoff);
	if (m == NULL)
		return ESRCH;

	*kva = kmem_alloc_wait(kernel_map, PAGE_SIZE);

	pmap_enter(vm_map_pmap(kernel_map), *kva, VM_PAGE_TO_PHYS(m),
			VM_PROT_DEFAULT, TRUE);

	return 0;
}

int
pfs_unmap(procp, kva)
struct proc	*procp;
vm_offset_t	kva;
{
	pmap_remove(vm_map_pmap(kernel_map), kva, kva + PAGE_SIZE);
	kmem_free_wakeup(kernel_map, kva, PAGE_SIZE);
}
#endif
