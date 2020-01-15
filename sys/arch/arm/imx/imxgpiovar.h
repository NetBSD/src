/*	$NetBSD: imxgpiovar.h,v 1.3 2020/01/15 01:09:56 jmcneill Exp $ */
/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas
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
#ifndef _ARM_IMX_IMXGPIOVAR_H
#define _ARM_IMX_IMXGPIOVAR_H

#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#include <arm/imx/imxgpioreg.h>	/* for GPIO_NPINS */

struct imxgpio_softc {
	struct pic_softc gpio_pic;

	device_t gpio_dev;
	bus_space_tag_t gpio_memt;
	bus_space_handle_t gpio_memh;

	void *gpio_is;
	void *gpio_is_high;

	int gpio_unit;
	int gpio_irqbase;
	uint32_t gpio_enable_mask;
	uint32_t gpio_edge_mask;
	uint32_t gpio_level_mask;
#if NGPIO > 0
	struct gpio_chipset_tag gpio_chipset;
	gpio_pin_t gpio_pins[32];
#endif

	kmutex_t gpio_lock;
};

struct imxgpio_pin {
	int pin_no;
	u_int pin_flags;
	bool pin_actlo;
};

void imxgpio_attach_common(device_t);
/* defined imx[35]1_gpio.c */
extern const int imxgpio_ngroups;
int imxgpio_match(device_t, cfdata_t, void *);
void imxgpio_attach(device_t, device_t, void *);

int imxgpio_pin_read(void *, int);
void imxgpio_pin_write(void *, int, int);
void imxgpio_pin_ctl(void *, int, int);

/* in-kernel GPIO access utility functions */
void imxgpio_set_direction(u_int, int);
void imxgpio_data_write(u_int, u_int);
bool imxgpio_data_read(u_int);

#define	GPIO_NO(group, pin)	(((group) - 1) * GPIO_NPINS + (pin))
#define	GPIO_MODULE(pin)	((pin) / GPIO_NPINS)

#endif	/* _ARM_IMX_IMXGPIOVAR_H */
