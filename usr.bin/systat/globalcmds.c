/*	$NetBSD: globalcmds.c,v 1.6 2000/01/08 23:12:37 itojun Exp $ */

/*-
 * Copyright (c) 1999
 *      The NetBSD Foundation, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD Foundation.
 * 4. Neither the name of the Foundation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "systat.h"
#include "extern.h"


static char *shortname __P((const char *, const char *));

static char *
shortname(key, s)
	const char *key;
	const char *s;
{
	char *p, *q;
	size_t l;

	if (key == NULL) {
		if ((p = strdup(s)) == NULL)
			return NULL;
		q = strchr(p, '.');
		if (q && strlen(q) > 1) {
			q[1] = '*';
			q[2] = '\0';
		}
		return p;
	} else if (strncmp(key, s, l = strlen(key)) == 0 && s[l] == '.') {
		p = strdup(s + l + 1);
		return p;
	} else
		return NULL;
}

void
global_help(args)
	char *args;
{
	int col, len;
	struct mode *p;
	char *cur, *prev;

	move(CMDLINE, col = 0);
	cur = prev = NULL;
	for (p = modes; p->c_name; p++) {
		if ((cur = shortname(args, p->c_name)) == NULL)
			continue;
		if (cur && prev && strcmp(cur, prev) == 0) {
			free(cur);
			continue;
		}
		len = strlen(cur);
		if (col + len > COLS)
			break;
		addstr(cur); col += len;
		if (col + 1 < COLS)
			addch(' ');
		if (prev)
			free(prev);
		prev = cur;
	}
	if (col == 0 && args) {
		standout();
		addstr("help: no matches");
		standend();
	}
	clrtoeol();
	if (cur)
		free(cur);
	if (prev)
		free(prev);
}

void
global_interval(args)
	char *args;
{
	int interval;

	if (!args) {
		interval = 5;
	} else {
		interval = atoi(args);
	}

	if (interval <= 0) {
		error("%d: bad interval.", interval);
		return;
	}

	alarm(0);
	naptime = interval;
	display(0);
	status();
}


void
global_load(args)
	char *args;
{
	(void)getloadavg(avenrun, sizeof(avenrun)/sizeof(avenrun[0]));
	mvprintw(CMDLINE, 0, "%4.1f %4.1f %4.1f",
	    avenrun[0], avenrun[1], avenrun[2]);
	clrtoeol();
}

void
global_quit(args)
	char *args;
{
	die(0);
}

void
global_stop(args)
	char *args;
{
	alarm(0);
	mvaddstr(CMDLINE, 0, "Refresh disabled.");
	clrtoeol();
}
