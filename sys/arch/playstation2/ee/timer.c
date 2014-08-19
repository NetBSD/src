/*	$NetBSD: timer.c,v 1.8.6.2 2014/08/20 00:03:18 tls Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: timer.c,v 1.8.6.2 2014/08/20 00:03:18 tls Exp $");

#include "debug_playstation2.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <playstation2/playstation2/interrupt.h>

#include <playstation2/ee/eevar.h>
#include <playstation2/ee/intcvar.h>
#include <playstation2/ee/timervar.h>
#include <playstation2/ee/timerreg.h>


#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

STATIC int timer0_intr(void *);

/*
 * EE timer usage
 *	0 ... 100 Hz clock interrupt.
 *      1 ... one shot interrupt for software interrupt for IPL_SOFT
 *	2 ... for IPL_SOFTCLOCK
 *	3 ... for IPL_SOFTNET, IPL_SOFTSERIAL
 */

void
timer_init(void)
{

	_reg_write_4(T0_MODE_REG, (T_MODE_EQUF | T_MODE_OVFF));
	_reg_write_4(T1_MODE_REG, (T_MODE_EQUF | T_MODE_OVFF));
	_reg_write_4(T2_MODE_REG, (T_MODE_EQUF | T_MODE_OVFF));
	_reg_write_4(T3_MODE_REG, (T_MODE_EQUF | T_MODE_OVFF));
}

void
timer_clock_init(void)
{
	/* clock interrupt (296.912MHz / 2 / 256) * 5760 = 100Hz */
	intc_intr_establish(I_CH9_TIMER0, IPL_CLOCK, timer0_intr, 0);
	_reg_write_4(T0_COUNT_REG, 0);
	_reg_write_4(T0_COMP_REG, 5760);
	_reg_write_4(T0_MODE_REG, T_MODE_CLKS_BUSCLK256 | T_MODE_ZRET |
	    T_MODE_CUE | T_MODE_CMPE);
}

void
timer_one_shot(int timer)
{
	KDASSERT(LEGAL_TIMER(timer) && timer != 0);

	_reg_write_4(T_COUNT_REG(timer), 0);
	_reg_write_4(T_COMP_REG(timer), 1);
	_reg_write_4(T_MODE_REG(timer), T_MODE_CUE | T_MODE_CMPE);
}

/* 
 * interrupt handler for clock interrupt (100Hz) 
 */
int
timer0_intr(void *arg)
{

	_reg_write_4(T0_MODE_REG, _reg_read_4(T0_MODE_REG) | T_MODE_EQUF);

	_playstation2_evcnt.clock.ev_count++;

	hardclock(&playstation2_clockframe);

	return (1);
}

/* one shot timer interrupt for software interrupt */
int
timer1_intr(void *arg)
{

	_reg_write_4(T1_MODE_REG, T_MODE_EQUF | T_MODE_OVFF);

#ifdef __HAVE_FAST_SOFTINTS
	softintr_dispatch(0); /* IPL_SOFT */
#endif

	return (1);
}

int
timer2_intr(void *arg)
{

	_reg_write_4(T2_MODE_REG, T_MODE_EQUF | T_MODE_OVFF);

#ifdef __HAVE_FAST_SOFTINTS
	softintr_dispatch(1); /* IPL_SOFTCLOCK */
#endif
	return (1);
}

int
timer3_intr(void *arg)
{

	_reg_write_4(T3_MODE_REG, T_MODE_EQUF | T_MODE_OVFF);

#ifdef __HAVE_FAST_SOFTINTS
	softintr_dispatch(3); /* IPL_SOFTSERIAL */
	softintr_dispatch(2); /* IPL_SOFTNET */
#endif

	return (1);
}
