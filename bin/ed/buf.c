/* buf.c: This file contains the scratch-file buffer rountines for the
   ed line editor. */
/*-
 * Copyright (c) 1993 Andrew Moore, Talke Studio.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef lint
/* static char sccsid[] = "@(#)buf.c	5.5 (Talke Studio) 3/28/93"; */
static char rcsid[] = "$Id: buf.c,v 1.10 1993/11/23 04:41:48 alm Exp $";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include "ed.h"

extern char errmsg[];

FILE *sfp;				/* scratch file pointer */
char *sfbuf = NULL;			/* scratch file input buffer */
int sfbufsz = 0;			/* scratch file input buffer size */
off_t sfseek;				/* scratch file position */
int seek_write;				/* seek before writing */
line_t line0;				/* initial node of line queue */

/* get_sbuf_line: get a line of text from the scratch file; return pointer
   to the text */
char *
get_sbuf_line(lp)
	line_t *lp;
{
	int len, ct;

	if (lp == &line0)
		return NULL;
	seek_write = 1;				/* force seek on write */
	/* out of position */
	if (sfseek != lp->seek) {
		sfseek = lp->seek;
		if (fseek(sfp, sfseek, SEEK_SET) < 0) {
			fprintf(stderr, "%s\n", strerror(errno));
			sprintf(errmsg, "cannot seek temp file");
			return NULL;
		}
	}
	len = lp->len;
	CKBUF(sfbuf, sfbufsz, len + 1, NULL);
	if ((ct = fread(sfbuf, sizeof(char), len, sfp)) <  0 || ct != len) {
		fprintf(stderr, "%s\n", strerror(errno));
		sprintf(errmsg, "cannot read temp file");
		return NULL;
	}
	sfseek += len;				/* update file position */
	sfbuf[len] = '\0';
	return sfbuf;
}


extern long current_addr;
extern long addr_last;

/* put_sbuf_line: write a line of text to the scratch file and add a line node
   to the editor buffer;  return a pointer to the end of the text */
char *
put_sbuf_line(cs)
	char *cs;
{
	line_t *lp;
	int len, ct;
	char *s;

	if ((lp = (line_t *) malloc(sizeof(line_t))) == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		sprintf(errmsg, "out of memory");
		return NULL;
	}
	/* assert: cs is '\n' terminated */
	for (s = cs; *s != '\n'; s++)
		;
	if (s - cs >= LINECHARS) {
		sprintf(errmsg, "line too long");
		return NULL;
	}
	len = s - cs;
	/* out of position */
	if (seek_write) {
		if (fseek(sfp, 0L, SEEK_END) < 0) {
			fprintf(stderr, "%s\n", strerror(errno));
			sprintf(errmsg, "cannot seek temp file");
			return NULL;
		}
		sfseek = ftell(sfp);
		seek_write = 0;
	}
	/* assert: SPL1() */
	if ((ct = fwrite(cs, sizeof(char), len, sfp)) < 0 || ct != len) {
		sfseek = -1;
		fprintf(stderr, "%s\n", strerror(errno));
		sprintf(errmsg, "cannot write temp file");
		return NULL;
	}
	lp->len = len;
	lp->seek  = sfseek;
	add_line_node(lp);
	sfseek += len;			/* update file position */
	return ++s;
}


/* add_line_node: add a line node in the editor buffer after the current line */
void
add_line_node(lp)
	line_t *lp;
{
	line_t *cp;

	cp = get_addressed_line_node(current_addr);				/* this get_addressed_line_node last! */
	insqueue(lp, cp);
	addr_last++;
	current_addr++;
}


/* get_line_node_addr: return line number of pointer */
long
get_line_node_addr(lp)
	line_t *lp;
{
	line_t *cp = &line0;
	long n = 0;

	while (cp != lp && (cp = cp->next) != &line0)
		n++;
	if (n && cp == &line0) {
		sprintf(errmsg, "invalid address");
		return ERR;
	 }
	 return n;
}


/* get_addressed_line_node: return pointer to a line node in the editor buffer */
line_t *
get_addressed_line_node(n)
	long n;
{
	static line_t *lp = &line0;
	static long on = 0;

	SPL1();
	if (n > on)
		if (n <= (on + addr_last) >> 1)
			for (; on < n; on++)
				lp = lp->next;
		else {
			lp = line0.prev;
			for (on = addr_last; on > n; on--)
				lp = lp->prev;
		}
	else
		if (n >= on >> 1)
			for (; on > n; on--)
				lp = lp->prev;
		else {
			lp = &line0;
			for (on = 0; on < n; on++)
				lp = lp->next;
		}
	SPL0();
	return lp;
}


char sfn[15] = "";				/* scratch file name */

/* open_sbuf: open scratch file */
int
open_sbuf()
{
	strcpy(sfn, "/tmp/ed.XXXXXX");
	if (mktemp(sfn) == NULL || (sfp = fopen(sfn, "w+")) == NULL) {
		fprintf(stderr, "%s: %s\n", sfn, strerror(errno));
		sprintf(errmsg, "cannot open temp file");
		return ERR;
	}
	return 0;
}


/* close_sbuf: close scratch file */
int
close_sbuf()
{
	if (sfp) {
		if (fclose(sfp) < 0) {
			fprintf(stderr, "%s: %s\n", sfn, strerror(errno));
			sprintf(errmsg, "cannot close temp file");
			return ERR;
		}
		sfp = NULL;
		unlink(sfn);
	}
	sfseek = seek_write = 0;
	return 0;
}


/* quit: remove_lines scratch file and exit */
void
quit(n)
	int n;
{
	if (sfp) {
		fclose(sfp);
		unlink(sfn);
	}
	exit(n);
}


unsigned char ctab[256];		/* character translation table */

/* init_buffers: open scratch buffer; initialize line queue */
void
init_buffers()
{
	int i = 0;

	if (open_sbuf() < 0)
		quit(2);
	requeue(&line0, &line0);
	for (i = 0; i < 256; i++)
		ctab[i] = i;
}


/* translit_text: translate characters in a string */
char *
translit_text(s, len, from, to)
	char *s;
	int len;
	int from;
	int to;
{
	static int i = 0;

	unsigned char *us;

	ctab[i] = i;			/* restore table to initial state */
	ctab[i = from] = to;
	for (us = (unsigned char *) s; len-- > 0; us++)
		*us = ctab[*us];
	return s;
}
