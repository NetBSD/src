/*	$NetBSD: leds.c,v 1.9.60.1 2019/06/10 22:06:51 christos Exp $	*/

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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: leds.c,v 1.9.60.1 2019/06/10 22:06:51 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <machine/sid.h>
#include <machine/leds.h>

#include "opt_cputype.h"

static volatile u_short *diagreg = 0;
static u_char led_countdown = 0;
static u_char led_px = 0;

/*
 * Initial value is the default pattern set.
 */
struct led_patterns ledpat = {
	16,	/* divisor */
	12,	/* patlen */
	{	/* patterns */
		0x03, 0x06, 0x0c, 0x18,
		0x30, 0x60, 0xc0, 0x60,
		0x30, 0x18, 0x0c, 0x06,
	}
};

void
ledsattach(int a)
{
	switch (vax_boardtype) {
#if VAX46 || VAXANY
	case VAX_BTYP_46: {
		extern struct vs_cpu *ka46_cpu;
		diagreg = (volatile u_short *)(&ka46_cpu->vc_diagdsp);
		break;
		}
#endif
#if VAX48 || VAXANY
	case VAX_BTYP_48: {
		extern struct vs_cpu *ka48_cpu;
		diagreg = (volatile u_short *)(&ka48_cpu->vc_diagdsp);
		break;
		}
#endif
#if VAX49 || VAXANY
	case VAX_BTYP_49:
		diagreg = (volatile u_short *)vax_map_physmem(0x25800000, 1);
		diagreg += 2;
		break;
#endif
	default:
		return;
	}

	/* Turn on some lights. */
	leds_intr();
}

/*
 * This is called by the clock interrupt.
 */
void
leds_intr(void)
{
	register u_char i;

	if (diagreg == 0)
		return;

	if (led_countdown) {
		led_countdown--;
		return;
	}

	led_countdown = ledpat.divisor - 1;
	i = led_px;

	*diagreg = ~ledpat.pat[i];

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
	char *va;

	off = uio->uio_offset;
	if ((off < 0) || (off > sizeof(ledpat)))
		return (EIO);

	cnt = uimin(uio->uio_resid, (sizeof(ledpat) - off));
	if (cnt == 0)
		return (0); /* EOF */


	va = (char *)&ledpat;
	va = va + off;
	error = uiomove(va, cnt, uio);

	return (error);
}

