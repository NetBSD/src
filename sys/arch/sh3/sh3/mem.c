/*	$NetBSD: mem.c,v 1.14 2003/04/02 02:56:41 thorpej Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 *	@(#)mem.c	8.3 (Berkeley) 1/12/94
 */

/*
 * Memory special file
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <uvm/uvm_extern.h>

caddr_t zeropage;
boolean_t __mm_mem_addr(paddr_t);

dev_type_read(mmrw);
dev_type_ioctl(mmioctl);
dev_type_mmap(mmmmap);

const struct cdevsw mem_cdevsw = {
	nullopen, nullclose, mmrw, mmrw, mmioctl,
	nostop, notty, nopoll, mmmmap, nokqfilter,
};

/*ARGSUSED*/
int
mmrw(dev_t dev, struct uio *uio, int flags)
{
	struct iovec *iov;
	vaddr_t v, o;
	int c;
	int error = 0;

	while (uio->uio_resid > 0 && !error) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}

		v = uio->uio_offset;

		switch (minor(dev)) {
		kmemphys:
		case DEV_MEM:
			/* Physical address */
			if (__mm_mem_addr(v)) {
				o = v & PGOFSET;
				c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
				error = uiomove((caddr_t)SH3_PHYS_TO_P1SEG(v),
				    c, uio);
			} else {
				return (EFAULT);
			}
			break;

		case DEV_KMEM:
			/* P0 */
			if (v < SH3_P1SEG_BASE)
				return (EFAULT);
			/* P1 */
			if (v < SH3_P2SEG_BASE) {
				v = SH3_P1SEG_TO_PHYS(v);
				goto kmemphys;
			}
			/* P2 */
			if (v < SH3_P3SEG_BASE)
				return (EFAULT);
			/* P3 */
			c = min(iov->iov_len, MAXPHYS);
			if (!uvm_kernacc((void *)v, c,
			    uio->uio_rw == UIO_READ ? B_READ : B_WRITE))
				return (EFAULT);
			error = uiomove((caddr_t)v, c, uio);
			break;

		case DEV_NULL:
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

		case DEV_ZERO:
			if (uio->uio_rw == UIO_WRITE) {
				uio->uio_resid = 0;
				return (0);
			}
			if (zeropage == NULL) {
				zeropage = malloc(PAGE_SIZE, M_TEMP, M_WAITOK);
				memset(zeropage, 0, PAGE_SIZE);
			}
			c = min(iov->iov_len, PAGE_SIZE);
			error = uiomove(zeropage, c, uio);
			break;

		default:
			return (ENXIO);
		}
	}

	return (error);
}

paddr_t
mmmmap(dev_t dev, off_t off, int prot)
{
	struct proc *p = curproc;

	if (minor(dev) != DEV_MEM)
		return (-1);

	if (!__mm_mem_addr(off) && suser(p->p_ucred, &p->p_acflag) != 0)
		return (-1);
	return (trunc_page((paddr_t)off));
}

/*
 * boolean_t __mm_mem_addr(paddr_t pa):
 *	Check specified physical address is memory device.
 */
boolean_t
__mm_mem_addr(paddr_t pa)
{

	return ((atop(pa) < vm_physmem[0].start || PHYS_TO_VM_PAGE(pa) != NULL)
	    ? TRUE : FALSE);
}
