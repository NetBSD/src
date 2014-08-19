/*	$NetBSD: hunt.c,v 1.41.8.1 2014/08/20 00:00:23 tls Exp $	*/
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
__RCSID("$NetBSD: hunt.c,v 1.41.8.1 2014/08/20 00:00:23 tls Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "hunt_common.h"
#include "pathnames.h"
#include "hunt_private.h"


#ifdef OVERRIDE_PATH_HUNTD
static const char Driver[] = OVERRIDE_PATH_HUNTD;
#else
static const char Driver[] = PATH_HUNTD;
#endif

#ifdef INTERNET
static const char *contactportstr;
static uint16_t contactport = TEST_PORT;
static const char *contacthost;
#else
static const char huntsockpath[] = PATH_HUNTSOCKET;
#endif


bool Last_player = false;
#ifdef MONITOR
bool Am_monitor = false;
#endif

char Buf[BUFSIZ];

/*static*/ int huntsocket;
#ifdef INTERNET
char *Send_message = NULL;
#endif

SOCKET Daemon;
#ifdef INTERNET
#define DAEMON_SIZE	(sizeof Daemon)
#else
#define DAEMON_SIZE	(sizeof Daemon - 1)
#endif

char map_key[256];			/* what to map keys to */
bool no_beep;

static char name[WIRE_NAMELEN];
static char team = ' ';

static int in_visual;

extern int cur_row, cur_col;

static void dump_scores(const struct sockaddr_storage *, socklen_t);
static int env_init(int);
static void fill_in_blanks(void);
static void fincurs(void);
static void rmnl(char *);
static void sigterm(int) __dead;
static void sigusr1(int) __dead;
static void find_driver(void);
static void start_driver(void);

extern int Otto_mode;

static const char *
lookuphost(const struct sockaddr_storage *host, socklen_t hostlen)
{
	static char buf[NI_MAXHOST];
	int result;

	result = getnameinfo((const struct sockaddr *)host, hostlen,
			     buf, sizeof(buf), NULL, 0, NI_NOFQDN);
	if (result) {
		leavex(1, "getnameinfo: %s", gai_strerror(result));
	}
	return buf;
}

/*
 * main:
 *	Main program for local process
 */
int
main(int ac, char **av)
{
	char *term;
	int c;
	int enter_status;
	bool Query_driver = false;
	bool Show_scores = false;

	enter_status = env_init(Q_CLOAK);
	while ((c = getopt(ac, av, "Sbcfh:l:mn:op:qst:w:")) != -1) {
		switch (c) {
		case 'l':	/* rsh compatibility */
		case 'n':
			(void) strncpy(name, optarg, sizeof(name));
			break;
		case 't':
			team = *optarg;
			if (!isdigit((unsigned char)team)) {
				warnx("Team names must be numeric");
				team = ' ';
			}
			break;
		case 'o':
#ifndef OTTO
			warnx("The -o flag is reserved for future use.");
			goto usage;
#else
			Otto_mode = true;
			break;
#endif
		case 'm':
#ifdef MONITOR
			Am_monitor = true;
#else
			warnx("The monitor was not compiled in.");
#endif
			break;
#ifdef INTERNET
		case 'S':
			Show_scores = true;
			break;
		case 'q':	/* query whether hunt is running */
			Query_driver = true;
			break;
		case 'w':
			Send_message = optarg;
			break;
		case 'h':
			contacthost = optarg;
			break;
		case 'p':
			contactportstr = optarg;
			contactport = atoi(contactportstr);
			break;
#else
		case 'S':
		case 'q':
		case 'w':
		case 'h':
		case 'p':
			wanrx("Need TCP/IP for S, q, w, h, and p options.");
			break;
#endif
		case 'c':
			enter_status = Q_CLOAK;
			break;
		case 'f':
#ifdef FLY
			enter_status = Q_FLY;
#else
			warnx("The flying code was not compiled in.");
#endif
			break;
		case 's':
			enter_status = Q_SCAN;
			break;
		case 'b':
			no_beep = !no_beep;
			break;
		default:
		usage:
			fputs(
"usage:\thunt [-qmcsfS] [-n name] [-t team] [-p port] [-w message] [host]\n",
			stderr);
			exit(1);
		}
	}
#ifdef INTERNET
	if (optind + 1 < ac)
		goto usage;
	else if (optind + 1 == ac)
		contacthost = av[ac - 1];
#else
	if (optind < ac)
		goto usage;
#endif

#ifdef INTERNET
	serverlist_setup(contacthost, contactport);

	if (Show_scores) {
		const struct sockaddr_storage *host;
		socklen_t hostlen;
		u_short msg = C_SCORES;
		unsigned i;

		serverlist_query(msg);
		for (i = 0; i < serverlist_num(); i++) {
			host = serverlist_gethost(i, &hostlen);
			dump_scores(host, hostlen);
		}
		exit(0);
	}
	if (Query_driver) {
		const struct sockaddr_storage *host;
		socklen_t hostlen;
		u_short msg = C_MESSAGE;
		u_short num_players;
		unsigned i;

		serverlist_query(msg);
		for (i = 0; i < serverlist_num(); i++) {
			host = serverlist_gethost(i, &hostlen);
			num_players = ntohs(serverlist_getresponse(i));

			printf("%d player%s hunting on %s!\n",
				num_players, (num_players == 1) ? "" : "s",
				lookuphost(host, hostlen));
		}
		exit(0);
	}
#endif
#ifdef OTTO
	if (Otto_mode)
		(void) strncpy(name, "otto", sizeof(name));
	else
#endif
	fill_in_blanks();

	(void) fflush(stdout);
	if (!isatty(0) || (term = getenv("TERM")) == NULL)
		errx(1, "no terminal type");
	if (!initscr())
		errx(0, "couldn't initialize screen");
	(void) noecho();
	(void) cbreak();
	in_visual = true;
	if (LINES < SCREEN_HEIGHT || COLS < SCREEN_WIDTH)
		leavex(1, "Need a larger window");
	clear_the_screen();
	(void) signal(SIGINT, intr);
	(void) signal(SIGTERM, sigterm);
	(void) signal(SIGUSR1, sigusr1);
	(void) signal(SIGPIPE, SIG_IGN);

	for (;;) {
#ifdef INTERNET
		find_driver();

		if (Daemon.sin_port == 0)
			leavex(1, "Game not found, try again");

	jump_in:
		do {
			int option;

			huntsocket = socket(SOCK_FAMILY, SOCK_STREAM, 0);
			if (huntsocket < 0)
				err(1, "socket");
			option = 1;
			if (setsockopt(huntsocket, SOL_SOCKET, SO_USELOOPBACK,
			    &option, sizeof option) < 0)
				warn("setsockopt loopback");
			errno = 0;
			if (connect(huntsocket, (struct sockaddr *) &Daemon,
			    DAEMON_SIZE) < 0) {
				if (errno != ECONNREFUSED) {
					leave(1, "connect");
				}
			}
			else
				break;
			sleep(1);
		} while (close(huntsocket) == 0);
#else /* !INTERNET */
		/*
		 * set up a socket
		 */

		if ((huntsocket = socket(SOCK_FAMILY, SOCK_STREAM, 0)) < 0)
			err(1, "socket");

		/*
		 * attempt to connect the socket to a name; if it fails that
		 * usually means that the driver isn't running, so we start
		 * up the driver.
		 */

		Daemon.sun_family = SOCK_FAMILY;
		(void) strcpy(Daemon.sun_path, huntsockpath);
		if (connect(huntsocket, &Daemon, DAEMON_SIZE) < 0) {
			if (errno != ENOENT) {
				leavex(1, "connect2");
			}
			start_driver();

			do {
				(void) close(huntsocket);
				if ((huntsocket = socket(SOCK_FAMILY, SOCK_STREAM,
				    0)) < 0)
					err(1, "socket");
				sleep(2);
			} while (connect(huntsocket, &Daemon, DAEMON_SIZE) < 0);
		}
#endif

		do_connect(name, sizeof(name), team, enter_status);
#ifdef INTERNET
		if (Send_message != NULL) {
			do_message();
			if (enter_status == Q_MESSAGE)
				break;
			Send_message = NULL;
			/* don't continue as that will call find_driver */
			goto jump_in;
		}
#endif
		playit();
		if ((enter_status = quit(enter_status)) == Q_QUIT)
			break;
	}
	leavex(0, NULL);
	/* NOTREACHED */
	return(0);
}

#ifdef INTERNET
static void
find_driver(void)
{
	u_short msg;
	const struct sockaddr_storage *host;
	socklen_t hostlen;
	unsigned num;
	int i, c;

	msg = C_PLAYER;
#ifdef MONITOR
	if (Am_monitor) {
		msg = C_MONITOR;
	}
#endif

	serverlist_query(msg);
	num = serverlist_num();
	if (num == 0) {
		start_driver();
		sleep(2);
		/* try again */
		serverlist_query(msg);
		num = serverlist_num();
		if (num == 0) {
			/* give up */
			return;
		}
	}

	if (num == 1) {
		host = serverlist_gethost(0, &hostlen);
	} else {
		clear_the_screen();
		move(1, 0);
		addstr("Pick one:");
		for (i = 0; i < HEIGHT - 4 && i < (int)num; i++) {
			move(3 + i, 0);
			host = serverlist_gethost(i, &hostlen);
			printw("%8c    %.64s", 'a' + i,
			       lookuphost(host, hostlen));
		}
		move(4 + i, 0);
		addstr("Enter letter: ");
		refresh();
		while (1) {
			c = getchar();
			if (c == EOF) {
				leavex(1, "EOF on stdin");
			}
			if (islower((unsigned char)c) && c - 'a' < i) {
				break;
			}
			beep();
			refresh();
		}
		clear_the_screen();
		host = serverlist_gethost(c - 'a', &hostlen);
	}

	/* XXX fix this (won't work in ipv6) */
	assert(hostlen == sizeof(Daemon));
	memcpy(&Daemon, host, sizeof(Daemon));
}

static void
dump_scores(const struct sockaddr_storage *host, socklen_t hostlen)
{
	int s;
	char buf[BUFSIZ];
	ssize_t cnt;

	printf("\n%s:\n", lookuphost(host, hostlen));
	fflush(stdout);

	s = socket(host->ss_family, SOCK_STREAM, 0);
	if (s < 0)
		err(1, "socket");
	if (connect(s, (const struct sockaddr *)host, hostlen) < 0)
		err(1, "connect");
	while ((cnt = read(s, buf, BUFSIZ)) > 0)
		write(fileno(stdout), buf, cnt);
	(void) close(s);
}

#endif

static void
start_driver(void)
{
	int procid;

#ifdef MONITOR
	if (Am_monitor) {
		leavex(1, "No one playing.");
		/* NOTREACHED */
	}
#endif

#ifdef INTERNET
	if (contacthost != NULL) {
		sleep(3);
		return;
	}
#endif

	move(HEIGHT, 0);
	addstr("Starting...");
	refresh();
	procid = fork();
	if (procid == -1) {
		leave(1, "fork failed.");
	}
	if (procid == 0) {
		(void) signal(SIGINT, SIG_IGN);
#ifndef INTERNET
		(void) close(huntsocket);
#else
		if (contactportstr == NULL)
#endif
			execl(Driver, "HUNT", (char *) NULL);
#ifdef INTERNET
		else
			execl(Driver, "HUNT", "-p", contactportstr,
			      (char *) NULL);
#endif
		/* only get here if exec failed */
		(void) kill(getppid(), SIGUSR1);	/* tell mom */
		_exit(1);
	}
	move(HEIGHT, 0);
	addstr("Connecting...");
	refresh();
}

/*
 * bad_con:
 *	We had a bad connection.  For the moment we assume that this
 *	means the game is full.
 */
void
bad_con(void)
{
	leavex(1, "The game is full.  Sorry.");
	/* NOTREACHED */
}

/*
 * bad_ver:
 *	version number mismatch.
 */
void
bad_ver(void)
{
	leavex(1, "Version number mismatch. No go.");
	/* NOTREACHED */
}

/*
 * sigterm:
 *	Handle a terminate signal
 */
static void
sigterm(int dummy __unused)
{
	leavex(0, NULL);
	/* NOTREACHED */
}


/*
 * sigusr1:
 *	Handle a usr1 signal
 */
static void
sigusr1(int dummy __unused)
{
	leavex(1, "Unable to start driver.  Try again.");
	/* NOTREACHED */
}

/*
 * rmnl:
 *	Remove a '\n' at the end of a string if there is one
 */
static void
rmnl(char *s)
{
	char *cp;

	cp = strrchr(s, '\n');
	if (cp != NULL)
		*cp = '\0';
}

/*
 * intr:
 *	Handle a interrupt signal
 */
void
intr(int dummy __unused)
{
	int ch;
	bool explained;
	int y, x;

	(void) signal(SIGINT, SIG_IGN);
	getyx(stdscr, y, x);
	move(HEIGHT, 0);
	addstr("Really quit? ");
	clrtoeol();
	refresh();
	explained = false;
	for (;;) {
		ch = getchar();
		if (isupper(ch))
			ch = tolower(ch);
		if (ch == 'y') {
			if (huntsocket != 0) {
				(void) write(huntsocket, "q", 1);
				(void) close(huntsocket);
			}
			leavex(0, NULL);
		}
		else if (ch == 'n') {
			(void) signal(SIGINT, intr);
			move(y, x);
			refresh();
			return;
		}
		if (!explained) {
			addstr("(Yes or No) ");
			refresh();
			explained = true;
		}
		beep();
		refresh();
	}
}

static void
fincurs(void)
{
	if (in_visual) {
		move(HEIGHT, 0);
		refresh();
		endwin();
	}
}

/*
 * leave:
 *	Leave the game somewhat gracefully, restoring all current
 *	tty stats, and print errno.
 */
void
leave(int exitval, const char *fmt, ...)
{
	int serrno = errno;
	va_list ap;

	fincurs();
	va_start(ap, fmt);
	errno = serrno;
	verr(exitval, fmt, ap);
	va_end(ap);
}

/*
 * leavex:
 *	Leave the game somewhat gracefully, restoring all current
 *	tty stats.
 */
void
leavex(int exitval, const char *fmt, ...)
{
	va_list ap;

	fincurs();
	va_start(ap, fmt);
	verrx(exitval, fmt, ap);
	va_end(ap);
}

static int
env_init(int enter_status)
{
	int i;
	char *envp, *envname, *s;

	for (i = 0; i < 256; i++)
		map_key[i] = (char) i;

	envname = NULL;
	if ((envp = getenv("HUNT")) != NULL) {
		while ((s = strpbrk(envp, "=,")) != NULL) {
			if (strncmp(envp, "cloak,", s - envp + 1) == 0) {
				enter_status = Q_CLOAK;
				envp = s + 1;
			}
			else if (strncmp(envp, "scan,", s - envp + 1) == 0) {
				enter_status = Q_SCAN;
				envp = s + 1;
			}
			else if (strncmp(envp, "fly,", s - envp + 1) == 0) {
				enter_status = Q_FLY;
				envp = s + 1;
			}
			else if (strncmp(envp, "nobeep,", s - envp + 1) == 0) {
				no_beep = true;
				envp = s + 1;
			}
			else if (strncmp(envp, "name=", s - envp + 1) == 0) {
				envname = s + 1;
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					strncpy(name, envname, sizeof(name));
					break;
				}
				*s = '\0';
				strncpy(name, envname, sizeof(name));
				envp = s + 1;
			}
#ifdef INTERNET
			else if (strncmp(envp, "port=", s - envp + 1) == 0) {
				contactportstr = s + 1;
				contactport = atoi(contactportstr);
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					break;
				}
				*s = '\0';
				envp = s + 1;
			}
			else if (strncmp(envp, "host=", s - envp + 1) == 0) {
				contacthost = s + 1;
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					break;
				}
				*s = '\0';
				envp = s + 1;
			}
			else if (strncmp(envp, "message=", s - envp + 1) == 0) {
				Send_message = s + 1;
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					break;
				}
				*s = '\0';
				envp = s + 1;
			}
#endif
			else if (strncmp(envp, "team=", s - envp + 1) == 0) {
				team = *(s + 1);
				if (!isdigit((unsigned char)team))
					team = ' ';
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					break;
				}
				*s = '\0';
				envp = s + 1;
			}			/* must be last option */
			else if (strncmp(envp, "mapkey=", s - envp + 1) == 0) {
				for (s = s + 1; *s != '\0'; s += 2) {
					map_key[(unsigned int) *s] = *(s + 1);
					if (*(s + 1) == '\0') {
						break;
					}
				}
				*envp = '\0';
				break;
			} else {
				*s = '\0';
				printf("unknown option %s\n", envp);
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					break;
				}
				envp = s + 1;
			}
		}
		if (*envp != '\0') {
			if (envname == NULL)
				strncpy(name, envp, sizeof(name));
			else
				printf("unknown option %s\n", envp);
		}
	}
	return enter_status;
}

static void
fill_in_blanks(void)
{
	int i;
	char *cp;

again:
	if (name[0] != '\0') {
		printf("Entering as '%s'", name);
		if (team != ' ')
			printf(" on team %c.\n", team);
		else
			putchar('\n');
	} else {
		printf("Enter your code name: ");
		if (fgets(name, sizeof(name), stdin) == NULL)
			exit(1);
	}
	rmnl(name);
	if (name[0] == '\0') {
		name[0] = '\0';
		printf("You have to have a code name!\n");
		goto again;
	}
	for (cp = name; *cp != '\0'; cp++)
		if (!isprint((unsigned char)*cp)) {
			name[0] = '\0';
			printf("Illegal character in your code name.\n");
			goto again;
		}
	if (team == ' ') {
		printf("Enter your team (0-9 or nothing): ");
		i = getchar();
		if (isdigit(i))
			team = i;
		while (i != '\n' && i != EOF)
			i = getchar();
	}
}
