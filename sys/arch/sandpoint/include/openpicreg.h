/*	$NetBSD: openpicreg.h,v 1.2 2001/02/05 19:22:24 briggs Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Allen Briggs for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * INTERRUPT SOURCE register
 * This is kind of odd on the MPC8240.  The interrupts are kind of
 * spread around.
 *	* The 5 external interrupts are at 0x10200 + irq * 0x20
 *	* The next 3 interrupts are at 0x11000 + (irq - 4) * 0x20
 *	* The next interrupt is at 0x110C0 + (irq - 4) * 0x20
 *	* The last interrupts are at 0x01120 + (irq - 9) * 0x20
 */

/* interrupt vector/priority reg */
#define OPENPIC_SRC_VECTOR(irq) \
	    ((irq <= 4) ? (0x10200 + (irq) * 0x20) \
		: ((irq <= 7) ? (0x11000 + ((irq) - 4) * 0x20) \
		    : ((irq == 8) ? 0x110C0 \
		       : (0x01120 + ((irq) - 9) * 0x40))))

#define	OPENPIC_INIT_SRC(irq) \
	    (((OPENPIC_IMASK | (8 << OPENPIC_PRIORITY_SHIFT)) | \
		(((irq) <= 4) ? \
		    (OPENPIC_POLARITY_NEGATIVE | OPENPIC_SENSE_LEVEL) : 0)) \
	    | (irq))
	    
#define OPENPIC_IDEST(irq)		OPENPIC_SRC_VECTOR(irq) + 0x10

void openpic_init __P((unsigned char *));

#include <powerpc/openpicreg.h>
