/*	$NetBSD: commands.c,v 1.17 2021/12/06 22:45:42 rillig Exp $	*/

/*-
 * Copyright 2009 Brett Lymn <blymn@NetBSD.org>
 * Copyright 2021 Roland Illig <rillig@NetBSD.org>
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <sys/types.h>

#include "returns.h"
#include "slave.h"
#include "command_table.h"

extern int initdone;

static void report_message(data_enum_t, const char *);

/*
 * Match the passed command string and execute the associated test
 * function.
 */
void
command_execute(char *func, int nargs, char **args)
{
	size_t i, j;

	for (i = 0; i < ncmds; i++) {
		if (strcmp(func, commands[i].name) != 0)
			continue;

		/* Check only restricted set of functions is called before
		 * initscr/newterm */
		if (initdone) {
			/* matched function */
			commands[i].func(nargs, args);
			return;
		}

		for (j = 0; j < nrcmds; j++) {
			if (strcmp(func, restricted_commands[j]) != 0)
				continue;

			if (strcmp(func, "initscr") == 0  ||
			    strcmp(func, "newterm") == 0)
				initdone = 1;

			/* matched function */
			commands[i].func(nargs, args);
			return;
		}
		report_status("YOU NEED TO CALL INITSCR/NEWTERM FIRST");
		return;
	}

	report_status("UNKNOWN_FUNCTION");
}

static void
write_to_director(const void *mem, size_t size)
{
	ssize_t nwritten = write(to_director, mem, size);
	if (nwritten == -1)
		err(1, "writing to director failed");
	if ((size_t)nwritten != size)
		errx(1, "short write to director, expected %zu, got %zd",
		    size, nwritten);
}

static void
write_to_director_int(int i)
{
	write_to_director(&i, sizeof(i));
}

static void
write_to_director_type(data_enum_t return_type)
{
	write_to_director_int(return_type);
}

/*
 * Report a pointer value back to the director
 */
void
report_ptr(void *ptr)
{
	char *string;

	if (ptr == NULL)
		asprintf(&string, "NULL");
	else
		asprintf(&string, "%p", ptr);
	report_status(string);
	free(string);
}

/*
 * Report an integer value back to the director
 */
void
report_int(int value)
{
	char *string;

	asprintf(&string, "%d", value);
	report_status(string);
	free(string);
}

/*
 * Report either an ERR or OK back to the director
 */
void
report_return(int status)
{
	if (status == ERR)
		write_to_director_type(data_err);
	else if (status == OK)
		write_to_director_type(data_ok);
	else if (status == KEY_CODE_YES)
		report_int(status);
	else
		report_status("INVALID_RETURN");
}

/*
 * Report the number of returns back to the director via the command pipe
 */
void
report_count(int count)
{
	write_to_director_type(data_count);
	write_to_director_int(count);
}

/*
 * Report the status back to the director via the command pipe
 */
void
report_status(const char *status)
{
	report_message(data_string, status);
}

/*
 * Report an error message back to the director via the command pipe.
 */
void
report_error(const char *status)
{
	report_message(data_slave_error, status);
}

/*
 * Report the message with the given type back to the director via the
 * command pipe.
 */
static void
report_message(data_enum_t type, const char *status)
{
	size_t len = strlen(status);
	write_to_director_type(type);
	write_to_director_int(len);
	write_to_director(status, len);
}

/*
 * Report a string of chtype back to the director via the command pipe.
 */
void
report_byte(chtype c)
{
	chtype string[2];

	string[0] = c;
	string[1] = A_NORMAL | '\0';
	report_nstr(string);
}

/*
 * Report a string of chtype back to the director via the command pipe.
 */
void
report_nstr(chtype *string)
{
	size_t size;
	chtype *p;

	for (p = string; (*p & __CHARTEXT) != 0; p++)
		continue;

	size = (size_t)(p + 1 - string) * sizeof(*p);

	write_to_director_type(data_byte);
	write_to_director_int(size);
	write_to_director(string, size);
}

/*
 * Report a cchar_t back to the director via the command pipe.
 */
void
report_cchar(cchar_t c)
{

	write_to_director_type(data_cchar);
	write_to_director_int(sizeof(c));
	write_to_director(&c, sizeof(c));
}

/*
 * Report a wchar_t back to the director via the command pipe.
 */
void
report_wchar(wchar_t ch)
{
	wchar_t wstr[2];

	wstr[0] = ch;
	wstr[1] = L'\0';
	report_wstr(wstr);
}


/*
 * Report a string of wchar_t back to the director via the command pipe.
 */
void
report_wstr(wchar_t *wstr)
{
	size_t size;
	wchar_t *p;

	for (p = wstr; *p != L'\0'; p++)
		continue;
	size = (size_t)(p + 1 - wstr) * sizeof(*p);


	write_to_director_type(data_wchar);
	write_to_director_int(size);
	write_to_director(wstr, size);
}

/*
 * Check the number of args we received are what we expect.  Return an
 * error if they do not match.
 */
int
check_arg_count(int nargs, int expected)
{
	if (nargs != expected) {
		report_count(1);
		report_error("INCORRECT_ARGUMENT_NUMBER");
		return 1;
	}

	return 0;
}
