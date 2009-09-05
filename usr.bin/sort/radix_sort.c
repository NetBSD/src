/*	$NetBSD: radix_sort.c,v 1.2 2009/09/05 12:00:25 dsl Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Peter McIlroy and by Dan Bernstein at New York University, 
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)radixsort.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: radix_sort.c,v 1.2 2009/09/05 12:00:25 dsl Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * 'stable' radix sort initially from libc/stdlib/radixsort.c
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include "sort.h"

typedef struct {
	const RECHEADER **sa;	/* Base of saved area */
	int sn;				/* Number of entries */
	int si;				/* index into data for compare */
} stack;

static inline int simplesort(const RECHEADER **, int, int, const u_char *, u_int);
static int r_sort_b(const RECHEADER **,
	    const RECHEADER **, int, int, const u_char *, u_int);

#define	THRESHOLD	20		/* Divert to simplesort(). */
#define	SIZE		512		/* Default stack size. */

#define empty(s)	(s >= sp)
#define pop(a, n, i)	a = (--sp)->sa, n = sp->sn, i = sp->si
#define push(a, n, i)	sp->sa = a, sp->sn = n, (sp++)->si = i
#define swap(a, b, t)	t = a, a = b, b = t

int
radix_sort(const RECHEADER **a, const RECHEADER **ta, int n, const u_char *tab, u_int endch)
{
	endch = tab[endch];
	if (n < THRESHOLD && !DEBUG('r')) {
		return simplesort(a, n, 0, tab, endch);
	}
	return r_sort_b(a, ta, n, 0, tab, endch);
}

static int
r_sort_b(const RECHEADER **a, const RECHEADER **ta, int n, int i, const u_char *tr, u_int endch)
{
	static u_int count[256], nc, bmin;
	u_int c;
	const RECHEADER **ak, **ai;
	stack s[512], *sp, *sp0, *sp1, temp;
	const RECHEADER **top[256];
	u_int *cp, bigc;
	int nrec = n;

	sp = s;
	push(a, n, i);
	while (!empty(s)) {
		pop(a, n, i);
		if (n < THRESHOLD && !DEBUG('r')) {
			simplesort(a, n, i, tr, endch);
			continue;
		}

		if (nc == 0) {
			bmin = 255;
			for (ak = a + n; --ak >= a;) {
				c = tr[(*ak)->data[i]];
				if (++count[c] == 1 && c != endch) {
					if (c < bmin)
						bmin = c;
					nc++;
				}
			}
			if (sp + nc > s + SIZE) {
				r_sort_b(a, ta, n, i, tr, endch);
				continue;
			}
		}

		sp0 = sp1 = sp;
		bigc = 2;
		if (endch == 0) {
			top[0] = ak = a + count[0];
			count[0] = 0;
		} else {
			ak = a;
			top[255] = a + n;
			count[255] = 0;
		}
		for (cp = count + bmin; nc > 0; cp++) {
			while (*cp == 0)
				cp++;
			if ((c = *cp) > 1) {
				if (c > bigc) {
					bigc = c;
					sp1 = sp;
				}
				push(ak, c, i+1);
			}
			top[cp-count] = ak += c;
			*cp = 0;			/* Reset count[]. */
			nc--;
		}
		swap(*sp0, *sp1, temp);

		for (ak = ta + n, ai = a+n; ak > ta;)	/* Copy to temp. */
			*--ak = *--ai;
		for (ak = ta+n; --ak >= ta;)		/* Deal to piles. */
			*--top[tr[(*ak)->data[i]]] = *ak;
	}

	return nrec;
}

/* insertion sort */
static inline int
simplesort(const RECHEADER **a, int n, int b, const u_char *tr, u_int endch)
{
	const RECHEADER **ak, **ai, *tmp;
	const RECHEADER **lim = a + n;
	const u_char *s, *t;
	u_char ch;

	if (n <= 1)
		return n;

	for (ak = a+1; ak < lim; ak++) {
		for (ai = ak; ai > a; ai--) {
			for (s = ai[0]->data + b, t = ai[-1]->data + b;
			    (ch = tr[*s]) != endch; s++, t++)
				if (ch != tr[*t])
					break;
			if (ch >= tr[*t]) {

				break;
			}
			swap(ai[0], ai[-1], tmp);
		}
	}

	return n;
}
