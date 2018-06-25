/*	$NetBSD: hidms.c,v 1.1.2.1 2018/06/25 07:25:50 pgoyette Exp $	*/

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

/*
 * HID spec: http://www.usb.org/developers/devclass_docs/HID1_11.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hidms.c,v 1.1.2.1 2018/06/25 07:25:50 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/hid/hid.h>
#include <dev/hid/hidms.h>

#ifdef HIDMS_DEBUG
#define DPRINTF(x)	if (hidmsdebug) printf x
#define DPRINTFN(n,x)	if (hidmsdebug>(n)) printf x
int	hidmsdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define HIDMS_BUT(i) ((i) == 1 || (i) == 2 ? 3 - (i) : i)

#define PS2LBUTMASK	x01
#define PS2RBUTMASK	x02
#define PS2MBUTMASK	x04
#define PS2BUTMASK 0x0f

static const struct {
	u_int feature;
	u_int flag;
} digbut[] = {
	{ HUD_TIP_SWITCH, HIDMS_TIP_SWITCH },
	{ HUD_SEC_TIP_SWITCH, HIDMS_SEC_TIP_SWITCH },
	{ HUD_BARREL_SWITCH, HIDMS_BARREL_SWITCH },
	{ HUD_ERASER, HIDMS_ERASER },
};

#define MOUSE_FLAGS_MASK (HIO_CONST|HIO_RELATIVE)

bool
hidms_setup(device_t self, struct hidms *ms, int id, void *desc, int size)
{
	uint32_t flags;
	struct hid_location *zloc;
	bool isdigitizer;
	int i, hl;

	isdigitizer = hid_is_collection(desc, size, id,
	    HID_USAGE2(HUP_DIGITIZERS, 0x0002));

	if (!hid_locate(desc, size, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_X),
	       id, hid_input, &ms->hidms_loc_x, &flags)) {
		aprint_error("\n%s: mouse has no X report\n",
		       device_xname(self));
		return false;
	}
	switch (flags & MOUSE_FLAGS_MASK) {
	case 0:
		ms->flags |= HIDMS_ABS;
		break;
	case HIO_RELATIVE:
		break;
	default:
		aprint_error("\n%s: X report 0x%04x not supported\n",
		       device_xname(self), flags);
		return false;
	}

	if (!hid_locate(desc, size, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Y),
	       id, hid_input, &ms->hidms_loc_y, &flags)) {
		aprint_error("\n%s: mouse has no Y report\n",
		       device_xname(self));
		return false;
	}
	switch (flags & MOUSE_FLAGS_MASK) {
	case 0:
		ms->flags |= HIDMS_ABS;
		break;
	case HIO_RELATIVE:
		break;
	default:
		aprint_error("\n%s: Y report 0x%04x not supported\n",
		       device_xname(self), flags);
		return false;
	}

	/* Try the wheel first as the Z activator since it's tradition. */
	hl = hid_locate(desc,
			size,
			HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_WHEEL),
			id,
			hid_input,
			&ms->hidms_loc_z,
			&flags);

	zloc = &ms->hidms_loc_z;
	if (hl) {
		if ((flags & MOUSE_FLAGS_MASK) != HIO_RELATIVE) {
			aprint_verbose("\n%s: Wheel report 0x%04x not "
			    "supported\n", device_xname(self),
			    flags);
			ms->hidms_loc_z.size = 0;	/* Bad Z coord, ignore it */
		} else {
			ms->flags |= HIDMS_Z;
			/* Wheels need the Z axis reversed. */
			ms->flags ^= HIDMS_REVZ;
			/* Put Z on the W coordinate */
			zloc = &ms->hidms_loc_w;
		}
	}

	hl = hid_locate(desc,
			size,
			HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Z),
			id,
			hid_input,
			zloc,
			&flags);

	/*
	 * The horizontal component of the scrollball can also be given by
	 * Application Control Pan in the Consumer page, so if we didnt see
	 * any Z then check that.
	 */
	if (!hl) {
		hl = hid_locate(desc,
				size,
				HID_USAGE2(HUP_CONSUMER, HUC_AC_PAN),
				id,
				hid_input,
				zloc,
				&flags);
	}

	if (hl) {
		if ((flags & MOUSE_FLAGS_MASK) != HIO_RELATIVE) {
			aprint_verbose("\n%s: Z report 0x%04x not supported\n",
			       device_xname(self), flags);
			zloc->size = 0;	/* Bad Z coord, ignore it */
		} else {
			if (ms->flags & HIDMS_Z)
				ms->flags |= HIDMS_W;
			else
				ms->flags |= HIDMS_Z;
		}
	}

	/* figure out the number of buttons */
	for (i = 1; i <= MAX_BUTTONS; i++)
		if (!hid_locate(desc, size, HID_USAGE2(HUP_BUTTON, i),
		    id, hid_input, &ms->hidms_loc_btn[i - 1], 0))
			break;

	if (isdigitizer) {
		ms->flags |= HIDMS_DIGITIZER;
		for (size_t j = 0; j < __arraycount(digbut); j++) {
			if (hid_locate(desc, size, HID_USAGE2(HUP_DIGITIZERS,
			    digbut[j].feature), id, hid_input,
			    &ms->hidms_loc_btn[i - 1], 0)) {
				if (i <= MAX_BUTTONS) {
					i++;
					ms->flags |= digbut[j].flag;
				} else
					aprint_error_dev(self,
					    "ran out of buttons\n");
			}
		}
	}
	ms->nbuttons = i - 1;
	return true;
}

void
hidms_attach(device_t self, struct hidms *ms,
    const struct wsmouse_accessops *ops)
{
	struct wsmousedev_attach_args a;
#ifdef HIDMS_DEBUG
	int i;
#endif
	aprint_normal(": %d button%s%s%s%s%s%s%s%s%s\n",
	    ms->nbuttons, ms->nbuttons == 1 ? "" : "s",
	    ms->flags & HIDMS_W ? ", W" : "",
	    ms->flags & HIDMS_Z ? " and Z dir" : "",
	    ms->flags & HIDMS_W ? "s" : "",
	    ms->flags & HIDMS_DIGITIZER ? " digitizer"  : "",
	    ms->flags & HIDMS_TIP_SWITCH ? ", tip" : "",
	    ms->flags & HIDMS_SEC_TIP_SWITCH ? ", sec tip" : "",
	    ms->flags & HIDMS_BARREL_SWITCH ? ", barrel" : "",
	    ms->flags & HIDMS_ERASER ? ", eraser" : "");

#ifdef HIDMS_DEBUG
	DPRINTF(("hidms_attach: ms=%p\n", ms));
	DPRINTF(("hidms_attach: X\t%d/%d\n",
		 ms->hidms_loc_x.pos, ms->hidms_loc_x.size));
	DPRINTF(("hidms_attach: Y\t%d/%d\n",
		 ms->hidms_loc_y.pos, ms->hidms_loc_y.size));
	if (ms->flags & HIDMS_Z)
		DPRINTF(("hidms_attach: Z\t%d/%d\n",
			 ms->hidms_loc_z.pos, ms->hidms_loc_z.size));
	if (ms->flags & HIDMS_W)
		DPRINTF(("hidms_attach: W\t%d/%d\n",
			 ms->hidms_loc_w.pos, ms->hidms_loc_w.size));
	for (i = 1; i <= ms->nbuttons; i++) {
		DPRINTF(("hidms_attach: B%d\t%d/%d\n",
			 i, ms->hidms_loc_btn[i-1].pos,ms->hidms_loc_btn[i-1].size));
	}
#endif

	a.accessops = ops;
	a.accesscookie = device_private(self);

	ms->hidms_wsmousedev = config_found(self, &a, wsmousedevprint);

	return;
}


void
hidms_intr(struct hidms *ms, void *ibuf, u_int len)
{
	int dx, dy, dz, dw;
	uint32_t buttons = 0;
	int i, flags, s;

	DPRINTFN(5,("hidms_intr: len=%d\n", len));

	flags = WSMOUSE_INPUT_DELTA;	/* equals 0 */

	dx =  hid_get_data(ibuf, &ms->hidms_loc_x);
	if (ms->flags & HIDMS_ABS) {
		flags |= (WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);
		dy = hid_get_data(ibuf, &ms->hidms_loc_y);
	} else
		dy = -hid_get_data(ibuf, &ms->hidms_loc_y);
	dz =  hid_get_data(ibuf, &ms->hidms_loc_z);
	dw =  hid_get_data(ibuf, &ms->hidms_loc_w);

	if (ms->flags & HIDMS_REVZ)
		dz = -dz;
	for (i = 0; i < ms->nbuttons; i++)
		if (hid_get_data(ibuf, &ms->hidms_loc_btn[i]))
			buttons |= (1 << HIDMS_BUT(i));

	if (dx != 0 || dy != 0 || dz != 0 || dw != 0 ||
	    buttons != ms->hidms_buttons) {
		DPRINTFN(10, ("hidms_intr: x:%d y:%d z:%d w:%d buttons:0x%x\n",
			dx, dy, dz, dw, buttons));
		ms->hidms_buttons = buttons;
		if (ms->hidms_wsmousedev != NULL) {
			s = spltty();
			wsmouse_input(ms->hidms_wsmousedev, buttons, dx, dy, dz,
			    dw, flags);
			splx(s);
		}
	}
}
