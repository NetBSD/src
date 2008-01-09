/* $NetBSD: sysmon_envsys_tables.c,v 1.3.4.3 2008/01/09 01:54:34 matt Exp $ */

/*-
 * Copyright (c) 2007 Juan Romero Pardines.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysmon_envsys_tables.c,v 1.3.4.3 2008/01/09 01:54:34 matt Exp $");

#include <sys/types.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_envsysvar.h>

/*
 * Available units type descriptions.
 */
static const struct sme_description_table sme_units_description[] = {
	{ ENVSYS_STEMP,		PENVSYS_TYPE_TEMP,	"Temperature" },
	{ ENVSYS_SFANRPM,	PENVSYS_TYPE_FAN,	"Fan" },
	{ ENVSYS_SVOLTS_AC,	PENVSYS_TYPE_VOLTAGE,	"Voltage AC" },
	{ ENVSYS_SVOLTS_DC,	PENVSYS_TYPE_VOLTAGE,	"Voltage DC" },
	{ ENVSYS_SOHMS,		PENVSYS_TYPE_RESISTANCE,"Ohms" },
	{ ENVSYS_SWATTS,	PENVSYS_TYPE_POWER,	"Watts" },
	{ ENVSYS_SAMPS,		PENVSYS_TYPE_POWER,	"Ampere" },
	{ ENVSYS_SWATTHOUR,	PENVSYS_TYPE_BATTERY,	"Watt hour" },
	{ ENVSYS_SAMPHOUR,	PENVSYS_TYPE_BATTERY,	"Ampere hour" },
	{ ENVSYS_INDICATOR,	PENVSYS_TYPE_INDICATOR,	"Indicator" },
	{ ENVSYS_INTEGER,	PENVSYS_TYPE_INDICATOR,	"Integer" },
	{ ENVSYS_DRIVE,		PENVSYS_TYPE_DRIVE,	"Drive" },
	{ ENVSYS_BATTERY_CAPACITY, PENVSYS_TYPE_BATTERY,"Battery capacity" },
	{ ENVSYS_BATTERY_CHARGE, -1,			"Battery charge" },
	{ -1,			-1,			"unknown" }
};

/*
 * Available sensor state descriptions.
 */
static const struct sme_description_table sme_state_description[] = {
	{ ENVSYS_SVALID,	-1, 	"valid" },
	{ ENVSYS_SINVALID,	-1, 	"invalid" },
	{ ENVSYS_SCRITICAL,	-1, 	"critical" },
	{ ENVSYS_SCRITUNDER,	-1, 	"critical-under" },
	{ ENVSYS_SCRITOVER,	-1, 	"critical-over" },
	{ ENVSYS_SWARNUNDER,	-1, 	"warning-under" },
	{ ENVSYS_SWARNOVER,	-1, 	"warning-over" },
	{ -1,			-1, 	"unknown" }
};

/*
 * Available drive state descriptions.
 */
static const struct sme_description_table sme_drivestate_description[] = {
	{ ENVSYS_DRIVE_EMPTY,		-1, 	"unknown" },
	{ ENVSYS_DRIVE_READY,		-1, 	"ready" },
	{ ENVSYS_DRIVE_POWERUP,		-1, 	"powering up" },
	{ ENVSYS_DRIVE_ONLINE,		-1, 	"online" },
	{ ENVSYS_DRIVE_IDLE,		-1, 	"idle" },
	{ ENVSYS_DRIVE_ACTIVE,		-1, 	"active" },
	{ ENVSYS_DRIVE_REBUILD,		-1, 	"rebuilding" },
	{ ENVSYS_DRIVE_POWERDOWN,	-1, 	"powering down" },
	{ ENVSYS_DRIVE_FAIL,		-1, 	"failed" },
	{ ENVSYS_DRIVE_PFAIL,		-1, 	"degraded" },
	{ ENVSYS_DRIVE_MIGRATING,	-1,	"migrating" },
	{ -1,				-1, 	"unknown" }
};

/*
 * Available battery capacity descriptions.
 */
static const struct sme_description_table sme_batterycap_description[] = {
	{ ENVSYS_BATTERY_CAPACITY_NORMAL,	-1,	"NORMAL" },
	{ ENVSYS_BATTERY_CAPACITY_WARNING,	-1, 	"WARNING" },
	{ ENVSYS_BATTERY_CAPACITY_CRITICAL,	-1, 	"CRITICAL" },
	{ ENVSYS_BATTERY_CAPACITY_LOW,		-1,	"LOW" },
	{ -1,					-1, 	"UNKNOWN" }
};

/*
 * Returns the table associated with type.
 */
const struct sme_description_table *
sme_get_description_table(int type)
{
	const struct sme_description_table *ud = sme_units_description;
	const struct sme_description_table *sd = sme_state_description;
	const struct sme_description_table *dd = sme_drivestate_description;
	const struct sme_description_table *bd = sme_batterycap_description;

	switch (type) {
	case SME_DESC_UNITS:
		return ud;
	case SME_DESC_STATES:
		return sd;
	case SME_DESC_DRIVE_STATES:
		return dd;
	case SME_DESC_BATTERY_CAPACITY:
		return bd;
	default:
		return NULL;
	}
}
