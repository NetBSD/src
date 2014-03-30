/*	$NetBSD: extern.c,v 1.10 2014/03/30 00:26:58 dholland Exp $	*/
/*
 * Copyright (c) 1983-2003, Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * + Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * + Neither the name of the University of California, San Francisco nor 
 *   the names of its contributors may be used to endorse or promote 
 *   products derived from this software without specific prior written 
 *   permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: extern.c,v 1.10 2014/03/30 00:26:58 dholland Exp $");
#endif /* not lint */

#include "hunt.h"

#if 0 /*def MONITOR*/ /* apparently unused (XXX?) */
bool Am_monitor = false;		/* current process is a monitor */
#endif

char Buf[BUFSIZ];			/* general scribbling buffer */
char Maze[HEIGHT][WIDTH2];		/* the maze */
char Orig_maze[HEIGHT][WIDTH2];		/* the original maze */

struct pollfd fdset[3+MAXPL+MAXMON];
int Nplayer = 0;			/* number of players */
int See_over[NASCII];			/* lookup table for determining whether
					 * character represents "transparent"
					 * item */

BULLET *Bullets = NULL;			/* linked list of bullets */

PLAYER Player[MAXPL];			/* all the players */
PLAYER *End_player = Player;		/* last active player slot */
#ifdef BOOTS
PLAYER Boot[NBOOTS];			/* all the boots */
#endif
IDENT *Scores;				/* score cache */
#ifdef MONITOR
PLAYER Monitor[MAXMON];			/* all the monitors */
PLAYER *End_monitor = Monitor;		/* last active monitor slot */
#endif

const int shot_req[MAXBOMB] = {
	BULREQ, GRENREQ, SATREQ,
	BOMB7REQ, BOMB9REQ, BOMB11REQ,
	BOMB13REQ, BOMB15REQ, BOMB17REQ,
	BOMB19REQ, BOMB21REQ,
};

const int shot_type[MAXBOMB] = {
	SHOT, GRENADE, SATCHEL,
	BOMB, BOMB, BOMB,
	BOMB, BOMB, BOMB,
	BOMB, BOMB,
};

#ifdef OOZE
const int slime_req[MAXSLIME] = {
	SLIMEREQ, SSLIMEREQ, SLIME2REQ, SLIME3REQ,
};
#endif
