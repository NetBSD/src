/*	$NetBSD: txtwalk.c,v 1.5 1999/06/20 06:08:15 cgd Exp $	*/

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
 *      This product includes software developed for the NetBSD Project by
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

/*
 *	walk a text buffer, processing matched lines
 *
 *	Written by Philip A. Nelson.
 *	7/29/97
 *
 */

#undef DEBUG

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

#include "txtwalk.h"

/* prototypes */

static void process __P((struct lookfor, char *));
static void match __P((char *, struct lookfor *, int));
static int finddata __P((struct lookfor, char *, struct data *, int *));
static char *strndup __P((char *, int));

/*
 * Walk the buffer, call match for each line.
 */
void
walk(buffer, size, these, numthese)
	char *buffer;
	size_t size;
	struct lookfor *these;
	int numthese;
{
	int i = 0;
	int len;
	int line = 1;

	while (i < size) {
		/* Ignore zero characters. */
		if (*buffer == '\0') {
			buffer++;
			i++;
		} else {
			/* Assume this starts a line. */
			len = 0;
			while (buffer[len] != '\n' && buffer[len] != '\0')
				len++;
			buffer[len] = '\0';
#ifdef DEBUG
			printf ("%5d: %s\n", line, buffer);
#endif
			match(buffer, these, numthese);
			buffer += len+1;
			i += len+1;
			line++;
		}
	}
}

/*
 * Match the current line with a string of interest.
 * For each match in these, process the match.
 */
static void
match(line, these, numthese)
	char *line;
	struct lookfor *these;
	int numthese;
{
	int linelen;		/* Line length */
	int patlen;		/* Pattern length */
	int which;		/* Which pattern we are using */

	linelen = strlen(line); 	

	for (which = 0; which < numthese; which++) {
		patlen = strlen(these[which].head);
		if (linelen < patlen)
			continue;
		if (strncmp(these[which].head, line, patlen) == 0)
			process(these[which], line);
	}
}


/* process the matched line. */
static void
process(this, line)
	struct lookfor this;
	char *line;
{
	struct data found[MAXDATA];
	int numfound = 0;
	char *p;
	int   i, j;
	
	if (finddata(this, line, found, &numfound)) {
#ifdef DEBUG
		printf("process: \"%s\"\n", line);
		for (i = 0; i < numfound; i++) {
			printf ("\t%d: ", i);
			switch (found[i].what) {
			case INT:
				printf ("%d\n", found[i].u.i_val);
				break;
			case STR:
				printf ("'%s'\n", found[i].u.s_val);
				break;
			}
		}
#endif
		/* Process the stuff. */
		switch (this.todo[0]) {
		case 'a':  /* Assign data */
			p = this.todo;
			j = 0;
			while (*p && *p != '$')
				p++;
			if (*p)
				p++;
			while (*p && isdigit(*p)) {
				i = atoi(p);
				switch (found[i].what) {
				case INT:
					*((int *)this.var+j)
						= found[i].u.i_val;
					break;
				case STR:
					strncpy(*((char **)this.var+j),
					        found[i].u.s_val,
						this.size-1);
					found[i].u.s_val[this.size-1] = 0;
					break;
				}
				while (isdigit(*p))
					p++;
				while (*p && *p != '$')
					p++;
				if (*p)
					p++;
				j++;
				if (j >= this.nument)
					break;
			}
			break;
		case 'c':  /* Call a function with data. */
			(*this.func)(found, numfound);
			break;
		}
	}
}

/*
 * find the expected data.  Return 1 if successful, return 0 if not.
 * Successful means running into the end of the expect string before
 * running out of line data or encountering other bad data.
 *
 * Side Effect -- sets numfound and found.
 */
static int
finddata(this, line, found, numfound )
	struct lookfor this;
	char *line;
	struct data *found;
	int *numfound;
{
	char *fmt = this.fmt;
	int len;

	*numfound = 0;
	while (*fmt) {
		if (!*line && *fmt)
			return 0;
		if (*fmt == '%') {
			fmt++;
			if (!*fmt)
				return 0;
			switch (*fmt) {
			case '%':  /* The char %. */
				if (*line != '%')
					return 0;
				line++;
				break;
			case 'i':  /* Ignore characters */
				if (!fmt[1])
					return 1;
				if (fmt[1] == ' ')
					while (*line && !isspace(*line))
						line++;
				else
					while (*line && *line != fmt[1])
						line++;
				break;
			case 'd':  /* Nextoken should be an integer. */
				if (!isdigit(*line))
					return 0;
				found[*numfound].what = INT;
				found[(*numfound)++].u.i_val = atoi(line);
				while (*line && isdigit(*line))
					line++;
				break;
			case 's':  /* Matches a 'space' separated string. */
				len = 0;
				while (line[len] && !isspace(line[len])
				    && line[len] != fmt[1])
					len++;
				found[*numfound].what = STR;
				found[*numfound].u.s_val = strndup(line, len);
				if (found[(*numfound)++].u.s_val == NULL) {
					(void)fprintf(stderr,
					    "msgwalk: strndup: out of vm.\n");
					exit(1);
				}
				line += len;
				break;
			default:
				return 0;
			}
			
		} else if (*fmt == ' ') {
			while (*line && isspace(*line))
				line++;
		} else if (*line == *fmt) {
			line++;
		} else {
			/* Mis match! */
			return 0;
		}
		fmt++;
	}
	
	/* Ran out of fmt. */
	return 1;
}

/*
 * Utility routines.... 
 */

static char *
strndup(str, len)
	char *str;
	int len;
{
	int alen;
	char *val;
	
	alen = strlen(str);
	alen = len < alen ? len + 1 : alen + 1;
	val = (char *)malloc(alen);
	if (!val)
		return NULL;
	strncpy(val, str, alen-1);
	return val;
}
