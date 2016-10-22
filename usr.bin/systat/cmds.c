/*	$NetBSD: cmds.c,v 1.30 2016/10/22 22:02:55 christos Exp $	*/

/*-
 * Copyright (c) 1980, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)cmds.c	8.2 (Berkeley) 4/29/95";
#endif
__RCSID("$NetBSD: cmds.c,v 1.30 2016/10/22 22:02:55 christos Exp $");
#endif /* not lint */

#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "systat.h"
#include "extern.h"

void	switch_mode(struct mode *p);

void
command(char *cmd)
{
	struct command *c;
	struct mode *p;
	char *args;

	if (cmd[0] == '\0')
		return;

	args = cmd;
	cmd = strsep(&args, " \t");

	if (curmode->c_commands) {
		for (c = curmode->c_commands; c->c_name; c++) {
			if (strstr(c->c_name, cmd) == c->c_name) {
				(c->c_cmd)(args);
				goto done;
			}
		}
	}

	for (c = global_commands; c->c_name; c++) {
		if (strstr(c->c_name, cmd) == c->c_name) {
			(c->c_cmd)(args);
			goto done;
		}
	}

	for (p = modes; p->c_name; p++) {
		if (strstr(p->c_name, cmd) == p->c_name) {
			switch_mode(p);
			goto done;
		}
	}

	if (isdigit((unsigned char)cmd[0])) {
		global_interval(cmd);
		goto done;
	}

	error("%s: Unknown command.", cmd);
done:
	;
}

void
switch_mode(struct mode *p)
{
	int switchfail;
	struct mode *r;

	switchfail = 0;
	r = p;

	if (curmode == p) {
		status();
		return;
	}

	alarm(0);
	(*curmode->c_close)(wnd);
	wnd = (*p->c_open)();
	if (wnd == 0) {
		wnd = (*curmode->c_open)();
		if (wnd == 0) {
			error("Couldn't change back to previous mode");
			die(0);
		}

		p = curmode;
		switchfail++;
	}

	if ((p->c_flags & CF_INIT) == 0) {
		if ((*p->c_init)())
			p->c_flags |= CF_INIT;
		else {
			(*p->c_close)(wnd);
			wnd = (*curmode->c_open)();
			p = curmode;
			switchfail++;
		}
	}

	curmode = p;
	labels();
	display(0);
	if (switchfail && !allflag)
		error("Couldn't switch mode, back to %s", curmode->c_name);
	else {
		if (switchfail && allflag) {
			r++;
			switch_mode(r);
		} else {
			status();
		}
	}
}

void
status(void)
{
	error("Showing %s, refresh every %g seconds.", curmode->c_name, naptime);
}

int
prefix(const char *s1, const char *s2)
{

	while (*s1 == *s2) {
		if (*s1 == '\0')
			return (1);
		s1++, s2++;
	}
	return (*s1 == '\0');
}
