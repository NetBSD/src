/*	$NetBSD: hack.rip.c,v 1.5 1997/10/19 16:58:53 christos Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: hack.rip.c,v 1.5 1997/10/19 16:58:53 christos Exp $");
#endif				/* not lint */

#include "hack.h"
#include "extern.h"

static char    *riptop = "\
                       ----------\n\
                      /          \\\n\
                     /    REST    \\\n\
                    /      IN      \\\n\
                   /     PEACE      \\\n\
                  /                  \\";

static char    *ripmid = "                  | %*s%*s |\n";

static char    *ripbot = "\
                 *|     *  *  *      | *\n\
        _________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______";

void
outrip()
{
	char            buf[BUFSZ];

	cls();
	curs(1, 8);
	puts(riptop);
	(void) strcpy(buf, plname);
	buf[16] = 0;
	center(6, buf);
	(void) sprintf(buf, "%ld AU", u.ugold);
	center(7, buf);
	(void) sprintf(buf, "killed by%s",
		       !strncmp(killer, "the ", 4) ? "" :
		       !strcmp(killer, "starvation") ? "" :
		       strchr(vowels, *killer) ? " an" : " a");
	center(8, buf);
	(void) strcpy(buf, killer);
	{
		int             i1;
		if ((i1 = strlen(buf)) > 16) {
			int             i, i0;
			i0 = i1 = 0;
			for (i = 0; i <= 16; i++)
				if (buf[i] == ' ')
					i0 = i, i1 = i + 1;
			if (!i0)
				i0 = i1 = 16;
			buf[i1 + 16] = 0;
			buf[i0] = 0;
		}
		center(9, buf);
		center(10, buf + i1);
	}
	(void) sprintf(buf, "%4d", getyear());
	center(11, buf);
	puts(ripbot);
	getret();
}

void
center(line, text)
	int             line;
	char           *text;
{
	int             n = strlen(text) / 2;
	printf(ripmid, 8 + n, text, 8 - n, "");
}
