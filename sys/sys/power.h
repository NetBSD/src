/*	$NetBSD: power.h,v 1.4.32.1 2007/07/11 20:12:33 mjf Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Definitions for power management.
 */

#ifndef _SYS_POWER_H_
#define	_SYS_POWER_H_

#include <sys/ioccom.h>

/*
 * Power Switches:
 *
 * Power switches are devices on the system that are used by the system
 * operator to cause certain types of power management events to happen.
 * This may be the closing of a laptop lid, the pressing of a power button,
 * or some other type of user-initiated hardware event.
 *
 * We define the following types of power switches:
 *
 *	Power button		This is the "on/off" button on a system,
 *				or another button which provides a similar
 *				function.  If there is no power management
 *				daemon present, an event on this button will
 *				cause a semi-graceful shutdown of the system
 *				to occur.  This kind of button doesn't keep
 *				state; we only know (care) if an event occurs.
 *
 *	Reset button		This is the "reset" button on a system, or
 *				another button which provides a similar
 *				function.  If there is no power management
 *				daemon present, an event on this button will
 *				cause a semi-graceful reboot of the system
 *				to occur.  This kind of button doesn't keep
 *				state; we only know (care) if an event occurs.
 *
 *	Sleep button		This is a button which is dedicated to a
 *				"sleep" function.  This kind of button doesn't
 *				keep state; we only know (care) if an event
 *				occurs.
 *
 *	Lid switch		This is e.g. the lid of a laptop.  This kind
 *				of switch has state.  We know if it is open
 *				or closed.
 *
 */

#define	PSWITCH_TYPE_POWER	0	/* power button */
#define	PSWITCH_TYPE_SLEEP	1	/* sleep button */
#define	PSWITCH_TYPE_LID	2	/* lid switch */
#define	PSWITCH_TYPE_RESET	3	/* reset button */
#define	PSWITCH_TYPE_ACADAPTER	4	/* AC adapter presence */

#define	PSWITCH_EVENT_PRESSED	0	/* button pressed, lid closed, AC off */
#define	PSWITCH_EVENT_RELEASED	1	/* button released, lid open, AC on */

/*
 * This structure describes the state of a power switch.
 */
struct pswitch_state {
	char	psws_name[16];		/* power switch name */
	int32_t	psws_type;		/* type of switch (qualifier) */
	int32_t	psws_state;		/* state of the switch/event */
};

/*
 * envsys(4) events:
 *
 * envsys events are sent by the sysmon envsys framework when
 * a critical condition happens in a sensor.
 *
 * We define the folowing types of envsys events:
 *
 *	sensor temperature	To handle temperature sensors.
 *
 *	sensor voltage		To handle voltage sensors (AC/DC).
 *
 *	sensor power		To handle power sensors (W/Ampere).
 *
 *	sensor resistance	To handle resistance sensors (Ohms).
 *
 *	sensor battery		To handle battery sensors (Ah/Wh).
 *
 *	sensor fan		To handle fan sensors.
 *
 *	sensor drive		To handle drive sensors.
 *
 * 	sensor indicator	To handle indicator sensors.
 */

#define PENVSYS_TYPE_TEMP		10
#define PENVSYS_TYPE_VOLTAGE		11
#define PENVSYS_TYPE_POWER		12
#define PENVSYS_TYPE_RESISTANCE 	13
#define PENVSYS_TYPE_BATTERY		14
#define PENVSYS_TYPE_FAN		15
#define PENVSYS_TYPE_DRIVE		16
#define PENVSYS_TYPE_INDICATOR		17

/*
 * The following events apply for temperatures, power, resistance, 
 * voltages, battery and fan sensors:
 *
 * 	PENVSYS_EVENT_CRITICAL		A critical limit.
 *
 * 	PENVSYS_EVENT_CRITOVER		A critical over limit.
 *
 * 	PENVSYS_EVENT_CRITUNDER		A critical under limit.
 *
 * 	PENVSYS_EVENT_WARNOVER		A warning under limit.
 *
 * 	PENVSYS_EVENT_WARNUNDER		A warning over limit.
 *
 * The following events apply to the same except for batteries:
 *
 * 	PENVSYS_EVENT_USER_CRITMAX	User critical max limit.
 *
 * 	PENVSYS_EVENT_USER_CRITMIN	User critical min limit.
 *
 * The folowing event apply to all sensors, when the state is
 * valid or the critical limit is not valid anymore:
 *
 * 	PENVSYS_EVENT_NORMAL		Normal state in the sensor.
 */

#define PENVSYS_EVENT_NORMAL		 90
#define PENVSYS_EVENT_CRITICAL 		100
#define PENVSYS_EVENT_CRITOVER 		110
#define PENVSYS_EVENT_CRITUNDER 	120
#define PENVSYS_EVENT_WARNOVER 		130
#define PENVSYS_EVENT_WARNUNDER 	140
#define PENVSYS_EVENT_USER_CRITMAX	150
#define PENVSYS_EVENT_USER_CRITMIN	160

/*
 * The following events apply for battery sensors:
 *
 * 	PENVSYS_EVENT_BATT_USERCAP	User capacity.
 *
 */

#define PENVSYS_EVENT_BATT_USERCAP	170

/*
 * The following events apply for drive sensors:
 *
 * 	PENVSYS_EVENT_DRIVE_STCHANGED	Drive state changed.
 *
 */
#define PENVSYS_EVENT_DRIVE_STCHANGED	180


/*
 * This structure defines the properties of an envsys event.
 */
struct penvsys_state {
	char	pes_dvname[16];		/* device name */
	char	pes_sensname[32];	/* sensor name */
	char	pes_statedesc[64];	/* sensor state description */
	int32_t	pes_type;		/* envsys power type */
	int32_t	pes_state;		/* state for the event */
};

/*
 * Power management event messages:
 *
 * We ensure that a message is always exactly 32 bytes long, so that
 * userland doesn't need to issue multiple reads to get a single event.
 */
#define	POWER_EVENT_MSG_SIZE	32

#define	POWER_EVENT_SWITCH_STATE_CHANGE		0
#define POWER_EVENT_ENVSYS_STATE_CHANGE		1

typedef struct power_event {
	int32_t		pev_type;	/* power event type */
	union {
		int32_t	 _pev_d_space[(POWER_EVENT_MSG_SIZE /
				       sizeof(int32_t)) - 1];

		/*
		 * This field is used for:
		 *
		 * 	POWER_EVENT_SWITCH_STATE_CHANGE
		 */
		struct pswitch_state _pev_d_switch;
	} _pev_data;
} power_event_t;

#define pev_switch	_pev_data._pev_d_switch

#define POWER_EVENT_RECVDICT	_IOWR('P', 1, struct plistref)

/*
 * POWER_IOC_GET_TYPE:
 *
 *	Get the power management back-end type.
 */
struct power_type {
	char	power_type[32];
};
#define	POWER_IOC_GET_TYPE	 _IOR('P', 0, sizeof(struct power_type))

#endif /* _SYS_POWER_H_ */
