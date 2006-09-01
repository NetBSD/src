/*	$NetBSD: button.h,v 1.1 2006/09/01 21:26:18 uwe Exp $	*/

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

#ifndef _MACHINE_BUTTON_H_
#define	_MACHINE_BUTTON_H_

#include <sys/ioccom.h>

#define	BUTTON_EVENT_PRESSED	0	/* button pressed */
#define	BUTTON_EVENT_RELEASED	1	/* button released */

#define	BUTTON_STATE_PRESSED	0	/* button pressed */
#define	BUTTON_STATE_RELEASED	1	/* button released */
#define	BUTTON_STATE_UNKNOWN	-1

/*
 * This structure describes the state of a button.  It is used
 * by the BUTTON_IOC_GET_SWSTATE ioctl, as well as in button
 * event messages.
 */
struct button_state {
	char	bs_name[32];		/* button name */
	int32_t	bs_state;		/* state of the switch/event */
};

/*
 * button event messages:
 *
 * We ensure that a message is always exactly 32 bytes long, so that
 * userland doesn't need to issue multiple reads to get a single event.
 */
#define	BUTTON_EVENT_MSG_SIZE	32

#define	BUTTON_EVENT_STATE_CHANGE		0

typedef struct {
	int32_t		bev_type;	/* button event type */
	union {
		int32_t	_bev_d_space[(BUTTON_EVENT_MSG_SIZE /
				      sizeof(int32_t)) - 1];

		/*
		 * This field is used for:
		 *
		 *	BUTTON_EVENT_STATE_CHANGE
		 */
		struct button_state _bev_d_event;
	} _bev_data;
} button_event_t;

#define	bev_event	_bev_data._bev_d_event

/*
 * BUTTON_IOC_GET_TYPE:
 *
 *	Get the button back-end type.
 */
struct button_type {
	char	button_type[32];
};
#define	BUTTON_IOC_GET_TYPE	 _IOR('P', 1, sizeof(struct button_type))

#endif /* _MACHINE_BUTTON_H_ */
