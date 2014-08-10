/*	$NetBSD: bcm2835_gpioreg.h,v 1.2.8.1 2014/08/10 06:53:51 tls Exp $	*/

/*
 * Copyright (c) 2013 Jonathan A. Kollasch
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BROADCOM_BCM2835_GPIOREG_H_
#define _BROADCOM_BCM2835_GPIOREG_H_

#define BCM2835_GPIO_GPFSEL(x)	(0x000 + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPFSEL_PINS_PER_REGISTER	10
#define BCM2835_GPIO_GPFSEL_BITS_PER_PIN	3

#define BCM2835_GPIO_GPSET(x)	(0x01C + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPSET_PINS_PER_REGISTER 32
#define BCM2835_GPIO_GPCLR(x)	(0x028 + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPCLR_PINS_PER_REGISTER BCM2835_GPIO_GPSET_PINS_PER_REGISTER
#define BCM2835_GPIO_GPLEV(x)	(0x034 + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPLEV_PINS_PER_REGISTER 32
#define BCM2835_GPIO_GPEDS(x)	(0x040 + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPREN(x)	(0x04C + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPFEN(x)	(0x058 + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPHEN(x)	(0x064 + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPLEN(x)	(0x070 + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPAREN(x)	(0x07C + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPAFEN(x)	(0x088 + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPPUD	(0x094)
#define BCM2835_GPIO_GPPUD_PULLOFF  0x0
#define BCM2835_GPIO_GPPUD_PULLDOWN 0x1
#define BCM2835_GPIO_GPPUD_PULLUP   0x2
#define BCM2835_GPIO_GPPUDCLK(x)	(0x098 + (x) * sizeof(uint32_t))
#define BCM2835_GPIO_GPPUD_PINS_PER_REGISTER 32

#define BCM2835_GPIO_IN		00
#define BCM2835_GPIO_OUT	01
#define BCM2835_GPIO_ALT5	02
#define BCM2835_GPIO_ALT4	03
#define BCM2835_GPIO_ALT0	04
#define BCM2835_GPIO_ALT1	05
#define BCM2835_GPIO_ALT2	06
#define BCM2835_GPIO_ALT3	07

#endif /* _BROADCOM_BCM2835_GPIOREG_H_ */
