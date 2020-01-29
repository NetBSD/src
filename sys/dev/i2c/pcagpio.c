/* $NetBSD: pcagpio.c,v 1.1 2020/01/29 05:27:05 macallan Exp $ */

/*-
 * Copyright (c) 2018 Michael Lorenz
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

/*
 * a driver for Philips Semiconductor PCA9555 GPIO controllers
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcagpio.c,v 1.1 2020/01/29 05:27:05 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>

/* commands */
#define PCAGPIO_INPUT	0x00	/* line status */
#define PCAGPIO_OUTPUT	0x01	/* output status */
#define PCAGPIO_REVERT	0x02	/* revert input if set */
#define PCAGPIO_CONFIG	0x03	/* input if set, output if not */

static int	pcagpio_match(device_t, cfdata_t, void *);
static void	pcagpio_attach(device_t, device_t, void *);

struct pcagpio_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	int		sc_is_16bit;
};


static void 	pcagpio_writereg(struct pcagpio_softc *, int, uint32_t);
static uint32_t pcagpio_readreg(struct pcagpio_softc *, int);

CFATTACH_DECL_NEW(pcagpio, sizeof(struct pcagpio_softc),
    pcagpio_match, pcagpio_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ "i2c-pca9555",	1 },
	{ "pca9555",		1 },
	{ "i2c-pca9556",	0 },
	{ "pca9556",		0 },
	{ NULL,			0 }
};

static int
pcagpio_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	return 0;
}

static void
printdir(uint32_t val, uint32_t mask, char letter)
{
	char flags[17], bits[17];
	uint32_t bit = 0x8000;
	int i;

	val &= mask;
	for (i = 0; i < 16; i++) {
		flags[i] = (mask & bit) ? letter : '-';
		bits[i] = (val & bit) ? 'X' : ' ';
		bit = bit >> 1;
	}
	flags[16] = 0;
	bits[16] = 0;
	printf("dir: %s\n", flags);
	printf("lvl: %s\n", bits);
}	

static void
pcagpio_attach(device_t parent, device_t self, void *aux)
{
	struct pcagpio_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	const struct device_compatible_entry *dce;
	uint32_t dir, in, out;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");
	sc->sc_is_16bit = 0;
	if (iic_compatible_match(ia, compat_data, &dce))
		sc->sc_is_16bit = dce->data;

	aprint_normal(": %s\n", sc->sc_is_16bit ? "PCA9555" : "PCA9556");

	if (sc->sc_addr == 0x38) pcagpio_writereg(sc, 1, 0xff & ~0x10);
	
	dir = pcagpio_readreg(sc, 3);
	in = pcagpio_readreg(sc, 0);
	out = pcagpio_readreg(sc, 1);

	out &= ~dir;
	in &= dir;
	
	printdir(in, dir, 'I');
	printdir(out, ~dir, 'O');
}

static void
pcagpio_writereg(struct pcagpio_softc *sc, int reg, uint32_t val)
{
	uint8_t cmd;

	iic_acquire_bus(sc->sc_i2c, 0);
	if (sc->sc_is_16bit) {
		uint16_t creg;
		cmd = reg << 1;
		creg = htole16(val);
		iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &creg, 2, 0);
	} else {
		uint8_t creg;
		cmd = reg;
		creg = (uint8_t)val;
		iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &creg, 1, 0);
	}
	iic_release_bus(sc->sc_i2c, 0);
}		

static uint32_t pcagpio_readreg(struct pcagpio_softc *sc, int reg)
{
	uint8_t cmd;
	uint32_t ret;

	iic_acquire_bus(sc->sc_i2c, 0);
	if (sc->sc_is_16bit) {
		uint16_t creg;
		cmd = reg << 1;
		iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &creg, 2, 0);
		ret = le16toh(creg);
	} else {
		uint8_t creg;
		cmd = reg;
		iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &creg, 1, 0);
		ret = creg;
	}
	iic_release_bus(sc->sc_i2c, 0);
	return ret;
}
