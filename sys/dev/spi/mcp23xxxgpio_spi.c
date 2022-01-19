/*      $NetBSD: mcp23xxxgpio_spi.c,v 1.4 2022/01/19 05:21:44 thorpej Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcp23xxxgpio_spi.c,v 1.4 2022/01/19 05:21:44 thorpej Exp $");

/* 
 * Driver for Microchip serial I/O expanders:
 *
 *	MCP23S08	8-bit, SPI interface
 *	MCP23S17	16-bit, SPI interface
 *	MCP23S18	16-bit (open-drain outputs), SPI interface
 *
 * Data sheet:
 *
 *	https://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf
 */

#include <sys/types.h>
#include <sys/bitops.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/mutex.h>

#include <dev/ic/mcp23xxxgpioreg.h>
#include <dev/ic/mcp23xxxgpiovar.h>

#include <dev/spi/spivar.h>

/*
 * Multi-chip-on-select configurations appear to the upper layers like
 * additional GPIO banks; mixing different chip types on the same chip
 * select is not allowed.
 *
 * Some chips have 2 banks per chip, and we have up to 8 chips per chip
 * select, it's a total of 16 banks per chip select / driver instance.
 */
#define	MCPGPIO_SPI_MAXBANKS	16

struct mcpgpio_spi_softc {
	struct mcpgpio_softc	sc_mcpgpio;

	kmutex_t		sc_mutex;
	struct spi_handle      *sc_sh; 
	uint8_t			sc_ha[MCPGPIO_SPI_MAXBANKS];
};

/*
 * SPI-specific commands (the serial interface on the I2C flavor of
 * the chip uses the I2C protocol to infer this information).  Careful
 * readers will note that this ends up being exactly the same bits
 * on the serial interface that the I2C flavor of the chip uses.
 *
 * The SPI version can have up to 4 (or 8) chips per chip-select, demuxed
 * using the hardware address (selected by tying the 2 or 3 HA pins high/low
 * as desired).
 */
#define	OP_READ(ha)	(0x41 | ((ha) << 1))
#define	OP_WRITE(ha)	(0x40 | ((ha) << 1))

#define	MCPGPIO_TO_SPI(sc)					\
	container_of((sc), struct mcpgpio_spi_softc, sc_mcpgpio)

#if 0
static const struct mcpgpio_variant mcp23s08 = {
	.name = "MCP23S08",
	.type = MCPGPIO_TYPE_23x08,
};
#endif

static const struct mcpgpio_variant mcp23s17 = {
	.name = "MCP23S17",
	.type = MCPGPIO_TYPE_23x17,
};

#if 0
static const struct mcpgpio_variant mcp23s18 = {
	.name = "MCP23S18",
	.type = MCPGPIO_TYPE_23x18,
};
#endif

#if 0
static const struct device_compatible_entry compat_data[] = {
	{ .compat = "microchip,mcp23s08",	.data = &mcp23s08 },
	{ .compat = "microchip,mcp23s17",	.data = &mcp23s17 },
	{ .compat = "microchip,mcp23s18",	.data = &mcp23s18 },
	DEVICE_COMPAT_EOL
};
#endif

static int
mcpgpio_spi_lock(struct mcpgpio_softc *sc)
{
	struct mcpgpio_spi_softc *ssc = MCPGPIO_TO_SPI(sc);

	mutex_enter(&ssc->sc_mutex);
	return 0;
}

static void
mcpgpio_spi_unlock(struct mcpgpio_softc *sc)
{
	struct mcpgpio_spi_softc *ssc = MCPGPIO_TO_SPI(sc);

	mutex_exit(&ssc->sc_mutex);
}

static int
mcpgpio_spi_read(struct mcpgpio_softc *sc, unsigned int bank,
    uint8_t reg, uint8_t *valp)
{
	struct mcpgpio_spi_softc *ssc = MCPGPIO_TO_SPI(sc);
	uint8_t buf[2];

	KASSERT(bank < (sc->sc_npins >> 3));

	buf[0] = OP_READ(ssc->sc_ha[bank]);
	buf[1] = reg;

	return spi_send_recv(ssc->sc_sh, 2, buf, 1, valp);
}

static int
mcpgpio_spi_write(struct mcpgpio_softc *sc, unsigned int bank,
    uint8_t reg, uint8_t val)
{
	struct mcpgpio_spi_softc *ssc = MCPGPIO_TO_SPI(sc);
	uint8_t buf[3];

	KASSERT(bank < (sc->sc_npins >> 3));

	buf[0] = OP_WRITE(ssc->sc_ha[bank]);
	buf[1] = reg;
	buf[2] = val;

	return spi_send(ssc->sc_sh, 3, buf);
}

static const struct mcpgpio_accessops mcpgpio_spi_accessops = {
	.lock	=	mcpgpio_spi_lock,
	.unlock	=	mcpgpio_spi_unlock,
	.read	=	mcpgpio_spi_read,
	.write	=	mcpgpio_spi_write,
};

static int
mcpgpio_spi_match(device_t parent, cfdata_t cf, void *aux)
{

	/* MCP23S17 has no way to detect it! */

	return 1;
}

static void
mcpgpio_spi_attach(device_t parent, device_t self, void *aux)
{
	struct mcpgpio_spi_softc *ssc = device_private(self);
	struct mcpgpio_softc *sc = &ssc->sc_mcpgpio;
	struct spi_attach_args *sa = aux;
	uint32_t spi_present_mask;
	int bank, nchips, error, ha;

	mutex_init(&ssc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	ssc->sc_sh = sa->sa_handle;

	sc->sc_dev = self;
	sc->sc_variant = &mcp23s17;		/* XXX */
	sc->sc_iocon = IOCON_HAEN | IOCON_SEQOP;
	sc->sc_npins = MCP23x17_GPIO_NPINS;
	sc->sc_accessops = &mcpgpio_spi_accessops;

	aprint_naive("\n");	
	aprint_normal(": %s I/O Expander\n", sc->sc_variant->name);

	/* run at 10MHz */
	error = spi_configure(self, sa->sa_handle, SPI_MODE_0, 10000000);
	if (error) {
		return;
	}

	/*
	 * Before we decode the topology information, ensure each
	 * chip has IOCON.HAEN set so that it will actually decode
	 * the address bits.
	 *
	 * XXX Going on blind faith that IOCON.BANK is already 0.
	 */
	if (sc->sc_variant->type == MCPGPIO_TYPE_23x08) {
		error = mcpgpio_spi_write(sc, 0, REG_IOCON, sc->sc_iocon);
	} else {
		error = mcpgpio_spi_write(sc, 0, REGADDR_BANK0(0, REG_IOCON),
		    sc->sc_iocon);
		if (error == 0) {
			error = mcpgpio_spi_write(sc, 1,
			    REGADDR_BANK0(1, REG_IOCON), sc->sc_iocon);
		}
	}
	if (error) {
		aprint_error_dev(self,
		    "unable to initialize IOCON, error=%d\n", error);
		return;
	}

#if 0
	/*
	 * The number of devices sharing this chip select, along
	 * with their assigned addresses, is encoded in the
	 * "microchip,spi-present-mask" property.  Note that this
	 * device tree binding means that we will just have a
	 * single driver instance for however many chips are on
	 * this chip select.  We treat them logically as banks.
	 */
	if (of_getprop_uint32(phandle, "microchip,spi-present-mask",
			      &spi_present_mask) != 0 ||
	    of_getprop_uint32(phandle, "mcp,spi-present-mask",
			      &spi_present_mask) != 0) {
		aprint_error_dev(self,
		    "missing \"microchip,spi-present-mask\" property\n");
		return false;
	}
#else
	/*
	 * XXX Until we support decoding the DT properties that
	 * XXX give us the topology information.
	 */
	spi_present_mask = __BIT(device_cfdata(self)->cf_flags & 0x7);
#endif

	/*
	 * The 23S08 has 2 address pins (4 devices per chip select),
	 * and the others have 3 (8 devices per chip select).
	 */
	if (spi_present_mask == 0 ||
	    (sc->sc_variant->type == MCPGPIO_TYPE_23x08 &&
	     spi_present_mask >= __BIT(4)) ||
	    (sc->sc_variant->type != MCPGPIO_TYPE_23x08 &&
	     spi_present_mask >= __BIT(8))) {
		aprint_error_dev(self,
		    "invalid \"microchip,spi-present-mask\" value: 0x%08x\n",
		    spi_present_mask);
		return;
	}
	nchips = popcount32(spi_present_mask);
	sc->sc_npins = nchips *
	    (sc->sc_variant->type == MCPGPIO_TYPE_23x08 ? MCP23x08_GPIO_NPINS
							: MCP23x17_GPIO_NPINS);

	/* Record the hardware addresses for each logical bank of 8 pins. */
	for (bank = 0; spi_present_mask != 0; spi_present_mask &= ~__BIT(ha)) {
		int ha_first, ha_last;

		ha = ffs32(spi_present_mask) - 1;
		ha_first = bank * MCPGPIO_PINS_PER_BANK;
		ssc->sc_ha[bank++] = ha;
		if (sc->sc_variant->type != MCPGPIO_TYPE_23x08) {
			ssc->sc_ha[bank++] = ha;
		}
		ha_last = (bank * MCPGPIO_PINS_PER_BANK) - 1;
		aprint_verbose_dev(self, "pins %d..%d at HA %d\n",
		    ha_first, ha_last, ha);
	}
	KASSERT((bank * MCPGPIO_PINS_PER_BANK) == sc->sc_npins);

	mcpgpio_attach(sc);
}

CFATTACH_DECL_NEW(mcpgpio_spi, sizeof(struct mcpgpio_spi_softc),
    mcpgpio_spi_match, mcpgpio_spi_attach, NULL, NULL);
