/*	$NetBSD: irqhandler.h,v 1.3 2002/02/17 23:58:35 bjh21 Exp $	*/

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * IRQ related stuff (defines + structures)
 *
 * Created      : 30/09/94
 */

#ifndef _ARM32_IRQHANDLER_H_
#define _ARM32_IRQHANDLER_H_

#if defined(_KERNEL_OPT)
#include "iomd.h"
#endif

#ifndef _LOCORE
#include <sys/types.h>
#endif /* _LOCORE */

/* Define the IRQ bits */

/*
 * XXX this is really getting rather horrible.
 * Shortly to be replaced with system specific interrupt tables and handling
 */

#if NIOMD > 0

/* Only for ARM7500 : */

/*#define IRQ_PRINTER	0x00*/
/*#define IRQ_RESERVED0	0x01*/
#define IRQ_BUTTON	0x02
#define IRQ_FLYBACK	0x03
#define IRQ_POR		0x04
#define IRQ_TIMER0	0x05
#define IRQ_TIMER1 	0x06

#define IRQ_DREQ3	0x08
/*#define IRQ_HD1	0x09*/
/*#define IRQ_HD	IRQ_HD1*/
#define IRQ_DREQ2	0x0A
/*#define IRQ_FLOPPY	0x0C*/
/*#define IRQ_SERIAL	0x0D*/
#define IRQ_KBDTX	0x0E
#define IRQ_KBDRX	0x0F

#define IRQ_IRQ3	0x10
#define IRQ_IRQ4	0x11
#define IRQ_IRQ5	0x12
#define IRQ_IRQ6	0x13
#define IRQ_IRQ7	0x14
#define IRQ_IRQ9	0x15
#define IRQ_IRQ10	0x16
#define IRQ_IRQ11	0x17

#define IRQ_MSDRX	0x18
#define IRQ_MSDTX	0x19
#define IRQ_ATOD	0x1A
#define IRQ_CLOCK	0x1B
#define IRQ_PANIC	0x1C
/*#define IRQ_RESERVED2	0x1D*/
/*#define IRQ_RESERVED3	0x1E*/


/*
 * Note that Sound DMA IRQ is on the 31st vector.
 * It's not part of the IRQD.
 */
#define IRQ_SDMA	0x1F

/* Several interrupts are different between the A7000 and RC7500 */
#ifdef RC7500

#define IRQ_FIQDOWN	0x07
#define IRQ_ETHERNET	0x0B
#define IRQ_HD2		IRQ_IRQ11

#else	/* RC7500 */

/*#define IRQ_RESERVED1	0x07 */
#define IRQ_EXTENDED	0x0B
#define IRQ_PODULE	0x0D

#endif	/* RC7500 */




/* for non ARM7500 machines : */

/*#define IRQ_PRINTER	0x00*/
/*#define IRQ_RESERVED0	0x01*/
/*#define IRQ_FLOPPYIDX	0x02*/
#define IRQ_FLYBACK	0x03
#define IRQ_POR		0x04
#define IRQ_TIMER0	0x05
#define IRQ_TIMER1	0x06
/*#define IRQ_RESERVED1	0x07*/

/*#define IRQ_RESERVED2	0x08*/
/*#define IRQ_HD	0x09*/
/*#define IRQ_SERIAL	0x0A*/
#define IRQ_EXTENDED	0x0B
/*#define IRQ_FLOPPY	0x0C*/
#define IRQ_PODULE	0x0D
#define IRQ_KBDTX	0x0E
#define IRQ_KBDRX	0x0F

#define IRQ_DMACH0	0x10
#define IRQ_DMACH1	0x11
#define IRQ_DMACH2	0x12
#define IRQ_DMACH3	0x13
#define IRQ_DMASCH0	0x14
#define IRQ_DMASCH1	0x15
/*#define IRQ_RESERVED3	0x16*/
/*#define IRQ_RESERVED4	0x17*/



#endif	/* NIOMD > 0 */

#define IRQ_VSYNC	IRQ_FLYBACK	/* Aliased */
#define IRQ_NETSLOT	IRQ_EXTENDED

#define IRQ_INSTRUCT	-1
#define NIRQS		0x20

#include <machine/intr.h>

#ifndef _LOCORE
typedef struct irqhandler {
	int (*ih_func) __P((void *arg));/* handler function */
	void *ih_arg;			/* Argument to handler */
	int ih_level;			/* Interrupt level */
	int ih_num;			/* Interrupt number (for accounting) */
	const char *ih_name;		/* Name of interrupt (for vmstat -i) */
	u_int ih_flags;			/* Interrupt flags */
	u_int ih_maskaddr;		/* mask address for expansion cards */
	u_int ih_maskbits;		/* interrupt bit for expansion cards */
	struct irqhandler *ih_next;	/* next handler */
} irqhandler_t;

#ifdef _KERNEL
extern u_int irqmasks[IPL_LEVELS];
extern irqhandler_t *irqhandlers[NIRQS];

void irq_init __P((void));
int irq_claim __P((int, irqhandler_t *));
int irq_release __P((int, irqhandler_t *));
void *intr_claim __P((int irq, int level, const char *name, int (*func) __P((void *)), void *arg));
int intr_release __P((void *ih));
void irq_setmasks __P((void));
void disable_irq __P((int));
void enable_irq __P((int));
void stray_irqhandler __P((u_int));
#endif	/* _KERNEL */
#endif	/* _LOCORE */

#define IRQ_FLAG_ACTIVE 0x00000001	/* This is the active handler in list */

#endif	/* _ARM32_IRQHANDLER_H_ */

/* End of irqhandler.h */
