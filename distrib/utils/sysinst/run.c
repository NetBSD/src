/*	$NetBSD: run.c,v 1.3 1997/11/02 03:45:30 jonathan Exp $	*/

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

#include "msg_defs.h"

#ifdef DEBUG
#define Xsystem(y) printf ("%s\n", y), 0
#else
#define Xsystem(y) system(y)
#endif

/*
 * local prototypes 
 */
int do_system(const char *cmdstr);
char* va_prog_cmdstr(char *cmd, va_list ap);


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


/*
 * system(3), but with a debug wrapper.
 */
int do_system(const char *execstr)
{
	register int ret;

	/* The following may be more than one function call.  Can't just
	   "return Xsystem (command);" */

	ret = Xsystem (execstr);
	return ret;

}

/*
 *  build command tring for do_system() from anonymous args.
 *  XXX return result is in a static buffer.
 */
char* va_prog_cmdstr(char *cmd, va_list ap)
{
	static char command [STRSIZE];

	bzero(command, STRSIZE);
	(void) vsnprintf (command, STRSIZE, cmd, ap);
	return command;
}
  

/*  run a program.  The caller handles any and all errors. */
int run_prog (char *cmd, ...)
{
	va_list ap;
	int ret;

    	va_start (ap, cmd);
	ret = do_system(va_prog_cmdstr(cmd, ap));
	va_end(ap);
	return ret;
}

/*
 * run a program where no errors are expected.
 * Like run_prog, except print a generic warning if the return is nonzero.
 * Used where sysinst doesn't yet know how to  recover from an error,
 * but where an error is not completely catastrophic.
 * Use stderr/stdin since we want the user to see any messages 
 * from the command before we  go back into curses.
 */
int run_prog_or_continue (char *cmd, ...)
{
	va_list ap;
	int ret;
	const char *execstr;


	va_start (ap, cmd);
	execstr = va_prog_cmdstr(cmd, ap);
	ret = do_system(execstr);
	va_end (ap);

	/* Warn copiously about any errors. */
	if (ret) {
	  /* XXX use messages instead */ 
	  printf(msg_string(MSG_anonprogfailed), execstr, strerror(ret));
	  fgetc(stdin);
	}
	return ret;
}

/*
 * run a program. Die if there are any errors.
 */
    
void run_prog_or_die (char *cmd, ...)
{
	va_list ap;
	int ret;
	const char *execstr;

	va_start (ap, cmd);
	execstr = va_prog_cmdstr(cmd, ap);
	ret = do_system(execstr);

	if (ret) {
	  printf(msg_string(MSG_progdie), execstr, strerror(ret));
	  exit(ret);
	}
}
  
