/*	$NetBSD: irqhandler.h,v 1.1 2001/04/19 07:11:02 matt Exp $	*/

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

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_cputypes.h"
#endif

#ifndef _LOCORE
#include <sys/types.h>
#endif /* _LOCORE */

/* Define the IRQ bits */

/*
 * XXX this is really getting rather horrible.
 * Shortly to be replaced with system specific interrupt tables and handling
 */

#if defined(RISCPC) || defined(CPU_ARM7500)

#ifdef CPU_ARM7500

/*#define IRQ_PRINTER	0x00*/
#define IRQ_RESERVED0	0x01
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
#define IRQ_RESERVED2	0x1D
#define IRQ_RESERVED3	0x1E

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

#define IRQ_RESERVED1	0x07
#define IRQ_EXTENDED	0x0B
#define IRQ_PODULE	0x0D

#define IRQ_EXPCARD0	0x20
#define IRQ_EXPCARD1	0x21
#define IRQ_EXPCARD2	0x22
#define IRQ_EXPCARD3	0x23
#define IRQ_EXPCARD4	0x24
#define IRQ_EXPCARD5	0x25
#define IRQ_EXPCARD6	0x26
#define IRQ_EXPCARD7	0x27

#endif	/* RC7500 */

#else	/* CPU_ARM7500 */

#ifdef	RISCPC
/*#define IRQ_PRINTER	0x00*/
#define IRQ_RESERVED0	0x01
/*#define IRQ_FLOPPYIDX	0x02*/
#define IRQ_FLYBACK	0x03
#define IRQ_POR		0x04
#define IRQ_TIMER0	0x05
#define IRQ_TIMER1	0x06
#define IRQ_RESERVED1	0x07

#define IRQ_RESERVED2	0x08
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
#define IRQ_RESERVED3	0x16
#define IRQ_RESERVED4	0x17

#define IRQ_EXPCARD0	0x18
#define IRQ_EXPCARD1	0x19
#define IRQ_EXPCARD2	0x1A
#define IRQ_EXPCARD3	0x1B
#define IRQ_EXPCARD4	0x1C
#define IRQ_EXPCARD5	0x1D
#define IRQ_EXPCARD6	0x1E
#define IRQ_EXPCARD7	0x1F
#endif	/* RISCPC */

#endif	/* CPU_ARM7500 */

#endif	/* RISPC || CPU_ARM7500 */

#ifdef  OFWGENCFG
/* These are just made up for now!  -JJK */
#define IRQ_TIMER0      0
#endif

/* XXX why is this in ARM7500? */
#ifdef SHARK
/*
 * shark hardware requirements for IRQ's:
 *	IDE:		14		(hardwired)
 *	PCI:		5, 9, 10, 11, 15(mapped to UMIPCI inta, intb, intc, intd)
 *	UMIISA:		10, 11, 12
 *	SuperIO:	1, 3..12, 14, 15(all may be remapped. defaults as follows.)
 *	KBC:		1
 *	USI:		3		(UART with Slow Infrared support)
 *	UART:		4
 *	FLOPPY:		6		(not currently used on shark)
 *	PARALLEL:	7
 *	RTC:		8		(not used on shark: RTC in sequoia used)
 *	MOUSE:		12
 *	Sequoia:	8		(internal RTC hardwired to irq 8)
 *	Codec:		5, 7, 9, 10, 15	(irqe, connected to 15, has special status.)
 *	CS8900:		5, 10, 11, 12	(P.14 of datasheet sez only 1 used/time)
 *	FERR#:		13		(unconnected floating point error)
 *
 * total of 15 irqs:
 * timer, ide, 2 umi = isa/pci, ethernet, 2 codec, kb, usi, uart, floppy, 
 * parallel, rtc, mouse, ferr (irq 13)
 *
 * eventually, need to read the OFW dev info tree, and allocate IRQs.
 * hardcoded for now.
 */
#define IRQ_TIMER0	0x00  /* hardwired to 8254 counter 0 in sequoia */
#define IRQ_KEYBOARD    0x01
#define IRQ_CASCADE     0x02  /* hardwired IRQ for second 8259 = IRQ_SLAVE */
#define IRQ_USI         0x03
#define IRQ_UART        0x04
#define IRQ_ETHERNET    0x05
#define IRQ_FLOPPY      0x06
#define IRQ_PARALLEL    0x07

#define IRQ_RTC         0x08  /* hardwired to the sequoia RTC */
#define IRQ_CODEC1      0x09
#define IRQ_UMI1        0x0A  /* isa or pci */
#define IRQ_UMI2        0x0B  /* isa or pci */

#define IRQ_MOUSE       0x0C
#define IRQ_FERR        0x0D  /* FERR# pin on sequoia needs to be connected */
#define IRQ_IDE         0x0E  /* hardwired to the IDE connector */
#define IRQ_CODEC2      0x0F  /* special interrupt on codec */

/* XXX should this go into isa_machdep.h.  Somewhere else? */
/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#endif /* SHARK */

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
#endif	/* _KERNEL */
#endif	/* _LOCORE */

#define IRQ_FLAG_ACTIVE 0x00000001	/* This is the active handler in list */

#ifndef _LOCORE
typedef struct fiqhandler {
	void (*fh_func) __P((void));/* handler function */
	u_int fh_size;		/* Size of handler function */
	u_int fh_mask;		/* FIQ mask */
	u_int fh_r8;		/* FIQ mode r8 */
	u_int fh_r9;		/* FIQ mode r9 */
	u_int fh_r10;		/* FIQ mode r10 */
	u_int fh_r11;		/* FIQ mode r11 */
	u_int fh_r12;		/* FIQ mode r12 */
	u_int fh_r13;		/* FIQ mode r13 */
} fiqhandler_t;

#ifdef _KERNEL
int fiq_claim __P((fiqhandler_t *));
int fiq_release __P((fiqhandler_t *));
#endif	/* _KERNEL */
#endif	/* _LOCORE */

#endif	/* _ARM32_IRQHANDLER_H_ */

/* End of irqhandler.h */
