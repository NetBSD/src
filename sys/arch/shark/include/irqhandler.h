/*	$NetBSD: irqhandler.h,v 1.3.34.1 2008/01/08 22:10:23 bouyer Exp $	*/

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

#ifndef _LOCORE
#include <sys/types.h>
#include <sys/evcnt.h>
#endif /* _LOCORE */

/* Define the IRQ bits */

/*
 * XXX this is really getting rather horrible.
 * Shortly to be replaced with system specific interrupt tables and handling
 */

#ifdef  OFWGENCFG
/* These are just made up for now!  -JJK */
#define IRQ_TIMER0      0
#endif

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
#endif

#if defined(SHARK) || defined(OFWGENCFG)
#define IRQ_RTC         0x08  /* hardwired to the sequoia RTC */
#endif
#ifdef SHARK
#define IRQ_CODEC1      0x09
#define IRQ_UMI1        0x0A  /* isa or pci */
#define IRQ_UMI2        0x0B  /* isa or pci */

#define IRQ_MOUSE       0x0C
#define IRQ_FERR        0x0D  /* FERR# pin on sequoia needs to be connected */
#define IRQ_IDE         0x0E  /* hardwired to the IDE connector */
#define IRQ_CODEC2      0x0F  /* special interrupt on codec */
#endif /* SHARK */

#define IRQ_VSYNC	IRQ_FLYBACK	/* Aliased */
#define IRQ_NETSLOT	IRQ_EXTENDED

#define IRQ_INSTRUCT	-1
#define NIRQS		0x20

#include <machine/intr.h>

#ifndef _LOCORE
typedef struct irqhandler {
	int (*ih_func)(void *arg);	/* handler function */
	void *ih_arg;			/* Argument to handler */
	int ih_level;			/* Interrupt level */
	int ih_num;			/* Interrupt number (for accounting) */
	u_int ih_flags;			/* Interrupt flags */
	u_int ih_maskaddr;		/* mask address for expansion cards */
	u_int ih_maskbits;		/* interrupt bit for expansion cards */
	struct irqhandler *ih_next;	/* next handler */
	struct evcnt ih_ev;		/* evcnt structure */
	int (*ih_realfunc)(void *arg);	/* XXX real handler function */
	void *ih_realarg;
} irqhandler_t;

#ifdef _KERNEL
extern u_int irqmasks[IPL_LEVELS];
extern irqhandler_t *irqhandlers[NIRQS];

void irq_init(void);
int irq_claim(int, irqhandler_t *, const char *group, const char *name);
int irq_release(int, irqhandler_t *);
void *intr_claim(int irq, int level, int (*func)(void *), void *arg,
	const char *group, const char *name);
int intr_release(void *ih);
void irq_setmasks(void);
void disable_irq(int);
void enable_irq(int);
#endif	/* _KERNEL */
#endif	/* _LOCORE */

#define IRQ_FLAG_ACTIVE 0x00000001	/* This is the active handler in list */

#endif	/* _ARM32_IRQHANDLER_H_ */

/* End of irqhandler.h */
