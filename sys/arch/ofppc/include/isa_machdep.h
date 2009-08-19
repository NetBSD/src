/*	$NetBSD: isa_machdep.h,v 1.7 2009/08/19 15:01:46 dyoung Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _OFPPC_ISA_MACHDEP_H_
#define _OFPPC_ISA_MACHDEP_H_

#include <powerpc/isa_machdep.h>

extern struct powerpc_bus_dma_tag isa_bus_dma_tag;
extern struct pic_ops *isa_pic;
extern int isa_pcmciamask;

/* prototypes from isa_machdep.c */
int map_isa_ioregs(void);
uint8_t isa_inb(uint32_t);
void isa_outb(uint32_t, uint8_t);

/* function mappings */
#define isa_attach_hook(p, s, iaa)					\
	genppc_isa_attach_hook(p, s, iaa)
#define isa_detach_hook(c, s)						\
	genppc_isa_detach_hook(c, s)
#define isa_intr_evcnt(ic, irq)						\
	genppc_isa_intr_evcnt(ic, irq)
#define isa_intr_establish(ic, irq, type, level, fun, arg)		\
	genppc_isa_intr_establish(ic, irq, type, level, fun, arg)
#define isa_intr_disestablish(ic, arg)					\
	genppc_isa_intr_disestablish(ic, arg)
#define isa_intr_alloc(ic, mask, type, irqp)				\
	genppc_isa_intr_alloc(ic, isa_pic, mask & isa_pcmciamask, type, irqp)

/*
 * Miscellanous functions.
 */
void isabeep(int, int);		/* beep with the system speaker */

#endif /* _OFPPC_ISA_MACHDEP_H_ */
