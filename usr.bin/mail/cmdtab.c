/*	$NetBSD: cmdtab.c,v 1.20.14.1 2009/05/13 19:19:56 jym Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
static char sccsid[] = "@(#)cmdtab.c	8.2 (Berkeley) 4/20/95";
#else
__RCSID("$NetBSD: cmdtab.c,v 1.20.14.1 2009/05/13 19:19:56 jym Exp $");
#endif
#endif /* not lint */

#include "def.h"
#include "extern.h"
#include "thread.h"

#ifdef USE_EDITLINE
# define CMP(x)	#x,
# define CMP0	0,
#else
# define CMP(x)
# define CMP0
#endif

#define A	(C_PIPE_SHELL | C_PIPE_PAGER)
#define C	(C_PIPE_SHELL | C_PIPE_CRT)
#define S	(C_PIPE_SHELL)

/*
 * Mail -- a mail program
 *
 * Define all of the command names and bindings.
 */
/* R - in left comment means recursive */
const struct cmd cmdtab[] = {
	{ "next",	next,		C, CMP(n)	NDMLIST,	0,		MMNDEL },
	{ "alias",	group,		S, CMP(A)	T|M|RAWLIST,	0,		1000 },
/* R */	{ "print",	type,		C, CMP(n)	MSGLIST,	0,		MMNDEL },
/* R */	{ "Print",	Type,		C, CMP(n)	MSGLIST,	0,		MMNDEL },
/* R */	{ "type",	type,		C, CMP(n)	MSGLIST,	0,		MMNDEL },
/* R */	{ "Type",	Type,		C, CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "visual",	visual,		0, CMP(n)	I|MSGLIST,	0,		MMNORM },
/* R */	{ "top",	top,		S, CMP(n)	MSGLIST,	0,		MMNDEL },
/* R */	{ "touch",	stouch,		0, CMP(n)	W|MSGLIST,	0,		MMNDEL },
/* R */	{ "preserve",	preserve,	0, CMP(n)	W|MSGLIST,	0,		MMNDEL },
/* R */	{ "delete",	delete,		0, CMP(n)	W|P|MSGLIST,	0,		MMNDEL },
/* R */	{ "dp",		deltype,	C, CMP(n)	W|MSGLIST,	0,		MMNDEL },
/* R */	{ "dt",		deltype,	C, CMP(n)	W|MSGLIST,	0,		MMNDEL },
/* R */	{ "undelete",	undeletecmd,	0, CMP(n)	P|MSGLIST,	MDELETED,	MMNDEL },
	{ "unset",	unset,		0, CMP(S)	T|M|RAWLIST,	1,		1000 },
	{ "mail",	sendmail,	0, CMP(A)	R|M|I|STRLIST,	0,		0 },
/* R */	{ "mbox",	mboxit,		0, CMP(n)	W|MSGLIST,	0,		MMNDEL },
/* R */	{ "more",	type,		A, CMP(n)	MSGLIST,	0,		MMNDEL },
/* R */	{ "More",	Type,		A, CMP(n)	MSGLIST,	0,		MMNDEL },
/* R */	{ "page",	type,		A, CMP(n)	MSGLIST,	0,		MMNDEL },
/* R */	{ "Page",	Type,		A, CMP(n)	MSGLIST,	0,		MMNDEL },
#ifdef MIME_SUPPORT
/* R */	{ "view",	view,		A, CMP(n)	MSGLIST,	0,		MMNDEL },
/* R */	{ "View",	View,		A, CMP(n)	MSGLIST,	0,		MMNDEL },
#endif
	{ "unalias",	unalias,	0, CMP(A)	T|M|RAWLIST,	1,		1000 },
/* R */	{ "unread",	unread,		0, CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "!",		shell,		0, CMP(xF)	T|I|STRLIST,	0,		0 },
	{ "|",		pipecmd,	0, CMP(xF)	I|STRLIST,	0,		0 },
/* R */	{ "copy",	copycmd,	0, CMP(F)	M|STRLIST,	0,		0 },
	{ "chdir",	schdir,		0, CMP(F)	T|M|RAWLIST,	0,		1 },
	{ "cd",		schdir,		0, CMP(F)	T|M|RAWLIST,	0,		1 },
/* R */	{ "save",	save,		0, CMP(F)	STRLIST,	0,		0 },
/* R */	{ "Save",	Save,		0, CMP(F)	STRLIST,	0,		0 },
	{ "source",	source,		0, CMP(F)	T|M|RAWLIST,	1,		1 },
	{ "set",	set,		S, CMP(sF)	T|M|RAWLIST,	0,		1000 },
	{ "shell",	dosh,		0, CMP(n)	T|I|NOLIST,	0,		0 },
	{ "show",	show,		S, CMP(S)	T|M|RAWLIST,	0,		1000 },
	{ "version",	pversion,	S, CMP(n)	T|M|NOLIST,	0,		0 },
	{ "group",	group,		S, CMP(a)	T|M|RAWLIST,	0,		1000 },
/* R */	{ "write",	swrite,		0, CMP(F)	STRLIST,	0,		0 },
	{ "from",	from,		S, CMP(n)	MSGLIST,	0,		MMNORM },
	{ "file",	file,		0, CMP(f)	T|M|RAWLIST,	0,		1 },
	{ "folder",	file,		0, CMP(f)	T|M|RAWLIST,	0,		1 },
	{ "folders",	folders,	S, CMP(n)	T|M|NOLIST,	0,		0 },
#ifdef MIME_SUPPORT
	{ "forward",	forward,	0, CMP(n)	R|I|MSGLIST,	0,		0 },
#endif
	{ "bounce",	bounce,		0, CMP(n)	R|I|MSGLIST,	0,		0 },
	{ "?",		help,		S, CMP(n)	T|M|NOLIST,	0,		0 },
	{ "z",		scroll,		S, CMP(n)	M|STRLIST,	0,		0 },
	{ "headers",	headers,	S, CMP(n)	T|MSGLIST,	0,		MMNDEL },
	{ "help",	help,		S, CMP(n)	T|M|NOLIST,	0,		0 },
	{ "=",		pdot,		S, CMP(n)	T|MSGLIST,	0,		MMNDEL },
	{ "Reply",	Respond,	0, CMP(n)	R|I|MSGLIST,	0,		MMNDEL },
	{ "Respond",	Respond,	0, CMP(n)	R|I|MSGLIST,	0,		MMNDEL },
	{ "reply",	respond,	0, CMP(n)	R|I|MSGLIST,	0,		MMNDEL },
	{ "respond",	respond,	0, CMP(n)	R|I|MSGLIST,	0,		MMNDEL },
	{ "edit",	editor,		0, CMP(n)	I|MSGLIST,	0,		MMNORM },
	{ "echo",	echo,		S, CMP(F)	T|M|RAWLIST,	0,		1000 },
	{ "quit",	quitcmd,	0, CMP(n)	NOLIST,		0,		0 },
	{ "list",	pcmdlist,	S, CMP(n)	T|M|NOLIST,	0,		0 },
	{ "xit",	rexit,		0, CMP(n)	M|NOLIST,	0,		0 },
	{ "exit",	rexit,		0, CMP(n)	M|NOLIST,	0,		0 },
	{ "size",	messize,	S, CMP(n)	T|MSGLIST,	0,		MMNDEL },
/* R */	{ "hold",	preserve,	0, CMP(n)	W|MSGLIST,	0,		MMNDEL },
	{ "if",		ifcmd,		0, CMP(F)	T|F|M|RAWLIST,	1,		1 },
	{ "ifdef",	ifdefcmd,	0, CMP(F)	T|F|M|RAWLIST,	1,		1 },
	{ "ifndef",	ifndefcmd,	0, CMP(F)	T|F|M|RAWLIST,	1,		1 },
	{ "else",	elsecmd,	0, CMP(F)	T|F|M|RAWLIST,	0,		0 },
	{ "endif",	endifcmd,	0, CMP(F)	T|F|M|RAWLIST,	0,		0 },
	{ "alternates",	alternates,	S, CMP(n)	T|M|RAWLIST,	0,		1000 },
	{ "ignore",	igfield,	S, CMP(n)	T|M|RAWLIST,	0,		1000 },
	{ "discard",	igfield,	S, CMP(n)	T|M|RAWLIST,	0,		1000 },
	{ "retain",	retfield,	S, CMP(n)	T|M|RAWLIST,	0,		1000 },
	{ "saveignore",	saveigfield,	S, CMP(n)	T|M|RAWLIST,	0,		1000 },
	{ "savediscard",saveigfield,	S, CMP(n)	T|M|RAWLIST,	0,		1000 },
	{ "saveretain",	saveretfield,	S, CMP(n)	T|M|RAWLIST,	0,		1000 },
	{ "Header",	Header,		S, CMP(n)	T|M|STRLIST,	0,		1000 },
	{ "core",	core,		0, CMP(F)	T|M|NOLIST,	0,		0 },
	{ "#",		null,		0, CMP(n)	T|M|NOLIST,	0,		0 },
	{ "clobber",	clobber,	0, CMP(n)	T|M|RAWLIST,	0,		1 },
	{ "inc",	inc,		S, CMP(n)	T|NOLIST,	0,		0 },
	{ "smopts",	smoptscmd,	S, CMP(m)	T|M|RAWLIST,	0,		1000 },
	{ "unsmopts",	unsmoptscmd,	S, CMP(M)	T|M|RAWLIST,	1,		1000 },
/* R */	{ "mkread",	markread,	0, CMP(n)	MSGLIST,	0,		MMNDEL },
#ifdef MIME_SUPPORT
/* R */	{ "detach",	detach,		S, CMP(F)	STRLIST,	0,		0 },
/* R */	{ "Detach",	Detach,		S, CMP(F)	STRLIST,	0,		0 },
#endif
#ifdef THREAD_SUPPORT
	{ "flatten",	flattencmd,	0, CMP(n)	T|NDMLIST,	0,		MMNDEL },
	{ "reverse",	reversecmd,	0, CMP(n)	T|STRLIST,	0,		0 },
	{ "sort",	sortcmd,	0, CMP(T)	T|STRLIST,	0,		0 },
	{ "thread",	threadcmd,	0, CMP(T)	T|STRLIST,	0,		0 },
	{ "unthread",	unthreadcmd,	0, CMP(n)	T|STRLIST,	0,		0 },

	{ "down",	downcmd,	0, CMP(n)	T|MSGLIST,	0,		MMNDEL },
	{ "tset",	tsetcmd,	0, CMP(n)	T|MSGLIST,	0,		MMNDEL },
	{ "up",		upcmd,		0, CMP(n)	T|STRLIST,	0,		0 },

	{ "expose",	exposecmd,	0, CMP(n)	T|STRLIST,	0,		0 },
	{ "hide",	hidecmd,	0, CMP(n)	T|STRLIST,	0,		0 },
	{ "showthreads",exposecmd,	0, CMP(n)	T|STRLIST,	0,		0 },
	{ "hidethreads",hidecmd,	0, CMP(n)	T|STRLIST,	0,		0 },
#ifdef THREAD_DEBUG
	{ "debug_links",thread_showcmd,	S, CMP(n)	T|MSGLIST,	0,		MMNDEL },
#endif
/* R */	{ "tag",	tagcmd,		0, CMP(n)	T|MSGLIST,	0,		MMNDEL },
/* R */	{ "untag",	untagcmd,	0, CMP(n)	T|MSGLIST,	0,		MMNDEL },
/* R */	{ "invtags",	invtagscmd,	0, CMP(n)	T|MSGLIST,	0,	 	MMNDEL },
	{ "tagbelow",	tagbelowcmd,	0, CMP(n)	T|MSGLIST,	0,		MMNDEL },

	{ "hidetags",	hidetagscmd,	0, CMP(n)	T|STRLIST,	0,		0 },
	{ "showtags",	showtagscmd,	0, CMP(n)	T|STRLIST,	0,		0 },

	{ "deldups",	deldupscmd,	0, CMP(n)	T|STRLIST,	0,		0 },
#endif /* THREAD_SUPPORT */
	{ 0,		0,		0, CMP0		0,		0,		0 }
};
