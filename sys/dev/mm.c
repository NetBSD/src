/*	$NetBSD: mm.c,v 1.13.16.1 2010/03/18 04:36:54 rmind Exp $	*/

/*-
 * Copyright (c) 2002, 2008, 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Joerg Sonnenberger.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mm.c,v 1.13.16.1 2010/03/18 04:36:54 rmind Exp $");

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/termios.h>

#include <dev/mm.h>

#include <uvm/uvm_extern.h>

static void *		dev_zero_page;
static kmutex_t		dev_mem_lock;
static vaddr_t		dev_mem_addr;

static dev_type_read(mm_readwrite);
static dev_type_ioctl(mm_ioctl);
static dev_type_mmap(mm_mmap);
static dev_type_ioctl(mm_ioctl);

const struct cdevsw mem_cdevsw = {
#ifdef __HAVE_MM_MD_OPEN
	mm_md_open,
#else
	nullopen,
#endif
	nullclose, mm_readwrite, mm_readwrite,
	mm_ioctl, nostop, notty, nopoll, mm_mmap, nokqfilter,
	D_MPSAFE
};

void
mm_init(void)
{
	vaddr_t pg;

	mutex_init(&dev_mem_lock, MUTEX_DEFAULT, IPL_NONE);

	pg = uvm_km_alloc(kernel_map, PAGE_SIZE, 0, UVM_KMF_WIRED|UVM_KMF_ZERO);
	KASSERT(pg != 0);
	pmap_protect(pmap_kernel(), pg, pg + PAGE_SIZE, VM_PROT_READ);
	dev_zero_page = (void *)pg;

	dev_mem_addr = uvm_km_alloc(kernel_map, PAGE_SIZE, 0, UVM_KMF_VAONLY);
	KASSERT(dev_mem_addr != 0);
}

static int
dev_mem_readwrite(struct uio *uio, struct iovec *iov)
{
	bool have_direct;
	paddr_t paddr;
	vaddr_t vaddr;
	vm_prot_t prot;
	size_t len, offset;
	int error;

	if ((intptr_t)uio->uio_offset != uio->uio_offset) {
		return EFAULT;
	}
	paddr = uio->uio_offset & ~PAGE_MASK;
	offset = uio->uio_offset & PAGE_MASK;
	len = min(uio->uio_resid, PAGE_SIZE - offset);
	prot = uio->uio_rw == UIO_WRITE ? VM_PROT_WRITE : VM_PROT_READ;

	error = mm_md_physacc(paddr, prot);
	if (error) {
		return error;
	}

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	have_direct = mm_md_direct_mapped_phys(paddr, &vaddr);
#else
	have_direct = false;
#endif
	if (!have_direct) {
		mutex_enter(&dev_mem_lock);
		pmap_enter(pmap_kernel(), dev_mem_addr, paddr, prot,
		    prot | PMAP_WIRED);
		pmap_update(pmap_kernel());

		vaddr = dev_mem_addr + offset;
		error = uiomove((void *)vaddr, len, uio);

		pmap_remove(pmap_kernel(), dev_mem_addr, PAGE_SIZE);
		mutex_exit(&dev_mem_lock);
	} else {
		error = uiomove((void *)vaddr, len, uio);
	}
	return error;
}

static int
dev_kmem_readwrite(struct uio *uio, struct iovec *iov)
{
	void *addr;
	size_t len, offset;
	vm_prot_t prot;
	int error;
	bool md_kva;

	addr = (void *)(intptr_t)uio->uio_offset;
	if ((uintptr_t)addr != uio->uio_offset) {
		return EFAULT;
	}
	offset = uio->uio_offset & PAGE_MASK;
	len = min(uio->uio_resid, PAGE_SIZE - offset);
	prot = uio->uio_rw == UIO_WRITE ? VM_PROT_WRITE : VM_PROT_READ;

	md_kva = false;

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_IO
	paddr_t paddr;
	if (mm_md_direct_mapped_io(addr, &paddr)) {
		error = mm_md_physacc(paddr, prot);
		if (error)
			return error;
		md_kva = true;
	}
#endif

#ifdef __HAVE_MM_MD_KERNACC
	if (!md_kva && (error = mm_md_kernacc(addr, prot, &md_kva)) != 0) {
		return error;
	}
#endif
	if (!md_kva && !uvm_kernacc(addr, prot)) {
		return EFAULT;
	}
	error = uiomove(addr, len, uio);
	return error;
}

static int
dev_zero_readwrite(struct uio *uio, struct iovec *iov)
{
	size_t len;

	if (uio->uio_rw == UIO_WRITE) {
		uio->uio_resid = 0;
		return 0;
	}
	len = min(iov->iov_len, PAGE_SIZE);
	return uiomove(dev_zero_page, len, uio);
}

static int
mm_readwrite(dev_t dev, struct uio *uio, int flags)
{
	struct iovec *iov;
	int error;

#ifdef __HAVE_MM_MD_READWRITE
	switch (minor(dev)) {
	case DEV_MEM:
	case DEV_KMEM:
	case DEV_NULL:
	case DEV_ZERO:
#if defined(COMPAT_16) && defined(__arm)
	case _DEV_ZERO_oARM:
#endif
		break;
	default:
		return mm_md_readwrite(dev, uio);
	}
#endif
	error = 0;
	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			++uio->uio_iov;
			--uio->uio_iovcnt;
			KASSERT(uio->uio_iovcnt >= 0);
			continue;
		}
		switch (minor(dev)) {
		case DEV_MEM:
			error = dev_mem_readwrite(uio, iov);
			break;
		case DEV_KMEM:
			error = dev_kmem_readwrite(uio, iov);
			break;
		case DEV_NULL:
			if (uio->uio_rw == UIO_WRITE) {
				uio->uio_resid = 0;
			}
			/* Break directly out of the loop. */
			return 0;
#if defined(COMPAT_16) && defined(__arm)
		case _DEV_ZERO_oARM:
#endif
		case DEV_ZERO:
			error = dev_zero_readwrite(uio, iov);
			break;
		default:
			error = ENXIO;
			break;
		}
	}
	return error;
}

static paddr_t
mm_mmap(dev_t dev, off_t off, int acc)
{
	vm_prot_t prot;

#ifdef __HAVE_MM_MD_MMAP
	switch (minor(dev)) {
	case DEV_MEM:
	case DEV_KMEM:
	case DEV_NULL:
#if defined(COMPAT_16) && defined(__arm)
	case _DEV_ZERO_oARM:
#endif
	case DEV_ZERO:
		break;
	default:
		return mm_md_mmap(dev, off, acc);
	}
#endif
	/*
	 * /dev/null does not make sense, /dev/kmem is volatile and
	 * /dev/zero is handled in mmap already.
	 */
	if (minor(dev) != DEV_MEM) {
		return -1;
	}

	prot = 0;
	if (acc & PROT_EXEC)
		prot |= VM_PROT_EXECUTE;
	if (acc & PROT_READ)
		prot |= VM_PROT_READ;
	if (acc & PROT_WRITE)
		prot |= VM_PROT_WRITE;

	if (mm_md_physacc(off, prot) != 0) {
		return -1;
	}
	return off >> PGSHIFT;
}

static int
mm_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{

	switch (cmd) {
	case FIONBIO:
		/* We never block anyway. */
		return 0;

	case FIOSETOWN:
	case FIOGETOWN:
	case TIOCGPGRP:
	case TIOCSPGRP:
	case TIOCGETA:
		return ENOTTY;

	case FIOASYNC:
		if ((*(int *)data) == 0) {
			return 0;
		}
		/* FALLTHROUGH */
	default:
		return EOPNOTSUPP;
	}
}
