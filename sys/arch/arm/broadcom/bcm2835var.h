/*	$NetBSD: bcm2835var.h,v 1.3.2.1 2018/04/07 04:12:11 pgoyette Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#ifndef	_ARM_BROADCOM_BCM2835_VAR_H_
#define	_ARM_BROADCOM_BCM2835_VAR_H_

#include <sys/types.h>
#include <sys/bus.h>

extern struct arm32_bus_dma_tag bcm2835_bus_dma_tag;

extern bus_space_tag_t al_iot;
extern bus_space_handle_t al_ioh;

bus_dma_tag_t bcm2835_bus_dma_init(struct arm32_bus_dma_tag *);

void bcm2835_tmr_delay(unsigned int);

void bcm2836_cpu_hatch(struct cpu_info *);

u_int bcm283x_clk_get_rate_uart(void);
u_int bcm283x_clk_get_rate_vpu(void);
u_int bcm283x_clk_get_rate_emmc(void);

#endif	/* _ARM_BROADCOM_BCM2835_VAR_H_ */
