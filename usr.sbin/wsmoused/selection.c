/* $NetBSD: selection.c,v 1.5 2003/08/06 23:58:40 jmmv Exp $ */

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

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: selection.c,v 1.5 2003/08/06 23:58:40 jmmv Exp $");
#endif /* not lint */

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/tty.h>
#include <dev/wscons/wsconsio.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathnames.h"
#include "wsmoused.h"

/* ---------------------------------------------------------------------- */

/*
 * Public interface exported by the `selection' mode.
 */

int  selection_startup(struct mouse *m);
int  selection_cleanup(void);
void selection_wsmouse_event(struct wscons_event);
void selection_wscons_event(struct wscons_event);
void selection_poll_timeout(void);

struct mode_bootstrap Selection_Mode = {
	"selection",
	selection_startup,
	selection_cleanup,
	selection_wsmouse_event,
	selection_wscons_event,
	selection_poll_timeout
};

/* ---------------------------------------------------------------------- */

/*
 * Structures used in this module only.
 */

/* The `selarea' structure is used to describe a selection in the screen.
   It also holds a copy of the selected text. */
struct selarea {
	size_t sa_x1;           /* Start column */
	size_t sa_y1;           /* Start row */
	size_t sa_x2;           /* End column */
	size_t sa_y2;           /* End row */
	size_t sa_startoff;     /* Absolute offset of start position */
	size_t sa_endoff;       /* Absolute offset of end position */
	size_t sa_buflen;       /* Length of selected text */
	char  *sa_buf;          /* A copy of the selected text */
};

/* The `selmouse' structure extends the `mouse' structure adding all fields
   required for this module to work. */
struct selmouse {
	struct mouse *sm_mouse; /* Pointer to parent structure */

	int sm_ttyfd;           /* Active TTY file descriptor */

	size_t sm_x;            /* Mouse pointer column */
	size_t sm_y;            /* Mouse pointer row */
	size_t sm_max_x;        /* Maximun column allowed */
	size_t sm_max_y;        /* Maximun row allowed */

	size_t sm_slowdown_x;   /* X axis slowdown */
	size_t sm_slowdown_y;   /* Y axis slowdown */
	size_t sm_count_x;      /* Number of X movements skipped */
	size_t sm_count_y;      /* Number of Y movements skipped */

	int sm_visible;         /* Whether pointer is visible or not */
	int sm_selecting;       /* Whether we are selecting or not */

	int sm_but_select;      /* Button number to select an area */
	int sm_but_paste;       /* Button number to paste buffer */
};

/* ---------------------------------------------------------------------- */

/*
 * Global variables.
 */

static struct selmouse Selmouse;
static struct selarea Selarea;
static int Initialized = 0;

/* ---------------------------------------------------------------------- */

/*
 * Prototypes for functions private to this module.
 */

static void cursor_hide(void);
static void cursor_show(void);
static void open_tty(int);
static void char_invert(size_t, size_t);
static void *alloc_sel(size_t);
static char *fill_buf(char *, size_t, size_t, size_t);
static size_t row_length(size_t);
static void selarea_copy_text(void);
static void selarea_start(void);
static void selarea_end(void);
static void selarea_calculate(void);
static void selarea_hide(void);
static void selarea_show(void);
static void selarea_paste(void);

/* ---------------------------------------------------------------------- */

/* Mode initialization.  Reads configuration, checks if the kernel has
 * support for mouse pointer and opens required files. */
int
selection_startup(struct mouse *m)
{
	int i;
	struct winsize ws;
	struct wsdisplay_char ch;
	struct block *conf;

	if (Initialized) {
		log_warnx("selection mode already initialized");
		return 1;
	}

	(void)memset(&Selmouse, 0, sizeof(struct selmouse));
	Selmouse.sm_mouse = m;

	conf = config_get_mode("selection");
	Selmouse.sm_slowdown_x = block_get_propval_int(conf, "slowdown_x", 0);
	Selmouse.sm_slowdown_y = block_get_propval_int(conf, "slowdown_y", 3);
	if (block_get_propval_int(conf, "lefthanded", 0)) {
		Selmouse.sm_but_select = 2;
		Selmouse.sm_but_paste = 0;
	} else {
		Selmouse.sm_but_select = 0;
		Selmouse.sm_but_paste = 2;
	}

	/* Get terminal size */
	if (ioctl(0, TIOCGWINSZ, &ws) < 0) {
		log_warn("cannot get terminal size");
		return 0;
	}

	/* Open current tty */
	(void)ioctl(Selmouse.sm_mouse->m_statfd, WSDISPLAYIO_GETACTIVESCREEN,
	    &i);
	Selmouse.sm_ttyfd = -1;
	open_tty(i);

	/* Check if the kernel has character functions */
	ch.row = ch.col = 0;
	if (ioctl(Selmouse.sm_ttyfd, WSDISPLAYIO_GETWSCHAR, &ch) < 0) {
		(void)close(Selmouse.sm_ttyfd);
		log_warn("ioctl(WSDISPLAYIO_GETWSCHAR) failed");
		return 0;
	}

	Selmouse.sm_max_y = ws.ws_row - 1;
	Selmouse.sm_max_x = ws.ws_col - 1;
	Selmouse.sm_y = Selmouse.sm_max_y / 2;
	Selmouse.sm_x = Selmouse.sm_max_x / 2;
	Selmouse.sm_count_y = 0;
	Selmouse.sm_count_x = 0;
	Selmouse.sm_visible = 0;
	Selmouse.sm_selecting = 0;
	Initialized = 1;

	return 1;
}

/* ---------------------------------------------------------------------- */

/* Mode cleanup. */
int
selection_cleanup(void)
{

	cursor_hide();
	if (Selmouse.sm_ttyfd >= 0)
		(void)close(Selmouse.sm_ttyfd);
	return 1;
}

/* ---------------------------------------------------------------------- */

/* Parse wsmouse events.  Both motion and button events are handled.  The
 * former move the mouse across the screen and the later create a new
 * selection or paste the buffer. */
void
selection_wsmouse_event(struct wscons_event evt)
{

	if (IS_MOTION_EVENT(evt.type)) {
		if (Selmouse.sm_selecting)
			selarea_hide();
		cursor_hide();

		switch (evt.type) {
		case WSCONS_EVENT_MOUSE_DELTA_X:
			if (Selmouse.sm_count_x >= Selmouse.sm_slowdown_x) {
				Selmouse.sm_count_x = 0;
				if (evt.value > 0)
					Selmouse.sm_x++;
				else if (Selmouse.sm_x != 0)
					Selmouse.sm_x--;
				if (Selmouse.sm_x > Selmouse.sm_max_x)
					Selmouse.sm_x = Selmouse.sm_max_x;
			} else
				Selmouse.sm_count_x++;
			break;

		case WSCONS_EVENT_MOUSE_DELTA_Y:
			if (Selmouse.sm_count_y >= Selmouse.sm_slowdown_y) {
				Selmouse.sm_count_y = 0;
				if (evt.value < 0)
					Selmouse.sm_y++;
				else if (Selmouse.sm_y != 0)
					Selmouse.sm_y--;
				if (Selmouse.sm_y > Selmouse.sm_max_y)
					Selmouse.sm_y = Selmouse.sm_max_y;
			} else
				Selmouse.sm_count_y++;
			break;

		case WSCONS_EVENT_MOUSE_DELTA_Z:
			break;

		default:
			log_warnx("unknown event");
		}

		if (Selmouse.sm_selecting)
			selarea_show();
		cursor_show();

	} else if (IS_BUTTON_EVENT(evt.type)) {
		switch (evt.type) {
		case WSCONS_EVENT_MOUSE_UP:
			if (evt.value == Selmouse.sm_but_select) {
				/* End selection */
				selarea_end();
				selarea_hide();
			}
			break;

		case WSCONS_EVENT_MOUSE_DOWN:
			if (evt.value == Selmouse.sm_but_select) {
				/* Start selection */
				selarea_start();
				cursor_hide();
				selarea_show();
			} else if (evt.value == Selmouse.sm_but_paste) {
				/* Paste selection */
				selarea_paste();
				break;
			}
			break;

		default:
			log_warnx("unknown button event");
		}
	}
}

/* ---------------------------------------------------------------------- */

/* Parse wscons status events. */
void
selection_wscons_event(struct wscons_event evt)
{

	switch (evt.type) {
	case WSCONS_EVENT_SCREEN_SWITCH:
		if (Selmouse.sm_selecting)
			selarea_hide();
		cursor_hide();

		open_tty(evt.value);

		cursor_show();
		if (Selmouse.sm_selecting)
			selarea_show();

		break;
	}
}

/* ---------------------------------------------------------------------- */

/* Device polling has timed out, so we hide the mouse to avoid further
 * console pollution. */
void
selection_poll_timeout(void)
{

	if (!Selmouse.sm_selecting)
		cursor_hide();
}

/* ---------------------------------------------------------------------- */

/* Hides the mouse pointer, if visible. */
static void
cursor_hide(void)
{

	if (Selmouse.sm_visible) {
		char_invert(Selmouse.sm_y, Selmouse.sm_x);
		Selmouse.sm_visible = 0;
	}
}

/* ---------------------------------------------------------------------- */

/* Shows the mouse pointer, if not visible. */
static void
cursor_show(void)
{

	if (!Selmouse.sm_visible) {
		char_invert(Selmouse.sm_y, Selmouse.sm_x);
		Selmouse.sm_visible = 1;
	}
}

/* ---------------------------------------------------------------------- */

/* Opens the specified TTY device, used when switching consoles. */
static void
open_tty(int ttyno)
{
	char buf[20];

	if (Selmouse.sm_ttyfd >= 0)
		(void)close(Selmouse.sm_ttyfd);

	if (!Selmouse.sm_mouse->m_disabled) {
		(void)snprintf(buf, sizeof(buf), _PATH_TTYPREFIX "%d", ttyno);
		Selmouse.sm_ttyfd = open(buf, O_RDONLY | O_NONBLOCK);
		if (Selmouse.sm_ttyfd < 0)
			log_warnx("cannot open %s", buf);
	}
}

/* ---------------------------------------------------------------------- */

/* Flips the background and foreground colors of the specified screen
 * position. */
static void
char_invert(size_t row, size_t col)
{
	int t;
	struct wsdisplay_char ch;

	ch.row = row;
	ch.col = col;

	if (ioctl(Selmouse.sm_ttyfd, WSDISPLAYIO_GETWSCHAR, &ch) == -1) {
		log_warn("ioctl(WSDISPLAYIO_GETWSCHAR) failed");
		return;
	}

	t = ch.foreground;
	ch.foreground = ch.background;
	ch.background = t;

	if (ioctl(Selmouse.sm_ttyfd, WSDISPLAYIO_PUTWSCHAR, &ch) == -1)
		log_warn("ioctl(WSDISPLAYIO_PUTWSCHAR) failed");
}

/* ---------------------------------------------------------------------- */

/* Allocates memory for a selection.  This function is very simple but is
 * used to get a consistent warning message. */
static void *
alloc_sel(size_t len)
{
	void *ptr;

	ptr = malloc(len);
	if (ptr == NULL)
		log_warn("cannot allocate memory for selection (%lu bytes)",
		    (unsigned long)len);
	return ptr;
}

/* ---------------------------------------------------------------------- */

/* Copies a region of a line inside the buffer pointed by `ptr'. */
static char *
fill_buf(char *ptr, size_t row, size_t col, size_t end)
{
	struct wsdisplay_char ch;

	ch.row = row;
	for (ch.col = col; ch.col < end; ch.col++) {
		if (ioctl(Selmouse.sm_ttyfd, WSDISPLAYIO_GETWSCHAR,
		    &ch) == -1) {
			log_warn("ioctl(WSDISPLAYIO_GETWSCHAR) failed");
			*ptr++ = ' ';
		} else {
			*ptr++ = ch.letter;
		}
	}
	return ptr;
}

/* ---------------------------------------------------------------------- */

/* Scans the specified line and checks its length.  Characters at the end
 * of it which match isspace() are discarded. */
static size_t
row_length(size_t row)
{
	struct wsdisplay_char ch;

	ch.col = Selmouse.sm_max_x;
	ch.row = row;
	do {
		if (ioctl(Selmouse.sm_ttyfd, WSDISPLAYIO_GETWSCHAR, &ch) == -1)
			log_warn("ioctl(WSDISPLAYIO_GETWSCHAR) failed");
		ch.col--;
	} while (isspace((unsigned char)ch.letter) && ch.col >= 0);
	return ch.col + 2;
}

/* ---------------------------------------------------------------------- */

/* Copies all the text selected to the selection buffer.  Whitespace at
 * end of lines is trimmed. */
static void
selarea_copy_text(void)
{
	char *ptr, *str;
	size_t l, r;

	if (Selarea.sa_y1 == Selarea.sa_y2) {
		/* Selection is one row */
		l = row_length(Selarea.sa_y1);
		if (Selarea.sa_x1 > l)
			/* Selection is after last character,
			 * therefore it is empty */
			str = NULL;
		else {
			if (Selarea.sa_x2 > l)
				Selarea.sa_x2 = l;
			ptr = str =
			    alloc_sel(Selarea.sa_x2 - Selarea.sa_x1 + 1);
			if (ptr == NULL)
				return;

			ptr = fill_buf(ptr, Selarea.sa_y1, Selarea.sa_x1,
			    Selarea.sa_x2);
			*ptr = '\0';
		}
	} else {
		/* Selection is multiple rows */
		ptr = str =
		    alloc_sel(Selarea.sa_endoff - Selarea.sa_startoff + 1);
		if (ptr == NULL)
			return;

		/* Calculate and copy first line */
		l = row_length(Selarea.sa_y1);
		if (Selarea.sa_x1 < l) {
			ptr = fill_buf(ptr, Selarea.sa_y1, Selarea.sa_x1, l);
			*ptr++ = '\r';
		}

		/* Copy mid lines if there are any */
		if ((Selarea.sa_y2 - Selarea.sa_y1) > 1) {
			for (r = Selarea.sa_y1 + 1; r <= Selarea.sa_y2 - 1;
			    r++) {
				ptr = fill_buf(ptr, r, 0, row_length(r));
				*ptr++ = '\r';
			}
		}

		/* Calculate and copy end line */
		l = row_length(Selarea.sa_y2);
		if (Selarea.sa_x2 < l)
			l = Selarea.sa_x2;
		ptr = fill_buf(ptr, Selarea.sa_y2, 0, l);
		*ptr = '\0';
	}

	if (Selarea.sa_buf != NULL) {
		free(Selarea.sa_buf);
		Selarea.sa_buf = NULL;
	}

	if (str != NULL) {
		Selarea.sa_buf = str;
		Selarea.sa_buflen = ptr - str;
	}
}

/* ---------------------------------------------------------------------- */

/* Starts a selection. */
static void
selarea_start(void)
{

	if (Selarea.sa_buf != NULL) {
		free(Selarea.sa_buf);
		Selarea.sa_buf = NULL;
	}

	Selarea.sa_y1 = Selmouse.sm_y;
	Selarea.sa_x1 = Selmouse.sm_x;
	selarea_calculate();
	Selmouse.sm_selecting = 1;
}

/* ---------------------------------------------------------------------- */

/* Ends a selection.  Highlighted text is copied to the buffer. */
static void
selarea_end(void)
{
	size_t i;

	selarea_calculate();

	/* Invert sel coordinates if needed */
	if (Selarea.sa_x1 > Selarea.sa_x2) {
		i = Selarea.sa_x2;
		Selarea.sa_x2 = Selarea.sa_x1;
		Selarea.sa_x1 = i;
	}
	if (Selarea.sa_y1 > Selarea.sa_y2) {
		i = Selarea.sa_y2;
		Selarea.sa_y2 = Selarea.sa_y1;
		Selarea.sa_y1 = i;
	}

	selarea_copy_text();
	Selmouse.sm_selecting = 0;
}

/* ---------------------------------------------------------------------- */

/* Calculates selection absolute positions in the screen buffer. */
static void
selarea_calculate(void)
{
	size_t i;
	
	i = Selmouse.sm_max_x + 1;
	Selarea.sa_y2 = Selmouse.sm_y;
	Selarea.sa_x2 = Selmouse.sm_x;
	Selarea.sa_startoff = Selarea.sa_y1 * i + Selarea.sa_x1;
	Selarea.sa_endoff = Selarea.sa_y2 * i + Selarea.sa_x2;

	if (Selarea.sa_startoff > Selarea.sa_endoff) {
		i = Selarea.sa_endoff;
		Selarea.sa_endoff = Selarea.sa_startoff;
		Selarea.sa_startoff = i;
	}
}

/* ---------------------------------------------------------------------- */

/* Hides the highlighted region, returning it to normal colors. */
static void
selarea_hide(void)
{
	size_t i;

	for (i = Selarea.sa_startoff; i <= Selarea.sa_endoff; i++)
		char_invert(0, i);
}

/* ---------------------------------------------------------------------- */

/* Highlights the selected region. */
static void
selarea_show(void)
{
	size_t i;

	selarea_calculate();
	for (i = Selarea.sa_startoff; i <= Selarea.sa_endoff; i++)
		char_invert(0, i);
}

/* ---------------------------------------------------------------------- */

/* Pastes selected text into the active console. */
static void
selarea_paste(void)
{
	size_t i;

	if (Selarea.sa_buf == NULL)
		return;
	for (i = 0; i < Selarea.sa_buflen; i++)
		if (ioctl(Selmouse.sm_ttyfd, TIOCSTI,
		    &Selarea.sa_buf[i]) == -1)
			log_warn("ioctl(TIOCSTI)");
}
