/* $NetBSD: machdep.h,v 1.2.78.1 2007/04/19 01:04:19 thorpej Exp $ */
/*-
 * Copyright (c) 1998 Ben Harris
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
/*
 * machdep.h - random exported functions that have to be declared somewhere.
 */

#ifndef _ARM26_MACHDEP_H
#define _ARM26_MACHDEP_H

struct bootconfig;

/* start.c */
extern void start __P((struct bootconfig *initbootconfig));

/* except.c */
extern void undefined_handler(struct trapframe *);
extern void swi_handler(struct trapframe *);
extern void data_abort_handler(struct trapframe *);
extern void prefetch_abort_handler(struct trapframe *);
extern void address_exception_handler(struct trapframe *);

/* irq.c */
extern void irq_handler	__P((struct irqframe *irqf));

/* locore.S */
extern register_t set_r13_irq	__P((register_t r13_irq));
extern void int_on	__P((void));
extern void int_off	__P((void));
extern void fiq_on	__P((void));
extern void fiq_off	__P((void));
extern void cpu_loswitch(struct switchframe **, struct switchframe *);
extern void proc_trampoline(void); /* not quite true */

/* pmap.c */
extern register_t update_memc	__P((register_t, register_t));

/* rtc.c */
extern int cmos_read(int);
extern int cmos_write(int, int);

#endif
