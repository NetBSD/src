/*	$NetBSD: mm_md.c,v 1.2.14.1 2014/08/20 00:03:26 tls Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mm_md.c,v 1.2.14.1 2014/08/20 00:03:26 tls Exp $");

#include <sys/param.h>
#include <sys/errno.h>
#include <uvm/uvm_extern.h>
#include <machine/eeprom.h>
#include <machine/leds.h>
#include <machine/pmap.h>
#include <dev/mm.h>

#define DEV_VME16D16	5	/* minor device 5 is /dev/vme16d16 */
#define DEV_VME24D16	6	/* minor device 6 is /dev/vme24d16 */
#define DEV_VME32D16	7	/* minor device 7 is /dev/vme32d16 */
#define DEV_VME16D32	8	/* minor device 8 is /dev/vme16d32 */
#define DEV_VME24D32	9	/* minor device 9 is /dev/vme24d32 */
#define DEV_VME32D32	10	/* minor device 10 is /dev/vme32d32 */
#define DEV_EEPROM	11 	/* minor device 11 is eeprom */
#define DEV_LEDS	13 	/* minor device 13 is leds */

int
mm_md_readwrite(dev_t dev, struct uio *uio)
{

	switch (minor(dev)) {
	case DEV_EEPROM:
		return eeprom_uio(uio);
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
#if 0 /* not yet */
	case DEV_VME16D16:
		if (off & 0xffff0000)
			return -1;
		off |= 0xff0000;
		/* fall through */
	case DEV_VME24D16:
		if (off & 0xff000000)
			return -1;
		off |= 0xff000000;
		/* fall through */
	case DEV_VME32D16:
		return (off | PMAP_VME16);

	case DEV_VME16D32:
		if (off & 0xffff0000)
			return -1;
		off |= 0xff0000;
		/* fall through */
	case DEV_VME24D32:
		if (off & 0xff000000)
			return -1;
		off |= 0xff000000;
		/* fall through */
	case DEV_VME32D32:
		return off | PMAP_VME32;
#endif
	default:
		return -1;
	}
}
