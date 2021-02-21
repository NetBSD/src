/* $NetBSD: cgram.c,v 1.9 2021/02/21 17:16:00 rillig Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David A. Holland.
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

////////////////////////////////////////////////////////////

static char *
xstrdup(const char *s)
{
	char *ret;

	ret = malloc(strlen(s) + 1);
	if (ret == NULL) {
		errx(1, "Out of memory");
	}
	strcpy(ret, s);
	return ret;
}

static char
ch_toupper(char ch)
{
	return (char)toupper((unsigned char)ch);
}

static char
ch_tolower(char ch)
{
	return (char)tolower((unsigned char)ch);
}

static bool
ch_isalpha(char ch)
{
	return isalpha((unsigned char)ch);
}

static bool
ch_islower(char ch)
{
	return islower((unsigned char)ch);
}

static bool
ch_isupper(char ch)
{
	return isupper((unsigned char)ch);
}

////////////////////////////////////////////////////////////

struct stringarray {
	char **v;
	size_t num;
};

static void
stringarray_init(struct stringarray *a)
{
	a->v = NULL;
	a->num = 0;
}

static void
stringarray_cleanup(struct stringarray *a)
{
	free(a->v);
}

static void
stringarray_add(struct stringarray *a, const char *s)
{
	a->v = realloc(a->v, (a->num + 1) * sizeof(a->v[0]));
	if (a->v == NULL) {
		errx(1, "Out of memory");
	}
	a->v[a->num] = xstrdup(s);
	a->num++;
}

////////////////////////////////////////////////////////////

static struct stringarray lines;
static struct stringarray sollines;
static bool hinting;
static int scrolldown;
static int curx;
static int cury;

static void
readquote(void)
{
	FILE *f = popen(_PATH_FORTUNE, "r");
	if (f == NULL) {
		err(1, "%s", _PATH_FORTUNE);
	}

	char buf[128], buf2[8 * sizeof(buf)];
	while (fgets(buf, sizeof buf, f) != NULL) {
		char *s = strrchr(buf, '\n');
		assert(s != NULL);
		assert(strlen(s) == 1);
		*s = '\0';

		int i, j;
		for (i = j = 0; buf[i] != '\0'; i++) {
			if (buf[i] == '\t') {
				buf2[j++] = ' ';
				while (j % 8 != 0)
					buf2[j++] = ' ';
			} else if (buf[i] == '\b') {
				if (j > 0)
					j--;
			} else {
				buf2[j++] = buf[i];
			}
		}
		buf2[j] = '\0';

		stringarray_add(&lines, buf2);
		stringarray_add(&sollines, buf2);
	}

	pclose(f);
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

	for (int y = 0; y < (int)lines.num; y++) {
		for (char *p = lines.v[y]; *p != '\0'; p++) {
			if (ch_islower(*p))
				*p = (char)('a' + key[*p - 'a']);
			if (ch_isupper(*p))
				*p = (char)('A' + key[*p - 'A']);
		}
	}
}

static bool
substitute(char ch)
{
	assert(cury >= 0 && cury < (int)lines.num);
	if (curx >= (int)strlen(lines.v[cury])) {
		beep();
		return false;
	}

	char och = lines.v[cury][curx];
	if (!ch_isalpha(och)) {
		beep();
		return false;
	}

	char loch = ch_tolower(och);
	char uoch = ch_toupper(och);
	char lch = ch_tolower(ch);
	char uch = ch_toupper(ch);

	for (int y = 0; y < (int)lines.num; y++) {
		for (char *p = lines.v[y]; *p != '\0'; p++) {
			if (*p == loch)
				*p = lch;
			else if (*p == uoch)
				*p = uch;
			else if (*p == lch)
				*p = loch;
			else if (*p == uch)
				*p = uoch;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////

static void
redraw(void)
{
	erase();
	bool won = true;
	for (int i = 0; i < LINES - 1; i++) {
		move(i, 0);
		int ln = i + scrolldown;
		if (ln < (int)lines.num) {
			for (unsigned j = 0; lines.v[i][j] != '\0'; j++) {
				char ch = lines.v[i][j];
				if (ch != sollines.v[i][j] && ch_isalpha(ch)) {
					won = false;
				}
				bool bold = false;
				if (hinting && ch == sollines.v[i][j] &&
				    ch_isalpha(ch)) {
					bold = true;
					attron(A_BOLD);
				}
				addch(lines.v[i][j]);
				if (bold) {
					attroff(A_BOLD);
				}
			}
		}
		clrtoeol();
	}

	move(LINES - 1, 0);
	if (won) {
		addstr("*solved* ");
	}
	addstr("~ to quit, * to cheat, ^pnfb to move");

	move(LINES - 1, 0);

	move(cury - scrolldown, curx);

	refresh();
}

static void
opencurses(void)
{
	initscr();
	cbreak();
	noecho();
}

static void
closecurses(void)
{
	endwin();
}

////////////////////////////////////////////////////////////

static void
loop(void)
{
	bool done = false;
	while (!done) {
		redraw();
		int ch = getch();
		switch (ch) {
		case 1:		/* ^A */
		case KEY_HOME:
			curx = 0;
			break;
		case 2:		/* ^B */
		case KEY_LEFT:
			if (curx > 0) {
				curx--;
			} else if (cury > 0) {
				cury--;
				curx = (int)strlen(lines.v[cury]);
			}
			break;
		case 5:		/* ^E */
		case KEY_END:
			curx = (int)strlen(lines.v[cury]);
			break;
		case 6:		/* ^F */
		case KEY_RIGHT:
			if (curx < (int)strlen(lines.v[cury])) {
				curx++;
			} else if (cury < (int)lines.num - 1) {
				cury++;
				curx = 0;
			}
			break;
		case 12:	/* ^L */
			clear();
			break;
		case 14:	/* ^N */
		case KEY_DOWN:
			if (cury < (int)lines.num - 1) {
				cury++;
			}
			if (curx > (int)strlen(lines.v[cury])) {
				curx = (int)strlen(lines.v[cury]);
			}
			if (scrolldown < cury - (LINES - 2)) {
				scrolldown = cury - (LINES - 2);
			}
			break;
		case 16:	/* ^P */
		case KEY_UP:
			if (cury > 0) {
				cury--;
			}
			if (curx > (int)strlen(lines.v[cury])) {
				curx = (int)strlen(lines.v[cury]);
			}
			if (scrolldown > cury) {
				scrolldown = cury;
			}
			break;
		case '*':
			hinting = !hinting;
			break;
		case '~':
			done = true;
			break;
		default:
			if (isascii(ch) && ch_isalpha((char)ch)) {
				if (substitute((char)ch)) {
					if (curx < (int)strlen(lines.v[cury])) {
						curx++;
					}
					if (curx == (int)strlen(lines.v[cury]) &&
					    cury < (int)lines.num - 1) {
						curx = 0;
						cury++;
					}
				}
			} else if (curx < (int)strlen(lines.v[cury]) &&
			    ch == lines.v[cury][curx]) {
				curx++;
				if (curx == (int)strlen(lines.v[cury]) &&
				    cury < (int)lines.num - 1) {
					curx = 0;
					cury++;
				}
			} else {
				beep();
			}
			break;
		}
	}
}

////////////////////////////////////////////////////////////

int
main(void)
{

	stringarray_init(&lines);
	stringarray_init(&sollines);
	srandom((unsigned int)time(NULL));
	readquote();
	encode();
	opencurses();

	keypad(stdscr, true);
	loop();

	closecurses();
	stringarray_cleanup(&sollines);
	stringarray_cleanup(&lines);
}
