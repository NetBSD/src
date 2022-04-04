/*	$NetBSD: synaptics.c,v 1.78 2022/04/04 07:04:20 blymn Exp $	*/

/*
 * Copyright (c) 2005, Steve C. Woodford
 * Copyright (c) 2004, Ales Krenek
 * Copyright (c) 2004, Kentaro A. Kurahone
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the authors nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * TODO:
 *	- Make the sysctl values per-instance instead of global.
 *	- Consider setting initial scaling factors at runtime according
 *	  to the values returned by the 'Read Resolutions' command.
 *	- Support the serial protocol (we only support PS/2 for now)
 *	- Support auto-repeat for up/down button Z-axis emulation.
 *	- Maybe add some more gestures (can we use Palm support somehow?)
 */

#include "opt_pms.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: synaptics.c,v 1.78 2022/04/04 07:04:20 blymn Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <sys/bus.h>

#include <dev/pckbport/pckbportvar.h>

#include <dev/pckbport/synapticsreg.h>
#include <dev/pckbport/synapticsvar.h>

#include <dev/pckbport/pmsreg.h>
#include <dev/pckbport/pmsvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

/*
 * Absolute-mode packets are decoded and passed around using
 * the following structure.
 */
struct synaptics_packet {
	signed short	sp_x;	/* Unscaled absolute X/Y coordinates */
	signed short	sp_y;
	u_char	sp_z;		/* Z (pressure) */
	signed short	sp_sx;	/* Unscaled absolute X/Y coordinates */
	signed short	sp_sy;  /* for secondary finger */
	u_char	sp_sz;		/* Z (pressure) */
	u_char	sp_w;		/* W (contact patch width) */
	u_char  sp_primary;	/* seen primary finger packet */
	u_char  sp_secondary;	/* seen secondary finger packet */
	u_char	sp_finger_status; /* seen extended finger packet */
	u_char	sp_finger_count; /* number of fingers seen */
	char	sp_left;	/* Left mouse button status */
	char	sp_right;	/* Right mouse button status */
	char	sp_middle;	/* Middle button status (possibly emulated) */
	char	sp_up;		/* Up button status */
	char	sp_down;	/* Down button status */
};

static void pms_synaptics_input(void *, int);
static void pms_synaptics_process_packet(struct pms_softc *,
		struct synaptics_packet *);
static void pms_sysctl_synaptics(struct sysctllog **);
static int pms_sysctl_synaptics_verify(SYSCTLFN_ARGS);

/* Controlled by sysctl. */
static int synaptics_up_down_emul = 3;
static int synaptics_up_down_motion_delta = 1;
static int synaptics_gesture_move = 200;
static int synaptics_gesture_length = 20;
static int synaptics_edge_left = SYNAPTICS_EDGE_LEFT;
static int synaptics_edge_right = SYNAPTICS_EDGE_RIGHT;
static int synaptics_edge_top = SYNAPTICS_EDGE_TOP;
static int synaptics_edge_bottom = SYNAPTICS_EDGE_BOTTOM;
static int synaptics_edge_motion_delta = 32;
static u_int synaptics_finger_high = SYNAPTICS_FINGER_LIGHT + 5;
static u_int synaptics_finger_low = SYNAPTICS_FINGER_LIGHT - 10;
static int synaptics_horiz_pct = 0;
static int synaptics_vert_pct = 0;
static int synaptics_button_pct = 30;
static int synaptics_button_boundary;
static int synaptics_button2;
static int synaptics_button3;
static int synaptics_two_fingers_emul = 0;
static int synaptics_scale_x = 16;
static int synaptics_scale_y = 16;
static int synaptics_scale_z = 32;
static int synaptics_max_speed_x = 32;
static int synaptics_max_speed_y = 32;
static int synaptics_max_speed_z = 2;
static int synaptics_movement_threshold = 4;
static int synaptics_movement_enable = 1;
static int synaptics_button_region_movement = 1;
static bool synaptics_aux_mid_button_scroll = TRUE;
static int synaptics_debug = 0;

#define	DPRINTF(LEVEL, SC, FMT, ARGS...) do					      \
{									      \
	if (synaptics_debug >= LEVEL) {						      \
		struct pms_softc *_dprintf_psc =			      \
		    container_of((SC), struct pms_softc, u.synaptics);	      \
		device_printf(_dprintf_psc->sc_dev, FMT, ##ARGS);	      \
	}								      \
} while (0)

/* Sysctl nodes. */
static int synaptics_button_boundary_nodenum;
static int synaptics_button2_nodenum;
static int synaptics_button3_nodenum;
static int synaptics_up_down_emul_nodenum;
static int synaptics_up_down_motion_delta_nodenum;
static int synaptics_gesture_move_nodenum;
static int synaptics_gesture_length_nodenum;
static int synaptics_edge_left_nodenum;
static int synaptics_edge_right_nodenum;
static int synaptics_edge_top_nodenum;
static int synaptics_edge_bottom_nodenum;
static int synaptics_edge_motion_delta_nodenum;
static int synaptics_finger_high_nodenum;
static int synaptics_finger_low_nodenum;
static int synaptics_two_fingers_emul_nodenum;
static int synaptics_scale_x_nodenum;
static int synaptics_scale_y_nodenum;
static int synaptics_scale_z_nodenum;
static int synaptics_max_speed_x_nodenum;
static int synaptics_max_speed_y_nodenum;
static int synaptics_max_speed_z_nodenum;
static int synaptics_movement_threshold_nodenum;
static int synaptics_movement_enable_nodenum;
static int synaptics_button_region_movement_nodenum;
static int synaptics_aux_mid_button_scroll_nodenum;
static int synaptics_horiz_pct_nodenum;
static int synaptics_vert_pct_nodenum;
static int synaptics_button_pct_nodenum;

/*
 * copy of edges so we can recalculate edge limit if there is 
 * vertical scroll region
 */
static int synaptics_actual_edge_right;
static int synaptics_actual_edge_bottom;

static int synaptics_old_vert_pct = 0;
static int synaptics_old_horiz_pct = 0;
static int synaptics_old_button_pct = 0;
static int synaptics_old_button_boundary = SYNAPTICS_EDGE_BOTTOM;
static int synaptics_old_horiz_edge = SYNAPTICS_EDGE_BOTTOM;
static int synaptics_old_vert_edge = SYNAPTICS_EDGE_RIGHT;

/*
 * This holds the processed packet data, it is global because multiple
 * packets from the trackpad may be processed when handling multiple
 * fingers on the trackpad to gather all the data.
 */
static struct synaptics_packet packet;

static int
synaptics_poll_cmd(struct pms_softc *psc, ...)
{
	u_char cmd[4];
	size_t i;
	va_list ap;

	va_start(ap, psc);

	for (i = 0; i < __arraycount(cmd); i++)
		if ((cmd[i] = (u_char)va_arg(ap, int)) == 0)
			break;
	va_end(ap);

	int res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot, cmd, i, 0,
    	    NULL, 0);
	if (res)
		aprint_error_dev(psc->sc_dev, "command error %#x\n", cmd[0]);
	return res;
}

static int
synaptics_poll_reset(struct pms_softc *psc)
{
	u_char resp[2];
	int res;

	u_char cmd[1] = { PMS_RESET };
	res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot, cmd, 1, 2,
	    resp, 1);
	aprint_debug_dev(psc->sc_dev, "reset %d 0x%02x 0x%02x\n",
	    res, resp[0], resp[1]);
	return res;
}

static int
synaptics_special_read(struct pms_softc *psc, u_char slice, u_char resp[3])
{
	u_char cmd[1] = { PMS_SEND_DEV_STATUS };
	int res = pms_sliced_command(psc->sc_kbctag, psc->sc_kbcslot, slice);

	return res | pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 1, 3, resp, 0);
}

static int
synaptics_special_write(struct pms_softc *psc, u_char command, u_char arg)
{
	int res = pms_sliced_command(psc->sc_kbctag, psc->sc_kbcslot, arg);
	if (res)
		return res;

	u_char cmd[2];
	cmd[0] = PMS_SET_SAMPLE;
	cmd[1] = command;
	res = pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot,
	    cmd, 2, 0, NULL, 0);
	return res;
}

static void
pms_synaptics_set_boundaries(void)
{
	if (synaptics_vert_pct != synaptics_old_vert_pct ) {
		synaptics_edge_right = synaptics_actual_edge_right -
		    ((unsigned long) synaptics_vert_pct *
		    (synaptics_actual_edge_right - synaptics_edge_left)) / 100;
		synaptics_old_vert_pct = synaptics_vert_pct;
		synaptics_old_vert_edge = synaptics_edge_right;
	}

	if (synaptics_edge_right != synaptics_old_vert_edge) {
		if (synaptics_edge_right >= synaptics_actual_edge_right) {
			synaptics_vert_pct = 0;
			synaptics_edge_right = synaptics_actual_edge_right;
		} else {
			synaptics_vert_pct = 100 -
			    ((unsigned long) 100 * synaptics_edge_right) /
			    (synaptics_actual_edge_right - synaptics_edge_left);
		}
		synaptics_old_vert_pct = synaptics_vert_pct;
		synaptics_old_vert_edge = synaptics_edge_right;
	}

	if (synaptics_horiz_pct != synaptics_old_horiz_pct ) {
		synaptics_edge_bottom = synaptics_actual_edge_bottom +
		    ((unsigned long) synaptics_horiz_pct *
		    (synaptics_edge_top - synaptics_actual_edge_bottom)) / 100;
		synaptics_old_horiz_pct = synaptics_horiz_pct;
		synaptics_old_horiz_edge = synaptics_edge_bottom;
	}

	if (synaptics_edge_bottom != synaptics_old_horiz_edge) {
		if (synaptics_edge_bottom <= synaptics_actual_edge_bottom) {
			synaptics_vert_pct = 0;
			synaptics_edge_bottom = synaptics_actual_edge_bottom;
		} else {
			synaptics_horiz_pct = 100 -
			    ((unsigned long) 100 * synaptics_edge_bottom) /
			    (synaptics_edge_top - synaptics_actual_edge_bottom);
		}
		synaptics_old_horiz_edge = synaptics_edge_bottom;
	}

	if (synaptics_button_pct != synaptics_old_button_pct) {
		synaptics_button_boundary = synaptics_edge_bottom + 
		    ((unsigned long) synaptics_button_pct * 
		    (synaptics_edge_top - synaptics_edge_bottom)) / 100;
		synaptics_old_button_pct = synaptics_button_pct;
		synaptics_old_button_boundary = synaptics_button_boundary;
	}

	if (synaptics_button_boundary != synaptics_old_button_boundary) {
		if (synaptics_button_boundary <= synaptics_edge_bottom) {
			synaptics_button_pct = 0;
			synaptics_button_boundary = synaptics_edge_bottom;
		} else {
			synaptics_button_pct = 100 -
			    ((unsigned long) 100 * synaptics_button_boundary) /
			    (synaptics_edge_top - synaptics_edge_bottom);
		}
		synaptics_old_button_boundary = synaptics_button_boundary;
	}

	/*
	 * recalculate the button boundary yet again just in case the
	 * bottom edge changed above.
	 */
	synaptics_button_boundary = synaptics_edge_bottom + 
	    ((unsigned long) synaptics_button_pct * 
	    (synaptics_edge_top - synaptics_edge_bottom)) / 100;
	synaptics_old_button_boundary = synaptics_button_boundary;

	synaptics_button2 = synaptics_edge_left +
	    (synaptics_edge_right - synaptics_edge_left) / 3;
	synaptics_button3 = synaptics_edge_left +
	    2 * (synaptics_edge_right - synaptics_edge_left) / 3;

}

static void
pms_synaptics_probe_extended(struct pms_softc *psc)
{
	struct synaptics_softc *sc = &psc->u.synaptics;
	u_char resp[3];
	int res;

	aprint_debug_dev(psc->sc_dev,
	    "synaptics_probe: Capabilities 0x%04x.\n", sc->caps);
	if (sc->caps & SYNAPTICS_CAP_PASSTHROUGH)
		sc->flags |= SYN_FLAG_HAS_PASSTHROUGH;

	if (sc->caps & SYNAPTICS_CAP_PALMDETECT)
		sc->flags |= SYN_FLAG_HAS_PALM_DETECT;

	if (sc->caps & SYNAPTICS_CAP_MULTIDETECT)
		sc->flags |= SYN_FLAG_HAS_MULTI_FINGER;

	if (sc->caps & SYNAPTICS_CAP_MULTIFINGERREPORT)
		sc->flags |= SYN_FLAG_HAS_MULTI_FINGER_REPORT;

	/* Ask about extra buttons to detect up/down. */
	if (((sc->caps & SYNAPTICS_CAP_EXTNUM) + 0x08)
	    >= SYNAPTICS_EXTENDED_QUERY)
	{
		res = synaptics_special_read(psc, SYNAPTICS_EXTENDED_QUERY, resp);
		if (res == 0) {
			sc->num_buttons = (resp[1] >> 4);
			if (sc->num_buttons > 0)
				sc->button_mask = sc->button_mask <<
				    ((sc->num_buttons + 1) >> 1);

			aprint_debug_dev(psc->sc_dev,
			    "%s: Extended Buttons: %d.\n", __func__,
			    sc->num_buttons);

			aprint_debug_dev(psc->sc_dev, "%s: Extended "
			    "Capabilities: 0x%02x 0x%02x 0x%02x.\n", __func__,
			    resp[0], resp[1], resp[2]);
			if (sc->num_buttons >= 2) {
				/* Yes. */
				sc->flags |= SYN_FLAG_HAS_UP_DOWN_BUTTONS;
			}
			if (resp[0] & 0x1) {
				/* Vertical scroll area */
				sc->flags |= SYN_FLAG_HAS_VERTICAL_SCROLL;
			}
			if (resp[0] & 0x2) {
				/* Horizontal scroll area */
				sc->flags |= SYN_FLAG_HAS_HORIZONTAL_SCROLL;
			}
			if (resp[0] & 0x4) {
				/* Extended W-Mode */
				sc->flags |= SYN_FLAG_HAS_EXTENDED_WMODE;
			}
		}
	}

	/* Ask about click pad */
	if (((sc->caps & SYNAPTICS_CAP_EXTNUM) + 0x08) >=
	    SYNAPTICS_CONTINUED_CAPABILITIES)
	{
		res = synaptics_special_read(psc,
		    SYNAPTICS_CONTINUED_CAPABILITIES, resp);

/*
 * The following describes response for the
 * SYNAPTICS_CONTINUED_CAPABILITIES query.
 *
 * byte	mask	name			meaning
 * ----	----	-------			------------
 * 0	0x01	adjustable threshold	capacitive button sensitivity
 *					can be adjusted
 * 0	0x02	report max		query 0x0d gives max coord reported
 * 0	0x04	clearpad		sensor is ClearPad product
 * 0	0x08	advanced gesture	not particularly meaningful
 * 0	0x10	clickpad bit 0		1-button ClickPad
 * 0	0x60	multifinger mode	identifies firmware finger counting
 *					(not reporting!) algorithm.
 *					Not particularly meaningful
 * 0	0x80	covered pad		W clipped to 14, 15 == pad mostly covered
 * 1	0x01	clickpad bit 1		2-button ClickPad
 * 1	0x02	deluxe LED controls	touchpad support LED commands
 *					ala multimedia control bar
 * 1	0x04	reduced filtering	firmware does less filtering on
 *					position data, driver should watch
 *					for noise.
 * 1	0x08	image sensor		image sensor tracks 5 fingers, but only
 *					reports 2.
 * 1	0x10	uniform clickpad	whole clickpad moves instead of being
 *					hinged at the top.
 * 1	0x20	report min		query 0x0f gives min coord reported
 */
		if (res == 0) {
			uint val = SYN_CCAP_VALUE(resp);

			aprint_debug_dev(psc->sc_dev, "%s: Continued "
			    "Capabilities 0x%02x 0x%02x 0x%02x.\n", __func__,
			    resp[0], resp[1], resp[2]);
			switch (SYN_CCAP_CLICKPAD_TYPE(val)) {
			case 0: /* not a clickpad */
				break;
			case 1:
				sc->flags |= SYN_FLAG_HAS_ONE_BUTTON_CLICKPAD;
				break;
			case 2:
				sc->flags |= SYN_FLAG_HAS_TWO_BUTTON_CLICKPAD;
				break;
			case 3: /* reserved */
			default:
				/* unreached */
				break;
			}

			if ((val & SYN_CCAP_HAS_ADV_GESTURE_MODE))
				sc->flags |= SYN_FLAG_HAS_ADV_GESTURE_MODE;

			if ((val & SYN_CCAP_REPORT_MAX))
				sc->flags |= SYN_FLAG_HAS_MAX_REPORT;

			if ((val & SYN_CCAP_REPORT_MIN))
				sc->flags |= SYN_FLAG_HAS_MIN_REPORT;
		}
	}
}

static const struct {
	int bit;
	const char *desc;
} syn_flags[] = {
	{ SYN_FLAG_HAS_EXTENDED_WMODE, "Extended W mode", },
	{ SYN_FLAG_HAS_PASSTHROUGH, "Passthrough", },
	{ SYN_FLAG_HAS_MIDDLE_BUTTON, "Middle button", },
	{ SYN_FLAG_HAS_BUTTONS_4_5, "Buttons 4/5", },
	{ SYN_FLAG_HAS_UP_DOWN_BUTTONS, "Up/down buttons", },
	{ SYN_FLAG_HAS_PALM_DETECT, "Palm detect", },
	{ SYN_FLAG_HAS_ONE_BUTTON_CLICKPAD, "One button click pad", },
	{ SYN_FLAG_HAS_TWO_BUTTON_CLICKPAD, "Two button click pad", },
	{ SYN_FLAG_HAS_VERTICAL_SCROLL, "Vertical scroll", },
	{ SYN_FLAG_HAS_HORIZONTAL_SCROLL, "Horizontal scroll", },
	{ SYN_FLAG_HAS_MULTI_FINGER_REPORT, "Multi-finger Report", },
	{ SYN_FLAG_HAS_MULTI_FINGER, "Multi-finger", },
	{ SYN_FLAG_HAS_MAX_REPORT, "Reports max", },
	{ SYN_FLAG_HAS_MIN_REPORT, "Reports min", },
};

int
pms_synaptics_probe_init(void *vsc)
{
	struct pms_softc *psc = vsc;
	struct synaptics_softc *sc = &psc->u.synaptics;
	u_char cmd[1], resp[3];
	int res, ver_minor, ver_major;
	struct sysctllog *clog = NULL;

	res = pms_sliced_command(psc->sc_kbctag, psc->sc_kbcslot,
	    SYNAPTICS_IDENTIFY_TOUCHPAD);
	cmd[0] = PMS_SEND_DEV_STATUS;
	res |= pckbport_poll_cmd(psc->sc_kbctag, psc->sc_kbcslot, cmd, 1, 3,
	    resp, 0);
	if (res) {
		aprint_debug_dev(psc->sc_dev,
		    "synaptics_probe: Identify Touchpad error.\n");
		/*
		 * Reset device in case the probe confused it.
		 */
 doreset:
		(void)synaptics_poll_reset(psc);
		return res;
	}

	if (resp[1] != SYNAPTICS_MAGIC_BYTE) {
		aprint_debug_dev(psc->sc_dev,
		    "synaptics_probe: Not synaptics.\n");
		res = 1;
		goto doreset;
	}

	sc->flags = 0;
	sc->num_buttons = 0;
	sc->button_mask = 0xff;

	pms_synaptics_set_boundaries();

	/* Check for minimum version and print a nice message. */
	ver_major = resp[2] & 0x0f;
	ver_minor = resp[0];
	aprint_normal_dev(psc->sc_dev, "Synaptics touchpad version %d.%d\n",
	    ver_major, ver_minor);
	if (ver_major * 10 + ver_minor < SYNAPTICS_MIN_VERSION) {
		/* No capability query support. */
		sc->caps = 0;
		goto done;
	}


	/* Query the hardware capabilities. */
	res = synaptics_special_read(psc, SYNAPTICS_READ_CAPABILITIES, resp);
	if (res) {
		/* Hmm, failed to get capabilities. */
		aprint_error_dev(psc->sc_dev,
		    "synaptics_probe: Failed to query capabilities.\n");
		goto doreset;
	}

	sc->caps = SYNAPTICS_CAP_VALUE(resp);

	if (sc->caps & SYNAPTICS_CAP_MBUTTON)
		sc->flags |= SYN_FLAG_HAS_MIDDLE_BUTTON;

	if (sc->caps & SYNAPTICS_CAP_4BUTTON)
		sc->flags |= SYN_FLAG_HAS_BUTTONS_4_5;

	if (sc->caps & SYNAPTICS_CAP_EXTENDED) {
		pms_synaptics_probe_extended(psc);
	}

	if (sc->flags) {
		const char comma[] = ", ";
		const char *sep = "";
		aprint_normal_dev(psc->sc_dev, "");
		for (size_t f = 0; f < __arraycount(syn_flags); f++) {
			if (sc->flags & syn_flags[f].bit) {
				aprint_normal("%s%s", sep, syn_flags[f].desc);
				sep = comma;
			}
		}
		aprint_normal("\n");
	}

	if (sc->flags & SYN_FLAG_HAS_MAX_REPORT) {
		res = synaptics_special_read(psc, SYNAPTICS_READ_MAX_COORDS,
		    resp);
		if (res) {
			aprint_error_dev(psc->sc_dev,
			    "synaptics_probe: Failed to query max coords.\n");
		} else {
			synaptics_edge_right = (resp[0] << 5) +
			    ((resp[1] & 0x0f) << 1);
			synaptics_edge_top = (resp[2] << 5) + 
			    ((resp[1] & 0xf0) >> 3);

			synaptics_actual_edge_right = synaptics_edge_right;

			/*
			 * If we have vertical scroll then steal 10%
			 * for that region.
			 */
			if (sc->flags & SYN_FLAG_HAS_VERTICAL_SCROLL)
				synaptics_edge_right -=
				    synaptics_edge_right / 10;

			aprint_normal_dev(psc->sc_dev,
			    "Probed max coordinates right: %d, top: %d\n",
			    synaptics_edge_right, synaptics_edge_top);
		}
	}

	if (sc->flags & SYN_FLAG_HAS_MIN_REPORT) {
		res = synaptics_special_read(psc, SYNAPTICS_READ_MIN_COORDS,
		    resp);
		if (res) {
			aprint_error_dev(psc->sc_dev,
			    "synaptics_probe: Failed to query min coords.\n");
		} else {
			synaptics_edge_left = (resp[0] << 5) +
			    ((resp[1] & 0x0f) << 1);
			synaptics_edge_bottom = (resp[2] << 5) + 
			    ((resp[1] & 0xf0) >> 3);

			synaptics_actual_edge_bottom = synaptics_edge_bottom;

			/*
			 * If we have horizontal scroll then steal 10%
			 * for that region.
			 */
			if (sc->flags & SYN_FLAG_HAS_HORIZONTAL_SCROLL)
				synaptics_horiz_pct = 10;

			aprint_normal_dev(psc->sc_dev,
			    "Probed min coordinates left: %d, bottom: %d\n",
			    synaptics_edge_left, synaptics_edge_bottom);
		}
	}

	pms_synaptics_set_boundaries();

done:
	pms_sysctl_synaptics(&clog);
	pckbport_set_inputhandler(psc->sc_kbctag, psc->sc_kbcslot,
	    pms_synaptics_input, psc, device_xname(psc->sc_dev));

	return (0);
}

void
pms_synaptics_enable(void *vsc)
{
	struct pms_softc *psc = vsc;
	struct synaptics_softc *sc = &psc->u.synaptics;
	u_char enable_modes;
	int res, i;

	if (sc->flags & SYN_FLAG_HAS_PASSTHROUGH) {
		/*
		 * Extended capability probes can confuse the passthrough
		 * device; reset the touchpad now to cure that.
		 */
		res = synaptics_poll_reset(psc);
	}

	/*
	 * Enable Absolute mode with W (width) reporting, and set
	 * the packet rate to maximum (80 packets per second). Enable
	 * extended W mode if supported so we can report second finger
	 * position.
	 */
	enable_modes =
	   SYNAPTICS_MODE_ABSOLUTE | SYNAPTICS_MODE_W | SYNAPTICS_MODE_RATE;

	if (sc->flags & SYN_FLAG_HAS_EXTENDED_WMODE)
		enable_modes |= SYNAPTICS_MODE_EXTENDED_W;

	/*
 	* Synaptics documentation says to disable device before
 	* setting mode.
 	*/
	synaptics_poll_cmd(psc, PMS_DEV_DISABLE, 0);
	/* a couple of set scales to clear out pending commands */
	for (i = 0; i < 2; i++)
		synaptics_poll_cmd(psc, PMS_SET_SCALE11, 0);

	res = synaptics_special_write(psc, SYNAPTICS_CMD_SET_MODE2, enable_modes);
	if (res)
		aprint_error("synaptics: set mode error\n");

	/* a couple of set scales to clear out pending commands */
	for (i = 0; i < 2; i++)
		synaptics_poll_cmd(psc, PMS_SET_SCALE11, 0);

	/* Set advanced gesture mode */
	if ((sc->flags & SYN_FLAG_HAS_EXTENDED_WMODE) ||
	    (sc->flags & SYN_FLAG_HAS_ADV_GESTURE_MODE))
		synaptics_special_write(psc, SYNAPTICS_WRITE_DELUXE_3, 0x3); 

	/* Disable motion in the button region for clickpads */
	if(sc->flags & SYN_FLAG_HAS_ONE_BUTTON_CLICKPAD)
		synaptics_button_region_movement = 0;

	sc->up_down = 0;
	sc->prev_fingers = 0;
	sc->gesture_start_x = sc->gesture_start_y = 0;
	sc->gesture_start_packet = 0;
	sc->gesture_tap_packet = 0;
	sc->gesture_type = 0;
	sc->gesture_buttons = 0;
	sc->total_packets = 0;
	for (i = 0; i < SYN_MAX_FINGERS; i++) {
		sc->rem_x[i] = sc->rem_y[i] = sc->rem_z[i] = 0;
	}
	sc->button_history = 0;

	/* clear the packet decode structure */
	memset(&packet, 0, sizeof(packet));
}

void
pms_synaptics_resume(void *vsc)
{
	(void)synaptics_poll_reset(vsc);
}

static void
pms_sysctl_synaptics(struct sysctllog **clog)
{
	int rc, root_num;
	const struct sysctlnode *node;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "synaptics",
	    SYSCTL_DESCR("Synaptics touchpad controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0)
	    goto err;

	root_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "up_down_emulation",
	    SYSCTL_DESCR("Middle button/Z-axis emulation with up/down buttons"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_up_down_emul,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_up_down_emul_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "up_down_motion_delta",
	    SYSCTL_DESCR("Up/down button Z-axis emulation rate"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_up_down_motion_delta,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_up_down_motion_delta_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "gesture_move",
	    SYSCTL_DESCR("Movement greater than this between taps cancels gesture"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_gesture_move,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_gesture_move_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "gesture_length",
	    SYSCTL_DESCR("Time period in which tap is recognised as a gesture"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_gesture_length,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_gesture_length_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "edge_left",
	    SYSCTL_DESCR("Define left edge of touchpad"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_edge_left,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_edge_left_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "edge_right",
	    SYSCTL_DESCR("Define right edge of touchpad"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_edge_right,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_edge_right_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "edge_top",
	    SYSCTL_DESCR("Define top edge of touchpad"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_edge_top,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_edge_top_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "edge_bottom",
	    SYSCTL_DESCR("Define bottom edge of touchpad"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_edge_bottom,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_edge_bottom_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "edge_motion_delta",
	    SYSCTL_DESCR("Define edge motion rate"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_edge_motion_delta,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_edge_motion_delta_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "finger_high",
	    SYSCTL_DESCR("Define finger applied pressure threshold"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_finger_high,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_finger_high_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "finger_low",
	    SYSCTL_DESCR("Define finger removed pressure threshold"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_finger_low,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_finger_low_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "two_fingers_emulation",
	    SYSCTL_DESCR("Map two fingers to middle button"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_two_fingers_emul,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_two_fingers_emul_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "scale_x",
	    SYSCTL_DESCR("Horizontal movement scale factor"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_scale_x,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_scale_x_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "scale_y",
	    SYSCTL_DESCR("Vertical movement scale factor"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_scale_y,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_scale_y_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "scale_z",
	    SYSCTL_DESCR("Sroll wheel emulation scale factor"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_scale_z,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_scale_z_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "max_speed_x",
	    SYSCTL_DESCR("Horizontal movement maximum speed"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_max_speed_x,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_max_speed_x_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "max_speed_y",
	    SYSCTL_DESCR("Vertical movement maximum speed"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_max_speed_y,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_max_speed_y_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "max_speed_z",
	    SYSCTL_DESCR("Scroll wheel emulation maximum speed"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_max_speed_z,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_max_speed_z_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "movement_threshold",
	    SYSCTL_DESCR("Minimum reported movement threshold"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_movement_threshold,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_movement_threshold_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "movement_enable",
	    SYSCTL_DESCR("Enable movement reporting"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_movement_enable,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_movement_enable_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "button_region_movement_enable",
	    SYSCTL_DESCR("Enable movement within clickpad button region"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_button_region_movement,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_button_region_movement_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "button_boundary",
	    SYSCTL_DESCR("Top edge of button area"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_button_boundary,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_button_boundary_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "button2_edge",
	    SYSCTL_DESCR("Left edge of button 2 region"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_button2,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_button2_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "button3_edge",
	    SYSCTL_DESCR("Left edge of button 3 region"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_button3,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_button3_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_BOOL, "aux_mid_button_scroll",
	    SYSCTL_DESCR("Interpet Y-Axis movement with the middle button held as scrolling on the passthrough device (e.g. TrackPoint)"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_aux_mid_button_scroll,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_aux_mid_button_scroll_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "vert_scroll_percent",
	    SYSCTL_DESCR("Percent of trackpad width to reserve for vertical scroll region"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_vert_pct,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_vert_pct_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "horizontal_scroll_percent",
	    SYSCTL_DESCR("Percent of trackpad height to reserve for scroll region"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_horiz_pct,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_horiz_pct_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "button_region_percent",
	    SYSCTL_DESCR("Percent of trackpad height to reserve for button region"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_button_pct,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_button_pct_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Enable debug output"),
	    NULL, 0,
	    &synaptics_debug,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	return;

err:
	aprint_error("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}

static int
pms_sysctl_synaptics_verify(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	/* Sanity check the params. */
	if (node.sysctl_num == synaptics_up_down_emul_nodenum) {
		if (t < 0 || t > 3)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_two_fingers_emul_nodenum) {
		if (t < 0 || t > 2)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_gesture_length_nodenum ||
	    node.sysctl_num == synaptics_edge_motion_delta_nodenum ||
	    node.sysctl_num == synaptics_up_down_motion_delta_nodenum ||
	    node.sysctl_num == synaptics_max_speed_x_nodenum ||
	    node.sysctl_num == synaptics_max_speed_y_nodenum ||
	    node.sysctl_num == synaptics_max_speed_z_nodenum) {
		if (t < 0)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_edge_left_nodenum ||
	    node.sysctl_num == synaptics_edge_bottom_nodenum) {
		if (t < 0 || t > (SYNAPTICS_EDGE_MAX / 2))
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_edge_right_nodenum ||
	    node.sysctl_num == synaptics_edge_top_nodenum) {
		if (t < (SYNAPTICS_EDGE_MAX / 2))
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_scale_x_nodenum ||
	    node.sysctl_num == synaptics_scale_y_nodenum ||
	    node.sysctl_num == synaptics_scale_z_nodenum) {
		if (t < 1 || t > (SYNAPTICS_EDGE_MAX / 4))
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_finger_high_nodenum) {
		if (t < 0 || t > SYNAPTICS_FINGER_PALM ||
		    t < synaptics_finger_low)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_finger_low_nodenum) {
		if (t < 0 || t > SYNAPTICS_FINGER_PALM ||
		    t > synaptics_finger_high)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_gesture_move_nodenum ||
	    node.sysctl_num == synaptics_movement_threshold_nodenum) {
		if (t < 0 || t > (SYNAPTICS_EDGE_MAX / 4))
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_button_boundary_nodenum) {
		if (t < 0 || t < synaptics_edge_bottom ||
		    t > synaptics_edge_top)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_button2_nodenum ||
	    node.sysctl_num == synaptics_button3_nodenum) {
		if (t < synaptics_edge_left || t > synaptics_edge_right)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_movement_enable_nodenum) {
		if (t < 0 || t > 1)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_button_region_movement_nodenum) {
		if (t < 0 || t > 1)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_aux_mid_button_scroll_nodenum) {
		if (t < 0 || t > 1)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_vert_pct_nodenum) {
		if (t < 0 || t > 100)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_horiz_pct_nodenum) {
		if (t < 0 || t > 100)
			return (EINVAL);
	} else
	if (node.sysctl_num == synaptics_button_pct_nodenum) {
		if (t < 0 || t > 100)
			return (EINVAL);
	} else
		return (EINVAL);

	*(int *)rnode->sysctl_data = t;

	pms_synaptics_set_boundaries();

	return (0);
}

/*
 * Extract the number of fingers from the current packet and return
 * it to the caller.
 */
static unsigned
pms_synaptics_get_fingers(struct pms_softc *psc, u_char w, short z)
{
	struct synaptics_softc *sc = &psc->u.synaptics;
	unsigned short ew_mode;
	unsigned fingers;

	fingers = 0;


	/*
	 * If w is zero and z says no fingers then return
	 * no fingers, w == can also mean 2 fingers... confusing.
	 */
	if (w == 0 && z == SYNAPTICS_FINGER_NONE)
		return 0;

	if ((sc->flags & SYN_FLAG_HAS_EXTENDED_WMODE) &&
	    (w == SYNAPTICS_WIDTH_EXTENDED_W)) {
		ew_mode = psc->packet[5] >> 4;
		switch (ew_mode)
		{
		case SYNAPTICS_EW_WHEEL:
			break;

		case SYNAPTICS_EW_SECONDARY_FINGER:
			/* to get here we must have 2 fingers at least */
			fingers = 2;
			break;

		case SYNAPTICS_EW_FINGER_STATUS:
			fingers = psc->packet[1] & 0x0f;
			break;

		default:
			aprint_error_dev(psc->sc_dev,
			    "invalid extended w mode %d\n",
			    ew_mode);
			return 0; /* pretend there are no fingers */
		}
	} else {

		fingers = 1;

		/*
		 * If SYN_FLAG_HAS_MULTI_FINGER is set then check
		 * sp_w is below SYNAPTICS_WIDTH_FINGER_MIN, if it is
		 * then this will be the finger count.
		 *
		 * There are a couple of "special" values otherwise
		 * just punt with one finger, if this really is a palm
		 * then it will be caught later.
		 */
		if (sc->flags & SYN_FLAG_HAS_MULTI_FINGER) {
			if (w == SYNAPTICS_WIDTH_TWO_FINGERS)
				fingers = 2;
			else if (w == SYNAPTICS_WIDTH_THREE_OR_MORE)
				fingers = 3;
		}

	}

	return fingers;
}

/* Masks for the first byte of a packet */
#define PMS_LBUTMASK 0x01
#define PMS_RBUTMASK 0x02
#define PMS_MBUTMASK 0x04

static void
pms_synaptics_parse(struct pms_softc *psc)
{
	struct synaptics_softc *sc = &psc->u.synaptics;
	struct synaptics_packet nsp;
	char new_buttons, ew_mode;
	uint8_t btn_mask, packet4, packet5;
	unsigned v, primary_finger, secondary_finger;
	int ext_left = -1, ext_right = -1, ext_middle = -1,
	    ext_up = -1, ext_down = -1;

	sc->total_packets++;

	memcpy(&nsp, &packet, sizeof(packet));

	/* Width of finger */
	nsp.sp_w = ((psc->packet[0] & 0x30) >> 2)
	    + ((psc->packet[0] & 0x04) >> 1)
	    + ((psc->packet[3] & 0x04) >> 2);

	v = 0;
	primary_finger = 0;
	secondary_finger = 0;
	if ((sc->flags & SYN_FLAG_HAS_EXTENDED_WMODE) &&
	    (nsp.sp_w == SYNAPTICS_WIDTH_EXTENDED_W)) {
		ew_mode = psc->packet[5] >> 4;
		switch (ew_mode)
		{
		case SYNAPTICS_EW_WHEEL:
			/* scroll wheel report, ignore for now */
			aprint_debug_dev(psc->sc_dev, "mouse wheel packet\n");
			return;

		case SYNAPTICS_EW_SECONDARY_FINGER:
			/* parse the second finger report */

			nsp.sp_secondary = 1;

			nsp.sp_sx = ((psc->packet[1] & 0xfe) << 1)
			    + ((psc->packet[4] & 0x0f) << 9);
			nsp.sp_sy = ((psc->packet[2] & 0xfe) << 1)
			    + ((psc->packet[4] & 0xf0) << 5);
			nsp.sp_sz = (psc->packet[3] & 0x30)
			    + ((psc->packet[5] & 0x0e) << 1);

			/*
			 * Check if the x and y are non-zero that they
			 * are within the bounds of the trackpad
			 * otherwise ignore the packet.
			 */
			if (((nsp.sp_sx != 0) &&
			    ((nsp.sp_sx < synaptics_edge_left) ||
			     (nsp.sp_sx > synaptics_edge_right))) ||
			   ((nsp.sp_sy != 0) &&
			    ((nsp.sp_sy < synaptics_edge_bottom) ||
			     (nsp.sp_sy > synaptics_edge_top)))) {
				sc->gesture_type = 0;
				sc->gesture_buttons = 0;
				sc->total_packets--;
				DPRINTF(20, sc,
				    "synaptics_parse: dropping out of bounds "
				    "packet sp_sx %d sp_sy %d\n",
				    nsp.sp_sx, nsp.sp_sy);
				return;
			}

			/* work out the virtual finger width */
			v = 8 + (psc->packet[1] & 0x01) +
				((psc->packet[2] & 0x01) << 1) +
				((psc->packet[5] & 0x01) << 2);

			/* keep same buttons down as primary */
			nsp.sp_left = sc->button_history & PMS_LBUTMASK;
			nsp.sp_middle = sc->button_history & PMS_MBUTMASK;
			nsp.sp_right = sc->button_history & PMS_RBUTMASK;
			break;

		case SYNAPTICS_EW_FINGER_STATUS:
			/* This works but what is it good for?
			 * it gives us an index of the primary/secondary
			 * fingers but no other packets pass this
			 * index.
			 *
			 * XXX Park this, possibly handle a finger
			 * XXX change if indexes change.
			 */
			primary_finger = psc->packet[2];
			secondary_finger = psc->packet[4];
			nsp.sp_finger_status = 1;
			nsp.sp_finger_count = pms_synaptics_get_fingers(psc,
			    nsp.sp_w, nsp.sp_z);
			goto skip_position;

		default:
			aprint_error_dev(psc->sc_dev,
			    "invalid extended w mode %d\n",
			    ew_mode);
			return;
		}
	} else {
		nsp.sp_primary = 1;

		/*
		 * If the trackpad has external buttons and one of
		 * those buttons is pressed then the lower bits of
		 * x and y are "stolen" for button status. We can tell
		 * this has happened by doing an xor of the two right
		 * button status bits residing in byte 0 and 3, if the
		 * result is non-zero then there is an external button
		 * report and the position bytes need to be masked.
		 */
		btn_mask = 0xff;
		if ((sc->num_buttons > 0) &&
		    ((psc->packet[0] & PMS_RBUTMASK) ^
		     (psc->packet[3] & PMS_RBUTMASK))) {
			btn_mask = sc->button_mask;
		}

		packet4 = psc->packet[4] & btn_mask;
		packet5 = psc->packet[5] & btn_mask;

		/*
		 * If SYN_FLAG_HAS_MULTI_FINGER is set then check
		 * sp_w is below SYNAPTICS_WIDTH_FINGER_MIN, if it is
		 * then this will be the finger count.
		 *
		 * There are a couple of "special" values otherwise
		 * just punt with one finger, if this really is a palm
		 * then it will be caught later.
		 */
		if ((sc->flags & SYN_FLAG_HAS_MULTI_FINGER) &&
		    ((nsp.sp_w == SYNAPTICS_WIDTH_TWO_FINGERS) ||
		     (nsp.sp_w == SYNAPTICS_WIDTH_THREE_OR_MORE))) {
			/*
			 * To make life interesting if there are
			 * two or more fingers on the touchpad then
			 * the coordinate reporting changes and an extra
			 * "virtual" finger width is reported.
			 */
			nsp.sp_x = (packet4 & 0xfc) +
				((packet4 & 0x02) << 1) +
				((psc->packet[1] & 0x0f) << 8) +
				((psc->packet[3] & 0x10) << 8);

			nsp.sp_y = (packet5 & 0xfc) +
				((packet5 & 0x02) << 1) +
				((psc->packet[1] & 0xf0) << 4) +
				((psc->packet[3] & 0x20) << 7);

			/* Pressure */
			nsp.sp_z = psc->packet[2] & 0xfe;

			/* derive the virtual finger width */
			v = 8 + ((packet4 & 0x02) >> 1) +
				(packet5 & 0x02) +
				((psc->packet[2] & 0x01) << 2);

		} else {
			/* Absolute X/Y coordinates of finger */
			nsp.sp_x = packet4 +
				((psc->packet[1] & 0x0f) << 8) +
				((psc->packet[3] & 0x10) << 8);

			nsp.sp_y = packet5 +
				((psc->packet[1] & 0xf0) << 4) +
				((psc->packet[3] & 0x20) << 7);

			/* Pressure */
			nsp.sp_z = psc->packet[2];
		}

		/*
		 * Check if the x and y are non-zero that they
		 * are within the bounds of the trackpad
		 * otherwise ignore the packet.
		 */
		if (((nsp.sp_x != 0) &&
		    ((nsp.sp_x < synaptics_edge_left) ||
		     (nsp.sp_x > synaptics_edge_right))) ||
		    ((nsp.sp_y != 0) &&
		    ((nsp.sp_y < synaptics_edge_bottom) ||
		     (nsp.sp_y > synaptics_edge_top)))) {
			sc->gesture_type = 0;
			sc->gesture_buttons = 0;
			sc->total_packets--;
			DPRINTF(20, sc,
			    "synaptics_parse: dropping out of bounds packet "
			    "sp_x %d sp_y %d\n",
			    nsp.sp_x, nsp.sp_y);
			return;
		}

		nsp.sp_finger_count = pms_synaptics_get_fingers(psc,
		    nsp.sp_w, nsp.sp_z);

		/*
		 * We don't have extended W so we only know if there
		 * are multiple fingers on the touchpad, only the primary
		 * location is reported so just pretend we have an
		 * unmoving second finger.
		 */
		if (((sc->flags & SYN_FLAG_HAS_EXTENDED_WMODE)
			!= SYN_FLAG_HAS_EXTENDED_WMODE) &&
		    (nsp.sp_finger_count > 1)) {
			nsp.sp_secondary = 1;
			nsp.sp_sx = 0;
			nsp.sp_sy = 0;
			nsp.sp_sz = 0;
		}

		if ((psc->packet[0] ^ psc->packet[3]) & 0x02) {
			/* extended buttons */

			aprint_debug_dev(psc->sc_dev,
			    "synaptics_parse: %02x %02x %02x %02x %02x %02x\n",
			    psc->packet[0], psc->packet[1], psc->packet[2],
			    psc->packet[3], psc->packet[4], psc->packet[5]);

			if ((psc->packet[4] & SYN_1BUTMASK) != 0)
				ext_left = PMS_LBUTMASK;
			else
				ext_left = 0;

			if ((psc->packet[4] & SYN_3BUTMASK) != 0)
				ext_middle = PMS_MBUTMASK;
			else
				ext_middle = 0;

			if ((psc->packet[5] & SYN_2BUTMASK) != 0)
				ext_right = PMS_RBUTMASK;
			else
				ext_right = 0;

			if ((psc->packet[5] & SYN_4BUTMASK) != 0)
				ext_up = 1;
			else
				ext_up = 0;

			if ((psc->packet[4] & SYN_5BUTMASK) != 0)
				ext_down = 1;
			else
				ext_down = 0;
		} else {
			/* Left/Right button handling. */
			nsp.sp_left = psc->packet[0] & PMS_LBUTMASK;
			nsp.sp_right = psc->packet[0] & PMS_RBUTMASK;
		}

		/* Up/Down buttons. */
		if (sc->flags & SYN_FLAG_HAS_BUTTONS_4_5) {
			/* Old up/down buttons. */
			nsp.sp_up = nsp.sp_left ^
		    	    (psc->packet[3] & PMS_LBUTMASK);
			nsp.sp_down = nsp.sp_right ^
		    	    (psc->packet[3] & PMS_RBUTMASK);
		} else if (sc->flags & SYN_FLAG_HAS_UP_DOWN_BUTTONS &&
	   	    ((psc->packet[0] & PMS_RBUTMASK) ^
	   	    (psc->packet[3] & PMS_RBUTMASK))) {
			/* New up/down button. */
			nsp.sp_up = psc->packet[4] & SYN_1BUTMASK;
			nsp.sp_down = psc->packet[5] & SYN_2BUTMASK;
		} else {
			nsp.sp_up = 0;
			nsp.sp_down = 0;
		}

		new_buttons = 0;
		if(sc->flags & SYN_FLAG_HAS_ONE_BUTTON_CLICKPAD) {
			/* This is not correctly specified. Read this button press
		 	* from L/U bit.  Emulate 3 buttons by checking the
		 	* coordinates of the click and returning the appropriate
		 	* button code.  Outside the button region default to a
		 	* left click.
		 	*/
			u_char bstate = (psc->packet[0] ^ psc->packet[3])
					    & 0x01;
			if (nsp.sp_y < synaptics_button_boundary) {
				if (nsp.sp_x > synaptics_button3) {
					nsp.sp_right =
			   			bstate ? PMS_RBUTMASK : 0;
				} else if (nsp.sp_x > synaptics_button2) {
					nsp.sp_middle =
				   		bstate ? PMS_MBUTMASK : 0;
				} else {
					nsp.sp_left = bstate ? PMS_LBUTMASK : 0;
				}
			} else
				nsp.sp_left = bstate ? 1 : 0;
			new_buttons = nsp.sp_left | nsp.sp_middle | nsp.sp_right;
			if (new_buttons != sc->button_history) {
				if (sc->button_history == 0)
					sc->button_history = new_buttons;
				else if (new_buttons == 0) {
					sc->button_history = 0;
				       /* ensure all buttons are cleared just in
				 	* case finger comes off in a different
				 	* region.
				 	*/
					nsp.sp_left = 0;
					nsp.sp_middle = 0;
					nsp.sp_right = 0;
				} else {
					/* make sure we keep the same button even
				 	* if the finger moves to a different
				 	* region.  This precludes chording
				 	* but, oh well.
				 	*/
					nsp.sp_left = sc->button_history & PMS_LBUTMASK;
					nsp.sp_middle = sc->button_history
				    	& PMS_MBUTMASK;
					nsp.sp_right = sc->button_history & PMS_RBUTMASK;
				}
			}
		} else if (sc->flags & SYN_FLAG_HAS_MIDDLE_BUTTON) {
			/* Old style Middle Button. */
			nsp.sp_middle = (psc->packet[0] & PMS_LBUTMASK) ^
		    	    (psc->packet[3] & PMS_LBUTMASK);
		} else {
			nsp.sp_middle = 0;
		}

		/*
		 * Overlay extended button state if anything changed,
		 * preserve the state if a button is being held.
		 */
		if (ext_left != -1)
			nsp.sp_left = sc->ext_left = ext_left;
		else if (sc->ext_left != 0)
			nsp.sp_left = sc->ext_left;

		if (ext_right != -1)
			nsp.sp_right = sc->ext_right = ext_right;
		else if (sc->ext_right != 0)
			nsp.sp_right = sc->ext_right;

		if (ext_middle != -1)
			nsp.sp_middle = sc->ext_middle = ext_middle;
		else if (sc->ext_middle != 0)
			nsp.sp_middle = sc->ext_middle;

		if (ext_up != -1)
			nsp.sp_up = sc->ext_up = ext_up;
		else if (sc->ext_up != 0)
			nsp.sp_up = sc->ext_up;

		if (ext_down != -1)
			nsp.sp_down = sc->ext_down = ext_down;
		else if (sc->ext_down != 0)
			nsp.sp_down = sc->ext_down;

		switch (synaptics_up_down_emul) {
		case 1:
			/* Do middle button emulation using up/down buttons */
			nsp.sp_middle = nsp.sp_up | nsp.sp_down;
			nsp.sp_up = nsp.sp_down = 0;
			break;
		case 3:
			/* Do left/right button emulation using up/down buttons */
			nsp.sp_left = nsp.sp_left | nsp.sp_up;
			nsp.sp_right = nsp.sp_right | nsp.sp_down;
			nsp.sp_up = nsp.sp_down = 0;
			break;
		default:
			/*
			 * Don't do any remapping...
			 * Z-axis emulation is handled in pms_synaptics_process_packet
			 */
			break;
		}
	}

	/* set the finger count only if we haven't seen an extended-w
	 * finger count status
	 */
	if (nsp.sp_finger_status == 0)
		nsp.sp_finger_count = pms_synaptics_get_fingers(psc, nsp.sp_w,
		    nsp.sp_z);

skip_position:
	DPRINTF(20, sc,
	    "synaptics_parse: sp_x %d sp_y %d sp_z %d, sp_sx %d, sp_sy %d, "
	    "sp_sz %d, sp_w %d sp_finger_count %d, sp_primary %d, "
	    "sp_secondary %d, v %d, primary_finger %d, secondary_finger %d\n",
	    nsp.sp_x, nsp.sp_y, nsp.sp_z, nsp.sp_sx,
	    nsp.sp_sy, nsp.sp_sz, nsp.sp_w, nsp.sp_finger_count, 
	    nsp.sp_primary, nsp.sp_secondary, v, primary_finger,
	    secondary_finger);


	/* If no fingers and we at least saw the primary finger
	 * or the buttons changed then process the last packet.
	 */
	if (pms_synaptics_get_fingers(psc, nsp.sp_w, nsp.sp_z) == 0 ||
	    nsp.sp_left != packet.sp_left ||
	    nsp.sp_right != packet.sp_right ||
	    nsp.sp_middle != packet.sp_middle ||
	    nsp.sp_up != packet.sp_up ||
	    nsp.sp_down != packet.sp_down) {
		if (nsp.sp_primary == 1) {
			pms_synaptics_process_packet(psc, &nsp);
			sc->packet_count[SYN_PRIMARY_FINGER] = 0;
			sc->packet_count[SYN_SECONDARY_FINGER] = 0;
		}

		/* clear the fingers seen since we have processed */
		nsp.sp_primary = 0;
		nsp.sp_secondary = 0;
		nsp.sp_finger_status = 0;
	} else if (nsp.sp_finger_count != packet.sp_finger_count) {
		/*
		 * If the number of fingers changes then send the current packet
		 * for processing and restart the process.
		 */
		if (packet.sp_primary == 1) {
			pms_synaptics_process_packet(psc, &packet);
			sc->packet_count[SYN_PRIMARY_FINGER]++;
		}

		sc->packet_count[SYN_PRIMARY_FINGER] = 0;
		sc->packet_count[SYN_SECONDARY_FINGER] = 0;
	}

	/* Only one finger, process the new packet */
	if (nsp.sp_finger_count == 1) {
		if (nsp.sp_finger_count != packet.sp_finger_count) {
			sc->packet_count[SYN_PRIMARY_FINGER] = 0;
			sc->packet_count[SYN_SECONDARY_FINGER] = 0;
		}
		pms_synaptics_process_packet(psc, &nsp);

		/* clear the fingers seen since we have processed */
		nsp.sp_primary = 0;
		nsp.sp_secondary = 0;
		nsp.sp_finger_status = 0;

		sc->packet_count[SYN_PRIMARY_FINGER]++;
	}

	/*
	 *  More than one finger and we have seen the primary and secondary
	 * fingers then process the packet.
	 */
	if ((nsp.sp_finger_count > 1) && (nsp.sp_primary == 1) 
	    && (nsp.sp_secondary == 1)) {
		if (nsp.sp_finger_count != packet.sp_finger_count) {
			sc->packet_count[SYN_PRIMARY_FINGER] = 0;
			sc->packet_count[SYN_SECONDARY_FINGER] = 0;
		}
		pms_synaptics_process_packet(psc, &nsp);

		/* clear the fingers seen since we have processed */
		nsp.sp_primary = 0;
		nsp.sp_secondary = 0;
		nsp.sp_finger_status = 0;

		sc->packet_count[SYN_PRIMARY_FINGER]++;
		sc->packet_count[SYN_SECONDARY_FINGER]++;
	}

	memcpy(&packet, &nsp, sizeof(packet));
}

/*
 * Passthrough is used for e.g. TrackPoints and additional pointing
 * devices connected to a Synaptics touchpad.
 */
static void
pms_synaptics_passthrough(struct pms_softc *psc)
{
	int dx, dy, dz;
	int buttons, changed;
	int s;

	buttons = ((psc->packet[1] & PMS_LBUTMASK) ? 0x20 : 0) |
		((psc->packet[1] & PMS_MBUTMASK) ? 0x40 : 0) |
		((psc->packet[1] & PMS_RBUTMASK) ? 0x80 : 0);

	dx = psc->packet[4];
	if (dx >= 128)
		dx -= 256;
	if (dx == -128)
		dx = -127;

	dy = psc->packet[5];
	if (dy >= 128)
		dy -= 256;
	if (dy == -128)
		dy = -127;

	dz = 0;

	changed = buttons ^ (psc->buttons & 0xe0);
	psc->buttons ^= changed;

	if (dx || dy || dz || changed) {
		s = spltty();
		/*
		 * If the middle button is held, interpret movement as
		 * scrolling.
		 */
		if (synaptics_aux_mid_button_scroll &&
		    dy && (psc->buttons & 0x2)) {
			wsmouse_precision_scroll(psc->sc_wsmousedev, dx, dy);
		} else {
			buttons = (psc->buttons & 0x1f) | ((psc->buttons >> 5) & 0x7);
			wsmouse_input(psc->sc_wsmousedev,
				buttons, dx, dy, dz, 0,
				WSMOUSE_INPUT_DELTA);
		}
		splx(s);
	}
}

static void
pms_synaptics_input(void *vsc, int data)
{
	struct pms_softc *psc = vsc;
	struct timeval diff;

	if (!psc->sc_enabled) {
		/* Interrupts are not expected. Discard the byte. */
		return;
	}

	getmicrouptime(&psc->current);

	if (psc->inputstate > 0) {
		timersub(&psc->current, &psc->last, &diff);
		if (diff.tv_sec > 0 || diff.tv_usec >= 40000) {
			aprint_debug_dev(psc->sc_dev,
			    "pms_synaptics_input: unusual delay (%ld.%06ld s), "
			    "scheduling reset\n",
			    (long)diff.tv_sec, (long)diff.tv_usec);
			printf("pms_synaptics_input: unusual delay (%ld.%06ld s), "
			    "scheduling reset\n",
			    (long)diff.tv_sec, (long)diff.tv_usec);
			psc->inputstate = 0;
			psc->sc_enabled = 0;
			wakeup(&psc->sc_enabled);
			return;
		}
	}
	psc->last = psc->current;

	switch (psc->inputstate) {
	case -5:
	case -4:
	case -3:
	case -2:
	case -1:
	case 0:
		if ((data & 0xc8) != 0x80) {
			aprint_debug_dev(psc->sc_dev,
			    "pms_synaptics_input: 0x%02x out of sync\n", data);
			/* use negative counts to limit resync phase */
			psc->inputstate--;
			return;	/* not in sync yet, discard input */
		}
		psc->inputstate = 0;
		/*FALLTHROUGH*/

	case -6:
	case 3:
		if ((data & 8) == 8) {
			aprint_debug_dev(psc->sc_dev,
			    "pms_synaptics_input: dropped in relative mode, reset\n");
			psc->inputstate = 0;
			psc->sc_enabled = 0;
			wakeup(&psc->sc_enabled);
			return;
		}
	}

	psc->packet[psc->inputstate++] = data & 0xff;
	if (psc->inputstate == 6) {
		/*
		 * We have a complete packet.
		 * Extract the pertinent details.
		 */
		psc->inputstate = 0;
		if ((psc->packet[0] & 0xfc) == 0x84 &&
		    (psc->packet[3] & 0xcc) == 0xc4) {
			/* W = SYNAPTICS_WIDTH_PASSTHROUGH, PS/2 passthrough */
			pms_synaptics_passthrough(psc);
		} else {
			pms_synaptics_parse(psc);
		}
	}
}

static inline int
synaptics_finger_detect(struct synaptics_softc *sc, struct synaptics_packet *sp,
    int *palmp)
{
	int fingers;

	/* Assume no palm */
	*palmp = 0;

	/*
	 * Apply some hysteresis when checking for a finger.
	 * When the finger is first applied, we ignore it until the
	 * pressure exceeds the 'high' threshold. The finger is considered
	 * removed only when pressure falls beneath the 'low' threshold.
	 */
	if ((sc->prev_fingers == 0 && sp->sp_z > synaptics_finger_high) ||
	    (sc->prev_fingers != 0 && sp->sp_z > synaptics_finger_low))
		fingers = 1;
	else
		fingers = 0;

	/*
	 * If the pad can't do palm detection, skip the rest.
	 */
	if (fingers == 0 || (sc->flags & SYN_FLAG_HAS_PALM_DETECT) == 0)
		return (fingers);

	/*
	 * Palm detection
	 */
	if (sp->sp_z > SYNAPTICS_FINGER_FLAT &&
	    sp->sp_w >= SYNAPTICS_WIDTH_PALM_MIN)
		*palmp = 1;

	if (sc->prev_fingers == 0 &&
	    (sp->sp_z > SYNAPTICS_FINGER_FLAT ||
	     sp->sp_w >= SYNAPTICS_WIDTH_PALM_MIN)) {
		/*
		 * Contact area or pressure is too great to be a finger.
		 * Just ignore it for now.
		 */
		return (0);
	}

	/*
	 * Detect 2 and 3 fingers if supported, but only if multiple
	 * fingers appear within the tap gesture time period.
	 */
	if ((sc->flags & SYN_FLAG_HAS_MULTI_FINGER) &&
	    ((SYN_TIME(sc, sc->gesture_start_packet)
	     < synaptics_gesture_length) ||
	    SYN_TIME(sc, sc->gesture_start_packet)
	     < synaptics_gesture_length)) {
		switch (sp->sp_w) {
		case SYNAPTICS_WIDTH_TWO_FINGERS:
			fingers = 2;
			break;

		case SYNAPTICS_WIDTH_THREE_OR_MORE:
			fingers = 3;
			break;

		default:
			/*
			 * The width value can report spurious single-finger
			 * events after a multi-finger event.
			 */
			fingers = sc->prev_fingers <= 1 ? 1 : sc->prev_fingers;
			break;
		}
	}

	return (fingers);
}

static inline void
synaptics_gesture_detect(struct synaptics_softc *sc,
    struct synaptics_packet *sp, int fingers)
{
	int gesture_len, gesture_buttons;
	int set_buttons;

	gesture_len = SYN_TIME(sc, sc->gesture_start_packet);
	gesture_buttons = sc->gesture_buttons;

	if (fingers > 0 && (fingers == sc->prev_fingers)) {
		/* Finger is still present */
		sc->gesture_move_x = abs(sc->gesture_start_x - sp->sp_x);
		sc->gesture_move_y = abs(sc->gesture_start_y - sp->sp_y);
	} else
	if (fingers && sc->prev_fingers == 0) {
		/*
		 * Finger was just applied.
		 * If the previous gesture was a single-click, set things
		 * up to deal with a possible drag or double-click gesture.
		 * Basically, if the finger is removed again within
		 * 'synaptics_gesture_length' packets, this is treated
		 * as a double-click. Otherwise we will emulate holding
		 * the left button down whilst dragging the mouse.
		 */
		if (SYN_IS_SINGLE_TAP(sc->gesture_type))
			sc->gesture_type |= SYN_GESTURE_DRAG;

		sc->gesture_start_x = abs(sp->sp_x);
		sc->gesture_start_y = abs(sp->sp_y);
		sc->gesture_move_x = 0;
		sc->gesture_move_y = 0;
		sc->gesture_start_packet = sc->total_packets;

		DPRINTF(10, sc, "Finger applied:"
		    " gesture_start_x: %d"
		    " gesture_start_y: %d\n",
		    sc->gesture_start_x, sc->gesture_start_y);
	} else
	if (fingers == 0 && sc->prev_fingers != 0) {
		/*
		 * Finger was just removed.
		 * Check if the contact time and finger movement were
		 * small enough to qualify as a gesture.
		 * Ignore finger movement if multiple fingers were
		 * detected (the pad may report coordinates for any
		 * of the fingers).
		 */

		DPRINTF(10, sc, "Finger removed: gesture_len: %d (%d)\n",
		    gesture_len, synaptics_gesture_length);
		DPRINTF(10, sc, "gesture_move_x: %d (%d) sp_x: %d\n",
		    sc->gesture_move_x, synaptics_gesture_move, abs(sp->sp_x));
		DPRINTF(10, sc, "gesture_move_y: %d (%d) sp_y: %d\n",
		    sc->gesture_move_y, synaptics_gesture_move, abs(sp->sp_y));

		if (gesture_len < synaptics_gesture_length &&
		    ((sc->gesture_move_x < synaptics_gesture_move &&
		     sc->gesture_move_y < synaptics_gesture_move))) {
			/*
			 * Looking good so far.
			 */
			if (SYN_IS_DRAG(sc->gesture_type)) {
				/*
				 * Promote this gesture to double-click.
				 */
				sc->gesture_type |= SYN_GESTURE_DOUBLE;
				sc->gesture_type &= ~SYN_GESTURE_SINGLE;
			} else {
				/*
				 * Single tap gesture. Set the tap length timer
				 * and flag a single-click.
				 */
				sc->gesture_tap_packet = sc->total_packets;
				sc->gesture_type |= SYN_GESTURE_SINGLE;

				/*
				 * The gesture can be modified depending on
				 * the number of fingers detected.
				 *
				 * 1: Normal left button emulation.
				 * 2: Either middle button or right button
				 *    depending on the value of the two_fingers
				 *    sysctl variable.
				 * 3: Right button.
				 */
				switch (sc->prev_fingers) {
				case 2:
					if (synaptics_two_fingers_emul == 1)
						gesture_buttons |= PMS_RBUTMASK;
					else
					if (synaptics_two_fingers_emul == 2)
						gesture_buttons |= PMS_MBUTMASK;
					break;
				case 3:
					gesture_buttons |= PMS_RBUTMASK;
					break;
				default:
					gesture_buttons |= PMS_LBUTMASK;
					break;
				}
			}
		}

		/*
		 * Always clear drag state when the finger is removed.
		 */
		sc->gesture_type &= ~SYN_GESTURE_DRAG;
	}

	if (sc->gesture_type == 0) {
		/*
		 * There is no gesture in progress.
		 * Clear emulated button state.
		 */
		sc->gesture_buttons = 0;
		return;
	}

	/*
	 * A gesture is in progress.
	 */
	set_buttons = 0;

	if (SYN_IS_SINGLE_TAP(sc->gesture_type)) {
		/*
		 * Single-click.
		 * Activate the relevant button(s) until the
		 * gesture tap timer has expired.
		 */
		if (SYN_TIME(sc, sc->gesture_tap_packet) <
		    synaptics_gesture_length)
			set_buttons = 1;
		else
			sc->gesture_type &= ~SYN_GESTURE_SINGLE;
		DPRINTF(10, sc, "synaptics_gesture: single tap, buttons %d\n",
		    set_buttons);
		DPRINTF(10, sc, "synaptics_gesture: single tap, tap at %d, current %d\n",
		    sc->gesture_tap_packet, sc->total_packets);
		DPRINTF(10, sc, "synaptics_gesture: single tap, tap_time %d, gesture len %d\n",
		    SYN_TIME(sc, sc->gesture_tap_packet), synaptics_gesture_length);
	} else
	if (SYN_IS_DOUBLE_TAP(sc->gesture_type) && sc->prev_fingers == 0) {
		/*
		 * Double-click.
		 * Activate the relevant button(s) once.
		 */
		set_buttons = 1;
		sc->gesture_type &= ~SYN_GESTURE_DOUBLE;
	}

	if (set_buttons || SYN_IS_DRAG(sc->gesture_type)) {
		/*
		 * Single-click and drag.
		 * Maintain button state until the finger is removed.
		 */
		sp->sp_left |= gesture_buttons & PMS_LBUTMASK;
		sp->sp_right |= gesture_buttons & PMS_RBUTMASK;
		sp->sp_middle |= gesture_buttons & PMS_MBUTMASK;
	}

	sc->gesture_buttons = gesture_buttons;
}

static inline int
synaptics_filter_policy(struct synaptics_softc *sc, int finger, int *history,
			int value, u_int count)
{
	int a, b, rv;

	/*
	 * Once we've accumulated at least SYN_HIST_SIZE values, combine
	 * each new value with the previous two and return the average.
	 *
	 * This is necessary when the touchpad is operating in 80 packets
	 * per second mode, as it performs little internal filtering on
	 * reported values.
	 *
	 * Using a rolling average helps to filter out jitter caused by
	 * tiny finger movements.
	 */
	if (count >= SYN_HIST_SIZE) {
		a = (history[(count + 0) % SYN_HIST_SIZE] +
		    history[(count + 1) % SYN_HIST_SIZE]) / 2;

		b = (value + history[(count + 0) % SYN_HIST_SIZE]) / 2;

		rv = b - a;

		/*
		 * Don't report the movement if it's below a certain
		 * threshold.
		 */
		if (abs(rv) < synaptics_movement_threshold)
			rv = 0;
	} else
		rv = 0;

	/*
	 * Add the new value to the history buffer.
	 */
	history[(count + 1) % SYN_HIST_SIZE] = value;

	return (rv);
}

/* Edge detection */
#define	SYN_EDGE_TOP		1
#define	SYN_EDGE_BOTTOM		2
#define	SYN_EDGE_LEFT		4
#define	SYN_EDGE_RIGHT		8

static inline int
synaptics_check_edge(int x, int y)
{
	int rv = 0;

	if (x < synaptics_edge_left)
		rv |= SYN_EDGE_LEFT;
	else
	if (x > synaptics_edge_right)
		rv |= SYN_EDGE_RIGHT;

	if (y < synaptics_edge_bottom)
		rv |= SYN_EDGE_BOTTOM;
	else
	if (y > synaptics_edge_top)
		rv |= SYN_EDGE_TOP;

	return (rv);
}

static inline int
synaptics_edge_motion(struct synaptics_softc *sc, int delta, int dir)
{

	/*
	 * When edge motion is enabled, synaptics_edge_motion_delta is
	 * combined with the current delta, together with the direction
	 * in which to simulate the motion. The result is added to
	 * the delta derived from finger movement. This provides a smooth
	 * transition from finger movement to edge motion.
	 */
	delta = synaptics_edge_motion_delta + (dir * delta);
	if (delta < 0)
		return (0);
	if (delta > synaptics_edge_motion_delta)
		return (synaptics_edge_motion_delta);
	return (delta);
}

static inline int
synaptics_scale(int delta, int scale, int *remp)
{
	int rv;

	/*
	 * Scale the raw delta in Synaptics coordinates (0-6143) into
	 * something more reasonable by dividing the raw delta by a
	 * scale factor. Any remainder from the previous scale result
	 * is added to the current delta before scaling.
	 * This prevents loss of resolution for very small/slow
	 * movements of the finger.
	 */
	delta += *remp;
	rv = delta / scale;
	*remp = delta % scale;

	return (rv);
}

static inline void
synaptics_movement(struct synaptics_softc *sc, struct synaptics_packet *sp,
    int *dxp, int *dyp, int *dzp, int *sdxp, int *sdyp, int *sdzp)
{
	int dx, dy, dz, sdx, sdy, sdz, edge;

	dx = dy = dz = sdx = sdy = sdz = 0;

	/*
	 * Compute the next values of dx, dy, dz, sdx, sdy, sdz.
	 */
	dx = synaptics_filter_policy(sc, 0,
	    sc->history_x[SYN_PRIMARY_FINGER], sp->sp_x,
	    sc->packet_count[SYN_PRIMARY_FINGER]);
	dy = synaptics_filter_policy(sc, 0,
	    sc->history_y[SYN_PRIMARY_FINGER], sp->sp_y,
	    sc->packet_count[SYN_PRIMARY_FINGER]);

	if (sp->sp_finger_count > 1) {
		sdx = synaptics_filter_policy(sc, 1,
		    sc->history_x[SYN_SECONDARY_FINGER], sp->sp_sx,
		    sc->packet_count[SYN_SECONDARY_FINGER]);
		sdy = synaptics_filter_policy(sc, 1,
		    sc->history_y[SYN_SECONDARY_FINGER], sp->sp_sy,
		    sc->packet_count[SYN_SECONDARY_FINGER]);
		DPRINTF(10, sc, "synaptics_movement: dx %d dy %d sdx %d sdy %d\n",
		    dx, dy, sdx, sdy);
	}

	/*
	 * If we're dealing with a drag gesture, and the finger moves to
	 * the edge of the touchpad, apply edge motion emulation if it
	 * is enabled.
	 */
	if (synaptics_edge_motion_delta && SYN_IS_DRAG(sc->gesture_type)) {
		edge = synaptics_check_edge(sp->sp_x, sp->sp_y);

		if (edge & SYN_EDGE_LEFT)
			dx -= synaptics_edge_motion(sc, dx, 1);
		if (edge & SYN_EDGE_RIGHT)
			dx += synaptics_edge_motion(sc, dx, -1);
		if (edge & SYN_EDGE_BOTTOM)
			dy -= synaptics_edge_motion(sc, dy, 1);
		if (edge & SYN_EDGE_TOP)
			dy += synaptics_edge_motion(sc, dy, -1);

		if (sp->sp_finger_count > 1) {
			edge = synaptics_check_edge(sp->sp_sx, sp->sp_sy);

			if (edge & SYN_EDGE_LEFT)
				sdx -= synaptics_edge_motion(sc, sdx, 1);
			if (edge & SYN_EDGE_RIGHT)
				sdx += synaptics_edge_motion(sc, sdx, -1);
			if (edge & SYN_EDGE_BOTTOM)
				sdy -= synaptics_edge_motion(sc, sdy, 1);
			if (edge & SYN_EDGE_TOP)
				sdy += synaptics_edge_motion(sc, sdy, -1);
		}
	}

	/*
	 * Apply scaling to the deltas
	 */
	dx = synaptics_scale(dx, synaptics_scale_x,
	    &sc->rem_x[SYN_PRIMARY_FINGER]);
	dy = synaptics_scale(dy, synaptics_scale_y,
	    &sc->rem_y[SYN_PRIMARY_FINGER]);
	dz = synaptics_scale(dz, synaptics_scale_z,
	    &sc->rem_z[SYN_PRIMARY_FINGER]);

	if (sp->sp_finger_count > 1) {
		sdx = synaptics_scale(sdx, synaptics_scale_x,
		    &sc->rem_x[SYN_SECONDARY_FINGER]);
		sdy = synaptics_scale(sdy, synaptics_scale_y,
		    &sc->rem_y[SYN_SECONDARY_FINGER]);
		sdz = synaptics_scale(sdz, synaptics_scale_z,
		    &sc->rem_z[SYN_SECONDARY_FINGER]);

		DPRINTF(10, sc,
		    "synaptics_movement 2: dx %d dy %d sdx %d sdy %d\n",
		    dx, dy, sdx, sdy);
	}

	/*
	 * Clamp deltas to specified maximums.
	 */
	if (abs(dx) > synaptics_max_speed_x)
		dx = ((dx >= 0)? 1 : -1) * synaptics_max_speed_x;
	if (abs(dy) > synaptics_max_speed_y)
		dy = ((dy >= 0)? 1 : -1) * synaptics_max_speed_y;
	if (abs(dz) > synaptics_max_speed_z)
		dz = ((dz >= 0)? 1 : -1) * synaptics_max_speed_z;

	if (sp->sp_finger_count > 1) {
		if (abs(sdx) > synaptics_max_speed_x)
			sdx = ((sdx >= 0)? 1 : -1) * synaptics_max_speed_x;
		if (abs(dy) > synaptics_max_speed_y)
			sdy = ((sdy >= 0)? 1 : -1) * synaptics_max_speed_y;
		if (abs(sdz) > synaptics_max_speed_z)
			sdz = ((sdz >= 0)? 1 : -1) * synaptics_max_speed_z;
	}

	*dxp = dx;
	*dyp = dy;
	*dzp = dz;
	*sdxp = sdx;
	*sdyp = sdy;
	*sdzp = sdz;

}

static void
pms_synaptics_process_packet(struct pms_softc *psc, struct synaptics_packet *sp)
{
	struct synaptics_softc *sc = &psc->u.synaptics;
	int dx, dy, dz, sdx, sdy, sdz;
	int fingers, palm, buttons, changed;
	int s;

	sdx = sdy = sdz = 0;

	/*
	 * Do Z-axis emulation using up/down buttons if required.
	 * Note that the pad will send a one second burst of packets
	 * when an up/down button is pressed and held. At the moment
	 * we don't deal with auto-repeat, so convert the burst into
	 * a one-shot.
	 */
	dz = 0;
	if (synaptics_up_down_emul == 2) {
		if (sc->up_down == 0) {
			if (sp->sp_up && sp->sp_down) {
				sp->sp_middle = 1;
			} else if (sp->sp_up) {
				dz = -synaptics_up_down_motion_delta;
			} else if (sp->sp_down) {
				dz = synaptics_up_down_motion_delta;
			}
		}

		sc->up_down = sp->sp_up | sp->sp_down;
		sp->sp_up = sp->sp_down = 0;
	}

	/*
	 * Determine whether or not a finger is on the pad.
	 * On some pads, this will return the number of fingers
	 * detected.
	 */
	fingers = synaptics_finger_detect(sc, sp, &palm);

	/*
	 * Do gesture processing only if we didn't detect a palm.
	 */
	if (palm == 0)
		synaptics_gesture_detect(sc, sp, fingers);
	else
		sc->gesture_type = sc->gesture_buttons = 0;

	/*
	 * Determine what buttons to report
	 */
	buttons = (sp->sp_left ? 0x1 : 0) |
	    (sp->sp_middle ? 0x2 : 0) |
	    (sp->sp_right ? 0x4 : 0) |
	    (sp->sp_up ? 0x8 : 0) |
	    (sp->sp_down ? 0x10 : 0);
	changed = buttons ^ (psc->buttons & 0x1f);
	psc->buttons ^= changed;

	sc->prev_fingers = fingers;

	/*
	 * Do movement processing IFF we have a single finger and no palm or
	 * a secondary finger and no palm.
	 */
	if (palm == 0 && synaptics_movement_enable) {
		if (fingers > 0) {
			synaptics_movement(sc, sp, &dx, &dy, &dz, &sdx, &sdy,
			    &sdz);

			/*
			 * Check if there are two fingers, if there are then
			 * check if we have a clickpad, if we do then we
			 * don't scroll iff one of the fingers is in the
			 * button region.  Otherwise interpret as a scroll
			 */
			if (sp->sp_finger_count >= 2 && sc->gesture_type == 0 ) {
				if (!(sc->flags & SYN_FLAG_HAS_ONE_BUTTON_CLICKPAD) ||
				    ((sc->flags & SYN_FLAG_HAS_ONE_BUTTON_CLICKPAD) &&
				    (sp->sp_y > synaptics_button_boundary)  &&
				    (sp->sp_sy > synaptics_button_boundary))) {
					s = spltty();
					wsmouse_precision_scroll(
					    psc->sc_wsmousedev, dx, dy);
					splx(s);
					return;
				}
			}

			/*
			 * Allow user to turn off movements in the button
			 * region of a click pad.
			 */
			if (sc->flags & SYN_FLAG_HAS_ONE_BUTTON_CLICKPAD) {
				if ((sp->sp_y < synaptics_button_boundary) &&
				    (!synaptics_button_region_movement)) {
					dx = dy = dz = 0;
				}

				if ((sp->sp_sy < synaptics_button_boundary) &&
				    (!synaptics_button_region_movement)) {
					sdx = sdy = sdz = 0;
				}
			}

			/* Now work out which finger to report, just
			 * go with the one that has moved the most.
			 */
			if ((abs(dx) + abs(dy) + abs(dz)) <
			    (abs(sdx) + abs(sdy) + abs(sdz))) {
				/* secondary finger wins */
				dx = sdx;
				dy = sdy;
				dz = sdz;
			}
		} else {
			/*
			 * No valid finger. Therefore no movement.
			 */
			sc->rem_x[SYN_PRIMARY_FINGER] = 0;
			sc->rem_y[SYN_PRIMARY_FINGER] = 0;
			sc->rem_x[SYN_SECONDARY_FINGER] = 0;
			sc->rem_y[SYN_SECONDARY_FINGER] = 0;
			dx = dy = 0;
		}
	} else {
		/*
		 * No valid finger. Therefore no movement.
		 */
		sc->rem_x[SYN_PRIMARY_FINGER] = 0;
		sc->rem_y[SYN_PRIMARY_FINGER] = 0;
		sc->rem_z[SYN_PRIMARY_FINGER] = 0;
		dx = dy = dz = 0;
	}

	DPRINTF(10, sc,
	    "pms_synaptics_process_packet: dx %d dy %d dz %d sdx %d sdy %d sdz %d\n",
	    dx, dy, dz, sdx, sdy, sdz);

	/*
	 * Pass the final results up to wsmouse_input() if necessary.
	 */
	if (dx || dy || dz || changed) {
		buttons = (psc->buttons & 0x1f) | ((psc->buttons >> 5) & 0x7);
		s = spltty();
		wsmouse_input(psc->sc_wsmousedev,
				buttons,
				dx, dy, dz, 0,
		    		WSMOUSE_INPUT_DELTA);
		splx(s);
	}
}
