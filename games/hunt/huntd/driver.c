/*	$NetBSD: driver.c,v 1.21.8.1 2014/08/20 00:00:23 tls Exp $	*/
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
__RCSID("$NetBSD: driver.c,v 1.21.8.1 2014/08/20 00:00:23 tls Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <err.h>

#include "hunt.h"
#include "pathnames.h"

/*
 * There are three listening sockets in this daemon:
 *    - a datagram socket that listens on a well known port
 *    - a stream socket for general player connections
 *    - a second stream socket that prints the stats/scores
 *
 * These are (now) named as follows:
 *    - contact (contactsock, contactaddr, etc.)
 *    - hunt (huntsock, huntaddr, etc.)
 *    - stat (statsock, stataddr, etc.)
 *
 * Until being cleaned up in 2014 the code used an assortment of
 * inconsistent and confusing names to refer to the pieces of these:
 *
 *    test_port -> contactaddr
 *    Test_port -> contactport
 *    Test_socket -> contactsock
 *
 *    sock_port -> huntport
 *    Socket -> huntsock
 *
 *    Daemon -> both stataddr and huntaddr
 *    stat_port -> statport
 *    status -> statsock
 *
 * It isn't clear to me what purpose contactsocket is supposed to
 * serve; maybe it was supposed to avoid asking inetd to support
 * tcp/wait sockets? Anyway, we can't really change the protocol at
 * this point. (To complicate matters, contactsocket doesn't exist if
 * using AF_UNIX sockets; you just connect to the game socket.)
 *
 * When using internet sockets:
 *    - the contact socket listens on INADDR_ANY using the game port
 *      (either specified or the default: CONTACT_PORT, currently
 *      spelled TEST_PORT)
 *    - the hunt socket listens on INADDR_ANY using whatever port
 *    - the stat socket listens on INADDR_ANY using whatever port
 *
 * When using AF_UNIX sockets:
 *    - the contact socket isn't used
 *    - the hunt socket listens on the game socket (either a specified path
 *      or /tmp/hunt)
 *    - the stat socket listens on its own socket (huntsocket's path +
 *      ".stats")
 */

static bool localmode;			/* true -> AF_UNIX; false -> AF_INET */
static bool inetd_spawned;		/* invoked via inetd? */
static bool standard_port = true;	/* listening on standard port? */

static struct sockaddr_storage huntaddr;
static struct sockaddr_storage stataddr;
static socklen_t huntaddrlen;
static socklen_t stataddrlen;

static uint16_t contactport = TEST_PORT;
static uint16_t	huntport;		/* port # of tcp listen socket */
static uint16_t	statport;		/* port # of statistics tcp socket */

static const char *huntsockpath = PATH_HUNTSOCKET;
static char *statsockpath;

static int contactsock;			/* socket to answer datagrams */
int huntsock;				/* main socket */
static int statsock;			/* stat socket */

#ifdef VOLCANO
static int volcano = 0;			/* Explosion size */
#endif

static void clear_scores(void);
static bool havechar(PLAYER *, int);
static void init(void);
static void makeboots(void);
static void send_stats(void);
static void zap(PLAYER *, bool, int);

static int
getnum(const char *s, unsigned long *ret)
{
	char *t;

	errno = 0;
	*ret = strtoul(s, &t, 0);
	if (errno || *t != '\0') {
		return -1;
	}
	return 0;
}

static __dead void
usage(const char *av0)
{
	fprintf(stderr, "Usage: %s [-s] [-p portnumber|socketpath]\n", av0);
	exit(1);
}

static void
makeaddr(const char *path, uint16_t port,
	 struct sockaddr_storage *ss, socklen_t *len)
{
	struct sockaddr_in *sin;
	struct sockaddr_un *sun;

	if (path == NULL) {
		sin = (struct sockaddr_in *)ss;
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = INADDR_ANY;
		sin->sin_port = htons(port);
		*len = sizeof(*sin);
	} else {
		sun = (struct sockaddr_un *)ss;
		sun->sun_family = AF_UNIX;
		strlcpy(sun->sun_path, path, sizeof(sun->sun_path));
		*len = SUN_LEN(sun);
	}
}

static uint16_t
getsockport(int sock)
{
	struct sockaddr_storage addr;
	socklen_t len;

	len = sizeof(addr);
	if (getsockname(sock, (struct sockaddr *)&addr, &len) < 0)  {
		complain(LOG_ERR, "getsockname");
		exit(1);
	}
	switch (addr.ss_family) {
	    case AF_INET:
		return ntohs(((struct sockaddr_in *)&addr)->sin_port);
	    case AF_INET6:
		return ntohs(((struct sockaddr_in6 *)&addr)->sin6_port);
	    default:
		break;
	}
	return 0;
}

/*
 * main:
 *	The main program.
 */
int
main(int ac, char **av)
{
	PLAYER *pp;
	unsigned long optargnum;
	uint16_t msg, reply;
	struct sockaddr_storage msgaddr;
	socklen_t msgaddrlen;
	static bool first = true;
	static bool server = false;
	int c, i;
	const int linger = 90 * 1000;

	while ((c = getopt(ac, av, "sp:")) != -1) {
		switch (c) {
		  case 's':
			server = true;
			break;
		  case 'p':
			standard_port = false;
			if (getnum(optarg, &optargnum) < 0) {
				localmode = true;
				huntsockpath = optarg;
			} else if (optargnum < 0xffff) {
				localmode = false;
				contactport = optargnum;
			} else {
				usage(av[0]);
			}
			break;
		  default:
			usage(av[0]);
		}
	}
	if (optind < ac)
		usage(av[0]);

	asprintf(&statsockpath, "%s.stats", huntsockpath);
	init();


again:
	do {
		errno = 0;
		while (poll(fdset, 3+MAXPL+MAXMON, INFTIM) < 0)
		{
			if (errno != EINTR)
				complain(LOG_WARNING, "poll");
			errno = 0;
		}
		if (!localmode && fdset[2].revents & POLLIN) {
			msgaddrlen = sizeof(msgaddr);
			(void) recvfrom(contactsock, &msg, sizeof msg,
				0, (struct sockaddr *)&msgaddr, &msgaddrlen);
			switch (ntohs(msg)) {
			  case C_MESSAGE:
				if (Nplayer <= 0)
					break;
				reply = htons((u_short) Nplayer);
				(void) sendto(contactsock, &reply,
					sizeof reply, 0,
					(struct sockaddr *)&msgaddr,
					msgaddrlen);
				break;
			  case C_SCORES:
				reply = htons(statport);
				(void) sendto(contactsock, &reply,
					sizeof reply, 0,
					(struct sockaddr *)&msgaddr,
					msgaddrlen);
				break;
			  case C_PLAYER:
			  case C_MONITOR:
				if (msg == C_MONITOR && Nplayer <= 0)
					break;
				reply = htons(huntport);
				(void) sendto(contactsock, &reply,
					sizeof reply, 0,
					(struct sockaddr *)&msgaddr,
					msgaddrlen);
				break;
			}
		}

		{
			for (pp = Player, i = 0; pp < End_player; pp++, i++)
				if (havechar(pp, i + 3)) {
					execute(pp);
					pp->p_nexec++;
				}
#ifdef MONITOR
			for (pp = Monitor, i = 0; pp < End_monitor; pp++, i++)
				if (havechar(pp, i + MAXPL + 3)) {
					mon_execute(pp);
					pp->p_nexec++;
				}
#endif
			moveshots();
			for (pp = Player, i = 0; pp < End_player; )
				if (pp->p_death[0] != '\0')
					zap(pp, true, i + 3);
				else
					pp++, i++;
#ifdef MONITOR
			for (pp = Monitor, i = 0; pp < End_monitor; )
				if (pp->p_death[0] != '\0')
					zap(pp, false, i + MAXPL + 3);
				else
					pp++, i++;
#endif
		}
		if (fdset[0].revents & POLLIN)
			if (answer()) {
				if (first) {
					/* announce start of game? */
				}
				first = false;
			}
		if (fdset[1].revents & POLLIN)
			send_stats();
		for (pp = Player, i = 0; pp < End_player; pp++, i++) {
			if (fdset[i + 3].revents & POLLIN)
				sendcom(pp, READY, pp->p_nexec);
			pp->p_nexec = 0;
			(void) fflush(pp->p_output);
		}
#ifdef MONITOR
		for (pp = Monitor, i = 0; pp < End_monitor; pp++, i++) {
			if (fdset[i + MAXPL + 3].revents & POLLIN)
				sendcom(pp, READY, pp->p_nexec);
			pp->p_nexec = 0;
			(void) fflush(pp->p_output);
		}
#endif
	} while (Nplayer > 0);

	if (poll(fdset, 3+MAXPL+MAXMON, linger) > 0) {
		goto again;
	}
	if (server) {
		clear_scores();
		makemaze();
		clearwalls();
#ifdef BOOTS
		makeboots();
#endif
		first = true;
		goto again;
	}

#ifdef MONITOR
	for (pp = Monitor, i = 0; pp < End_monitor; i++)
		zap(pp, false, i + MAXPL + 3);
#endif
	cleanup(0);
	/* NOTREACHED */
	return(0);
}

/*
 * init:
 *	Initialize the global parameters.
 */
static void
init(void)
{
	int i;
	struct sockaddr_storage stdinaddr;
	struct sockaddr_storage contactaddr;
	socklen_t contactaddrlen;
	socklen_t len;

#ifndef DEBUG
#ifdef TIOCNOTTY
	(void) ioctl(fileno(stdout), TIOCNOTTY, NULL);
#endif
	(void) setpgrp(getpid(), getpid());
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTERM, cleanup);
#endif

	(void) chdir("/var/tmp");	/* just in case it core dumps */
	(void) umask(0);		/* No privacy at all! */
	(void) signal(SIGPIPE, SIG_IGN);

#ifdef LOG
	openlog("huntd", LOG_PID, LOG_DAEMON);
#endif

	/*
	 * check for inetd
	 */
	len = sizeof(stdinaddr);
	if (getsockname(STDIN_FILENO, (struct sockaddr *)&stdinaddr,
			&len) >= 0) {
		inetd_spawned = true;
		/* force localmode, assimilate stdin as appropriate */
		if (stdinaddr.ss_family == AF_UNIX) {
			localmode = true;
			contactsock = -1;
			huntsock = STDIN_FILENO;
		}
		else {
			localmode = false;
			contactsock = STDIN_FILENO;
			huntsock = -1;
		}
	} else {
		/* keep value of localmode; no sockets yet */
		contactsock = -1;
		huntsock = -1;
	}

	/*
	 * initialize contact socket
	 */
	if (!localmode && contactsock < 0) {
		makeaddr(NULL, contactport, &contactaddr, &contactaddrlen);
		contactsock = socket(AF_INET, SOCK_DGRAM, 0);
		if (bind(contactsock, (struct sockaddr *) &contactaddr,
			 contactaddrlen) < 0) {
			complain(LOG_ERR, "bind");
			exit(1);
		}
		(void) listen(contactsock, 5);
	}

	/*
	 * Initialize main socket
	 */
	if (huntsock < 0) {
		makeaddr(localmode ? huntsockpath : NULL, 0, &huntaddr,
			 &huntaddrlen);
		huntsock = socket(huntaddr.ss_family, SOCK_STREAM, 0);
		if (bind(huntsock, (struct sockaddr *)&huntaddr,
			 huntaddrlen) < 0) {
			if (errno == EADDRINUSE)
				exit(0);
			else {
				complain(LOG_ERR, "bind");
				cleanup(1);
			}
		}
		(void) listen(huntsock, 5);
	}

	/*
	 * Initialize statistics socket
	 */
	makeaddr(localmode ? statsockpath : NULL, 0, &stataddr, &stataddrlen);
	statsock = socket(stataddr.ss_family, SOCK_STREAM, 0);
	if (bind(statsock, (struct sockaddr *)&stataddr, stataddrlen) < 0) {
		if (errno == EADDRINUSE)
			exit(0);
		else {
			complain(LOG_ERR, "bind");
			cleanup(1);
		}
	}
	(void) listen(statsock, 5);

	if (!localmode) {
		contactport = getsockport(contactsock);
		statport = getsockport(statsock);
		huntport = getsockport(huntsock);
		if (contactport != TEST_PORT) {
			standard_port = false;
		}
	}

	/*
	 * Initialize minimal poll mask
	 */
	fdset[0].fd = huntsock;
	fdset[0].events = POLLIN;
	fdset[1].fd = statsock;
	fdset[1].events = POLLIN;
	if (localmode) {
		fdset[2].fd = -1;
		fdset[2].events = 0;
	} else {
		fdset[2].fd = contactsock;
		fdset[2].events = POLLIN;
	}

	srandom(time(NULL));
	makemaze();
#ifdef BOOTS
	makeboots();
#endif

	for (i = 0; i < NASCII; i++)
		See_over[i] = true;
	See_over[DOOR] = false;
	See_over[WALL1] = false;
	See_over[WALL2] = false;
	See_over[WALL3] = false;
#ifdef REFLECT
	See_over[WALL4] = false;
	See_over[WALL5] = false;
#endif

}

#ifdef BOOTS
/*
 * makeboots:
 *	Put the boots in the maze
 */
static void
makeboots(void)
{
	int x, y;
	PLAYER *pp;

	do {
		x = rand_num(WIDTH - 1) + 1;
		y = rand_num(HEIGHT - 1) + 1;
	} while (Maze[y][x] != SPACE);
	Maze[y][x] = BOOT_PAIR;
	for (pp = Boot; pp < &Boot[NBOOTS]; pp++)
		pp->p_flying = -1;
}
#endif


/*
 * checkdam:
 *	Check the damage to the given player, and see if s/he is killed
 */
void
checkdam(PLAYER *ouch, PLAYER *gotcha, IDENT *credit, int amt,
	 char this_shot_type)
{
	const char *cp;

	if (ouch->p_death[0] != '\0')
		return;
#ifdef BOOTS
	if (this_shot_type == SLIME)
		switch (ouch->p_nboots) {
		  default:
			break;
		  case 1:
			amt = (amt + 1) / 2;
			break;
		  case 2:
			if (gotcha != NULL)
				message(gotcha, "He has boots on!");
			return;
		}
#endif
	ouch->p_damage += amt;
	if (ouch->p_damage <= ouch->p_damcap) {
		(void) snprintf(Buf, sizeof(Buf), "%2d", ouch->p_damage);
		cgoto(ouch, STAT_DAM_ROW, STAT_VALUE_COL);
		outstr(ouch, Buf, 2);
		return;
	}

	/* Someone DIED */
	switch (this_shot_type) {
	  default:
		cp = "Killed";
		break;
#ifdef FLY
	  case FALL:
		cp = "Killed on impact";
		break;
#endif
	  case KNIFE:
		cp = "Stabbed to death";
		ouch->p_ammo = 0;		/* No exploding */
		break;
	  case SHOT:
		cp = "Shot to death";
		break;
	  case GRENADE:
	  case SATCHEL:
	  case BOMB:
		cp = "Bombed";
		break;
	  case MINE:
	  case GMINE:
		cp = "Blown apart";
		break;
#ifdef	OOZE
	  case SLIME:
		cp = "Slimed";
		if (credit != NULL)
			credit->i_slime++;
		break;
#endif
#ifdef	VOLCANO
	  case LAVA:
		cp = "Baked";
		break;
#endif
#ifdef DRONE
	  case DSHOT:
		cp = "Eliminated";
		break;
#endif
	}
	if (credit == NULL) {
		(void) snprintf(ouch->p_death, sizeof(ouch->p_death),
			"| %s by %s |", cp,
			(this_shot_type == MINE || this_shot_type == GMINE) ?
			"a mine" : "act of God");
		return;
	}

	(void) snprintf(ouch->p_death, sizeof(ouch->p_death),
		"| %s by %s |", cp, credit->i_name);

	if (ouch == gotcha) {		/* No use killing yourself */
		credit->i_kills--;
		credit->i_bkills++;
	}
	else if (ouch->p_ident->i_team == ' '
	|| ouch->p_ident->i_team != credit->i_team) {
		credit->i_kills++;
		credit->i_gkills++;
	}
	else {
		credit->i_kills--;
		credit->i_bkills++;
	}
	credit->i_score = credit->i_kills / (double) credit->i_entries;
	ouch->p_ident->i_deaths++;
	if (ouch->p_nchar == 0)
		ouch->p_ident->i_stillb++;
	if (gotcha == NULL)
		return;
	gotcha->p_damcap += STABDAM;
	gotcha->p_damage -= STABDAM;
	if (gotcha->p_damage < 0)
		gotcha->p_damage = 0;
	(void) snprintf(Buf, sizeof(Buf), "%2d/%2d", gotcha->p_damage,
			gotcha->p_damcap);
	cgoto(gotcha, STAT_DAM_ROW, STAT_VALUE_COL);
	outstr(gotcha, Buf, 5);
	(void) snprintf(Buf, sizeof(Buf), "%3d",
			(gotcha->p_damcap - MAXDAM) / 2);
	cgoto(gotcha, STAT_KILL_ROW, STAT_VALUE_COL);
	outstr(gotcha, Buf, 3);
	(void) snprintf(Buf, sizeof(Buf), "%5.2f", gotcha->p_ident->i_score);
	for (ouch = Player; ouch < End_player; ouch++) {
		cgoto(ouch, STAT_PLAY_ROW + 1 + (gotcha - Player),
			STAT_NAME_COL);
		outstr(ouch, Buf, 5);
	}
#ifdef MONITOR
	for (ouch = Monitor; ouch < End_monitor; ouch++) {
		cgoto(ouch, STAT_PLAY_ROW + 1 + (gotcha - Player),
			STAT_NAME_COL);
		outstr(ouch, Buf, 5);
	}
#endif
}

/*
 * zap:
 *	Kill off a player and take him out of the game.
 */
static void
zap(PLAYER *pp, bool was_player, int i)
{
	int n, len;
	BULLET *bp;
	PLAYER *np;
	int x, y;

	if (was_player) {
		if (pp->p_undershot)
			fixshots(pp->p_y, pp->p_x, pp->p_over);
		drawplayer(pp, false);
		Nplayer--;
	}

	len = strlen(pp->p_death);	/* Display the cause of death */
	x = (WIDTH - len) / 2;
	cgoto(pp, HEIGHT / 2, x);
	outstr(pp, pp->p_death, len);
	for (n = 1; n < len; n++)
		pp->p_death[n] = '-';
	pp->p_death[0] = '+';
	pp->p_death[len - 1] = '+';
	cgoto(pp, HEIGHT / 2 - 1, x);
	outstr(pp, pp->p_death, len);
	cgoto(pp, HEIGHT / 2 + 1, x);
	outstr(pp, pp->p_death, len);
	cgoto(pp, HEIGHT, 0);

#ifdef MONITOR
	if (was_player) {
#endif
		for (bp = Bullets; bp != NULL; bp = bp->b_next) {
			if (bp->b_owner == pp)
				bp->b_owner = NULL;
			if (bp->b_x == pp->p_x && bp->b_y == pp->p_y)
				bp->b_over = SPACE;
		}

		n = rand_num(pp->p_ammo);
		x = rand_num(pp->p_ammo);
		if (x > n)
			n = x;
		if (pp->p_ammo == 0)
			x = 0;
		else if (n == pp->p_ammo - 1) {
			x = pp->p_ammo;
			len = SLIME;
		}
		else {
			for (x = MAXBOMB - 1; x > 0; x--)
				if (n >= shot_req[x])
					break;
			for (y = MAXSLIME - 1; y > 0; y--)
				if (n >= slime_req[y])
					break;
			if (y >= 0 && slime_req[y] > shot_req[x]) {
				x = slime_req[y];
				len = SLIME;
			}
			else if (x != 0) {
				len = shot_type[x];
				x = shot_req[x];
			}
		}
		if (x > 0) {
			(void) add_shot(len, pp->p_y, pp->p_x, pp->p_face, x,
				NULL, true, SPACE);
			(void) snprintf(Buf, sizeof(Buf), "%s detonated.",
				pp->p_ident->i_name);
			for (np = Player; np < End_player; np++)
				message(np, Buf);
#ifdef MONITOR
			for (np = Monitor; np < End_monitor; np++)
				message(np, Buf);
#endif
#ifdef BOOTS
			while (pp->p_nboots-- > 0) {
				for (np = Boot; np < &Boot[NBOOTS]; np++)
					if (np->p_flying < 0)
						break;
				if (np >= &Boot[NBOOTS])
					err(1, "Too many boots");
				np->p_undershot = false;
				np->p_x = pp->p_x;
				np->p_y = pp->p_y;
				np->p_flying = rand_num(20);
				np->p_flyx = 2 * rand_num(6) - 5;
				np->p_flyy = 2 * rand_num(6) - 5;
				np->p_over = SPACE;
				np->p_face = BOOT;
				showexpl(np->p_y, np->p_x, BOOT);
			}
#endif
		}
#ifdef BOOTS
		else if (pp->p_nboots > 0) {
			if (pp->p_nboots == 2)
				Maze[pp->p_y][pp->p_x] = BOOT_PAIR;
			else
				Maze[pp->p_y][pp->p_x] = BOOT;
			if (pp->p_undershot)
				fixshots(pp->p_y, pp->p_x,
					Maze[pp->p_y][pp->p_x]);
		}
#endif

#ifdef VOLCANO
		volcano += pp->p_ammo - x;
		if (rand_num(100) < volcano / 50) {
			do {
				x = rand_num(WIDTH / 2) + WIDTH / 4;
				y = rand_num(HEIGHT / 2) + HEIGHT / 4;
			} while (Maze[y][x] != SPACE);
			(void) add_shot(LAVA, y, x, LEFTS, volcano,
				NULL, true, SPACE);
			for (np = Player; np < End_player; np++)
				message(np, "Volcano eruption.");
			volcano = 0;
		}
#endif

#ifdef DRONE
		if (rand_num(100) < 2) {
			do {
				x = rand_num(WIDTH / 2) + WIDTH / 4;
				y = rand_num(HEIGHT / 2) + HEIGHT / 4;
			} while (Maze[y][x] != SPACE);
			add_shot(DSHOT, y, x, rand_dir(),
				shot_req[MINDSHOT +
				rand_num(MAXBOMB - MINDSHOT)],
				NULL, false, SPACE);
		}
#endif

		sendcom(pp, ENDWIN);
		(void) putc(' ', pp->p_output);
		(void) fclose(pp->p_output);

		End_player--;
		if (pp != End_player) {
			memcpy(pp, End_player, sizeof (PLAYER));
			fdset[i] = fdset[End_player - Player + 3];
			fdset[End_player - Player + 3].fd = -1;
			(void) snprintf(Buf, sizeof(Buf), "%5.2f%c%-10.10s %c",
				pp->p_ident->i_score, stat_char(pp),
				pp->p_ident->i_name, pp->p_ident->i_team);
			n = STAT_PLAY_ROW + 1 + (pp - Player);
			for (np = Player; np < End_player; np++) {
				cgoto(np, n, STAT_NAME_COL);
				outstr(np, Buf, STAT_NAME_LEN);
			}
#ifdef MONITOR
			for (np = Monitor; np < End_monitor; np++) {
				cgoto(np, n, STAT_NAME_COL);
				outstr(np, Buf, STAT_NAME_LEN);
			}
#endif
		} else
			fdset[i].fd = -1;

		/* Erase the last player */
		n = STAT_PLAY_ROW + 1 + Nplayer;
		for (np = Player; np < End_player; np++) {
			cgoto(np, n, STAT_NAME_COL);
			ce(np);
		}
#ifdef MONITOR
		for (np = Monitor; np < End_monitor; np++) {
			cgoto(np, n, STAT_NAME_COL);
			ce(np);
		}
	}
	else {
		sendcom(pp, ENDWIN);
		(void) putc(LAST_PLAYER, pp->p_output);
		(void) fclose(pp->p_output);

		End_monitor--;
		if (pp != End_monitor) {
			memcpy(pp, End_monitor, sizeof (PLAYER));
			fdset[i] = fdset[End_monitor - Monitor + MAXPL + 3];
			fdset[End_monitor - Monitor + MAXPL + 3].fd = -1;
			(void) snprintf(Buf, sizeof(Buf), "%5.5s %-10.10s %c",
				" ",
				pp->p_ident->i_name, pp->p_ident->i_team);
			n = STAT_MON_ROW + 1 + (pp - Player);
			for (np = Player; np < End_player; np++) {
				cgoto(np, n, STAT_NAME_COL);
				outstr(np, Buf, STAT_NAME_LEN);
			}
			for (np = Monitor; np < End_monitor; np++) {
				cgoto(np, n, STAT_NAME_COL);
				outstr(np, Buf, STAT_NAME_LEN);
			}
		} else
			fdset[i].fd = -1;

		/* Erase the last monitor */
		n = STAT_MON_ROW + 1 + (End_monitor - Monitor);
		for (np = Player; np < End_player; np++) {
			cgoto(np, n, STAT_NAME_COL);
			ce(np);
		}
		for (np = Monitor; np < End_monitor; np++) {
			cgoto(np, n, STAT_NAME_COL);
			ce(np);
		}
	}
#endif
}

/*
 * rand_num:
 *	Return a random number in a given range.
 */
int
rand_num(int range)
{
	return (range == 0 ? 0 : random() % range);
}

/*
 * havechar:
 *	Check to see if we have any characters in the input queue; if
 *	we do, read them, stash them away, and return true; else return
 *	false.
 */
static bool
havechar(PLAYER *pp, int i)
{

	if (pp->p_ncount < pp->p_nchar)
		return true;
	if (!(fdset[i].revents & POLLIN))
		return false;
check_again:
	pp->p_nchar = read(pp->p_fd, pp->p_cbuf, sizeof pp->p_cbuf);
	if (pp->p_nchar < 0 && errno == EINTR) {
		goto check_again;
	} else if (pp->p_nchar <= 0) {
		if (errno == EINTR)
		pp->p_cbuf[0] = 'q';
	}
	pp->p_ncount = 0;
	return true;
}

/*
 * cleanup:
 *	Exit with the given value, cleaning up any droppings lying around
 */
void
cleanup(int exitval)
{
	PLAYER *pp;

	for (pp = Player; pp < End_player; pp++) {
		cgoto(pp, HEIGHT, 0);
		sendcom(pp, ENDWIN);
		(void) putc(LAST_PLAYER, pp->p_output);
		(void) fclose(pp->p_output);
	}
#ifdef MONITOR
	for (pp = Monitor; pp < End_monitor; pp++) {
		cgoto(pp, HEIGHT, 0);
		sendcom(pp, ENDWIN);
		(void) putc(LAST_PLAYER, pp->p_output);
		(void) fclose(pp->p_output);
	}
#endif
	(void) close(huntsock);
#ifdef AF_UNIX_HACK
	(void) unlink(huntsockpath);
#endif

	exit(exitval);
}

/*
 * send_stats:
 *	Print stats to requestor
 */
static void
send_stats(void)
{
	IDENT *ip;
	FILE *fp;
	int s;
	struct sockaddr_storage newaddr;
	socklen_t socklen;

	/*
	 * Get the output stream ready
	 */
	socklen = sizeof(newaddr);
	s = accept(statsock, (struct sockaddr *)&newaddr, &socklen);
	if (s < 0) {
		if (errno == EINTR)
			return;
		complain(LOG_WARNING, "accept");
		return;
	}
	fp = fdopen(s, "w");
	if (fp == NULL) {
		complain(LOG_WARNING, "fdopen");
		(void) close(s);
		return;
	}

	/*
	 * Send output to requestor
	 */
	fputs("Name\t\tScore\tDucked\tAbsorb\tFaced\tShot\tRobbed\tMissed\tSlimeK\n", fp);
	for (ip = Scores; ip != NULL; ip = ip->i_next) {
		fprintf(fp, "%s\t", ip->i_name);
		if (strlen(ip->i_name) < 8)
			putc('\t', fp);
		fprintf(fp, "%.2f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
			ip->i_score, ip->i_ducked, ip->i_absorbed,
			ip->i_faced, ip->i_shot, ip->i_robbed,
			ip->i_missed, ip->i_slime);
	}
	fputs("\n\nName\t\tEnemy\tFriend\tDeaths\tStill\tSaved\n", fp);
	for (ip = Scores; ip != NULL; ip = ip->i_next) {
		if (ip->i_team == ' ') {
			fprintf(fp, "%s\t", ip->i_name);
			if (strlen(ip->i_name) < 8)
				putc('\t', fp);
		}
		else {
			fprintf(fp, "%s[%c]\t", ip->i_name, ip->i_team);
			if (strlen(ip->i_name) + 3 < 8)
				putc('\t', fp);
		}
		fprintf(fp, "%d\t%d\t%d\t%d\t%d\n",
			ip->i_gkills, ip->i_bkills, ip->i_deaths,
			ip->i_stillb, ip->i_saved);
	}

	(void) fclose(fp);
}

/*
 * clear_scores:
 *	Clear out the scores so the next session start clean
 */
static void
clear_scores(void)
{
	IDENT *ip, *nextip;

	for (ip = Scores; ip != NULL; ip = nextip) {
		nextip = ip->i_next;
		(void) free(ip);
	}
	Scores = NULL;
}
