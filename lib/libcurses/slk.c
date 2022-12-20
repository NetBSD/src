/*	$NetBSD: slk.c,v 1.21 2022/12/20 04:57:01 blymn Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
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

#include <sys/cdefs.h>
#include <limits.h>
#ifndef lint
__RCSID("$NetBSD: slk.c,v 1.21 2022/12/20 04:57:01 blymn Exp $");
#endif				/* not lint */

#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_WCHAR
#include <wctype.h>
#endif

#include "curses.h"
#include "curses_private.h"

/* Terminals with real soft labels have NOT been tested.
 * If you have such a device, please let us know so this comment
 * can be adjusted. */

/* POSIX says that each label can be up to 8 columns.
 * However, our implementation can allow labels to expand beyond that. */
//#define	SLK_SIZE_DYNAMIC
#ifdef SLK_SIZE_DYNAMIC
#define	SLK_SIZE	MAX_SLK_LABEL
#else
#define	SLK_SIZE	MAX_SLK_COLS
#endif

static int	 slk_fmt = SLK_FMT_INVAL;	/* fmt of slk_init */

/* Safe variants of public functions. */
static int	 __slk_attroff(SCREEN *, const chtype);
static int	 __slk_attron(SCREEN *, const chtype);
static int	 __slk_attrset(SCREEN *, const chtype);
#ifdef HAVE_WCHAR
static int	 __slk_attr_off(SCREEN *, const attr_t, void *);
static int	 __slk_attr_on(SCREEN *, const attr_t, void *);
static int	 __slk_attr_set(SCREEN *, const attr_t, short, void *opt);
static int	 __slk_color(SCREEN *, short);
#endif

static int	 __slk_clear(SCREEN *);
static char	*__slk_label(SCREEN *, int);
static int	 __slk_restore(SCREEN *);
static int	 __slk_set(SCREEN *, int, const char *, int);
static int	 __slk_touch(SCREEN *);
#ifdef HAVE_WCHAR
static int	 __slk_wset(SCREEN *, int, const wchar_t *, int);
#endif

/* Internal engine parts. */
static int	 __slk_ripoffline(WINDOW *, int);
static int	 __slk_set_finalise(SCREEN *, int);
static int	 __slk_draw(SCREEN *, int);
static int	 __slk_redraw(SCREEN *);

/*
 * slk_init --
 *	Init Soft Label Keys.
 */
int
slk_init(int fmt)
{

	switch(fmt) {
	case SLK_FMT_3_2_3:
	case SLK_FMT_4_4:
		break;
	default:
		return ERR;
	}

	slk_fmt = fmt;
	/* Even if the terminal supports soft label keys directly,
	 * we need to reserve a line. */
	return ripoffline(-1, __slk_ripoffline);
}

/*
 * slk_attron --
 *	Test and set attributes on ripped off slk window.
 */
int
slk_attron(const chtype attr)
{

	return __slk_attron(_cursesi_screen, attr);
}

#ifdef HAVE_WCHAR
/*
 * slk_attr_on --
 *	Test and set wide attributes on ripped off slk window.
 */
int
slk_attr_on(const attr_t attr, void *opt)
{

	return __slk_attr_on(_cursesi_screen, attr, opt);
}
#endif	/* HAVE_WCHAR */

/*
 * slk_attroff --
 *	Test and unset attributes on ripped off slk window.
 */
int
slk_attroff(const chtype attr)
{

	return __slk_attroff(_cursesi_screen, attr);
}

#ifdef HAVE_WCHAR
/*
 * slk_attr_off --
 *	Test and unset wide attributes on ripped off slk window.
 */
int
slk_attr_off(const attr_t attr, void *opt)
{

	return __slk_attr_off(_cursesi_screen, attr, opt);
}
#endif	/* HAVE_WCHAR */

/*
 * slk_attrset --
 *	Set attributes and color pair on ripped off slk window.
 */
int
slk_attrset(const chtype attr)
{

	return __slk_attrset(_cursesi_screen, attr);
}

#ifdef HAVE_WCHAR
/*
 * slk_attr_set --
 *	Set wide attributes and color pair on ripped off slk window.
 */
int
slk_attr_set(const attr_t attr, short pair, void *opt)
{

	return __slk_attr_set(_cursesi_screen, attr, pair, opt);
}
#endif	/* HAVE_WCHAR */

/*
 * slk_clear --
 *	Clear slk from the current screen.
 */
int
slk_clear(void)
{

	return __slk_clear(_cursesi_screen);
}

#ifdef HAVE_WCHAR
/*
 * slk_color --
 *	Set color pair on ripped off slk window.
 */
int
slk_color(short pair)
{

	return __slk_color(_cursesi_screen, pair);
}
#endif	/* HAVE_WCHAR */

/*
 * slk_label --
 *	Return a pointer to the saved label for key labnum.
 */
char *
slk_label(int labnum)
{

	return __slk_label(_cursesi_screen, labnum);
}

/*
 * slk_wnoutrefresh --
 *	Add the contents of the ripped off slk window to the virtual window.
 */
int
slk_noutrefresh(void)
{

	return __slk_noutrefresh(_cursesi_screen);
}

/*
 * slk_refresh --
 *	Force a refresh for the ripped off slk window.
 */
int
slk_refresh(void)
{

	if (slk_noutrefresh() == ERR)
		return ERR;
	return doupdate();
}

/*
 * slk_restore --
 *	Retore slk to the screen after a slk_clear.
 */
int
slk_restore(void)
{

	return __slk_restore(_cursesi_screen);
}

/*
 * slk_set --
 *	Sets the text of the label specified by labnum
 *	and how it is displayed.
 */
int
slk_set(int labnum, const char *label, int justify)
{

	return __slk_set(_cursesi_screen, labnum, label, justify);
}

/*
 * slk_touch --
 *	Sets the ripped off slk window as modified.
 */
int
slk_touch(void)
{

	return __slk_touch(_cursesi_screen);
}

#ifdef HAVE_WCHAR
/*
 * slk_wset --
 *	Sets the wide text of the label specified by labnum
 *	and how it is displayed.
 */
int
slk_wset(int labnum, const wchar_t *label, int justify)
{

	return __slk_wset(_cursesi_screen, labnum, label, justify);
}
#endif	/* HAVE_WCHAR */

/*
 * __slk_attron --
 *	Test and set attributes on ripped off slk window.
 */
static int
__slk_attron(SCREEN *screen, const chtype attr)
{

	if (screen == NULL || screen->slk_window == NULL)
		return ERR;
	return wattron(screen->slk_window, attr);
}

#ifdef HAVE_WCHAR
/*
 * __slk_attr_on --
 *	Test and set wide attributes on ripped off slk window.
 */
static int
__slk_attr_on(SCREEN *screen, const attr_t attr, void *opt)
{

	if (screen == NULL || screen->slk_window == NULL)
		return ERR;
	return wattr_on(screen->slk_window, attr, opt);
}
#endif	/* HAVE_WCHAR */

/*
 * __slk_attroff --
 *	Test and unset attributes on ripped off slk window.
 */
static int
__slk_attroff(SCREEN *screen, const chtype attr)
{

	if (screen == NULL || screen->slk_window == NULL)
		return ERR;
	return wattroff(screen->slk_window, attr);
}

#ifdef HAVE_WCHAR
/*
 * __slk_attr_off --
 *	Test and unset wide attributes on ripped off slk window.
 */
static int
__slk_attr_off(SCREEN *screen, const attr_t attr, void *opt)
{

	if (screen == NULL || screen->slk_window == NULL)
		return ERR;
	return wattr_off(screen->slk_window, attr, opt);
}
#endif	/* HAVE_WCHAR */

/*
 * __slk_attrset --
 *	Set attributes and color pair on ripped off slk window.
 */
static int
__slk_attrset(SCREEN *screen, const chtype attr)
{

	if (screen == NULL || screen->slk_window == NULL)
		return ERR;
	return wattrset(screen->slk_window, attr);
}

#ifdef HAVE_WCHAR
/*
 * __slk_attr_set --
 *	Set wide attributes and color pair on ripped off slk window.
 */
static int
__slk_attr_set(SCREEN *screen, const attr_t attr, short pair, void *opt)
{

	if (screen == NULL || screen->slk_window == NULL)
		return ERR;
	return wattr_set(screen->slk_window, attr, pair, opt);
}
#endif	/* HAVE_WCHAR */

/*
 * __slk_clear --
 *	Clear slk from the current screen.
 */
static int
__slk_clear(SCREEN *screen)
{

	if (screen == NULL)
		return ERR;
	screen->slk_hidden = true;
	if (screen->is_term_slk) {
		if (t_label_off(screen->term) == NULL)
			return ERR;
		return ti_putp(screen->term,
		    ti_tiparm(screen->term, t_label_off(screen->term)));
	}
	if (screen->slk_window == NULL)
		return ERR;
	werase(screen->slk_window);
	return wrefresh(screen->slk_window);
}

#ifdef HAVE_WCHAR
/*
 * __slk_color --
 *	Set color pair on ripped off slk window.
 */
static int
__slk_color(SCREEN *screen, short pair)
{

	if (screen == NULL || screen->slk_window == NULL)
		return ERR;
	return wcolor_set(screen->slk_window, pair, NULL);
}
#endif	/* HAVE_WCHAR */

/*
 * __slk_label --
 *	Return a pointer to the saved label for key labnum.
 */
static char *
__slk_label(SCREEN *screen, int labnum)
{

	if (screen == NULL || labnum < 1 || labnum > screen->slk_nlabels)
		return NULL;
	return screen->slk_labels[--labnum].text;
}

/*
 * __slk_wnoutrefresh --
 *	Add the contents of the ripped off slk window to the virtual window.
 */
int
__slk_noutrefresh(SCREEN *screen)
{

	if (screen == NULL || screen->slk_window == NULL)
		return ERR;
	return wnoutrefresh(screen->slk_window);
}

/*
 * __slk_restore --
 *	Retore slk to the screen after a slk_clear.
 */
static int
__slk_restore(SCREEN *screen)
{

	if (screen == NULL)
		return ERR;
	screen->slk_hidden = false;
	if (screen->is_term_slk) {
		if (t_label_on(screen->term) == NULL)
			return ERR;
		return ti_putp(screen->term,
		    ti_tiparm(screen->term, t_label_on(screen->term)));
	}
	if (screen->slk_window == NULL)
		return ERR;
	if (__slk_redraw(screen) == ERR)
		return ERR;
	return wrefresh(screen->slk_window);
}

/*
 * __slk_set --
 *	Sets the text of the label specified by labnum
 *	and how it is displayed.
 */
static int
__slk_set(SCREEN *screen, int labnum, const char *label, int justify)
{
	struct __slk_label *l;
	const char *end;
	size_t len;
	char *text;
#ifdef HAVE_WCHAR
	wchar_t wc;
	size_t wc_len;
#endif

	/* Check args. */
	if (screen == NULL || labnum < 1 || labnum > screen->slk_nlabels)
		return ERR;
	switch(justify) {
	case SLK_JUSTIFY_LEFT:
	case SLK_JUSTIFY_CENTER:
	case SLK_JUSTIFY_RIGHT:
		break;
	default:
		return ERR;
	}
	if (label == NULL)
		label = "";

	/* Skip leading whitespace. */
	while(isspace((unsigned char)*label))
		label++;
	/* Grab end. */
	end = label;

#ifdef HAVE_WCHAR
	size_t endlen = strlen(end);
	while (*end != '\0') {
		wc_len = mbrtowc(&wc, end, endlen, &screen->sp);
		if ((ssize_t)wc_len < 0)
			return ERR;
		if (!iswprint((wint_t)wc))
			break;
		end += wc_len;
		endlen -= wc_len;
	}
#else
	while(isprint((unsigned char)*end))
		end++;
#endif
	len = end - label;

	/* Take a backup, in-case we can grow the label. */
	if ((text = strndup(label, len)) == NULL)
		return ERR;

	/* All checks out, assign. */
	l = &screen->slk_labels[--labnum]; /* internal zero based index */
	l->text = text;
	l->justify = justify;

	__slk_set_finalise(screen, labnum);
	return OK;
}

/*
 * __slk_touch --
 *	Sets the ripped off slk window as modified.
 */
static int
__slk_touch(SCREEN *screen)
{

	if (screen == NULL || screen->slk_window == NULL)
		return ERR;
	return touchwin(screen->slk_window);
}


#ifdef HAVE_WCHAR
/*
 * __slk_wset --
 *	Sets the wide text of the label specified by labnum
 *	and how it is displayed.
 */
static int
__slk_wset(SCREEN *screen, int labnum, const wchar_t *label, int justify)
{
	const wchar_t *olabel;
	size_t len;
	char *str;
	int result = ERR;

	if (screen == NULL)
		return ERR;
	__CTRACE(__CTRACE_INPUT, "__slk_wset: entry\n");
	olabel = label;
	if ((len = wcsrtombs(NULL, &olabel, 0, &screen->sp)) == -1) {
	__CTRACE(__CTRACE_INPUT,
	    "__slk_wset: conversion failed on char 0x%hx\n",
	    (uint16_t)*olabel);
		return ERR;
	}

	__CTRACE(__CTRACE_INPUT, "__slk_wset: wcsrtombs %zu\n", len);
	len++; /* We need to store the NULL character. */
	if ((str = malloc(len)) == NULL)
		return ERR;
	olabel = label;
	if (wcsrtombs(str, &olabel, len, &screen->sp) == -1)
		goto out;
	result = __slk_set(screen, labnum, str, justify);
out:
	free(str);
	__CTRACE(__CTRACE_INPUT, "__slk_wset: return %s\n",
	    result == OK ? "OK" : "ERR");
	return result;
}
#endif	/* HAVE_WCHAR */


/*
 * __slk_init --
 *	Allocate structures.
 */
int
__slk_init(SCREEN *screen)
{

	__slk_free(screen);	/* safety */

	screen->slk_format = slk_fmt;
	if (slk_fmt == SLK_FMT_INVAL)
		return OK;
	slk_fmt = SLK_FMT_INVAL;

	switch(screen->slk_format) {
	case SLK_FMT_3_2_3:
	case SLK_FMT_4_4:
		screen->slk_nlabels = 8;
		break;
	default:	/* impossible */
		return ERR;
	}

	screen->slk_labels = calloc(screen->slk_nlabels,
				    sizeof(*screen->slk_labels));
	if (screen->slk_labels == NULL)
		return ERR;

	screen->is_term_slk =
	    t_plab_norm(screen->term) != NULL &&
	    t_num_labels(screen->term) > 0;
	if (screen->is_term_slk) {
		__unripoffline(__slk_ripoffline);
		screen->slk_nlabels = t_num_labels(screen->term);
		screen->slk_label_len = t_label_width(screen->term);
		/* XXX label_height, label_format? */
	}

	return OK;
}

/*
 * __slk_free --
 *	Free allocates resources.
 */
void
__slk_free(SCREEN *screen)
{
	int i;

	if (screen->slk_window != NULL)
		delwin(screen->slk_window);
	for (i = 0; i < screen->slk_nlabels; i++)
		free(screen->slk_labels[i].text);
	free(screen->slk_labels);
}

/*
 * __slk_ripoffline --
 *	ripoffline callback to accept a WINDOW to create our keys.
 */
static int
__slk_ripoffline(WINDOW *window, int cols)
{

	if (window == NULL)
		return ERR;
	window->screen->slk_window = window;
	wattron(window,
	    (t_no_color_video(window->screen->term) & 1) == 0
	    ? A_STANDOUT : A_REVERSE);
	__slk_resize(window->screen, cols);
	return OK;
}

/*
 * __slk_resize --
 *	Size and position the labels in the ripped off slk window.
 */
int
__slk_resize(SCREEN *screen, int cols)
{
	int x = 0;
	struct __slk_label *l;

	if (screen == NULL)
		return ERR;
	if (screen->is_term_slk || screen->slk_nlabels == 0)
		return OK;

	screen->slk_label_len = (cols / screen->slk_nlabels) - 1;
	if (screen->slk_label_len > SLK_SIZE)
		screen->slk_label_len = SLK_SIZE;

	l = screen->slk_labels;

	switch(screen->slk_format) {
	case SLK_FMT_3_2_3:
		/* Left 3 */
		(l++)->x = x;
		(l++)->x = (x += screen->slk_label_len + 1);
		(l++)->x = (x += screen->slk_label_len + 1);

		/* Middle 2 */
		x = cols / 2;
		(l++)->x = x -(screen->slk_label_len + 1);
		(l++)->x = x + 1;

		/* Right 3 */
		x = (cols - ((screen->slk_label_len + 1) * 3)) + 1;
		(l++)->x = x;
		(l++)->x = (x += screen->slk_label_len + 1);
		(l++)->x = (x += screen->slk_label_len + 1);
		break;

	case SLK_FMT_4_4:
	{
		int i, half;

		half = screen->slk_nlabels / 2;
		for (i = 0; i < screen->slk_nlabels; i++) {
			(l++)->x = x;
			x += screen->slk_label_len;
			/* Split labels in half */
			if (i == half - 1)
				x = cols - (screen->slk_label_len * half) + 1;
		}
		break;
	}
	}

	/* Write text to the labels. */
	for (x = 0; x < screen->slk_nlabels; x++)
		__slk_set_finalise(screen, x);

	return __slk_redraw(screen);
}

/*
 * __slk_set_finalise --
 *	Does the grunt work of positioning and sizing the text in the label.
 */
static int
__slk_set_finalise(SCREEN *screen, int labnum)
{
	struct __slk_label *l;
	size_t spc, len, width, x;
	char *p;

	l = &screen->slk_labels[labnum];
	spc = screen->slk_label_len;

#ifdef HAVE_WCHAR
	len = 0;
	width = 0;
	if (l->text != NULL) {
		size_t plen;

		p = l->text;
		plen = strlen(l->text);
		while (*p != '\0') {
			size_t mblen;
			wchar_t wc;
			int w;

			mblen = mbrtowc(&wc, p, plen, &screen->sp);
			if ((ssize_t)mblen < 0)
				return ERR;
			w = wcwidth(wc);
			if (width + w > spc)
				break;
			width += w;
			len += mblen;
			p += mblen;
			plen -= mblen;
		}
	}
#else
	len = l->text == NULL ? 0 : strlen(l->text);
	if (len > spc)
		len = spc;
	width = len;
#endif

	switch(l->justify) {
	case SLK_JUSTIFY_LEFT:
		x = 0;
		break;
	case SLK_JUSTIFY_CENTER:
		x = (spc - width) / 2;
		if (x + width > spc)
			x--;
		break;
	case SLK_JUSTIFY_RIGHT:
		x = spc - width;
		break;
	default:
		return ERR; /* impossible */
	}

	p = l->label;
	if (x != 0) {
		memset(p, ' ', x);
		p += x;
		spc -= x;
	}
	if (len != 0) {
		memcpy(p, l->text, len);
		p += len;
		spc -= width;
	}
	if (spc != 0) {
		memset(p, ' ', spc);
		p += spc;
	}
	*p = '\0'; /* Terminate for plab_norm. */

	return __slk_draw(screen, labnum);
}

/*
 * __slk_draw --
 *	Draws the specified key.
 */
static int
__slk_draw(SCREEN *screen, int labnum)
{
	const struct __slk_label *l;
	int retval, inc, lcnt, tx;
	char ts[MB_LEN_MAX];
#ifdef HAVE_WCHAR
	cchar_t cc;
	wchar_t wc[2];
#endif

	__CTRACE(__CTRACE_INPUT, "__slk_draw: screen %p, label %d\n", screen,
	    labnum);

	if (screen->slk_hidden)
		return OK;

	retval = OK; /* quiet gcc... */

	l = &screen->slk_labels[labnum];
	if (screen->is_term_slk)
		return ti_putp(screen->term,
		    ti_tiparm(screen->term,
		    t_plab_norm(screen->term), labnum + 1, l->label));
	else if (screen->slk_window != NULL) {
		if ((labnum != screen->slk_nlabels - 1) ||
		    (screen->slk_window->flags & __SCROLLOK) ||
		    ((l->x + screen->slk_label_len) < screen->slk_window->maxx)) {
			retval = mvwaddnstr(screen->slk_window, 0, l->x,
			    l->label, strlen(l->label));
		} else {
			lcnt = 0;
			tx = 0;
			while (lcnt < screen->slk_label_len) {
				inc = wctomb(ts, l->label[lcnt]);
				if (inc < 0) {
					/* conversion failed, skip? */
					lcnt++;
					continue;
				}

	__CTRACE(__CTRACE_INPUT,
	    "__slk_draw: last label, (%d,%d) char[%d] 0x%x\n",
	    l->x + tx, 0, lcnt, l->label[lcnt]);
	__CTRACE(__CTRACE_INPUT, "__slk_draw: label len %d, wcwidth %d\n",
	    screen->slk_label_len, wcwidth(l->label[lcnt]));
#ifdef HAVE_WCHAR
				wc[0] = l->label[lcnt];
				wc[1] = L'\0';
				if (setcchar(&cc, wc,
				    screen->slk_window->wattr, 0,
				    NULL) == ERR)
					return ERR;
#endif

				if (l->x + wcwidth(l->label[lcnt] + tx) >=
				    screen->slk_label_len) {
					/* last character that will fit
					 * so insert it to avoid scroll
					 */
#ifdef HAVE_WCHAR
					retval = mvwins_wch(screen->slk_window,
					    0, l->x + tx, &cc);
#else
					retval = mvwinsch(screen->slk_window,
					    0, l->x + tx, l->label[lcnt]);
#endif
				} else {
#ifdef HAVE_WCHAR
					retval = mvwadd_wch(screen->slk_window,
					    0, l->x + tx, &cc);
#else
					retval = mvwaddch(screen->slk_window,
					    0, l->x + tx, l->label[lcnt]);
#endif
				}
				tx += wcwidth(l->label[lcnt]);
				lcnt += inc;
			}
		}

		return retval;
	} else
		return ERR;
}

/*
 * __slk_draw --
 *	Draws all the keys.
 */
static int
__slk_redraw(SCREEN *screen)
{
	int i, result = OK;

	for (i = 0; i < screen->slk_nlabels; i++) {
		if (__slk_draw(screen, i) == ERR)
			result = ERR;
	}
	return result;
}
