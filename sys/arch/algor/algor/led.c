/*	$NetBSD: led.c,v 1.4 2003/07/14 22:57:47 lukem Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: led.c,v 1.4 2003/07/14 22:57:47 lukem Exp $");

#include "opt_algor_p4032.h"
#include "opt_algor_p5064.h" 
#include "opt_algor_p6032.h"

#include <sys/param.h>

#include <machine/autoconf.h>

#ifdef ALGOR_P4032
#include <algor/algor/algor_p4032reg.h>
#endif

#ifdef ALGOR_P5064
#include <algor/algor/algor_p5064reg.h>
#endif 
 
#ifdef ALGOR_P6032
#include <algor/algor/algor_p6032reg.h>
#endif

#if defined(ALGOR_P4032)
#define	LEDBASE		MIPS_PHYS_TO_KSEG1(P4032_LED)
#define	LED(x)		((3 - (x)) * 4)
#elif defined(ALGOR_P5064)
#define	LEDBASE		MIPS_PHYS_TO_KSEG1(P5064_LED1)
#define	LED(x)		((3 - (x)) * 4)
#elif defined(ALGOR_P6032)
#define	HD2532_STRIDE		4
#define	HD2532_NFLASH_OFFSET	0x80
#define	HD2532_CRAM	(HD2532_NFLASH_OFFSET + (0x18 * HD2532_STRIDE))
#define	LEDBASE		MIPS_PHYS_TO_KSEG1(P6032_HDSP2532_BASE + HD2532_CRAM)
#define	LED(x)		((x) * HD2532_STRIDE)
#endif

/*
 * led_display:
 *
 *	Set the LED display to the characters provided.
 */
void
led_display(u_int8_t a, u_int8_t b, u_int8_t c, u_int8_t d)
{
	u_int8_t *leds = (u_int8_t *) LEDBASE;

	leds[LED(0)] = a;
	leds[LED(1)] = b;
	leds[LED(2)] = c;
	leds[LED(3)] = d;
#if defined(ALGOR_P6032)	/* XXX Should support these */
	leds[LED(4)] = ' ';
	leds[LED(5)] = ' ';
	leds[LED(6)] = ' ';
	leds[LED(7)] = ' ';
#endif
}
