/*	$NetBSD: playit.c,v 1.16.12.1 2014/08/20 00:00:23 tls Exp $	*/
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
__RCSID("$NetBSD: playit.c,v 1.16.12.1 2014/08/20 00:00:23 tls Exp $");
#endif /* not lint */

#include <sys/file.h>
#include <sys/poll.h>
#include <err.h>
#include <errno.h>
#include <curses.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "hunt_common.h"
#include "hunt_private.h"

#ifndef FREAD
#define FREAD	1
#endif


static int nchar_send;
#ifdef OTTO
int Otto_count;
int Otto_mode;
static int otto_y, otto_x;
static char otto_face;
#endif

#define MAX_SEND	5

/*
 * ibuf is the input buffer used for the stream from the driver.
 * It is small because we do not check for user input when there
 * are characters in the input buffer.
 */
static int icnt = 0;
static unsigned char ibuf[256], *iptr = ibuf;

#define GETCHR()	(--icnt < 0 ? getchr() : *iptr++)

static unsigned char getchr(void);
static void send_stuff(void);
static void redraw_screen(void);

/*
 * playit:
 *	Play a given game, handling all the curses commands from
 *	the driver.
 */
void
playit(void)
{
	int ch;
	int y, x;
	uint32_t version;
	ssize_t result;

	result = read(huntsocket, &version, sizeof(version));
	if (result != (ssize_t)sizeof(version)) {
		bad_con();
		/* NOTREACHED */
	}
	if (ntohl(version) != (uint32_t)HUNT_VERSION) {
		bad_ver();
		/* NOTREACHED */
	}
	errno = 0;
#ifdef OTTO
	Otto_count = 0;
#endif
	nchar_send = MAX_SEND;
	while ((ch = GETCHR()) != EOF) {
#ifdef DEBUG
		fputc(ch, stderr);
#endif
		switch (ch & 0377) {
		  case MOVE:
			y = GETCHR();
			x = GETCHR();
			move(y, x);
			break;
		  case ADDCH:
			ch = GETCHR();
#ifdef OTTO
			switch (ch) {

			case '<':
			case '>':
			case '^':
			case 'v':
				otto_face = ch;
				getyx(stdscr, otto_y, otto_x);
				break;
			}
#endif
			addch(ch);
			break;
		  case CLRTOEOL:
			clrtoeol();
			break;
		  case CLEAR:
			clear_the_screen();
			break;
		  case REFRESH:
			refresh();
			break;
		  case REDRAW:
			redraw_screen();
			refresh();
			break;
		  case ENDWIN:
			refresh();
			if ((ch = GETCHR()) == LAST_PLAYER)
				Last_player = true;
			ch = EOF;
			goto out;
		  case BELL:
			beep();
			break;
		  case READY:
			refresh();
			if (nchar_send < 0)
				tcflush(STDIN_FILENO, TCIFLUSH);
			nchar_send = MAX_SEND;
#ifndef OTTO
			(void) GETCHR();
#else
			Otto_count -= (GETCHR() & 0xff);
			if (!Am_monitor) {
#ifdef DEBUG
				fputc('0' + Otto_count, stderr);
#endif
				if (Otto_count == 0 && Otto_mode)
					otto(otto_y, otto_x, otto_face);
			}
#endif
			break;
		  default:
#ifdef OTTO
			switch (ch) {

			case '<':
			case '>':
			case '^':
			case 'v':
				otto_face = ch;
				getyx(stdscr, otto_y, otto_x);
				break;
			}
#endif
			addch(ch);
			break;
		}
	}
out:
	(void) close(huntsocket);
}

/*
 * getchr:
 *	Grab input and pass it along to the driver
 *	Return any characters from the driver
 *	When this routine is called by GETCHR, we already know there are
 *	no characters in the input buffer.
 */
static unsigned char
getchr(void)
{
	struct pollfd set[2];
	int nfds;

	set[0].fd = huntsocket;
	set[0].events = POLLIN;
	set[1].fd = STDIN_FILENO;
	set[1].events = POLLIN;

one_more_time:
	do {
		errno = 0;
		nfds = poll(set, 2, INFTIM);
	} while (nfds <= 0 && errno == EINTR);

	if (set[1].revents && POLLIN)
		send_stuff();
	if (! (set[0].revents & POLLIN))
		goto one_more_time;
	icnt = read(huntsocket, ibuf, sizeof ibuf);
	if (icnt < 0) {
		bad_con();
		/* NOTREACHED */
	}
	if (icnt == 0)
		goto one_more_time;
	iptr = ibuf;
	icnt--;
	return *iptr++;
}

/*
 * send_stuff:
 *	Send standard input characters to the driver
 */
static void
send_stuff(void)
{
	int count;
	char *sp, *nsp;
	static char inp[sizeof Buf];

	count = read(STDIN_FILENO, Buf, sizeof(Buf) - 1);
	if (count <= 0)
		return;
	if (nchar_send <= 0 && !no_beep) {
		(void) beep();
		return;
	}

	/*
	 * look for 'q'uit commands; if we find one,
	 * confirm it.  If it is not confirmed, strip
	 * it out of the input
	 */
	Buf[count] = '\0';
	nsp = inp;
	for (sp = Buf; *sp != '\0'; sp++)
		if ((*nsp = map_key[(unsigned char)*sp]) == 'q')
			intr(0);
		else
			nsp++;
	count = nsp - inp;
	if (count) {
#ifdef OTTO
		Otto_count += count;
#endif
		nchar_send -= count;
		if (nchar_send < 0)
			count += nchar_send;
		(void) write(huntsocket, inp, count);
	}
}

/*
 * quit:
 *	Handle the end of the game when the player dies
 */
int
quit(int old_status)
{
	bool explain;
	int ch;

	if (Last_player)
		return Q_QUIT;
#ifdef OTTO
	if (Otto_mode)
		return Q_CLOAK;
#endif
	move(HEIGHT, 0);
	addstr("Re-enter game [ynwo]? ");
	clrtoeol();
	explain = false;
	for (;;) {
		refresh();
		if (isupper(ch = getchar()))
			ch = tolower(ch);
		if (ch == 'y')
			return old_status;
		else if (ch == 'o')
			break;
		else if (ch == 'n') {
#ifndef INTERNET
			return Q_QUIT;
#else
			move(HEIGHT, 0);
			addstr("Write a parting message [yn]? ");
			clrtoeol();
			refresh();
			for (;;) {
				if (isupper(ch = getchar()))
					ch = tolower(ch);
				if (ch == 'y')
					goto get_message;
				if (ch == 'n')
					return Q_QUIT;
			}
#endif
		}
#ifdef INTERNET
		else if (ch == 'w') {
			static char buf[WIDTH + WIDTH % 2];
			char *cp, c;

get_message:
			c = ch;		/* save how we got here */
			move(HEIGHT, 0);
			addstr("Message: ");
			clrtoeol();
			refresh();
			cp = buf;
			for (;;) {
				refresh();
				if ((ch = getchar()) == '\n' || ch == '\r')
					break;
				if (ch == erasechar()) {
					if (cp > buf) {
						int y, x;
						getyx(stdscr, y, x);
						move(y, x - 1);
						cp -= 1;
						clrtoeol();
					}
					continue;
				}
				else if (ch == killchar()) {
					int y, x;
					getyx(stdscr, y, x);
					move(y, x - (cp - buf));
					cp = buf;
					clrtoeol();
					continue;
				} else if (!isprint(ch)) {
					beep();
					continue;
				}
				addch(ch);
				*cp++ = ch;
				if (cp + 1 >= buf + sizeof buf)
					break;
			}
			*cp = '\0';
			Send_message = buf;
			return (c == 'w') ? old_status : Q_MESSAGE;
		}
#endif
		beep();
		if (!explain) {
			addstr("(Yes, No, Write message, or Options) ");
			explain = true;
		}
	}

	move(HEIGHT, 0);
#ifdef FLY
	addstr("Scan, Cloak, Flying, or Quit? ");
#else
	addstr("Scan, Cloak, or Quit? ");
#endif
	clrtoeol();
	refresh();
	explain = false;
	for (;;) {
		if (isupper(ch = getchar()))
			ch = tolower(ch);
		if (ch == 's')
			return Q_SCAN;
		else if (ch == 'c')
			return Q_CLOAK;
#ifdef FLY
		else if (ch == 'f')
			return Q_FLY;
#endif
		else if (ch == 'q')
			return Q_QUIT;
		beep();
		if (!explain) {
#ifdef FLY
			addstr("[SCFQ] ");
#else
			addstr("[SCQ] ");
#endif
			explain = true;
		}
		refresh();
	}
}

void
clear_the_screen(void)
{
	clear();
	move(0, 0);
	refresh();
}

static void
redraw_screen(void)
{
	clearok(stdscr, TRUE);
	touchwin(stdscr);
}

/*
 * do_message:
 *	Send a message to the driver and return
 */
void
do_message(void)
{
	uint32_t version;
	ssize_t result;

	result = read(huntsocket, &version, sizeof(version));
	if (result != (ssize_t)sizeof(version)) {
		bad_con();
		/* NOTREACHED */
	}
	if (ntohl(version) != (uint32_t)HUNT_VERSION) {
		bad_ver();
		/* NOTREACHED */
	}
#ifdef INTERNET
	if (write(huntsocket, Send_message, strlen(Send_message)) < 0) {
		bad_con();
		/* NOTREACHED */
	}
#endif
	(void) close(huntsocket);
}
