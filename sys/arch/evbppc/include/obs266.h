/*	$NetBSD: obs266.h,v 1.1.10.2 2005/09/14 20:53:59 tron Exp $	*/

/*
 * Copyright 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima for The NetBSD Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef	_EVBPPC_OBS266_H_
#define	_EVBPPC_OBS266_H_

#include <machine/obs405.h>

/*
 * Device Properties for OpenBlockS266 (IBM405GPr 266MHz)
 */

/* UART Clock */
#define OBS266_COM_FREQ		(COM_FREQ * 4)	/* UART CLK 7.3728 MHz */

/* OpenBlockS266 GPIO LED */
#define OBS266_LED1		(1)
#define OBS266_LED2		(2)
#define OBS266_LED4		(4)
#define OBS266_LED_ON		(OBS266_LED1 | OBS266_LED2 | OBS266_LED4)
#define OBS266_LED_OFF		(~OBS266_LED1 & ~OBS266_LED2 & ~OBS266_LED4)

#define OBS266_GPIO_LED1	(12)
#define OBS266_GPIO_LED2	(13)
#define OBS266_GPIO_LED4	(14)

/*
 * extern variables and functions
 */
extern void obs266_led_set(int led);

#endif	/* _EVBPPC_OBS266_H_ */
