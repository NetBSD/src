/*	$NetBSD: machdep.h,v 1.7 2009/05/08 09:33:58 skrll Exp $	*/

/*	$OpenBSD: cpufunc.h,v 1.17 2000/05/15 17:22:40 mickey Exp $	*/

/*
 * Copyright (c) 1998,2000 Michael Shalayeff
 * All rights reserved.
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
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  (c) Copyright 1988 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */
/*
 * Copyright (c) 1990,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: c_support.s 1.8 94/12/14$
 *	Author: Bob Wheeler, University of Utah CSL
 */

/*
 * Definitions for the hp700 that are completely private
 * to the machine-dependent code.  Anything needed by
 * machine-independent code is covered in cpu.h or in
 * other headers.
 */

/*
 * XXX there is a lot of stuff in various headers under
 * hp700/include and hppa/include, and a lot of one-off
 * `extern's in C files that could probably be moved here.
 */

#ifdef _KERNEL

#ifdef _KERNEL_OPT
#include "opt_useleds.h"
#endif

#include <hppa/hppa/machdep.h>

/* The LEDs. */
#define	HP700_LED_NETSND	(0)
#define	HP700_LED_NETRCV	(1)
#define	HP700_LED_DISK		(2)
#define	HP700_LED_HEARTBEAT	(3)
#define	_HP700_LEDS_BLINKABLE	(4)
#define	_HP700_LEDS_COUNT	(8)

/* This forcefully reboots the machine. */
void cpu_die(void);

/* These map and unmap page zero. */
int hp700_pagezero_map(void);
void hp700_pagezero_unmap(int);

/* Blinking the LEDs. */
#ifdef USELEDS
#define	_HP700_LED_FREQ		(16)
extern volatile u_int8_t *machine_ledaddr;
extern int machine_ledword, machine_leds;
extern int _hp700_led_on_cycles[];
#define hp700_led_blink(i)			\
do {						\
	if (_hp700_led_on_cycles[i] == -1)	\
		_hp700_led_on_cycles[i] = 1;	\
} while (/* CONSTCOND */ 0)
#define hp700_led_on(i, ms)			\
do {						\
	_hp700_led_on_cycles[i] = (((ms) * _HP700_LED_FREQ) / 1000); \
} while (/* CONSTCOND */ 0)
void hp700_led_ctl(int, int, int);
#else  /* !USELEDS */
#define hp700_led_blink(i)
#define hp700_led_on(i, ms)
#define hp700_led_ctl(off, on, toggle)
#endif /* !USELEDS */

#endif /* _KERNEL */
