/*	$NetBSD: memc_3min.c,v 1.4 1999/05/20 14:15:57 nisimura Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
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
 * from: Utah Hdr: trap.c 1.32 91/04/06
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/11/94
 */


#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: memc_3min.c,v 1.4 1999/05/20 14:15:57 nisimura Exp $");

/*
 * Motherboard memory error contoller used in both
 * 3min (kn02ba) and maxine (kn02ca).
 */

#include <sys/types.h>
#include <sys/systm.h>

#include <mips/cpuregs.h>

#include <pmax/pmax/kmin.h>
#include <pmax/pmax/dec_kn02_subr.h>


/* forward declarations */
static unsigned kn02ba_recover_erradr __P((u_int phys, u_int mer));


static unsigned
kn02ba_recover_erradr(phys, mer)
	unsigned phys, mer;
{
	/* phys holds bits 28:2, mer knows which byte */
	switch (mer & KMIN_MER_LASTBYTE) {
	case KMIN_LASTB31:
		mer = 3; break;
	case KMIN_LASTB23:
		mer = 2; break;
	case KMIN_LASTB15:
		mer = 1; break;
	case KMIN_LASTB07:
		mer = 0; break;
	}
	return ((phys & KMIN_AER_ADDR_MASK) | mer);
}



/*
 * Handle error
 * All we can do with parity is panic.
 * XXX check for clean user pages, replace frame,  and reload ?
 */
void
kn02ba_errintr()
{
	int mer, adr, siz;
	static int errintr_cnt = 0;

	siz = *(volatile int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_MSR);
	mer = *(volatile int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_MER);
	adr = *(volatile int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_AER);

	/* clear interrupt bit */
	*(unsigned int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;

	errintr_cnt++;
	printf("(%d)%s%x [%x %x %x]\n", errintr_cnt,
	       "Bad memory chip at phys ",
	       kn02ba_recover_erradr(adr, mer),
	       mer, siz, adr);
}
