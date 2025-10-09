/* $NetBSD: imx23_timrotvar.h,v 1.1 2025/10/09 06:15:16 skrll Exp $ */

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger.
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

#ifndef _ARM_IMX23_TIMROT_VAR_H_
#define _ARM_IMX23_TIMROT_VAR_H_

#include "opt_arm_timer.h"
#ifdef __HAVE_GENERIC_CPU_INITCLOCKS
void	imx23timrot_cpu_initclocks(void);
#else
#define imx23timrot_cpu_initclocks	cpu_initclocks
#endif


/* Allocated for each timer instance. */
struct timrot_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
	int8_t sc_irq;
	int (*irq_handler)(void *);
	int freq;
};

int imx23timrot_systimer_init(struct timrot_softc*, bus_space_tag_t, int8_t);
int imx23timrot_stattimer_init(struct timrot_softc*, bus_space_tag_t, int8_t);
int imx23timrot_systimer_irq(void *frame);
int imx23timrot_stattimer_irq(void *);

#endif /* _ARM_IMX23_TIMROT_VAR_H_ */
