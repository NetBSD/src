/* $NetBSD: irq.h,v 1.2.2.4 2000/12/13 15:49:19 bouyer Exp $ */
/*-
 * Copyright (c) 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */

#ifndef _ARM26_IRQ_H
#define _ARM26_IRQ_H
#include <arch/arm26/iobus/iocreg.h>

/* return values from interrupt handlers */
/* These are the same as arm32 uses */
#define IRQ_HANDLED		1
#define IRQ_NOT_HANDLED		0
#define IRQ_MAYBE_HANDLED	-1

/*
 * These definitions specify how the devices are wired to the IOC
 * interrupt lines.
 */
/* All systems */
#define IRQ_PFIQ	IOC_IRQ_IL0	/* Podule FIQ request */
#define	IRQ_SIRQ	IOC_IRQ_IL1	/* Sound buffer pointer used */
#define IRQ_PIRQ	IOC_IRQ_IL5	/* Podule IRQ request */
#define IRQ_VFLYBK	IOC_IRQ_IR	/* Start of display vertical flyback */
/* Archimedes systems */
#define IRQ_SLCI	IOC_IRQ_IL2	/* Serial line controller interrupt */
#define IRQ_WIRQ	IOC_IRQ_IL3	/* Winchester interrupt request */
#define IRQ_DCIRQ	IOC_IRQ_IL4	/* Disc change interrupt request */
#define IRQ_PBSY	IOC_IRQ_IL6	/* Printer Busy Input */
#define IRQ_RII		IOC_IRQ_IL7	/* Serial line ring indicator input */
#define IRQ_PACK	IOC_IRQ_IF	/* Printer acknowledge input */
/* IOEB systems */
#define IRQ_SINTR	IOC_IRQ_IL2	/* Serial line interrupt */
#define IRQ_IDEINTR	IOC_IRQ_IL3	/* IDE interrupt */
#define IRQ_FINTR	IOC_IRQ_IL4	/* Floppy disc interrupt */
#define IRQ_LPINTR	IOC_IRQ_IL6	/* Parallel port latched interrupt */
#define IRQ_INDEX	IOC_IRQ_IF	/* Start of floppy disc index pulse */

/* IRQ numbers above 15 are non-IOC IRQs */
#define IRQ_UNIXBP_BASE	16

struct irq_handler;

extern void irq_init __P((void));
/* irq_handler is declared in machdep.h */
/* splx, raisespl and lowerspl are declared in intr.h */
extern struct irq_handler *irq_establish __P((int irqnum, int ipl,
					      int func(void *), void *arg));
extern char const *irq_string __P((struct irq_handler *h));
extern void irq_enable __P((struct irq_handler *h));
extern void irq_disable __P((struct irq_handler *h));
extern void irq_genmasks __P((void));
#endif
