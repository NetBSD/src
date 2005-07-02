/*	$NetBSD: main.cc,v 1.3 2005/07/02 15:48:03 jdc Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * main.C: Main dots program
 */
#include "defs.h"
RCSID("$NetBSD: main.cc,v 1.3 2005/07/02 15:48:03 jdc Exp $")

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "algor.h"
#include "board.h"
#include "human.h"
#include "ttyscrn.h"

GAMESCREEN *sc;

// Print the command line usage
static void usage(char* pname)
{
    char* p = strrchr(pname, '/');
    if (p)
	p++;
    else
	p = pname;
    std::cerr << "Usage: " << p
	<< " [-w] [-p <c|h><c|h>] [-n <ngames>] [<ydim> [<xdim>]]" << std::endl;
}

// Play a single game
static void play(BOARD& b, PLAYER* p[2])
{
    // Initialize
    b.init();
    p[0]->init();
    p[1]->init();
    b.paint();

    // Alternate turns between players, scoring each turn
    for (size_t i = 0;; i = (i + 1) & 1) {
	b.score(i, *p[i]);
	if (!p[i]->domove(b)) {
	    // No more moves, game over
	    break;
	}
	b.score(i, *p[i]);
    }

    // Find who won
    p[0]->wl(p[1]->getScore());
    p[1]->wl(p[0]->getScore());

    // Post scores
    b.score(0, *p[0]);
    b.score(1, *p[1]);

    // Post totals
    b.total(0, *p[0]);
    b.total(1, *p[1]);

    // Post games
    b.games(0, *p[0]);
    b.games(1, *p[1]);

    // Post ties
    b.ties(*p[0]);
}

int main(int argc, char** argv)
{
    size_t ny, nx, nn = 1, wt = 0;
    const char* nc = "ch";
    int c;
    int acs = 1;

    while ((c = getopt(argc, argv, "awp:n:")) != -1)
	switch (c) {
	case 'a':
	    acs = 0;
	    break;
	case 'w':
	    wt++;
	    break;

	case 'p':
	    nc = optarg;
	    break;

	case 'n':
	    nn = atoi(optarg);
	    break;

	default:
	    usage(argv[0]);
	    return 1;
	}

    // Get the size of the board if specified
    switch (argc - optind) {
    case 0:
	ny = nx = 3;
	break;

    case 1:
	ny = nx = atoi(argv[optind]);
	break;

    case 2:
	nx = atoi(argv[optind]);
	ny = atoi(argv[optind+1]);
	break;

    default:
	usage(argv[0]);
	return 1;
    }
    

    PLAYER* p[2];

    // Allocate players
    for (size_t i = 0; i < 2; i++) {
	char n = nc[1] == nc[0] ? i + '0' : nc[i];
	switch (nc[i]) {
	case 'c':
	    p[i] = new ALGOR(n);
	    break;

	case 'h':
	    p[i] = new HUMAN(n);
	    break;

	default:
	    usage(argv[0]);
	    return 1;
	}
    }

    sc = TTYSCRN::create(acs, ny, nx);
    if (sc == NULL)
	::errx(1, "Dimensions too large for current screen.");

    BOARD b(ny, nx, sc);

    // Play games
    while (nn--) {
	play(b, p);
	if (wt)
	    b.getmove();
    }

    if (wt == 0)
	b.getmove();
    // Cleanup
    delete sc;
    delete p[0];
    delete p[1];
    return 0;
}
