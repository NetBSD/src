/*	$NetBSD: power.h,v 1.3 2004/05/03 07:41:47 kochi Exp $	*/

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
 */

#define	PSWITCH_TYPE_POWER	0	/* power button */
#define	PSWITCH_TYPE_SLEEP	1	/* sleep button */
#define	PSWITCH_TYPE_LID	2	/* lid switch */
#define	PSWITCH_TYPE_RESET	3	/* reset button */
#define	PSWITCH_TYPE_ACADAPTER	4	/* AC adapter presence */

#define	PSWITCH_EVENT_PRESSED	0	/* button pressed, lid closed,
					   AC adapter online */
#define	PSWITCH_EVENT_RELEASED	1	/* button released, lid open,
					   AC adapter offline */

#define	PSWITCH_STATE_PRESSED	0	/* button pressed/lid closed */
#define	PSWITCH_STATE_RELEASED	1	/* button released/lid open */
#define	PSWITCH_STATE_UNKNOWN	-1

/*
 * This structure describes the state of a power switch.  It is used
 * by the POWER_IOC_GET_SWSTATE ioctl, as well as in power mangement
 * event messages.
 */
struct pswitch_state {
	char	psws_name[16];		/* power switch name */
	int32_t	psws_type;		/* type of switch (qualifier) */
	int32_t	psws_state;		/* state of the switch/event */
};

/*
 * Power management event messages:
 *
 * We ensure that a message is always exactly 32 bytes long, so that
 * userland doesn't need to issue multiple reads to get a single event.
 */
#define	POWER_EVENT_MSG_SIZE	32

#define	POWER_EVENT_SWITCH_STATE_CHANGE		0

typedef struct {
	int32_t		pev_type;	/* power event type */
	union {
		int32_t	_pev_d_space[(POWER_EVENT_MSG_SIZE /
				      sizeof(int32_t)) - 1];

		/*
		 * This field is used for:
		 *
		 *	POWER_EVENT_SWITCH_STATE_CHANGE
		 */
		struct pswitch_state _pev_d_switch;
	} _pev_data;
} power_event_t;

#define	pev_switch	_pev_data._pev_d_switch

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
