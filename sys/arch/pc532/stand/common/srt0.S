/*	$NetBSD: srt0.S,v 1.1 1997/05/17 13:56:12 matthias Exp $	*/

/*-
 * Copyright (c) 1994 Philip L. Budne.
 * All rights reserved.
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
 *	This product includes software developed by Philip L. Budne.
 * 4. The name of Philip L. Budne may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * srt0.s -- standalone C startup
 * Phil Budne, May 10, 1994
 */

#define _LOCORE
#include <machine/asm.h>
#include <machine/psl.h>

	BSS(bootdev, 4)
	BSS(howto, 4)

ENTRY(stext)
ASENTRY(start)
	/* Make sure that interrupts are off and we are using sp0. */
	bicpsrw	(PSL_I | PSL_US)

	lprd	sp,HEAP_LIMIT			/* Switch to a larger stack. */
	lprd    sb,0				/* gcc expects this. */
restart:
	/* Zero the bss segment. */
	addr	_C_LABEL(edata)(pc),r0		/* load start of bss segment */
	addr	_C_LABEL(end)(pc),r1		/* load end of bss segment */

	br	2f
1:	movqb	0,0(r0)
	addqd	1,r0
2:	cmpd	r0,r1
	bne	1b

	movqd	0,_C_LABEL(bootdev)(pc)		/* trash bootdev */

	/* Initialize Hardware */
	bsr	_C_LABEL(cninit)

	/* Call main program */
	bsr	_C_LABEL(main)
	br	_C_LABEL(_rtt)

ENTRY(run_prog)
	cmpqd	0,tos				/* drop return address. */
	restore [r0,r3,r4,r5,r6,r7]		/* load registers. */
	lprd	sp,0x1ffc			/* switch to new stack. */
	jump	0(r0)				/* jump to start address. */
