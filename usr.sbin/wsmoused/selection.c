/* $NetBSD: selection.c,v 1.1 2002/06/26 23:13:08 christos Exp $ */

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
__RCSID("$NetBSD: selection.c,v 1.1 2002/06/26 23:13:08 christos Exp $");
#endif /* not lint */

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/tty.h>
#include <dev/wscons/wsconsio.h>
 
#include <ctype.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathnames.h"
#include "wsmoused.h"

/* This struct holds information about a sel. For now there is
 * only one global instace, but using a structre gives us a place for
 * maintaining all the variables together. Also, someone may want to
 * allow multiple sels, so it is easier this way. */
static struct selection {
	size_t start_row, start_col;
	size_t end_row, end_col;
	size_t abs_start, abs_end;
	size_t text_size;
	char *text;
} sel;


static void *
alloc_sel(size_t len)
{
	void *ptr;
	if ((ptr = malloc(len)) == NULL) {
		warn("Cannot allocate memory for sel %lu",
		    (unsigned long)len);
		return NULL;
	}
	return ptr;
}

static void
fill_buf(char *ptr, struct mouse *m, size_t row, size_t col, size_t end)
{
	struct wsdisplay_char ch;
	ch.row = sel.start_row;
	for (ch.col = col; ch.col < end; ch.col++) {
		if (ioctl(m->tty_fd, WSDISPLAYIO_GETWSCHAR, &ch) == -1) {
			warn("ioctl(WSDISPLAYIO_GETWSCHAR) failed");
			*ptr++ = ' ';
		} else {
			*ptr++ = ch.letter;
		}
	}
}


/*
 * This function scans the specified line and checks its
 * length. Characters at the end of it which match isspace() are not
 * counted.
 */
static size_t
row_length(struct mouse *m, size_t row)
{
	struct wsdisplay_char ch;

	ch.col = m->max_col;
	ch.row = row;
	do {
		if (ioctl(m->tty_fd, WSDISPLAYIO_GETWSCHAR, &ch) == -1)
			warn("ioctl(WSDISPLAYIO_GETWSCHAR) failed");
		ch.col--;
	} while (isspace((unsigned char)ch.letter));
	return ch.col + 2;
}

/*
 * This (complex) function copies all the text englobed in the current
 * sel to the sel buffer. It does space trimming at end of
 * lines if it is selected.
 */
static void
sel_copy_text(struct mouse *m)
{
	char *str, *ptr;
	size_t r, l;

	if (sel.start_row == sel.end_row) {
		/* Selection is one row */
		l = row_length(m, sel.start_row);
		if (sel.start_col > l)
			/* Selection is after last character,
			 * therefore it is empty */
			str = NULL;
		else {
			if (sel.end_col > l)
				sel.end_col = l;
			ptr = str = alloc_sel( sel.end_col - sel.start_col + 1);
			if (ptr == NULL)
				return;
			
			fill_buf(ptr, m, sel.start_row, sel.start_col,
			    sel.end_col);
			*ptr = '\0';
		}
	} else {
		/* Selection is multiple rows */
		ptr = str =  alloc_sel(sel.abs_end - sel.abs_start + 1);
		if (ptr == NULL)
			return;

		/* Calculate and copy first line */
		l = row_length(m, sel.start_row);
		if (sel.start_col < l) {
			fill_buf(ptr, m, sel.start_row, sel.start_col, l);
			*ptr++ = '\r';
		}

		/* Copy mid lines if there are any */
		if ((sel.end_row - sel.start_row) > 1) {
			for (r = sel.start_row + 1; r <= sel.end_row - 1; r++) {
				fill_buf(ptr, m, r, 0, row_length(m, r));
				*ptr++ = '\r';
			}
		}

		/* Calculate and copy end line */
		l = row_length(m, sel.end_row);
		if (sel.end_col < l)
			l = sel.end_col;
		fill_buf(ptr, m, sel.end_row, 0, l);
		*ptr = '\0';
	}

	if (sel.text != NULL) {
		free(sel.text);
		sel.text = NULL;
	}
	
	if (str != NULL) {
		sel.text = str;
		sel.text_size = ptr - str;
	}
}

/*
 * Initializes sel data. It should be called only once, at
 * wsmoused startup to initialize pointers.
 */
void
mouse_sel_init()
{
	memset(&sel, 0, sizeof(struct selection));
}

/*
 * Starts a sel (when mouse is pressed).
 */
void
mouse_sel_start(struct mouse *m)
{
	if (sel.text != NULL) {
		free(sel.text);
		sel.text = NULL;
	}
	
	sel.start_row = m->row;
	sel.start_col = m->col;
	mouse_sel_calculate(m);
	m->selecting = 1;
}

/*
 * Ends a sel. Text is copied to memory for future pasting and
 * highlighted region is returned to normal state.
 */
void
mouse_sel_end(struct mouse *m)
{
	size_t i;

	mouse_sel_calculate(m);

	/* Invert sel coordinates if needed */
	if (sel.start_col > sel.end_col) {
		i = sel.end_col;
		sel.end_col = sel.start_col;
		sel.start_col = i;
	}
	if (sel.start_row > sel.end_row) {
		i = sel.end_row;
		sel.end_row = sel.start_row;
		sel.start_row = i;
	}

	sel_copy_text(m);
	m->selecting = 0;
}

/*
 * Calculates sel absolute postitions.
 */
void
mouse_sel_calculate(struct mouse *m)
{
	size_t i = m->max_col + 1;

	sel.end_row = m->row;
	sel.end_col = m->col;
	sel.abs_start = sel.start_row * i + sel.start_col;
	sel.abs_end = sel.end_row * i + sel.end_col;

	if (sel.abs_start > sel.abs_end) {
		i = sel.abs_end;
		sel.abs_end = sel.abs_start;
		sel.abs_start = i;
	}
}

/*
 * Hides highlighted region, returning it to normal colors.
 */
void
mouse_sel_hide(struct mouse *m)
{
	size_t i;

	for (i = sel.abs_start; i <= sel.abs_end; i++)
		char_invert(m, 0, i);
}

/*
 * Highlights selected region.
 */
void
mouse_sel_show(struct mouse *m)
{
	size_t i;
	
	mouse_sel_calculate(m);
	for (i = sel.abs_start; i <= sel.abs_end; i++)
		char_invert(m, 0, i);
}


/*
 * Pastes selected text into the active console.
 */
void
mouse_sel_paste(struct mouse *m)
{
	size_t i;
	
	if (sel.text == NULL)
		return;
	for (i = 0; i < sel.text_size; i++)
		if (ioctl(m->tty_fd, TIOCSTI, &sel.text[i]) == -1)
			warn("ioctl(TIOCSTI)");
}
