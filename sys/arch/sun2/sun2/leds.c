/*	$NetBSD: leds.c,v 1.1 2001/04/06 15:05:56 fredette Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross and der Mouse.
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
 * Functions to flash the LEDs with some pattern.
 * All other Sun2 machines have an 8-position LED
 * array in which some pattern is animated.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <machine/autoconf.h>
#include <machine/idprom.h>
#include <machine/leds.h>

#include <sun2/sun2/machdep.h>
#include <sun2/sun2/control.h>

static u_char led_countdown = 0;
static u_char led_px = 0;

/*
 * Initial value is the default pattern set.
 */
static struct led_patterns ledpat = {
	8,	/* divisor */
	8,	/* patlen */
	{	/* patterns */
		0x0F, 0x1E, 0x3C, 0x78,
		0xF0, 0xE1, 0xC3, 0x87,
	}
};

/*
 * This is called early during startup to find the
 * diag register (LEDs) and turn on the light(s).
 */
void
leds_init()
{

	/* Turn on some lights. */
	leds_intr();
}

/*
 * This is called by the clock interrupt.
 */
void
leds_intr()
{
	register u_char i;

	if (led_countdown) {
		led_countdown--;
		return;
	}

	led_countdown = ledpat.divisor - 1;
	i = led_px;

	set_control_byte(DIAG_REG, ledpat.pat[i]);

	i = i+1;
	if (i == ledpat.patlen)
		i = 0;
	led_px = i;
}

/*
 * This is called by mem.c to handle /dev/leds
 */
int
leds_uio(struct uio *uio)
{
	int cnt, error;
	int off;	/* NOT off_t */
	caddr_t va;

	off = uio->uio_offset;
	if ((off < 0) || (off > sizeof(ledpat)))
		return (EIO);

	cnt = min(uio->uio_resid, (sizeof(ledpat) - off));
	if (cnt == 0)
		return (0); /* EOF */

	va = ((char*)(&ledpat)) + off;
	error = uiomove(va, cnt, uio);

	return (error);
}
