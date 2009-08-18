/*	$NetBSD: fsort.c,v 1.37 2009/08/18 18:00:28 dsl Exp $	*/

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

/*
 * Read in a block of records (until 'enough').
 * sort, write to temp file.
 * Merge sort temp files into output file
 * Small files miss out the temp file stage.
 * Large files might get multiple merges.
 */
#include "sort.h"
#include "fsort.h"

#ifndef lint
__RCSID("$NetBSD: fsort.c,v 1.37 2009/08/18 18:00:28 dsl Exp $");
__SCCSID("@(#)fsort.c	8.1 (Berkeley) 6/6/93");
#endif /* not lint */

#include <stdlib.h>
#include <string.h>

static const u_char **keylist = 0;
u_char *buffer = 0;
size_t bufsize = DEFBUFSIZE;
#define FSORTMAX 4
int PANIC = FSORTMAX;

struct tempfile fstack[MAXFCT];
#define MSTART		(MAXFCT - MERGE_FNUM)
#define	CHECKFSTACK(n)					\
	if (n >= MAXFCT)				\
		errx(2, "fstack: too many temporary files; use -H or sort in pieces")
	
#define SALIGN(n) ((n+sizeof(length_t)-1) & ~(sizeof(length_t)-1))

void
fsort(int binno, int depth, int top, struct filelist *filelist, int nfiles,
    FILE *outfp, struct field *ftbl)
{
	const u_char **keypos;
	u_char *bufend;
	u_char *weights;
	int ntfiles, mfct = 0;
	int c, nelem;
	get_func_t get;
	struct recheader *crec;
	struct field tfield[2];
	u_char *nbuffer;

	memset(tfield, 0, sizeof(tfield));
	if (ftbl[0].flags & R)
		tfield[0].weights = Rascii;
	else
		tfield[0].weights = ascii;
	tfield[0].icol.num = 1;
	weights = ftbl[0].weights;
	if (!buffer) {
		buffer = malloc(bufsize);
		keylist = malloc(MAXNUM * sizeof(u_char *));
		memset(keylist, 0, MAXNUM * sizeof(u_char *));
	}
	bufend = buffer + bufsize;
	if (SINGL_FLD)
		get = makeline;
	else
		get = makekey;

	c = nelem = ntfiles = 0; /* XXXGCC -Wuninitialized m68k */
	keypos = keylist;	     /* XXXGCC -Wuninitialized m68k */
	crec = (RECHEADER *) buffer; /* XXXGCC -Wuninitialized m68k */
	while (c != EOF) {
		keypos = keylist;
		nelem = 0;
		crec = (RECHEADER *) buffer;

	   do_read:
		while ((c = get(-1, top, filelist, nfiles, crec,
		    bufend, ftbl)) == 0) {
			*keypos++ = crec->data + depth;
			if (++nelem == MAXNUM) {
				c = BUFFEND;
				break;
			}
			crec =(RECHEADER *)((char *) crec +
			    SALIGN(crec->length) + REC_DATA_OFFSET);
		}

		if (c == BUFFEND && nelem < MAXNUM
		    && bufsize < MAXBUFSIZE) {
			const u_char **keyp;
			u_char *oldb = buffer;

			/* buffer was too small for data, allocate
			 * bigger buffer */
			nbuffer = realloc(buffer, bufsize * 2);
			if (!nbuffer) {
				err(2, "failed to realloc buffer to %lu bytes",
					(unsigned long) bufsize * 2);
			}
			buffer = nbuffer;
			bufsize *= 2;
			bufend = buffer + bufsize;

			/* patch up keylist[] */
			for (keyp = &keypos[-1]; keyp >= keylist; keyp--)
				*keyp = buffer + (*keyp - oldb);

			crec = (RECHEADER *) (buffer + ((u_char *)crec - oldb));
			goto do_read;
		}

		if (c != BUFFEND && !ntfiles && !mfct) {
			/* do not push */
			continue;
		}

		/* push */
		fstack[MSTART + mfct].fp = ftmp();
		if (radix_sort(keylist, nelem, weights, REC_D))
			err(2, NULL);
		append(keylist, nelem, depth, fstack[MSTART + mfct].fp, putrec,
		    ftbl);
		mfct++;
		/* reduce number of open files */
		if (mfct == MERGE_FNUM ||(c == EOF && ntfiles)) {
			/*
			 * Only copy extra incomplete crec
			 * data if there are any.
			 */
			int nodata = (bufend >= (u_char *)crec
			    && bufend <= crec->data);
			size_t sz=0;
			u_char *tmpbuf=NULL;

			if (!nodata) {
				sz = bufend - crec->data;
				tmpbuf = malloc(sz);
				memmove(tmpbuf, crec->data, sz);
			}

			CHECKFSTACK(ntfiles);
			fstack[ntfiles].fp = ftmp();
			fmerge(0, MSTART, filelist, mfct, geteasy,
			    fstack[ntfiles].fp, putrec, ftbl);
			ntfiles++;
			mfct = 0;

			if (!nodata) {
				memmove(crec->data, tmpbuf, sz);
				free(tmpbuf);
			}
		}
	}

	if (!ntfiles && !mfct) {	/* everything in memory--pop */
		if (nelem > 1 && radix_sort(keylist, nelem, weights, REC_D))
			err(2, NULL);
		if (nelem > 0)
			append(keylist, nelem, depth, outfp, putline, ftbl);
	}
	if (!ntfiles)
		fmerge(0, MSTART, filelist, mfct, geteasy,
		    outfp, putline, ftbl);
	else
		fmerge(0, 0, filelist, ntfiles, geteasy,
		    outfp, putline, ftbl);

	free(keylist);
	keylist = NULL;
	free(buffer);
	buffer = NULL;
}
