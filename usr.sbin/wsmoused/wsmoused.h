/* $NetBSD: wsmoused.h,v 1.5 2003/08/06 23:58:40 jmmv Exp $ */

/*
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
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

#define IS_MOTION_EVENT(type) (((type) == WSCONS_EVENT_MOUSE_DELTA_X) || \
                               ((type) == WSCONS_EVENT_MOUSE_DELTA_Y) || \
                               ((type) == WSCONS_EVENT_MOUSE_DELTA_Z))
#define IS_BUTTON_EVENT(type) (((type) == WSCONS_EVENT_MOUSE_UP) || \
                               ((type) == WSCONS_EVENT_MOUSE_DOWN))

struct mouse {
	int   m_devfd;          /* File descriptor of wsmouse device */
	int   m_fifofd;         /* File descriptor of fifo */
	int   m_statfd;         /* File descriptor of wscons status device */
	char *m_devname;        /* File name of wsmouse device */
	char *m_fifoname;       /* File name of fifo */
	int   m_disabled;       /* Whether if the mouse is disabled or not */
};

struct mode_bootstrap {
	char  *mb_name;
	int  (*mb_startup)(struct mouse *);
	int  (*mb_cleanup)(void);
	void (*mb_wsmouse_event)(struct wscons_event);
	void (*mb_wscons_event)(struct wscons_event);
	void (*mb_poll_timeout)(void);
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
void log_err(int, const char *, ...);
void log_errx(int, const char *, ...);
void log_info(const char *, ...);
void log_warn(const char *, ...);
void log_warnx(const char *, ...);

/* Prototypes for config.c */
struct prop *prop_new(void);
void prop_free(struct prop *);
struct block *block_new(int);
void block_free(struct block *);
void block_add_prop(struct block *, struct prop *);
void block_add_child(struct block *, struct block *);
char *block_get_propval(struct block *, const char *, char *);
int block_get_propval_int(struct block *, const char *, int);
struct block *config_get_mode(const char *);
void config_read(const char *, int);
void config_free(void);

#endif /* _WSMOUSED_WSMOUSED_H */
