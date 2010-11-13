/*	$NetBSD: intr.h,v 1.33 2010/11/13 14:07:07 uebayasi Exp $	*/

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

#ifndef _PREP_INTR_H_
#define _PREP_INTR_H_

#include <powerpc/intr.h>

#ifndef _LOCORE
#include <machine/cpu.h>

void init_intr_ivr(void);
void init_intr_openpic(void);
void openpic_init(unsigned char *);
void enable_intr(void);
void disable_intr(void);

extern vaddr_t prep_intr_reg;
extern uint32_t prep_intr_reg_off;

#define	ICU_LEN			32

#define	IRQ_SLAVE		2
#define	LEGAL_IRQ(x)		((x) >= 0 && (x) < ICU_LEN && (x) != IRQ_SLAVE)
#define	I8259_INTR_NUM		16
#define	OPENPIC_INTR_NUM	((ICU_LEN)-(I8259_INTR_NUM))

#define	PREP_INTR_REG	0xbffff000
#define	INTR_VECTOR_REG	0xff0

#endif /* !_LOCORE */

#endif /* !_PREP_INTR_H_ */
