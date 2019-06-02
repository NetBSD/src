/*	$NetBSD: synapticsvar.h,v 1.9 2019/06/02 08:55:00 blymn Exp $	*/

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
 *   * Neither the name of the Kentaro A. Kurahone nor the names of its
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

#ifndef _DEV_PCKBCPORT_SYNAPTICSVAR_H_
#define _DEV_PCKBCPORT_SYNAPTICSVAR_H_

struct synaptics_softc {
	int	caps;

	int	flags;
#define	SYN_FLAG_HAS_MIDDLE_BUTTON		(1 << 0)
#define	SYN_FLAG_HAS_BUTTONS_4_5		(1 << 1)
#define	SYN_FLAG_HAS_UP_DOWN_BUTTONS		(1 << 2)
#define	SYN_FLAG_HAS_PASSTHROUGH		(1 << 3)
#define	SYN_FLAG_HAS_PALM_DETECT		(1 << 4)
#define	SYN_FLAG_HAS_MULTI_FINGER		(1 << 5)
#define	SYN_FLAG_HAS_MULTI_FINGER_REPORT	(1 << 6)
#define	SYN_FLAG_HAS_VERTICAL_SCROLL		(1 << 7)
#define	SYN_FLAG_HAS_HORIZONTAL_SCROLL		(1 << 8)
#define	SYN_FLAG_HAS_ONE_BUTTON_CLICKPAD	(1 << 9)
#define	SYN_FLAG_HAS_TWO_BUTTON_CLICKPAD	(1 << 10)
#define	SYN_FLAG_HAS_EXTENDED_WMODE		(1 << 11)
#define	SYN_FLAG_HAS_ADV_GESTURE_MODE		(1 << 12)

	u_int	total_packets[2];	/* Total number of packets received */
#define	SYN_TIME(sc,c,n)	(((sc)->total_packets[(n)] >= (c)) ?	\
				((sc)->total_packets[(n)] - (c)) :	\
				((c) - (sc)->total_packets[(n)]))

	int	up_down;
	int	prev_fingers;

	int	gesture_start_x, gesture_start_y;
	int	gesture_move_x, gesture_move_y;
	u_int	gesture_start_packet;
	u_int	gesture_tap_packet;

	int	gesture_buttons;
	int	gesture_type;
#define	SYN_GESTURE_SINGLE	0x01
#define	SYN_GESTURE_DOUBLE	0x02
#define	SYN_GESTURE_DRAG	0x04
#define	SYN_IS_SINGLE_TAP(t)	((t) & SYN_GESTURE_SINGLE)
#define	SYN_IS_DOUBLE_TAP(t)	((t) & SYN_GESTURE_DOUBLE)
#define	SYN_IS_DRAG(t)		((t) & SYN_GESTURE_DRAG)

#define	SYN_HIST_SIZE	4
#define SYN_MAX_FINGERS 2
	char	button_history;
	int	dz_hold;
	int	rem_x[SYN_MAX_FINGERS];
	int	rem_y[SYN_MAX_FINGERS];
	int	rem_z[SYN_MAX_FINGERS];
	u_int	movement_history[SYN_MAX_FINGERS];
	int	history_x[SYN_MAX_FINGERS][SYN_HIST_SIZE];
	int	history_y[SYN_MAX_FINGERS][SYN_HIST_SIZE];
	int	history_z[SYN_MAX_FINGERS][SYN_HIST_SIZE];
};

int pms_synaptics_probe_init(void *vsc);
void pms_synaptics_enable(void *vsc);
void pms_synaptics_resume(void *vsc);

#endif
