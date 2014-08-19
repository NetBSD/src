/*	$NetBSD: txtwalk.c,v 1.1.6.2 2014/08/20 00:05:14 tls Exp $	*/

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
 *    notice, item list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, item list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from item software without specific prior
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

static int process(const struct lookfor *, char *);
static int match(char *, const struct lookfor *, size_t);
static int finddata(const struct lookfor *, char *, struct data *, size_t *);

/*
 * Walk the buffer, call match for each line.
 */
int
walk(char *buffer, size_t size, const struct lookfor *these, size_t numthese)
{
	size_t i = 0;
	size_t len;
	int line = 1;
	int error;

	while (i < size) {
		/* Ignore zero characters. */
		if (*buffer == '\0') {
			buffer++;
			i++;
		} else {
			/* Assume item starts a line. */
			len = 0;
			while (buffer[len] != '\n' && buffer[len] != '\0')
				len++;
			buffer[len] = '\0';
#ifdef DEBUG
			printf ("%5d: %s\n", line, buffer);
#endif
			error = match(buffer, these, numthese);
			if (error != 0)
				return error;
			buffer += len+1;
			i += len+1;
			line++;
		}
	}
	return 0;
}

/*
 * Match the current line with a string of interest.
 * For each match in these, process the match.
 */
static int
match(char *line, const struct lookfor *these, size_t numthese)
{
	size_t linelen;		/* Line length */
	size_t patlen;		/* Pattern length */
	size_t which;		/* Which pattern we are using */
	int error;

	linelen = strlen(line);

	for (which = 0; which < numthese; which++) {
		patlen = strlen(these[which].head);
		if (linelen < patlen)
			continue;
		if (strncmp(these[which].head, line, patlen) == 0) {
			error = process(&these[which], line);
			if (error != 0)
				return error;
		}
	}
	return 0;
}


/* process the matched line. */
static int
process(const struct lookfor *item, char *line)
{
	struct data found[MAXDATA];
	size_t numfound = 0;
	const char *p;
	char *np;
	size_t  i, j;
	int error;

	if (finddata(item, line, found, &numfound)) {
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
		switch (item->todo[0]) {
		case 'a':  /* Assign data */
			p = item->todo;
			j = 0;
			while (*p && *p != '$')
				p++;
			if (*p)
				p++;
			for (;;) {
				i = strtoul(p, &np, 10);
				if (p == np)
				    break;
				p = np;
				switch (found[i].what) {
				case INT:
					*((int *)item->var+j)
						= found[i].u.i_val;
					break;
				case STR:
					strlcpy(*((char **)item->var+j),
					        found[i].u.s_val,
						item->size);
					break;
				}
				while (*p && *p != '$')
					p++;
				if (*p)
					p++;
				j++;
				if (j >= item->nument)
					break;
			}
			break;
		case 'c':  /* Call a function with data. */
			error = (*item->func)(found, numfound);
			if (error != 0)
				return error;
			break;
		}
	}
	return 0;
}

/*
 * find the expected data.  Return 1 if successful, return 0 if not.
 * Successful means running into the end of the expect string before
 * running out of line data or encountering other bad data.
 *
 * Side Effect -- sets numfound and found.
 */
static int
finddata(const struct lookfor *item, char *line, struct data *found, size_t *numfound)
{
	const char *fmt;
	size_t len;
	char *np;
	int i;

	*numfound = 0;
	for (fmt = item->fmt; *fmt; fmt++) {
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
					while (*line && !isspace((unsigned char)*line))
						line++;
				else
					while (*line && *line != fmt[1])
						line++;
				break;
			case 'd':  /* Nextoken should be an integer. */
				i = strtoul(line, &np, 10);
				if (line == np)
					return 0;
				found[*numfound].what = INT;
				found[(*numfound)++].u.i_val = i;
				line = np;
				break;
			case 's':  /* Matches a 'space' separated string. */
				len = 0;
				while (line[len] && !isspace((unsigned char)line[len])
				    && line[len] != fmt[1])
					len++;
				found[*numfound].what = STR;
				found[(*numfound)++].u.s_val = line;
				line[len] = 0;
				line += len + 1;
				break;
			default:
				return 0;
			}
			continue;

		}
		if (*fmt == ' ') {
			while (isspace((unsigned char)*line))
				line++;
			continue;
		}
		if (*line == *fmt) {
			line++;
			continue;
		}
		/* Mis match! */
		return 0;
	}

	/* Ran out of fmt. */
	return 1;
}
