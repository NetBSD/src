/*	$NetBSD: rootfil.c,v 1.15 1997/01/31 02:13:42 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
 * All rights reserved.
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
 * from: Utah Hdr: machdep.c 1.63 91/04/24
 *
 *	@(#)machdep.c	7.16 (Berkeley) 6/3/91
 */
 /* All bugs are subject to removal without further notice */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/macros.h>
#include <machine/nexus.h>
#include <machine/sid.h>
#include <machine/pte.h>
#include <machine/cpu.h>

#include "hp.h"
#include "ra.h"

u_long  bootdev;                /* should be dev_t, but not until 32 bits */

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
void
findroot(devpp, partp)
	struct device **devpp;
	int *partp;
{
        int majdev, unit, part, controller, adaptor;
	extern int boothowto;

	/*
	 * Default to "not found".
	 */
	*devpp = NULL;
	*partp = 0;

#if 0
	printf("howto %x bootdev %x ", boothowto, bootdev);
#endif

        if ((bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
                return;

        majdev = B_TYPE(bootdev);
        adaptor = B_ADAPTOR(bootdev);
        controller = B_CONTROLLER(bootdev);
        part = B_PARTITION(bootdev);
        unit = B_UNIT(bootdev);

	switch (majdev) {
	case 0:	/* MBA disk */
#if NHP > 0
		if (hp_getdev(adaptor, unit, devpp) < 0)
#endif
			return;
		break;

	case 9:	/* MSCP disk */
#if NRA > 0
		if (ra_getdev(adaptor, controller, unit, devpp) < 0)
#endif
			return;
		break;

	default:
		return;
	}

	*partp = part;
}
