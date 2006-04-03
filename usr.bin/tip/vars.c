/*	$NetBSD: vars.c,v 1.12 2006/04/03 04:53:58 christos Exp $	*/

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
static char sccsid[] = "@(#)vars.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: vars.c,v 1.12 2006/04/03 04:53:58 christos Exp $");
#endif /* not lint */

#include "tip.h"
#include "pathnames.h"

static char path_bshell[] = _PATH_BSHELL;
/*
 * Definition of variables
 */
value_t vtable[] = {
	{ "beautify",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "be",		(void *)TRUE },
	{ "baudrate",	NUMBER|IREMOTE|INIT,	(READ<<PUBLIC)|(WRITE<<ROOT),
	  "ba",		&BR },
	{ "dialtimeout",NUMBER,			(READ<<PUBLIC)|(WRITE<<ROOT),
	  "dial",	(void *)60 },
	{ "eofread",	STRING|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "eofr",	&IE },
	{ "eofwrite",	STRING|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "eofw",	&OE },
	{ "eol",	STRING|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  NULL,		&EL },
	{ "escape",	CHAR,			(READ|WRITE)<<PUBLIC,
	  "es",		(void *)'~' },
	{ "exceptions",	STRING|INIT|IREMOTE,	(READ|WRITE)<<PUBLIC,
	  "ex",		&EX },
	{ "force",	CHAR,			(READ|WRITE)<<PUBLIC,
	  "fo",		NULL },
	{ "framesize",	NUMBER|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "fr",		&FS },
	{ "host",	STRING|IREMOTE|INIT,	READ<<PUBLIC,
	  "ho",		&HO },
	{ "phones",	STRING|INIT|IREMOTE,	READ<<PUBLIC,
	  NULL,		&PH },
	{ "prompt",	CHAR,			(READ|WRITE)<<PUBLIC,
	  "pr",		(void *)'\n' },
	{ "raise",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "ra",		(void *)FALSE },
	{ "raisechar",	CHAR,			(READ|WRITE)<<PUBLIC,
	  "rc",		NULL },
	{ "record",	STRING|INIT|IREMOTE,	(READ|WRITE)<<PUBLIC,
	  "rec",	&RE },
	{ "remote",	STRING|INIT|IREMOTE,	READ<<PUBLIC,
	  NULL,		&RM },
	{ "script",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "sc",		FALSE },
	{ "tabexpand",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "tab",	(void *)FALSE },
	{ "verbose",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "verb",	(void *)TRUE },
	{ "SHELL",	STRING|ENVIRON|INIT,	(READ|WRITE)<<PUBLIC,
	  NULL,		path_bshell },
	{ "HOME",	STRING|ENVIRON,		(READ|WRITE)<<PUBLIC,
	  NULL,		NULL },
	{ "echocheck",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "ec",		(void *)FALSE },
	{ "disconnect",	STRING|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "di",		&DI },
	{ "tandem",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "ta",		(void *)TRUE },
	{ "linedelay",	NUMBER|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "ldelay",	&DL },
	{ "chardelay",	NUMBER|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "cdelay",	&CL },
	{ "etimeout",	NUMBER|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "et",		&ET },
	{ "rawftp",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "raw",	(void *)FALSE },
	{ "halfduplex",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "hdx",	(void *)FALSE },
	{ "localecho",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "le",		(void *)FALSE },
	{ "parity",	STRING|INIT|IREMOTE,	(READ|WRITE)<<PUBLIC,
	  "par",	&PA },
	{ "hardwareflow", BOOL,			(READ|WRITE)<<PUBLIC,
	  "hf",		(void *)FALSE },
	{ NULL,	0, 0, NULL, NULL }
};
