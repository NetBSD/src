/* buf.c: This file contains the scratch-file buffer rountines for the
   ed line editor. */
/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rodney Ruddock of the University of Guelph.
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
 */

#ifndef lint
static char sccsid[] = "@(#)buf.c	5.5 (Berkeley) 3/28/93";
#endif /* not lint */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include "ed.h"

extern char errmsg[];
extern line_t line0;

FILE *sfp;				/* scratch file pointer */
off_t sfseek;				/* scratch file position */
int seek_write;				/* seek before writing */

/* gettxt: get a line of text from the scratch file; return pointer
   to the text */
char *
gettxt(lp)
	line_t *lp;
{
	static char txtbuf[MAXLINE];
	int len, ct;

	if (lp == &line0)
		return NULL;
	seek_write = 1;				/* force seek on write */
	/* out of position */
	if (sfseek != lp->seek) {
		sfseek = lp->seek;
		if (fseek(sfp, sfseek, SEEK_SET) < 0) {
			sprintf(errmsg, "cannot seek temp file");
			return (char *) ERR;
		}
	}
	len = lp->len & ~ACTV;
	if ((ct = fread(txtbuf, sizeof(char), len, sfp)) <  0 || ct != len) {
		sprintf(errmsg, "cannot read temp file");
		return (char *) ERR;
	}
	sfseek += len;				/* update file position */
	txtbuf[len] = '\0';
	return txtbuf;
}


extern long curln;
extern long lastln;

/* puttxt: write a line of text to the scratch file and add a line node
   to the editor buffer;  return a pointer to the end of the text */
char *
puttxt(cs)
	char *cs;
{
	line_t *lp;
	int len, ct;
	char *s;

	if ((lp = (line_t *) malloc(sizeof(line_t))) == NULL) {
		sprintf(errmsg, "out of memory");
		return (char *) ERR;
	}
	/* assert: cs is '\n' terminated */
	for (s = cs; *s != '\n'; s++)
		;
	len = (s - cs) & ~ACTV;
	/* out of position */
	if (seek_write) {
		if (fseek(sfp, 0L, SEEK_END) < 0) {
			sprintf(errmsg, "cannot seek temp file");
			return (char *) ERR;
		}
		sfseek = ftell(sfp);
		seek_write = 0;
	}
	/* assert: spl1() */
	if ((ct = fwrite(cs, sizeof(char), len, sfp)) < 0 || ct != len) {
		sfseek = -1;
		sprintf(errmsg, "cannot write temp file");
		return (char *) ERR;
	}
	lp->len = len;
	lp->seek  = sfseek;
	lpqueue(lp);
	sfseek += len;			/* update file position */
	return (*++s == '\0') ? NULL : s;
}


/* lpqueue: add a line node in the editor buffer after the current line */
void
lpqueue(lp)
	line_t *lp;
{
	line_t *cp;

	cp = getptr(curln);				/* this getptr last! */
	insqueue(lp, cp);
	lastln++;
	curln++;
}


extern int mutex;
extern int sigflags;

/* getptr: return pointer to a line node in the editor buffer */
line_t *
getptr(n)
	long n;
{
	static line_t *lp = &line0;
	static long on = 0;

	spl1();
	if (n > on)
		if (n <= (on + lastln) >> 1)
			for (; on < n; on++)
				lp = lp->next;
		else {
			lp = line0.prev;
			for (on = lastln; on > n; on--)
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
	spl0();
	return lp;
}


char sfn[15] = "";				/* scratch file name */

/* sbopen: open scratch file */
sbopen()
{
	strcpy(sfn, "/tmp/ed.XXXXXX");
	if (mktemp(sfn) == NULL || (sfp = fopen(sfn, "w+")) == NULL) {
		sprintf(errmsg, "cannot open temp file");
		return ERR;
	}
	return 0;
}


/* sbclose: close scratch file */
sbclose()
{
	if (sfp) {
		fclose(sfp);
		sfp = NULL;
		unlink(sfn);
	}
	sfseek = seek_write = 0;
}


/* quit: remove scratch file and exit */
void
quit(n)
	int n;
{
	if (sfp) {
		unlink(sfn);
		fclose(sfp);
	}
	exit(n);
}
