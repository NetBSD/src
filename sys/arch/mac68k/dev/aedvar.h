/*	$NetBSD: aedvar.h,v 1.8.12.1 2012/10/30 17:19:55 yamt Exp $	*/

/*
 * Copyright (C) 1994	Bradley A. Grantham
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/callout.h>
#include <machine/adbsys.h>

/* Event queue definitions */
#ifndef AED_MAX_EVENTS
#define AED_MAX_EVENTS 200	/* Maximum events to be kept in queue */  
				/* maybe should be higher for slower macs? */
#endif				/* AED_MAX_EVENTS */

struct aed_softc {
	struct callout sc_repeat_ch;

	/* ADB info */
	int		origaddr;	/* ADB device type (ADBADDR_AED) */
	int		adbaddr;	/* current ADB address */
	int		handler_id;	/* type of device */

	/* ADB event queue */
	adb_event_t	sc_evq[AED_MAX_EVENTS];	/* the queue */
	int		sc_evq_tail;	/* event queue tail pointer */
	int		sc_evq_len;	/* event queue length */

	/* Keyboard repeat state */
	int		sc_rptdelay;	/* ticks before auto-repeat */
	int		sc_rptinterval;	/* ticks between auto-repeat */
	int		sc_repeating;	/* key that is auto-repeating */
	adb_event_t	sc_rptevent;	/* event to auto-repeat */

	int		sc_buttons;	/* mouse button state */

	struct selinfo	sc_selinfo;	/* select() info */
	struct proc *	sc_ioproc;	/* process to wakeup */

	int		sc_open;	/* Are we queuing events? */
	int		sc_options;	/* config options */
};

/* Options */
#define AED_MSEMUL	0x1		/* emulate mouse buttons */

int	aed_input(adb_event_t *);
