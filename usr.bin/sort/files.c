/*	$NetBSD: files.c,v 1.31 2009/08/15 16:50:29 dsl Exp $	*/

/*-
 * Copyright (c) 2000-2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ben Harris and Jaromir Dolecek.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Peter McIlroy.
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

#include "sort.h"
#include "fsort.h"

#ifndef lint
__RCSID("$NetBSD: files.c,v 1.31 2009/08/15 16:50:29 dsl Exp $");
__SCCSID("@(#)files.c	8.1 (Berkeley) 6/6/93");
#endif /* not lint */

#include <string.h>

static int	seq(FILE *, DBT *);

/*
 * this is the subroutine for file management for fsort().
 * It keeps the buffers for all temporary files.
 */
int
getnext(int binno, int infl0, struct filelist *filelist, int nfiles,
    RECHEADER *pos, u_char *end, struct field *dummy)
{
	int i;
	u_char *hp;
	static size_t nleft = 0;
	static int cnt = 0, flag = -1;
	static u_char maxb = 0;
	static FILE *fp;

	if (nleft == 0) {
		if (binno < 0)	/* reset files. */ {
			for (i = 0; i < nfiles; i++) {
				rewind(fstack[infl0 + i].fp);
				fstack[infl0 + i].max_o = 0;
			}
			flag = -1;
			nleft = cnt = 0;
			return (-1);
		}
		maxb = fstack[infl0].maxb;
		for (; nleft == 0; cnt++) {
			if (cnt >= nfiles) {
				cnt = 0;
				return (EOF);
			}
			fp = fstack[infl0 + cnt].fp;
			fread(&nleft, sizeof(nleft), 1, fp);
			if (binno < maxb)
				fstack[infl0+cnt].max_o
					+= sizeof(nleft) + nleft;
			else if (binno == maxb) {
				if (binno != fstack[infl0].lastb) {
					fseeko(fp, fstack[infl0+
						cnt].max_o, SEEK_SET);
					fread(&nleft, sizeof(nleft), 1, fp);
				}
				if (nleft == 0)
					fclose(fp);
			} else if (binno == maxb + 1) {		/* skip a bin */
				fseek(fp, nleft, SEEK_CUR);
				fread(&nleft, sizeof(nleft), 1, fp);
				flag = cnt;
			}
		}
	}
	if ((u_char *) pos > end - sizeof(TRECHEADER))
		return (BUFFEND);
	fread(pos, sizeof(TRECHEADER), 1, fp);
	if (end - pos->data < (ptrdiff_t)pos->length) {
		hp = ((u_char *)pos) + sizeof(TRECHEADER);
		for (i = sizeof(TRECHEADER); i ;  i--)
			ungetc(*--hp, fp);
		return (BUFFEND);
	}
	fread(pos->data, pos->length, 1, fp);
	nleft -= pos->length + sizeof(TRECHEADER);
	if (nleft == 0 && binno == fstack[infl0].maxb)
		fclose(fp);
	return (0);
}

/*
 * this is called when there is no special key. It's only called
 * in the first fsort pass.
 */
int
makeline(int flno, int top, struct filelist *filelist, int nfiles,
    RECHEADER *recbuf, u_char *bufend, struct field *dummy2)
{
	static u_char *obufend;
	static size_t osz;
	u_char *pos;
	static int filenum = 0, overflow = 0;
	static FILE *fp = 0;
	int c;

	c = 0;		/* XXXGCC -Wuninitialized [pmppc] */

	pos = recbuf->data;
	if (overflow) {
		/*
		 * Buffer shortage is solved by either of two ways:
		 * o flush previous buffered data and start using the
		 *   buffer from start (see fsort())
		 * o realloc buffer and bump bufend
		 * 
		 * The former is preferred, realloc is only done when
		 * there is exactly one item in buffer which does not fit. 
		 */
		if (bufend == obufend)
			memmove(pos, bufend - osz, osz);

		pos += osz;
		overflow = 0;
	}
	for (;;) {
		if (flno >= 0 && (fp = fstack[flno].fp) == NULL)
			return (EOF);
		else if (fp == NULL) {
			if (filenum  >= nfiles)
				return (EOF);
			if (!(fp = fopen(filelist->names[filenum], "r")))
				err(2, "%s", filelist->names[filenum]);
			filenum++;
		}
		while ((pos < bufend) && ((c = getc(fp)) != EOF)) {
			*pos++ = c;
			if (c == REC_D) {
				recbuf->offset = 0;
				recbuf->length = pos - recbuf->data;
				return (0);
			}
		}
		if (pos >= bufend) {
			if (recbuf->data < bufend) {
				overflow = 1;
				obufend = bufend;
				osz = (pos - recbuf->data);
			}
			return (BUFFEND);
		} else if (c == EOF) {
			if (recbuf->data != pos) {
				*pos++ = REC_D;
				recbuf->offset = 0;
				recbuf->length = pos - recbuf->data;
				return (0);
			}
			FCLOSE(fp);
			fp = 0;
			if (flno >= 0)
				fstack[flno].fp = 0;
		} else {
			
			warnx("makeline: line too long: ignoring '%.100s...'", recbuf->data);

			/* Consume the rest of line from input */
			while ((c = getc(fp)) != REC_D && c != EOF)
				;

			recbuf->offset = 0;
			recbuf->length = 0;

			return (BUFFEND);
		}
	}
}

/*
 * This generates keys. It's only called in the first fsort pass
 */
int
makekey(int flno, int top, struct filelist *filelist, int nfiles,
    RECHEADER *recbuf, u_char *bufend, struct field *ftbl)
{
	static int filenum = 0;
	static FILE *dbdesc = 0;
	static DBT line[1];
	static int overflow = 0;
	int c;

	if (overflow) {
		overflow = enterkey(recbuf, line, bufend - (u_char *)recbuf,
									ftbl);
		if (overflow)
			return (BUFFEND);
		else
			return (0);
	}

	for (;;) {
		if (flno >= 0) {
			if (!(dbdesc = fstack[flno].fp))
				return (EOF);
		} else if (!dbdesc) {
			if (filenum  >= nfiles)
				return (EOF);
			dbdesc = fopen(filelist->names[filenum], "r");
			if (!dbdesc)
				err(2, "%s", filelist->names[filenum]);
			filenum++;
		}
		if (!(c = seq(dbdesc, line))) {
			if ((signed)line->size > bufend - recbuf->data) {
				overflow = 1;
			} else {
				overflow = enterkey(recbuf, line,
				    bufend - (u_char *) recbuf, ftbl);
			}
			if (overflow)
				return (BUFFEND);
			else
				return (0);
		}
		if (c == EOF) {
			FCLOSE(dbdesc);
			dbdesc = 0;
			if (flno >= 0)
				fstack[flno].fp = 0;
		} else {
			warnx("makekey: line too long: ignoring %.60s...",
			    (char *)line->data);
		}
	}
}

/*
 * get a line pair from fp
 */
static int
seq(FILE *fp, DBT *line)
{
	static u_char *buf;
	static size_t buf_size = DEFLLEN;
	u_char *end, *pos;
	int c;
	u_char *new_buf;

	if (!buf) {
		/* one-time initialization */
		buf = malloc(buf_size);
		if (!buf)
		    err(2, "malloc of linebuf for %zu bytes failed",
			    buf_size);
		line->data = buf;
	}

	end = buf + buf_size;
	pos = buf;
	while ((c = getc(fp)) != EOF) {
		if ((*pos++ = c) == REC_D) {
			line->size = pos - buf;
			return (0);
		}
		if (pos == end) {
			/* Long line - double size of buffer */
			buf_size *= 2;
			new_buf = realloc(buf, buf_size);
			if (!new_buf)
				err(2, "realloc of linebuf to %zu bytes failed",
					buf_size);
		
			end = new_buf + buf_size;
			pos = new_buf + (pos - buf);
			buf = new_buf;
			line->data = buf;
		}
	}

	if (pos != buf) {
		/* EOF part way through line - add line terminator */
		*pos++ = REC_D;
		line->size = pos - buf;
		return (0);
	}

	return (EOF);
}

/*
 * write a key/line pair to a temporary file
 */
void
putrec(const RECHEADER *rec, FILE *fp)
{
	EWRITE(rec, 1, rec->length + sizeof(TRECHEADER), fp);
}

/*
 * write a line to output
 */
void
putline(const RECHEADER *rec, FILE *fp)
{
	EWRITE(rec->data+rec->offset, 1, rec->length - rec->offset, fp);
}

/*
 * get a record from a temporary file. (Used by merge sort.)
 */
int
geteasy(int flno, int top, struct filelist *filelist, int nfiles,
    RECHEADER *rec, u_char *end, struct field *dummy2)
{
	int i;
	FILE *fp;

	fp = fstack[flno].fp;
	if ((u_char *) rec > end - sizeof(TRECHEADER))
		return (BUFFEND);
	if (!fread(rec, 1, sizeof(TRECHEADER), fp)) {
		fclose(fp);
		fstack[flno].fp = 0;
		return (EOF);
	}
	if (end - rec->data < (ptrdiff_t)rec->length) {
		for (i = sizeof(TRECHEADER) - 1; i >= 0;  i--)
			ungetc(*((char *) rec + i), fp);
		return (BUFFEND);
	}
	fread(rec->data, rec->length, 1, fp);
	return (0);
}
