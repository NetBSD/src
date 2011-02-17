/*	$NetBSD: mem.c,v 1.48.50.1 2011/02/17 11:59:39 bouyer Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	@(#)mem.c	8.3 (Berkeley) 1/12/94
 */

/*
 * Memory special file
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mem.c,v 1.48.50.1 2011/02/17 11:59:39 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/uio.h>

#include <machine/cpu.h>

#include <uvm/uvm_extern.h>

extern u_int lowram;
static void *devzeropage;

static dev_type_read(mmrw);
dev_type_ioctl(mmioctl);
static dev_type_mmap(mmmmap);

const struct cdevsw mem_cdevsw = {
	nullopen, nullclose, mmrw, mmrw, mmioctl,
	nostop, notty, nopoll, mmmmap, nokqfilter,
};

/*ARGSUSED*/
static int
mmrw(dev_t dev, struct uio *uio, int flags)
{
	vaddr_t o, v;
	int c;
	struct iovec *iov;
	int error = 0;
	static int physlock;
	vm_prot_t prot;

	if (minor(dev) == DEV_MEM) {
		/* lock against other uses of shared vmmap */
		while (physlock > 0) {
			physlock++;
			error = tsleep((void *)&physlock, PZERO | PCATCH,
			    "mmrw", 0);
			if (error)
				return error;
		}
		physlock = 1;
	}
	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {

		case DEV_MEM:
			v = uio->uio_offset;

			/*
			 * Only allow reads in physical RAM.
			 */
			if (v >= 0xFFFFFFFC || v < lowram) {
				error = EFAULT;
				goto unlock;
			}

			prot = uio->uio_rw == UIO_READ ? VM_PROT_READ :
			    VM_PROT_WRITE;
			pmap_enter(pmap_kernel(), (vaddr_t)vmmap,
			    trunc_page(v), prot, prot|PMAP_WIRED);
			pmap_update(pmap_kernel());
			o = uio->uio_offset & PGOFSET;
			c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
			error = uiomove(vmmap + o, c, uio);
			pmap_remove(pmap_kernel(), (vaddr_t)vmmap,
			    (vaddr_t)vmmap + PAGE_SIZE);
			pmap_update(pmap_kernel());
			continue;

		case DEV_KMEM:
			v = uio->uio_offset;
			c = min(iov->iov_len, MAXPHYS);
			if (!uvm_kernacc((void *)v, c,
			    uio->uio_rw == UIO_READ ? B_READ : B_WRITE))
				return EFAULT;

			/*
			 * Don't allow reading intio or dio
			 * device space.  This could lead to
			 * corruption of device registers.
			 */
			if (ISIIOVA(v) ||
			    (v >= (vaddr_t)extiobase &&
			    v < ((vaddr_t)extiobase +
			    (EIOMAPSIZE * PAGE_SIZE))))
				return EFAULT;

			error = uiomove((void *)v, c, uio);
			continue;

		case DEV_NULL:
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return 0;

		case DEV_ZERO:
			if (uio->uio_rw == UIO_WRITE) {
				c = iov->iov_len;
				break;
			}
			/*
			 * On the first call, allocate and zero a page
			 * of memory for use with /dev/zero.
			 */
			if (devzeropage == NULL) {
				devzeropage = malloc(PAGE_SIZE, M_TEMP,
				    M_WAITOK | M_ZERO);
			}
			c = min(iov->iov_len, PAGE_SIZE);
			error = uiomove(devzeropage, c, uio);
			continue;

		default:
			return ENXIO;
		}
		if (error)
			break;
		iov->iov_base = (char *)iov->iov_base + c;
		iov->iov_len -= c;
		uio->uio_offset += c;
		uio->uio_resid -= c;
	}
	if (minor(dev) == 0) {
unlock:
		if (physlock > 1)
			wakeup((void *)&physlock);
		physlock = 0;
	}
	return error;
}

static paddr_t
mmmmap(dev_t dev, off_t off, int prot)
{
	/*
	 * /dev/mem is the only one that makes sense through this
	 * interface.  For /dev/kmem any physaddr we return here
	 * could be transient and hence incorrect or invalid at
	 * a later time.  /dev/null just doesn't make any sense
	 * and /dev/zero is a hack that is handled via the default
	 * pager in mmap().
	 */
	if (minor(dev) != DEV_MEM)
		return -1;

	/*
	 * Allow access only in RAM.
	 */
	if ((u_int)off < lowram || (u_int)off >= 0xFFFFFFFC)
		return -1;
	return m68k_btop((u_int)off);
}
