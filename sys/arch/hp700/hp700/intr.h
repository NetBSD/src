/*	$NetBSD: intr.h,v 1.13.2.1 2011/06/06 09:05:40 jruoho Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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

/*
 * The maximum number of bits in a cpl value/spl mask, the maximum number of
 * bits in an interrupt request register, and the maximum number of interrupt
 * registers.
 */
#define	HP700_INTERRUPT_BITS	(32)
#define	CPU_NINTS		HP700_INTERRUPT_BITS		/* Use this one */

/*
 * This describes one HP700 interrupt register.
 */
struct hp700_interrupt_register {

	/*
	 * The device name for this interrupt register.
	 */
	const char *ir_name;

	/*
	 * The virtual address of the mask, request and level
	 * registers.
	 */
	volatile int *ir_mask;
	volatile int *ir_req;
	volatile int *ir_level;

	/*
	 * This array has one entry for each bit in the interrupt request
	 * register.
	 *
	 * If the 24 most significant bits are set, the low 8 bits are the
	 * index of the hp700_interrupt_register that this interrupt bit leads
	 * to, with zero meaning that the interrupt bit is unused.
	 *
	 * Otherwise these bits correspond to hp700_int_bits. That is, these
	 * bits are ORed to ipending_new in hp700_intr_ipending_new() when an
	 * interrupt happens.
	 *
	 * Note that this array is indexed by HP bit number, *not* by "normal"
	 * bit number.  In other words, the least significant bit in the inter-
	 * rupt register corresponds to array index 31.
	 */

	unsigned int ir_bits_map[HP700_INTERRUPT_BITS];

#define	IR_BIT_REG		0xffffff00
#define	IR_BIT_UNUSED		IR_BIT_REG
#define	IR_BIT_NESTED_P(x)	(((x) & ~IR_BIT_UNUSED) != 0)

	/*
	 * The mask of allocatable bit numbers.
	 */
	int ir_bits;
};

extern struct hp700_interrupt_register ir_cpu;

void	hp700_intr_bootstrap(void);
void	hp700_interrupt_register_establish(struct hp700_interrupt_register *);
void *	hp700_intr_establish(int, int (*)(void *), void *,
    struct hp700_interrupt_register *, int);
int	hp700_intr_allocate_bit(struct hp700_interrupt_register *);
int	_hp700_intr_ipl_next(void);
void	hp700_intr_init(void);
void	hp700_intr_dispatch(int, int, struct trapframe *);
