/*	$NetBSD: timer_hb.c,v 1.12 2005/12/24 20:07:20 perry Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: timer_hb.c,v 1.12 2005/12/24 20:07:20 perry Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/clock_subr.h>

#include <news68k/news68k/isr.h>
#include <news68k/news68k/clockvar.h>

#include <news68k/dev/hbvar.h>

#include "ioconf.h"

/*
 * interrupt level for clock
 */

#define TIMER_LEVEL 6
#define TIMER_SIZE 8	/* XXX */

static int timer_hb_match(struct device *, struct cfdata  *, void *);
static void timer_hb_attach(struct device *, struct device *, void *);
static void timer_hb_initclocks(int, int);
void clock_intr(struct clockframe *);

static inline void leds_intr(void);

extern void _isr_clock(void);	/* locore.s */

CFATTACH_DECL(timer_hb, sizeof(struct device),
    timer_hb_match, timer_hb_attach, NULL, NULL);

static volatile uint8_t *ctrl_timer; /* XXX */

extern volatile u_char *ctrl_led; /* XXX */

static int
timer_hb_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hb_attach_args *ha = aux;
	static int timer_hb_matched;

	/* Only one timer, please. */
	if (timer_hb_matched)
		return 0;

	if (strcmp(ha->ha_name, timer_cd.cd_name))
		return 0;

	if (ha->ha_ipl == -1)
		ha->ha_ipl = TIMER_LEVEL;

	ha->ha_size = TIMER_SIZE;

	timer_hb_matched = 1;

	return 1;
}

static void
timer_hb_attach(struct device *parent, struct device *self, void *aux)
{
	struct hb_attach_args *ha = aux;

	if (ha->ha_ipl != TIMER_LEVEL)
		panic("clock_hb_attach: wrong interrupt level");

	ctrl_timer = (uint8_t *)IIOV(ha->ha_address); /* XXX needs bus_space */

	printf("\n");

	timer_config(timer_hb_initclocks);

#if 0 /* XXX this will be done in timer_hb_initclocks() */
	isrlink_custom(ha->ha_ipl, (void *)_isr_clock);
#endif
}

/*
 * Set up the interval timer (enable clock interrupts).
 * Leave stathz 0 since there is no secondary clock available.
 * Note that clock interrupts MUST STAY DISABLED until here.
 */
static void
timer_hb_initclocks(int prof, int stat)
{
	int s;

	s = splhigh();

	/* Install isr (in locore.s) that calls clock_intr(). */
	isrlink_custom(TIMER_LEVEL, (void *)_isr_clock);

	/* enable the clock */
	*ctrl_timer = 1;
	splx(s);
}

/*
 * Clock interrupt handler.
 * This is is called by the "custom" interrupt handler.
 *
 * from sun3/sun3x/clock.c -tsutsui
 */
void
clock_intr(struct clockframe *cf)
{
#ifdef	LED_IDLE_CHECK
	extern char _Idle[];	/* locore.s */
#endif

	/* Pulse the clock intr. enable low. */
	*ctrl_timer = 0;
	*ctrl_timer = 1;
	intrcnt[TIMER_LEVEL]++;

	/* Entertainment! */
#ifdef	LED_IDLE_CHECK
	if (cf.cf_pc == (long)_Idle)
#endif
		leds_intr();

	/* Call common clock interrupt handler. */
	hardclock(cf);
	uvmexp.intrs++;
}

/* heartbeat LED */
#define LED0	0x01
#define LED1	0x02

static inline void
leds_intr(void)
{
	static u_char led_countdown, led_stat;
	u_char i;

	if (led_countdown) {
		led_countdown--;
		return;
	}

	i = led_stat ^ LED0;
	*ctrl_led = i;
	led_stat = i;
	led_countdown = hz;
}

