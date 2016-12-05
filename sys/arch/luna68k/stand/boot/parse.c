/*	$NetBSD: parse.c,v 1.6.6.2 2016/12/05 10:54:54 skrll Exp $	*/

/*
 * Copyright (c) 1992 OMRON Corporation.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *
 *	@(#)parse.c	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *
 *	@(#)parse.c	8.1 (Berkeley) 6/10/93
 */

/*
 * parse.c -- command parser
 * by A.Fujita, JAN-30-1992
 */

#include <lib/libkern/libkern.h>
#include <luna68k/stand/boot/samachdep.h>
#include <luna68k/stand/boot/status.h>

static int cmd_help(int, char *[]);

int
check_args(int argc, char *argv[])
{
	int i;

	for (i = 0; i < argc; i++)
		printf("argv[%d] = \"%s\"\n", i, argv[i]);

	return ST_NORMAL;
}

int
exit_program(int argc, char *argv[])
{

	return ST_EXIT;
}

static const char helpmsg[] =
	"commands are:\n"
	"boot [device(unit,part)filename] [-ads]\n"
	" (ex. \"boot sd(6,0)netbsd\", \"boot le()netbsd.old\" etc.)\n"
	"  Note unit number for SCSI device is (ctlr) * 10 + (id).\n"
	"ls [device(unit, part)[path]]\n"
	" (ex. \"ls sd(0,0)/bin\")\n"
	"help\n"
	"exit\n"
#if 0 /* debug commands */
	"checkargs\n"
	"disklabel\n"
	"howto\n"
	"screen\n"
	"scsi\n"
#endif
;

static int
cmd_help(int argc, char *argv[])
{

	printf(helpmsg);
	return ST_NORMAL;
}

struct command_entry {
	char *name;
	int (*func)(int, char **);
};

static const struct command_entry entries[] = {
	{ "b",		boot         },
	{ "boot",	boot         },
	{ "chkargs",	check_args   },
	{ "disklabel",	disklabel    },
	{ "exit",	exit_program },
#ifdef notyet
	{ "fsdump",	fsdump       },
	{ "fsrestore",	fsrestore    },
#endif
	{ "help",	cmd_help     },
	{ "ls",		cmd_ls       },
	{ "screen",	screen	     },
#ifdef notyet
	{ "tape",	tape	     },
	{ "tp",		tape	     },
#endif
	{ "scsi",	scsi         },
	{ "quit",	exit_program },
	{ NULL, NULL }
};


int
parse(int argc, char *argv[])
{
	int i, status = ST_NOTFOUND;

	for (i = 0; entries[i].name != NULL; i++) {
		if (!strcmp(argv[0], entries[i].name)) {
			status = (*entries[i].func)(argc, argv);
			break;
		}
	}

	return status;
}



/*
 * getargs -- make argument arrays
 */

int
getargs(char buffer[], char *argv[], int maxargs)
{
	int n = 0;
	char *p = buffer;

	argv[n++] = p;
	while (*p != '\0') {
		if (*p == ' ') {
			*p = '\0';
		} else if (p != buffer && *(p - 1) == '\0') {
			if (n < maxargs)
				argv[n++] = p;
		}
		p++;
	}

	return n;
}
