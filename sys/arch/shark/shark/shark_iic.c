/*	$NetBSD: shark_iic.c,v 1.2 2021/08/07 16:19:05 thorpej Exp $	*/

/*
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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

__KERNEL_RCSID(0, "$NetBSD: shark_iic.c,v 1.2 2021/08/07 16:19:05 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>

#include <shark/shark/sequoia.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

#include <arm/cpufunc.h>

/* define registers on sequoia used by pins  */
#define	SEQUOIA_1GPIO		PMC_GPCR_REG		/* reg 0x007 gpio 0-3 */
#define	SEQUOIA_2GPIO		SEQ2_OGPIOCR_REG	/* reg 0x304 gpio 4.8 */

/* define pins on sequoia that talk to the DIMM I2C bus */
#define	IIC_CLK			FOMPCR_M_PCON9
#define	IIC_DATA_IN_DIR		GPCR_M_GPIODIR3
#define	IIC_DATA_IN		GPCR_M_GPIODATA3
#define	IIC_DATA_OUT_DIR	GPIOCR2_M_GPIOBDIR1

/* logical i2c signals */
#define	SHARK_IIC_BIT_SDA	0x01
#define	SHARK_IIC_BIT_SCL	0x02
#define	SHARK_IIC_BIT_OUTPUT	0x04
#define	SHARK_IIC_BIT_INPUT	0x08

struct sharkiic_softc {
	device_t		sc_dev;
	struct i2c_controller	sc_i2c;
};

/* I2C bitbanging */

static void
sequoiaBBInit(void)
{
	uint16_t	seqReg;

	sequoiaLock();

	/*
	 * SCL initialization
	 * - enable pc[9]
	 * - set pin to low (0)
	 */
	sequoiaRead(SEQR_SEQPSR3_REG, &seqReg);
	CLR(seqReg, SEQPSR3_M_PC9PINEN);
	sequoiaWrite(SEQR_SEQPSR3_REG, seqReg);
	sequoiaRead(PMC_FOMPCR_REG, &seqReg);
	SET(seqReg, IIC_CLK);
	sequoiaWrite(PMC_FOMPCR_REG, seqReg);

	/*
	 * SDA (Output) initialization
	 * - set direction to output
	 * - enable GPIO B1 (sets pin to low)
	 */
	sequoiaRead(PMC_GPIOCR2_REG, &seqReg);
	SET(seqReg, IIC_DATA_OUT_DIR);
	sequoiaWrite(PMC_GPIOCR2_REG, seqReg);
	sequoiaRead(SEQR_SEQPSR2_REG, &seqReg);
	SET(seqReg, SEQPSR2_M_GPIOB1PINEN);
	sequoiaWrite(SEQR_SEQPSR2_REG, seqReg);

	/*
	 * SDA (Input) initialization
	 * - enable GPIO
	 * - set direction to input
	 */
	sequoiaRead(SEQ2_SEQ2PSR_REG, &seqReg);
	CLR(seqReg, SEQ2PSR_M_GPIOPINEN);
	sequoiaWrite(SEQ2_SEQ2PSR_REG, seqReg);
	sequoiaRead(SEQUOIA_1GPIO, &seqReg);
	CLR(seqReg, IIC_DATA_IN_DIR);
	sequoiaWrite(SEQUOIA_1GPIO, seqReg);

	sequoiaUnlock();
}

static void
sequoiaBBSetBits(uint32_t bits)
{
	uint16_t	seqRegSDA, seqRegSCL;

	sequoiaLock();

	sequoiaRead(PMC_FOMPCR_REG, &seqRegSCL);
	sequoiaRead(SEQR_SEQPSR2_REG, &seqRegSDA);

	/*
	 * For SCL and SDA:
	 * - output is the inverse of the desired signal
	 * - the pin enable bit drives the signal
	 */
	if (bits & SHARK_IIC_BIT_SCL) {
		CLR(seqRegSCL, IIC_CLK);
	} else {
		SET(seqRegSCL, IIC_CLK);
	}

	if (bits & SHARK_IIC_BIT_SDA) {
		CLR(seqRegSDA, SEQPSR2_M_GPIOB1PINEN);
	} else {
		SET(seqRegSDA, SEQPSR2_M_GPIOB1PINEN);
	}

	sequoiaWrite(PMC_FOMPCR_REG, seqRegSCL);
	sequoiaWrite(SEQR_SEQPSR2_REG, seqRegSDA);

	sequoiaUnlock();
}

static void
sequoiaBBSetDir(uint32_t dir)
{
	uint16_t	seqReg;

	sequoiaLock();

	/*
	 * For direction = Input, set SDA (Output) direction to input,
	 * otherwise we'll only read our own signal on SDA (Input)
	 */
	sequoiaRead(PMC_GPIOCR2_REG, &seqReg);
	if (dir & SHARK_IIC_BIT_OUTPUT)
		SET(seqReg, IIC_DATA_OUT_DIR);
	else
		CLR(seqReg, IIC_DATA_OUT_DIR);
	sequoiaWrite(PMC_GPIOCR2_REG, seqReg);

	sequoiaUnlock();
}

static uint32_t
sequoiaBBRead(void)
{
	uint16_t	seqRegSDA, seqRegSCL;
	uint32_t	bits = 0;

	sequoiaLock();

	sequoiaRead(SEQUOIA_1GPIO, &seqRegSDA);
	sequoiaRead(PMC_FOMPCR_REG, &seqRegSCL);

	sequoiaUnlock();

	if (ISSET(seqRegSDA, IIC_DATA_IN))
		bits |= SHARK_IIC_BIT_SDA;
	if (!ISSET(seqRegSCL, IIC_CLK))
		bits |= SHARK_IIC_BIT_SCL;

	return bits;
}

static void
sharkiicbb_set_bits(void *cookie, uint32_t bits)
{
	sequoiaBBSetBits(bits);
}

static void
sharkiicbb_set_dir(void *cookie, uint32_t dir)
{
	sequoiaBBSetDir(dir);
}

static uint32_t
sharkiicbb_read(void *cookie)
{
	return sequoiaBBRead();
}

static const struct i2c_bitbang_ops sharkiicbb_ops = {
	.ibo_set_bits	=	sharkiicbb_set_bits,
	.ibo_set_dir	=	sharkiicbb_set_dir,
	.ibo_read_bits	=	sharkiicbb_read,
	.ibo_bits = {
		[I2C_BIT_SDA]		=	SHARK_IIC_BIT_SDA,
		[I2C_BIT_SCL]		=	SHARK_IIC_BIT_SCL,
		[I2C_BIT_OUTPUT]	=	SHARK_IIC_BIT_OUTPUT,
		[I2C_BIT_INPUT]		=	SHARK_IIC_BIT_INPUT,
	},
};

/* higher level I2C stuff */

static int
sharkiic_send_start(void *cookie, int flags)
{
	return (i2c_bitbang_send_start(cookie, flags, &sharkiicbb_ops));
}

static int
sharkiic_send_stop(void *cookie, int flags)
{
	return (i2c_bitbang_send_stop(cookie, flags, &sharkiicbb_ops));
}

static int
sharkiic_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	return (i2c_bitbang_initiate_xfer(cookie, addr, flags, 
	    &sharkiicbb_ops));
}

static int
sharkiic_read_byte(void *cookie, uint8_t *valp, int flags)
{
	return (i2c_bitbang_read_byte(cookie, valp, flags, &sharkiicbb_ops));
}

static int
sharkiic_write_byte(void *cookie, uint8_t val, int flags)
{
	return (i2c_bitbang_write_byte(cookie, val, flags, &sharkiicbb_ops));
}

static int
sharkiic_match(device_t parent, cfdata_t match, void *aux)
{
	struct ofbus_attach_args *oba = aux;

	/* "sequoia" interface fills out oba_ofname */
	return strcmp(oba->oba_ofname, "dec,dnard-i2c") == 0;
}

static void
sharkiic_attach(device_t parent, device_t self, void *aux)
{
	struct sharkiic_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;

	aprint_naive("\n");
	aprint_normal("\n");

	sequoiaBBInit();

	iic_tag_init(&sc->sc_i2c);
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_send_start = sharkiic_send_start;
	sc->sc_i2c.ic_send_stop = sharkiic_send_stop;
	sc->sc_i2c.ic_initiate_xfer = sharkiic_initiate_xfer;
	sc->sc_i2c.ic_read_byte = sharkiic_read_byte;
	sc->sc_i2c.ic_write_byte = sharkiic_write_byte;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;

	config_found(self, &iba, iicbus_print, CFARGS_NONE);
}

CFATTACH_DECL_NEW(sharkiic, sizeof(struct sharkiic_softc),
    sharkiic_match, sharkiic_attach, NULL, NULL);
