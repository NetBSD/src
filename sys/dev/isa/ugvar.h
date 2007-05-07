/*     $NetBSD: ugvar.h,v 1.4 2007/05/07 07:48:28 xtraeme Exp $ */

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

/*
 * Abit uGuru (first version)
 */

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

/* Voltage and Fan sensors offsets */
#define UG_VOLT_MIN	3
#define UG_FAN_MIN	14

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

/*
 * Abit uGuru2 or uGuru 2005 settings
 */ 

/* Sensor banks */
#define UG2_SETTINGS_BANK		0x01
#define UG2_SENSORS_BANK		0x08
#define UG2_MISC_BANK			0x09

/* Sensor offsets */
#define UG2_ALARMS_OFFSET		0x1E
#define UG2_SETTINGS_OFFSET		0x24
#define UG2_VALUES_OFFSET		0x80

/* Misc Sensor */
#define UG2_BOARD_ID			0x0A

/* sensor types */
#define UG2_VOLTAGE_SENSOR		0
#define UG2_TEMP_SENSOR			1
#define UG2_FAN_SENSOR			2

/* uGuru status flags */
#define UG2_STATUS_READY_FOR_READ	0x01
#define UG2_STATUS_BUSY			0x02
/* No more than 32 sensors */
#define UG2_MAX_NO_SENSORS 32

struct ug2_sensor_info {
	const char *name;
	int port;
	int type;
	int multiplier;
	int divisor;
	int offset;
};

struct ug2_motherboard_info {
	uint16_t id;
	const char *name;
	struct ug2_sensor_info sensors[UG2_MAX_NO_SENSORS + 1];
};


/* Unknown board should be the last. Now is 0x0016 */
#define UG_MAX_MSB_BOARD 0x00
#define UG_MAX_LSB_BOARD 0x16
#define UG_MIN_LSB_BOARD 0x0c

/*
 * Imported from linux driver
 */

struct ug2_motherboard_info ug2_mb[] = {
	{ 0x000C, "unknown. Please send-pr(1)", {
		{ "CPU Core", 0, 0, 10, 1, 0 },
		{ "DDR", 1, 0, 10, 1, 0 },
		{ "DDR VTT", 2, 0, 10, 1, 0 },
		{ "CPU VTT 1.2V", 3, 0, 10, 1, 0 },
		{ "MCH & PCIE 1.5V", 4, 0, 10, 1, 0 },
		{ "MCH 2.5V", 5, 0, 20, 1, 0 },
		{ "ICH 1.05V", 6, 0, 10, 1, 0 },
		{ "ATX +12V (24-Pin)", 7, 0, 60, 1, 0 },
		{ "ATX +12V (4-pin)", 8, 0, 60, 1, 0 },
		{ "ATX +5V", 9, 0, 30, 1, 0 },
		{ "+3.3V", 10, 0, 20, 1, 0 },
		{ "5VSB", 11, 0, 30, 1, 0 },
		{ "CPU", 24, 1, 1, 1, 0 },
		{ "System", 25, 1, 1, 1, 0 },
		{ "PWM", 26, 1, 1, 1, 0 },
		{ "CPU Fan", 32, 2, 60, 1, 0 },
		{ "NB Fan", 33, 2, 60, 1, 0 },
		{ "SYS FAN", 34, 2, 60, 1, 0 },
		{ "AUX1 Fan", 35, 2, 60, 1, 0 },
		{ NULL, 0, 0, 0, 0, 0 } }
	},
	{ 0x000D, "Abit AW8", {
		{ "CPU Core", 0, 0, 10, 1, 0 },
		{ "DDR", 1, 0, 10, 1, 0 },
		{ "DDR VTT", 2, 0, 10, 1, 0 },
		{ "CPU VTT 1.2V", 3, 0, 10, 1, 0 },
		{ "MCH & PCIE 1.5V", 4, 0, 10, 1, 0 },
		{ "MCH 2.5V", 5, 0, 20, 1, 0 },
		{ "ICH 1.05V", 6, 0, 10, 1, 0 },
		{ "ATX +12V (24-Pin)", 7, 0, 60, 1, 0 },
		{ "ATX +12V (4-pin)", 8, 0, 60, 1, 0 },
		{ "ATX +5V", 9, 0, 30, 1, 0 },
		{ "+3.3V", 10, 0, 20, 1, 0 },
		{ "5VSB", 11, 0, 30, 1, 0 },
		{ "CPU", 24, 1, 1, 1, 0 },
		{ "System", 25, 1, 1, 1, 0 },
		{ "PWM1", 26, 1, 1, 1, 0 },
		{ "PWM2", 27, 1, 1, 1, 0 },
		{ "PWM3", 28, 1, 1, 1, 0 },
		{ "PWM4", 29, 1, 1, 1, 0 },
		{ "CPU Fan", 32, 2, 60, 1, 0 },
		{ "NB Fan", 33, 2, 60, 1, 0 },
		{ "SYS Fan", 34, 2, 60, 1, 0 },
		{ "AUX1 Fan", 35, 2, 60, 1, 0 },
		{ "AUX2 Fan", 36, 2, 60, 1, 0 },
		{ "AUX3 Fan", 37, 2, 60, 1, 0 },
		{ "AUX4 Fan", 38, 2, 60, 1, 0 },
		{ "AUX5 Fan", 39, 2, 60, 1, 0 },
		{ NULL, 0, 0, 0, 0, 0 } }
	},
	{ 0x000E, "Abit AL8", {
		{ "CPU Core", 0, 0, 10, 1, 0 },
		{ "DDR", 1, 0, 10, 1, 0 },
		{ "DDR VTT", 2, 0, 10, 1, 0 },
		{ "CPU VTT 1.2V", 3, 0, 10, 1, 0 },
		{ "MCH & PCIE 1.5V", 4, 0, 10, 1, 0 },
		{ "MCH 2.5V", 5, 0, 20, 1, 0 },
		{ "ICH 1.05V", 6, 0, 10, 1, 0 },
		{ "ATX +12V (24-Pin)", 7, 0, 60, 1, 0 },
		{ "ATX +12V (4-pin)", 8, 0, 60, 1, 0 },
		{ "ATX +5V", 9, 0, 30, 1, 0 },
		{ "+3.3V", 10, 0, 20, 1, 0 },
		{ "5VSB", 11, 0, 30, 1, 0 },
		{ "CPU", 24, 1, 1, 1, 0 },
		{ "System", 25, 1, 1, 1, 0 },
		{ "PWM", 26, 1, 1, 1, 0 },
		{ "CPU Fan", 32, 2, 60, 1, 0 },
		{ "NB Fan", 33, 2, 60, 1, 0 },
		{ "SYS Fan", 34, 2, 60, 1, 0 },
		{ NULL, 0, 0, 0, 0, 0 } }
	},
	{ 0x000F, "unknown. Please send-pr(1)", {
		{ "CPU Core", 0, 0, 10, 1, 0 },
		{ "DDR", 1, 0, 10, 1, 0 },
		{ "DDR VTT", 2, 0, 10, 1, 0 },
		{ "CPU VTT 1.2V", 3, 0, 10, 1, 0 },
		{ "MCH & PCIE 1.5V", 4, 0, 10, 1, 0 },
		{ "MCH 2.5V", 5, 0, 20, 1, 0 },
		{ "ICH 1.05V", 6, 0, 10, 1, 0 },
		{ "ATX +12V (24-Pin)", 7, 0, 60, 1, 0 },
		{ "ATX +12V (4-pin)", 8, 0, 60, 1, 0 },
		{ "ATX +5V", 9, 0, 30, 1, 0 },
		{ "+3.3V", 10, 0, 20, 1, 0 },
		{ "5VSB", 11, 0, 30, 1, 0 },
		{ "CPU", 24, 1, 1, 1, 0 },
		{ "System", 25, 1, 1, 1, 0 },
		{ "PWM", 26, 1, 1, 1, 0 },
		{ "CPU Fan", 32, 2, 60, 1, 0 },
		{ "NB Fan", 33, 2, 60, 1, 0 },
		{ "SYS Fan", 34, 2, 60, 1, 0 },
		{ NULL, 0, 0, 0, 0, 0 } }
	},
	{ 0x0010, "Abit NI8 SLI GR", {
		{ "CPU Core", 0, 0, 10, 1, 0 },
		{ "DDR", 1, 0, 10, 1, 0 },
		{ "DDR VTT", 2, 0, 10, 1, 0 },
		{ "CPU VTT 1.2V", 3, 0, 10, 1, 0 },
		{ "NB 1.4V", 4, 0, 10, 1, 0 },
		{ "SB 1.5V", 6, 0, 10, 1, 0 },
		{ "ATX +12V (24-Pin)", 7, 0, 60, 1, 0 },
		{ "ATX +12V (4-pin)", 8, 0, 60, 1, 0 },
		{ "ATX +5V", 9, 0, 30, 1, 0 },
		{ "+3.3V", 10, 0, 20, 1, 0 },
		{ "5VSB", 11, 0, 30, 1, 0 },
		{ "CPU", 24, 1, 1, 1, 0 },
		{ "SYS", 25, 1, 1, 1, 0 },
		{ "PWM", 26, 1, 1, 1, 0 },
		{ "CPU Fan", 32, 2, 60, 1, 0 },
		{ "NB Fan", 33, 2, 60, 1, 0 },
		{ "SYS Fan", 34, 2, 60, 1, 0 },
		{ "AUX1 Fan", 35, 2, 60, 1, 0 },
		{ "OTES1 Fan", 36, 2, 60, 1, 0 },
		{ NULL, 0, 0, 0, 0, 0 } }
	},
	{ 0x0011, "Abit AT8 32X", {
		{ "CPU Core", 0, 0, 10, 1, 0 },
		{ "DDR", 1, 0, 20, 1, 0 },
		{ "DDR VTT", 2, 0, 10, 1, 0 },
		{ "CPU VDDA 2.5V", 6, 0, 20, 1, 0 },
		{ "NB 1.8V", 4, 0, 10, 1, 0 },
		{ "NB 1.8V Dual", 5, 0, 10, 1, 0 },
		{ "HTV 1.2", 3, 0, 10, 1, 0 },
		{ "PCIE 1.2V", 12, 0, 10, 1, 0 },
		{ "NB 1.2V", 13, 0, 10, 1, 0 },
		{ "ATX +12V (24-Pin)", 7, 0, 60, 1, 0 },
		{ "ATX +12V (4-pin)", 8, 0, 60, 1, 0 },
		{ "ATX +5V", 9, 0, 30, 1, 0 },
		{ "+3.3V", 10, 0, 20, 1, 0 },
		{ "5VSB", 11, 0, 30, 1, 0 },
		{ "CPU", 24, 1, 1, 1, 0 },
		{ "NB", 25, 1, 1, 1, 0 },
		{ "System", 26, 1, 1, 1, 0 },
		{ "PWM", 27, 1, 1, 1, 0 },
		{ "CPU Fan", 32, 2, 60, 1, 0 },
		{ "NB Fan", 33, 2, 60, 1, 0 },
		{ "SYS Fan", 34, 2, 60, 1, 0 },
		{ "AUX1 Fan", 35, 2, 60, 1, 0 },
		{ "AUX2 Fan", 36, 2, 60, 1, 0 },
		{ NULL, 0, 0, 0, 0, 0 } }
	},
	{ 0x0012, "unknown. Please send-pr(1)", {
		{ "CPU Core", 0, 0, 10, 1, 0 },
		{ "DDR", 1, 0, 20, 1, 0 },
		{ "DDR VTT", 2, 0, 10, 1, 0 },
		{ "HyperTransport", 3, 0, 10, 1, 0 },
		{ "CPU VDDA 2.5V", 5, 0, 20, 1, 0 },
		{ "NB", 4, 0, 10, 1, 0 },
		{ "SB", 6, 0, 10, 1, 0 },
		{ "ATX +12V (24-Pin)", 7, 0, 60, 1, 0 },
		{ "ATX +12V (4-pin)", 8, 0, 60, 1, 0 },
		{ "ATX +5V", 9, 0, 30, 1, 0 },
		{ "+3.3V", 10, 0, 20, 1, 0 },
		{ "5VSB", 11, 0, 30, 1, 0 },
		{ "CPU", 24, 1, 1, 1, 0 },
		{ "SYS", 25, 1, 1, 1, 0 },
		{ "PWM", 26, 1, 1, 1, 0 },
		{ "CPU Fan", 32, 2, 60, 1, 0 },
		{ "NB Fan", 33, 2, 60, 1, 0 },
		{ "SYS Fan", 34, 2, 60, 1, 0 },
		{ "AUX1 Fan", 36, 2, 60, 1, 0 },
		{ NULL, 0, 0, 0, 0, 0 } }
	},
	{ 0x0013, "unknown. Please send-pr(1)", {
		{ "CPU Core", 0, 0, 10, 1, 0 },
		{ "DDR", 1, 0, 10, 1, 0 },
		{ "DDR VTT", 2, 0, 10, 1, 0 },
		{ "CPU VTT 1.2V", 3, 0, 10, 1, 0 },
		{ "MCH & PCIE 1.5V", 4, 0, 10, 1, 0 },
		{ "MCH 2.5V", 5, 0, 20, 1, 0 },
		{ "ICH 1.05V", 6, 0, 10, 1, 0 },
		{ "ATX +12V (24-Pin)", 7, 0, 60, 1, 0 },
		{ "ATX +12V (4-pin)", 8, 0, 60, 1, 0 },
		{ "ATX +5V", 9, 0, 30, 1, 0 },
		{ "+3.3V", 10, 0, 20, 1, 0 },
		{ "5VSB", 11, 0, 30, 1, 0 },
		{ "CPU", 24, 1, 1, 1, 0 },
		{ "System", 25, 1, 1, 1, 0 },
		{ "PWM1", 26, 1, 1, 1, 0 },
		{ "PWM2", 27, 1, 1, 1, 0 },
		{ "PWM3", 28, 1, 1, 1, 0 },
		{ "PWM4", 29, 1, 1, 1, 0 },
		{ "CPU Fan", 32, 2, 60, 1, 0 },
		{ "NB Fan", 33, 2, 60, 1, 0 },
		{ "SYS Fan", 34, 2, 60, 1, 0 },
		{ "AUX1 Fan", 35, 2, 60, 1, 0 },
		{ "AUX2 Fan", 36, 2, 60, 1, 0 },
		{ "AUX3 Fan", 37, 2, 60, 1, 0 },
		{ "AUX4 Fan", 38, 2, 60, 1, 0 },
		{ NULL, 0, 0, 0, 0, 0 } }
	},
	{ 0x0014, "Abit AB9 Pro", {
		{ "CPU Core", 0, 0, 10, 1, 0 },
		{ "DDR", 1, 0, 10, 1, 0 },
		{ "DDR VTT", 2, 0, 10, 1, 0 },
		{ "CPU VTT 1.2V", 3, 0, 10, 1, 0 },
		{ "MCH & PCIE 1.5V", 4, 0, 10, 1, 0 },
		{ "MCH 2.5V", 5, 0, 20, 1, 0 },
		{ "ICH 1.05V", 6, 0, 10, 1, 0 },
		{ "ATX +12V (24-Pin)", 7, 0, 60, 1, 0 },
		{ "ATX +12V (4-pin)", 8, 0, 60, 1, 0 },
		{ "ATX +5V", 9, 0, 30, 1, 0 },
		{ "+3.3V", 10, 0, 20, 1, 0 },
		{ "5VSB", 11, 0, 30, 1, 0 },
		{ "CPU", 24, 1, 1, 1, 0 },
		{ "System", 25, 1, 1, 1, 0 },
		{ "PWM", 26, 1, 1, 1, 0 },
		{ "CPU Fan", 32, 2, 60, 1, 0 },
		{ "NB Fan", 33, 2, 60, 1, 0 },
		{ "SYS Fan", 34, 2, 60, 1, 0 },
		{ NULL, 0, 0, 0, 0, 0 } }
	},
	{ 0x0015, "unknown. Please send-pr(1)", {
		{ "CPU Core", 0, 0, 10, 1, 0 },
		{ "DDR", 1, 0, 20, 1, 0 },
		{ "DDR VTT", 2, 0, 10, 1, 0 },
		{ "HyperTransport", 3, 0, 10, 1, 0 },
		{ "CPU VDDA 2.5V", 5, 0, 20, 1, 0 },
		{ "NB", 4, 0, 10, 1, 0 },
		{ "SB", 6, 0, 10, 1, 0 },
		{ "ATX +12V (24-Pin)", 7, 0, 60, 1, 0 },
		{ "ATX +12V (4-pin)", 8, 0, 60, 1, 0 },
		{ "ATX +5V", 9, 0, 30, 1, 0 },
		{ "+3.3V", 10, 0, 20, 1, 0 },
		{ "5VSB", 11, 0, 30, 1, 0 },
		{ "CPU", 24, 1, 1, 1, 0 },
		{ "SYS", 25, 1, 1, 1, 0 },
		{ "PWM", 26, 1, 1, 1, 0 },
		{ "CPU Fan", 32, 2, 60, 1, 0 },
		{ "NB Fan", 33, 2, 60, 1, 0 },
		{ "SYS Fan", 34, 2, 60, 1, 0 },
		{ "AUX1 Fan", 33, 2, 60, 1, 0 },
		{ "AUX2 Fan", 35, 2, 60, 1, 0 },
		{ "AUX3 Fan", 36, 2, 60, 1, 0 },
		{ NULL, 0, 0, 0, 0, 0 } }
	},
	{ 0x0016, "generic", {
		{ "CPU Core", 0, 0, 10, 1, 0 },
		{ "DDR", 1, 0, 20, 1, 0 },
		{ "DDR VTT", 2, 0, 10, 1, 0 },
		{ "CPU VTT 1.2V", 3, 0, 10, 1, 0 },
		{ "MCH & PCIE 1.5V", 4, 0, 10, 1, 0 },
		{ "MCH 2.5V", 5, 0, 20, 1, 0 },
		{ "ICH 1.05V", 6, 0, 10, 1, 0 },
		{ "ATX +12V (24-Pin)", 7, 0, 60, 1, 0 },
		{ "ATX +12V (4-pin)", 8, 0, 60, 1, 0 },
		{ "ATX +5V", 9, 0, 30, 1, 0 },
		{ "+3.3V", 10, 0, 20, 1, 0 },
		{ "5VSB", 11, 0, 30, 1, 0 },
		{ "CPU", 24, 1, 1, 1, 0 },
		{ "System", 25, 1, 1, 1, 0 },
		{ "PWM", 26, 1, 1, 1, 0 },
		{ "CPU Fan", 32, 2, 60, 1, 0 },
		{ "NB Fan", 33, 2, 60, 1, 0 },
		{ "SYS FAN", 34, 2, 60, 1, 0 },
		{ "AUX1 Fan", 35, 2, 60, 1, 0 },
		{ NULL, 0, 0, 0, 0, 0 } }
	},
	{ 0x0000, NULL, { { NULL, 0, 0, 0, 0, 0 } } }
};

#endif /* _DEV_ISA_UGVAR_H_ */
