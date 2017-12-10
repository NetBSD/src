/*	$NetBSD: hidms.h,v 1.1 2017/12/10 17:03:07 bouyer Exp $	*/

/*
 * Copyright (c) 1998, 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#define MAX_BUTTONS	31	/* must not exceed size of sc_buttons */

struct hidms {
	struct hid_location hidms_loc_x, hidms_loc_y, hidms_loc_z, hidms_loc_w;
	struct hid_location hidms_loc_btn[MAX_BUTTONS];

	u_int flags;		/* device configuration */
#define HIDMS_Z			0x001	/* z direction available */
#define HIDMS_SPUR_BUT_UP	0x002	/* spurious button up events */
#define HIDMS_REVZ		0x004	/* Z-axis is reversed */
#define HIDMS_W			0x008	/* w direction/tilt available */
#define HIDMS_ABS		0x010	/* absolute position, touchpanel */
#define HIDMS_TIP_SWITCH  	0x020	/* digitizer tip switch */
#define HIDMS_SEC_TIP_SWITCH 	0x040	/* digitizer secondary tip switch */
#define HIDMS_BARREL_SWITCH 	0x080	/* digitizer barrel switch */
#define HIDMS_ERASER 		0x100	/* digitizer eraser */
#define HIDMS_DIGITIZER 	0x200	/* digitizer */

	int nbuttons;

	uint32_t hidms_buttons;	/* mouse button status */
	device_t hidms_wsmousedev;
};

bool hidms_setup(device_t, struct hidms *, int, void *, int);
void hidms_attach(device_t, struct hidms *, const struct wsmouse_accessops *);
void hidms_intr(struct hidms *, void *, u_int);

