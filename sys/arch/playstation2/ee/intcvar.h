/*	$NetBSD: intcvar.h,v 1.4.6.2 2014/08/20 00:03:18 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/* DO NOT REORDER */
enum intc_channel {
	I_CH0_GS = 0,
	I_CH1_SBUS,
	I_CH2_VB_ON,
	I_CH3_VB_OFF,
	I_CH4_VIF0,
	I_CH5_VIF1,
	I_CH6_VU0,
	I_CH7_VU1,
	I_CH8_IPU,
	I_CH9_TIMER0,
	I_CH10_TIMER1,
	I_CH11_TIMER2,
	I_CH12_TIMER3,
	I_CH13_SFIFO,
	I_CH14_VU0WD,
	I_CH15_PGPU
};

extern u_int32_t __intc_enabled_channel;

void	intc_init(void);

int	intc_intr(u_int32_t);

void	*intc_intr_establish(enum intc_channel, int, int (*)(void *), void *);
void	intc_intr_disestablish(void *);

void	intc_intr_disable(enum intc_channel);
void	intc_intr_enable(enum intc_channel);
void	intc_update_mask(u_int32_t);
