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
 *	$Id: mem.c,v 1.10 1994/09/20 16:51:09 gwr Exp $
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
#include <vm/vm_param.h>
#include <vm/lock.h>
#include <vm/pmap.h>
#include <vm/vm_prot.h>

#include <machine/cpu.h>

extern int eeprom_uio();

extern int physmem;
extern caddr_t vmmap;
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
	register u_int c, v;
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
			v = uio->uio_offset;
#ifndef DEBUG
			/* allow reads only in RAM (except for DEBUG) */
			if (v >= (u_int)physmem)
				return (EFAULT);
#endif
			pmap_enter(kernel_pmap, (vm_offset_t)vmmap,
			    trunc_page(v), uio->uio_rw == UIO_READ ?
			    VM_PROT_READ : VM_PROT_WRITE, TRUE);
			o = (int)uio->uio_offset & PGOFSET;
			c = (u_int)(NBPG - ((int)iov->iov_base & PGOFSET));
			c = min(c, (u_int)(NBPG - o));
			c = min(c, (u_int)iov->iov_len);
			error = uiomove((caddr_t)&vmmap[o], (int)c, uio);
			pmap_remove(kernel_pmap, (vm_offset_t)vmmap,
			    (vm_offset_t)&vmmap[NBPG]);
			continue;

/* minor device 1 is kernel memory */
		case 1:
			o = uio->uio_offset;
			c = min(iov->iov_len, MAXPHYS);
			if (!kernacc((caddr_t)o, c,
			    uio->uio_rw == UIO_READ ? B_READ : B_WRITE))
				return (EFAULT);
			error = uiomove((caddr_t)o, (int)c, uio);
			continue;

/* minor device 2 is EOF/RATHOLE */
		case 2:
			if (uio->uio_rw == UIO_READ)
				return (0);
			c = iov->iov_len;
			break;

/* minor device 11 (/dev/eeprom) accesses Non-Volatile RAM */
		case 11:
			error = eeprom_uio(uio);
			return (error);

/* minor device 12 (/dev/zero) is source of nulls on read, rathole on write */
		case 12:
			if (uio->uio_rw == UIO_WRITE) {
				c = iov->iov_len;
				break;
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
