/* $NetBSD: envsys.h,v 1.4.4.1 2000/07/30 17:55:18 bouyer Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour and Bill Squier.
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

#include <sys/ioccom.h>

/* Returns API Version * 1000 */

#define ENVSYS_VERSION _IOR('E', 0, int32_t)

/* returns the range for a particular sensor */

struct envsys_range {
	u_int low;
	u_int high;
	u_int units;			/* see GTREDATA */
};
typedef struct envsys_range envsys_range_t;

#define ENVSYS_GRANGE _IOWR('E', 1, envsys_range_t)

/* get sensor data */

struct envsys_tre_data {
	u_int sensor;
	union {				/* all data is given */
		u_int32_t data_us;	/* in microKelvins, */
		int32_t data_s;		/* rpms, volts, amps, */
	} cur, min, max, avg;		/* ohms, watts, etc */
					/* see units below */

	u_int32_t	warnflags;	/* warning flags */
	u_int32_t	validflags;	/* sensor valid flags */
	u_int		units;		/* type of sensor */
};
typedef struct envsys_tre_data envsys_temp_data_t;
typedef struct envsys_tre_data envsys_rpm_data_t;
typedef struct envsys_tre_data envsys_electrical_data_t;
typedef struct envsys_tre_data envsys_tre_data_t;

/* flags for warnflags */
#define ENVSYS_WARN_OK		0x00000000  /* All is well */
#define ENVSYS_WARN_UNDER	0x00000001  /* an under condition */
#define ENVSYS_WARN_CRITUNDER	0x00000002  /* a critical under condition */
#define ENVSYS_WARN_OVER	0x00000004  /* an over condition */
#define ENVSYS_WARN_CRITOVER	0x00000008  /* a critical over condition */

/* type of sensor for units */
enum envsys_units {
	ENVSYS_STEMP		= 0,
	ENVSYS_SFANRPM,
	ENVSYS_SVOLTS_AC,
	ENVSYS_SVOLTS_DC,
	ENVSYS_SOHMS,
	ENVSYS_SWATTS,
	ENVSYS_SAMPS,
	ENVSYS_NSENSORS,
};

/* flags for validflags */
#define ENVSYS_FVALID		0x00000001  /* sensor is valid */
#define ENVSYS_FCURVALID	0x00000002  /* cur for this sens is valid */
#define ENVSYS_FMINVALID	0x00000004  /* min for this sens is valid */
#define ENVSYS_FMAXVALID	0x00000008  /* max for this sens is valid */
#define ENVSYS_FAVGVALID	0x00000010  /* avg for this sens is valid */

#define ENVSYS_GTREDATA _IOWR('E', 2, envsys_temp_data_t)

/* set and check sensor info */

struct envsys_basic_info {
	u_int	sensor;		/* sensor number */
	u_int	units;		/* type of sensor */
	char	desc[33];	/* sensor description */
	u_int	rfact;		/* for volts, (int)(factor x 10^4) */
	u_int	rpms;		/* for fans, set nominal RPMs */
	u_int32_t validflags;	/* sensor valid flags */
};
typedef struct envsys_basic_info envsys_temp_info_t;
typedef struct envsys_basic_info envsys_rpm_info_t;
typedef struct envsys_basic_info envsys_electrical_info_t;
typedef struct envsys_basic_info envsys_basic_info_t;

#define ENVSYS_STREINFO _IOWR('E', 3, envsys_temp_info_t)
#define ENVSYS_GTREINFO _IOWR('E', 4, envsys_temp_info_t)

#endif /* _SYS_ENVSYS_H_ */
