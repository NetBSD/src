/* $NetBSD: wsmoused.h,v 1.1 2002/06/26 23:13:11 christos Exp $ */

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

#ifndef _WSMOUSED_WSMOUSED_H
#define _WSMOUSED_WSMOUSED_H

struct mouse {
	/* File descriptors and names */
	int  fd;
	int  tty_fd;
	int  stat_fd;
	int  fifo_fd;
	char *device_name;
	char *fifo_name;

	/* Screen coordinates */
	size_t row, col;
	size_t max_row, max_col;

	/* Movement information */
	size_t slowdown_x, slowdown_y;
	size_t count_row, count_col;

	int cursor;
	int selecting;
	int disabled;
};

/* Prototypes for wsmoused.c */
void char_invert(struct mouse *, size_t, size_t);
void mouse_cursor_show(struct mouse *);
void mouse_cursor_hide(struct mouse *);
void mouse_open_tty(struct mouse *, int);

/* Prototypes for event.c */
void mouse_motion_event(struct mouse *, struct wscons_event *);
void mouse_button_event(struct mouse *, struct wscons_event *);
void screen_event(struct mouse *, struct wscons_event *);

/* Prototypes for selection.c */
void mouse_sel_init(void);
void mouse_sel_start(struct mouse *);
void mouse_sel_end(struct mouse *);
void mouse_sel_calculate(struct mouse *);
void mouse_sel_hide(struct mouse *);
void mouse_sel_show(struct mouse *);
void mouse_sel_paste(struct mouse *);

#endif /* _WSMOUSED_WSMOUSED_H */
