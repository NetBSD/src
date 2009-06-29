/*	$NetBSD: hack.topl.c,v 1.8.10.2 2009/06/29 23:33:53 snj Exp $	*/

/*
 * Copyright (c) 1985, Stichting Centrum voor Wiskunde en Informatica,
 * Amsterdam
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Stichting Centrum voor Wiskunde en
 * Informatica, nor the names of its contributors may be used to endorse or
 * promote products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982 Jay Fenlason <hack@gnu.org>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: hack.topl.c,v 1.8.10.2 2009/06/29 23:33:53 snj Exp $");
#endif				/* not lint */

#include <stdlib.h>
#include "hack.h"
#include "extern.h"

char            toplines[BUFSZ];
xchar           tlx, tly;	/* set by pline; used by addtopl */

struct topl {
	struct topl    *next_topl;
	char           *topl_text;
}              *old_toplines, *last_redone_topl;
#define	OTLMAX	20		/* max nr of old toplines remembered */

int
doredotopl()
{
	if (last_redone_topl)
		last_redone_topl = last_redone_topl->next_topl;
	if (!last_redone_topl)
		last_redone_topl = old_toplines;
	if (last_redone_topl) {
		(void) strcpy(toplines, last_redone_topl->topl_text);
	}
	redotoplin();
	return (0);
}

void
redotoplin()
{
	home();
	if (strchr(toplines, '\n'))
		cl_end();
	putstr(toplines);
	cl_end();
	tlx = curx;
	tly = cury;
	flags.toplin = 1;
	if (tly > 1)
		more();
}

void
remember_topl()
{
	struct topl    *tl;
	int             cnt = OTLMAX;
	if (last_redone_topl &&
	    !strcmp(toplines, last_redone_topl->topl_text))
		return;
	if (old_toplines &&
	    !strcmp(toplines, old_toplines->topl_text))
		return;
	last_redone_topl = 0;
	tl = (struct topl *)
		alloc((unsigned) (strlen(toplines) + sizeof(struct topl) + 1));
	tl->next_topl = old_toplines;
	tl->topl_text = (char *) (tl + 1);
	(void) strcpy(tl->topl_text, toplines);
	old_toplines = tl;
	while (cnt && tl) {
		cnt--;
		tl = tl->next_topl;
	}
	if (tl && tl->next_topl) {
		free((char *) tl->next_topl);
		tl->next_topl = 0;
	}
}

void
addtopl(s)
	const char           *s;
{
	curs(tlx, tly);
	if (tlx + (int)strlen(s) > CO)
		putsym('\n');
	putstr(s);
	tlx = curx;
	tly = cury;
	flags.toplin = 1;
}

void
xmore(s)
	const char *s;	/* allowed chars besides space/return */
{
	if (flags.toplin) {
		curs(tlx, tly);
		if (tlx + 8 > CO)
			putsym('\n'), tly++;
	}
	if (flags.standout)
		standoutbeg();
	putstr("--More--");
	if (flags.standout)
		standoutend();

	xwaitforspace(s);
	if (flags.toplin && tly > 1) {
		home();
		cl_end();
		docorner(1, tly - 1);
	}
	flags.toplin = 0;
}

void
more()
{
	xmore("");
}

void
cmore(s)
	const char           *s;
{
	xmore(s);
}

void
clrlin()
{
	if (flags.toplin) {
		home();
		cl_end();
		if (tly > 1)
			docorner(1, tly - 1);
		remember_topl();
	}
	flags.toplin = 0;
}

void
pline(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vpline(fmt, ap);
	va_end(ap);
}

void
vpline(line, ap)
	const char *line;
	va_list ap;
{
	char            pbuf[BUFSZ];
	char           *bp = pbuf, *tl;
	int             n, n0, tlpos, dead;

	if (!line || !*line)
		return;
	if (!strchr(line, '%'))
		(void) strlcpy(pbuf, line, sizeof(pbuf));
	else
		(void) vsnprintf(pbuf, sizeof(pbuf), line, ap);
	if (flags.toplin == 1 && !strcmp(pbuf, toplines))
		return;
	nscr();			/* %% */

	/* If there is room on the line, print message on same line */
	/* But messages like "You die..." deserve their own line */
	n0 = strlen(bp);
	if (flags.toplin == 1 && tly == 1 &&
	    n0 + (int)strlen(toplines) + 3 < CO - 8 &&	/* leave room for
							 * --More-- */
	    strncmp(bp, "You ", 4)) {
		(void) strcat(toplines, "  ");
		(void) strcat(toplines, bp);
		tlx += 2;
		addtopl(bp);
		return;
	}
	if (flags.toplin == 1)
		more();
	remember_topl();
	dead = 0;
	toplines[0] = 0;
	while (n0 && !dead) {
		if (n0 >= CO) {
			/* look for appropriate cut point */
			n0 = 0;
			for (n = 0; n < CO; n++)
				if (bp[n] == ' ')
					n0 = n;
			if (!n0)
				for (n = 0; n < CO - 1; n++)
					if (!letter(bp[n]))
						n0 = n;
			if (!n0)
				n0 = CO - 2;
		}
		tlpos = strlen(toplines);
		tl = toplines + tlpos;
		/* avoid overflow */
		if (tlpos + n0 > (int)sizeof(toplines) - 1) {
			n0 = sizeof(toplines) - 1 - tlpos;
			dead = 1;
		}
		(void) memcpy(tl, bp, n0);
		tl[n0] = 0;
		bp += n0;

		/* remove trailing spaces, but leave one */
		while (n0 > 1 && tl[n0 - 1] == ' ' && tl[n0 - 2] == ' ')
			tl[--n0] = 0;

		n0 = strlen(bp);
		if (n0 && tl[0])
			(void) strlcat(toplines, "\n", sizeof(toplines));
	}
	redotoplin();
}

void
putsym(c)
	char            c;
{
	switch (c) {
	case '\b':
		backsp();
		return;
	case '\n':
		curx = 1;
		cury++;
		if (cury > tly)
			tly = cury;
		break;
	default:
		if (curx == CO)
			putsym('\n');	/* 1 <= curx <= CO; avoid CO */
		else
			curx++;
	}
	(void) putchar(c);
}

void
putstr(s)
	const char           *s;
{
	while (*s)
		putsym(*s++);
}
