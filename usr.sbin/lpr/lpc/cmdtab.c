/*	$NetBSD: cmdtab.c,v 1.8 2006/10/22 21:09:47 christos Exp $	*/

/*
 * Copyright (c) 1983, 1993
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
static char sccsid[] = "@(#)cmdtab.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: cmdtab.c,v 1.8 2006/10/22 21:09:47 christos Exp $");
#endif
#endif /* not lint */

#include <stdio.h>

#include "lpc.h"
#include "extern.h"

/*
 * lpc -- command tables
 */
#define aborthelp	"terminate a spooling daemon immediately and disable printing"
#define cleanhelp	"remove cruft files from a queue"
#define enablehelp	"turn a spooling queue on"
#define disablehelp	"turn a spooling queue off"
#define downhelp	"do a 'stop' followed by 'disable' and put a message in status"
#define helphelp	"get help on commands"
#define quithelp	"exit lpc"
#define restarthelp	"kill (if possible) and restart a spooling daemon"
#define starthelp	"enable printing and start a spooling daemon"
#define statushelp	"show status of daemon and queue"
#define stophelp	"stop a spooling daemon after current job completes and disable printing"
#define topqhelp	"put job at top of printer queue"
#define uphelp		"enable everything and restart spooling daemon"

struct cmd cmdtab[] = {
	{ "abort",	aborthelp,	doabort,	1 },
	{ "clean",	cleanhelp,	clean,		1 },
	{ "enable",	enablehelp,	enable,		1 },
	{ "exit",	quithelp,	quit,		0 },
	{ "disable",	disablehelp,	disable,	1 },
	{ "down",	downhelp,	down,		1 },
	{ "help",	helphelp,	help,		0 },
	{ "quit",	quithelp,	quit,		0 },
	{ "restart",	restarthelp,	restart,	0 },
	{ "start",	starthelp,	startcmd,	1 },
	{ "status",	statushelp,	status,		0 },
	{ "stop",	stophelp,	stop,		1 },
	{ "topq",	topqhelp,	topq,		1 },
	{ "up",		uphelp,		up,		1 },
	{ "?",		helphelp,	help,		0 },
	{ .c_name = NULL },
};

int	NCMDS = sizeof (cmdtab) / sizeof (cmdtab[0]);
