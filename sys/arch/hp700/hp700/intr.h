/*	$NetBSD: intr.h,v 1.1 2002/06/06 19:48:06 fredette Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * This describes one HP700 interrupt register.
 */
struct hp700_int_reg {

	/*
	 * The device name for this interrupt register.
	 */
	const char *int_reg_dev;

	/*
	 * The virtual address of the mask and request 
	 * registers.
	 */
	volatile int *int_reg_mask;
	volatile int *int_reg_req;

	/*
	 * These registers are on a linked list.
	 */
	SLIST_ENTRY(hp700_int_reg) next;
	
	/*
	 * The mask of "frobbable" bits in this
	 * register.  This is filled in by the
	 * mask generating code.
	 */
	int int_reg_frobbable;
};

extern	struct hp700_int_reg int_reg_cpu;
void	hp700_intr_bootstrap __P((void));
void	hp700_intr_reg_establish __P((struct hp700_int_reg *));
void *	hp700_intr_establish __P((struct device *, int, int (*)(void *), void *,
				  struct hp700_int_reg *, int));
void	hp700_intr_init __P((void));
void	hp700_intr_dispatch __P((int, int, struct trapframe *));
