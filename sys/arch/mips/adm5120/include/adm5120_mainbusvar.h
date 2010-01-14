/* $NetBSD: adm5120_mainbusvar.h,v 1.1.62.1 2010/01/14 00:40:35 matt Exp $ */

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#ifndef _ADM5120_MAINBUSVAR_H_
#define _ADM5120_MAINBUSVAR_H_

#include <sys/types.h>
#include <sys/gpio.h>
#include <sys/ioctl.h>
#include <dev/gpio/gpiovar.h>

struct mainbus_attach_args {
	const char		*ma_name;	/* name of device */
	bus_dma_tag_t		ma_dmat;
	bus_space_tag_t		ma_obiot;
	bus_space_handle_t	ma_gpioh;
	void			*ma_gpio;
	int			ma_gpio_offset;
	uint32_t		ma_gpio_mask;
};

struct mainbus_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_obiot;
	bus_space_handle_t	sc_gpioh;
	struct gpio_chipset_tag	sc_gp;
	gpio_pin_t		sc_pins[8];
	device_t		sc_gpio;
};

device_t admgpio_attach(struct mainbus_softc *);

#endif /* _ADM5120_MAINBUSVAR_H_ */
