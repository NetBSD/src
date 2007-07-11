/*	$NetBSD: pmsvar.h,v 1.4.32.1 2007/07/11 20:08:07 mjf Exp $	*/

/*-
 * Copyright (c) 2004 Kentaro Kurahone.
 * Copyright (c) 2004 Ales Krenek.
 * Copyright (c) 1994 Charles M. Hannum.
 * Copyright (c) 1992, 1993 Erik Forsberg.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL I BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_PCKBCPORT_PMSVAR_H_
#define _DEV_PCKBCPORT_PMSVAR_H_

enum pms_type {
	PMS_UNKNOWN,
	PMS_STANDARD,
	PMS_SCROLL3,
	PMS_SCROLL5,
	PMS_SYNAPTICS
};

struct pms_protocol {
	int rates[3];
	int response;
	const char *name;
};

struct pms_softc {		/* driver status information */
	struct device sc_dev;

	pckbport_tag_t sc_kbctag;
	int sc_kbcslot;

	int sc_enabled;		/* input enabled? */
#ifndef PMS_DISABLE_POWERHOOK
	void *sc_powerhook;	/* cookie from power hook */
	int sc_suspended;	/* suspended? */
#endif /* !PMS_DISABLE_POWERHOOK */
	int inputstate;		/* number of bytes received for this packet */
	u_int buttons;		/* mouse button status */
	enum pms_type protocol;
	unsigned char packet[6];
	struct timeval last, current;

	struct device *sc_wsmousedev;
	struct lwp *sc_event_thread;

#ifdef PMS_SYNAPTICS_TOUCHPAD
	union {
		struct synaptics_softc synaptics;
	} u;
#endif
};

#endif
