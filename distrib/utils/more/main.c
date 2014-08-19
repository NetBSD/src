/*	$NetBSD: main.c,v 1.10.14.1 2014/08/19 23:45:45 tls Exp $	*/

/*
 * Copyright (c) 1988 Mark Nudelman
 * Copyright (c) 1988, 1993
 *	Regents of the University of California.  All rights reserved.
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
__COPYRIGHT("@(#) Copyright (c) 1988 Mark Nudelman.");
__COPYRIGHT("@(#) Copyright (c) 1988, 1993 "
"Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.1 (Berkeley) 6/7/93";
#else
__RCSID("$NetBSD: main.c,v 1.10.14.1 2014/08/19 23:45:45 tls Exp $");
#endif
#endif /* not lint */

/*
 * Entry point, initialization, miscellaneous routines.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "less.h"
#include "extern.h"

int	ispipe;
int	new_file;
int	is_tty;
char	*current_file, *previous_file, *current_name, *next_name;
off_t	prev_pos;
int	any_display;
int	scroll_lines;
int	ac;
char	**av;
int	curr_ac;
int	quitting;

static void cat_file __P((void));
/*
 * Edit a new file.
 * Filename "-" means standard input.
 * No filename means the "current" file, from the command line.
 */
int
edit(filename)
	char *filename;
{
	int f;
	char *m;
	off_t initial_pos;
	static int didpipe;
	char message[100], *p;

	initial_pos = NULL_POSITION;
	if (filename == NULL || *filename == '\0') {
		if (curr_ac >= ac) {
			error("No current file");
			return(0);
		}
		filename = save(av[curr_ac]);
	}
	else if (strcmp(filename, "#") == 0) {
		if (*previous_file == '\0') {
			error("no previous file");
			return(0);
		}
		filename = save(previous_file);
		initial_pos = prev_pos;
	} else
		filename = save(filename);

	/* use standard input. */
	if (!strcmp(filename, "-")) {
		if (didpipe) {
			error("Can view standard input only once");
			return(0);
		}
		f = 0;
	}
	else if ((m = bad_file(filename, message, sizeof(message))) != NULL) {
		error(m);
		free(filename);
		return(0);
	}
	else if ((f = open(filename, O_RDONLY, 0)) < 0) {
		(void)snprintf(message, sizeof(message), "%s: %s", filename,
		    strerror(errno));
		error(message);
		free(filename);
		return(0);
	}

	if (isatty(f)) {
		/*
		 * Not really necessary to call this an error,
		 * but if the control terminal (for commands)
		 * and the input file (for data) are the same,
		 * we get weird results at best.
		 */
		error("Can't take input from a terminal");
		if (f > 0)
			(void)close(f);
		(void)free(filename);
		return(0);
	}

	/*
	 * We are now committed to using the new file.
	 * Close the current input file and set up to use the new one.
	 */
	if (file > 0)
		(void)close(file);
	new_file = 1;
	if (previous_file != NULL)
		free(previous_file);
	previous_file = current_file;
	current_file = filename;
	pos_clear();
	prev_pos = position(TOP);
	ispipe = (f == 0);
	if (ispipe) {
		didpipe = 1;
		current_name = "stdin";
	} else
		current_name = (p = rindex(filename, '/')) ? p + 1 : filename;
	if (curr_ac >= ac)
		next_name = NULL;
	else
		next_name = av[curr_ac + 1];
	file = f;
	ch_init(cbufs, 0);
	init_mark();

	if (is_tty) {
		int no_display = !any_display;
		any_display = 1;
		if (no_display && errmsgs > 0) {
			/*
			 * We displayed some messages on error output
			 * (file descriptor 2; see error() function).
			 * Before erasing the screen contents,
			 * display the file name and wait for a keystroke.
			 */
			error(filename);
		}
		/*
		 * Indicate there is nothing displayed yet.
		 */
		if (initial_pos != NULL_POSITION)
			jump_loc(initial_pos);
		clr_linenum();
	}
	return(1);
}

/*
 * Edit the next file in the command line list.
 */
void
next_file(n)
	int n;
{
	if (curr_ac + n >= ac) {
		if (quit_at_eof || position(TOP) == NULL_POSITION)
			quit();
		error("No (N-th) next file");
	}
	else
		(void)edit(av[curr_ac += n]);
}

/*
 * Edit the previous file in the command line list.
 */
void
prev_file(n)
	int n;
{
	if (curr_ac - n < 0)
		error("No (N-th) previous file");
	else
		(void)edit(av[curr_ac -= n]);
}

/*
 * copy a file directly to standard output; used if stdout is not a tty.
 * the only processing is to squeeze multiple blank input lines.
 */
static void
cat_file()
{
	int c, empty;

	if (squeeze) {
		empty = 0;
		while ((c = ch_forw_get()) != EOI)
			if (c != '\n') {
				putchr(c);
				empty = 0;
			}
			else if (empty < 2) {
				putchr(c);
				++empty;
			}
	}
	else while ((c = ch_forw_get()) != EOI)
		putchr(c);
	flush();
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	int envargc, argcnt;
	char *envargv[3];

	/*
	 * Process command line arguments and MORE environment arguments.
	 * Command line arguments override environment arguments.
	 */
	if ((envargv[1] = getenv("MORE")) != NULL) {
		envargc = 2;
		envargv[0] = "more";
		envargv[2] = NULL;
		(void)option(envargc, envargv);
	}
	argcnt = option(argc, argv);
	argv += argcnt;
	argc -= argcnt;

	/*
	 * Set up list of files to be examined.
	 */
	ac = argc;
	av = argv;
	curr_ac = 0;

	/*
	 * Set up terminal, etc.
	 */
	is_tty = isatty(1);
	if (!is_tty) {
		/*
		 * Output is not a tty.
		 * Just copy the input file(s) to output.
		 */
		if (ac < 1) {
			(void)edit("-");
			cat_file();
		} else {
			do {
				(void)edit((char *)NULL);
				if (file >= 0)
					cat_file();
			} while (++curr_ac < ac);
		}
		exit(0);
	}

	raw_mode(1);
	get_term();
	open_getchr();
	init();
	init_signals(1);

	/* select the first file to examine. */
	if (ac < 1)
		(void)edit("-");	/* Standard input */
	else {
		/*
		 * Try all the files named as command arguments.
		 * We are simply looking for one which can be
		 * opened without error.
		 */
		do {
			(void)edit((char *)NULL);
		} while (file < 0 && ++curr_ac < ac);
	}

	if (file >= 0)
		commands();
	quit();
}

/*
 * Copy a string to a "safe" place
 * (that is, to a buffer allocated by malloc).
 */
char *
save(s)
	char *s;
{
	char *p;

	p = strdup(s);
	if (p == NULL)
	{
		error("cannot allocate memory");
		quit();
	}
	return(p);
}

/*
 * Exit the program.
 */
void
quit()
{
	/*
	 * Put cursor at bottom left corner, clear the line,
	 * reset the terminal modes, and exit.
	 */
	quitting = 1;
	lower_left();
	clear_eol();
	deinit();
	flush();
	raw_mode(0);
	exit(0);
}
