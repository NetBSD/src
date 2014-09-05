/*	$NetBSD: mct_var.h,v 1.3 2014/09/05 08:01:05 skrll Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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

#ifndef _ARM_SAMSUNG_MCT_VAR_H_
#define _ARM_SAMSUNG_MCT_VAR_H_

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

static struct mct_softc {
	device_t		 sc_dev;
	bus_space_tag_t		 sc_bst;
	bus_space_handle_t	 sc_bsh;
	uint32_t		 sc_irq;

	uint32_t		 sc_freq;
	void			*sc_global_ih;

	uint64_t		 sc_lastintr;
	uint32_t		 sc_autoinc;
	struct evcnt		 sc_ev_missing_ticks;

	/* blinking led */
	bool			 sc_has_blink_led;
	struct exynos_gpio_pindata sc_gpio_led;
	bool			 sc_led_state;
	int			 sc_led_timer;
} mct_sc;

void mct_init_cpu_clock(struct cpu_info *ci);

#endif /* _ARM_SAMSUNG_MCT_VAR_H_ */

