/*	$NetBSD: dmacvar.h,v 1.4.6.2 2014/08/20 00:03:18 tls Exp $	*/

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
enum dmac_channel {
	D_CH0_VIF0 = 0,
	D_CH1_VIF1,
	D_CH2_GIF,
	D_CH3_FROMIPU,
	D_CH4_TOIPU,
	D_CH5_SIF0,
	D_CH6_SIF1,
	D_CH7_SIF2,
	D_CH8_FROMSPR,
	D_CH9_TOSPR
};

extern u_int32_t __dmac_enabled_channel;

void	dmac_init(void);

/* start/stop */
void	dmac_start_channel(enum dmac_channel);
void	dmac_stop_channel(enum dmac_channel);
void	dmac_sync_buffer(void);

/* interrupt */
int	dmac_intr(u_int32_t);
void	*dmac_intr_establish(enum dmac_channel, int, int (*)(void *), void *);
void	dmac_intr_disestablish(void *);
void	dmac_intr_disable(enum dmac_channel);
void	dmac_intr_enable(enum dmac_channel);
void	dmac_update_mask(u_int32_t);

/* polling */
void	dmac_cpc_set(enum dmac_channel);
void	dmac_cpc_clear(enum dmac_channel);
void	dmac_cpc_poll(void);
void	dmac_bus_poll(enum dmac_channel);	/* slow */

/* misc */
void	dmac_chcr_write(enum dmac_channel, u_int32_t);
