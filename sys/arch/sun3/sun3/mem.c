/*	$NetBSD: mem.c,v 1.14 1994/12/13 18:42:59 gwr Exp $	*/

/*
 * Copyright (c) 1994 Gordon W. Ross
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
 *	from: Utah $Hdr: mem.c 1.14 90/10/12$
 *	from: @(#)mem.c	8.3 (Berkeley) 1/12/94
 */

/*
 * Memory special file
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/pmap.h>

extern int eeprom_uio();

extern vm_offset_t avail_start;
extern int physmem;
caddr_t zeropage;

/*
 * Sun3: XXXXX
 * 
 * Need support for various vme spaces,
 * Also make the unit constants into macros.
 *
 */

/*ARGSUSED*/
mmrw(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	register int o;
	register u_int c;
	register caddr_t v;
	register struct iovec *iov;
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

/* minor device 0 is physical memory */
		case 0:
			o = uio->uio_offset;
#ifndef DEBUG
			/* allow reads only in RAM (except for DEBUG) */
			if (o >= (u_int)physmem)
				return (EFAULT);
#endif
			if (o >= avail_start) {
				vm_offset_t kva, pa;

				pa = trunc_page(o);
				o &= PGOFSET;
				kva = kmem_alloc_wait(phys_map, 1);
				pmap_enter(kernel_pmap, kva, pa | PMAP_NC,
					uio->uio_rw == UIO_READ ?
						VM_PROT_READ : VM_PROT_WRITE, TRUE);
				v = (caddr_t) kva + o;
				c = min(uio->uio_resid, (u_int)(NBPG - o));
				error = uiomove(v, (int)c, uio);
				pmap_remove(kernel_pmap, kva, kva + NBPG);
				kmem_free_wakeup(phys_map, kva, 1);
				continue;
			}
			/*
			 * The offset was below avail_start, where the pmap
			 * will refuse to create mappings.  Everything below
			 * there is mapped linearly with physical zero at
			 * virtual KERNBASE, so use kmem! (hack alert! 8-)
			 */
			v = (caddr_t)KERNBASE + o;
			goto use_kmem;

/* minor device 1 is kernel memory */
		case 1:
			v = (caddr_t)(long)uio->uio_offset;
		use_kmem:
			c = min(iov->iov_len, NBPG);	/* MAXPHYS? */
			if (!kernacc(v, c,
			    uio->uio_rw == UIO_READ ? B_READ : B_WRITE))
				return (EFAULT);
			error = uiomove(v, (int)c, uio);
			continue;

/* minor device 2 is EOF/RATHOLE */
		case 2:
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

/* minor device 11 (/dev/eeprom) accesses Non-Volatile RAM */
		case 11:
			error = eeprom_uio(uio);
			return (error);

/* minor device 12 (/dev/zero) is source of nulls on read, rathole on write */
		case 12:
			if (uio->uio_rw == UIO_WRITE) {
				uio->uio_resid = 0;
				return (0);
			}
			/*
			 * On the first call, allocate and zero a page
			 * of memory for use with /dev/zero.
			 */
			if (zeropage == NULL) {
				zeropage = (caddr_t)
				    malloc(CLBYTES, M_TEMP, M_WAITOK);
				bzero(zeropage, CLBYTES);
			}
			c = min(iov->iov_len, CLBYTES);
			error = uiomove(zeropage, (int)c, uio);
			continue;

		default:
			return (ENXIO);
		}
		if (error)
			break;
		iov->iov_base += c;
		iov->iov_len -= c;
		uio->uio_offset += c;
		uio->uio_resid -= c;
	}
	return (error);
}

mmmap(dev, off, prot)
	dev_t dev;
	int off, prot;
{
	/*
	 * /dev/mem is the only one that makes sense through this
	 * interface.  For /dev/kmem any physaddr we return here
	 * could be transient and hence incorrect or invalid at
	 * a later time.  /dev/null just doesn't make any sense
	 * and /dev/zero is a hack that is handled via the default
	 * pager in mmap().
	 */
	if (minor(dev) != 0)
		return (-1);
	/*
	 * Allow access only in RAM.
	 *
	 * This could be extended to allow access to IO space but
	 * it is probably better to make device drivers do it.
	 */
	if ((u_int)off >= (u_int)physmem)
		return (-1);
	return (sun3_btop(off));
}
