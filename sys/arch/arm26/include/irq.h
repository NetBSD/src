/* $NetBSD: irq.h,v 1.1 2000/05/09 21:55:59 bjh21 Exp $ */
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

/* return values from interrupt handlers */
/* These are the same as arm32 uses */
#define IRQ_HANDLED		1
#define IRQ_NOT_HANDLED		0
#define IRQ_MAYBE_HANDLED	-1

struct irq_handler;

extern void irq_init __P((void));
/* irq_handler is declared in machdep.h */
/* splx, raisespl and lowerspl are declared in spl.h */
extern struct irq_handler *irq_establish __P((int irqnum, int ipl,
					      int func(void *), void *arg));
extern char const *irq_string __P((struct irq_handler *h));
extern void irq_enable __P((struct irq_handler *h));
extern void irq_disable __P((struct irq_handler *h));
extern void irq_genmasks __P((void));
