/*	$NetBSD: pxa2x0_gpio.h,v 1.5 2009/08/04 12:11:33 kiyohara Exp $	*/

/*
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

#ifndef _PXA2X0_GPIO_H
#define _PXA2X0_GPIO_H

/*
 * If you want to fiddle with GPIO registers before the
 * driver has been autoconfigured (e.g. from initarm()),
 * call this function with the virtual address of the
 * GPIO controller's registers
 */
extern void pxa2x0_gpio_bootstrap(vaddr_t);

/*
 * GPIO pin function query/manipulation functions
 */
extern u_int pxa2x0_gpio_get_function(u_int);
extern u_int pxa2x0_gpio_set_function(u_int, u_int);
extern int pxa2x0_gpio_get_bit(u_int);
extern void pxa2x0_gpio_set_bit(u_int);
extern void pxa2x0_gpio_clear_bit(u_int);
extern void pxa2x0_gpio_set_dir(u_int, int);
extern void pxa2x0_gpio_clear_intr(u_int);

/*
 * Establish/Disestablish interrupt handlers for GPIO pins
 */
extern void *pxa2x0_gpio_intr_establish(u_int, int, int,
		int (*)(void *), void *);
extern void pxa2x0_gpio_intr_disestablish(void *);
extern void pxa2x0_gpio_intr_mask(void *);
extern void pxa2x0_gpio_intr_unmask(void *);
extern void pxa2x0_gpio_set_intr_level(u_int, int);


struct pxa2x0_gpioconf {
	int pin;
	u_int value;
};
void pxa2x0_gpio_config(struct pxa2x0_gpioconf **);

extern struct pxa2x0_gpioconf pxa25x_com_ffuart_gpioconf[];
extern struct pxa2x0_gpioconf pxa25x_com_stuart_gpioconf[];
extern struct pxa2x0_gpioconf pxa25x_com_btuart_gpioconf[];
extern struct pxa2x0_gpioconf pxa25x_com_hwuart_gpioconf[];
extern struct pxa2x0_gpioconf pxa25x_i2c_gpioconf[];
extern struct pxa2x0_gpioconf pxa25x_i2s_gpioconf[];
extern struct pxa2x0_gpioconf pxa25x_pcic_gpioconf[];
extern struct pxa2x0_gpioconf pxa25x_pxaacu_gpioconf[];
extern struct pxa2x0_gpioconf pxa25x_pxamci_gpioconf[];

extern struct pxa2x0_gpioconf pxa27x_com_ffuart_gpioconf[];
extern struct pxa2x0_gpioconf pxa27x_com_stuart_gpioconf[];
extern struct pxa2x0_gpioconf pxa27x_com_btuart_gpioconf[];
extern struct pxa2x0_gpioconf pxa27x_i2c_gpioconf[];
extern struct pxa2x0_gpioconf pxa27x_i2s_gpioconf[];
extern struct pxa2x0_gpioconf pxa27x_ohci_gpioconf[];
extern struct pxa2x0_gpioconf pxa27x_pcic_gpioconf[];
extern struct pxa2x0_gpioconf pxa27x_pxaacu_gpioconf[];
extern struct pxa2x0_gpioconf pxa27x_pxamci_gpioconf[];

#endif /* _PXA2X0_GPIO_H */
