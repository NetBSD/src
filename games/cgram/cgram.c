/* $NetBSD: cgram.c,v 1.27 2022/03/28 20:00:29 rillig Exp $ */

/*-
 * Copyright (c) 2013, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland Illig.
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
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: cgram.c,v 1.27 2022/03/28 20:00:29 rillig Exp $");
#endif

#include <assert.h>
#include <ctype.h>
#include <curses.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pathnames.h"


static bool
ch_islower(char ch)
{
	return ch >= 'a' && ch <= 'z';
}

static bool
ch_isupper(char ch)
{
	return ch >= 'A' && ch <= 'Z';
}

static bool
ch_isalpha(char ch)
{
	return ch_islower(ch) || ch_isupper(ch);
}

static char
ch_toupper(char ch)
{
	return ch_islower(ch) ? (char)(ch - 'a' + 'A') : ch;
}

static char
ch_tolower(char ch)
{
	return ch_isupper(ch) ? (char)(ch - 'A' + 'a') : ch;
}

static int
imax(int a, int b)
{
	return a > b ? a : b;
}

static int
imin(int a, int b)
{
	return a < b ? a : b;
}

////////////////////////////////////////////////////////////

struct string {
	char *s;
	size_t len;
	size_t cap;
};

struct stringarray {
	struct string *v;
	size_t num;
};

static void
string_init(struct string *s)
{
	s->s = NULL;
	s->len = 0;
	s->cap = 0;
}

static void
string_add(struct string *s, char ch)
{
	if (s->len >= s->cap) {
		s->cap = 2 * s->cap + 16;
		s->s = realloc(s->s, s->cap);
		if (s->s == NULL)
			errx(1, "Out of memory");
	}
	s->s[s->len++] = ch;
}

static void
string_finish(struct string *s)
{
	string_add(s, '\0');
	s->len--;
}

static void
stringarray_init(struct stringarray *a)
{
	a->v = NULL;
	a->num = 0;
}

static void
stringarray_done(struct stringarray *a)
{
	for (size_t i = 0; i < a->num; i++)
		free(a->v[i].s);
	free(a->v);
}

static void
stringarray_add(struct stringarray *a, struct string *s)
{
	size_t num = a->num++;
	if (reallocarr(&a->v, a->num, sizeof(a->v[0])) != 0)
		errx(1, "Out of memory");
	a->v[num] = *s;
}

static void
stringarray_dup(struct stringarray *dst, const struct stringarray *src)
{
	assert(dst->num == 0);
	for (size_t i = 0; i < src->num; i++) {
		struct string str;
		string_init(&str);
		for (const char *p = src->v[i].s; *p != '\0'; p++)
			string_add(&str, *p);
		string_finish(&str);
		stringarray_add(dst, &str);
	}
}

////////////////////////////////////////////////////////////

static struct stringarray lines;
static struct stringarray sollines;
static bool hinting;
static int extent_x;
static int extent_y;
static int offset_x;
static int offset_y;
static int cursor_x;
static int cursor_y;

static int
cur_max_x(void)
{
	return (int)lines.v[cursor_y].len;
}

static int
cur_max_y(void)
{
	return extent_y - 1;
}

static char
char_left_of_cursor(void)
{
	if (cursor_x > 0)
		return lines.v[cursor_y].s[cursor_x - 1];
	assert(cursor_y > 0);
	return '\n'; /* eol of previous line */
}

static char
char_at_cursor(void)
{
	if (cursor_x == cur_max_x())
		return '\n';
	return lines.v[cursor_y].s[cursor_x];
}

static void
getquote(FILE *f)
{
	struct string line;
	string_init(&line);

	int ch;
	while ((ch = fgetc(f)) != EOF) {
		if (ch == '\n') {
			string_finish(&line);
			stringarray_add(&lines, &line);
			string_init(&line);
		} else if (ch == '\t') {
			string_add(&line, ' ');
			while (line.len % 8 != 0)
				string_add(&line, ' ');
		} else if (ch == '\b') {
			if (line.len > 0)
				line.len--;
		} else {
			string_add(&line, (char)ch);
		}
	}

	stringarray_dup(&sollines, &lines);

	extent_y = (int)lines.num;
	for (int i = 0; i < extent_y; i++)
		extent_x = imax(extent_x, (int)lines.v[i].len);
}

static void
readfile(const char *name)
{
	FILE *f = fopen(name, "r");
	if (f == NULL)
		err(1, "%s", name);

	getquote(f);

	if (fclose(f) != 0)
		err(1, "%s", name);
}


static void
readquote(void)
{
	FILE *f = popen(_PATH_FORTUNE, "r");
	if (f == NULL)
		err(1, "%s", _PATH_FORTUNE);

	getquote(f);

	if (pclose(f) != 0)
		exit(1); /* error message must come from child process */
}

static void
encode(void)
{
	int key[26];

	for (int i = 0; i < 26; i++)
		key[i] = i;

	for (int i = 26; i > 1; i--) {
		int c = (int)(random() % i);
		int t = key[i - 1];
		key[i - 1] = key[c];
		key[c] = t;
	}

	for (int y = 0; y < extent_y; y++) {
		for (char *p = lines.v[y].s; *p != '\0'; p++) {
			if (ch_islower(*p))
				*p = (char)('a' + key[*p - 'a']);
			if (ch_isupper(*p))
				*p = (char)('A' + key[*p - 'A']);
		}
	}
}

static void
substitute(char a, char b)
{
	char la = ch_tolower(a);
	char ua = ch_toupper(a);
	char lb = ch_tolower(b);
	char ub = ch_toupper(b);

	for (int y = 0; y < (int)lines.num; y++) {
		for (char *p = lines.v[y].s; *p != '\0'; p++) {
			if (*p == la)
				*p = lb;
			else if (*p == ua)
				*p = ub;
			else if (*p == lb)
				*p = la;
			else if (*p == ub)
				*p = ua;
		}
	}
}

static bool
is_solved(void)
{
	for (size_t i = 0; i < lines.num; i++)
		if (strcmp(lines.v[i].s, sollines.v[i].s) != 0)
			return false;
	return true;
}

////////////////////////////////////////////////////////////

static void
redraw(void)
{
	erase();

	int max_y = imin(LINES - 1, extent_y - offset_y);
	for (int y = 0; y < max_y; y++) {
		move(y, 0);

		int len = (int)lines.v[offset_y + y].len;
		int max_x = imin(COLS - 1, len - offset_x);
		const char *line = lines.v[offset_y + y].s;
		const char *solline = sollines.v[offset_y + y].s;

		for (int x = 0; x < max_x; x++) {
			char ch = line[offset_x + x];
			bool bold = hinting &&
			    (ch == solline[offset_x + x] || !ch_isalpha(ch));

			if (bold)
				attron(A_BOLD);
			addch(ch);
			if (bold)
				attroff(A_BOLD);
		}
		clrtoeol();
	}

	move(LINES - 1, 0);
	addstr("~ to quit, * to cheat, ^pnfb to move");

	if (is_solved()) {
		if (extent_y + 1 - offset_y < LINES - 2)
			move(extent_y + 1 - offset_y, 0);
		else
			addch(' ');
		attron(A_BOLD | A_STANDOUT);
		addstr("*solved*");
		attroff(A_BOLD | A_STANDOUT);
	}

	move(cursor_y - offset_y, cursor_x - offset_x);

	refresh();
}

////////////////////////////////////////////////////////////

static void
saturate_cursor(void)
{
	cursor_y = imax(cursor_y, 0);
	cursor_y = imin(cursor_y, cur_max_y());

	assert(cursor_x >= 0);
	cursor_x = imin(cursor_x, cur_max_x());
}

static void
scroll_into_view(void)
{
	if (cursor_x < offset_x)
		offset_x = cursor_x;
	if (cursor_x > offset_x + COLS - 1)
		offset_x = cursor_x - (COLS - 1);

	if (cursor_y < offset_y)
		offset_y = cursor_y;
	if (cursor_y > offset_y + LINES - 2)
		offset_y = cursor_y - (LINES - 2);
}

static bool
can_go_left(void)
{
	return cursor_y > 0 ||
	    (cursor_y == 0 && cursor_x > 0);
}

static bool
can_go_right(void)
{
	return cursor_y < cur_max_y() ||
	    (cursor_y == cur_max_y() && cursor_x < cur_max_x());
}

static void
go_to_prev_line(void)
{
	cursor_y--;
	cursor_x = cur_max_x();
}

static void
go_to_next_line(void)
{
	cursor_x = 0;
	cursor_y++;
}

static void
go_left(void)
{
	if (cursor_x > 0)
		cursor_x--;
	else if (cursor_y > 0)
		go_to_prev_line();
}

static void
go_right(void)
{
	if (cursor_x < cur_max_x())
		cursor_x++;
	else if (cursor_y < cur_max_y())
		go_to_next_line();
}

static void
go_to_prev_word(void)
{
	while (can_go_left() && !ch_isalpha(char_left_of_cursor()))
		go_left();

	while (can_go_left() && ch_isalpha(char_left_of_cursor()))
		go_left();
}

static void
go_to_next_word(void)
{
	while (can_go_right() && ch_isalpha(char_at_cursor()))
		go_right();

	while (can_go_right() && !ch_isalpha(char_at_cursor()))
		go_right();
}

static bool
can_substitute_here(int ch)
{
	return isascii(ch) &&
	    ch_isalpha((char)ch) &&
	    cursor_x < cur_max_x() &&
	    ch_isalpha(char_at_cursor());
}

static void
handle_char_input(int ch)
{
	if (ch == char_at_cursor())
		go_right();
	else if (can_substitute_here(ch)) {
		substitute(char_at_cursor(), (char)ch);
		go_right();
	} else
		beep();
}

static bool
handle_key(void)
{
	int ch = getch();

	switch (ch) {
	case 1:			/* ^A */
	case KEY_HOME:
		cursor_x = 0;
		break;
	case 2:			/* ^B */
	case KEY_LEFT:
		go_left();
		break;
	case 5:			/* ^E */
	case KEY_END:
		cursor_x = cur_max_x();
		break;
	case 6:			/* ^F */
	case KEY_RIGHT:
		go_right();
		break;
	case '\t':
		go_to_next_word();
		break;
	case KEY_BTAB:
		go_to_prev_word();
		break;
	case '\n':
		go_to_next_line();
		break;
	case 12:		/* ^L */
		clear();
		break;
	case 14:		/* ^N */
	case KEY_DOWN:
		cursor_y++;
		break;
	case 16:		/* ^P */
	case KEY_UP:
		cursor_y--;
		break;
	case KEY_PPAGE:
		cursor_y -= LINES - 2;
		break;
	case KEY_NPAGE:
		cursor_y += LINES - 2;
		break;
	case '*':
		hinting = !hinting;
		break;
	case '~':
		return false;
	case KEY_RESIZE:
		break;
	default:
		handle_char_input(ch);
		break;
	}
	return true;
}

static void
init(const char *filename)
{
	stringarray_init(&lines);
	stringarray_init(&sollines);
	srandom((unsigned int)time(NULL));
	if (filename != NULL) {
	    readfile(filename);
	} else {
	    readquote();
	}
	encode();

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, true);
}

static void
loop(void)
{
	for (;;) {
		redraw();
		if (!handle_key())
			break;
		saturate_cursor();
		scroll_into_view();
	}
}

static void
done(void)
{
	endwin();

	stringarray_done(&sollines);
	stringarray_done(&lines);
}


static void __dead
usage(void)
{

	fprintf(stderr, "usage: %s [file]\n", getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{

	setprogname(argv[0]);
	if (argc != 1 && argc != 2)
		usage();

	init(argc > 1 ? argv[1] : NULL);
	loop();
	done();
	return 0;
}
