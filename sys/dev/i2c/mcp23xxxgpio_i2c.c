/*      $NetBSD: mcp23xxxgpio_i2c.c,v 1.2 2022/01/17 19:36:54 thorpej Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: mcp23xxxgpio_i2c.c,v 1.2 2022/01/17 19:36:54 thorpej Exp $");

/* 
 * Driver for Microchip serial I/O expanders:
 *
 *	MCP23008	8-bit, I2C interface
 *	MCP23017	16-bit, I2C interface
 *	MCP23018	16-bit (open-drain outputs), I2C interface
 *
 * Data sheet:
 *
 *	https://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf
 */

#include <sys/types.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/ic/mcp23xxxgpioreg.h>
#include <dev/ic/mcp23xxxgpiovar.h>

#include <dev/i2c/i2cvar.h>

struct mcpgpio_i2c_softc {
	struct mcpgpio_softc	sc_mcpgpio;

	i2c_tag_t		sc_i2c;
	i2c_addr_t		sc_addr;
};

#define	MCPGPIO_TO_I2C(sc)					\
	container_of((sc), struct mcpgpio_i2c_softc, sc_mcpgpio)

static const struct mcpgpio_variant mcp23008 = {
	.name = "MCP23008",
	.type = MCPGPIO_TYPE_23x08,
};

static const struct mcpgpio_variant mcp23017 = {
	.name = "MCP23017",
	.type = MCPGPIO_TYPE_23x17,
};

static const struct mcpgpio_variant mcp23018 = {
	.name = "MCP23018",
	.type = MCPGPIO_TYPE_23x18,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "microchip,mcp23008",	.data = &mcp23008 },
	{ .compat = "microchip,mcp23017",	.data = &mcp23017 },
	{ .compat = "microchip,mcp23018",	.data = &mcp23018 },
	DEVICE_COMPAT_EOL
};

static int
mcpgpio_i2c_lock(struct mcpgpio_softc *sc)
{
	struct mcpgpio_i2c_softc *isc = MCPGPIO_TO_I2C(sc);

	return iic_acquire_bus(isc->sc_i2c, 0);
}

static void
mcpgpio_i2c_unlock(struct mcpgpio_softc *sc)
{
	struct mcpgpio_i2c_softc *isc = MCPGPIO_TO_I2C(sc);

	iic_release_bus(isc->sc_i2c, 0);
}

static int
mcpgpio_i2c_read(struct mcpgpio_softc *sc, unsigned int bank,
    uint8_t reg, uint8_t *valp)
{
	struct mcpgpio_i2c_softc *isc = MCPGPIO_TO_I2C(sc);

	/* Only one chip per hardware address in i2c. */
	KASSERT((bank & ~1U) == 0);

	return iic_exec(isc->sc_i2c, I2C_OP_READ_WITH_STOP, isc->sc_addr,
	    &reg, 1, valp, 1, 0);
}

static int
mcpgpio_i2c_write(struct mcpgpio_softc *sc, unsigned int bank,
    uint8_t reg, uint8_t val)
{
	struct mcpgpio_i2c_softc *isc = MCPGPIO_TO_I2C(sc);

	/* Only one chip per hardware address in i2c. */
	KASSERT((bank & ~1U) == 0);

	return iic_exec(isc->sc_i2c, I2C_OP_WRITE_WITH_STOP, isc->sc_addr,
	    &reg, 1, &val, 1, 0);
}

static const struct mcpgpio_accessops mcpgpio_i2c_accessops = {
	.lock	=	mcpgpio_i2c_lock,
	.unlock	=	mcpgpio_i2c_unlock,
	.read	=	mcpgpio_i2c_read,
	.write	=	mcpgpio_i2c_write,
};

static int
mcpgpio_i2c_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result)) {
		return match_result;
	}
	return 0;
}

static void
mcpgpio_i2c_attach(device_t parent, device_t self, void *aux)
{
	struct mcpgpio_i2c_softc *isc = device_private(self);
	struct mcpgpio_softc *sc = &isc->sc_mcpgpio;
	struct i2c_attach_args *ia = aux;
	const struct device_compatible_entry *dce;

	isc->sc_i2c = ia->ia_tag;
	isc->sc_addr = ia->ia_addr;

	dce = iic_compatible_lookup(ia, compat_data);
	KASSERT(dce != NULL);

	sc->sc_dev = self;
	sc->sc_variant = dce->data;
	sc->sc_iocon = IOCON_SEQOP;
	sc->sc_accessops = &mcpgpio_i2c_accessops;

	aprint_naive("\n");	
	aprint_normal(": %s I/O Expander\n", sc->sc_variant->name);

	mcpgpio_attach(sc);
}

CFATTACH_DECL_NEW(mcpgpio_i2c, sizeof(struct mcpgpio_i2c_softc),
    mcpgpio_i2c_match, mcpgpio_i2c_attach, NULL, NULL);
