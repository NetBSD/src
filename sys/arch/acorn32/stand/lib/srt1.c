/*	$NetBSD: srt1.c,v 1.5.94.1 2009/05/13 17:16:04 jym Exp $	*/

/*
 * Copyright (c) 2001 Ben Harris.
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Command-line parsing from i386 doscommain.c */

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <riscoscalls.h>
#include <srt0.h>


/* limit of the memory given to us by RISC OS's WIMPslot; inited by srt0.S */
/* init it also to a reasonable value also so it doesnt get into the BSS ! */
void *HIMEM = (void *) 0x12000;


static int whitespace(char);

static int whitespace(char c) {
	if ((c == '\0') || (c == ' ') || (c == '\t')
	    || (c == '\r') || (c == '\n'))
		return (1);
	return (0);
}


enum state {skipping, doing_arg, doing_long_arg};


/* build argv/argc, start real main() */
void __start(void);
int splitargs(char *, int, char**);
extern int main(int, char**);

extern char edata[], end[];


void
__start(void)
{
	int argc;
	char *ro_args, *args, **argv;

	/* Clear BSS */
	memset(edata, 0, end - edata);

	/* Define heap. */
	setheap(end, (void *)(HIMEM - 0x1000));

	ro_args = os_get_env(NULL, NULL);

	args = alloc(strlen(ro_args)+10);	/* some spare extra */
	strcpy(args, ro_args);

	argc = splitargs(args, 0, NULL);
	argv = alloc(argc * sizeof(*argv));
	if (argv == NULL)
		panic("alloc of argv failed");
	argc = splitargs(args, 1, argv);

	/* start real main() */
	os_exit(NULL, main(argc, argv));
}

int
splitargs(char *args, int really, char **argv)
{
	int argc, i;
	enum state s;

	argc = 0;
	s = skipping;

	for (i = 0; args[i]; i++){

		if (whitespace(args[i])) {
			if (s == doing_arg) {
				/* end of argument word */
				if (really)
					args[i] = '\0';
				s = skipping;
			}
			continue;
		}

		if (args[i] == '"') {
			/* start or end long arg
			 * (end only if next char is whitespace)
			 *  XXX but '" ' cannot be in argument
			 */
			switch (s) {
			case skipping:
				/* next char begins new argument word */
				if (really)
					argv[argc] = &args[i + 1];
				argc++;
				s = doing_long_arg;
				break;
			case doing_long_arg:
				if (whitespace(args[i + 1])) {
					if (really)
						args[i] = '\0';
					s = skipping;
				}
				break;
			case doing_arg:
				/* ignore in the middle of arguments */
			default:
				break;
			}
			continue;
		}

		/* all other characters */
		if (s == skipping) {
			/* begin new argument word */
			if (really)
				argv[argc] = &args[i];
			argc++;
			s = doing_arg;
		}
	}
	return argc;
}

void _rtt(void)
{

	os_exit(NULL, 0);
}
