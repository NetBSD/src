/*	$NetBSD: cmds.c,v 1.21 2000/06/05 21:48:25 mycroft Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__RCSID("$NetBSD: cmds.c,v 1.21 2000/06/05 21:48:25 mycroft Exp $");
#endif /* not lint */

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include "systat.h"
#include "extern.h"

void	switch_mode __P((struct mode *p));

void
command(cmd)
	char *cmd;
{
	struct command *c;
	struct mode *p;
	char *args;
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, NULL);

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

	if (isdigit(cmd[0])) {
		global_interval(cmd);
		goto done;
	}

	error("%s: Unknown command.", cmd);
done:
	sigprocmask(SIG_UNBLOCK, &set, NULL);
}

void
switch_mode(p)
	struct mode *p;
{
	int switchfail;

	switchfail = 0;

	if (curmode == p)
		return;

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
	if (switchfail)
		error("Couldn't switch mode, back to %s", curmode->c_name);
	else
		status();
}

void
status()
{
	error("Showing %s, refresh every %d seconds.", curmode->c_name, naptime);
}
