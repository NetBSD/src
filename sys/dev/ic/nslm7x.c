/*	$NetBSD: nslm7x.c,v 1.29.2.2 2007/03/24 14:55:28 yamt Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: nslm7x.c,v 1.29.2.2 2007/03/24 14:55:28 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/time.h>

#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ic/nslm7xvar.h>

#include <machine/intr.h>

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

const struct envsys_range lm_ranges[] = {	/* sc->sensors sub-intervals */
						/* for each unit type */
	{ 7, 7,    ENVSYS_STEMP   },
	{ 8, 10,   ENVSYS_SFANRPM },
	{ 1, 0,    ENVSYS_SVOLTS_AC },	/* None */
	{ 0, 6,    ENVSYS_SVOLTS_DC },
	{ 1, 0,    ENVSYS_SOHMS },	/* None */
	{ 1, 0,    ENVSYS_SWATTS },	/* None */
	{ 1, 0,    ENVSYS_SAMPS }	/* None */
};

static int lm_match(struct lm_softc *);
static int wb_match(struct lm_softc *);
static int def_match(struct lm_softc *);

static void lm_generic_banksel(struct lm_softc *, int);
static void lm_setup_sensors(struct lm_softc *, struct lm_sensor *);

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

static void as_refresh_temp(struct lm_softc *, int);

static int lm_gtredata(struct sysmon_envsys *, struct envsys_tre_data *);
static int generic_streinfo_fan(struct lm_softc *, struct envsys_basic_info *,
           int, struct envsys_basic_info *);
static int lm_streinfo(struct sysmon_envsys *, struct envsys_basic_info *);
static int wb781_streinfo(struct sysmon_envsys *, struct envsys_basic_info *);
static int wb782_streinfo(struct sysmon_envsys *, struct envsys_basic_info *);

struct lm_chip {
	int (*chip_match)(struct lm_softc *);
};

static struct lm_chip lm_chips[] = {
	{ wb_match },
	{ lm_match },
	{ def_match } /* Must be last */
};

/* LM78/78J/79/81 */
static struct lm_sensor lm78_sensors[] = {
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
static struct lm_sensor w83627hf_sensors[] = {
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
static struct lm_sensor w83627ehf_sensors[] = {
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
		.desc = "+3.3V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(34, 24) / 2
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
		.desc = "Unknown",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE / 2
	},
	{
		.desc = "Unknown",
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
		.desc = "Unknown",
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
static struct lm_sensor w83627dhg_sensors[] = {
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
		.rfact = RFACT_NONE
	},
	{
		.desc = "AVCC",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x23,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
	},
	{
		.desc = "+5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x25,
		.refresh = lm_refresh_volt,
		.rfact = RFACT(32, 56)
	},
	/* 
	 * I'm not sure about which one is -12V or -5V.
	 */
#if 0
	{
		.desc = "-12V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x24,
		.refresh = wb_refresh_nvolt,
		.rfact = RFACT(232, 60)
	},
	{
		.desc = "-5V",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 0,
		.reg = 0x26,
		.refresh = wb_w83627ehf_refresh_nvolt
		.rfact = 0
	},
#endif
	{
		.desc = "+3.3VSB",
		.type = ENVSYS_SVOLTS_DC,
		.bank = 5,
		.reg = 0x50,
		.refresh = lm_refresh_volt,
		.rfact = RFACT_NONE
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
		.desc = "System Temp",
		.type = ENVSYS_STEMP,
		.bank = 0,
		.reg = 0x27,
		.refresh = lm_refresh_temp,
		.rfact = 0
	},
	{
		.desc = "CPU Temp",
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
static struct lm_sensor w83637hf_sensors[] = {
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
static struct lm_sensor w83697hf_sensors[] = {
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
static struct lm_sensor w83781d_sensors[] = {
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
static struct lm_sensor w83782d_sensors[] = {
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
static struct lm_sensor w83783s_sensors[] = {
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
static struct lm_sensor w83791d_sensors[] = {
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
static struct lm_sensor w83792d_sensors[] = {
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
static struct lm_sensor as99127f_sensors[] = {
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

static void
lm_generic_banksel(struct lm_softc *lmsc, int bank)
{
	(*lmsc->lm_writereg)(lmsc, WB_BANKSEL, bank);
}

/*
 * bus independent probe
 */
int
lm_probe(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	uint8_t cr;
	int rv;

	/* Check for some power-on defaults */
	bus_space_write_1(iot, ioh, LMC_ADDR, LMD_CONFIG);

	/* Perform LM78 reset */
	bus_space_write_1(iot, ioh, LMC_DATA, 0x80);

	/* XXX - Why do I have to reselect the register? */
	bus_space_write_1(iot, ioh, LMC_ADDR, LMD_CONFIG);
	cr = bus_space_read_1(iot, ioh, LMC_DATA);

	/* XXX - spec says *only* 0x08! */
	if ((cr == 0x08) || (cr == 0x01) || (cr == 0x03))
		rv = 1;
	else
		rv = 0;

	DPRINTF(("lm: rv = %d, cr = %x\n", rv, cr));

	return rv;
}


/*
 * pre:  lmsc contains valid busspace tag and handle
 */
void
lm_attach(struct lm_softc *lmsc)
{
	uint32_t i;

	for (i = 0; i < __arraycount(lm_chips); i++)
		if (lm_chips[i].chip_match(lmsc))
			break;

	/* Start the monitoring loop */
	(*lmsc->lm_writereg)(lmsc, LMD_CONFIG, 0x01);

	/* Indicate we have never read the registers */
	timerclear(&lmsc->lastread);

	/* Initialize sensors */
	for (i = 0; i < lmsc->numsensors; ++i) {
		lmsc->sensors[i].sensor = lmsc->info[i].sensor = i;
		lmsc->sensors[i].validflags = (ENVSYS_FVALID|ENVSYS_FCURVALID);
		lmsc->info[i].validflags = ENVSYS_FVALID;
		lmsc->sensors[i].warnflags = ENVSYS_WARN_OK;
	}
	/*
	 * Hook into the System Monitor.
	 */
	lmsc->sc_sysmon.sme_ranges = lm_ranges;
	lmsc->sc_sysmon.sme_sensor_info = lmsc->info;
	lmsc->sc_sysmon.sme_sensor_data = lmsc->sensors;
	lmsc->sc_sysmon.sme_cookie = lmsc;

	lmsc->sc_sysmon.sme_gtredata = lm_gtredata;
	/* sme_streinfo set in chip-specific attach */

	lmsc->sc_sysmon.sme_nsensors = lmsc->numsensors;
	lmsc->sc_sysmon.sme_envsys_version = 1000;

	if (sysmon_envsys_register(&lmsc->sc_sysmon))
		aprint_error("%s: unable to register with sysmon\n",
		    lmsc->sc_dev.dv_xname);
}

static int
lm_match(struct lm_softc *sc)
{
	const char *model = NULL;
	int chipid;

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
		return 0;
	}

	aprint_normal(": National Semiconductor %s Hardware monitor\n", model);

	lm_setup_sensors(sc, lm78_sensors);
	sc->sc_sysmon.sme_streinfo = lm_streinfo;
	sc->refresh_sensor_data = lm_refresh_sensor_data;
	return 1;
}

static int
def_match(struct lm_softc *sc)
{
	int chipid;

	chipid = (*sc->lm_readreg)(sc, LMD_CHIPID) & LM_ID_MASK;
	aprint_error(": Unknown chip (ID %d)\n", chipid);

	lm_setup_sensors(sc, lm78_sensors);
	sc->sc_sysmon.sme_streinfo = lm_streinfo;
	sc->refresh_sensor_data = lm_refresh_sensor_data;
	return 1;
}

static int
wb_match(struct lm_softc *sc)
{
	const char *model;
	int banksel, vendid, devid;

	model = NULL;

	/* Read vendor ID */
	banksel = (*sc->lm_readreg)(sc, WB_BANKSEL);
	lm_generic_banksel(sc, WB_BANKSEL_HBAC);

	vendid = (*sc->lm_readreg)(sc, WB_VENDID) << 8;
	lm_generic_banksel(sc, 0);
	vendid |= (*sc->lm_readreg)(sc, WB_VENDID);
	DPRINTF(("winbond vend id 0x%x\n", vendid));
	if (vendid != WB_VENDID_WINBOND && vendid != WB_VENDID_ASUS)
		return 0;

	/* Read device/chip ID */
	lm_generic_banksel(sc, WB_BANKSEL_B0);
	devid = (*sc->lm_readreg)(sc, LMD_CHIPID);
	sc->chipid = (*sc->lm_readreg)(sc, WB_BANK0_CHIPID);
	lm_generic_banksel(sc, banksel);
	DPRINTF(("winbond chip id 0x%x\n", sc->chipid));

	switch(sc->chipid) {
	case WB_CHIPID_W83627HF:
		model = "W83627HF";
		lm_setup_sensors(sc, w83627hf_sensors);
		break;
	case WB_CHIPID_W83627THF:
		model = "W83627THF";
		lm_setup_sensors(sc, w83637hf_sensors);
		break;
	case WB_CHIPID_W83627EHF:
		model = "W83627EHF";
		lm_setup_sensors(sc, w83627ehf_sensors);
		break;
	case WB_CHIPID_W83627DHG:
		model = "W83627DHG";
		lm_setup_sensors(sc, w83627dhg_sensors);
		break;
	case WB_CHIPID_W83637HF:
		model = "W83637HF";
		lm_generic_banksel(sc, WB_BANKSEL_B0);
		if ((*sc->lm_readreg)(sc, WB_BANK0_CONFIG) & WB_CONFIG_VMR9)
			sc->vrm9 = 1;
		lm_generic_banksel(sc, banksel);
		lm_setup_sensors(sc, w83637hf_sensors);
		break;
	case WB_CHIPID_W83697HF:
		model = "W83697HF";
		lm_setup_sensors(sc, w83697hf_sensors);
		break;
	case WB_CHIPID_W83781D:
	case WB_CHIPID_W83781D_2:
		model = "W83781D";
		lm_setup_sensors(sc, w83781d_sensors);
		sc->sc_sysmon.sme_streinfo = wb781_streinfo;
		break;
	case WB_CHIPID_W83782D:
		model = "W83782D";
		lm_setup_sensors(sc, w83782d_sensors);
		sc->sc_sysmon.sme_streinfo = wb782_streinfo;
		break;
	case WB_CHIPID_W83783S:
		model = "W83783S";
		lm_setup_sensors(sc, w83783s_sensors);
		break;
	case WB_CHIPID_W83791D:
		model = "W83791D";
		lm_setup_sensors(sc, w83791d_sensors);
		break;
	case WB_CHIPID_W83791SD:
		model = "W83791SD";
		break;
	case WB_CHIPID_W83792D:
		model = "W83792D";
		lm_setup_sensors(sc, w83792d_sensors);
		break;
	case WB_CHIPID_AS99127F:
		if (vendid == WB_VENDID_ASUS) {
			model = "AS99127F";
			lm_setup_sensors(sc, w83781d_sensors);
		} else {
			model = "AS99127F rev 2";
			lm_setup_sensors(sc, as99127f_sensors);
		}
		break;
	default:
		aprint_normal(": unknown Winbond chip (ID 0x%x)\n",
		    sc->chipid);
		/* Handle as a standard LM78. */
		lm_setup_sensors(sc, lm78_sensors);
		sc->refresh_sensor_data = lm_refresh_sensor_data;
		return 1;
	}

	aprint_normal(": Winbond %s Hardware monitor\n", model);

	sc->sc_sysmon.sme_streinfo = lm_streinfo;
	sc->refresh_sensor_data = wb_refresh_sensor_data;
	return 1;
}

static void
lm_setup_sensors(struct lm_softc *sc, struct lm_sensor *sensors)
{
	int i;

	for (i = 0; sensors[i].desc; i++) {
		sc->sensors[i].units = sc->info[i].units = sensors[i].type;
		strlcpy(sc->info[i].desc, sensors[i].desc,
		    sizeof(sc->info[i].desc));
		sc->numsensors++;
	}
	sc->lm_sensors = sensors;
}

static void
lm_refresh_sensor_data(struct lm_softc *sc)
{
	int i;

	/* Refresh our stored data for every sensor */
	for (i = 0; i < sc->numsensors; i++)
		sc->lm_sensors[i].refresh(sc, i);
}

static void
lm_refresh_volt(struct lm_softc *sc, int n)
{
	int data;

	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	DPRINTF(("%s: volt[%d] 0x%x\n", __func__, n, data)); 
	sc->sensors[n].cur.data_s = (data << 4);
	sc->sensors[n].cur.data_s *= sc->lm_sensors[n].rfact;
	sc->sensors[n].cur.data_s /= 10;
	sc->info[n].rfact = sc->lm_sensors[n].rfact;
}

#define INVALIDATE_SENSOR(x)						\
	do {								\
		sc->sensors[(x)].validflags &= ~ENVSYS_FCURVALID;	\
		sc->sensors[(x)].cur.data_us = 0;			\
	} while (/* CONSTCOND */ 0)

static void
lm_refresh_temp(struct lm_softc *sc, int n)
{
	int sdata;

	/*
	 * The data sheet suggests that the range of the temperature
	 * sensor is between -55 degC and +125 degC.
	 */
	sdata = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	if (sdata > 0x7d && sdata < 0xc9) {
		INVALIDATE_SENSOR(n);
	} else {
		if (sdata & 0x80)
			sdata -= 0x100;
		sc->sensors[n].validflags |= (ENVSYS_FVALID|ENVSYS_FCURVALID);
		sc->sensors[n].cur.data_us = sdata * 1000000 + 273150000;
	}
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
	if (data == 0xff || data == 0x00) {
		INVALIDATE_SENSOR(n);
	} else {
		sc->sensors[n].validflags |= (ENVSYS_FVALID|ENVSYS_FCURVALID);
		sc->sensors[n].cur.data_us = 1350000 / (data << divisor);
	}
}

static void
wb_refresh_sensor_data(struct lm_softc *sc)
{
	int banksel, bank, i;

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
		sc->sensors[n].cur.data_s = (data * 4880) + 700000;
	else
		sc->sensors[n].cur.data_s = (data * 16000);
}

static void
wb_refresh_nvolt(struct lm_softc *sc, int n)
{
	int data;

	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	sc->sensors[n].cur.data_s = ((data << 4) - WB_VREF);
	sc->sensors[n].cur.data_s *= sc->lm_sensors[n].rfact;
	sc->sensors[n].cur.data_s /= 10;
	sc->sensors[n].cur.data_s += WB_VREF * 1000;
}

static void
wb_w83627ehf_refresh_nvolt(struct lm_softc *sc, int n)
{
	int data;

	data = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg);
	sc->sensors[n].cur.data_s = ((data << 3) - WB_W83627EHF_VREF);
	sc->sensors[n].cur.data_s *= RFACT(232, 10);
	sc->sensors[n].cur.data_s /= 10;
	sc->sensors[n].cur.data_s += WB_W83627EHF_VREF * 1000;
}

static void
wb_refresh_temp(struct lm_softc *sc, int n)
{
	int sdata;

	/*
	 * The data sheet suggests that the range of the temperature
	 * sensor is between -55 degC and +125 degC.  However, values
	 * around -48 degC seem to be a very common bogus values.
	 * Since such values are unreasonably low, we use -45 degC for
	 * the lower limit instead.
	 */
	sdata = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg) << 1;
	sdata += (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg + 1) >> 7;
	if (sdata > 0x0fa && sdata < 0x1a6) {
		INVALIDATE_SENSOR(n);
	} else {
		if (sdata & 0x100)
			sdata -= 0x200;
		sc->sensors[n].validflags |= (ENVSYS_FVALID|ENVSYS_FCURVALID);
		sc->sensors[n].cur.data_us = sdata * 500000 + 273150000;
	}
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
	if (data == 0xff || data == 0x00) {
		INVALIDATE_SENSOR(n);
	} else {
		sc->sensors[n].validflags |= (ENVSYS_FVALID|ENVSYS_FCURVALID);
		sc->sensors[n].cur.data_us = 1350000 / (data << divisor);
	}
}

static void
wb_w83792d_refresh_fanrpm(struct lm_softc *sc, int n)
{
	int reg, shift, data, divisor = 1;

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
	if (data == 0xff || data == 0x00) {
		INVALIDATE_SENSOR(n);
	} else {
		if (reg != 0)
			divisor = ((*sc->lm_readreg)(sc, reg) >> shift) & 0x7;
		sc->sensors[n].validflags |= (ENVSYS_FVALID|ENVSYS_FCURVALID);
		sc->sensors[n].cur.data_us = 1350000 / (data << divisor);
	}
}

static void
as_refresh_temp(struct lm_softc *sc, int n)
{
	int sdata;

	/*
	 * It seems a shorted temperature diode produces an all-ones
	 * bit pattern.
	 */
	sdata = (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg) << 1;
	sdata += (*sc->lm_readreg)(sc, sc->lm_sensors[n].reg + 1) >> 7;
	if (sdata == 0x1ff) {
		INVALIDATE_SENSOR(n);
	} else {
		if (sdata & 0x100)
			sdata -= 0x200;
		sc->sensors[n].validflags |= (ENVSYS_FVALID|ENVSYS_FCURVALID);
		sc->sensors[n].cur.data_us = sdata * 500000 + 273150000;
	}
}

#undef INVALIDATE_SENSOR

static int
lm_gtredata(struct sysmon_envsys *sme, envsys_tre_data_t *tred)
{
	static const struct timeval onepointfive = { 1, 500000 };
	struct timeval t, utv;
	struct lm_softc *sc = sme->sme_cookie;

	/* read new values at most once every 1.5 seconds */
	getmicrouptime(&utv);
	timeradd(&sc->lastread, &onepointfive, &t);
	if (timercmp(&utv, &t, >)) {
		sc->lastread = utv;
		sc->refresh_sensor_data(sc);
	}

	*tred = sc->sensors[tred->sensor];

	return 0;
}

static int
generic_streinfo_fan(struct lm_softc *sc, envsys_basic_info_t *info, int n,
	envsys_basic_info_t *binfo)
{
	uint8_t sdata;
	int divisor;

	/* FAN1 and FAN2 can have divisors set, but not FAN3 */
	if ((sc->info[binfo->sensor].units == ENVSYS_SFANRPM)
	    && (n < 2)) {
		if (binfo->rpms == 0) {
			binfo->validflags = 0;
			return 0;
		}

		/* write back the nominal FAN speed  */
		info->rpms = binfo->rpms;

		/* 153 is the nominal FAN speed value */
		divisor = 1350000 / (binfo->rpms * 153);

		/* ...but we need lg(divisor) */
		if (divisor <= 1)
		    divisor = 0;
		else if (divisor <= 2)
		    divisor = 1;
		else if (divisor <= 4)
		    divisor = 2;
		else
		    divisor = 3;

		/*
		 * FAN1 div is in bits <5:4>, FAN2 div is
		 * in <7:6>
		 */
		sdata = (*sc->lm_readreg)(sc, LMD_VIDFAN);
		if ( n == 0 ) {  /* FAN1 */
		    divisor <<= 4;
		    sdata = (sdata & 0xCF) | divisor;
		} else { /* FAN2 */
		    divisor <<= 6;
		    sdata = (sdata & 0x3F) | divisor;
		}

		(*sc->lm_writereg)(sc, LMD_VIDFAN, sdata);
	}
	return 0;

}

static int
lm_streinfo(struct sysmon_envsys *sme, envsys_basic_info_t *binfo)
{
	 struct lm_softc *sc = sme->sme_cookie;

	 if (sc->info[binfo->sensor].units == ENVSYS_SVOLTS_DC)
		  sc->info[binfo->sensor].rfact = binfo->rfact;
	 else {
		if (sc->info[binfo->sensor].units == ENVSYS_SFANRPM) {
			generic_streinfo_fan(sc, &sc->info[binfo->sensor],
			    binfo->sensor - 8, binfo);
		}
		strlcpy(sc->info[binfo->sensor].desc, binfo->desc,
		    sizeof(sc->info[binfo->sensor].desc));
		binfo->validflags = ENVSYS_FVALID;
	 }
	 return 0;
}

static int
wb781_streinfo(struct sysmon_envsys *sme, envsys_basic_info_t *binfo)
{
	 struct lm_softc *sc = sme->sme_cookie;
	 int divisor;
	 uint8_t sdata;
	 int i;

	 if (sc->info[binfo->sensor].units == ENVSYS_SVOLTS_DC)
		  sc->info[binfo->sensor].rfact = binfo->rfact;
	 else {
		if (sc->info[binfo->sensor].units == ENVSYS_SFANRPM) {
			if (binfo->rpms == 0) {
				binfo->validflags = 0;
				return 0;
			}

			/* write back the nominal FAN speed  */
			sc->info[binfo->sensor].rpms = binfo->rpms;

			/* 153 is the nominal FAN speed value */
			divisor = 1350000 / (binfo->rpms * 153);

			/* ...but we need lg(divisor) */
			for (i = 0; i < 7; i++) {
				if (divisor <= (1 << i))
				 	break;
			}
			divisor = i;

			if (binfo->sensor == 10 || binfo->sensor == 11) {
				/*
				 * FAN1 div is in bits <5:4>, FAN2 div
				 * is in <7:6>
				 */
				sdata = (*sc->lm_readreg)(sc, LMD_VIDFAN);
				if ( binfo->sensor == 10 ) {  /* FAN1 */
					 sdata = (sdata & 0xCF) |
					     ((divisor & 0x3) << 4);
				} else { /* FAN2 */
					 sdata = (sdata & 0x3F) |
					     ((divisor & 0x3) << 6);
				}
				(*sc->lm_writereg)(sc, LMD_VIDFAN, sdata);
			} else {
				/* FAN3 is in WB_PIN <7:6> */
				sdata = (*sc->lm_readreg)(sc, WB_PIN);
				sdata = (sdata & 0x3F) |
				     ((divisor & 0x3) << 6);
				(*sc->lm_writereg)(sc, WB_PIN, sdata);
			}
		}
		strlcpy(sc->info[binfo->sensor].desc, binfo->desc,
		    sizeof(sc->info[binfo->sensor].desc));
		binfo->validflags = ENVSYS_FVALID;
	 }
	 return 0;
}

static int
wb782_streinfo(struct sysmon_envsys *sme, envsys_basic_info_t *binfo)
{
	 struct lm_softc *sc = sme->sme_cookie;
	 int divisor;
	 uint8_t sdata;
	 int i;

	 if (sc->info[binfo->sensor].units == ENVSYS_SVOLTS_DC)
		  sc->info[binfo->sensor].rfact = binfo->rfact;
	 else {
	 	if (sc->info[binfo->sensor].units == ENVSYS_SFANRPM) {
			if (binfo->rpms == 0) {
				binfo->validflags = 0;
				return 0;
			}

			/* write back the nominal FAN speed  */
			sc->info[binfo->sensor].rpms = binfo->rpms;

			/* 153 is the nominal FAN speed value */
			divisor = 1350000 / (binfo->rpms * 153);

			/* ...but we need lg(divisor) */
			for (i = 0; i < 7; i++) {
				if (divisor <= (1 << i))
				 	break;
			}
			divisor = i;

			if (binfo->sensor == 12 || binfo->sensor == 13) {
				/*
				 * FAN1 div is in bits <5:4>, FAN2 div
				 * is in <7:6>
				 */
				sdata = (*sc->lm_readreg)(sc, LMD_VIDFAN);
				if ( binfo->sensor == 12 ) {  /* FAN1 */
					 sdata = (sdata & 0xCF) |
					     ((divisor & 0x3) << 4);
				} else { /* FAN2 */
					 sdata = (sdata & 0x3F) |
					     ((divisor & 0x3) << 6);
				}
				(*sc->lm_writereg)(sc, LMD_VIDFAN, sdata);
			} else {
				/* FAN3 is in WB_PIN <7:6> */
				sdata = (*sc->lm_readreg)(sc, WB_PIN);
				sdata = (sdata & 0x3F) |
				     ((divisor & 0x3) << 6);
				(*sc->lm_writereg)(sc, WB_PIN, sdata);
			}
			/* Bit 2 of divisor is in WB_BANK0_VBAT */
			lm_generic_banksel(sc, WB_BANKSEL_B0);
			sdata = (*sc->lm_readreg)(sc, WB_BANK0_VBAT);
			sdata &= ~(0x20 << (binfo->sensor - 12));
			sdata |= (divisor & 0x4) << (binfo->sensor - 9);
			(*sc->lm_writereg)(sc, WB_BANK0_VBAT, sdata);
		}

		strlcpy(sc->info[binfo->sensor].desc, binfo->desc,
		    sizeof(sc->info[binfo->sensor].desc));
		binfo->validflags = ENVSYS_FVALID;
	}
	return 0;
}
