/*	$NetBSD: msort.c,v 1.23 2009/08/22 10:53:28 dsl Exp $	*/

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
__RCSID("$NetBSD: msort.c,v 1.23 2009/08/22 10:53:28 dsl Exp $");
__SCCSID("@(#)msort.c	8.1 (Berkeley) 6/6/93");
#endif /* not lint */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Subroutines using comparisons: merge sort and check order */
#define DELETE (1)

typedef struct mfile {
	u_char *end;
	short flno;
	struct recheader rec[1];
} MFILE;

static u_char *wts;

static int cmp(RECHEADER *, RECHEADER *);
static int insert(struct mfile **, struct mfile **, int, int);
static void merge(int, int, get_func_t, FILE *, put_func_t, struct field *);

void
fmerge(int binno, struct filelist *filelist, int nfiles,
    get_func_t get, FILE *outfp, put_func_t fput, struct field *ftbl)
{
	FILE *tout;
	int i, j, last;
	put_func_t put;

	wts = ftbl->weights;

	while (nfiles) {
		put = putrec;
		for (j = 0; j < nfiles; j += MERGE_FNUM) {
			if (nfiles <= MERGE_FNUM) {
				tout = outfp;
				put = fput;
			}
			else
				tout = ftmp();
			last = min(MERGE_FNUM, nfiles - j);
			if (binno < 0) {
				for (i = 0; i < last; i++)
					if (!(fstack[i+MAXFCT-1-MERGE_FNUM].fp =
					    fopen(filelist->names[j+i], "r")))
						err(2, "%s",
							filelist->names[j+i]);
				merge(MAXFCT-1-MERGE_FNUM, last, get, tout, put, ftbl);
			} else {
				for (i = 0; i< last; i++)
					rewind(fstack[i+j].fp);
				merge(j, last, get, tout, put, ftbl);
			}
			if (nfiles > MERGE_FNUM)
				fstack[j/MERGE_FNUM].fp = tout;
		}
		nfiles = (nfiles + (MERGE_FNUM - 1)) / MERGE_FNUM;
		if (nfiles == 1)
			nfiles = 0;
		if (binno < 0) {
			binno = 0;
			get = geteasy;
		}
	}
}

static void
merge(int infl0, int nfiles, get_func_t get, FILE *outfp, put_func_t put,
    struct field *ftbl)
{
	int c, i, j, nf = nfiles;
	struct mfile *flistb[MERGE_FNUM], **flist = flistb, *cfile;
	size_t availsz = bufsize;
	static void *bufs[MERGE_FNUM + 1];
	static size_t bufs_sz[MERGE_FNUM + 1];

	/*
	 * We need nfiles + 1 buffers.
	 */
	for (i = 0; i < nfiles + 1; i++) {
		if (bufs[i])
			continue;

		bufs[i] = malloc(DEFLLEN);
		if (!bufs[i])
			err(2, "merge: malloc");
		memset(bufs[i], 0, DEFLLEN);
		bufs_sz[i] = DEFLLEN;
	}

	/* Read one record from each file (read again if a duplicate) */
	for (i = j = 0; i < nfiles; i++, j++) {
		cfile = (struct mfile *) bufs[j];
		cfile->flno = infl0 + j;
		cfile->end = (u_char *) bufs[j] + bufs_sz[j];
		for (c = 1; c == 1;) {
			c = get(cfile->flno, 0, NULL, nfiles, cfile->rec,
			    cfile->end, ftbl);
			if (c == EOF) {
				--i;
				--nfiles;
				break;
			}

			if (c == BUFFEND) {
				bufs_sz[j] *= 2;
				cfile = realloc(bufs[j], bufs_sz[j]);
				if (!cfile)
					err(2, "merge: realloc");

				bufs[j] = (void *) cfile;
				cfile->end = (u_char *)cfile + bufs_sz[j];

				c = 1;
				continue;
			}

			if (i)
				c = insert(flist, &cfile, i, !DELETE);
			else
				flist[0] = cfile;
		}
	}

	cfile = (struct mfile *) bufs[nf];
	cfile->flno = flist[0]->flno;
	cfile->end = (u_char *) cfile + bufs_sz[nf];
	while (nfiles) {
		for (c = 1; c == 1;) {
			c = get(cfile->flno, 0, NULL, nfiles, cfile->rec,
			    cfile->end, ftbl);
			if (c == EOF) {
				put(flist[0]->rec, outfp);
				if (--nfiles > 0) {
					flist++;
					cfile->flno = flist[0]->flno;
				}
				break;
			}
			if (c == BUFFEND) {
				char *oldbuf = (char *) cfile;
				availsz = (char *) cfile->end - oldbuf;
				availsz *= 2;
				cfile = realloc(oldbuf, availsz);
				if (!cfile)
					err(2, "merge: realloc");

				for (i = 0; i < nf + 1; i++) {
					if (bufs[i] == oldbuf) {
						bufs[i] = (char *)cfile;
						bufs_sz[i] = availsz;
						break;
					}
				}

				cfile->end = (u_char *)cfile + availsz;
				c = 1;
				continue;
			}
				
			c = insert(flist, &cfile, nfiles, DELETE);
			if (c == 0)
				put(cfile->rec, outfp);
		}
	}	
}

/*
 * if delete: inserts *rec in flist, deletes flist[0], and leaves it in *rec;
 * otherwise just inserts *rec in flist.
 */
static int
insert(struct mfile **flist, struct mfile **rec, int ttop, int delete)
	/* delete, ttop:			 delete = 0 or 1 */
{
	struct mfile *tmprec = *rec;
	int mid, top = ttop, bot = 0, cmpv = 1;

	for (mid = top / 2; bot + 1 != top; mid = (bot + top) / 2) {
		cmpv = cmp(tmprec->rec, flist[mid]->rec);
		if (cmpv < 0)
			top = mid;
		else if (cmpv > 0)
			bot = mid;
		else {
			if (UNIQUE)
				break;

			/*
			 * Apply sort by fileno, to give priority
			 * to earlier specified files, hence providing
			 * more stable sort.
			 * If fileno is same, the new record should
			 * be put _after_ the previous entry.
			 */
			/* XXX (dsl) this doesn't seem right */
			cmpv = tmprec->flno - flist[mid]->flno;
			if (cmpv >= 0)
				bot = mid;
			else
				bot = mid - 1;

			break;
		}
	}

	if (delete) {
		if (UNIQUE) {
			if (!bot && cmpv)
				cmpv = cmp(tmprec->rec, flist[0]->rec);
			if (!cmpv)
				return (1);
		}
		tmprec = flist[0];
		if (bot)
			memmove(flist, flist + 1, bot * sizeof(MFILE **));
		flist[bot] = *rec;
		*rec = tmprec;
		(*rec)->flno = flist[0]->flno;
		return (0);
	} else {
		if (!bot && !(UNIQUE && !cmpv)) {
			cmpv = cmp(tmprec->rec, flist[0]->rec);
			if (cmpv < 0)
				bot = -1;
		}
		if (UNIQUE && !cmpv)
			return (1);
		bot++;
		memmove(flist + bot + 1, flist + bot,
		    (ttop - bot) * sizeof(MFILE **));
		flist[bot] = *rec;
		return (0);
	}
}

/*
 * check order on one file
 */
void
order(struct filelist *filelist, get_func_t get, struct field *ftbl)
{
	u_char *crec_end, *prec_end, *trec_end;
	int c;
	RECHEADER *crec, *prec, *trec;

	buffer = malloc(2 * (DEFLLEN + REC_DATA_OFFSET));
	crec = (RECHEADER *) buffer;
	crec_end = buffer + DEFLLEN + REC_DATA_OFFSET;
	prec = (RECHEADER *) (buffer + DEFLLEN + REC_DATA_OFFSET);
	prec_end = buffer + 2*(DEFLLEN + REC_DATA_OFFSET);
	wts = ftbl->weights;

	/* XXX this does exit(0) for overlong lines */
	if (get(-1, 0, filelist, 1, prec, prec_end, ftbl) != 0)
		exit(0);
	while (get(-1, 0, filelist, 1, crec, crec_end, ftbl) == 0) {
		if (0 < (c = cmp(prec, crec))) {
			crec->data[crec->length-1] = 0;
			errx(1, "found disorder: %s", crec->data+crec->offset);
		}
		if (UNIQUE && !c) {
			crec->data[crec->length-1] = 0;
			errx(1, "found non-uniqueness: %s",
			    crec->data+crec->offset);
		}
		/*
		 * Swap pointers so that this record is on place pointed
		 * to by prec and new record is read to place pointed to by
		 * crec.
		 */ 
		trec = prec;
		prec = crec;
		crec = trec;
		trec_end = prec_end;
		prec_end = crec_end;
		crec_end = trec_end;
	}
	exit(0);
}

static int
cmp(RECHEADER *rec1, RECHEADER *rec2)
{
	int r;
	size_t len, i;
	u_char *pos1, *pos2;
	u_char *cwts;

	if (!SINGL_FLD)
		/* key is weights, and is 0x00 terminated */
		return memcmp(rec1->data, rec2->data, rec1->offset);

	/* We have to apply the weights ourselves */
	cwts = wts;

	pos1 = rec1->data;
	pos2 = rec2->data;
	len = rec1->length;

	for (i = 0; i < len; i++) {
		r = cwts[pos1[i]] - cwts[pos2[i]];
		if (r)
			return r;
	}

	return (0);
}
