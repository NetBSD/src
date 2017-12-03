/*	$NetBSD: omap2_nand.c,v 1.4.12.2 2017/12/03 11:35:55 jdolecek Exp $	*/

/*-
 * Copyright (c) 2010 Department of Software Engineering,
 *		      University of Szeged, Hungary
 * Copyright (c) 2010 Adam Hoka <ahoka@NetBSD.org>
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by the Department of Software Engineering, University of Szeged, Hungary
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Device driver for the NAND controller found in Texas Instruments OMAP2
 * and later SOCs.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap2_nand.c,v 1.4.12.2 2017/12/03 11:35:55 jdolecek Exp $");

#include "opt_omap.h"
#include "opt_flash.h"

/* TODO move to opt_* */
#undef OMAP2_NAND_HARDWARE_ECC

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cdefs.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <arch/arm/omap/omap2_gpmcvar.h>
#include <arch/arm/omap/omap2_gpmcreg.h>

#include <dev/nand/nand.h>
#include <dev/nand/onfi.h>

/* GPMC_STATUS */
#define WAIT0		__BIT(8)	/* active low */

/* GPMC_ECC_CONTROL */
#define ECCCLEAR	__BIT(8)
#define ECCPOINTER	__BITS(3,0)

/* GPMC_ECC_CONFIG */
#define ECCALGORITHM	__BIT(16)
#define ECCCS		__BITS(3,1)
#define ECC16B		__BIT(7)
#define ECCENABLE	__BIT(0)
/* GPMC_ECC_SIZE_CONFIG */
#define ECCSIZE1	__BITS(29,22)

/* GPMC_CONFIG1_i */
#define DEVICETYPE	__BITS(11,10)
#define DEVICESIZE	__BITS(13,12)

#define MASKEDINT(mask, integer) ((integer) << (ffs(mask) - 1) & mask)

/* NAND status register */
#define NAND_WP_BIT __BIT(4)

static int	omap2_nand_match(device_t, cfdata_t, void *);
static void	omap2_nand_attach(device_t, device_t, void *);
static int	omap2_nand_detach(device_t, int);

void omap2_nand_command(device_t self, uint8_t command);
void omap2_nand_address(device_t self, uint8_t address);
void omap2_nand_busy(device_t self);
void omap2_nand_read_1(device_t self, uint8_t *data);
void omap2_nand_write_1(device_t self, uint8_t data);
void omap2_nand_read_2(device_t self, uint16_t *data);
void omap2_nand_write_2(device_t self, uint16_t data);
bool omap2_nand_isbusy(device_t self);
void omap2_nand_read_buf_1(device_t self, void *buf, size_t len);
void omap2_nand_read_buf_2(device_t self, void *buf, size_t len);
void omap2_nand_write_buf_1(device_t self, const void *buf, size_t len);
void omap2_nand_write_buf_2(device_t self, const void *buf, size_t len);

int omap2_nand_ecc_init(device_t self);
int omap2_nand_ecc_prepare(device_t self, int mode);
int omap2_nand_ecc_compute(device_t self, const uint8_t *data, uint8_t *ecc);
int omap2_nand_ecc_correct(device_t self, uint8_t *data, const uint8_t *oldecc,
    const uint8_t *calcecc);

struct omap2_nand_softc {
	device_t sc_dev;
	device_t sc_nanddev;
	struct gpmc_softc *sc_gpmcsc;

	int sc_cs;
	int sc_buswidth;	/* 0: 8bit, 1: 16bit */

	struct nand_interface	sc_nand_if;

	bus_space_handle_t	sc_ioh;
	bus_space_tag_t		sc_iot;

	bus_size_t		sc_cmd_reg;
	bus_size_t		sc_addr_reg;
	bus_size_t		sc_data_reg;
};

CFATTACH_DECL_NEW(omapnand, sizeof(struct omap2_nand_softc), omap2_nand_match,
    omap2_nand_attach, omap2_nand_detach, NULL);

void
omap2_nand_command(device_t self, uint8_t command)
{
	struct omap2_nand_softc *sc = device_private(self);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->sc_cmd_reg, command);
};

void
omap2_nand_address(device_t self, uint8_t address)
{
	struct omap2_nand_softc *sc = device_private(self);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->sc_addr_reg, address);
};

bool
omap2_nand_isbusy(device_t self)
{
	struct omap2_nand_softc *sc = device_private(self);
	uint8_t status;

	DELAY(1);		/* just to be sure we are not early */

	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
	    sc->sc_cmd_reg, ONFI_READ_STATUS);

	DELAY(1);

	status = bus_space_read_1(sc->sc_iot,
	    sc->sc_ioh, sc->sc_data_reg);

	return !(status & ONFI_STATUS_RDY);
};

static int
omap2_nand_match(device_t parent, cfdata_t match, void *aux)
{
	struct gpmc_attach_args *gpmc = aux;
	bus_space_tag_t	iot;
	bus_space_handle_t ioh;
	bus_size_t cs_offset;
	uint32_t result;
	int ret = 0;

	iot = gpmc->gpmc_iot;

	cs_offset = GPMC_CS_CONFIG_BASE(gpmc->gpmc_cs);

	/* map i/o space */
	if (bus_space_map(iot, cs_offset, GPMC_CS_SIZE, 0, &ioh) != 0) {
		aprint_error("omap2_nand_match: can't map i/o space");
		return 1;
	}

	/* read GPMC_CONFIG1_i */
	result = bus_space_read_4(iot, ioh, GPMC_CONFIG1_i);

	/* check if memory device is NAND type */
	if ((result & DEVICETYPE) == MASKEDINT(DEVICETYPE, 0x02)) {
		/* we got NAND, report positive match */
		ret = 1;
	}

	bus_space_unmap(iot, ioh, GPMC_CS_SIZE);

	return ret;
}

static void
omap2_nand_attach(device_t parent, device_t self, void *aux)
{
	struct omap2_nand_softc *sc = device_private(self);
	sc->sc_gpmcsc = device_private(parent);
	struct gpmc_attach_args *gpmc = aux;
	bus_size_t cs_offset;
	uint32_t val;

	aprint_normal("\n");

	sc->sc_iot = gpmc->gpmc_iot;
	sc->sc_dev = self;
	sc->sc_cs = gpmc->gpmc_cs;

	cs_offset = GPMC_CS_CONFIG_BASE(sc->sc_cs);

	/* map i/o space */
	if (bus_space_map(sc->sc_iot, cs_offset, GPMC_CS_SIZE, 0,
		&sc->sc_ioh) != 0) {
		aprint_error(": omap2_nand_attach: can't map i/o space");
		return;
	}

        sc->sc_cmd_reg = GPMC_NAND_COMMAND_0 - GPMC_CONFIG1_0;
	sc->sc_addr_reg = GPMC_NAND_ADDRESS_0 - GPMC_CONFIG1_0;
	sc->sc_data_reg = GPMC_NAND_DATA_0 - GPMC_CONFIG1_0;

	/* turn off write protection if enabled */
	val = gpmc_register_read(sc->sc_gpmcsc, GPMC_CONFIG);
	val |= NAND_WP_BIT;
	gpmc_register_write(sc->sc_gpmcsc, GPMC_CONFIG, val);

	/*
	 * do the reset dance for NAND
	 */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
	    sc->sc_cmd_reg, ONFI_RESET);

	omap2_nand_busy(self);

	/* read GPMC_CONFIG1_i to get buswidth */
	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GPMC_CONFIG1_i);

	if ((val & DEVICESIZE) == MASKEDINT(DEVICESIZE, 0x01)) {
		/* 16bit */
		sc->sc_buswidth = 1;
	} else if ((val & DEVICESIZE) == MASKEDINT(DEVICESIZE, 0x00)) {
		/* 8bit */
		sc->sc_buswidth = 0;
	} else {
		panic("invalid buswidth reported by config1");
	}

	nand_init_interface(&sc->sc_nand_if);

	sc->sc_nand_if.command = &omap2_nand_command;
	sc->sc_nand_if.address = &omap2_nand_address;
	sc->sc_nand_if.read_buf_1 = &omap2_nand_read_buf_1;
	sc->sc_nand_if.read_buf_2 = &omap2_nand_read_buf_2;
	sc->sc_nand_if.read_1 = &omap2_nand_read_1;
	sc->sc_nand_if.read_2 = &omap2_nand_read_2;
	sc->sc_nand_if.write_buf_1 = &omap2_nand_write_buf_1;
	sc->sc_nand_if.write_buf_2 = &omap2_nand_write_buf_2;
	sc->sc_nand_if.write_1 = &omap2_nand_write_1;
	sc->sc_nand_if.write_2 = &omap2_nand_write_2;
	sc->sc_nand_if.busy = &omap2_nand_busy;

#ifdef OMAP2_NAND_HARDWARE_ECC
	omap2_nand_ecc_init(self);
	sc->sc_nand_if.ecc_compute = &omap2_nand_ecc_compute;
	sc->sc_nand_if.ecc_correct = &omap2_nand_ecc_correct;
	sc->sc_nand_if.ecc_prepare = &omap2_nand_ecc_prepare;
	sc->sc_nand_if.ecc.necc_code_size = 3;
	sc->sc_nand_if.ecc.necc_block_size = 512;
	sc->sc_nand_if.ecc.necc_type = NAND_ECC_TYPE_HW;
#else
	sc->sc_nand_if.ecc.necc_code_size = 3;
	sc->sc_nand_if.ecc.necc_block_size = 256;
#endif	/* OMAP2_NAND_HARDWARE_ECC */

	if (!pmf_device_register1(sc->sc_dev, NULL, NULL, NULL))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

	sc->sc_nanddev = nand_attach_mi(&sc->sc_nand_if, sc->sc_dev);
}

static int
omap2_nand_detach(device_t device, int flags)
{
	struct omap2_nand_softc *sc = device_private(device);
	int ret = 0;

	bus_space_unmap(sc->sc_iot, sc->sc_ioh, GPMC_CS_SIZE);

	pmf_device_deregister(sc->sc_dev);

	if (sc->sc_nanddev != NULL)
		ret = config_detach(sc->sc_nanddev, flags);

	return ret;
}

void
omap2_nand_busy(device_t self)
{
	struct omap2_nand_softc *sc = device_private(self);

	while (!(gpmc_register_read(sc->sc_gpmcsc, GPMC_STATUS) & WAIT0)) {
		DELAY(1);
	}
}

void
omap2_nand_read_1(device_t self, uint8_t *data)
{
	struct omap2_nand_softc *sc = device_private(self);

	*data = bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->sc_data_reg);
}

void
omap2_nand_write_1(device_t self, uint8_t data)
{
	struct omap2_nand_softc *sc = device_private(self);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->sc_data_reg, data);
}

void
omap2_nand_read_2(device_t self, uint16_t *data)
{
	struct omap2_nand_softc *sc = device_private(self);

	*data = bus_space_read_2(sc->sc_iot, sc->sc_ioh, sc->sc_data_reg);
}

void
omap2_nand_write_2(device_t self, uint16_t data)
{
	struct omap2_nand_softc *sc = device_private(self);

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, sc->sc_data_reg, data);
}

void
omap2_nand_read_buf_1(device_t self, void *buf, size_t len)
{
	struct omap2_nand_softc *sc = device_private(self);

	KASSERT(buf != NULL);
	KASSERT(len >= 1);

	bus_space_read_multi_1(sc->sc_iot, sc->sc_ioh,
	    sc->sc_data_reg, buf, len);
}

void
omap2_nand_read_buf_2(device_t self, void *buf, size_t len)
{
	struct omap2_nand_softc *sc = device_private(self);

	KASSERT(buf != NULL);
	KASSERT(len >= 2);
	KASSERT(!(len & 0x01));

	bus_space_read_multi_2(sc->sc_iot, sc->sc_ioh,
	    sc->sc_data_reg, buf, len / 2);
}

void
omap2_nand_write_buf_1(device_t self, const void *buf, size_t len)
{
	struct omap2_nand_softc *sc = device_private(self);

	KASSERT(buf != NULL);
	KASSERT(len >= 1);

	bus_space_write_multi_1(sc->sc_iot, sc->sc_ioh,
	    sc->sc_data_reg, buf, len);
}

void
omap2_nand_write_buf_2(device_t self, const void *buf, size_t len)
{
	struct omap2_nand_softc *sc = device_private(self);

	KASSERT(buf != NULL);
	KASSERT(len >= 2);
	KASSERT(!(len & 0x01));

	bus_space_write_multi_2(sc->sc_iot, sc->sc_ioh,
	    sc->sc_data_reg, buf, len / 2);
}

static uint32_t
convert_ecc(const uint8_t *ecc)
{
	return ecc[0] | (ecc[1] << 16) | ((ecc[2] & 0xf0) << 20) |
	    ((ecc[2] & 0x0f) << 8);
}

int
omap2_nand_ecc_init(device_t self)
{
	struct omap2_nand_softc *sc = device_private(self);
	uint32_t val;

	val = gpmc_register_read(sc->sc_gpmcsc, GPMC_ECC_CONTROL);
	/* clear ecc, select ecc register 1 */
	val &= ~ECCPOINTER;
	val |= ECCCLEAR | MASKEDINT(ECCPOINTER, 1);
	gpmc_register_write(sc->sc_gpmcsc, GPMC_ECC_CONTROL, val);

	/* XXX too many MAGIC */
	/* set ecc size to 512, set all regs to eccsize1*/
	val = gpmc_register_read(sc->sc_gpmcsc, GPMC_ECC_SIZE_CONFIG);
	val &= ~ECCSIZE1;
	val |= MASKEDINT(ECCSIZE1, 512) | 0x0f;
	gpmc_register_write(sc->sc_gpmcsc, GPMC_ECC_CONTROL, val);

	return 0;
}

int
omap2_nand_ecc_compute(device_t self, const uint8_t *data, uint8_t *ecc)
{
	struct omap2_nand_softc *sc = device_private(self);
	uint32_t val;

	/* read ecc result register */
	val = gpmc_register_read(sc->sc_gpmcsc, GPMC_ECC1_RESULT);

	ecc[0] = val & 0xff;
	ecc[1] = (val >> 16) & 0xff;
	ecc[2] = ((val >> 8) & 0x0f) | ((val >> 20) & 0xf0);

	/* disable ecc engine */
	val = gpmc_register_read(sc->sc_gpmcsc, GPMC_ECC_CONFIG);
	val &= ~ECCENABLE;
	gpmc_register_write(sc->sc_gpmcsc, GPMC_ECC_CONFIG, val);

	return 0;
}

int
omap2_nand_ecc_prepare(device_t self, int mode)
{
	struct omap2_nand_softc *sc = device_private(self);
	uint32_t val;

	/* same for read/write */
	switch (mode) {
	case NAND_ECC_READ:
	case NAND_ECC_WRITE:
		val = gpmc_register_read(sc->sc_gpmcsc, GPMC_ECC_CONTROL);
		/* clear ecc, select ecc register 1 */
		val &= ~ECCPOINTER;
		val |= ECCCLEAR | MASKEDINT(ECCPOINTER, 1);
		gpmc_register_write(sc->sc_gpmcsc, GPMC_ECC_CONTROL, val);

		val = gpmc_register_read(sc->sc_gpmcsc, GPMC_ECC_CONFIG);
		val &= ~ECCCS;
		val |= ECCENABLE | MASKEDINT(ECCCS, sc->sc_cs);
		if (sc->sc_buswidth == 1)
			val |= ECC16B;
		else
			val &= ~ECC16B;
		gpmc_register_write(sc->sc_gpmcsc, GPMC_ECC_CONFIG, val);

		break;
	default:
		aprint_error_dev(self, "invalid i/o mode for ecc prepare\n");
		return -1;
	}

	return 0;
}

int
omap2_nand_ecc_correct(device_t self, uint8_t *data, const uint8_t *oldecc,
    const uint8_t *calcecc)
{
	uint32_t oecc, cecc, xor;
	uint16_t parity, offset;
	uint8_t bit;

	oecc = convert_ecc(oldecc);
	cecc = convert_ecc(calcecc);

	/* get the difference */
	xor = oecc ^ cecc;

	/* the data was correct if all bits are zero */
	if (xor == 0x00)
		return NAND_ECC_OK;

	switch (popcount32(xor)) {
	case 12:
		/* single byte error */
		parity = xor >> 16;
		bit = (parity & 0x07);
		offset = (parity >> 3) & 0x01ff;
		/* correct bit */
		data[offset] ^= (0x01 << bit);
		return NAND_ECC_CORRECTED;
	case 1:
		return NAND_ECC_INVALID;
	default:
		/* erased page! */
		if ((oecc == 0x0fff0fff) && (cecc == 0x00000000))
			return NAND_ECC_OK;

		return NAND_ECC_TWOBIT;
	}
}
