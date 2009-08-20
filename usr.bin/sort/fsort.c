/*	$NetBSD: fsort.c,v 1.38 2009/08/20 06:36:25 dsl Exp $	*/

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
__RCSID("$NetBSD: fsort.c,v 1.38 2009/08/20 06:36:25 dsl Exp $");
__SCCSID("@(#)fsort.c	8.1 (Berkeley) 6/6/93");
#endif /* not lint */

#include <stdlib.h>
#include <string.h>

static const u_char **keylist = 0;
u_char *buffer = 0;
size_t bufsize = DEFBUFSIZE;

struct tempfile fstack[MAXFCT];

#define SALIGN(n) ((n+sizeof(length_t)-1) & ~(sizeof(length_t)-1))

void
fsort(struct filelist *filelist, int nfiles, FILE *outfp, struct field *ftbl)
{
	const u_char **keypos, **keyp;
	u_char *bufend;
	int mfct = 0;
	int c, nelem;
	get_func_t get;
	struct recheader *crec;
	u_char *nbuffer;

	if (!buffer) {
		buffer = malloc(bufsize);
		keylist = malloc(MAXNUM * sizeof(u_char *));
		memset(keylist, 0, MAXNUM * sizeof(u_char *));
	}
	bufend = buffer + bufsize;

	if (SINGL_FLD)
		/* Key and data are one! */
		get = makeline;
	else
		/* Key (merged key fields) added before data */
		get = makekey;

	/* Loop through reads of chunk of input files that get sorted
	 * and then merged together. */
	for (;;) {
		keypos = keylist;
		nelem = 0;
		crec = (RECHEADER *) buffer;

		/* Loop reading records */
		for (;;) {
			c = get(-1, 0, filelist, nfiles, crec, bufend, ftbl);
			/* 'c' is 0, EOF or BUFFEND */
			if (c == 0) {
				/* Save start of key in input buffer */
				*keypos++ = crec->data;
				if (++nelem == MAXNUM) {
					c = BUFFEND;
					break;
				}
				crec = (RECHEADER *)((char *) crec +
				    SALIGN(crec->length) + REC_DATA_OFFSET);
				continue;
			}
			if (c == EOF)
				break;
			if (nelem >= MAXNUM || bufsize >= MAXBUFSIZE)
				/* Need to sort and save this lot of data */
				break;

			/* c == BUFFEND, and we can process more data */
			/* Allocate a larger buffer for this lot of data */
			bufsize *= 2;
			nbuffer = realloc(buffer, bufsize);
			if (!nbuffer) {
				err(2, "failed to realloc buffer to %zu bytes",
					bufsize);
			}

			/* patch up keylist[] */
			for (keyp = &keypos[-1]; keyp >= keylist; keyp--)
				*keyp = nbuffer + (*keyp - buffer);

			crec = (RECHEADER *) (nbuffer + ((u_char *)crec - buffer));
			buffer = nbuffer;
			bufend = buffer + bufsize;
		}

		/* Sort this set of records */
		if (radix_sort(keylist, nelem, ftbl[0].weights, REC_D))
			err(2, NULL);

		if (c == EOF && mfct == 0) {
			/* all the data is (sorted) in the buffer */
			append(keylist, nelem, outfp, putline, ftbl);
			break;
		}

		/* Save current data to a temporary file for a later merge */
		fstack[mfct].fp = ftmp();
		append(keylist, nelem, fstack[mfct].fp, putrec, ftbl);
		mfct++;

		if (c == EOF) {
			/* merge to output file */
			fmerge(0, filelist, mfct, geteasy, outfp, putline,
			    ftbl);
			break;
		}

		if (mfct == MERGE_FNUM) {
			/* Merge the files we have */
			FILE *fp = ftmp();
			fmerge(0, filelist, mfct, geteasy, fp, putrec, ftbl);
			mfct = 1;
			fstack[0].fp = fp;
		}
	}

	free(keylist);
	keylist = NULL;
	free(buffer);
	buffer = NULL;
}
