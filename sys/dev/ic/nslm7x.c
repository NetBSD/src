/*	$NetBSD: nslm7x.c,v 1.59.6.2 2017/12/03 11:37:03 jdolecek Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Squier.
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
__KERNEL_RCSID(0, "$NetBSD: nslm7x.c,v 1.59.6.2 2017/12/03 11:37:03 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/conf.h>
#include <sys/time.h>

#include <sys/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/wbsioreg.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ic/nslm7xvar.h>

#include <sys/intr.h>

#if defined(LMDEBUG)
#define DPRINTF(x)	do { printf x; } while (0)
#else
#define DPRINTF(x)
#endif

/*
 * LM78-compatible chips can typically measure voltages up to 4.096 V.
 * To measure higher voltages the input is attenuated with (external)
 * resistors.  Negative voltages are measured using inverting op amps
 * and resistors.  So we have to convert the sensor values back to
 * real voltages by applying the appropriate resistor factor.
 */
#define RFACT_NONE	10000
#define RFACT(x, y)	(RFACT_NONE * ((x) + (y)) / (y))
#define NRFACT(x, y)	(-RFACT_NONE * (x) / (y))

#define LM_REFRESH_TIMO	(2 * hz)	/* 2 seconds */

static const struct wb_product *wb_lookup(struct lm_softc *,
    const struct wb_product *, uint16_t);
static int wb_match(struct lm_softc *);
static int wb_attach(struct lm_softc *);
static int nslm_match(struct lm_softc *);
static int nslm_attach(struct lm_softc *);
static int def_match(struct lm_softc *);
static int def_attach(struct lm_softc *);
static void wb_temp_diode_type(struct lm_softc *, int);
static uint16_t wb_read_vendorid(struct lm_softc *);

static void lm_refresh(void *);

static void lm_generic_banksel(struct lm_softc *, uint8_t);
static void lm_setup_sensors(struct lm_softc *, const struct lm_sensor *);
static void lm_refresh_sensor_data(struct lm_softc *);
static void lm_refresh_volt(struct lm_softc *, int);
static void lm_refresh_temp(struct lm_softc *, int);
static void lm_refresh_fanrpm(struct lm_softc *, int);

static void wb_refresh_sensor_data(struct lm_softc *);
static void wb_w83637hf_refresh_vcore(struct lm_softc *, int);
static void wb_refresh_nvolt(struct lm_softc *, int);
static void wb_w83627ehf_refresh_nvolt(struct lm_softc *, int);
static void wb_refresh_temp(struct lm_softc *, int);
static void wb_refresh_fanrpm(struct lm_softc *, int);
static void wb_w83792d_refresh_fanrpm(struct lm_softc *, int);
static void wb_nct6776f_refresh_fanrpm(struct lm_softc *, int);

static void as_refresh_temp(struct lm_softc *, int);

struct lm_chip {
	int (*chip_match)(struct lm_softc *);
	int (*chip_attach)(struct lm_softc *);
};

static struct lm_chip lm_chips[] = {
	{ wb_match,	wb_attach },
	{ nslm_match,	nslm_attach },
	{ def_match,	def_attach } /* Must be last */
};

/* LM78/78J/79/81 */
static const struct lm_sensor lm78_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore A",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VCore B",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x21,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(68, 100)
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(30, 10)
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = lm_refresh_volt,
		.rfact = NRFACT(240, 60)
	},
	{
		.desc = "-5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = lm_refresh_volt,
		.rfact = NRFACT(100, 60)
	},
        
	/* Temperature */
	{
		.desc = "Temp0",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = lm_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = lm_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x2a,
		.refresh = lm_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* W83627HF */
static const struct lm_sensor w83627hf_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore A",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VCore B",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x21,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 50)
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(28, 10)
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(232, 56)
	},
	{
		.desc = "-5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(120, 56)
	},
	{
		.desc = "5VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x50,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(17, 33)
	},
	{
		.desc = "VBAT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x51,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},

	/* Temperature */
	{
		.desc = "Temp0",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp1",
		.type = ENVSYS_STEMP,
		.bank = 1,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp2",
		.type = ENVSYS_STEMP,
		.bank = 2,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x2a,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* W8627EHF */

/*
 * The W83627EHF can measure voltages up to 2.048 V instead of the
 * traditional 4.096 V.  For measuring positive voltages, this can be
 * accounted for by halving the resistor factor.  Negative voltages
 * need special treatment, also because the reference voltage is 2.048 V
 * instead of the traditional 3.6 V.
 */
static const struct lm_sensor w83627ehf_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE / 2
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x21,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(56, 10) / 2
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "VIN3",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = wb_w83627ehf_refresh_nvolt,
		.rfact = 0
	},
	{
		.desc = "VIN5",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE / 2
	},
	{
		.desc = "VIN6",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE / 2
	},
	{
		.desc = "3.3VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x50,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "VBAT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x51,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE / 2
	},
	{
		.desc = "VIN8",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x52,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE / 2
	},

	/* Temperature */
	{
		.desc = "Temp0",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp1",
		.type = ENVSYS_STEMP,
		.bank = 1,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp2",
		.type = ENVSYS_STEMP,
		.bank = 2,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x2a,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/*  W83627DHG */
static const struct lm_sensor w83627dhg_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE / 2
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x21,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(56, 10) / 2
	},
	{
		.desc = "AVCC",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = wb_w83627ehf_refresh_nvolt,
		.rfact = 0
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = lm_refresh_volt,
		.rfact = 16000
	},
	{
		.desc = "VIN3",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+3.3VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x50,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "VBAT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x51,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},

	/* Temperature */
	{
		.desc = "MB Temperature",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "CPU Temperature",
		.type = ENVSYS_STEMP,
		.bank = 1,
		.reg = 0x50,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Aux Temp",
		.type = ENVSYS_STEMP,
		.bank = 2,
		.reg = 0x50,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "System Fan",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "CPU Fan",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Aux Fan",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x2a,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* W83637HF */
static const struct lm_sensor w83637hf_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = wb_w83637hf_refresh_vcore,
		.rfact = 0
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x21,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(28, 10)
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 51)
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(232, 56)
	},
	{
		.desc = "5VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x50,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 51)
	},
	{
		.desc = "VBAT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x51,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},

	/* Temperature */
	{
		.desc = "Temp0",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp1",
		.type = ENVSYS_STEMP,
		.bank = 1,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp2",
		.type = ENVSYS_STEMP,
		.bank = 2,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x2a,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* W83697HF */
static const struct lm_sensor w83697hf_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 50)
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(28, 10)
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(232, 56)
	},
	{
		.desc = "-5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(120, 56)
	},
	{
		.desc = "5VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x50,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(17, 33)
	},
	{
		.desc = "VBAT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x51,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},

	/* Temperature */
	{
		.desc = "Temp0",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp1",
		.type = ENVSYS_STEMP,
		.bank = 1,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* W83781D */

/*
 * The datasheet doesn't mention the (internal) resistors used for the
 * +5V, but using the values from the W83782D datasheets seems to
 * provide sensible results.
 */
static const struct lm_sensor w83781d_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore A",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VCore B",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x21,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 50)
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(28, 10)
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = lm_refresh_volt,
		.rfact = NRFACT(2100, 604)
	},
	{
		.desc = "-5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = lm_refresh_volt,
		.rfact = NRFACT(909, 604)
	},

	/* Temperature */
	{
		.desc = "Temp0",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp1",
		.type = ENVSYS_STEMP,
		.bank = 1,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp2",
		.type = ENVSYS_STEMP,
		.bank = 2,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = lm_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = lm_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x2a,
		.refresh = lm_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* W83782D */
static const struct lm_sensor w83782d_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VINR0",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x21,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 50)
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(28, 10)
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(232, 56)
	},
	{
		.desc = "-5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(120, 56)
	},
	{
		.desc = "5VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x50,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(17, 33)
	},
	{
		.desc = "VBAT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x51,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},

	/* Temperature */
	{
		.desc = "Temp0",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp1",
		.type = ENVSYS_STEMP,
		.bank = 1,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp2",
		.type = ENVSYS_STEMP,
		.bank = 2,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x2a,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* W83783S */
static const struct lm_sensor w83783s_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 50)
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(28, 10)
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(232, 56)
	},
	{
		.desc = "-5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(120, 56)
	},

	/* Temperature */
	{
		.desc = "Temp0",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp1",
		.type = ENVSYS_STEMP,
		.bank = 1,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x2a,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* W83791D */
static const struct lm_sensor w83791d_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = 10000
	},
	{
		.desc = "VINR0",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x21,
		.refresh = lm_refresh_volt,
		.rfact = 10000
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = 10000
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 50)
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(28, 10)
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(232, 56)
	},
	{
		.desc = "-5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(120, 56)
	},
	{
		.desc = "5VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0xb0,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(17, 33)
	},
	{
		.desc = "VBAT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0xb1,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VINR1",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0xb2,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},

	/* Temperature */
	{
		.desc = "Temp0",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp1",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0xc0,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp2",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0xc8,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x2a,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan3",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0xba,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan4",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0xbb,
		.refresh = wb_refresh_fanrpm,
		.rfact = 0
	},

        { .desc = NULL }
};

/* W83792D */
static const struct lm_sensor w83792d_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore A",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VCore B",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x21,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "-5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(120, 56)
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(28, 10)
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(232, 56)
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 50)
	},
	{
		.desc = "5VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0xb0,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(17, 33)
	},
	{
		.desc = "VBAT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0xb1,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},

	/* Temperature */
	{
		.desc = "Temp0",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp1",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0xc0,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp2",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0xc8,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = wb_w83792d_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = wb_w83792d_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x2a,
		.refresh = wb_w83792d_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan3",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0xb8,
		.refresh = wb_w83792d_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan4",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0xb9,
		.refresh = wb_w83792d_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan5",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0xba,
		.refresh = wb_w83792d_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan6",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0xbe,
		.refresh = wb_w83792d_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* AS99127F */
static const struct lm_sensor as99127f_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore A",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VCore B",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x21,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 50)
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(28, 10)
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(232, 56)
	},
	{
		.desc = "-5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(120, 56)
	},

	/* Temperature */
	{
		.desc = "Temp0",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp1",
		.type = ENVSYS_STEMP,
		.bank = 1,
		.reg = 0x50,
		.refresh = as_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Temp2",
		.type = ENVSYS_STEMP,
		.bank = 2,
		.reg = 0x50,
		.refresh = as_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x28,
		.refresh = lm_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x29,
		.refresh = lm_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x2a,
		.refresh = lm_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* NCT6776F */
static const struct lm_sensor nct6776f_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x20,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE / 2
	},
	{
		.desc = "+12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x21,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(56, 10) / 2
	},
	{
		.desc = "AVCC",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x22,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = wb_w83627ehf_refresh_nvolt,
		.rfact = 0
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = lm_refresh_volt,
		.rfact = 16000
	},
	{
		.desc = "VIN3",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+3.3VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x50,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "VBAT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x51,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},

	/* Temperature */
	{
		.desc = "MB Temperature",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "CPU Temperature",
		.type = ENVSYS_STEMP,
		.bank = 1,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Aux Temp",
		.type = ENVSYS_STEMP,
		.bank = 2,
		.reg = 0x50,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "System Fan",
		.type = ENVSYS_SFANRPM,
		.bank = 6,
		.reg = 0x56,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "CPU Fan",
		.type = ENVSYS_SFANRPM,
		.bank = 6,
		.reg = 0x58,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Aux Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 6,
		.reg = 0x5a,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Aux Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 6,
		.reg = 0x5c,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},

	{
		.desc = "Aux Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 6,
		.reg = 0x5e,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* NCT610[246]D */
static const struct lm_sensor nct6102d_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x00,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VIN0",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x01,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "AVCC",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x02,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "3VCC",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x03,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "VIN1",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x04,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VIN2",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x05,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "+3.3VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x07,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "VBAT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x08,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "VTT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x09,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},

	/* Temperature */
	{
		.desc = "MB Temperature",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x18,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "CPU Temperature",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x19,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Aux Temp",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x1a,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "System Fan",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x30,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "CPU Fan",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x32,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Aux Fan",
		.type = ENVSYS_SFANRPM,
		.bank = 0,
		.reg = 0x34,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

/* NCT6779D */
static const struct lm_sensor nct6779d_sensors[] = {
	/* Voltage */
	{
		.desc = "VCore",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x80,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE / 2
	},
	{
		.desc = "VIN1",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x81,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(56, 10) / 2
	},
	{
		.desc = "AVCC",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x82,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x83,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "VIN0",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x84,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(48600, 10000)
	},
	{
		.desc = "VIN8",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x85,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE / 2
	},
	{
		.desc = "VIN4",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x86,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+3.3VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x87,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 34) / 2
	},
	{
		.desc = "VBAT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x88,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VTT",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x89,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VIN5",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x8a,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VIN6",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x8b,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VIN2",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x8c,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "VIN3",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x8d,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(14414, 10000)
	},
	{
		.desc = "VIN7",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 4,
		.reg = 0x8e,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE / 2
	},

	/* Temperature */
	{
		.desc = "MB Temperature",
		.type = ENVSYS_STEMP,
		.bank = 4,
		.reg = 0x90,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "CPU Temperature",
		.type = ENVSYS_STEMP,
		.bank = 4,
		.reg = 0x91,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Aux Temp0",
		.type = ENVSYS_STEMP,
		.bank = 4,
		.reg = 0x92,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Aux Temp1",
		.type = ENVSYS_STEMP,
		.bank = 4,
		.reg = 0x93,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Aux Temp2",
		.type = ENVSYS_STEMP,
		.bank = 4,
		.reg = 0x94,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "Aux Temp3",
		.type = ENVSYS_STEMP,
		.bank = 4,
		.reg = 0x95,
		.refresh = wb_refresh_temp,
		.rfact = 0
	},

	/* Fans */
	{
		.desc = "System Fan",
		.type = ENVSYS_SFANRPM,
		.bank = 4,
		.reg = 0xc0,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "CPU Fan",
		.type = ENVSYS_SFANRPM,
		.bank = 4,
		.reg = 0xc2,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Aux Fan0",
		.type = ENVSYS_SFANRPM,
		.bank = 4,
		.reg = 0xc4,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Aux Fan1",
		.type = ENVSYS_SFANRPM,
		.bank = 4,
		.reg = 0xc6,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},
	{
		.desc = "Aux Fan2",
		.type = ENVSYS_SFANRPM,
		.bank = 4,
		.reg = 0xc8,
		.refresh = wb_nct6776f_refresh_fanrpm,
		.rfact = 0
	},

	{ .desc = NULL }
};

static const struct wb_product wb_products[] = {
    { WB_CHIPID_W83627HF,   "W83627HF",	w83627hf_sensors, NULL },
    { WB_CHIPID_W83627THF,  "W83627THF",w83637hf_sensors, NULL },
    { WB_CHIPID_W83627EHF_A,"W83627EHF-A",w83627ehf_sensors,NULL },
    { WB_CHIPID_W83627EHF,  "W83627EHF",w83627ehf_sensors,NULL },
    { WB_CHIPID_W83627DHG,  NULL,	NULL,   NULL },
    { WB_CHIPID_W83637HF,   "W83637HF",	w83637hf_sensors, NULL },
    { WB_CHIPID_W83697HF,   "W83697HF",	w83697hf_sensors, NULL },
    { WB_CHIPID_W83781D,    "W83781D",	w83781d_sensors,  NULL },
    { WB_CHIPID_W83781D_2,  "W83781D",	w83781d_sensors,  NULL },
    { WB_CHIPID_W83782D,    "W83782D",	w83782d_sensors,  NULL },
    { WB_CHIPID_W83783S,    "W83783S",	w83783s_sensors,  NULL },
    { WB_CHIPID_W83791D,    "W83791D",	w83791d_sensors,  NULL },
    { WB_CHIPID_W83791SD,   "W83791SD",	NULL,		  NULL },
    { WB_CHIPID_W83792D,    "W83792D",	w83792d_sensors,  NULL },
    { WB_CHIPID_AS99127F,   NULL,	NULL,  NULL },
    { 0, NULL, NULL, NULL }
};

static const struct wb_product wbsio_products[] = {
    { WBSIO_ID_W83627DHG,   "W83627DHG",w83627dhg_sensors,NULL },
    { WBSIO_ID_NCT6775F,    "NCT6775F", nct6776f_sensors, NULL },
    { WBSIO_ID_NCT6776F,    "NCT6776F", nct6776f_sensors, NULL },
    { WBSIO_ID_NCT5104D,    "NCT5104D or 610[246]D",nct6102d_sensors,NULL },
    { WBSIO_ID_NCT6779D,    "NCT6779D", nct6779d_sensors, NULL },
    { WBSIO_ID_NCT6791D,    "NCT6791D", nct6779d_sensors, NULL },
    { WBSIO_ID_NCT6792D,    "NCT6792D", nct6779d_sensors, NULL },
    { WBSIO_ID_NCT6793D,    "NCT6793D", nct6779d_sensors, NULL },
    { WBSIO_ID_NCT6795D,    "NCT6795D", nct6779d_sensors, NULL },
    { 0, NULL, NULL, NULL }
};

static const struct wb_product as99127f_products[] = {
    { WB_VENDID_ASUS,       "AS99127F", w83781d_sensors,  NULL },
    { WB_VENDID_WINBOND,    "AS99127F rev 2",as99127f_sensors,NULL },
    { 0, NULL, NULL, NULL }
};

static void
lm_generic_banksel(struct lm_softc *lmsc, uint8_t bank)
{
	(*lmsc->lm_writereg)(lmsc, WB_BANKSEL, bank);
}

/*
 * bus independent match
 *
 * prerequisites:  lmsc contains valid lm_{read,write}reg() routines
 * and associated bus access data is present in attachment's softc
 */
int
lm_match(struct lm_softc *lmsc)
{
	uint8_t cr;
	int i, rv;

	/* Perform LM78 reset */
	/*(*lmsc->lm_writereg)(lmsc, LMD_CONFIG, 0x80); */

	cr = (*lmsc->lm_readreg)(lmsc, LMD_CONFIG);

	/* XXX - spec says *only* 0x08! */
	if ((cr != 0x08) && (cr != 0x01) && (cr != 0x03) && (cr != 0x06))
		return 0;

	DPRINTF(("%s: 0x80 check: cr = %x\n", __func__, cr));

	for (i = 0; i < __arraycount(lm_chips); i++)
		if ((rv = lm_chips[i].chip_match(lmsc)) != 0)
			return rv;

	return 0;
}

int
nslm_match(struct lm_softc *sc)
{
	uint8_t chipid;

	/* See if we have an LM78/LM78J/LM79 or LM81 */
	chipid = (*sc->lm_readreg)(sc, LMD_CHIPID) & LM_ID_MASK;
	switch(chipid) {
	case LM_ID_LM78:
	case LM_ID_LM78J:
	case LM_ID_LM79:
	case LM_ID_LM81:
		break;
	default:
		return 0;
	}
	DPRINTF(("%s: chipid %x\n", __func__, chipid));
	return 1;
}

void
lm_attach(struct lm_softc *lmsc)
{
	uint32_t i;
	int rv;

	for (i = 0; i < __arraycount(lm_chips); i++) {
		if (lm_chips[i].chip_match(lmsc) != 0) {
			if (lm_chips[i].chip_attach(lmsc) == 0)
				break;
			else
				return;
		}
	}

	/* Start the monitoring loop */
	(*lmsc->lm_writereg)(lmsc, LMD_CONFIG, 0x01);

	lmsc->sc_sme = sysmon_envsys_create();
	/* Initialize sensors */
	for (i = 0; i < lmsc->numsensors; i++) {
		lmsc->sensors[i].state = ENVSYS_SINVALID;
		if ((rv = sysmon_envsys_sensor_attach(lmsc->sc_sme,
			    &lmsc->sensors[i])) != 0) {
			sysmon_envsys_destroy(lmsc->sc_sme);
			aprint_error_dev(lmsc->sc_dev,
			    "sysmon_envsys_sensor_attach() returned %d\n", rv);
			return;
		}
	}

	/*
	 * Setup the callout to refresh sensor data every 2 seconds.
	 */
	callout_init(&lmsc->sc_callout, 0);
	callout_setfunc(&lmsc->sc_callout, lm_refresh, lmsc);
	callout_schedule(&lmsc->sc_callout, LM_REFRESH_TIMO);

	/*
	 * Hook into the System Monitor.
	 */
	lmsc->sc_sme->sme_name = device_xname(lmsc->sc_dev);
	lmsc->sc_sme->sme_flags = SME_DISABLE_REFRESH;

	if (sysmon_envsys_register(lmsc->sc_sme)) {
		aprint_error_dev(lmsc->sc_dev,
		    "unable to register with sysmon\n");
		sysmon_envsys_destroy(lmsc->sc_sme);
	}
}

/*
 * Stop, destroy the callout and unregister the driver with the
 * sysmon_envsys(9) framework.
 */
void
lm_detach(struct lm_softc *lmsc)
{
	callout_halt(&lmsc->sc_callout, NULL);
	callout_destroy(&lmsc->sc_callout);
	sysmon_envsys_unregister(lmsc->sc_sme);
}

static void
lm_refresh(void *arg)
{
	struct lm_softc *lmsc = arg;

	lmsc->refresh_sensor_data(lmsc);
	callout_schedule(&lmsc->sc_callout, LM_REFRESH_TIMO);
}

static int
nslm_attach(struct lm_softc *sc)
{
	const char *model = NULL;
	uint8_t chipid;

	/* See if we have an LM78/LM78J/LM79 or LM81 */
	chipid = (*sc->lm_readreg)(sc, LMD_CHIPID) & LM_ID_MASK;
	switch(chipid) {
	case LM_ID_LM78:
		model = "LM78";
		break;
	case LM_ID_LM78J:
		model = "LM78J";
		break;
	case LM_ID_LM79:
		model = "LM79";
		break;
	case LM_ID_LM81:
		model = "LM81";
		break;
	default:
		return -1;
	}

	aprint_naive("\n");
	aprint_normal("\n");
	aprint_normal_dev(sc->sc_dev,
	    "National Semiconductor %s Hardware monitor\n", model);

	lm_setup_sensors(sc, lm78_sensors);
	sc->refresh_sensor_data = lm_refresh_sensor_data;
	return 0;
}

static int
def_match(struct lm_softc *sc)
{

	return 1;
}

static int
def_attach(struct lm_softc *sc)
{
	uint8_t chipid;

	chipid = (*sc->lm_readreg)(sc, LMD_CHIPID) & LM_ID_MASK;
	aprint_naive("\n");
	aprint_normal("\n");
	aprint_error_dev(sc->sc_dev, "Unknown chip (ID 0x%02x)\n", chipid);

	lm_setup_sensors(sc, lm78_sensors);
	sc->refresh_sensor_data = lm_refresh_sensor_data;
	return 0;
}

static void
wb_temp_diode_type(struct lm_softc *sc, int diode_type)
{
	uint8_t regval, banksel;

	banksel = (*sc->lm_readreg)(sc, WB_BANKSEL);
	switch (diode_type) {
	    case 1:	/* Switch to Pentium-II diode mode */
		lm_generic_banksel(sc, WB_BANKSEL_B0);
		regval = (*sc->lm_readreg)(sc, WB_BANK0_VBAT);
		regval |= 0x0e;
		(*sc->lm_writereg)(sc, WB_BANK0_VBAT, regval);
		regval = (*sc->lm_readreg)(sc, WB_BANK0_RESVD1);
		regval |= 0x70;
		(*sc->lm_writereg)(sc, WB_BANK0_RESVD1, 0x0);
		lm_generic_banksel(sc, banksel);
		aprint_verbose_dev(sc->sc_dev, "Pentium-II diode temp sensors\n");
		break;
	    case 2:	/* Switch to 2N3904 mode */
		lm_generic_banksel(sc, WB_BANKSEL_B0);
		regval = (*sc->lm_readreg)(sc, WB_BANK0_VBAT);
		regval |= 0xe;
		(*sc->lm_writereg)(sc, WB_BANK0_VBAT, regval);
		regval = (*sc->lm_readreg)(sc, WB_BANK0_RESVD1);
		regval &= ~0x70;
		(*sc->lm_writereg)(sc, WB_BANK0_RESVD1, 0x0);
		lm_generic_banksel(sc, banksel);
		aprint_verbose_dev(sc->sc_dev, "2N3904 bipolar temp sensors\n");
		break;
	    case 4:	/* Switch to generic thermistor mode */
		lm_generic_banksel(sc, WB_BANKSEL_B0);
		regval = (*sc->lm_readreg)(sc, WB_BANK0_VBAT);
		regval &= ~0xe;
		(*sc->lm_writereg)(sc, WB_BANK0_VBAT, regval);
		lm_generic_banksel(sc, banksel);
		aprint_verbose_dev(sc->sc_dev, "Thermistor temp sensors\n");
		break;
	    case 0:	/* Unspecified - use default */
		aprint_verbose_dev(sc->sc_dev, "Using default temp sensors\n");
		break;
	    default:
		aprint_error_dev(sc->sc_dev,
				 "Ignoring invalid temp sensor mode %d\n",
				 diode_type);
		break;
	}
}

static const struct wb_product *
wb_lookup(struct lm_softc *sc, const struct wb_product *products, uint16_t id)
{
	const struct wb_product *prod = products;
	int i = 0;

	while (prod[i].id != 0) {
		if (prod[i].id != id) {
			i++;
			continue;
		}
		if (prod[i].str == NULL) {
			if (products == wb_products) {
				if (id == WB_CHIPID_W83627DHG) {
					/*
					 *  Lookup wbsio_products
					 * with WBSIO_ID.
					 */
					return wb_lookup(sc, wbsio_products,
					    sc->sioid);
				} else if (id == WB_CHIPID_AS99127F) {
					/*
					 *  Lookup as99127f_products
					 * with WB_VENDID.
					 */
					return wb_lookup(sc, as99127f_products,
					    wb_read_vendorid(sc));
				} else
					return NULL; /* not occur */
			}
			return NULL; /* not occur */
		}
		return &prod[i];
	}

	/* Not found */
	return NULL;
}

static uint16_t
wb_read_vendorid(struct lm_softc *sc)
{
	uint16_t vendid;
	uint8_t vendidreg;
	uint8_t banksel;

	/* Save bank */
	banksel = (*sc->lm_readreg)(sc, WB_BANKSEL);

	/* Check default vendor ID register first */
	vendidreg = WB_VENDID;

retry:
	/* Read vendor ID */
	lm_generic_banksel(sc, WB_BANKSEL_HBAC);
	vendid = (*sc->lm_readreg)(sc, vendidreg) << 8;
	lm_generic_banksel(sc, 0);
	vendid |= (*sc->lm_readreg)(sc, vendidreg);

	if ((vendidreg == WB_VENDID)
	    &&  (vendid != WB_VENDID_WINBOND && vendid != WB_VENDID_ASUS)) {
		/* If it failed, try NCT6102 vendor ID register */
		vendidreg = WB_NCT6102_VENDID;
		goto retry;
	} else if ((vendidreg == WB_NCT6102_VENDID)
	    && (vendid != WB_VENDID_WINBOND))
		vendid = 0; /* XXX */
	
	/* Restore bank */
	lm_generic_banksel(sc, banksel);

	return vendid;
}

static uint8_t
wb_read_chipid(struct lm_softc *sc)
{
	const struct wb_product *prod;
	uint8_t chipidreg, chipid, banksel;

	/* Save bank */
	banksel = (*sc->lm_readreg)(sc, WB_BANKSEL);

	/* Check default vendor ID register first */
	chipidreg = WB_BANK0_CHIPID;
	lm_generic_banksel(sc, WB_BANKSEL_B0);

retry:
	(void)(*sc->lm_readreg)(sc, LMD_CHIPID);
	chipid = (*sc->lm_readreg)(sc, chipidreg);
	prod = wb_lookup(sc, wb_products, chipid);
	if (prod == NULL) {
		if (chipidreg == WB_BANK0_CHIPID) {
			chipidreg = WB_BANK0_NCT6102_CHIPID;
			goto retry;
		} else
			chipid = 0;
	}
	/* Restore bank */
	lm_generic_banksel(sc, banksel);

	return chipid;
}

static int
wb_match(struct lm_softc *sc)
{
	const struct wb_product *prod;
	uint16_t vendid;
	uint8_t chipid;

	/* Read vendor ID */
	vendid = wb_read_vendorid(sc);
	DPRINTF(("%s: winbond vend id 0x%x\n", __func__, vendid));
	if ((vendid != WB_VENDID_WINBOND && vendid != WB_VENDID_ASUS))
		return 0;

	/* Read device/chip ID */
	chipid = wb_read_chipid(sc);
	DPRINTF(("%s: winbond chip id 0x%x\n", __func__, chipid));
	prod = wb_lookup(sc, wb_products, chipid);

	if (prod == NULL) {
		if (vendid == WB_VENDID_WINBOND)
			return 1; /* Generic match */
		else
			return 0;
	}
	DPRINTF(("%s: chipid %02x, sioid = %04x\n", __func__, chipid,
		sc->sioid));

	return 10; /* found */
}

static int
wb_attach(struct lm_softc *sc)
{
	device_t dev = sc->sc_dev;
	const struct wb_product *prod;
	const char *model = NULL;
	const char *vendor = "Winbond";
	const struct lm_sensor *sensors;
	uint16_t vendid;
	uint8_t banksel;
	int cf_flags;

	aprint_naive("\n");
	aprint_normal("\n");
	/* Read device/chip ID */
	sc->chipid = wb_read_chipid(sc);
	DPRINTF(("%s: winbond chip id 0x%x\n", __func__, sc->chipid));

	if ((prod = wb_lookup(sc, wb_products, sc->chipid)) != NULL) {
		switch (prod->str[0]) {
		case 'W':
			vendor = "Winbond";
			break;
		case 'A':
			vendor = "ASUS";
			break;
		case 'N':
			vendor = "Nuvoton";
			break;
		default:
			aprint_error_dev(dev, "Unknown model (%s)\n", model);
			return -1;
		}
		model = prod->str;
		sensors = prod->sensors;
		sc->refresh_sensor_data = wb_refresh_sensor_data;
		if (prod->extattach != NULL)
			prod->extattach(sc);
	} else {
		vendid = wb_read_vendorid(sc);
		if (vendid == WB_VENDID_WINBOND) {
			vendor = "Winbond";
			model = "unknown-model";

			/* Handle as a standard LM78. */
			sensors = lm78_sensors;
			sc->refresh_sensor_data = lm_refresh_sensor_data;
		} else {
			aprint_error_dev(dev, "Unknown chip (ID %02x)\n",
			    sc->chipid);
			return -1;
		}
	}
	
	cf_flags = device_cfdata(dev)->cf_flags;

	if (sensors != NULL) {
		lm_setup_sensors(sc, sensors);

		/* XXX Is this correct? Check all datasheets. */
		switch (sc->chipid) {
		case WB_CHIPID_W83627EHF_A:
		case WB_CHIPID_W83781D:
		case WB_CHIPID_W83781D_2:
		case WB_CHIPID_W83791SD:
		case WB_CHIPID_W83792D:
		case WB_CHIPID_AS99127F:
			break;
		default:
			wb_temp_diode_type(sc, cf_flags);
			break;
		}
	}

	/* XXX Is this correct? Check all datasheets. */
	banksel = (*sc->lm_readreg)(sc, WB_BANKSEL);
	switch(sc->chipid) {
	case WB_CHIPID_W83627THF:
		lm_generic_banksel(sc, WB_BANKSEL_B0);
		if ((*sc->lm_readreg)(sc, WB_BANK0_CONFIG) & WB_CONFIG_VMR9)
			sc->vrm9 = 1;
		lm_generic_banksel(sc, banksel);
		break;
	case WB_CHIPID_W83637HF:
		lm_generic_banksel(sc, WB_BANKSEL_B0);
		if ((*sc->lm_readreg)(sc, WB_BANK0_CONFIG) & WB_CONFIG_VMR9)
			sc->vrm9 = 1;
		lm_generic_banksel(sc, banksel);
		break;
	default:
		break;
	}

	aprint_normal_dev(dev, "%s %s Hardware monitor\n", vendor, model);

	return 0;
}

static void
lm_setup_sensors(struct lm_softc *sc, const struct lm_sensor *sensors)
{
	int i;

	for (i = 0; sensors[i].desc; i++) {
		sc->sensors[i].units = sensors[i].type;
		if (sc->sensors[i].units == ENVSYS_SVOLTS_DC)
			sc->sensors[i].flags = ENVSYS_FCHANGERFACT;
		strlcpy(sc->sensors[i].desc, sensors[i].desc,
		    sizeof(sc->sensors[i].desc));
		sc->numsensors++;
	}
	sc->lm_sensors = sensors;
}

static void
lm_refresh_sensor_data(struct lm_softc *sc)
{
	int i;

	for (i = 0; i < sc->numsensors; i++)
		sc->lm_sensors[i].refresh(sc, i);
}

static void
lm_refresh_volt(struct lm_softc *sc, int n)
{
	int data;

	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	if (data == 0xff) {
		sc->sensors[n].state = ENVSYS_SINVALID;
	} else {
		sc->sensors[n].value_cur = (data << 4);
		if (sc->sensors[n].rfact) {
			sc->sensors[n].value_cur *= sc->sensors[n].rfact;
			sc->sensors[n].value_cur /= 10;
		} else {
			sc->sensors[n].value_cur *= sc->lm_sensors[n].rfact;
			sc->sensors[n].value_cur /= 10;
			sc->sensors[n].rfact = sc->lm_sensors[n].rfact;
		}
		sc->sensors[n].state = ENVSYS_SVALID;
	}

	DPRINTF(("%s: volt[%d] data=0x%x value_cur=%d\n",
	    __func__, n, data, sc->sensors[n].value_cur));
}

static void
lm_refresh_temp(struct lm_softc *sc, int n)
{
	int data;

	/*
	 * The data sheet suggests that the range of the temperature
	 * sensor is between -55 degC and +125 degC.
	 */
	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	if (data > 0x7d && data < 0xc9)
		sc->sensors[n].state = ENVSYS_SINVALID;
	else {
		if (data & 0x80)
			data -= 0x100;
		sc->sensors[n].state = ENVSYS_SVALID;
		sc->sensors[n].value_cur = data * 1000000 + 273150000;
	}
	DPRINTF(("%s: temp[%d] data=0x%x value_cur=%d\n",
	    __func__, n, data, sc->sensors[n].value_cur));
}

static void
lm_refresh_fanrpm(struct lm_softc *sc, int n)
{
	int data, divisor = 1;

	/*
	 * We might get more accurate fan readings by adjusting the
	 * divisor, but that might interfere with APM or other SMM
	 * BIOS code reading the fan speeds.
	 */

	/* FAN3 has a fixed fan divisor. */
	if (sc->lm_sensors[n].reg == LMD_FAN1 ||
	    sc->lm_sensors[n].reg == LMD_FAN2) {
		data = (*sc->lm_readreg)(sc, LMD_VIDFAN);
		if (sc->lm_sensors[n].reg == LMD_FAN1)
			divisor = (data >> 4) & 0x03;
		else
			divisor = (data >> 6) & 0x03;
	}

	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	if (data == 0xff || data == 0x00)
		sc->sensors[n].state = ENVSYS_SINVALID;
	else {
		sc->sensors[n].state = ENVSYS_SVALID;
		sc->sensors[n].value_cur = 1350000 / (data << divisor);
	}
	DPRINTF(("%s: fan[%d] data=0x%x value_cur=%d\n",
	    __func__, n, data, sc->sensors[n].value_cur));
}

static void
wb_refresh_sensor_data(struct lm_softc *sc)
{
	uint8_t banksel, bank;
	int i;

	/*
	 * Properly save and restore bank selection register.
	 */
	banksel = bank = sc->lm_readreg(sc, WB_BANKSEL);
	for (i = 0; i < sc->numsensors; i++) {
		if (bank != sc->lm_sensors[i].bank) {
			bank = sc->lm_sensors[i].bank;
			lm_generic_banksel(sc, bank);
		}
		sc->lm_sensors[i].refresh(sc, i);
	}
	lm_generic_banksel(sc, banksel);
}

static void
wb_w83637hf_refresh_vcore(struct lm_softc *sc, int n)
{
	int data;

	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	/*
	 * Depending on the voltage detection method,
	 * one of the following formulas is used:
	 *	VRM8 method: value = raw * 0.016V
	 *	VRM9 method: value = raw * 0.00488V + 0.70V
	 */
	if (sc->vrm9)
		sc->sensors[n].value_cur = (data * 4880) + 700000;
	else
		sc->sensors[n].value_cur = (data * 16000);
	sc->sensors[n].state = ENVSYS_SVALID;
	DPRINTF(("%s: volt[%d] data=0x%x value_cur=%d\n",
	   __func__, n, data, sc->sensors[n].value_cur));
}

static void
wb_refresh_nvolt(struct lm_softc *sc, int n)
{
	int data;

	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	sc->sensors[n].value_cur = ((data << 4) - WB_VREF);
	if (sc->sensors[n].rfact)
		sc->sensors[n].value_cur *= sc->sensors[n].rfact;
	else
		sc->sensors[n].value_cur *= sc->lm_sensors[n].rfact;

	sc->sensors[n].value_cur /= 10;
	sc->sensors[n].value_cur += WB_VREF * 1000;
	sc->sensors[n].state = ENVSYS_SVALID;
	DPRINTF(("%s: volt[%d] data=0x%x value_cur=%d\n",
	     __func__, n , data, sc->sensors[n].value_cur));
}

static void
wb_w83627ehf_refresh_nvolt(struct lm_softc *sc, int n)
{
	int data;

	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	sc->sensors[n].value_cur = ((data << 3) - WB_W83627EHF_VREF);
	if (sc->sensors[n].rfact)
		sc->sensors[n].value_cur *= sc->sensors[n].rfact;
	else	
		sc->sensors[n].value_cur *= RFACT(232, 10);

	sc->sensors[n].value_cur /= 10;
	sc->sensors[n].value_cur += WB_W83627EHF_VREF * 1000;
	sc->sensors[n].state = ENVSYS_SVALID;
	DPRINTF(("%s: volt[%d] data=0x%x value_cur=%d\n",
	    __func__, n , data, sc->sensors[n].value_cur));
}

static void
wb_refresh_temp(struct lm_softc *sc, int n)
{
	int data;

	/*
	 * The data sheet suggests that the range of the temperature
	 * sensor is between -55 degC and +125 degC.  However, values
	 * around -48 degC seem to be a very common bogus values.
	 * Since such values are unreasonably low, we use -45 degC for
	 * the lower limit instead.
	 */
	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg) << 1;
	data += (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg + 1) >> 7;
	if (data > 0xfffffff || (data > 0x0fa && data < 0x1a6)) {
		sc->sensors[n].state = ENVSYS_SINVALID;
	} else {
		if (data & 0x100)
			data -= 0x200;
		sc->sensors[n].state = ENVSYS_SVALID;
		sc->sensors[n].value_cur = data * 500000 + 273150000;
	}
	DPRINTF(("%s: temp[%d] data=0x%x value_cur=%d\n",
	    __func__, n , data, sc->sensors[n].value_cur));
}

static void
wb_refresh_fanrpm(struct lm_softc *sc, int n)
{
	int fan, data, divisor = 0;

	/* 
	 * This is madness; the fan divisor bits are scattered all
	 * over the place.
	 */

	if (sc->lm_sensors[n].reg == LMD_FAN1 ||
	    sc->lm_sensors[n].reg == LMD_FAN2 ||
	    sc->lm_sensors[n].reg == LMD_FAN3) {
		data = (*sc->lm_readreg)(sc, WB_BANK0_VBAT);
		fan = (sc->lm_sensors[n].reg - LMD_FAN1);
		if ((data >> 5) & (1 << fan))
			divisor |= 0x04;
	}

	if (sc->lm_sensors[n].reg == LMD_FAN1 ||
	    sc->lm_sensors[n].reg == LMD_FAN2) {
		data = (*sc->lm_readreg)(sc, LMD_VIDFAN);
		if (sc->lm_sensors[n].reg == LMD_FAN1)
			divisor |= (data >> 4) & 0x03;
		else
			divisor |= (data >> 6) & 0x03;
	} else if (sc->lm_sensors[n].reg == LMD_FAN3) {
		data = (*sc->lm_readreg)(sc, WB_PIN);
		divisor |= (data >> 6) & 0x03;
	} else if (sc->lm_sensors[n].reg == WB_BANK0_FAN4 ||
		   sc->lm_sensors[n].reg == WB_BANK0_FAN5) {
		data = (*sc->lm_readreg)(sc, WB_BANK0_FAN45);
		if (sc->lm_sensors[n].reg == WB_BANK0_FAN4)
			divisor |= (data >> 0) & 0x07;
		else
			divisor |= (data >> 4) & 0x07;
	}

	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	if (data >= 0xff || data == 0x00)
		sc->sensors[n].state = ENVSYS_SINVALID;
	else {
		sc->sensors[n].state = ENVSYS_SVALID;
		sc->sensors[n].value_cur = 1350000 / (data << divisor);
	}
	DPRINTF(("%s: fan[%d] data=0x%x value_cur=%d\n",
	    __func__, n , data, sc->sensors[n].value_cur));
}

static void
wb_nct6776f_refresh_fanrpm(struct lm_softc *sc, int n)
{
	int datah, datal;

	datah = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	datal = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg + 1);

	if ((datah == 0xff) || (datah == 0)) {
		sc->sensors[n].state = ENVSYS_SINVALID;
	} else {
		sc->sensors[n].state = ENVSYS_SVALID;
		sc->sensors[n].value_cur = (datah << 8) | datal;
	}
}

static void
wb_w83792d_refresh_fanrpm(struct lm_softc *sc, int n)
{
	int shift, data, divisor = 1;
	uint8_t reg;

	shift = 0;

	switch (sc->lm_sensors[n].reg) {
	case 0x28:
		reg = 0x47; shift = 0;
		break;
	case 0x29:
		reg = 0x47; shift = 4;
		break;
	case 0x2a:
		reg = 0x5b; shift = 0;
		break;
	case 0xb8:
		reg = 0x5b; shift = 4;
		break;
	case 0xb9:
		reg = 0x5c; shift = 0;
		break;
	case 0xba:
		reg = 0x5c; shift = 4;
		break;
	case 0xbe:
		reg = 0x9e; shift = 0;
		break;
	default:
		reg = 0;
		break;
	}

	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	if (data == 0xff || data == 0x00)
		sc->sensors[n].state = ENVSYS_SINVALID;
	else {
		if (reg != 0)
			divisor = ((*sc->lm_readreg)(sc, reg) >> shift) & 0x7;
		sc->sensors[n].state = ENVSYS_SVALID;
		sc->sensors[n].value_cur = 1350000 / (data << divisor);
	}
	DPRINTF(("%s: fan[%d] data=0x%x value_cur=%d\n",
	    __func__, n , data, sc->sensors[n].value_cur));
}

static void
as_refresh_temp(struct lm_softc *sc, int n)
{
	int data;

	/*
	 * It seems a shorted temperature diode produces an all-ones
	 * bit pattern.
	 */
	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg) << 1;
	data += (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg + 1) >> 7;
	if (data == 0x1ff)
		sc->sensors[n].state = ENVSYS_SINVALID;
	else {
		if (data & 0x100)
			data -= 0x200;
		sc->sensors[n].state = ENVSYS_SVALID;
		sc->sensors[n].value_cur = data * 500000 + 273150000;
	}
	DPRINTF(("%s: temp[%d] data=0x%x value_cur=%d\n",
	    __func__, n, data, sc->sensors[n].value_cur));
}

MODULE(MODULE_CLASS_DRIVER, lm, "sysmon_envsys");

static int
lm_modcmd(modcmd_t cmd, void *opaque)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
}
