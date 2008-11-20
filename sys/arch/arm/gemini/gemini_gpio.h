/*	$NetBSD: gemini_gpio.h,v 1.1 2008/11/20 22:36:36 cliff Exp $	*/

/* adapted from omap_gpio.h */

/*
 * The OMAP GPIO Controller interface is inspired by pxa2x0_gpio.h
 *
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_GEMINI_GEMINI_GPIO_H_
#define _ARM_GEMINI_GEMINI_GPIO_H_

#define	GPIO_IN		0x00
#define	GPIO_OUT	0x01
#define	GPIO_IS_IN(n)	(((n) & GPIO_OUT) == GPIO_IN)
#define	GPIO_IS_OUT(n)	(((n) & GPIO_OUT) == GPIO_OUT)

/*
 * GPIO pin function query/manipulation functions
 */
extern u_int gemini_gpio_get_direction(u_int);
extern void  gemini_gpio_set_direction(u_int, u_int);

/*
 * Establish/Disestablish interrupt handlers for GPIO pins
 */
extern void *gemini_gpio_intr_establish(u_int, int, int,
     const char *, int (*)(void *), void *);
extern void gemini_gpio_intr_disestablish(void *);

/* Mask/unmask GPIO interrupt, enable/disable sleep wakeups */

extern void gemini_gpio_intr_mask(void *);
extern void gemini_gpio_intr_unmask(void *);
extern void gemini_gpio_intr_wakeup(void *cookie, int enable);

/*
 * Read/write
 */

extern u_int gemini_gpio_read(u_int gpio);
extern void gemini_gpio_write(u_int, u_int);

#endif /* _ARM_GEMINI_GEMINI_GPIO_H_ */
