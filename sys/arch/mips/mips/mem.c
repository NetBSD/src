/*	$NetBSD: mem.c,v 1.35.38.10 2012/01/10 18:36:58 matt Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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

#include "opt_cputype.h"
#include "opt_mips_cache.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mem.c,v 1.35.38.10 2012/01/10 18:36:58 matt Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/msgbuf.h>
#include <sys/event.h>

#include <machine/cpu.h>

#include <mips/cache.h>
#ifdef _LP64
#include <mips/mips3_pte.h>
#endif

#include <uvm/uvm_extern.h>

void *zeropage;

dev_type_read(mmrw);
dev_type_ioctl(mmioctl);

const struct cdevsw mem_cdevsw = {
	nullopen, nullclose, mmrw, mmrw, mmioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

#if defined(pmax)
const struct cdevsw mem_ultrix_cdevsw = {
	nullopen, nullclose, mmrw, mmrw, mmioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};
#endif /* defined(pmax) */

/*ARGSUSED*/
int
mmrw(dev_t dev, struct uio *uio, int flags)
{
	vaddr_t v;
	int c;
	struct iovec *iov;
	int error = 0;

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
			c = iov->iov_len;
			/*
			 * XXX Broken; assumes contiguous physical memory.
			 */
#ifdef _LP64
			if (v + c > ctob(physmem))
				return (EFAULT);
			v = MIPS_PHYS_TO_XKPHYS_CACHED(v);
#else
			if (MIPS_KSEG0_P(v + c - 1)) {
				v = MIPS_PHYS_TO_KSEG0(v);
#ifdef ENABLE_MIPS_KSEGX
			} else if (mips_ksegx_start <= v
			    && v + c <= mips_ksegx_start + VM_KSEGX_SIZE) {
				v += VM_KSEGX_ADDRESS - mips_ksegx_start;
#endif
			} else
				return (EFAULT);
#endif
			error = uiomove((void *)v, c, uio);
#if defined(MIPS3_PLUS)
			if (MIPS_CACHE_VIRTUAL_ALIAS)
				mips_dcache_wbinv_range(v, c);
#endif
			continue;

		case DEV_KMEM:
			v = uio->uio_offset;
			c = min(iov->iov_len, MAXPHYS);
#ifdef _LP64
			if (v < MIPS_XKPHYS_START) {
				return (EFAULT);
			} else if (MIPS_XKPHYS_P(v)
			    && v > MIPS_PHYS_TO_XKPHYS_CACHED(mips_avail_end +
					mips_round_page(MSGBUFSIZE) - c)) {
				return (EFAULT);
			} else if (MIPS_XKSEG_P(v)
			    && v < MIPS_KSEG0_START
			    && !uvm_kernacc((void *)v, c,
			    uio->uio_rw == UIO_READ ? B_READ : B_WRITE)) {
				return (EFAULT);
			} else if (MIPS_KSEG1_P(v) || MIPS_KSEG2_P(v)) {
				return (EFAULT);
			}
#else
			if (v < MIPS_KSEG0_START)
				return (EFAULT);
			if (MIPS_KSEG0_P(v + c - 1)) {
				/*
				 * If all of memory is in KSEG0, make sure we
				 * don't go beyond its limit.  (mips_avail_end
				 * may be beyond the end of KSEG0).
				 */
				if (MIPS_KSEG0_TO_PHYS(v) >= mips_avail_end
				    + mips_round_page(MSGBUFSIZE) - c)
					return (EFAULT);
#ifdef ENABLE_MIPS_KSEGX
			} else if (VM_KSEGX_ADDRESS <= v
			    && v + c <= VM_KSEGX_ADDRESS + VM_KSEGX_SIZE) {
				/* nothing */
#endif
			} else if (v < MIPS_KSEG2_START
				   || !uvm_kernacc((void *)v, c,
					    uio->uio_rw == UIO_READ
						? B_READ
						: B_WRITE))
				return (EFAULT);
#endif
			error = uiomove((void *)v, c, uio);
#if defined(MIPS3_PLUS)
			if (MIPS_CACHE_VIRTUAL_ALIAS)
				mips_dcache_wbinv_range(v, c);
#endif
			continue;

		case DEV_NULL:
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

		case DEV_ZERO:
			if (uio->uio_rw == UIO_WRITE) {
				c = iov->iov_len;
				break;
			}
			if (zeropage == NULL) {
				zeropage = malloc(PAGE_SIZE, M_TEMP, M_WAITOK);
				memset(zeropage, 0, PAGE_SIZE);
			}
			c = min(iov->iov_len, PAGE_SIZE);
			error = uiomove(zeropage, c, uio);
			continue;

		default:
			return (ENXIO);
		}
		if (error)
			break;
		iov->iov_base = (char *)iov->iov_base + c;
		iov->iov_len -= c;
		uio->uio_offset += c;
		uio->uio_resid -= c;
	}
	return (error);
}
