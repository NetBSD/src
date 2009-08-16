/*	$NetBSD: fsort.c,v 1.36 2009/08/16 20:02:04 dsl Exp $	*/

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
 * Read in the next bin.  If it fits in one segment sort it;
 * otherwise refine it by segment deeper by one character,
 * and try again on smaller bins.  Sort the final bin at this level
 * of recursion to keep the head of fstack at 0.
 * After PANIC passes, abort to merge sort.
 */
#include "sort.h"
#include "fsort.h"

#ifndef lint
__RCSID("$NetBSD: fsort.c,v 1.36 2009/08/16 20:02:04 dsl Exp $");
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
	int ntfiles, mfct = 0, total, i, maxb, lastb, panic = 0;
	int c, nelem, base;
	long sizes [NBINS + 1];
	get_func_t get;
	struct recheader *crec;
	struct field tfield[2];
	FILE *prevfp, *tailfp[FSORTMAX + 1];
	u_char *nbuffer;

	memset(tailfp, 0, sizeof(tailfp));
	prevfp = outfp;
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
	if (binno >= 0) {
		base = top + nfiles;
		get = getnext;
	} else {
		base = 0;
		if (SINGL_FLD)
			get = makeline;
		else
			get = makekey;
	}				
	for (;;) {
		memset(sizes, 0, sizeof(sizes));
		c = nelem = ntfiles = 0; /* XXXGCC -Wuninitialized m68k */
		if (binno == weights[REC_D] &&
		    !(SINGL_FLD && ftbl[0].flags & F)) {	/* pop */
			rd_append(weights[REC_D], top,
			    nfiles, prevfp, buffer, bufend);
			break;
		} else if (binno == weights[REC_D]) {
			depth = 0;		/* start over on flat weights */
			ftbl = tfield;
			weights = ftbl[0].weights;
		}
		keypos = keylist;	     /* XXXGCC -Wuninitialized m68k */
		crec = (RECHEADER *) buffer; /* XXXGCC -Wuninitialized m68k */
		while (c != EOF) {
			keypos = keylist;
			nelem = 0;
			crec = (RECHEADER *) buffer;

		   do_read:
			while ((c = get(binno, top, filelist, nfiles, crec,
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
			if (panic >= PANIC) {
				fstack[MSTART + mfct].fp = ftmp();
				if ((stable_sort)
					? sradixsort(keylist, nelem,
						weights, REC_D)
					: radixsort(keylist, nelem,
						weights, REC_D) )
					err(2, NULL);
				append(keylist, nelem, depth,
				    fstack[MSTART + mfct].fp, putrec,
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

					CHECKFSTACK(base + ntfiles);
					fstack[base + ntfiles].fp = ftmp();
					fmerge(0, MSTART, filelist,
					    mfct, geteasy,
					    fstack[base + ntfiles].fp,
					    putrec, ftbl);
					ntfiles++;
					mfct = 0;

					if (!nodata) {
						memmove(crec->data, tmpbuf, sz);
						free(tmpbuf);
					}
				}
			} else {
				CHECKFSTACK(base + ntfiles);
				fstack[base + ntfiles].fp = ftmp();
				onepass(keylist, depth, nelem, sizes,
				    weights, fstack[base + ntfiles].fp);
				ntfiles++;
			}
		}
		if (!ntfiles && !mfct) {	/* everything in memory--pop */
			if (nelem > 1
			    && ((stable_sort)
				? sradixsort(keylist, nelem, weights, REC_D)
				: radixsort(keylist, nelem, weights, REC_D) ))
				err(2, NULL);
			if (nelem > 0)
			  append(keylist, nelem, depth, outfp, putline, ftbl);
			break;					/* pop */
		}
		if (panic >= PANIC) {
			if (!ntfiles)
				fmerge(0, MSTART, filelist, mfct, geteasy,
				    outfp, putline, ftbl);
			else
				fmerge(0, base, filelist, ntfiles, geteasy,
				    outfp, putline, ftbl);
			break;
				
		}
		total = maxb = lastb = 0;	/* find if one bin dominates */
		for (i = 0; i < NBINS; i++)
			if (sizes[i]) {
				if (sizes[i] > sizes[maxb])
					maxb = i;
				lastb = i;
				total += sizes[i];
			}
		if (sizes[maxb] < max((total / 2) , BUFSIZE))
			maxb = lastb;	/* otherwise pop after last bin */
		fstack[base].lastb = lastb;
		fstack[base].maxb = maxb;

		/* start refining next level. */
		getnext(-1, base, NULL, ntfiles, crec, bufend, 0); /* rewind */
		for (i = 0; i < maxb; i++) {
			if (!sizes[i])	/* bin empty; step ahead file offset */
				getnext(i, base, NULL,ntfiles, crec, bufend, 0);
			else
				fsort(i, depth + 1, base, filelist, ntfiles,
					outfp, ftbl);
		}

		get = getnext;

		if (lastb != maxb) {
			if (prevfp != outfp)
				tailfp[panic] = prevfp;
			prevfp = ftmp();
			for (i = maxb + 1; i <= lastb; i++)
				if (!sizes[i])
					getnext(i, base, NULL, ntfiles, crec,
					    bufend,0);
				else
					fsort(i, depth + 1, base, filelist,
					    ntfiles, prevfp, ftbl);
		}

		/* sort biggest (or last) bin at this level */
		depth++;
		panic++;
		binno = maxb;
		top = base;
		nfiles = ntfiles;		/* so overwrite them */
	}
	if (prevfp != outfp) {
		concat(outfp, prevfp);
		fclose(prevfp);
	}
	for (i = panic; i >= 0; --i)
		if (tailfp[i]) {
			concat(outfp, tailfp[i]);
			fclose(tailfp[i]);
		}

	/* If on top level, free our structures */
	if (depth == 0) {
		free(keylist), keylist = NULL;
		free(buffer), buffer = NULL;
	}
}

/*
 * This is one pass of radix exchange, dumping the bins to disk.
 */
#define swap(a, b, t) t = a, a = b, b = t
void
onepass(const u_char **a, int depth, long n, long sizes[], u_char *tr, FILE *fp)
{
	size_t tsizes[NBINS + 1];
	const u_char **bin[257], ***bp, ***bpmax, **top[256], ***tp;
	static int histo[256];
	int *hp;
	int c;
	int hdr_off;
	const u_char **an, *t, **aj;
	const u_char **ak, *r;

	memset(tsizes, 0, sizeof(tsizes));
	hdr_off = REC_DATA_OFFSET + depth;
	an = &a[n];
	for (ak = a; ak < an; ak++) {
		histo[c = tr[**ak]]++;
		tsizes[c] += ((const RECHEADER *) (*ak -= hdr_off))->length;
	}

	bin[0] = a;
	bpmax = bin + 256;
	tp = top, hp = histo;
	for (bp = bin; bp < bpmax; bp++) {
		*tp++ = *(bp + 1) = *bp + (c = *hp);
		*hp++ = 0;
		if (c <= 1)
			continue;
	}
	for (aj = a; aj < an; *aj = r, aj = bin[c + 1]) 
		for (r = *aj; aj < (ak = --top[c = tr[r[hdr_off]]]) ;)
			swap(*ak, r, t);

	for (ak = a, c = 0; c < 256; c++) {
		an = bin[c + 1];
		n = an - ak;
		tsizes[c] += n * REC_DATA_OFFSET;
		/* tell getnext how many elements in this bin, this segment. */
		EWRITE(&tsizes[c], sizeof(size_t), 1, fp);
		sizes[c] += tsizes[c];
		for (; ak < an; ++ak)
			putrec((const RECHEADER *) *ak, fp);
	}
}
