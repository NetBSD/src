/*	$NetBSD: mem.c,v 1.32.14.1 1999/12/27 18:34:06 wrstuden Exp $	*/

/*
 * Copyright (c) 1994, 1995 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
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
 *	from: @(#)mem.c	8.3 (Berkeley) 1/12/94
 */

/*
 * Memory special file
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/uio.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/leds.h>
#include <machine/mon.h>
#include <machine/pmap.h>
#include <machine/pte.h>

#include <sun3/sun3/machdep.h>

#define	mmread	mmrw
cdev_decl(mm);
static int promacc __P((caddr_t, int, int));
static caddr_t devzeropage;


/*ARGSUSED*/
int
mmopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{

	return (0);
}

/*ARGSUSED*/
int
mmclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{

	return (0);
}

/*ARGSUSED*/
int
mmrw(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	register struct iovec *iov;
	register vm_offset_t o, v;
	register int c, rw;
	int error = 0;
	static int physlock;
	vm_prot_t prot;

	if (minor(dev) == 0) {
		if (vmmap == 0)
			return (EIO);
		/* lock against other uses of shared vmmap */
		while (physlock > 0) {
			physlock++;
			error = tsleep((caddr_t)&physlock, PZERO | PCATCH,
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

		case 0:                        /*  /dev/mem  */
			v = uio->uio_offset;
			/* allow reads only in RAM */
			if (v >= avail_end) {
				error = EFAULT;
				goto unlock;
			}
			/*
			 * If the offset (physical address) is outside the
			 * region of physical memory that is "managed" by
			 * the pmap system, then we are not allowed to
			 * call pmap_enter with that physical address.
			 * Everything from zero to avail_start is mapped
			 * linearly with physical zero at virtual KERNBASE,
			 * so redirect the access to /dev/kmem instead.
			 * This is a better alternative than hacking the
			 * pmap to deal with requests on unmanaged memory.
			 * Also note: unlock done at end of function.
			 */
			if (v < avail_start) {
				v += KERNBASE;
				goto use_kmem;
			}
			/* Temporarily map the memory at vmmap. */
			prot = uio->uio_rw == UIO_READ ? VM_PROT_READ :
			    VM_PROT_WRITE;
			pmap_enter(pmap_kernel(), (vaddr_t)vmmap,
			    trunc_page(v), prot, prot|PMAP_WIRED);
			o = v & PGOFSET;
			c = min(uio->uio_resid, (int)(NBPG - o));
			error = uiomove((caddr_t)vmmap + o, c, uio);
			pmap_remove(pmap_kernel(), (vaddr_t)vmmap,
			    (vaddr_t)vmmap + NBPG);
			break;

		case 1:                        /*  /dev/kmem  */
			v = uio->uio_offset;
		use_kmem:
			/*
			 * Watch out!  You might assume it is OK to copy
			 * up to MAXPHYS bytes here, but that is wrong.
			 * The next page might NOT be part of the range:
			 *   	(KERNBASE..(KERNBASE+avail_start))
			 * which is asked for here via the goto in the
			 * /dev/mem case above.  The consequence is that
			 * we copy one page at a time.  No big deal, as
			 * most requests are less than one page anyway.
			 */
			o = v & PGOFSET;
			c = min(uio->uio_resid, (int)(NBPG - o));
			rw = (uio->uio_rw == UIO_READ) ? B_READ : B_WRITE;
			if (!(uvm_kernacc((caddr_t)v, c, rw) ||
			      promacc((caddr_t)v, c, rw)))
			{
				error = EFAULT;
				/* Note: case 0 can get here, so must unlock! */
				goto unlock;
			}
			error = uiomove((caddr_t)v, c, uio);
			break;

		case 2:                        /*  /dev/null  */
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

		case 11:                        /*  /dev/eeprom  */
			error = eeprom_uio(uio);
			/* Yes, return (not break) so EOF works. */
			return (error);

		case 12:                        /*  /dev/zero  */
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
				devzeropage = (caddr_t)
				    malloc(NBPG, M_TEMP, M_WAITOK);
				bzero(devzeropage, NBPG);
			}
			c = min(iov->iov_len, NBPG);
			error = uiomove(devzeropage, c, uio);
			break;

		case 13:                        /*  /dev/leds  */
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
	if (minor(dev) == 0) {
		if (physlock > 1)
			wakeup((caddr_t)&physlock);
		physlock = 0;
	}
	return (error);
}

int
mmmmap(dev, off, prot)
	dev_t dev;
	int off, prot;
{
	register u_int v = off;

	/*
	 * Check address validity.
	 */
	if (v & PGOFSET)
		return (-1);

	switch (minor(dev)) {

	case 0:		/* dev/mem */
		/* Allow access only in "managed" RAM. */
		if (v < avail_start || v >= avail_end)
			break;
		return (v);

	case 5: 	/* dev/vme16d16 */
		if (v & 0xffff0000)
			break;
		v |= 0xff0000;
		/* fall through */
	case 6: 	/* dev/vme24d16 */
		if (v & 0xff000000)
			break;
		v |= 0xff000000;
		/* fall through */
	case 7: 	/* dev/vme32d16 */
		return (v | PMAP_VME16);

	case 8: 	/* dev/vme16d32 */
		if (v & 0xffff0000)
			break;
		v |= 0xff0000;
		/* fall through */
	case 9: 	/* dev/vme24d32 */
		if (v & 0xff000000)
			break;
		v |= 0xff000000;
		/* fall through */
	case 10:	/* dev/vme32d32 */
		return (v | PMAP_VME32);
	}

	return (-1);
}


/*
 * Just like uvm_kernacc(), but for the PROM mappings.
 * Return non-zero if access at VA is allowed.
 */
static int
promacc(va, len, rw)
	caddr_t va;
	int len, rw;
{
	vm_offset_t sva, eva;

	sva = (vm_offset_t)va;
	eva = (vm_offset_t)va + len;

	/* Test for the most common case first. */
	if (sva < SUN3_PROM_BASE)
		return (0);

	/* Read in the PROM itself is OK. */
	if ((rw == B_READ) && (eva <= SUN3_MONEND))
		return (1);

	/* PROM data page is OK for read/write. */
	if ((sva >= SUN3_MONSHORTPAGE) &&
		(eva <= (SUN3_MONSHORTPAGE+NBPG)))
		return (1);

	/* otherwise, not OK to touch */
	return (0);
}
