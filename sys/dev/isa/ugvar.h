/*     $NetBSD: ugvar.h,v 1.2 2007/01/20 18:18:51 xtraeme Exp $ */

/*
 * Copyright (c) 2007 Mihai Chelaru <kefren@netbsd.ro>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_ISA_UGVAR_H
#define _DEV_ISA_UGVAR_H

#define UG_DRV_VERSION	1000

#define UG_DELAY_CYCLES	5000
#define UG_NUM_SENSORS	19
#define UG_MAX_SENSORS	32

/* Data and Cmd offsets - Base is ussualy 0xE0 */
#define UG_CMD		0
#define UG_DATA		4

/* Temp and Voltage Sensors */
#define UG_CPUTEMP	0x2100
#define UG_SYSTEMP	0x2101
#define UG_HTV		0x2102
#define UG_VCORE	0x2103
#define UG_DDRVDD	0x2104
#define UG_3V3		0x2105
#define UG_5V		0x2106
#define UG_NBVDD	0x2108
#define UG_AGP		0x2109
#define UG_DDRVTT	0x210A
#define UG_5VSB		0x210B
#define UG_3VDUAL	0x210D
#define UG_SBVDD	0x210E
#define UG_PWMTEMP	0x210F

/* Fans */
#define UG_CPUFAN	0x2600
#define UG_NBFAN	0x2601
#define UG_SYSFAN	0x2602
#define UG_AUXFAN1	0x2603
#define UG_AUXFAN2	0x2604

/* RFacts */
#define UG_RFACT	1000
#define UG_RFACT3	3490 * UG_RFACT / 255
#define UG_RFACT4	4360 * UG_RFACT / 255
#define UG_RFACT6	6250 * UG_RFACT / 255
#define UG_RFACT_FAN	15300/255

/*
 * sc->sensors sub-intervals for each unit type.
 */
static const struct envsys_range ug_ranges[] = {
	{ 0,	2,	ENVSYS_STEMP },
	{ 14,	18,	ENVSYS_SFANRPM },
	{ 1,	0,	ENVSYS_SVOLTS_AC },	/* None */
	{ 3,	13,	ENVSYS_SVOLTS_DC },
	{ 1,	0,	ENVSYS_SOHMS },		/* None */
	{ 1,	0,	ENVSYS_SWATTS },	/* None */
	{ 1,	0,	ENVSYS_SAMPS }		/* None */
};

struct ug_softc {
	struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct sysmon_envsys sc_sysmon;
	envsys_tre_data_t sc_data[UG_MAX_SENSORS];
	envsys_basic_info_t sc_info[UG_MAX_SENSORS];
	uint8_t version;
	void *mbsens;
};

#endif /* _DEV_ISA_UGVAR_H_ */
