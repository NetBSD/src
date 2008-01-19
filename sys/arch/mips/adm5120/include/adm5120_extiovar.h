/* $NetBSD: adm5120_extiovar.h,v 1.1.32.1 2008/01/19 12:14:23 bouyer Exp $ */

/*-
 * Copyright (c) 2007 David Young.  All rights reserved.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
#ifndef _ADM5120_EXTIOVAR_H_
#define _ADM5120_EXTIOVAR_H_

#include <sys/device.h>
#include <sys/types.h>
#include <sys/gpio.h>
#include <sys/ioctl.h>
#include <dev/gpio/gpiovar.h>

struct extio_attach_args {
	const char	*ea_name;	/* name of device */
	bus_space_tag_t	ea_st;
	bus_addr_t	ea_addr;	/* address of device */
	int		ea_irq;		/* interrupt bit # */
	void		*ea_gpio;
	int		ea_gpio_offset;
	uint32_t	ea_gpio_mask;
	int		ea_cfio;
};

struct extio_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_obiot;
	bus_space_handle_t	sc_gpioh;
	bus_space_handle_t	sc_mpmch;
	device_t		sc_gpio;
	int			sc_map[5];
	struct gpio_pinmap	sc_pm;
	struct mips_bus_space	sc_cfio;
};

void cfio_bus_mem_init(bus_space_tag_t, bus_space_tag_t);
void extio_bus_mem_init(bus_space_tag_t, void *);

#endif /* _ADM5120_EXTIOVAR_H_ */
