/*	$NetBSD: clock_hb.c,v 1.4 2000/06/29 08:02:51 mrg Exp $	*/

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

/*
 * Mostek clock chip found on the news68k.
 * based on mvme68k/dev/clock_pcc.c
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>

#include <news68k/news68k/isr.h>
#include <news68k/news68k/clockreg.h>
#include <news68k/news68k/clockvar.h>

#include <news68k/dev/hbvar.h>

int 	clock_hb_match __P((struct device *, struct cfdata  *, void *));
void	clock_hb_attach __P((struct device *, struct device *, void *));
void	clock_hb_initclocks __P((int, int));
void 	clock_intr __P((struct clockframe *));

static void leds_intr __P((void));

extern void _isr_clock __P((void));

struct cfattach clock_hb_ca = {
	sizeof(struct device), clock_hb_match, clock_hb_attach
};

extern volatile u_char *ctrl_timer, *ctrl_led;
extern struct cfdriver clock_cd;

int
clock_hb_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct hb_attach_args *ha = aux;
	static int clock_hb_matched;

	/* Only one clock, please. */
	if (clock_hb_matched)
		return (0);

	if (strcmp(ha->ha_name, clock_cd.cd_name))
		return (0);

	if (ha->ha_ipl == -1)
		ha->ha_ipl = CLOCK_LEVEL;

	clock_hb_matched = 1;

	return 1;
}

void
clock_hb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct hb_attach_args *ha = aux;
	caddr_t nvram, clockregs;

	if (ha->ha_ipl != CLOCK_LEVEL)
		panic("clock_hb_attach: wrong interrupt level");

	nvram = (caddr_t)(IIOV(ha->ha_address) - 0x7f8); /* XXX */
	clockregs = (caddr_t)IIOV(ha->ha_address);

        clock_config(self, clockregs, nvram, MK48T02_SIZE,
		clock_hb_initclocks);

#if 0 /* XXX this will be done in clock_hb_initclocks() */
	isrlink_custom(ha->ha_ipl, (void *)_isr_clock);
#endif
}

/*
 * Set up the real-time clock (enable clock interrupts).
 * Leave stathz 0 since there is no secondary clock available.
 * Note that clock interrupts MUST STAY DISABLED until here.
 */
void
clock_hb_initclocks(tick, statint)
	int tick, statint;
{
	int s;

	s = splhigh();

	/* Install isr (in locore.s) that calls clock_intr(). */
	isrlink_custom(CLOCK_LEVEL, (void *)_isr_clock);

	/* enable the clock */
	*ctrl_timer = 1;
	splx(s);
}

/*
 * Clock interrupt handler for Mostek.
 * This is is called by the "custom" interrupt handler.
 *
 * from sun3/sun3x/clock.c -tsutsui
 */
void
clock_intr(cf)
	struct clockframe *cf;
{
#ifdef	LED_IDLE_CHECK 
	extern char _Idle[];	/* locore.s */
#endif

	/* Pulse the clock intr. enable low. */
	*ctrl_timer = 0;
	*ctrl_timer = 1;
	intrcnt[CLOCK_LEVEL]++;

	{
	/* Entertainment! */
#ifdef	LED_IDLE_CHECK
	if (cf.cf_pc == (long)_Idle)
#endif
		leds_intr();
	}

	/* Call common clock interrupt handler. */
	hardclock(cf);
	uvmexp.intrs++;
}

/* heartbeat LED */
static u_char led_countdown = 0;
static u_char led_stat = 0;

#define LED0	0x01
#define LED1	0x02

static void
leds_intr()
{
	u_char i;

	if (led_countdown) {
		led_countdown--;
		return;
	}

	i = led_stat ^ LED0;
	*ctrl_led = i;
	led_stat = i;
	led_countdown = hz;

	return;
}
