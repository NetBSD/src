/* $NetBSD: events.c,v 1.2 2002/07/02 12:41:26 christos Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio Merino.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name authors may not be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: events.c,v 1.2 2002/07/02 12:41:26 christos Exp $");
#endif /* not lint */

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/tty.h>
#include <dev/wscons/wsconsio.h>
 
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>

#include "pathnames.h"
#include "wsmoused.h"

/*
 * Process mouse motion events. All mouse movement is controlled here,
 * except the drawing.
 */
void
mouse_motion_event(struct mouse *m, struct wscons_event *evt)
{
	if (m->selecting) mouse_sel_hide(m);
	mouse_cursor_hide(m);

	switch (evt->type) {
	case WSCONS_EVENT_MOUSE_DELTA_X:
		if (m->count_col >= m->slowdown_x) {
			m->count_col = 0;
			if (evt->value > 0) m->col++;
			else if (m->col != 0) m->col--;
			if (m->col > m->max_col) m->col = m->max_col;
		} else
			m->count_col++;
		break;

	case WSCONS_EVENT_MOUSE_DELTA_Y:
		if (m->count_row >= m->slowdown_y) {
			m->count_row = 0;
			if (evt->value < 0) m->row++;
			else if (m->row != 0) m->row--;
			if (m->row > m->max_row) m->row = m->max_row;
		} else
			m->count_row++;
		break;

	case WSCONS_EVENT_MOUSE_DELTA_Z:
		break;

	default:
		warnx("unknown event");
	}

	if (m->selecting) mouse_sel_show(m);
	mouse_cursor_show(m);
}

/*
 * Process mouse button events
 */
void
mouse_button_event(struct mouse *m, struct wscons_event *evt)
{
	switch (evt->type) {
	case WSCONS_EVENT_MOUSE_UP:
		if (evt->value == 0) {
			mouse_sel_end(m);
			mouse_sel_hide(m);
		}
		break;

	case WSCONS_EVENT_MOUSE_DOWN:
		switch (evt->value) {
		case 0: /* Button 1 */
			mouse_sel_start(m);
			mouse_cursor_hide(m);
			mouse_sel_show(m);
			break;
		case 2: /* Button 2 */
			mouse_sel_paste(m);
			break;
		}
		break;

	default:
		warnx("unknown event");
	}
}

/*
 * Process screen events that may affect mouse selections and other
 * behavior.
 */
void
screen_event(struct mouse *m, struct wscons_event *evt)
{
	switch (evt->type) {
	case WSCONS_EVENT_SCREEN_SWITCH:
		if (m->selecting) mouse_sel_hide(m);
		mouse_cursor_hide(m);

		mouse_open_tty(m, evt->value);

		mouse_cursor_show(m);
		if (m->selecting) mouse_sel_show(m);

		break;
	}
	return;
}
