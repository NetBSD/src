/*	$NetBSD: openpicreg.h,v 1.3 2001/08/30 02:08:43 briggs Exp $	*/

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

#include "opt_openpic.h"

#if defined(OPENPIC_SERIAL_MODE)
#define	SANDPOINT_INTR_SIOINT		0
#define	SANDPOINT_INTR_ISA		SANDPOINT_INTR_SIOINT
#define	SANDPOINT_INTR_SIOINT2		1 /* unknown... */
#define	SANDPOINT_INTR_PCI0		2
#define	SANDPOINT_INTR_PCI1		3
#define	SANDPOINT_INTR_PCI2		4
#define	SANDPOINT_INTR_PCI3		5
#define	SANDPOINT_INTR_WINBOND_A	6
#define	SANDPOINT_INTR_WINBOND_B	7
#define	SANDPOINT_INTR_WINBOND_C	8
#define	SANDPOINT_INTR_IDE0		SANDPOINT_INTR_WINBOND_C
#define	SANDPOINT_INTR_WINBOND_D	9
#define	SANDPOINT_INTR_IDE1		SANDPOINT_INTR_WINBOND_D
#define	SANDPOINT_INTR_RESERVED_10	10
#define	SANDPOINT_INTR_RESERVED_11	11
#define	SANDPOINT_INTR_RESERVED_12	12
#define	SANDPOINT_INTR_RESERVED_13	13
#define	SANDPOINT_INTR_RESERVED_14	14
#define	SANDPOINT_INTR_RESERVED_15	15
#define	SANDPOINT_INTR_SERCON		16	/* Not often used */
#define	SANDPOINT_INTR_I2C		17
#define	SANDPOINT_INTR_DMA0		18
#define	SANDPOINT_INTR_DMA1		19
#define	SANDPOINT_INTR_I2O		20
#define	SANDPOINT_INTR_TIMER0		21
#define	SANDPOINT_INTR_TIMER1		22
#define	SANDPOINT_INTR_TIMER2		23
#define	SANDPOINT_INTR_TIMER3		24

#define	OPENPIC_MAX_EXTERNAL_INT	15

#else

#define	SANDPOINT_INTR_PCI0		0
#define	SANDPOINT_INTR_IDE0		SANDPOINT_INTR_PCI0
#define	SANDPOINT_INTR_PCI1		1
#define	SANDPOINT_INTR_IDE1		SANDPOINT_INTR_PCI1
#define	SANDPOINT_INTR_ISA		SANDPOINT_INTR_PCI1
#define	SANDPOINT_INTR_PCI2		2
#define	SANDPOINT_INTR_PCI3		3
#define	SANDPOINT_INTR_SERCON		4	/* Not often used */
#define	SANDPOINT_INTR_I2C		5
#define	SANDPOINT_INTR_DMA0		6
#define	SANDPOINT_INTR_DMA1		7
#define	SANDPOINT_INTR_I2O		8
#define	SANDPOINT_INTR_TIMER0		9
#define	SANDPOINT_INTR_TIMER1		10
#define	SANDPOINT_INTR_TIMER2		11
#define	SANDPOINT_INTR_TIMER3		12

#define	OPENPIC_MAX_EXTERNAL_INT	4
#endif

/* interrupt vector/priority reg */
#define OPENPIC_SRC_VECTOR(irq) \
	    ((irq <= OPENPIC_MAX_EXTERNAL_INT) ? (0x10200 + (irq) * 0x20) \
		: ((irq <= OPENPIC_MAX_EXTERNAL_INT + 3) ? \
			(0x11000 + ((irq) - OPENPIC_MAX_EXTERNAL_INT) * 0x20) \
		    : ((irq == OPENPIC_MAX_EXTERNAL_INT + 4) ? 0x110C0 \
		       : (0x01120 + \
			  ((irq) - (OPENPIC_MAX_EXTERNAL_INT + 5)) * 0x40))))

#define	OPENPIC_INIT_SRC(irq) \
	    (((OPENPIC_IMASK | (8 << OPENPIC_PRIORITY_SHIFT)) | \
		(((irq) <= OPENPIC_MAX_EXTERNAL_INT) ? \
		    (OPENPIC_POLARITY_NEGATIVE | OPENPIC_SENSE_LEVEL) : 0)) \
	    | (irq))
	    
#define OPENPIC_IDEST(irq)		OPENPIC_SRC_VECTOR(irq) + 0x10

void openpic_init __P((unsigned char *));

#include <powerpc/openpicreg.h>
