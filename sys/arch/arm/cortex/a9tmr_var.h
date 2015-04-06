/* $NetBSD: a9tmr_var.h,v 1.3.12.1 2015/04/06 15:17:52 skrll Exp $ */
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _ARM_CORTEX_A9TMR_VAR_
#define _ARM_CORTEX_A9TMR_VAR_

struct a9tmr_softc {
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	bus_space_handle_t sc_global_memh;
	bus_space_handle_t sc_private_memh;
	bus_space_handle_t sc_wdog_memh;
	struct evcnt sc_ev_missing_ticks;
	uint32_t sc_freq;
	u_long sc_autoinc;
	void *sc_global_ih;
};

#ifdef _KERNEL
struct cpu_info;
void	a9tmr_init_cpu_clock(struct cpu_info *);
void	a9tmr_update_freq(uint32_t);
void	a9tmr_delay(unsigned int n);
#endif

#endif /* _ARM_CORTEX_A9TMR_VAR_ */
