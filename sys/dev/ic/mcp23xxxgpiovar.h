/*      $NetBSD: mcp23xxxgpiovar.h,v 1.1 2022/01/17 16:31:23 thorpej Exp $	*/

/*-
 * Copyright (c) 2014, 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank Kardel, and by Jason R. Thorpe.
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

#ifndef _DEV_IC_MCP23xxxGPIOVAR_H_
#define	_DEV_IC_MCP23xxxGPIOVAR_H_

/* 
 * Driver for Microchip serial I/O expansers:
 *
 *	MCP23008	8-bit, I2C interface
 *	MCP23S08	8-bit, SPI interface
 *	MCP23017	16-bit, I2C interface
 *	MCP23S17	16-bit, SPI interface
 *	MCP23018	16-bit (open-drain outputs), I2C interface
 *	MCP23S18	16-bit (open-drain outputs), SPI interface
 *
 * Data sheet:
 *
 *	https://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf
 */

#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

struct mcpgpio_softc;

typedef enum {
	MCPGPIO_TYPE_23x08	= 0,
	MCPGPIO_TYPE_23x17	= 1,
	MCPGPIO_TYPE_23x18	= 2,
} mcpgpio_type;

struct mcpgpio_variant {
	const char		*name;
	mcpgpio_type		type;
};

struct mcpgpio_accessops {
	int			(*lock)(struct mcpgpio_softc *);
	void			(*unlock)(struct mcpgpio_softc *);
	int			(*read)(struct mcpgpio_softc *,
				    unsigned int, uint8_t, uint8_t *);
	int			(*write)(struct mcpgpio_softc *,
				    unsigned int, uint8_t, uint8_t);
};

struct mcpgpio_softc {
	device_t                sc_dev;
	const struct mcpgpio_variant *sc_variant;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		*sc_gpio_pins;
	unsigned int		sc_npins;
	int			sc_phandle;
	uint8_t			sc_iocon; /* I/O configuration */

	/* Bus-specific access functions. */
	const struct mcpgpio_accessops *sc_accessops;
};

void	mcpgpio_attach(struct mcpgpio_softc *);

#endif /* _DEV_IC_MCP23xxxGPIOVAR_H_ */
