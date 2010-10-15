/*	$NetBSD: mem.c,v 1.34 2010/10/15 15:55:53 tsutsui Exp $	*/

/*
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
 *	from: @(#)mem.c	8.3 (Berkeley) 1/12/94
 */
/*
 * Copyright (c) 1988 University of Utah.
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
 *	from: @(#)mem.c	8.3 (Berkeley) 1/12/94
 */

/*
 * Memory special file
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mem.c,v 1.34 2010/10/15 15:55:53 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/uio.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/leds.h>
#include <machine/mon.h>
#include <machine/pmap.h>
#include <machine/pte.h>

#include <sun3/sun3/machdep.h>

#define DEV_VME16D16	5	/* minor device 5 is /dev/vme16d16 */
#define DEV_VME24D16	6	/* minor device 6 is /dev/vme24d16 */
#define DEV_VME32D16	7	/* minor device 7 is /dev/vme32d16 */
#define DEV_VME16D32	8	/* minor device 8 is /dev/vme16d32 */
#define DEV_VME24D32	9	/* minor device 9 is /dev/vme24d32 */
#define DEV_VME32D32	10	/* minor device 10 is /dev/vme32d32 */
#define DEV_EEPROM	11 	/* minor device 11 is eeprom */
#define DEV_LEDS	13 	/* minor device 13 is leds */

/* XXX - Put this in pmap_pvt.h or something? */
extern paddr_t avail_start;

static int promacc(void *, int, int);
static void *devzeropage;

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
	vaddr_t o, v;
	int c, rw;
	int error = 0;
	static int physlock;
	vm_prot_t prot;

	if (minor(dev) == DEV_MEM) {
		if (vmmap == 0)
			return (EIO);
		/* lock against other uses of shared vmmap */
		while (physlock > 0) {
			physlock++;
			error = tsleep((void *)&physlock, PZERO | PCATCH,
			    "mmrw", 0);
			if (error)
				return (error);
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
			/* allow reads only in RAM */
			if (!pmap_pa_exists(v)) {
				error = EFAULT;
				goto unlock;
			}
			/*
			 * If the offset (physical address) is within the
			 * linearly mapped range (0 .. avail_start) then
			 * we can save some hair by using the /dev/kmem
			 * alias mapping known to exist for this range.
			 */
			if (v < avail_start) {
				v += KERNBASE3X;
				goto use_kmem;
			}
			/* Temporarily map the memory at vmmap. */
			prot = uio->uio_rw == UIO_READ ? VM_PROT_READ :
			    VM_PROT_WRITE;
			pmap_enter(pmap_kernel(), (vaddr_t)vmmap,
			    trunc_page(v), prot, prot|PMAP_WIRED);
			pmap_update(pmap_kernel());
			o = v & PGOFSET;
			c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
			error = uiomove((char *)vmmap + o, c, uio);
			pmap_remove(pmap_kernel(), (vaddr_t)vmmap,
			    (vaddr_t)vmmap + PAGE_SIZE);
			pmap_update(pmap_kernel());
			break;

		case DEV_KMEM:
			v = uio->uio_offset;
		use_kmem:
			/*
			 * One page at a time to simplify access checks.
			 * Note that we can get here from case 0 above!
			 */
			o = v & PGOFSET;
			c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
			rw = (uio->uio_rw == UIO_READ) ? B_READ : B_WRITE;
			if (!(uvm_kernacc((void *)v, c, rw) ||
			      promacc((void *)v, c, rw)))
			{
				error = EFAULT;
				/* Note: case 0 can get here, so must unlock! */
				goto unlock;
			}
			error = uiomove((void *)v, c, uio);
			break;

		case DEV_NULL:
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

		case DEV_EEPROM:
			error = eeprom_uio(uio);
			/* Yes, return (not break) so EOF works. */
			return (error);

		case DEV_ZERO:
			/* Write to /dev/zero is ignored. */
			if (uio->uio_rw == UIO_WRITE) {
				uio->uio_resid = 0;
				return (0);
			}
			/*
			 * On the first call, allocate and zero a page
			 * of memory for use with /dev/zero.
			 */
			if (devzeropage == NULL) {
				devzeropage = (void *)
				    malloc(PAGE_SIZE, M_TEMP, M_WAITOK);
				memset(devzeropage, 0, PAGE_SIZE);
			}
			c = min(iov->iov_len, PAGE_SIZE);
			error = uiomove(devzeropage, c, uio);
			break;

		case DEV_LEDS:
			error = leds_uio(uio);
			/* Yes, return (not break) so EOF works. */
			return (error);

		default:
			return (ENXIO);
		}
	}

	/*
	 * Note the different location of this label, compared with
	 * other ports.  This is because the /dev/mem to /dev/kmem
	 * redirection above jumps here on error to do its unlock.
	 */
unlock:
	if (minor(dev) == DEV_MEM) {
		if (physlock > 1)
			wakeup((void *)&physlock);
		physlock = 0;
	}
	return (error);
}

paddr_t 
mmmmap(dev_t dev, off_t off, int prot)
{
	/*
	 * Check address validity.
	 */
	if (off & PGOFSET)
		return (-1);

	switch (minor(dev)) {

	case DEV_MEM:		/* dev/mem */
		/* Allow access only in valid memory. */
		if (!pmap_pa_exists(off))
			break;
		return (off);

#if 0	/* XXX - NOTYET */
		/* XXX - Move this to bus_subr.c? */
	case DEV_VME16D16:
		if (off & 0xffff0000)
			break;
		off |= 0xff0000;
		/* fall through */
	case DEV_VME24D16:
		if (off & 0xff000000)
			break;
		off |= 0xff000000;
		/* fall through */
	case DEV_VME32D16:
		return (off | PMAP_VME16);

	case DEV_VME16D32:
		if (off & 0xffff0000)
			break;
		off |= 0xff0000;
		/* fall through */
	case DEV_VME24D32:
		if (off & 0xff000000)
			break;
		off |= 0xff000000;
		/* fall through */
	case DEV_VME32D32:
		return (off | PMAP_VME32);
#endif	/* XXX */
	}

	return (-1);
}


/*
 * Just like uvm_kernacc(), but for the PROM mappings.
 * Return non-zero if access at VA is allowed.
 */
static int 
promacc(void *va, int len, int rw)
{
	vaddr_t sva, eva;

	sva = (vaddr_t)va;
	eva = (vaddr_t)va + len;

	/* Test for the most common case first. */
	if (sva < SUN3X_PROM_BASE)
		return (0);

	/* Read in the PROM itself is OK. */
	if ((rw == B_READ) && (eva <= SUN3X_MONEND))
		return (1);

	/* PROM data page is OK for read/write. */
	if ((sva >= SUN3X_MONDATA) &&
		(eva <= (SUN3X_MONDATA + PAGE_SIZE)))
		return (1);

	/* otherwise, not OK to touch */
	return (0);
}
