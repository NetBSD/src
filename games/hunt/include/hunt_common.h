/*	$NetBSD: hunt_common.h,v 1.4.8.2 2014/08/20 00:00:23 tls Exp $	*/

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


/*
 * There is no particular significance to the numbers assigned
 * to Test_port.  They're just random numbers greater than the
 * range reserved for privileged sockets.
 */

#ifdef INTERNET
#define SOCK_FAMILY	AF_INET
#define TEST_PORT	(('h' << 8) | 't')
#else
#define SOCK_FAMILY	AF_UNIX
#define AF_UNIX_HACK			/* 4.2 hack; leaves files around */
#endif

/*
 * Preprocessor define dependencies
 */
#if defined(VOLCANO) && !defined(OOZE)
#define OOZE
#endif
#if defined(BOOTS) && !defined(FLY)
#define FLY
#endif
#if !defined(REFLECT) && !defined(RANDOM)
#define RANDOM
#endif

/* decrement version number for each change in startup protocol */
#define HUNT_VERSION		-1

#define ADDCH		('a' | 0200)
#define MOVE		('m' | 0200)
#define REFRESH		('r' | 0200)
#define CLRTOEOL	('c' | 0200)
#define ENDWIN		('e' | 0200)
#define CLEAR		('C' | 0200)
#define REDRAW		('R' | 0200)
#define LAST_PLAYER	('l' | 0200)
#define BELL		('b' | 0200)
#define READY		('g' | 0200)

/*
 * Choose MAXPL and MAXMON carefully.  The screen is assumed to be
 * 23 lines high and will only tolerate (MAXPL == 17 && MAXMON == 0)
 * or (MAXPL + MAXMON <= 16).
 */
#ifdef MONITOR
#define MAXPL		15
#define MAXMON		1
#else
#define MAXPL		17
#define MAXMON		0
#endif
#define WIRE_NAMELEN	20
#define MSGLEN		SCREEN_WIDTH
#define DECAY		50.0

#define NASCII		128

#define WIDTH		51
#define WIDTH2		64	/* Next power of 2 >= WIDTH (for fast access) */
#define HEIGHT		23
#define UBOUND		1
#define DBOUND		(HEIGHT - 1)
#define LBOUND		1
#define RBOUND		(WIDTH - 1)

#define SCREEN_HEIGHT	24
#define SCREEN_WIDTH	80
#define SCREEN_WIDTH2	128	/* Next power of 2 >= SCREEN_WIDTH */

#define STAT_LABEL_COL	60
#define STAT_VALUE_COL	74
#define STAT_NAME_COL	61
#define STAT_SCAN_COL	(STAT_NAME_COL + 5)
#define STAT_AMMO_ROW	0
#define STAT_GUN_ROW	1
#define STAT_DAM_ROW	2
#define STAT_KILL_ROW	3
#define STAT_PLAY_ROW	5
#ifdef MONITOR
#define STAT_MON_ROW	(STAT_PLAY_ROW + MAXPL + 1)
#endif
#define STAT_NAME_LEN	18

#define DOOR		'#'
#define WALL1		'-'
#define WALL2		'|'
#define WALL3		'+'
#ifdef REFLECT
#define WALL4		'/'
#define WALL5		'\\'
#endif
#define KNIFE		'K'
#define SHOT		':'
#define GRENADE		'o'
#define SATCHEL		'O'
#define BOMB		'@'
#define MINE		';'
#define GMINE		'g'
#ifdef OOZE
#define SLIME		'$'
#endif
#ifdef VOLCANO
#define LAVA		'~'
#endif
#ifdef DRONE
#define DSHOT		'?'
#endif
#ifdef FLY
#define FALL		'F'
#endif
#ifdef BOOTS
#define NBOOTS		2
#define BOOT		'b'
#define BOOT_PAIR	'B'
#endif
#define SPACE		' '

#define ABOVE		'i'
#define BELOW		'!'
#define RIGHT		'}'
#define LEFTS		'{'
#ifdef FLY
#define FLYER		'&'
#define isplayer(c)	(c == LEFTS || c == RIGHT ||\
			 c == ABOVE || c == BELOW || c == FLYER)
#else
#define	isplayer(c)	(c == LEFTS || c == RIGHT ||\
			 c == ABOVE || c == BELOW)
#endif

#define NORTH	01
#define SOUTH	02
#define EAST	010
#define WEST	020

#undef CTRL
#define CTRL(x) ((x) & 037)

#define BULSPD		5		/* bullets movement speed */
#define ISHOTS		15
#define NSHOTS		5
#define MAXNCSHOT	2
#define MAXDAM		10
#define MINDAM		5
#define STABDAM		2

#define BULREQ		1
#define GRENREQ		9
#define SATREQ		25
#define BOMB7REQ	49
#define BOMB9REQ	81
#define BOMB11REQ	121
#define BOMB13REQ	169
#define BOMB15REQ	225
#define BOMB17REQ	289
#define BOMB19REQ	361
#define BOMB21REQ	441
#define MAXBOMB		11
#ifdef DRONE
#define MINDSHOT	2	/* At least a satchel bomb */
#endif

#ifdef OOZE
#define SLIME_FACTOR	3
#define SLIMEREQ	5
#define SSLIMEREQ	10
#define SLIME2REQ	15
#define SLIME3REQ	20
#define MAXSLIME	4
#define SLIMESPEED	5
#endif
#ifdef VOLCANO
#define LAVASPEED	1
#endif

#define CLOAKLEN	20
#define SCANLEN		(Nplayer * 20)
#define EXPLEN		4

#define Q_QUIT		0
#define Q_CLOAK		1
#define Q_FLY		2
#define Q_SCAN		3
#define Q_MESSAGE	4

#define C_PLAYER	0
#define C_MONITOR	1
#define C_MESSAGE	2
#define C_SCORES	3

#ifdef FLY
#define _scan_char(pp)	(((pp)->p_scan < 0) ? ' ' : '*')
#define _cloak_char(pp)	(((pp)->p_cloak < 0) ? _scan_char(pp) : '+')
#define stat_char(pp)	(((pp)->p_flying < 0) ? _cloak_char(pp) : FLYER)
#else
#define _scan_char(pp)	(((pp)->p_scan < 0) ? ' ' : '*')
#define stat_char(pp)	(((pp)->p_cloak < 0) ? _scan_char(pp) : '+')
#endif

#ifdef INTERNET
typedef struct sockaddr_in	SOCKET;
#else
typedef struct sockaddr_un	SOCKET;
#endif

