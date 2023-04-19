/*	$NetBSD: worms.c,v 1.28 2023/04/19 07:40:49 kre Exp $	*/

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)worms.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: worms.c,v 1.28 2023/04/19 07:40:49 kre Exp $");
#endif
#endif /* not lint */

/*
 *
 *	 @@@        @@@    @@@@@@@@@@     @@@@@@@@@@@    @@@@@@@@@@@@
 *	 @@@        @@@   @@@@@@@@@@@@    @@@@@@@@@@@@   @@@@@@@@@@@@@
 *	 @@@        @@@  @@@@      @@@@   @@@@           @@@@ @@@  @@@@
 *	 @@@   @@   @@@  @@@        @@@   @@@            @@@  @@@   @@@
 *	 @@@  @@@@  @@@  @@@        @@@   @@@            @@@  @@@   @@@
 *	 @@@@ @@@@ @@@@  @@@        @@@   @@@            @@@  @@@   @@@
 *	  @@@@@@@@@@@@   @@@@      @@@@   @@@            @@@  @@@   @@@
 *	   @@@@  @@@@     @@@@@@@@@@@@    @@@            @@@  @@@   @@@
 *	    @@    @@       @@@@@@@@@@     @@@            @@@  @@@   @@@
 *
 *				 Eric P. Scott
 *			  Caltech High Energy Physics
 *				 October, 1980
 *
 */
#include <sys/types.h>

#include <ctype.h>
#include <curses.h>
#include <err.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

static const struct options {
	int nopts;
	int opts[3];
}
	normal[8] = {
	{ 3, { 7, 0, 1 } },
	{ 3, { 0, 1, 2 } },
	{ 3, { 1, 2, 3 } },
	{ 3, { 2, 3, 4 } },
	{ 3, { 3, 4, 5 } },
	{ 3, { 4, 5, 6 } },
	{ 3, { 5, 6, 7 } },
	{ 3, { 6, 7, 0 } }
},	upper[8] = {
	{ 1, { 1, 0, 0 } },
	{ 2, { 1, 2, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 2, { 4, 5, 0 } },
	{ 1, { 5, 0, 0 } },
	{ 2, { 1, 5, 0 } }
},
	left[8] = {
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 2, { 2, 3, 0 } },
	{ 1, { 3, 0, 0 } },
	{ 2, { 3, 7, 0 } },
	{ 1, { 7, 0, 0 } },
	{ 2, { 7, 0, 0 } }
},
	right[8] = {
	{ 1, { 7, 0, 0 } },
	{ 2, { 3, 7, 0 } },
	{ 1, { 3, 0, 0 } },
	{ 2, { 3, 4, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 2, { 6, 7, 0 } }
},
	lower[8] = {
	{ 0, { 0, 0, 0 } },
	{ 2, { 0, 1, 0 } },
	{ 1, { 1, 0, 0 } },
	{ 2, { 1, 5, 0 } },
	{ 1, { 5, 0, 0 } },
	{ 2, { 5, 6, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } }
},
	upleft[8] = {
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 1, { 3, 0, 0 } },
	{ 2, { 1, 3, 0 } },
	{ 1, { 1, 0, 0 } }
},
	upright[8] = {
	{ 2, { 3, 5, 0 } },
	{ 1, { 3, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 1, { 5, 0, 0 } }
},
	lowleft[8] = {
	{ 3, { 7, 0, 1 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 1, { 1, 0, 0 } },
	{ 2, { 1, 7, 0 } },
	{ 1, { 7, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } }
},
	lowright[8] = {
	{ 0, { 0, 0, 0 } },
	{ 1, { 7, 0, 0 } },
	{ 2, { 5, 7, 0 } },
	{ 1, { 5, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } },
	{ 0, { 0, 0, 0 } }
};


static const char	flavor[] = {
	'O', '*', '#', '$', '%', '0', '@', '~',
	'+', 'w', ':', '^', '_', '&', 'x', 'o'
};
static const short	xinc[] = {
	1,  1,  1,  0, -1, -1, -1,  0
}, yinc[] = {
	-1,  0,  1,  1,  1,  0, -1, -1
};
static struct	worm {
	int orientation, head;
	short *xpos, *ypos;
} *worm;

static volatile sig_atomic_t sig_caught = 0;

static void nomem(void) __dead;
static void onsig(int);

int
main(int argc, char *argv[])
{
	int x, y, h, n;
	struct worm *w;
	const struct options *op;
	short *ip;
	int CO, LI, last, bottom, ch, length, number, trail;
	unsigned int seed;
	short **ref;
	const char *field;
	char *ep;
	unsigned int delay = 20000;
	unsigned long ul;
	bool argerror = false;

	length = 16;
	number = 3;
	trail = ' ';
	field = NULL;
	seed = 0;
	while ((ch = getopt(argc, argv, "d:fl:n:S:t")) != -1) {
		switch(ch) {
		case 'd':
			ul = strtoul(optarg, &ep, 10);
			if (ep != optarg) {
				while (isspace(*(unsigned char *)ep))
					ep++;
			}
			if (ep == optarg ||
			    (*ep != '\0' &&
				( ep[1] == '\0' ? (*ep != 'm' && *ep != 'u') :
				( strcasecmp(ep, "ms") != 0 &&
				  strcasecmp(ep, "us") != 0 )) )) {
				    errx(1, "-d: invalid delay (%s)", optarg);
			}
			/*
			 * if ul >= INT_MAX/1000 we don't need the *1000,
			 * as even without that it will exceed the limit
			 * just below and be treated as an error.
			 * (This does assume >=32 bit int, but so does POSIX)
			 */
			if (*ep != 'u' && ul < INT_MAX / 1000)
				ul *= 1000;  /* ms -> us */
			if (ul > 1000*1000) {
				errx(1,
				   "-d: delay (%s) out of range [0 - 1000]",
				   optarg);
			}
			delay = (unsigned int)ul;
			continue;
		case 'f':
			field = "WORM";
			continue;
		case 'l':
			ul = strtoul(optarg, &ep, 10);
			if (ep == optarg || *ep != '\0' ||
			    ul < 2 || ul > 1024) {
				errx(1, "-l: invalid length (%s) [%d - %d].",
				     optarg, 2, 1024);
			}
			length = (int)ul;
			continue;
		case 'n':
			ul = strtoul(optarg, &ep, 10);
			if (ep == optarg || *ep != '\0' ||
			    ul < 1 || ul > INT_MAX / 10 ) {
				errx(1, "-n: invalid number of worms (%s).",
				    optarg);
			}
			/* upper bound is further limited later */
			number = (int)ul;
			continue;
		case 'S':
			ul = strtoul(optarg, &ep, 0);
			if (ep == optarg || *ep != '\0' ||
			    ul > UINT_MAX ) {
				errx(1, "-S: invalid seed (%s).", optarg);
			}
			seed = (unsigned int)ul;
			continue;
		case 't':
			trail = '.';
			continue;
		case '?':
		default:
			argerror = true;
			break;
		}
		break;
	}

	if (argerror || argc > optind)
		errx(1,
		    "Usage: worms [-ft] [-d delay] [-l length] [-n number]");
		/* -S omitted deliberately, not useful often enough */

	if (!initscr())
		errx(1, "couldn't initialize screen");
	curs_set(0);
	CO = COLS;
	LI = LINES;

	if (CO == 0 || LI == 0) {
		endwin();
		errx(1, "screen must be a rectangle, not (%dx%d)", CO, LI);
	}
	if (CO >= INT_MAX / LI) {
		endwin();
		errx(1, "screen (%dx%d) too large for worms", CO, LI);
	}

	/* now known that LI*CO cannot overflow an int => also not a long */

	if (LI < 3 || CO < 3 || LI * CO < 40) {
		/*
		 * The placement algorithm is too weak for dimensions < 3.
		 * Need at least 40 spaces so we can have (n > 1) worms
		 * of a reasonable length, and still leave empty space.
		 */
		endwin();
		errx(1, "screen (%dx%d) too small for worms", CO, LI);
	}

	ul = (unsigned long)CO * LI;
	if ((unsigned long)length > ul / 20) {
		endwin();
		errx(1, "-l: worms loo long (%d) for screen; max: %lu",
		    length, ul / 20);
	}

	ul /= (length * 3);	/* no more than 33% arena occupancy */
	if ((unsigned long)(unsigned)number > ul) {
		endwin();
		errx(1, "-n: too many worms (%d) max: %lu", number, ul);
	}
	if (!(worm = calloc((size_t)number, sizeof(struct worm))))
		nomem();

	last = CO - 1;
	bottom = LI - 1;
	if (!(ip = malloc((size_t)(LI * CO * sizeof(short)))))
		nomem();
	if (!(ref = malloc((size_t)(LI * sizeof(short *)))))
		nomem();
	for (n = 0; n < LI; ++n) {
		ref[n] = ip;
		ip += CO;
	}
	for (ip = ref[0], n = LI * CO; --n >= 0;)
		*ip++ = 0;
	for (n = number, w = &worm[0]; --n >= 0; w++) {
		w->orientation = w->head = 0;
		if (!(ip = malloc((size_t)(length * sizeof(short)))))
			nomem();
		w->xpos = ip;
		for (x = length; --x >= 0;)
			*ip++ = -1;
		if (!(ip = malloc((size_t)(length * sizeof(short)))))
			nomem();
		w->ypos = ip;
		for (y = length; --y >= 0;)
			*ip++ = -1;
	}

	(void)signal(SIGHUP, onsig);
	(void)signal(SIGINT, onsig);
	(void)signal(SIGQUIT, onsig);
	(void)signal(SIGTSTP, onsig);
	(void)signal(SIGTERM, onsig);

	if (field) {
		const char *p = field;

		for (y = LI; --y >= 0;) {
			for (x = CO; --x >= 0;) {
				addch(*p++);
				if (!*p)
					p = field;
			}
			refresh();
		}
	}
	srandom(seed ? seed : arc4random());
	for (;;) {
		refresh();
		if (sig_caught) {
			endwin();
			exit(0);
		}
		if (delay) {
			if (delay % 1000000 != 0)
				usleep(delay % 1000000);
			if (delay >= 1000000)
				sleep(delay / 1000000);
		}
		for (n = 0, w = &worm[0]; n < number; n++, w++) {
			if ((x = w->xpos[h = w->head]) < 0) {
				mvaddch(y = w->ypos[h] = bottom,
					x = w->xpos[h] = 0,
					flavor[n % sizeof(flavor)]);
				ref[y][x]++;
			}
			else
				y = w->ypos[h];
			if (++h == length)
				h = 0;
			if (w->xpos[w->head = h] >= 0) {
				int x1, y1;

				x1 = w->xpos[h];
				y1 = w->ypos[h];
				if (--ref[y1][x1] == 0) {
					mvaddch(y1, x1, trail);
				}
			}

			op = &(!x
				? (!y
				    ? upleft
				    : (y == bottom ? lowleft : left))
				: (x == last
				    ? (!y ? upright
					  : (y == bottom ? lowright : right))
				    : (!y ? upper
					  : (y == bottom ? lower : normal)))
			      )[w->orientation];

			switch (op->nopts) {
			case 0:
				refresh();
				endwin();
				abort();
				/* NOTREACHED */
			case 1:
				w->orientation = op->opts[0];
				break;
			default:
				w->orientation =
				    op->opts[(int)random() % op->nopts];
			}
			mvaddch(y += yinc[w->orientation],
				x += xinc[w->orientation],
				flavor[n % sizeof(flavor)]);
			ref[w->ypos[h] = y][w->xpos[h] = x]++;
		}
	}
}

static void
onsig(int signo __unused)
{
	sig_caught = 1;
}

/* This is never called before curses is initialised */
static void
nomem(void)
{
	endwin();
	errx(1, "not enough memory.");
}
