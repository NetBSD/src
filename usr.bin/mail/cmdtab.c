/*	$NetBSD: cmdtab.c,v 1.14 2006/10/21 21:37:20 christos Exp $	*/

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
__RCSID("$NetBSD: cmdtab.c,v 1.14 2006/10/21 21:37:20 christos Exp $");
#endif
#endif /* not lint */

#include "def.h"
#include "extern.h"

#ifdef USE_EDITLINE
# define CMP(x)	#x,
# define CMP0	0,
#else
# define CMP(x)
# define CMP0
#endif


/*
 * Mail -- a mail program
 *
 * Define all of the command names and bindings.
 */

const struct cmd cmdtab[] = {
	{ "next",	next,		CMP(n)	NDMLIST,	0,		MMNDEL },
	{ "alias",	group,		CMP(a)	M|RAWLIST,	0,		1000 },
#ifdef MIME_SUPPORT
	{ "print",	print,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "Print",	Print,		CMP(n)	MSGLIST,	0,		MMNDEL },
#else
	{ "print",	type,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "Print",	Type,		CMP(n)	MSGLIST,	0,		MMNDEL },
#endif
	{ "type",	type,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "Type",	Type,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "visual",	visual,		CMP(n)	I|MSGLIST,	0,		MMNORM },
	{ "top",	top,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "touch",	stouch,		CMP(n)	W|MSGLIST,	0,		MMNDEL },
	{ "preserve",	preserve,	CMP(n)	W|MSGLIST,	0,		MMNDEL },
	{ "delete",	delete,		CMP(n)	W|P|MSGLIST,	0,		MMNDEL },
	{ "dp",		deltype,	CMP(n)	W|MSGLIST,	0,		MMNDEL },
	{ "dt",		deltype,	CMP(n)	W|MSGLIST,	0,		MMNDEL },
	{ "undelete",	undeletecmd,	CMP(n)	P|MSGLIST,	MDELETED,	MMNDEL },
	{ "unset",	unset,		CMP(S)	M|RAWLIST,	1,		1000 },
	{ "mail",	sendmail,	CMP(A)	R|M|I|STRLIST,	0,		0 },
	{ "mbox",	mboxit,		CMP(n)	W|MSGLIST,	0,		0 },
	{ "more",	more,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "More",	More,		CMP(n)	MSGLIST,	0,		MMNDEL },
#ifdef MIME_SUPPORT
	{ "page",	page,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "Page",	Page,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "view",	view,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "View",	View,		CMP(n)	MSGLIST,	0,		MMNDEL },
#endif
	{ "page",	more,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "Page",	More,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "unalias",	unalias,	CMP(a)	M|RAWLIST,	1,		1000 },
	{ "unread",	unread,		CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "!",		shell,		CMP(xF)	I|STRLIST,	0,		0 },
	{ "|",		pipecmd,	CMP(xF)	I|STRLIST,	0,		0 },
	{ "copy",	copycmd,	CMP(F)	M|STRLIST,	0,		0 },
	{ "chdir",	schdir,		CMP(F)	M|RAWLIST,	0,		1 },
	{ "cd",		schdir,		CMP(F)	M|RAWLIST,	0,		1 },
	{ "save",	save,		CMP(F)	STRLIST,	0,		0 },
	{ "source",	source,		CMP(F)	M|RAWLIST,	1,		1 },
	{ "set",	set,		CMP(sF)	M|RAWLIST,	0,		1000 },
	{ "shell",	dosh,		CMP(n)	I|NOLIST,	0,		0 },
	{ "show",	show,		CMP(S)	M|RAWLIST,	0,		1000 },
	{ "version",	pversion,	CMP(n)	M|NOLIST,	0,		0 },
	{ "group",	group,		CMP(a)	M|RAWLIST,	0,		1000 },
	{ "write",	swrite,		CMP(F)	STRLIST,	0,		0 },
	{ "from",	from,		CMP(n)	MSGLIST,	0,		MMNORM },
	{ "file",	file,		CMP(F)	T|M|RAWLIST,	0,		1 },
	{ "folder",	file,		CMP(F)	T|M|RAWLIST,	0,		1 },
	{ "folders",	folders,	CMP(F)	T|M|NOLIST,	0,		0 },
	{ "?",		help,		CMP(n)	M|NOLIST,	0,		0 },
	{ "z",		scroll,		CMP(n)	M|STRLIST,	0,		0 },
	{ "headers",	headers,	CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "help",	help,		CMP(n)	M|NOLIST,	0,		0 },
	{ "=",		pdot,		CMP(n)	NOLIST,		0,		0 },
	{ "Reply",	Respond,	CMP(n)	R|I|MSGLIST,	0,		MMNDEL },
	{ "Respond",	Respond,	CMP(n)	R|I|MSGLIST,	0,		MMNDEL },
	{ "reply",	respond,	CMP(n)	R|I|MSGLIST,	0,		MMNDEL },
	{ "respond",	respond,	CMP(n)	R|I|MSGLIST,	0,		MMNDEL },
	{ "edit",	editor,		CMP(n)	I|MSGLIST,	0,		MMNORM },
	{ "echo",	echo,		CMP(F)	M|RAWLIST,	0,		1000 },
	{ "quit",	quitcmd,	CMP(n)	NOLIST,		0,		0 },
	{ "list",	pcmdlist,	CMP(n)	M|NOLIST,	0,		0 },
	{ "xit",	rexit,		CMP(n)	M|NOLIST,	0,		0 },
	{ "exit",	rexit,		CMP(n)	M|NOLIST,	0,		0 },
	{ "size",	messize,	CMP(n)	MSGLIST,	0,		MMNDEL },
	{ "hold",	preserve,	CMP(n)	W|MSGLIST,	0,		MMNDEL },
	{ "if",		ifcmd,		CMP(F)	F|M|RAWLIST,	1,		1 },
	{ "else",	elsecmd,	CMP(F)	F|M|RAWLIST,	0,		0 },
	{ "endif",	endifcmd,	CMP(F)	F|M|RAWLIST,	0,		0 },
	{ "alternates",	alternates,	CMP(n)	M|RAWLIST,	0,		1000 },
	{ "ignore",	igfield,	CMP(n)	M|RAWLIST,	0,		1000 },
	{ "discard",	igfield,	CMP(n)	M|RAWLIST,	0,		1000 },
	{ "retain",	retfield,	CMP(n)	M|RAWLIST,	0,		1000 },
	{ "saveignore",	saveigfield,	CMP(n)	M|RAWLIST,	0,		1000 },
	{ "savediscard",saveigfield,	CMP(n)	M|RAWLIST,	0,		1000 },
	{ "saveretain",	saveretfield,	CMP(n)	M|RAWLIST,	0,		1000 },
/*	{ "Header",	Header,		CMP(n)	STRLIST,	0,		1000 },	*/
	{ "core",	core,		CMP(F)	M|NOLIST,	0,		0 },
	{ "#",		null,		CMP(n)	M|NOLIST,	0,		0 },
	{ "clobber",	clobber,	CMP(n)	M|RAWLIST,	0,		1 },
	{ "inc",	inc,		CMP(n)	T|NOLIST,	0,		0 },
	{ 0,		0,		CMP0		0,	0,		0 }
};
