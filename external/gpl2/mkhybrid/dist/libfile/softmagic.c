/* @(#)softmagic.c	1.12 09/07/11 joerg */
#ifndef lint
static	const char sccsid[] =
	"@(#)softmagic.c	1.12 09/07/11 joerg";
#endif
/*
**	find file types by using a modified "magic" file
**
**	based on file v3.22 by Ian F. Darwin (see below)
**
**	Modified for mkhybrid James Pearson 19/5/98
*/

/*
 * softmagic - interpret variable magic from /etc/magic
 *
 * Copyright (c) Ian F. Darwin, 1987.
 * Written by Ian F. Darwin.
 *
 * This software is not subject to any export provision of the United States
 * Department of Commerce, and may be exported to any country or planet.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "file.h"

#ifndef	lint
static const char moduleid[] = 
	"@(#)Id: softmagic.c,v 1.34 1997/01/15 19:28:35 christos Exp";
#endif	/* lint */

#ifdef DEBUG
int	debug = 1; 	/* debugging 				*/
#else
#define	debug	0 	/* debugging 				*/
#endif /* DEBUG */

static char *match	(unsigned char *, int);
static int mget		(union VALUETYPE *,
			     unsigned char *, struct magic *, int);
/* QNX has a mcheck() prototyp in a public include file */
static int magcheck	(union VALUETYPE *, struct magic *);
#ifdef	__used__
static void mdebug	(Int32_t, char *, int);
#endif
static int mconvert	(union VALUETYPE *, struct magic *);

/*
 * softmagic - lookup one file in database 
 * (already read from /etc/magic by apprentice.c).
 * Passed the name and FILE * of one file to be typed.
 */
/*ARGSUSED1*/		/* nbytes passed for regularity, maybe need later */
char *
softmagic(unsigned char *buf, int nbytes)
{
	return (match(buf, nbytes));
}

/*
 * Go through the whole list, stopping if you find a match.  Process all
 * the continuations of that match before returning.
 *
 * We support multi-level continuations:
 *
 *	At any time when processing a successful top-level match, there is a
 *	current continuation level; it represents the level of the last
 *	successfully matched continuation.
 *
 *	Continuations above that level are skipped as, if we see one, it
 *	means that the continuation that controls them - i.e, the
 *	lower-level continuation preceding them - failed to match.
 *
 *	Continuations below that level are processed as, if we see one,
 *	it means we've finished processing or skipping higher-level
 *	continuations under the control of a successful or unsuccessful
 *	lower-level continuation, and are now seeing the next lower-level
 *	continuation and should process it.  The current continuation
 *	level reverts to the level of the one we're seeing.
 *
 *	Continuations at the current level are processed as, if we see
 *	one, there's no lower-level continuation that may have failed.
 *
 *	If a continuation matches, we bump the current continuation level
 *	so that higher-level continuations are processed.
 */
static char *
match(unsigned char *s, int nbytes)
{
	int magindex = 0;
	union VALUETYPE p;

	for (magindex = 0; magindex < __f_nmagic; magindex++) {
		/* if main entry matches, print it... */
		if (!mget(&p, s, &__f_magic[magindex], nbytes) ||
		    !magcheck(&p, &__f_magic[magindex])) {
			    /* 
			     * main entry didn't match,
			     * flush its continuations
			     */
			    while (magindex < __f_nmagic &&
			    	   __f_magic[magindex + 1].cont_level != 0)
			    	   magindex++;
			    continue;
		}

		return (__f_magic[magindex].desc);
	}
	return 0;			/* no match at all */
}


/*
 * Convert the byte order of the data we are looking at
 */
static int
mconvert(union VALUETYPE *p, struct magic *m)
{
	switch (m->type) {
	case BYTE:
	case SHORT:
	case LONG:
	case DATE:
		return 1;
	case STRING:
		{
			char *ptr;

			/* Null terminate and eat the return */
			p->s[sizeof(p->s) - 1] = '\0';
			if ((ptr = strchr(p->s, '\n')) != NULL)
				*ptr = '\0';
			return 1;
		}
	case BESHORT:
		p->h = (short)((p->hs[0]<<8)|(p->hs[1]));
		return 1;
	case BELONG:
	case BEDATE:
		p->l = (Int32_t)
		    ((p->hl[0]<<24)|(p->hl[1]<<16)|(p->hl[2]<<8)|(p->hl[3]));
		return 1;
	case LESHORT:
		p->h = (short)((p->hs[1]<<8)|(p->hs[0]));
		return 1;
	case LELONG:
	case LEDATE:
		p->l = (Int32_t)
		    ((p->hl[3]<<24)|(p->hl[2]<<16)|(p->hl[1]<<8)|(p->hl[0]));
		return 1;
	default:
		return 0;
	}
}

#ifdef	__used__
static void
mdebug(Int32_t offset, char *str, int len)
{
	(void) fprintf(stderr, "mget @%d: ", offset);
	showstr(stderr, (char *) str, len);
	(void) fputc('\n', stderr);
	(void) fputc('\n', stderr);
}
#endif

static int
mget(union VALUETYPE* p, unsigned char *s, struct magic *m, int nbytes)
{
	Int32_t offset = m->offset;

	if (offset + sizeof(union VALUETYPE) <= nbytes)
		memcpy(p, s + offset, sizeof(union VALUETYPE));
	else {
		/*
		 * the usefulness of padding with zeroes eludes me, it
		 * might even cause problems
		 */
		Int32_t have = nbytes - offset;
		memset(p, 0, sizeof(union VALUETYPE));
		if (have > 0)
			memcpy(p, s + offset, have);
	}

	if (!mconvert(p, m))
		return 0;

	if (m->flag & INDIR) {

		switch (m->in.type) {
		case BYTE:
			offset = p->b + m->in.offset;
			break;
		case SHORT:
			offset = p->h + m->in.offset;
			break;
		case LONG:
			offset = p->l + m->in.offset;
			break;
		}

		if (offset + sizeof(union VALUETYPE) > nbytes)
			return 0;

		memcpy(p, s + offset, sizeof(union VALUETYPE));

		if (!mconvert(p, m))
			return 0;
	}
	return 1;
}

static int
magcheck(union VALUETYPE* p, struct magic *m)
{
	register UInt32_t l = m->value.l;
	register UInt32_t v;
	int matched;

	if ( (m->value.s[0] == 'x') && (m->value.s[1] == '\0') ) {
		fprintf(stderr, "BOINK");
		return 1;
	}


	switch (m->type) {
	case BYTE:
		v = p->b;
		break;

	case SHORT:
	case BESHORT:
	case LESHORT:
		v = p->h;
		break;

	case LONG:
	case BELONG:
	case LELONG:
	case DATE:
	case BEDATE:
	case LEDATE:
		v = p->l;
		break;

	case STRING:
		l = 0;
		/* What we want here is:
		 * v = strncmp(m->value.s, p->s, m->vallen);
		 * but ignoring any nulls.  bcmp doesn't give -/+/0
		 * and isn't universally available anyway.
		 */
		v = 0;
		{
			register unsigned char *a = (unsigned char*)m->value.s;
			register unsigned char *b = (unsigned char*)p->s;
			register int len = m->vallen;

			while (--len >= 0)
				if ((v = *b++ - *a++) != '\0')
					break;
		}
		break;
	default:
		return 0;/*NOTREACHED*/
	}

	v = signextend(m, v) & m->mask;

	switch (m->reln) {
	case 'x':
		if (debug)
			(void) fprintf(stderr, "%u == *any* = 1\n", v);
		matched = 1;
		break;

	case '!':
		matched = v != l;
		if (debug)
			(void) fprintf(stderr, "%u != %u = %d\n",
				       v, l, matched);
		break;

	case '=':
		matched = v == l;
		if (debug)
			(void) fprintf(stderr, "%u == %u = %d\n",
				       v, l, matched);
		break;

	case '>':
		if (m->flag & UNSIGNED) {
			matched = v > l;
			if (debug)
				(void) fprintf(stderr, "%u > %u = %d\n",
					       v, l, matched);
		}
		else {
			matched = (Int32_t) v > (Int32_t) l;
			if (debug)
				(void) fprintf(stderr, "%d > %d = %d\n",
					    (Int32_t)v, (Int32_t)l, matched);
		}
		break;

	case '<':
		if (m->flag & UNSIGNED) {
			matched = v < l;
			if (debug)
				(void) fprintf(stderr, "%u < %u = %d\n",
					       v, l, matched);
		}
		else {
			matched = (Int32_t) v < (Int32_t) l;
			if (debug)
				(void) fprintf(stderr, "%d < %d = %d\n",
					    (Int32_t)v, (Int32_t)l, matched);
		}
		break;

	case '&':
		matched = (v & l) == l;
		if (debug)
			(void) fprintf(stderr, "((%x & %x) == %x) = %d\n",
				       v, l, l, matched);
		break;

	case '^':
		matched = (v & l) != l;
		if (debug)
			(void) fprintf(stderr, "((%x & %x) != %x) = %d\n",
				       v, l, l, matched);
		break;

	default:
		matched = 0;
		break;/*NOTREACHED*/
	}

	return matched;
}
