/*	$NetBSD: answer.c,v 1.16.12.1 2014/08/20 00:00:23 tls Exp $	*/
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
__RCSID("$NetBSD: answer.c,v 1.16.12.1 2014/08/20 00:00:23 tls Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "hunt.h"

#define SCOREDECAY	15

static char Ttyname[WIRE_NAMELEN];

static IDENT *get_ident(uint32_t, uint32_t, const char *, char);
static void stmonitor(PLAYER *);
static void stplayer(PLAYER *, int);

bool
answer(void)
{
	PLAYER *pp;
	int newsock;
	static uint32_t mode;
	static char name[WIRE_NAMELEN];
	static char team;
	static int32_t enter_status;
	static socklen_t socklen;
	static uint32_t machine;
	static uint32_t uid;
	static struct sockaddr_storage newaddr;
	char *cp1, *cp2;
	int flags;
	uint32_t version;
	int i;

	socklen = sizeof(newaddr);
	newsock = accept(huntsock, (struct sockaddr *)&newaddr, &socklen);
	if (newsock < 0) {
		if (errno == EINTR)
			return false;
		complain(LOG_ERR, "accept");
		cleanup(1);
	}

	/*
	 * XXX this is pretty bollocks
	 */
	switch (newaddr.ss_family) {
	    case AF_INET:
		machine = ((struct sockaddr_in *)&newaddr)->sin_addr.s_addr;
		machine = ntohl(machine);
		break;
	    case AF_INET6: 
		{
			struct sockaddr_in6 *sin6;

			sin6 = (struct sockaddr_in6 *)&newaddr;
			assert(sizeof(sin6->sin6_addr.s6_addr) >
			       sizeof(machine));
			memcpy(&machine, sin6->sin6_addr.s6_addr,
			       sizeof(machine));
		}
		break;
	    case AF_UNIX:
		machine = gethostid();
		break;
	    default:
		machine = 0; /* ? */
		break;
	}

	version = htonl((uint32_t) HUNT_VERSION);
	(void) write(newsock, &version, sizeof(version));
	(void) read(newsock, &uid, sizeof(uid));
	uid = ntohl(uid);
	(void) read(newsock, name, sizeof(name));
	(void) read(newsock, &team, 1);
	(void) read(newsock, &enter_status, sizeof(enter_status));
	enter_status = ntohl(enter_status);
	(void) read(newsock, Ttyname, sizeof(Ttyname));
	(void) read(newsock, &mode, sizeof(mode));
	mode = ntohl(mode);

	/*
	 * Ensure null termination.
	 */
	name[sizeof(name)-1] = '\0';
	Ttyname[sizeof(Ttyname)-1] = '\0';

	/*
	 * Turn off blocking I/O, so a slow or dead terminal won't stop
	 * the game.  All subsequent reads check how many bytes they read.
	 */
	flags = fcntl(newsock, F_GETFL, 0);
	flags |= O_NDELAY;
	(void) fcntl(newsock, F_SETFL, flags);

	/*
	 * Make sure the name contains only printable characters
	 * since we use control characters for cursor control
	 * between driver and player processes
	 */
	for (cp1 = cp2 = name; *cp1 != '\0'; cp1++)
		if (isprint((unsigned char)*cp1) || *cp1 == ' ')
			*cp2++ = *cp1;
	*cp2 = '\0';

	if (mode == C_MESSAGE) {
		char	buf[BUFSIZ + 1];
		int	n;

		if (team == ' ')
			(void) snprintf(buf, sizeof(buf), "%s: ", name);
		else
			(void) snprintf(buf, sizeof(buf), "%s[%c]: ", name,
					team);
		n = strlen(buf);
		for (pp = Player; pp < End_player; pp++) {
			cgoto(pp, HEIGHT, 0);
			outstr(pp, buf, n);
		}
		while ((n = read(newsock, buf, BUFSIZ)) > 0)
			for (pp = Player; pp < End_player; pp++)
				outstr(pp, buf, n);
		for (pp = Player; pp < End_player; pp++) {
			ce(pp);
			sendcom(pp, REFRESH);
			sendcom(pp, READY, 0);
			(void) fflush(pp->p_output);
		}
		(void) close(newsock);
		return false;
	}
	else
#ifdef MONITOR
	if (mode == C_MONITOR)
		if (End_monitor < &Monitor[MAXMON]) {
			pp = End_monitor++;
			i = pp - Monitor + MAXPL + 3;
		} else {
			socklen = 0;
			(void) write(newsock, &socklen,
				sizeof socklen);
			(void) close(newsock);
			return false;
		}
	else
#endif
		if (End_player < &Player[MAXPL]) {
			pp = End_player++;
			i = pp - Player + 3;
		} else {
			socklen = 0;
			(void) write(newsock, &socklen,
				sizeof socklen);
			(void) close(newsock);
			return false;
		}

#ifdef MONITOR
	if (mode == C_MONITOR && team == ' ')
		team = '*';
#endif
	pp->p_ident = get_ident(machine, uid, name, team);
	pp->p_output = fdopen(newsock, "w");
	pp->p_death[0] = '\0';
	pp->p_fd = newsock;
	fdset[i].fd = newsock;
	fdset[i].events = POLLIN;

	pp->p_y = 0;
	pp->p_x = 0;

#ifdef MONITOR
	if (mode == C_MONITOR)
		stmonitor(pp);
	else
#endif
		stplayer(pp, enter_status);
	return true;
}

#ifdef MONITOR
static void
stmonitor(PLAYER *pp)
{
	int line;
	PLAYER *npp;

	memcpy(pp->p_maze, Maze, sizeof Maze);

	drawmaze(pp);

	(void) snprintf(Buf, sizeof(Buf), "%5.5s%c%-10.10s %c", " ",
		stat_char(pp),
		pp->p_ident->i_name, pp->p_ident->i_team);
	line = STAT_MON_ROW + 1 + (pp - Monitor);
	for (npp = Player; npp < End_player; npp++) {
		cgoto(npp, line, STAT_NAME_COL);
		outstr(npp, Buf, STAT_NAME_LEN);
	}
	for (npp = Monitor; npp < End_monitor; npp++) {
		cgoto(npp, line, STAT_NAME_COL);
		outstr(npp, Buf, STAT_NAME_LEN);
	}

	sendcom(pp, REFRESH);
	sendcom(pp, READY, 0);
	(void) fflush(pp->p_output);
}
#endif

static void
stplayer(PLAYER *newpp, int enter_status)
{
	int x, y;
	PLAYER *pp;

	Nplayer++;

	for (y = 0; y < UBOUND; y++)
		for (x = 0; x < WIDTH; x++)
			newpp->p_maze[y][x] = Maze[y][x];
	for (     ; y < DBOUND; y++) {
		for (x = 0; x < LBOUND; x++)
			newpp->p_maze[y][x] = Maze[y][x];
		for (     ; x < RBOUND; x++)
			newpp->p_maze[y][x] = SPACE;
		for (     ; x < WIDTH;  x++)
			newpp->p_maze[y][x] = Maze[y][x];
	}
	for (     ; y < HEIGHT; y++)
		for (x = 0; x < WIDTH; x++)
			newpp->p_maze[y][x] = Maze[y][x];

	do {
		x = rand_num(WIDTH - 1) + 1;
		y = rand_num(HEIGHT - 1) + 1;
	} while (Maze[y][x] != SPACE);
	newpp->p_over = SPACE;
	newpp->p_x = x;
	newpp->p_y = y;
	newpp->p_undershot = false;

#ifdef FLY
	if (enter_status == Q_FLY) {
		newpp->p_flying = rand_num(20);
		newpp->p_flyx = 2 * rand_num(6) - 5;
		newpp->p_flyy = 2 * rand_num(6) - 5;
		newpp->p_face = FLYER;
	}
	else
#endif
	{
		newpp->p_flying = -1;
		newpp->p_face = rand_dir();
	}
	newpp->p_damage = 0;
	newpp->p_damcap = MAXDAM;
	newpp->p_nchar = 0;
	newpp->p_ncount = 0;
	newpp->p_nexec = 0;
	newpp->p_ammo = ISHOTS;
#ifdef BOOTS
	newpp->p_nboots = 0;
#endif
	if (enter_status == Q_SCAN) {
		newpp->p_scan = SCANLEN;
		newpp->p_cloak = 0;
	}
	else {
		newpp->p_scan = 0;
		newpp->p_cloak = CLOAKLEN;
	}
	newpp->p_ncshot = 0;

	do {
		x = rand_num(WIDTH - 1) + 1;
		y = rand_num(HEIGHT - 1) + 1;
	} while (Maze[y][x] != SPACE);
	Maze[y][x] = GMINE;
#ifdef MONITOR
	for (pp = Monitor; pp < End_monitor; pp++)
		check(pp, y, x);
#endif

	do {
		x = rand_num(WIDTH - 1) + 1;
		y = rand_num(HEIGHT - 1) + 1;
	} while (Maze[y][x] != SPACE);
	Maze[y][x] = MINE;
#ifdef MONITOR
	for (pp = Monitor; pp < End_monitor; pp++)
		check(pp, y, x);
#endif

	(void) snprintf(Buf, sizeof(Buf), "%5.2f%c%-10.10s %c",
		newpp->p_ident->i_score,
		stat_char(newpp), newpp->p_ident->i_name,
		newpp->p_ident->i_team);
	y = STAT_PLAY_ROW + 1 + (newpp - Player);
	for (pp = Player; pp < End_player; pp++) {
		if (pp != newpp) {
			char	smallbuf[16];

			pp->p_ammo += NSHOTS;
			newpp->p_ammo += NSHOTS;
			cgoto(pp, y, STAT_NAME_COL);
			outstr(pp, Buf, STAT_NAME_LEN);
			(void) snprintf(smallbuf, sizeof(smallbuf),
					"%3d", pp->p_ammo);
			cgoto(pp, STAT_AMMO_ROW, STAT_VALUE_COL);
			outstr(pp, smallbuf, 3);
		}
	}
#ifdef MONITOR
	for (pp = Monitor; pp < End_monitor; pp++) {
		cgoto(pp, y, STAT_NAME_COL);
		outstr(pp, Buf, STAT_NAME_LEN);
	}
#endif

	drawmaze(newpp);
	drawplayer(newpp, true);
	look(newpp);
#ifdef FLY
	if (enter_status == Q_FLY)
		/* Make sure that the position you enter in will be erased */
		showexpl(newpp->p_y, newpp->p_x, FLYER);
#endif
	sendcom(newpp, REFRESH);
	sendcom(newpp, READY, 0);
	(void) fflush(newpp->p_output);
}

/*
 * rand_dir:
 *	Return a random direction
 */
int
rand_dir(void)
{
	switch (rand_num(4)) {
	  case 0:
		return LEFTS;
	  case 1:
		return RIGHT;
	  case 2:
		return BELOW;
	  case 3:
		return ABOVE;
	}
	/* NOTREACHED */
	return(-1);
}

/*
 * get_ident:
 *	Get the score structure of a player
 */
static IDENT *
get_ident(uint32_t machine, uint32_t uid, const char *name, char team)
{
	IDENT *ip;
	static IDENT punt;

	for (ip = Scores; ip != NULL; ip = ip->i_next)
		if (ip->i_machine == machine
		&&  ip->i_uid == uid
		&&  ip->i_team == team
		&&  strncmp(ip->i_name, name, WIRE_NAMELEN) == 0)
			break;

	if (ip != NULL) {
		if (ip->i_entries < SCOREDECAY)
			ip->i_entries++;
		else
			ip->i_kills = (ip->i_kills * (SCOREDECAY - 1))
				/ SCOREDECAY;
		ip->i_score = ip->i_kills / (double) ip->i_entries;
	}
	else {
		ip = malloc(sizeof(*ip));
		if (ip == NULL) {
			/* Fourth down, time to punt */
			ip = &punt;
		}
		ip->i_machine = machine;
		ip->i_team = team;
		ip->i_uid = uid;
		strncpy(ip->i_name, name, sizeof(ip->i_name));
		ip->i_kills = 0;
		ip->i_entries = 1;
		ip->i_score = 0;
		ip->i_absorbed = 0;
		ip->i_faced = 0;
		ip->i_shot = 0;
		ip->i_robbed = 0;
		ip->i_slime = 0;
		ip->i_missed = 0;
		ip->i_ducked = 0;
		ip->i_gkills = ip->i_bkills = ip->i_deaths = 0;
		ip->i_stillb = ip->i_saved = 0;
		ip->i_next = Scores;
		Scores = ip;
	}

	return ip;
}
