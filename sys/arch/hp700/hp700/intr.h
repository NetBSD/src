/*	$NetBSD: intr.h,v 1.9 2009/05/07 15:34:49 skrll Exp $	*/

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
 * The maximum number of bits in a cpl value/spl mask,
 * the maximum number of bits in an interrupt request register,
 * and the maximum number of interrupt registers.
 */
#define	HP700_INT_BITS	(32)
#define CPU_NINTS	HP700_INT_BITS		/* Use this one */

/*
 * This describes one HP700 interrupt register.
 */
struct hp700_int_reg {

	/*
	 * The device name for this interrupt register.
	 */
	const char *int_reg_dev;

	/*
	 * The virtual address of the mask, request and level
	 * registers.
	 */
	volatile int *int_reg_mask;
	volatile int *int_reg_req;
	volatile int *int_reg_level;

	/*
	 * This array has one entry for each bit in the 
	 * interrupt request register. If the 24 most 
	 * significant bits are set, the low 8
	 * bits are the index of the hp700_int_reg
	 * that this interrupt bit leads to, with zero 
	 * meaning that the interrupt bit is unused.
	 * Otherwise this bits correspond to the 
	 * hp700_int_bits. I.e. this bits are ored to 
	 * ipending_new in hp700_intr_ipending_new()
	 * when an interrupt happend.
	 *
	 * Note that this array is indexed by HP bit
	 * number, *not* by "normal" bit number.  In
	 * other words, the least significant bit in
	 * the interrupt register corresponds to array
	 * index 31.
	 */
	unsigned int int_reg_bits_map[HP700_INT_BITS];
#define	INT_REG_BIT_REG		0xffffff00
#define	INT_REG_BIT_UNUSED	INT_REG_BIT_REG

	/*
	 * The mask of allocatable bit numbers.
	 */
	int int_reg_allocatable_bits;
};

extern	struct hp700_int_reg int_reg_cpu;
void	hp700_intr_bootstrap(void);
void	hp700_intr_reg_establish(struct hp700_int_reg *);
void *	hp700_intr_establish(device_t, int, int (*)(void *), void *,
	    struct hp700_int_reg *, int);
int	hp700_intr_allocate_bit(struct hp700_int_reg *);
int	_hp700_intr_ipl_next(void);
int	_hp700_intr_spl_mask(void *);
void	hp700_intr_init(void);
void	hp700_intr_dispatch(int, int, struct trapframe *);
