/* $NetBSD: envsys.h,v 1.14.8.2 2007/07/22 18:17:03 xtraeme Exp $ */

/*-
 * Copyright (c) 1999, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour, Juan Romero Pardines and Bill Squier.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#ifndef _SYS_ENVSYS_H_
#define _SYS_ENVSYS_H_

#ifndef _KERNEL
#include <stdbool.h>
#endif

#include <sys/ioccom.h>
#include <sys/power.h>

/*
 * ENVironmental SYStem version 2 (aka ENVSYS 2)
 */

#define ENVSYS_MAXSENSORS	512
#define ENVSYS_DESCLEN		32

/* struct used by a device sensor */
struct envsys_data {
	uint32_t	sensor;		/* sensor number */
	uint32_t	units;		/* type of sensor */
	uint32_t	state;		/* sensor state */
	uint32_t	flags;		/* sensor flags */
	uint32_t	rpms;		/* for fans, nominal RPMs */
	int32_t		rfact;		/* for volts, factor x 10^4 */
	int32_t		value_cur;	/* current value */
	int32_t		value_max;	/* max value */
	int32_t		value_min;	/* min value */
	int32_t		value_avg;	/* avg value */
	bool		monitor;	/* monitoring enabled/disabled */
	char		desc[ENVSYS_DESCLEN];	/* sensor description */
};

typedef struct envsys_data envsys_data_t;

/* sensor units */
enum envsys_units {
	ENVSYS_STEMP		= 0,	/* Temperature */
	ENVSYS_SFANRPM,			/* Fan RPM */
	ENVSYS_SVOLTS_AC,		/* AC Volts */
	ENVSYS_SVOLTS_DC,		/* DC Volts */
	ENVSYS_SOHMS,			/* Ohms */
	ENVSYS_SWATTS,			/* Watts */
	ENVSYS_SAMPS,			/* Ampere */
	ENVSYS_SWATTHOUR,		/* Watt hour */
	ENVSYS_SAMPHOUR,		/* Ampere hour */
	ENVSYS_INDICATOR,		/* Indicator */
	ENVSYS_INTEGER,			/* Integer */
	ENVSYS_DRIVE,			/* Drive */
	ENVSYS_NSENSORS
};

/* sensor states */
enum envsys_states {
	ENVSYS_SVALID		= 10,	/* sensor state is valid */
	ENVSYS_SINVALID,		/* sensor state is invalid */
	ENVSYS_SCRITICAL,		/* sensor state is critical */
	ENVSYS_SCRITUNDER,		/* sensor state is critical under */
	ENVSYS_SCRITOVER,		/* sensor state is critical over */
	ENVSYS_SWARNUNDER,		/* sensor state is warn under */
	ENVSYS_SWARNOVER		/* sensor state is warn over */
};

/* sensor drive states */
enum envsys_drive_states {
	ENVSYS_DRIVE_EMPTY	= 1,	/* drive is empty */
	ENVSYS_DRIVE_READY,		/* drive is ready */
	ENVSYS_DRIVE_POWERUP,		/* drive is powered up */
	ENVSYS_DRIVE_ONLINE,		/* drive is online */
	ENVSYS_DRIVE_IDLE,		/* drive is idle */
	ENVSYS_DRIVE_ACTIVE,		/* drive is active */
	ENVSYS_DRIVE_REBUILD,		/* drive is rebuilding */
	ENVSYS_DRIVE_POWERDOWN,		/* drive is powered down */
	ENVSYS_DRIVE_FAIL,		/* drive failed */
	ENVSYS_DRIVE_PFAIL		/* drive is degraded */
};

/* sensor flags */
#define ENVSYS_FPERCENT 	0x00000001	/* sensor wants a percentage */
#define ENVSYS_FVALID_MAX	0x00000002	/* max value is ok */
#define ENVSYS_FVALID_MIN	0x00000004	/* min value is ok */
#define ENVSYS_FVALID_AVG	0x00000008	/* avg value is ok */
#define ENVSYS_FCHANGERFACT	0x00000010	/* sensor can change rfact */

/* monitoring flags */
#define ENVSYS_FMONCRITICAL	0x00000020	/* monitor a critical state */
#define ENVSYS_FMONCRITUNDER	0x00000040	/* monitor a critunder state */
#define ENVSYS_FMONCRITOVER	0x00000080	/* monitor a critover state */
#define ENVSYS_FMONWARNUNDER	0x00000100	/* monitor a warnunder state */
#define ENVSYS_FMONWARNOVER	0x00000200	/* monitor a warnover state */
#define ENVSYS_FMONDRVSTATE	0x00000400	/* monitor a drive state */
#define ENVSYS_FMONNOTSUPP	0x00000800	/* monitoring not supported */
#define ENVSYS_FNOTVALID 	0x00001000	/* sensor is invalid */

#define ENVSYS_GETDICTIONARY		_IOWR('E', 0, struct plistref)
#define ENVSYS_SETDICTIONARY		_IOWR('E', 1, struct plistref)

/*
 * Compatibility with old interface. Only ENVSYS_GTREDATA
 * and ENVSYS_GTREINFO ioctls are supported.
 */

/* get sensor data */

struct envsys_tre_data {
	u_int sensor;
	union {				/* all data is given */
		uint32_t data_us;	/* in microKelvins, */
		int32_t data_s;		/* rpms, volts, amps, */
	} cur, min, max, avg;		/* ohms, watts, etc */
					/* see units below */

	uint32_t	warnflags;	/* warning flags */
	uint32_t	validflags;	/* sensor valid flags */
	u_int		units;		/* type of sensor */
};
typedef struct envsys_tre_data envsys_tre_data_t;

/* flags for warnflags */
#define ENVSYS_WARN_OK		0x00000000  /* All is well */
#define ENVSYS_WARN_UNDER	0x00000001  /* an under condition */
#define ENVSYS_WARN_CRITUNDER	0x00000002  /* a critical under condition */
#define ENVSYS_WARN_OVER	0x00000004  /* an over condition */
#define ENVSYS_WARN_CRITOVER	0x00000008  /* a critical over condition */

/* drive status */
#define ENVSYS_DRIVE_EMPTY      1
#define ENVSYS_DRIVE_READY      2
#define ENVSYS_DRIVE_POWERUP    3
#define ENVSYS_DRIVE_ONLINE     4
#define ENVSYS_DRIVE_IDLE       5
#define ENVSYS_DRIVE_ACTIVE     6
#define ENVSYS_DRIVE_REBUILD    7
#define ENVSYS_DRIVE_POWERDOWN  8
#define ENVSYS_DRIVE_FAIL       9
#define ENVSYS_DRIVE_PFAIL      10

#ifdef ENVSYSUNITNAMES
static const char * const envsysunitnames[] = {
    "degC", "RPM", "VAC", "V", "Ohms", "W",
    "A", "Wh", "Ah", "bool", "integer", "drive", "Unk"
};
static const char * const envsysdrivestatus[] = {
    "unknown", "empty", "ready", "powering up", "online", "idle", "active",
    "rebuilding", "powering down", "failed", "degraded"
};
#endif


/* flags for validflags */
#define ENVSYS_FVALID		0x00000001  /* sensor is valid */
#define ENVSYS_FCURVALID	0x00000002  /* cur for this sens is valid */
#define ENVSYS_FMINVALID	0x00000004  /* min for this sens is valid */
#define ENVSYS_FMAXVALID	0x00000008  /* max for this sens is valid */
#define ENVSYS_FAVGVALID	0x00000010  /* avg for this sens is valid */
#define ENVSYS_FFRACVALID	0x00000020  /* display fraction of max */

#define ENVSYS_GTREDATA _IOWR('E', 2, envsys_tre_data_t)

/* set and check sensor info */

struct envsys_basic_info {
	u_int	sensor;		/* sensor number */
	u_int	units;		/* type of sensor */
	char	desc[33];	/* sensor description */
	u_int	rfact;		/* for volts, (int)(factor x 10^4) */
	u_int	rpms;		/* for fans, set nominal RPMs */
	uint32_t validflags;	/* sensor valid flags */
};
typedef struct envsys_basic_info envsys_basic_info_t;

#define ENVSYS_GTREINFO _IOWR('E', 4, envsys_basic_info_t)

#endif /* _SYS_ENVSYS_H_ */
