/*	$NetBSD: run.c,v 1.2 1997/09/27 00:09:29 phil Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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
 *      This product includes software develooped for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* run.c -- routines to interact with other programs. */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "defs.h"

#ifdef DEBUG
#define Xsystem(y) printf ("%s\n", y), 0
#else
#define Xsystem(y) system(y)
#endif

#define BUFSIZE 4096

int
collect (int kind, char **buffer, char *name, ...)
{
	size_t nbytes;		/* Number of bytes in buffer. */
	size_t fbytes;		/* Number of bytes in file. */
	struct stat st;		/* stat information. */
	int ch;
	FILE *f;
	char fileorcmd [STRSIZE];
	va_list ap;


	va_start (ap, name);
	vsnprintf (fileorcmd, STRSIZE, name, ap);
	va_end (ap);

	if (kind == T_FILE) {
		/* Get the file information. */
		if (stat (fileorcmd, &st)) {
			*buffer = NULL;
			return -1;
		}
		fbytes = (size_t)st.st_size;

		/* Open the file. */
		f = fopen (fileorcmd, "r");
		if (f == NULL) {
			*buffer = NULL;
			return -1;
		}

	} else {
		/* Open the program. */
		f = popen (fileorcmd, "r");
		if (f == NULL) {
			*buffer = NULL;
			return -1;
		}
		
		fbytes = BUFSIZE;
	}

	if (fbytes == 0)
		fbytes = BUFSIZE;
	
	/* Allocate the buffer size. */
	*buffer = (char *) malloc (fbytes + 1);
	if (!*buffer) 
		return -1;

	/* Read the buffer. */
	nbytes = 0;
	while (nbytes < fbytes && (ch = fgetc(f)) != EOF)
		(*buffer)[nbytes++] = ch;

	(*buffer)[nbytes] = 0;

	if (kind == T_FILE)
		fclose(f);
	else
		pclose(f);

	return nbytes;
}

/* Run a program ... using printf style to format the command! */

int run_prog (char *cmd, ...)
{
	char command [STRSIZE];
	va_list ap;
	int ret;

	va_start (ap, cmd);
	(void) vsnprintf (command, STRSIZE, cmd, ap);
	va_end (ap);
	/* The following may be more than one function call.  Can't just
	   "return Xsystem (command);" */
	ret = Xsystem (command);
	return ret;
}

