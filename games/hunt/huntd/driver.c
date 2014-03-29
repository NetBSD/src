/*	$NetBSD: driver.c,v 1.25 2014/03/29 20:10:10 dholland Exp $	*/
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
__RCSID("$NetBSD: driver.c,v 1.25 2014/03/29 20:10:10 dholland Exp $");
#endif /* not lint */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include"hunt.h"


static SOCKET Daemon;
char *First_arg;			/* pointer to argv[0] */
char *Last_arg;			/* pointer to end of argv/environ */

#ifdef INTERNET
static int Test_socket;			/* test socket to answer datagrams */
static bool inetd_spawned;		/* invoked via inetd */
static bool standard_port = true;	/* true if listening on standard port */
static u_short	sock_port;		/* port # of tcp listen socket */
static u_short	stat_port;		/* port # of statistics tcp socket */
#define DAEMON_SIZE	(sizeof Daemon)
#else
#define DAEMON_SIZE	(sizeof Daemon - 1)
#endif

static void clear_scores(void);
static bool havechar(PLAYER *, int);
static void init(void);
int main(int, char *[], char *[]);
static void makeboots(void);
static void send_stats(void);
static void zap(PLAYER *, bool, int);


/*
 * main:
 *	The main program.
 */
int
main(int ac, char **av, char **ep)
{
	PLAYER *pp;
#ifdef INTERNET
	u_short msg;
	short reply;
	socklen_t namelen;
	SOCKET test;
#endif
	static bool first = true;
	static bool server = false;
	int c, i;
	const int linger = 90 * 1000;

	First_arg = av[0];
	if (ep == NULL || *ep == NULL)
		ep = av + ac;
	while (*ep)
		ep++;
	Last_arg = ep[-1] + strlen(ep[-1]);

	while ((c = getopt(ac, av, "sp:")) != -1) {
		switch (c) {
		  case 's':
			server = true;
			break;
#ifdef INTERNET
		  case 'p':
			standard_port = false;
			Test_port = atoi(optarg);
			break;
#endif
		  default:
erred:
			fprintf(stderr, "Usage: %s [-s] [-p port]\n", av[0]);
			exit(1);
		}
	}
	if (optind < ac)
		goto erred;

	init();


again:
	do {
		errno = 0;
		while (poll(fdset, 3+MAXPL+MAXMON, INFTIM) < 0)
		{
			if (errno != EINTR)
#ifdef LOG
				syslog(LOG_WARNING, "poll: %m");
#else
				warn("poll");
#endif
			errno = 0;
		}
#ifdef INTERNET
		if (fdset[2].revents & POLLIN) {
			namelen = DAEMON_SIZE;
			(void) recvfrom(Test_socket, &msg, sizeof msg,
				0, (struct sockaddr *) &test, &namelen);
			switch (ntohs(msg)) {
			  case C_MESSAGE:
				if (Nplayer <= 0)
					break;
				reply = htons((u_short) Nplayer);
				(void) sendto(Test_socket, &reply,
					sizeof reply, 0,
					(struct sockaddr *) &test, DAEMON_SIZE);
				break;
			  case C_SCORES:
				reply = htons(stat_port);
				(void) sendto(Test_socket, &reply,
					sizeof reply, 0,
					(struct sockaddr *) &test, DAEMON_SIZE);
				break;
			  case C_PLAYER:
			  case C_MONITOR:
				if (msg == C_MONITOR && Nplayer <= 0)
					break;
				reply = htons(sock_port);
				(void) sendto(Test_socket, &reply,
					sizeof reply, 0,
					(struct sockaddr *) &test, DAEMON_SIZE);
				break;
			}
		}
#endif
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
#ifdef INTERNET
				if (first && standard_port)
					faketalk();
#endif
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
#ifdef INTERNET
	SOCKET test_port;
	int msg;
	socklen_t len;
#endif

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
	 * Initialize statistics socket
	 */
#ifdef INTERNET
	Daemon.sin_family = SOCK_FAMILY;
	Daemon.sin_addr.s_addr = INADDR_ANY;
	Daemon.sin_port = 0;
#else
	Daemon.sun_family = SOCK_FAMILY;
	(void) strcpy(Daemon.sun_path, Stat_name);
#endif

	Status = socket(SOCK_FAMILY, SOCK_STREAM, 0);
	if (bind(Status, (struct sockaddr *) &Daemon, DAEMON_SIZE) < 0) {
		if (errno == EADDRINUSE)
			exit(0);
		else {
#ifdef LOG
			syslog(LOG_ERR, "bind: %m");
#else
			warn("bind");
#endif
			cleanup(1);
		}
	}
	(void) listen(Status, 5);

#ifdef INTERNET
	len = sizeof (SOCKET);
	if (getsockname(Status, (struct sockaddr *) &Daemon, &len) < 0)  {
#ifdef LOG
		syslog(LOG_ERR, "getsockname: %m");
#else
		warn("getsockname");
#endif
		exit(1);
	}
	stat_port = ntohs(Daemon.sin_port);
#endif

	/*
	 * Initialize main socket
	 */
#ifdef INTERNET
	Daemon.sin_family = SOCK_FAMILY;
	Daemon.sin_addr.s_addr = INADDR_ANY;
	Daemon.sin_port = 0;
#else
	Daemon.sun_family = SOCK_FAMILY;
	(void) strcpy(Daemon.sun_path, Sock_name);
#endif

	Socket = socket(SOCK_FAMILY, SOCK_STREAM, 0);
#if defined(INTERNET)
	msg = 1;
	if (setsockopt(Socket, SOL_SOCKET, SO_USELOOPBACK, &msg, sizeof msg)<0)
#ifdef LOG
		syslog(LOG_WARNING, "setsockopt loopback %m");
#else
		warn("setsockopt loopback");
#endif
#endif
	if (bind(Socket, (struct sockaddr *) &Daemon, DAEMON_SIZE) < 0) {
		if (errno == EADDRINUSE)
			exit(0);
		else {
#ifdef LOG
			syslog(LOG_ERR, "bind: %m");
#else
			warn("bind");
#endif
			cleanup(1);
		}
	}
	(void) listen(Socket, 5);

#ifdef INTERNET
	len = sizeof (SOCKET);
	if (getsockname(Socket, (struct sockaddr *) &Daemon, &len) < 0)  {
#ifdef LOG
		syslog(LOG_ERR, "getsockname: %m");
#else
		warn("getsockname");
#endif
		exit(1);
	}
	sock_port = ntohs(Daemon.sin_port);
#endif

	/*
	 * Initialize minimal poll mask
	 */
	fdset[0].fd = Socket;
	fdset[0].events = POLLIN;
	fdset[1].fd = Status;
	fdset[1].events = POLLIN;

#ifdef INTERNET
	len = sizeof (SOCKET);
	if (getsockname(0, (struct sockaddr *) &test_port, &len) >= 0
	&& test_port.sin_family == AF_INET) {
		inetd_spawned = true;
		Test_socket = 0;
		if (test_port.sin_port != htons((u_short) Test_port)) {
			standard_port = false;
			Test_port = ntohs(test_port.sin_port);
		}
	} else {
		test_port = Daemon;
		test_port.sin_port = htons((u_short) Test_port);

		Test_socket = socket(SOCK_FAMILY, SOCK_DGRAM, 0);
		if (bind(Test_socket, (struct sockaddr *) &test_port,
		    DAEMON_SIZE) < 0) {
#ifdef LOG
			syslog(LOG_ERR, "bind: %m");
#else
			warn("bind");
#endif
			exit(1);
		}
		(void) listen(Test_socket, 5);
	}

	fdset[2].fd = Test_socket;
	fdset[2].events = POLLIN;
#else
	fdset[2].fd = -1;
#endif

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
	errno = 0;
	if ((pp->p_nchar = read(pp->p_fd, pp->p_cbuf, sizeof pp->p_cbuf)) <= 0)
	{
		if (errno == EINTR)
			goto check_again;
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
cleanup(int eval)
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
	(void) close(Socket);
#ifdef AF_UNIX_HACK
	(void) unlink(Sock_name);
#endif

	exit(eval);
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
	SOCKET sockstruct;
	socklen_t socklen;

	/*
	 * Get the output stream ready
	 */
#ifdef INTERNET
	socklen = sizeof sockstruct;
#else
	socklen = sizeof sockstruct - 1;
#endif
	s = accept(Status, (struct sockaddr *) &sockstruct, &socklen);
	if (s < 0) {
		if (errno == EINTR)
			return;
#ifdef LOG
		syslog(LOG_WARNING, "accept: %m");
#else
		warn("accept");
#endif
		return;
	}
	fp = fdopen(s, "w");
	if (fp == NULL) {
#ifdef LOG
		syslog(LOG_WARNING, "fdopen: %m");
#else
		warn("fdopen");
#endif
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
