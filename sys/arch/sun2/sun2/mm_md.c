/*	$NetBSD: mm_md.c,v 1.1.2.1 2010/03/18 04:36:52 rmind Exp $	*/

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
 * Copyright (c) 1994, 1995 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mm_md.c,v 1.1.2.1 2010/03/18 04:36:52 rmind Exp $");

#include <sys/param.h>
#include <sys/errno.h>
#include <uvm/uvm_extern.h>
#include <machine/leds.h>
#include <machine/pmap.h>
#include <dev/mm.h>

#define DEV_VME16D16	5	/* minor device 5 is /dev/vme16d16 */
#define DEV_VME24D16	6	/* minor device 6 is /dev/vme24d16 */
#define DEV_EEPROM	11 	/* minor device 11 is eeprom */
#define DEV_LEDS	13 	/* minor device 13 is leds */

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	/* Allow access only in "managed" RAM. */
	if (pa < avail_start || pa >= avail_end)
		return EFAULT;
	return 0;
}

bool
mm_md_direct_mapped_phys(paddr_t paddr, vaddr_t *vaddr)
{

	if (paddr >= avail_start)
		return false;
	*vaddr = paddr;
	return true;
}

/*
 * Allow access to the PROM mapping similiar to uvm_kernacc().
 */
int
mm_md_kernacc(void *ptr, vm_prot_t prot, bool *handled)
{

	if ((vaddr_t)ptr < SUN2_PROM_BASE || (vaddr_t)ptr > SUN2_MONEND) {
		*handled = false;
		return 0;
	}

	*handled = true;
	/* Read in the PROM itself is OK, write not. */
	if ((prot & VM_PROT_WRITE) == 0)
		return 0;
	return EFAULT;
}

int
mm_md_readwrite(dev_t dev, struct uio *uio)
{

	switch (minor(dev)) {
	case DEV_LEDS:
		return leds_uio(uio);
	default:
		return ENXIO;
	}
}

paddr_t 
mm_md_mmap(dev_t dev, off_t off, int prot)
{

	switch (minor(dev)) {
	case DEV_VME16D16:
		if (off & 0xffff0000)
			return -1;
		off |= 0xff0000;
		/* fall through */
	case DEV_VME24D16:
		if (off & 0xff000000)
			return -1;
		return (off | (off & 0x800000 ? PMAP_VME8: PMAP_VME0));
	default:
		return (-1);
	}
}
