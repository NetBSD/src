/* $NetBSD: kshell_input.c,v 1.1 1996/01/31 23:24:01 mark Exp $ */

/*
 * Copyright (c) 1994 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * shell_input.c
 *
 * string input functions
 *
 * Created      : 09/10/94
 * Last updated : 18/10/94
 *
 *    $Id: kshell_input.c,v 1.1 1996/01/31 23:24:01 mark Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

/* Special compilation symbols
 *
 * HISTORY_BUFFER - enables the use of a input history buffer. This
 *                  requires malloc and free.
 */

/*#define SOFT_CURSOR*/
#define MAX_LINES 32

/* Declare global variables */

/*#ifdef HISTORY_BUFFER*/
static int line = -1;
static char *lines[MAX_LINES];
/*#endif*/

/* Prototype declarations */

char    *strchr __P((const char *, int));
char	*strcpy __P((char *, const char *));
size_t	strlen __P((const char *));

int WaitForKey __P((caddr_t));
void deleteline __P((int, int));

/*
 * Reads a line from the keyboard into an input buffer allowing
 * cursor editing and input history.
 */

int
readstring(string, length, valid_string, insert, ident)
	char *string;
	int length;
	char *valid_string;
	char *insert;
	caddr_t ident;
{
	int key;
	int loop;
	int entered;
	int insert_mode = 1;

/*
 * If we are compiling with the history buffer we need to initialise
 * it on the first call (line == -1)
 */

/*#ifdef HISTORY_BUFFER*/
	int cur_line = 0;

	if (ident != 0) {
		if (line == -1) {
			for (loop = 0; loop < MAX_LINES; ++loop)
				lines[loop] = NULL;

			line = 0;
		}

		cur_line = line;
	}
/*#endif*/

/*
 * If we have text to preinsert into the buffer enter it and echo it
 * to the display.
 */

	if (insert && insert[0] != 0) {
		strcpy(string, insert);
		loop = strlen(insert);
		printf("%s", insert);
	} else
		loop = 0;

	entered = loop;

/*
 * The main loop.
 * Keep looping until return or CTRL-D is pressed.
 */

	do {
#ifdef SOFT_CURSOR
/*
 * Display the cursor depending on what mode we are in
 */
		if (!insert_mode)
			printf("\xe2");
		else
			printf("\xe1");
#endif

/*
 * Read the keyboard
 */

		key = WaitForKey(ident);

#ifdef SOFT_CURSOR
/*
 * Remove the cursor, restoring the text under it if necessary.
 */

		if (loop == entered || entered == 0)
			printf("\x7f");
		else
			printf("\x08%c\x08", string[loop]);
#endif

/*
 * Decode the key
 */

		switch (key) {
/*
 * DELETE
 */

		case 0x109 :
		case 0x7f :
 		{
			int loop1;

			if (loop == entered) break;
			for (loop1 = loop; loop1 < (entered - 1); ++loop1) {
				string[loop1] = string[loop1+1];
			}
			--entered;
			string[entered] = 0;
			printf("\x1b[s%s \x1b[u", &string[loop]);
		}
		break;

/*
 * BACKSPACE
 */

		case 0x08 :
		{
			int loop1;

			if (loop == 0) {
				printf("\x07");
				break;
			}
			for (loop1 = loop-1; loop1 < (entered - 1); ++loop1) {
				string[loop1] = string[loop1+1];
			}
			--loop;
			--entered;
			string[entered] = 0;
			printf("\x1b[D\x1b[s%s \x1b[u", &string[loop]);
		}
		break;

/*
 * CTRL-U
 */
		case 0x15 :
			deleteline(loop, entered);
			loop = 0;
			entered = 0;
			break;

/*
 * CTRL-A
 */
		case 0x01 :
			insert_mode = !insert_mode;
			break;

/*
 * CTRL-D
 */
		case 0x04 :
			return(-1);
			break;

/*#ifdef HISTORY_BUFFER*/
/*
 * CURSOR UP
 */

		case 0x100 :
			if (ident == 0) break;
			--cur_line;
			if (cur_line < 0)
				cur_line = MAX_LINES - 1;

			if (lines[cur_line]) {
				deleteline(loop, entered);
				loop = 0;
				entered = 0;

				for (entered = 0; lines[cur_line][entered]
				    && entered < length; ++entered) {
					string[entered] = lines[cur_line][entered];
				}

				string[entered] = 0;
				loop = entered;
				printf("%s", string);
			} else {
				deleteline(loop, entered);
				loop = 0;
				entered = 0;
			}
			break;

/*
 * CURSOR DOWN
 */

		case 0x101 :
			if (ident == 0) break;
			++cur_line;
			if (cur_line >= MAX_LINES)
				cur_line = 0;

			if (lines[cur_line]) {
				deleteline(loop, entered);
				loop = 0;
				entered = 0;
				for (entered = 0; lines[cur_line][entered]
				    && entered < length; ++entered) {
					string[entered] = lines[cur_line][entered];
				}

				loop = entered;
				string[entered] = 0;
				printf("%s", string);
			} else {
				deleteline(loop, entered);
				loop = 0;
				entered = 0;
			}
			break;
/*#endif*/

/*
 * CURSOR LEFT
 */

		case 0x102 :
			--loop;
			if (loop < 0)
				loop = 0;
			else
				printf("\x1b[D");
			break;

/*
 * CURSOR RIGHT
 */

		case 0x103 :
			++loop;
			if (loop > entered)
				loop = entered;
			else
				printf("\x1b[C");
			break;

/*
 * RETURN
 */

		case 0x0d :
			break;

/*
 * Another key
 */

		default :

/*
 * Check for a valid key to enter
 */

			if (key < 0x100 && key > 0x1f
			    && (valid_string == NULL || strchr(valid_string, key))) {
				if (!insert_mode && loop < length) {
					string[loop] = key;
					printf("%c", key);
					++loop;
					if (loop > entered) entered = loop;
				}
				else if (entered < length) {
					int loop1;

					for (loop1 = entered; loop1 >= loop; --loop1) {
						string[loop1+1] = string[loop1];
					}
					string[loop] = key;
					++loop;
					++entered;
					string[entered] = 0;
					if (loop != entered) printf("\x1b[s");
					printf("%s", &string[loop-1]);
					if (loop != entered) printf("\x1b[u\x1b[C");
				} else {
					printf("\x07");
				}
			}
			break;
		}
	} while (key != 0x0d);

	printf("\n\r");

	string[entered] = 0;

/*#ifdef HISTORY_BUFFER*/
/*
 * Update the history buffer
 */

	if (ident != 0 && entered > 0) {
		if (lines[line]) free(lines[line], M_TEMP);
			lines[line] = (char *)malloc(strlen(string) + 1, M_TEMP, M_NOWAIT);
		if (lines[line] != NULL) {
			strcpy(lines[line], string);
			++line;
			if (line >= MAX_LINES)
			line = 0;
		}
	}
/*#endif*/

	return(entered);
}


/* This erases a line of text */

void
deleteline(loop, entered)
	int loop;
	int entered;
{
	while (loop < entered) {
		++loop;
		printf("\x1b[C");
	}

	while (loop > 0) {
		--loop;
		--entered;
		printf("\x7f");
	}
}


/* Prints the contents of the history buffer */

void
shell_printhistory(argc, argv)
	int argc;
	char *argv[];
{
/*#ifdef HISTORY_BUFFER*/
	int ptr;
	int count;

	ptr = line - 1;
	count = 0;

	while (count < MAX_LINES) {
		if (ptr < 0) ptr = MAX_LINES - 1;
		if (lines[ptr])
			printf("%2d: %s\n\r", count, lines[ptr]);
		--ptr;
		++count;
	}
/*#else
	printf("History buffer not built in\n\r");
#endif*/
}

/* End of input.c */
