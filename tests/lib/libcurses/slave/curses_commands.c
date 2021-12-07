/*	$NetBSD: curses_commands.c,v 1.31 2021/12/07 06:55:44 rillig Exp $	*/

/*-
 * Copyright 2009 Brett Lymn <blymn@NetBSD.org>
 * Copyright 2021 Roland Illig <rillig@NetBSD.org>
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <stdarg.h>

#include "slave.h"
#include "curses_commands.h"

static int
set_int(const char *arg, int *x)
{
	if (sscanf(arg, "%d", x) == 0) {
		report_count(1);
		report_error("BAD ARGUMENT");
		return -1;
	}

	return 0;
}

static int
set_uint(const char *arg, unsigned int *x)
{
	if (sscanf(arg, "%u", x) == 0) {
		report_count(1);
		report_error("BAD ARGUMENT");
		return -1;
	}

	return 0;
}

static int
set_short(const char *arg, short *x)
{
	if (sscanf(arg, "%hd", x) == 0) {
		report_count(1);
		report_error("BAD ARGUMENT");
		return -1;
	}

	return 0;
}

static int
set_win(const char *arg, WINDOW **x)
{
	if (sscanf(arg, "%p", x) == 0) {
		report_count(1);
		report_error("BAD ARGUMENT");
		return -1;
	}

	return 0;
}

static int
set_scrn(const char *arg, SCREEN **x)
{
	if (sscanf(arg, "%p", x) == 0) {
		report_count(1);
		report_error("BAD ARGUMENT");
		return -1;
	}

	return 0;
}

#define ARGC(n) \
	if (check_arg_count(nargs, n) == 1)				\
		return

#define ARG_SHORT(arg) \
	short arg;							\
	if (set_short(*args++, &arg) != 0)				\
		return

#define ARG_INT(arg) \
	int arg;							\
	if (set_int(*args++, &arg) != 0)				\
		return

#define ARG_UINT(arg) \
	unsigned int arg;						\
	if (set_uint(*args++, &arg) != 0)				\
		return

#define ARG_CHTYPE(arg) \
	chtype arg = ((const chtype *)*args++)[0]

#define ARG_WCHAR(arg) \
	wchar_t arg = ((const wchar_t *)*args++)[0]

#define ARG_STRING(arg) \
	const char *arg = *args++

/* Only used for legacy interfaces that are missing the 'const'. */
#define ARG_MODIFIABLE_STRING(arg) \
	char *arg = *args++

#define ARG_CHTYPE_STRING(arg) \
	const chtype *arg = (const chtype *)*args++

#define ARG_CCHAR_STRING(arg) \
	const cchar_t *arg = (const cchar_t *)*args++

#define ARG_WCHAR_STRING(arg) \
	wchar_t *arg = (wchar_t *)*args++

#define ARG_WINDOW(arg) \
	WINDOW *arg;							\
	if (set_win(*args++, &arg) != 0)				\
		return

#define ARG_SCREEN(arg) \
	SCREEN *arg;							\
	if (set_scrn(*args++, &arg) != 0)				\
		return

/*
 * These NULL arguments are required by the API, intended for future
 * extensions, but this implementation does not support any extensions.
 */
#define ARG_NULL() \
	args++

#define ARG_IGNORE() \
	args++

void
cmd_DRAIN(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	while (wgetch(win) != ERR)
		continue;

	report_count(1);
	report_return(OK);
}

void
cmd_addbytes(int nargs, char **args)
{
	ARGC(2);
	ARG_STRING(str);
	ARG_INT(count);

	report_count(1);
	report_return(addbytes(str, count));
}


void
cmd_addch(int nargs, char **args)
{
	ARGC(1);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(addch(ch));
}


void
cmd_addchnstr(int nargs, char **args)
{
	ARGC(2);
	ARG_CHTYPE_STRING(chstr);
	ARG_INT(count);

	report_count(1);
	report_return(addchnstr(chstr, count));
}


void
cmd_addchstr(int nargs, char **args)
{
	ARGC(1);
	ARG_CHTYPE_STRING(chstr);

	report_count(1);
	report_return(addchstr(chstr));
}


void
cmd_addnstr(int nargs, char **args)
{
	ARGC(2);
	ARG_STRING(str);
	ARG_INT(count);

	report_count(1);
	report_return(addnstr(str, count));
}


void
cmd_addstr(int nargs, char **args)
{
	ARGC(1);
	ARG_STRING(str);

	report_count(1);
	report_return(addstr(str));
}


void
cmd_attr_get(int nargs, char **args)
{
	attr_t attrs;
	short colours;
	int retval;

	ARGC(0);

	retval = attr_get(&attrs, &colours, NULL);

	report_count(3);
	report_return(retval);
	report_int(attrs);
	report_int(colours);
}


void
cmd_attr_off(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(attrib);

	report_count(1);
	report_return(attr_off(attrib, NULL));
}


void
cmd_attr_on(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(attrib);

	report_count(1);
	report_return(attr_on(attrib, NULL));
}


void
cmd_attr_set(int nargs, char **args)
{
	ARGC(2);
	ARG_INT(attrib);
	ARG_SHORT(pair);

	report_count(1);
	report_return(attr_set(attrib, pair, NULL));
}


void
cmd_attroff(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(attrib);

	report_count(1);
	report_return(attroff(attrib));
}


void
cmd_attron(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(attrib);

	report_count(1);
	report_return(attron(attrib));
}


void
cmd_attrset(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(attrib);

	report_count(1);
	report_return(attrset(attrib));
}


void
cmd_bkgd(int nargs, char **args)
{
	ARGC(1);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(bkgd(ch));
}


void
cmd_bkgdset(int nargs, char **args)
{
	ARGC(1);
	ARG_CHTYPE(ch);

	bkgdset(ch);		/* returns void */
	report_count(1);
	report_return(OK);
}


void
cmd_border(int nargs, char **args)
{
	ARGC(8);
	ARG_INT(ls);
	ARG_INT(rs);
	ARG_INT(ts);
	ARG_INT(bs);
	ARG_INT(tl);
	ARG_INT(tr);
	ARG_INT(bl);
	ARG_INT(br);

	report_count(1);
	report_return(border(ls, rs, ts, bs, tl, tr, bl, br));
}


void
cmd_clear(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(clear());
}


void
cmd_clrtobot(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(clrtobot());
}


void
cmd_clrtoeol(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(clrtoeol());
}


void
cmd_color_set(int nargs, char **args)
{
	ARGC(2);
	ARG_SHORT(colour_pair);
	ARG_NULL();

	report_count(1);
	report_return(color_set(colour_pair, NULL));
}


void
cmd_delch(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(delch());
}


void
cmd_deleteln(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(deleteln());
}


void
cmd_echochar(int nargs, char **args)
{
	ARGC(1);
	ARG_CHTYPE(ch);

	/* XXX causes refresh */
	report_count(1);
	report_return(echochar(ch));
}


void
cmd_erase(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(erase());
}


void
cmd_getch(int nargs, char **args)
{
	ARGC(0);

	/* XXX causes refresh */
	report_count(1);
	report_int(getch());
}


void
cmd_getnstr(int nargs, char **args)
{
	char *string;

	ARGC(1);
	ARG_INT(limit);

	if ((string = malloc(limit + 1)) == NULL) {
		report_count(1);
		report_error("MALLOC_FAILED");
		return;
	}

	report_count(2);
	report_return(getnstr(string, limit));
	report_status(string);
	free(string);
}


void
cmd_getstr(int nargs, char **args)
{
	char string[256];

	ARGC(0);

	report_count(2);
	report_return(getstr(string));
	report_status(string);
}


void
cmd_inch(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_byte(inch());
}


void
cmd_inchnstr(int nargs, char **args)
{
	chtype *string;

	ARGC(1);
	ARG_INT(limit);

	if ((string = malloc((limit + 1) * sizeof(chtype))) == NULL) {
		report_count(1);
		report_error("MALLOC_FAILED");
		return;
	}

	report_count(2);
	report_return(inchnstr(string, limit));
	report_nstr(string);
	free(string);
}


void
cmd_inchstr(int nargs, char **args)
{
	chtype string[256];

	ARGC(0);

	report_count(2);
	report_return(inchstr(string));
	report_nstr(string);
}


void
cmd_innstr(int nargs, char **args)
{
	char *string;

	ARGC(1);
	ARG_INT(limit);

	if ((string = malloc(limit + 1)) == NULL) {
		report_count(1);
		report_error("MALLOC_FAILED");
		return;
	}

	report_count(2);
	report_int(innstr(string, limit));
	report_status(string);
	free(string);
}


void
cmd_insch(int nargs, char **args)
{
	ARGC(1);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(insch(ch));
}


void
cmd_insdelln(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(nlines);

	report_count(1);
	report_return(insdelln(nlines));
}


void
cmd_insertln(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(insertln());
}


void
cmd_instr(int nargs, char **args)
{
	char string[256];

	ARGC(0);

	report_count(2);
	report_return(instr(string));
	report_status(string);
}


void
cmd_move(int nargs, char **args)
{
	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_return(move(y, x));
}


void
cmd_refresh(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(refresh());
}


void
cmd_scrl(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(nlines);

	report_count(1);
	report_return(scrl(nlines));
}


void
cmd_setscrreg(int nargs, char **args)
{
	ARGC(2);
	ARG_INT(top);
	ARG_INT(bottom);

	report_count(1);
	report_return(setscrreg(top, bottom));
}


void
cmd_standend(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(standend());
}


void
cmd_standout(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(standout());
}


void
cmd_timeout(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(tval);

	timeout(tval);		/* void return */
	report_count(1);
	report_return(OK);
}


void
cmd_underscore(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(underscore());
}


void
cmd_underend(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(underend());
}


void
cmd_waddbytes(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_STRING(str);
	ARG_INT(count);

	report_count(1);
	report_return(waddbytes(win, str, count));
}


void
cmd_waddstr(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_STRING(str);

	report_count(1);
	report_return(waddstr(win, str));
}


void
cmd_mvaddbytes(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(str);
	ARG_INT(count);

	report_count(1);
	report_return(mvaddbytes(y, x, str, count));
}


void
cmd_mvaddch(int nargs, char **args)
{
	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(mvaddch(y, x, ch));
}


void
cmd_mvaddchnstr(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE_STRING(chstr);
	ARG_INT(count);

	report_count(1);
	report_return(mvaddchnstr(y, x, chstr, count));
}


void
cmd_mvaddchstr(int nargs, char **args)
{
	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE_STRING(chstr);

	report_count(1);
	report_return(mvaddchstr(y, x, chstr));
}


void
cmd_mvaddnstr(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(str);
	ARG_INT(count);

	report_count(1);
	report_return(mvaddnstr(y, x, str, count));
}


void
cmd_mvaddstr(int nargs, char **args)
{
	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(str);

	report_count(1);
	report_return(mvaddstr(y, x, str));
}


void
cmd_mvdelch(int nargs, char **args)
{
	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_return(mvdelch(y, x));
}


void
cmd_mvgetch(int nargs, char **args)
{
	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_int(mvgetch(y, x));
}


void
cmd_mvgetnstr(int nargs, char **args)
{
	char *string;

	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(count);

	if ((string = malloc(count + 1)) == NULL) {
		report_count(1);
		report_error("MALLOC_FAILED");
		return;
	}

	report_count(2);
	report_return(mvgetnstr(y, x, string, count));
	report_status(string);
	free(string);
}


void
cmd_mvgetstr(int nargs, char **args)
{
	char string[256];

	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvgetstr(y, x, string));
	report_status(string);
}


void
cmd_mvinch(int nargs, char **args)
{
	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_byte(mvinch(y, x));
}


void
cmd_mvinchnstr(int nargs, char **args)
{
	chtype *string;

	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(count);

	if ((string = malloc((count + 1) * sizeof(chtype))) == NULL) {
		report_count(1);
		report_error("MALLOC_FAILED");
		return;
	}

	report_count(2);
	report_return(mvinchnstr(y, x, string, count));
	report_nstr(string);
	free(string);
}


void
cmd_mvinchstr(int nargs, char **args)
{
	chtype string[256];

	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvinchstr(y, x, string));
	report_nstr(string);
}


void
cmd_mvinnstr(int nargs, char **args)
{
	char *string;

	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(count);

	if ((string = malloc(count + 1)) == NULL) {
		report_count(1);
		report_error("MALLOC_FAILED");
		return;
	}

	report_count(2);
	report_int(mvinnstr(y, x, string, count));
	report_status(string);
	free(string);
}


void
cmd_mvinsch(int nargs, char **args)
{
	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(mvinsch(y, x, ch));
}


void
cmd_mvinstr(int nargs, char **args)
{
	char string[256];

	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvinstr(y, x, string));
	report_status(string);
}


void
cmd_mvwaddbytes(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(str);
	ARG_INT(count);

	report_count(1);
	report_return(mvwaddbytes(win, y, x, str, count));
}


void
cmd_mvwaddch(int nargs, char **args)
{
	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(mvwaddch(win, y, x, ch));
}


void
cmd_mvwaddchnstr(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE_STRING(chstr);
	ARG_INT(count);

	report_count(1);
	report_return(mvwaddchnstr(win, y, x, chstr, count));
}


void
cmd_mvwaddchstr(int nargs, char **args)
{
	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE_STRING(chstr);

	report_count(1);
	report_return(mvwaddchstr(win, y, x, chstr));
}


void
cmd_mvwaddnstr(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(str);
	ARG_INT(count);

	report_count(1);
	report_return(mvwaddnstr(win, y, x, str, count));
}


void
cmd_mvwaddstr(int nargs, char **args)
{
	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(str);

	report_count(1);
	report_return(mvwaddstr(win, y, x, str));
}


void
cmd_mvwdelch(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_return(mvwdelch(win, y, x));
}


void
cmd_mvwgetch(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	/* XXX - implicit refresh */
	report_count(1);
	report_int(mvwgetch(win, y, x));
}


void
cmd_mvwgetnstr(int nargs, char **args)
{
	char *string;

	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(count);

	if ((string = malloc(count + 1)) == NULL) {
		report_count(1);
		report_error("MALLOC_FAILED");
		return;
	}

	report_count(2);
	report_return(mvwgetnstr(win, y, x, string, count));
	report_status(string);
	free(string);
}


void
cmd_mvwgetstr(int nargs, char **args)
{
	char string[256];

	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvwgetstr(win, y, x, string));
	report_status(string);
}


void
cmd_mvwinch(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_byte(mvwinch(win, y, x));
}


void
cmd_mvwinsch(int nargs, char **args)
{
	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(mvwinsch(win, y, x, ch));
}


void
cmd_assume_default_colors(int nargs, char **args)
{
	ARGC(2);
	ARG_SHORT(fore);
	ARG_SHORT(back);

	report_count(1);
	report_return(assume_default_colors(fore, back));
}


void
cmd_baudrate(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(baudrate());
}


void
cmd_beep(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(beep());
}


void
cmd_box(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_CHTYPE(vertical);
	ARG_CHTYPE(horizontal);

	report_count(1);
	report_return(box(win, vertical, horizontal));
}


void
cmd_can_change_color(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(can_change_color());
}


void
cmd_cbreak(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(cbreak());
}


void
cmd_clearok(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(flag);

	report_count(1);
	report_return(clearok(win, flag));
}


void
cmd_color_content(int nargs, char **args)
{
	ARGC(1);
	ARG_SHORT(colour);

	short red, green, blue;
	int ret = color_content(colour, &red, &green, &blue);

	report_count(4);
	report_return(ret);
	report_int(red);
	report_int(green);
	report_int(blue);
}


void
cmd_copywin(int nargs, char **args)
{
	ARGC(9);
	ARG_WINDOW(source);
	ARG_WINDOW(destination);
	ARG_INT(sminrow);
	ARG_INT(smincol);
	ARG_INT(dminrow);
	ARG_INT(dmincol);
	ARG_INT(dmaxrow);
	ARG_INT(dmaxcol);
	ARG_INT(ovlay);

	report_count(1);
	report_return(copywin(source, destination, sminrow, smincol, dminrow,
		dmincol, dmaxrow, dmaxcol, ovlay));
}


void
cmd_curs_set(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(vis);

	report_count(1);
	report_int(curs_set(vis));
}


void
cmd_def_prog_mode(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(def_prog_mode());
}


void
cmd_def_shell_mode(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(def_shell_mode());
}


void
cmd_define_key(int nargs, char **args)
{
	ARGC(2);
	ARG_MODIFIABLE_STRING(sequence);
	ARG_INT(symbol);

	report_count(1);
	report_return(define_key(sequence, symbol));
}


void
cmd_delay_output(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(dtime);

	report_count(1);
	report_return(delay_output(dtime));
}


void
cmd_delscreen(int nargs, char **args)
{
	ARGC(1);
	ARG_SCREEN(scrn);

	delscreen(scrn);	/* void return */

	report_count(1);
	report_return(OK);
}


void
cmd_delwin(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(delwin(win));
}


void
cmd_derwin(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(lines);
	ARG_INT(cols);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_ptr(derwin(win, lines, cols, y, x));
}


void
cmd_dupwin(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_ptr(dupwin(win));
}


void
cmd_doupdate(int nargs, char **args)
{
	ARGC(0);

	/* XXX - implicit refresh */
	report_count(1);
	report_return(doupdate());
}


void
cmd_echo(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(echo());
}


void
cmd_endwin(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(endwin());
}


void
cmd_erasechar(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(erasechar());
}


void
cmd_flash(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(flash());
}


void
cmd_flushinp(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(flushinp());
}


void
cmd_flushok(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(flag);

	report_count(1);
	report_return(flushok(win, flag));
}


void
cmd_fullname(int nargs, char **args)
{
	char string[256];

	ARGC(1);
	ARG_STRING(termbuf);

	report_count(2);
	report_status(fullname(termbuf, string));
	report_status(string);
}


void
cmd_getattrs(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(getattrs(win));
}


void
cmd_getbkgd(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_byte(getbkgd(win));
}


void
cmd_getcury(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(getcury(win));
}


void
cmd_getcurx(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(getcurx(win));
}


void
cmd_getyx(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	int y, x;
	getyx(win, y, x);
	report_count(2);
	report_int(y);
	report_int(x);
}


void
cmd_getbegy(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(getbegy(win));
}


void
cmd_getbegx(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(getbegx(win));
}


void
cmd_getmaxy(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(getmaxy(win));
}


void
cmd_getmaxx(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(getmaxx(win));
}


void
cmd_getpary(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(getpary(win));
}


void
cmd_getparx(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(getparx(win));
}


void
cmd_getparyx(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	int y, x;
	report_count(2);
	getparyx(win, y, x);
	report_int(y);
	report_int(x);
}

void
cmd_getmaxyx(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	int y, x;
	getmaxyx(win, y, x);

	report_count(2);
	report_int(y);
	report_int(x);
}

void
cmd_getbegyx(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	int y, x;
	getbegyx(win, y, x);

	report_count(2);
	report_int(y);
	report_int(x);
}

void
cmd_setsyx(int nargs, char **args)
{
	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	setsyx(y, x);
	report_return(OK);
}

void
cmd_getsyx(int nargs, char **args)
{
	int y, x;

	ARGC(0);

	report_count(3);
	getsyx(y, x);
	report_return(OK);
	report_int(y);
	report_int(x);
}

void
cmd_gettmode(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(gettmode());
}


void
cmd_getwin(int nargs, char **args)
{
	FILE *fp;

	ARGC(1);
	ARG_STRING(filename);

	if ((fp = fopen(filename, "r")) == NULL) {
		report_count(1);
		report_error("BAD FILE_ARGUMENT");
		return;
	}
	report_count(1);
	report_ptr(getwin(fp));
	fclose(fp);
}


void
cmd_halfdelay(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(ms);

	report_count(1);
	report_return(halfdelay(ms));
}


void
cmd_has_colors(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(has_colors());
}


void
cmd_has_ic(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(has_ic());
}


void
cmd_has_il(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(has_il());
}


void
cmd_hline(int nargs, char **args)
{
	ARGC(2);
	ARG_CHTYPE(ch);
	ARG_INT(count);

	report_count(1);
	report_return(hline(ch, count));
}


void
cmd_idcok(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(flag);

	report_count(1);
	report_return(idcok(win, flag));
}


void
cmd_idlok(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(flag);

	report_count(1);
	report_return(idlok(win, flag));
}


void
cmd_init_color(int nargs, char **args)
{
	ARGC(4);
	ARG_SHORT(colour);
	ARG_SHORT(red);
	ARG_SHORT(green);
	ARG_SHORT(blue);

	report_count(1);
	report_return(init_color(colour, red, green, blue));
}


void
cmd_init_pair(int nargs, char **args)
{
	ARGC(3);
	ARG_SHORT(pair);
	ARG_SHORT(fore);
	ARG_SHORT(back);

	report_count(1);
	report_return(init_pair(pair, fore, back));
}


void
cmd_initscr(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_ptr(initscr());
}


void
cmd_intrflush(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(flag);

	report_count(1);
	report_return(intrflush(win, flag));
}


void
cmd_isendwin(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(isendwin());
}


void
cmd_is_linetouched(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(line);

	report_count(1);
	report_int(is_linetouched(win, line));
}


void
cmd_is_wintouched(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(is_wintouched(win));
}


void
cmd_keyok(int nargs, char **args)
{
	ARGC(2);
	ARG_INT(keysym);
	ARG_INT(flag);

	report_count(1);
	report_return(keyok(keysym, flag));
}


void
cmd_keypad(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(flag);

	report_count(1);
	report_return(keypad(win, flag));
}

void
cmd_is_keypad(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(is_keypad(win));
}

void
cmd_keyname(int nargs, char **args)
{
	ARGC(1);
	ARG_UINT(key);

	report_count(1);
	report_status(keyname(key));
}


void
cmd_killchar(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(killchar());
}


void
cmd_leaveok(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(flag);

	report_count(1);
	report_return(leaveok(win, flag));
}

void
cmd_is_leaveok(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(is_leaveok(win));
}

void
cmd_meta(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(flag);

	report_count(1);
	report_return(meta(win, flag));
}


void
cmd_mvcur(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(oldy);
	ARG_INT(oldx);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_return(mvcur(oldy, oldx, y, x));
}


void
cmd_mvderwin(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_return(mvderwin(win, y, x));
}


void
cmd_mvhline(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE(ch);
	ARG_INT(n);

	report_count(1);
	report_return(mvhline(y, x, ch, n));
}


void
cmd_mvprintw(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(fmt);	/* Must have a single "%s" in this test. */
	ARG_STRING(arg);

	report_count(1);
	report_return(mvprintw(y, x, fmt, arg));
}


void
cmd_mvscanw(int nargs, char **args)
{
	int ret;
	char string[256];

	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(fmt);

	report_count(2);
	if (strchr(fmt, 's') != NULL) {
		report_return(ret = mvscanw(y, x, fmt, string));
	} else {
		int val; /* XXX assume 32-bit integer */
		report_return(ret = mvscanw(y, x, fmt, &val));
		if (ret == ERR)
			goto out;
		snprintf(string, sizeof(string), fmt, val);
	}
out:
	/*
	 * When mvscanw(3) fails, string is not modified.
	 * Let's ignore the 2nd result for this case.
	 */
	report_status(ret == ERR ? "ERR" : string);
}


void
cmd_mvvline(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE(ch);
	ARG_INT(n);

	report_count(1);
	report_return(mvvline(y, x, ch, n));
}


void
cmd_mvwhline(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE(ch);
	ARG_INT(n);

	report_count(1);
	report_return(mvwhline(win, y, x, ch, n));
}


void
cmd_mvwvline(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CHTYPE(ch);
	ARG_INT(n);

	report_count(1);
	report_return(mvwvline(win, y, x, ch, n));
}


void
cmd_mvwin(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_return(mvwin(win, y, x));
}


void
cmd_mvwinchnstr(int nargs, char **args)
{
	chtype *string;

	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(count);

	if ((string = malloc((count + 1) * sizeof(chtype))) == NULL) {
		report_count(1);
		report_error("MALLOC_FAILED");
		return;
	}

	report_count(2);
	report_return(mvwinchnstr(win, y, x, string, count));
	report_nstr(string);
	free(string);
}


void
cmd_mvwinchstr(int nargs, char **args)
{
	chtype string[256];

	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvwinchstr(win, y, x, string));
	report_nstr(string);
}


void
cmd_mvwinnstr(int nargs, char **args)
{
	char *string;

	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(count);

	if ((string = malloc(count + 1)) == NULL) {
		report_count(1);
		report_error("MALLOC_FAILED");
		return;
	}

	report_count(2);
	report_int(mvwinnstr(win, y, x, string, count));
	report_status(string);
	free(string);
}


void
cmd_mvwinstr(int nargs, char **args)
{
	char string[256];

	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvwinstr(win, y, x, string));
	report_status(string);
}


void
cmd_mvwprintw(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(fmt);	/* Must have a single "%s" in this test. */
	ARG_STRING(arg);

	report_count(1);
	report_return(mvwprintw(win, y, x, fmt, arg));
}


void
cmd_mvwscanw(int nargs, char **args)
{
	char string[256];

	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(fmt);	/* Must have a single "%s" in this test. */

	report_count(2);
	report_int(mvwscanw(win, y, x, fmt, &string));
	report_status(string);
}


void
cmd_napms(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(naptime);

	report_count(1);
	report_return(napms(naptime));
}


void
cmd_newpad(int nargs, char **args)
{
	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_ptr(newpad(y, x));
}


void
cmd_newterm(int nargs, char **args)
{
	FILE *in, *out;

	ARGC(3);
	ARG_MODIFIABLE_STRING(type);
	ARG_STRING(in_fname);
	ARG_STRING(out_fname);

	if ((in = fopen(in_fname, "rw")) == NULL) {
		report_count(1);
		report_error("BAD FILE_ARGUMENT");
		return;
	}
	if ((out = fopen(out_fname, "rw")) == NULL) {
		report_count(1);
		report_error("BAD FILE_ARGUMENT");
		return;
	}

	report_count(1);
	report_ptr(newterm(type, out, in));
}


void
cmd_newwin(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(lines);
	ARG_INT(cols);
	ARG_INT(begin_y);
	ARG_INT(begin_x);

	report_count(1);
	report_ptr(newwin(lines, cols, begin_y, begin_x));
}


void
cmd_nl(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(nl());
}


void
cmd_no_color_attributes(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(no_color_attributes());
}


void
cmd_nocbreak(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(nocbreak());
}


void
cmd_nodelay(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(flag);

	report_count(1);
	report_return(nodelay(win, flag));
}


void
cmd_noecho(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(noecho());
}


void
cmd_nonl(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(nonl());
}


void
cmd_noqiflush(int nargs, char **args)
{
	ARGC(0);

	noqiflush();
	report_count(1);
	report_return(OK);	/* fake a return, the call returns void */
}


void
cmd_noraw(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(noraw());
}


void
cmd_notimeout(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(flag);

	report_count(1);
	report_return(notimeout(win, flag));
}


void
cmd_overlay(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(source);
	ARG_WINDOW(dest);

	report_count(1);
	report_return(overlay(source, dest));
}


void
cmd_overwrite(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(source);
	ARG_WINDOW(dest);

	report_count(1);
	report_return(overwrite(source, dest));
}


void
cmd_pair_content(int nargs, char **args)
{
	ARGC(1);
	ARG_SHORT(pair);

	short fore, back;
	int ret = pair_content(pair, &fore, &back);

	report_count(3);
	report_return(ret);
	report_int(fore);
	report_int(back);
}


void
cmd_pechochar(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(pad);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(pechochar(pad, ch));
}


void
cmd_pnoutrefresh(int nargs, char **args)
{
	ARGC(7);
	ARG_WINDOW(pad);
	ARG_INT(pbeg_y);
	ARG_INT(pbeg_x);
	ARG_INT(sbeg_y);
	ARG_INT(sbeg_x);
	ARG_INT(smax_y);
	ARG_INT(smax_x);

	report_count(1);
	report_return(pnoutrefresh(pad, pbeg_y, pbeg_x, sbeg_y, sbeg_x, smax_y,
		smax_x));
}


void
cmd_prefresh(int nargs, char **args)
{
	ARGC(7);
	ARG_WINDOW(pad);
	ARG_INT(pbeg_y);
	ARG_INT(pbeg_x);
	ARG_INT(sbeg_y);
	ARG_INT(sbeg_x);
	ARG_INT(smax_y);
	ARG_INT(smax_x);

	/* XXX causes refresh */
	report_count(1);
	report_return(prefresh(pad, pbeg_y, pbeg_x, sbeg_y, sbeg_x, smax_y,
		smax_x));
}


void
cmd_printw(int nargs, char **args)
{
	ARGC(2);
	ARG_STRING(fmt);	/* Must have a single "%s" in this test. */
	ARG_STRING(arg);

	report_count(1);
	report_return(printw(fmt, arg));
}


void
cmd_putwin(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_STRING(filename);

	FILE *fp;
	if ((fp = fopen(filename, "w")) == NULL) {
		report_count(1);
		report_error("BAD FILE_ARGUMENT");
		return;
	}

	report_count(1);
	report_return(putwin(win, fp));
	fclose(fp);
}


void
cmd_qiflush(int nargs, char **args)
{
	ARGC(0);

	qiflush();
	report_count(1);
	report_return(OK);	/* fake a return because call returns void */
}


void
cmd_raw(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(raw());
}


void
cmd_redrawwin(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(redrawwin(win));
}


void
cmd_reset_prog_mode(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(reset_prog_mode());
}


void
cmd_reset_shell_mode(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(reset_shell_mode());
}


void
cmd_resetty(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(resetty());
}


void
cmd_resizeterm(int nargs, char **args)
{
	ARGC(2);
	ARG_INT(rows);
	ARG_INT(cols);

	report_count(1);
	report_return(resizeterm(rows, cols));
}


void
cmd_savetty(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(savetty());
}


void
cmd_scanw(int nargs, char **args)
{
	char string[256];

	ARGC(0);

	report_count(2);
	report_return(scanw("%s", string));
	report_status(string);
}


void
cmd_scroll(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(scroll(win));
}


void
cmd_scrollok(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(flag);

	report_count(1);
	report_return(scrollok(win, flag));
}


void
cmd_setterm(int nargs, char **args)
{
	ARGC(1);
	ARG_MODIFIABLE_STRING(name);

	report_count(1);
	report_return(setterm(name));
}


void
cmd_set_term(int nargs, char **args)
{
	ARGC(1);
	ARG_SCREEN(scrn);

	report_count(1);
	report_ptr(set_term(scrn));
}


void
cmd_start_color(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(start_color());
}


void
cmd_subpad(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(pad);
	ARG_INT(lines);
	ARG_INT(cols);
	ARG_INT(begin_y);
	ARG_INT(begin_x);

	report_count(1);
	report_ptr(subpad(pad, lines, cols, begin_y, begin_x));
}


void
cmd_subwin(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(lines);
	ARG_INT(cols);
	ARG_INT(begin_y);
	ARG_INT(begin_x);

	report_count(1);
	report_ptr(subwin(win, lines, cols, begin_y, begin_x));
}


void
cmd_termattrs(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(termattrs());
}


void
cmd_term_attrs(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_int(term_attrs());
}


void
cmd_touchline(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(start);
	ARG_INT(count);

	report_count(1);
	report_return(touchline(win, start, count));
}


void
cmd_touchoverlap(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win1);
	ARG_WINDOW(win2);

	report_count(1);
	report_return(touchoverlap(win1, win2));
}


void
cmd_touchwin(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(touchwin(win));
}


void
cmd_ungetch(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(ch);

	report_count(1);
	report_return(ungetch(ch));
}


void
cmd_untouchwin(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(untouchwin(win));
}


void
cmd_use_default_colors(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(use_default_colors());
}


void
cmd_vline(int nargs, char **args)
{
	ARGC(2);
	ARG_CHTYPE(ch);
	ARG_INT(count);

	report_count(1);
	report_return(vline(ch, count));
}


static int
internal_vw_printw(WINDOW * win, const char *fmt, ...)
{
	va_list va;
	int rv;

	va_start(va, fmt);
	rv = vw_printw(win, fmt, va);
	va_end(va);

	return rv;
}

void
cmd_vw_printw(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_STRING(fmt);	/* Must have a single "%s" in this test. */
	ARG_STRING(arg);

	report_count(1);
	report_return(internal_vw_printw(win, fmt, arg));
}


static int
internal_vw_scanw(WINDOW * win, const char *fmt, ...)
{
	va_list va;
	int rv;

	va_start(va, fmt);
	rv = vw_scanw(win, fmt, va);
	va_end(va);

	return rv;
}

void
cmd_vw_scanw(int nargs, char **args)
{
	char string[256];

	ARGC(2);
	ARG_WINDOW(win);
	ARG_STRING(fmt);

	report_count(2);
	report_int(internal_vw_scanw(win, fmt, string));
	report_status(string);
}


void
cmd_vwprintw(int nargs, char **args)
{
	cmd_vw_printw(nargs, args);
}


void
cmd_vwscanw(int nargs, char **args)
{
	cmd_vw_scanw(nargs, args);
}


void
cmd_waddch(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(waddch(win, ch));
}


void
cmd_waddchnstr(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_CHTYPE_STRING(chstr);
	ARG_INT(count);

	report_count(1);
	report_return(waddchnstr(win, chstr, count));
}


void
cmd_waddchstr(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_CHTYPE_STRING(chstr);

	report_count(1);
	report_return(waddchstr(win, chstr));
}


void
cmd_waddnstr(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_STRING(str);
	ARG_INT(count);

	report_count(1);
	report_return(waddnstr(win, str, count));

}


void
cmd_wattr_get(int nargs, char **args)
{
	int attr;
	short pair;

	ARGC(1);
	ARG_WINDOW(win);

	report_count(3);
	report_return(wattr_get(win, &attr, &pair, NULL));
	report_int(attr);
	report_int(pair);
}


void
cmd_wattr_off(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(attr);

	report_count(1);
	report_return(wattr_off(win, attr, NULL));
}


void
cmd_wattr_on(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(attr);

	report_count(1);
	report_return(wattr_on(win, attr, NULL));
}


void
cmd_wattr_set(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(attr);
	ARG_SHORT(pair);

	report_count(1);
	report_return(wattr_set(win, attr, pair, NULL));
}


void
cmd_wattroff(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(attr);

	report_count(1);
	report_return(wattroff(win, attr));
}


void
cmd_wattron(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(attr);

	report_count(1);
	report_return(wattron(win, attr));
}


void
cmd_wattrset(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(attr);

	report_count(1);
	report_return(wattrset(win, attr));
}


void
cmd_wbkgd(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(wbkgd(win, ch));
}


void
cmd_wbkgdset(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_CHTYPE(ch);

	wbkgdset(win, ch);	/* void return */
	report_count(1);
	report_return(OK);
}


void
cmd_wborder(int nargs, char **args)
{
	ARGC(9);
	ARG_WINDOW(win);
	ARG_INT(ls);
	ARG_INT(rs);
	ARG_INT(ts);
	ARG_INT(bs);
	ARG_INT(tl);
	ARG_INT(tr);
	ARG_INT(bl);
	ARG_INT(br);

	report_count(1);
	report_return(wborder(win, ls, rs, ts, bs, tl, tr, bl, br));
}


void
cmd_wclear(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(wclear(win));
}


void
cmd_wclrtobot(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(wclrtobot(win));
}


void
cmd_wclrtoeol(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(wclrtoeol(win));

}


void
cmd_wcolor_set(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_SHORT(pair);
	ARG_NULL();

	report_count(1);
	report_return(wcolor_set(win, pair, NULL));
}


void
cmd_wdelch(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(wdelch(win));
}


void
cmd_wdeleteln(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(wdeleteln(win));

}


void
cmd_wechochar(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(wechochar(win, ch));
}


void
cmd_werase(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(werase(win));
}


void
cmd_wgetch(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(wgetch(win));
}


void
cmd_wgetnstr(int nargs, char **args)
{
	char string[256];

	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(count);

	report_count(2);
	report_return(wgetnstr(win, string, count));
	report_status(string);
}


void
cmd_wgetstr(int nargs, char **args)
{
	char string[256];

	ARGC(1);
	ARG_WINDOW(win);

	string[0] = '\0';

	report_count(2);
	report_return(wgetstr(win, string));
	report_status(string);
}


void
cmd_whline(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_CHTYPE(ch);
	ARG_INT(count);

	report_count(1);
	report_return(whline(win, ch, count));
}


void
cmd_winch(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_byte(winch(win));
}


void
cmd_winchnstr(int nargs, char **args)
{
	chtype string[256];

	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(count);

	report_count(2);
	report_return(winchnstr(win, string, count));
	report_nstr(string);
}


void
cmd_winchstr(int nargs, char **args)
{
	chtype string[256];

	ARGC(1);
	ARG_WINDOW(win);

	report_count(2);
	report_return(winchstr(win, string));
	report_nstr(string);
}


void
cmd_winnstr(int nargs, char **args)
{
	char string[256];

	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(count);

	report_count(2);
	report_int(winnstr(win, string, count));
	report_status(string);
}


void
cmd_winsch(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(winsch(win, ch));
}


void
cmd_winsdelln(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(count);

	report_count(1);
	report_return(winsdelln(win, count));
}


void
cmd_winsertln(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(winsertln(win));
}


void
cmd_winstr(int nargs, char **args)
{
	char string[256];

	ARGC(1);
	ARG_WINDOW(win);

	report_count(2);
	report_return(winstr(win, string));
	report_status(string);
}


void
cmd_wmove(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(1);
	report_return(wmove(win, y, x));
}


void
cmd_wnoutrefresh(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_return(wnoutrefresh(win));
}


void
cmd_wprintw(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_STRING(fmt);
	ARG_STRING(arg);

	report_count(1);
	report_return(wprintw(win, fmt, arg));
}


void
cmd_wredrawln(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(beg_line);
	ARG_INT(num_lines);

	report_count(1);
	report_return(wredrawln(win, beg_line, num_lines));
}


void
cmd_wrefresh(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	/* XXX - generates output */
	report_count(1);
	report_return(wrefresh(win));
}


void
cmd_wresize(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(lines);
	ARG_INT(cols);

	report_count(1);
	report_return(wresize(win, lines, cols));
}


void
cmd_wscanw(int nargs, char **args)
{
	char string[256];

	ARGC(2);
	ARG_WINDOW(win);
	ARG_STRING(fmt);

	report_count(1);
	report_return(wscanw(win, fmt, &string));
}


void
cmd_wscrl(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(n);

	report_count(1);
	report_return(wscrl(win, n));
}


void
cmd_wsetscrreg(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(top);
	ARG_INT(bottom);

	report_count(1);
	report_return(wsetscrreg(win, top, bottom));
}


void
cmd_wstandend(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(wstandend(win));
}


void
cmd_wstandout(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(wstandout(win));
}


void
cmd_wtimeout(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(tval);

	wtimeout(win, tval);	/* void return */
	report_count(1);
	report_return(OK);
}


void
cmd_wtouchln(int nargs, char **args)
{
	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(line);
	ARG_INT(n);
	ARG_INT(changed);

	report_count(1);
	report_return(wtouchln(win, line, n, changed));
}


void
cmd_wunderend(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(wunderend(win));
}


void
cmd_wunderscore(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	report_int(wunderscore(win));
}


void
cmd_wvline(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_CHTYPE(ch);
	ARG_INT(n);

	report_count(1);
	report_return(wvline(win, ch, n));
}


void
cmd_insnstr(int nargs, char **args)
{
	ARGC(2);
	ARG_STRING(str);
	ARG_INT(n);

	report_count(1);
	report_return(insnstr(str, n));
}


void
cmd_insstr(int nargs, char **args)
{
	ARGC(1);
	ARG_STRING(str);

	report_count(1);
	report_return(insstr(str));
}


void
cmd_mvinsnstr(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(str);
	ARG_INT(n);

	report_count(1);
	report_return(mvinsnstr(y, x, str, n));
}


void
cmd_mvinsstr(int nargs, char **args)
{
	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(str);

	report_count(1);
	report_return(mvinsstr(y, x, str));
}


void
cmd_mvwinsnstr(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(str);
	ARG_INT(n);

	report_count(1);
	report_return(mvwinsnstr(win, y, x, str, n));

}


void
cmd_mvwinsstr(int nargs, char **args)
{
	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_STRING(str);

	report_count(1);
	report_return(mvwinsstr(win, y, x, str));
}


void
cmd_winsnstr(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_STRING(str);
	ARG_INT(n);

	report_count(1);
	report_return(winsnstr(win, str, n));
}


void
cmd_winsstr(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_STRING(str);

	report_count(1);
	report_return(winsstr(win, str));
}


void
cmd_chgat(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(n);
	ARG_INT(attr);
	ARG_INT(colour);
	ARG_NULL();

	report_count(1);
	report_return(chgat(n, attr, colour, NULL));
}


void
cmd_wchgat(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(n);
	ARG_INT(attr);
	ARG_SHORT(colour);
	ARG_NULL();

	report_count(1);
	report_return(wchgat(win, n, attr, colour, NULL));
}


void
cmd_mvchgat(int nargs, char **args)
{
	ARGC(6);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(n);
	ARG_INT(attr);
	ARG_SHORT(colour);
	ARG_NULL();

	report_count(1);
	report_return(mvchgat(y, x, n, attr, colour, NULL));
}


void
cmd_mvwchgat(int nargs, char **args)
{
	ARGC(7);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(n);
	ARG_INT(attr);
	ARG_SHORT(colour);
	ARG_NULL();

	report_count(1);
	report_return(mvwchgat(win, y, x, n, attr, colour, NULL));
}


void
cmd_add_wch(int nargs, char **args)
{
	ARGC(1);
	ARG_CCHAR_STRING(ch);

	report_count(1);
	report_return(add_wch(ch));
}


void
cmd_wadd_wch(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_CCHAR_STRING(ch);

	report_count(1);
	report_return(wadd_wch(win, ch));
}


void
cmd_mvadd_wch(int nargs, char **args)
{
	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CCHAR_STRING(ch);

	report_count(1);
	report_return(mvadd_wch(y, x, ch));
}


void
cmd_mvwadd_wch(int nargs, char **args)
{
	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CCHAR_STRING(ch);

	report_count(1);
	report_return(mvwadd_wch(win, y, x, ch));
}


void
cmd_add_wchnstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_add_wchstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_wadd_wchnstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_wadd_wchstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_mvadd_wchnstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_mvadd_wchstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_mvwadd_wchnstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_mvwadd_wchstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_addnwstr(int nargs, char **args)
{
	ARGC(2);
	ARG_WCHAR_STRING(wstr);
	ARG_INT(n);

	report_count(1);
	report_return(addnwstr(wstr, n));
}


void
cmd_addwstr(int nargs, char **args)
{
	ARGC(1);
	ARG_WCHAR_STRING(wstr);

	report_count(1);
	report_return(addwstr(wstr));
}


void
cmd_mvaddnwstr(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(y);
	ARG_INT(x);
	ARG_WCHAR_STRING(wstr);
	ARG_INT(n);

	report_count(1);
	report_return(mvaddnwstr(y, x, wstr, n));
}


void
cmd_mvaddwstr(int nargs, char **args)
{
	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_WCHAR_STRING(wstr);

	report_count(1);
	report_return(mvaddwstr(y, x, wstr));
}


void
cmd_mvwaddnwstr(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_WCHAR_STRING(wstr);
	ARG_INT(n);

	report_count(1);
	report_return(mvwaddnwstr(win, y, x, wstr, n));
}


void
cmd_mvwaddwstr(int nargs, char **args)
{
	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_WCHAR_STRING(wstr);

	report_count(1);
	report_return(mvwaddwstr(win, y, x, wstr));
}


void
cmd_waddnwstr(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_WCHAR_STRING(wstr);
	ARG_INT(n);

	report_count(1);
	report_return(waddnwstr(win, wstr, n));
}


void
cmd_waddwstr(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_WCHAR_STRING(wstr);

	report_count(1);
	report_return(waddwstr(win, wstr));
}


void
cmd_echo_wchar(int nargs, char **args)
{
	ARGC(1);
	ARG_CCHAR_STRING(ch);

	report_count(1);
	report_return(echo_wchar(ch));
}


void
cmd_wecho_wchar(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_CCHAR_STRING(ch);

	report_count(1);
	report_return(wecho_wchar(win, ch));
}


void
cmd_pecho_wchar(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(pad);
	ARG_CCHAR_STRING(wch);

	report_count(1);
	report_return(pecho_wchar(pad, wch));
}


/* insert */
void
cmd_ins_wch(int nargs, char **args)
{
	ARGC(1);
	ARG_CCHAR_STRING(wch);

	report_count(1);
	report_return(ins_wch(wch));
}


void
cmd_wins_wch(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_CCHAR_STRING(wch);

	report_count(1);
	report_return(wins_wch(win, wch));
}


void
cmd_mvins_wch(int nargs, char **args)
{
	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CCHAR_STRING(wch);

	report_count(1);
	report_return(mvins_wch(y, x, wch));
}


void
cmd_mvwins_wch(int nargs, char **args)
{
	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CCHAR_STRING(wch);

	report_count(1);
	report_return(mvwins_wch(win, y, x, wch));
}


void
cmd_ins_nwstr(int nargs, char **args)
{
	ARGC(2);
	ARG_WCHAR_STRING(wstr);
	ARG_INT(n);

	report_count(1);
	report_return(ins_nwstr(wstr, n));
}


void
cmd_ins_wstr(int nargs, char **args)
{
	ARGC(1);
	ARG_WCHAR_STRING(wstr);

	report_count(1);
	report_return(ins_wstr(wstr));
}


void
cmd_mvins_nwstr(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(y);
	ARG_INT(x);
	ARG_WCHAR_STRING(wstr);
	ARG_INT(n);

	report_count(1);
	report_return(mvins_nwstr(y, x, wstr, n));
}


void
cmd_mvins_wstr(int nargs, char **args)
{
	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_WCHAR_STRING(wstr);

	report_count(1);
	report_return(mvins_wstr(y, x, wstr));
}


void
cmd_mvwins_nwstr(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_WCHAR_STRING(wstr);
	ARG_INT(n);

	report_count(1);
	report_return(mvwins_nwstr(win, y, x, wstr, n));
}


void
cmd_mvwins_wstr(int nargs, char **args)
{
	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_WCHAR_STRING(wstr);

	report_count(1);
	report_return(mvwins_wstr(win, y, x, wstr));
}


void
cmd_wins_nwstr(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_WCHAR_STRING(wstr);
	ARG_INT(n);

	report_count(1);
	report_return(wins_nwstr(win, wstr, n));
}


void
cmd_wins_wstr(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_WCHAR_STRING(wstr);

	report_count(1);
	report_return(wins_wstr(win, wstr));
}


/* input */
void
cmd_get_wch(int nargs, char **args)
{
	wchar_t ch;
	ARGC(0);

	report_count(2);
	report_return(get_wch(&ch));
	report_wchar(ch);
}


void
cmd_unget_wch(int nargs, char **args)
{
	ARGC(1);
	ARG_WCHAR(wch);

	report_count(1);
	report_return(unget_wch(wch));
}


void
cmd_mvget_wch(int nargs, char **args)
{
	wchar_t ch;

	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvget_wch(y, x, &ch));
	report_wchar(ch);
}


void
cmd_mvwget_wch(int nargs, char **args)
{
	wchar_t ch;

	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvwget_wch(win, y, x, &ch));
	report_wchar(ch);
}


void
cmd_wget_wch(int nargs, char **args)
{
	wchar_t ch;

	ARGC(1);
	ARG_WINDOW(win);

	report_count(2);
	report_return(wget_wch(win, &ch));
	report_wchar(ch);
}


void
cmd_getn_wstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(1);
	ARG_INT(n);

	report_count(2);
	report_return(getn_wstr(wstr, n));
	report_wstr(wstr);
}


void
cmd_get_wstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(0);

	report_count(2);
	report_return(get_wstr(wstr));
	report_wstr(wstr);
}

void
cmd_mvgetn_wstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(n);

	report_count(2);
	report_return(mvgetn_wstr(y, x, wstr, n));
	report_wstr(wstr);
}

void
cmd_mvget_wstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvget_wstr(y, x, wstr));
	report_wstr(wstr);
}


void
cmd_mvwgetn_wstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(n);

	report_count(2);
	report_return(mvwgetn_wstr(win, y, x, wstr, n));
	report_wstr(wstr);
}


void
cmd_mvwget_wstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvwget_wstr(win, y, x, wstr));
	report_wstr(wstr);
}


void
cmd_wgetn_wstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(n);

	report_count(2);
	report_return(wgetn_wstr(win, wstr, n));
	report_wstr(wstr);
}


void
cmd_wget_wstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(1);
	ARG_WINDOW(win);

	report_count(2);
	report_return(wget_wstr(win, wstr));
	report_wstr(wstr);
}


void
cmd_in_wch(int nargs, char **args)
{
	cchar_t wcval;
	ARGC(0);

	report_count(2);
	report_return(in_wch(&wcval));
	report_cchar(wcval);
}


void
cmd_mvin_wch(int nargs, char **args)
{
	cchar_t wcval;

	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvin_wch(y, x, &wcval));
	report_cchar(wcval);
}


void
cmd_mvwin_wch(int nargs, char **args)
{
	cchar_t wcval;

	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvwin_wch(win, y, x, &wcval));
	report_cchar(wcval);
}


void
cmd_win_wch(int nargs, char **args)
{
	cchar_t wcval;

	ARGC(1);
	ARG_WINDOW(win);

	report_count(2);
	report_return(win_wch(win, &wcval));
	report_cchar(wcval);
}


void
cmd_in_wchnstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_in_wchstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_mvin_wchnstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_mvin_wchstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_mvwin_wchnstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_mvwin_wchstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_win_wchnstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_win_wchstr(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}


void
cmd_innwstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(1);
	ARG_INT(n);

	report_count(2);
	report_int(innwstr(wstr, n));
	report_wstr(wstr);
}


void
cmd_inwstr(int nargs, char **args)
{
	wchar_t wstr[256];
	ARGC(0);

	report_count(2);
	report_return(inwstr(wstr));
	report_wstr(wstr);
}


void
cmd_mvinnwstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(3);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(n);

	report_count(2);
	report_int(mvinnwstr(y, x, wstr, n));
	report_wstr(wstr);
}


void
cmd_mvinwstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(2);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvinwstr(y, x, wstr));
	report_wstr(wstr);
}


void
cmd_mvwinnwstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(4);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_INT(n);

	report_count(2);
	report_int(mvwinnwstr(win, y, x, wstr, n));
	report_wstr(wstr);
}


void
cmd_mvwinwstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(3);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);

	report_count(2);
	report_return(mvwinwstr(win, y, x, wstr));
	report_wstr(wstr);
}


void
cmd_winnwstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(n);

	report_count(2);
	report_int(winnwstr(win, wstr, n));
	report_wstr(wstr);
}


void
cmd_winwstr(int nargs, char **args)
{
	wchar_t wstr[256];

	ARGC(1);
	ARG_WINDOW(win);

	report_count(2);
	report_return(winwstr(win, wstr));
	report_wstr(wstr);
}


/* cchar handling */
void
cmd_setcchar(int nargs, char **args)
{
	cchar_t wcval;

	ARGC(4);
	ARG_WCHAR_STRING(wch);
	ARG_INT(attrs);
	ARG_SHORT(color_pair);
	ARG_NULL();

	report_count(2);
	report_return(setcchar(&wcval, wch, attrs, color_pair, NULL));
	report_cchar(wcval);
}


void
cmd_getcchar(int nargs, char **args)
{
	wchar_t wch[256];
	attr_t attrs;
	short color_pair;

	/*
         * XXX - not handling passing of wch as NULL
         */

	ARGC(2);
	ARG_CCHAR_STRING(wcval);
	ARG_NULL();

	report_count(4);
	report_return(getcchar(wcval, wch, &attrs, &color_pair, NULL));
	report_wstr(wch);
	report_int(attrs);
	report_int(color_pair);
}


/* misc */
void
cmd_key_name(int nargs, char **args)
{
	ARGC(1);
	ARG_WCHAR(w);

	report_count(1);
	report_status(key_name(w));
}


void
cmd_border_set(int nargs, char **args)
{
	ARGC(8);
	ARG_CCHAR_STRING(ls);
	ARG_CCHAR_STRING(rs);
	ARG_CCHAR_STRING(ts);
	ARG_CCHAR_STRING(bs);
	ARG_CCHAR_STRING(tl);
	ARG_CCHAR_STRING(tr);
	ARG_CCHAR_STRING(bl);
	ARG_CCHAR_STRING(br);

	report_count(1);
	report_return(border_set(ls, rs, ts, bs, tl, tr, bl, br));
}


void
cmd_wborder_set(int nargs, char **args)
{
	ARGC(9);
	ARG_WINDOW(win);
	ARG_CCHAR_STRING(ls);
	ARG_CCHAR_STRING(rs);
	ARG_CCHAR_STRING(ts);
	ARG_CCHAR_STRING(bs);
	ARG_CCHAR_STRING(tl);
	ARG_CCHAR_STRING(tr);
	ARG_CCHAR_STRING(bl);
	ARG_CCHAR_STRING(br);

	report_count(1);
	report_return(wborder_set(win, ls, rs, ts, bs, tl, tr, bl, br));
}


void
cmd_box_set(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_CCHAR_STRING(verch);
	ARG_CCHAR_STRING(horch);

	report_count(1);
	report_return(box_set(win, verch, horch));
}


void
cmd_erasewchar(int nargs, char **args)
{
	wchar_t ch;

	ARGC(0);

	report_count(2);
	report_return(erasewchar(&ch));
	report_wchar(ch);
}


void
cmd_killwchar(int nargs, char **args)
{
	wchar_t ch;

	ARGC(0);

	report_count(2);
	report_return(killwchar(&ch));
	report_wchar(ch);
}


void
cmd_hline_set(int nargs, char **args)
{
	ARGC(2);
	ARG_CCHAR_STRING(wch);
	ARG_INT(n);

	report_count(1);
	report_return(hline_set(wch, n));
}


void
cmd_mvhline_set(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CCHAR_STRING(wch);
	ARG_INT(n);

	report_count(1);
	report_return(mvhline_set(y, x, wch, n));
}


void
cmd_mvvline_set(int nargs, char **args)
{
	ARGC(4);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CCHAR_STRING(wch);
	ARG_INT(n);

	report_count(1);
	report_return(mvvline_set(y, x, wch, n));
}


void
cmd_mvwhline_set(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CCHAR_STRING(wch);
	ARG_INT(n);

	report_count(1);
	report_return(mvwhline_set(win, y, x, wch, n));
}


void
cmd_mvwvline_set(int nargs, char **args)
{
	ARGC(5);
	ARG_WINDOW(win);
	ARG_INT(y);
	ARG_INT(x);
	ARG_CCHAR_STRING(wch);
	ARG_INT(n);

	report_count(1);
	report_return(mvwvline_set(win, y, x, wch, n));
}


void
cmd_vline_set(int nargs, char **args)
{
	ARGC(2);
	ARG_CCHAR_STRING(wch);
	ARG_INT(n);

	report_count(1);
	report_return(vline_set(wch, n));
}


void
cmd_whline_set(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_CCHAR_STRING(wch);
	ARG_INT(n);

	report_count(1);
	report_return(whline_set(win, wch, n));
}


void
cmd_wvline_set(int nargs, char **args)
{
	ARGC(3);
	ARG_WINDOW(win);
	ARG_CCHAR_STRING(wch);
	ARG_INT(n);

	report_count(1);
	report_return(wvline_set(win, wch, n));
}


void
cmd_bkgrnd(int nargs, char **args)
{
	ARGC(1);
	ARG_CCHAR_STRING(wch);

	report_count(1);
	report_return(bkgrnd(wch));
}


void
cmd_bkgrndset(int nargs, char **args)
{
	ARGC(1);
	ARG_CCHAR_STRING(wch);

	report_count(1);
	bkgrndset(wch);
	report_return(OK);
}


void
cmd_getbkgrnd(int nargs, char **args)
{
	cchar_t wch;
	ARGC(0);

	report_count(2);
	report_return(getbkgrnd(&wch));
	report_cchar(wch);
}


void
cmd_wbkgrnd(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_CCHAR_STRING(wch);

	report_count(1);
	report_return(wbkgrnd(win, wch));
}


void
cmd_wbkgrndset(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_CCHAR_STRING(wch);

	report_count(1);
	wbkgrndset(win, wch);
	report_return(OK);
}


void
cmd_wgetbkgrnd(int nargs, char **args)
{
	cchar_t wch;
	ARGC(1);
	ARG_WINDOW(win);

	report_count(2);
	report_return(wgetbkgrnd(win, &wch));
	report_cchar(wch);
}


void
cmd_immedok(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(bf);

	report_count(1);
	immedok(win, bf);
	report_return(OK);
}

void
cmd_syncok(int nargs, char **args)
{
	ARGC(2);
	ARG_WINDOW(win);
	ARG_INT(bf);

	report_count(1);
	report_return(syncok(win, bf));
}

void
cmd_wcursyncup(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	wcursyncup(win);
	report_return(OK);
}

void
cmd_wsyncup(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	wsyncup(win);
	report_return(OK);
}

void
cmd_wsyncdown(int nargs, char **args)
{
	ARGC(1);
	ARG_WINDOW(win);

	report_count(1);
	wsyncdown(win);
	report_return(OK);
}


/* Soft label key routines */
void
cmd_slk_attroff(int nargs, char **args)
{
	ARGC(1);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(slk_attroff(ch));
}

void
cmd_slk_attr_off(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(attrs);

	report_count(1);
	report_return(slk_attr_off(attrs, NULL));
}

void
cmd_slk_attron(int nargs, char **args)
{
	ARGC(1);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(slk_attron(ch));
}

void
cmd_slk_attr_on(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(attrs);

	report_count(1);
	report_return(slk_attr_on(attrs, NULL));
}

void
cmd_slk_attrset(int nargs, char **args)
{
	ARGC(1);
	ARG_CHTYPE(ch);

	report_count(1);
	report_return(slk_attrset(ch));
}

void
cmd_slk_attr_set(int nargs, char **args)
{
	ARGC(2);
	ARG_INT(attrs);
	ARG_SHORT(color_pair_number);

	report_count(1);
	report_return(slk_attr_set(attrs, color_pair_number, NULL));
}

void
cmd_slk_clear(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(slk_clear());
}

void
cmd_slk_color(int nargs, char **args)
{
	ARGC(1);
	ARG_SHORT(color_pair_number);

	report_count(1);
	report_return(slk_color(color_pair_number));
}

void
cmd_slk_label(int nargs, char **args)
{
	char *label;

	ARGC(1);
	ARG_INT(labnum);

	label = slk_label(labnum);
	report_count(1);
	if (label == NULL)
		report_status("NULL");
	else
		report_status(label);
}

void
cmd_slk_noutrefresh(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(slk_noutrefresh());
}

void
cmd_slk_refresh(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(slk_refresh());
}

void
cmd_slk_restore(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(slk_restore());
}

void
cmd_slk_set(int nargs, char **args)
{
	ARGC(3);
	ARG_INT(labnum);
	ARG_STRING(label);
	ARG_INT(justify);

	report_count(1);
	report_return(slk_set(labnum, label, justify));
}

void
cmd_slk_touch(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	report_return(slk_touch());
}

void
cmd_slk_wset(int nargs, char **args)
{
	ARGC(3);
	ARG_INT(labnum);
	ARG_WCHAR_STRING(label);
	ARG_INT(justify);

	report_count(1);
	report_return(slk_wset(labnum, label, justify));
}


void
cmd_slk_init(int nargs, char **args)
{
	ARGC(1);
	ARG_INT(fmt);

	report_count(1);
	report_return(slk_init(fmt));
}

void
cmd_use_env(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}

void
cmd_ripoffline(int nargs, char **args)
{
	ARGC(1);
	ARG_IGNORE();

	report_count(1);
	report_error("UNSUPPORTED");
}

void
cmd_filter(int nargs, char **args)
{
	ARGC(0);

	report_count(1);
	filter();
	report_return(OK);
}
