/* $NetBSD: wsmoused.h,v 1.3 2003/03/04 14:33:55 jmmv Exp $ */

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
	char *tstat_name;

	/* Screen coordinates */
	size_t row, col;
	size_t max_row, max_col;

	/* Movement information */
	size_t slowdown_x, slowdown_y;
	size_t count_row, count_col;

	int cursor;
	int selecting;
	int disabled;

	/* Button configuration */
	int but_select;
	int but_paste;
};

struct prop {
	char *p_name;
	char *p_value;
};

#define MAX_EVENTS	10
#define MAX_BLOCKS	10
#define MAX_PROPS	100
#define BLOCK_GLOBAL	1
#define BLOCK_MODE	2
#define BLOCK_EVENT	3
struct block {
	char *b_name;
	int b_type;
	int b_prop_count;
	int b_child_count;
	struct prop *b_prop[MAX_BLOCKS];
	struct block *b_child[MAX_BLOCKS];
	struct block *b_parent;
};

/* Prototypes for wsmoused.c */
void char_invert(struct mouse *, size_t, size_t);
void mouse_cursor_show(struct mouse *);
void mouse_cursor_hide(struct mouse *);
void mouse_open_tty(struct mouse *, int);

/* Prototypes for config.c */
struct prop *prop_new(void);
void prop_free(struct prop *);
struct block *block_new(int);
void block_free(struct block *);
void block_add_prop(struct block *, struct prop *);
void block_add_child(struct block *, struct block *);
char *block_get_propval(struct block *, char *, char *);
int block_get_propval_int(struct block *, char *, int);
struct block *config_get_mode(char *);
void config_read(char *, int);
void config_free(void);

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
