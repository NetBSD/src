/*	$NetBSD: worms.c,v 1.31 2023/05/12 13:29:41 kre Exp $	*/

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
__RCSID("$NetBSD: worms.c,v 1.31 2023/05/12 13:29:41 kre Exp $");
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
#include <term.h>
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
},
	upper[8] = {
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
	'O', '*', '#', '$', '%', '0', 'o', '~',
	'+', 'x', ':', '^', '_', '&', '@', 'w'
};
static const int flavors = __arraycount(flavor);
static const char	eyeball[] = {
	'0', 'X', '@', 'S', 'X', '@', 'O', '=',
	'#', '*', '=', 'v', 'L', '@', 'o', 'm'
};
__CTASSERT(sizeof(flavor) == sizeof(eyeball));

static const short	xinc[] = {
	1,  1,  1,  0, -1, -1, -1,  0
}, yinc[] = {
	-1,  0,  1,  1,  1,  0, -1, -1
};
static struct	worm {
	int orientation, head, len;
	short *xpos, *ypos;
	chtype ch, eye, attr;
} *worm;

static volatile sig_atomic_t sig_caught;

static int initclr(int**);
static void nomem(void) __dead;
static void onsig(int);
static int worm_length(int, int);

int
main(int argc, char *argv[])
{
	int CO, LI, last, bottom, ch, number, trail;
	int x, y, h, n, nc;
	int maxlength, minlength;
	unsigned int seed, delay;
	const struct options *op;
	struct worm *w;
	short **ref;
	short *ip;
	const char *field;
	char *ep;
	unsigned long ul, up;
	bool argerror = false;
	bool docolour = false;		/* -C, use coloured worms */
	bool docaput = false;		/* -H, show which end of worm is head */
	int *ctab = NULL;

	delay = 20000;
	maxlength = minlength = 16;
	number = 3;
	seed = 0;
	trail = ' ';
	field = NULL;

	if ((ep = getenv("WORMS")) != NULL) {
		ul = up = 0;
		while ((ch = *ep++) != '\0') {
			switch (ch) {
			case 'C':
				docolour = !docolour;
				continue;
			case 'f':
				if (field)
					field = NULL;
				else
					field = "WORM";
				continue;
			case 'H':
				docaput = !docaput;
				continue;
			case 'r':
				minlength = 5;
				maxlength = 0;
				continue;
			case 't':
				if (trail == ' ')
					trail = '.';
				else
					trail = ' ';
				continue;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				if (up > 1)
					continue;
				if (ul >= 100000)	/* 1/10 second, in us */
					continue;
				ul *= 10;
				ul += (ch - '0');
				up = 1;
				continue;
			case 'm':
				if (up == 1 && ul <= 1000)
					ul *= 1000;
				up += 2;
				continue;
			default:
				continue;
			}
		}
		if ((up & 1) != 0)	/* up == 1 || up == 3 */
			delay = ul;
	}

	while ((ch = getopt(argc, argv, "Cd:fHl:n:rS:t")) != -1) {
		switch(ch) {
		case 'C':
			docolour = !docolour;
			continue;
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
			if (field == NULL)
				field = "WORM";
			else
				field = NULL;
			continue;
		case 'H':
			docaput = !docaput;
			continue;
		case 'l':
			up = ul = strtoul(optarg, &ep, 10);
			if (ep != optarg) {
				while (isspace(*(unsigned char *)ep))
					ep++;
				if (*ep == '-')
					up = strtoul(++ep, &ep, 10);
			}
			if (ep == optarg || *ep != '\0' ||
			    ul < 2 || up < ul || up > 1024) {
				errx(1, "-l: invalid length (%s) [%d - %d].",
				     optarg, 2, 1024);
			}
			minlength = (int)ul;
			maxlength = (int)up;
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
		case 'r':
			minlength = 5;
			maxlength = 0;
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
			if (trail == ' ')
				trail = '.';
			else
				trail = ' ';
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
		    "Usage: worms [-CfHrt] [-d delay] [-l length] [-n number]");
		/* -S omitted deliberately, not useful often enough */

	if (!initscr())
		errx(1, "couldn't initialize screen");
	curs_set(0);
	nc = docolour ? initclr(&ctab) : 0;
	CO = COLS;
	LI = LINES;

	if (CO == 0 || LI == 0) {
		endwin();
		errx(1, "screen must be a rectangle, not (%dx%d)", CO, LI);
	}

	/*
	 * The "sizeof(short *)" noise is to abslutely guarantee
	 * that a LI * CO * sizeof(short *) cannot overflow an int
	 */
	if (CO >= (int)(INT_MAX / (2 * sizeof(short *)) / LI)) {
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

	if (maxlength == 0)
		maxlength = minlength + (CO * LI / 40);

	ul = (unsigned long)CO * LI;
	if ((unsigned long)maxlength > ul / 20) {
		endwin();
		errx(1, "-l: worms too long (%d) for screen; max: %lu",
		    maxlength, ul / 20);
	}

	ul /= maxlength * 3;	/* no more than 33% arena occupancy */

	if ((unsigned long)(unsigned)number > ul && maxlength > minlength) {
		maxlength = CO * LI / 3 / number;
		if (maxlength < minlength)
			maxlength = minlength;
		ul = (CO * LI) / ((minlength + maxlength)/2 * 3);;
	}

	if ((unsigned long)(unsigned)number > ul) {
		endwin();
		errx(1, "-n: too many worms (%d) max: %lu", number, ul);
	}
	if (!(worm = calloc((size_t)number, sizeof(struct worm))))
		nomem();

	srandom(seed ? seed : arc4random());

	last = CO - 1;
	bottom = LI - 1;

	if (!(ip = calloc(LI * CO, sizeof(short))))
		nomem();
	if (!(ref = malloc((size_t)LI * sizeof(short *))))
		nomem();
	for (n = 0; n < LI; ++n) {
		ref[n] = ip;
		ip += CO;
	}

	for (n = number, w = &worm[0]; --n >= 0; w++) {
		int i;

		w->orientation = w->head = 0;
		w->len = worm_length(minlength, maxlength);
		w->attr = nc ? ctab[n % nc] : 0;
		i = (nc && number > flavors ? n / nc : n) % flavors;
		w->ch = flavor[i];
		w->eye = eyeball[i];

		if (!(ip = malloc((size_t)(w->len * sizeof(short)))))
			nomem();
		w->xpos = ip;
		for (x = w->len; --x >= 0;)
			*ip++ = -1;
		if (!(ip = malloc((size_t)(w->len * sizeof(short)))))
			nomem();
		w->ypos = ip;
		for (y = w->len; --y >= 0;)
			*ip++ = -1;
	}
	free(ctab);		/* not needed any more */

	(void)signal(SIGHUP, onsig);
	(void)signal(SIGINT, onsig);
	(void)signal(SIGQUIT, onsig);
	(void)signal(SIGTERM, onsig);
	(void)signal(SIGTSTP, onsig);
	(void)signal(SIGWINCH, onsig);

	if (field) {
		const char *p = field;

		for (y = LI; --y >= 0;) {
			for (x = CO; --x >= 0;) {
				addch(*p++);
				if (!*p)
					p = field;
			}
		}
	}

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
			chtype c = docaput ? w->eye : w->ch;

			if ((x = w->xpos[h = w->head]) < 0) {
				mvaddch(y = w->ypos[h] = bottom,
					x = w->xpos[h] = 0, c | w->attr);
				ref[y][x]++;
			} else
				y = w->ypos[h];
			if (++h == w->len)
				h = 0;
			if (w->xpos[w->head = h] >= 0) {
				int x1, y1;

				x1 = w->xpos[h];
				y1 = w->ypos[h];
				if (--ref[y1][x1] == 0)
					mvaddch(y1, x1, trail);
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
				c | w->attr);
			ref[w->ypos[h] = y][w->xpos[h] = x]++;
			if (docaput && w->len > 1) {
				int prev = (h ? h : w->len) - 1;

				mvaddch(w->ypos[prev], w->xpos[prev],
					w->ch | w->attr);
			}
		}
	}
}

static int
initclr(int** ctab)
{
	int *ip, clr[] = {
		COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
		COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
	}, attr[] = {
		A_NORMAL, A_BOLD, A_DIM
	};
	int nattr = __arraycount(attr);
	int nclr = __arraycount(clr);
	int nc = 0;

	/* terminfo first */
	char* s;
	bool canbold = (s = tigetstr("bold")) != (char* )-1 && s != NULL;
	bool candim  = (s = tigetstr("dim")) != (char* )-1 && s != NULL;

#ifdef DO_TERMCAP
	/* termcap if terminfo fails */
	canbold = canbold || (s = tgetstr("md", NULL)) != NULL;
	candim  = candim  || (s = tgetstr("mh", NULL)) != NULL;
#endif

	if (has_colors() == FALSE)
		return 0;
	use_default_colors();
	if (start_color() == ERR)
		return 0;
	if ((*ctab = calloc(COLOR_PAIRS, sizeof(int))) == NULL)
		nomem();
	ip = *ctab;

	for (int i = 0; i < nattr; i++) {
		if (!canbold && attr[i] == A_BOLD)
			continue;
		if (!candim && attr[i] == A_DIM)
			continue;

		for (int j = 0; j < nclr; j++) {
			if (clr[j] == COLOR_BLACK && attr[i] != A_BOLD)
				continue;	/* invisible */
			if (nc + 1 >= COLOR_PAIRS)
				break;
			if (init_pair(nc + 1, clr[j], -1) == ERR)
				break;
			*ip++ = COLOR_PAIR(nc + 1) | attr[i];
			nc++;
		}
	}

	return nc;
}

static int
worm_length(int low, int high)
{
	if (low >= high)
		return low;

	return low + (random() % (high - low + 1));
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
