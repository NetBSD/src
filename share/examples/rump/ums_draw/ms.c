/*      $NetBSD: ms.c,v 1.1 2010/01/11 02:18:45 pooka Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Experimental proof-of-concept program:
 *
 * Read mouse events and draw silly cursor on screen.
 */

#include <sys/types.h>
#include <sys/time.h>

#include <dev/wscons/wsconsio.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <curses.h>
#include <err.h>
#include <paths.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	struct wscons_event *wev;
	char buf[128];
	int fd, x = 0, y = 0;

	rump_boot_sethowto(RUMP_AB_VERBOSE);
	rump_init();

	fd = rump_sys_open("/dev/wsmouse", 0);
	if (fd == -1)
		err(1, "open");

	initscr();

	while (rump_sys_read(fd, buf, sizeof(buf)) > 0) {
		/* XXX: timespec in 5.0 vs. -current */
		wev = (void *)buf;

		switch (wev->type) {
		case WSCONS_EVENT_MOUSE_DELTA_X:
			if (wev->value > 1)
				x++;
			else if (wev->value < 1)
				x--;
			if (x < 0)
				x = 0;
			break;
		case WSCONS_EVENT_MOUSE_DELTA_Y:
			if (wev->value > 1)
				y--;
			else if (wev->value < 1)
				y++;
			if (y < 0)
				y = 0;
			break;
		case WSCONS_EVENT_MOUSE_DOWN:
			mvprintw(0, 0, "button %d pressed", wev->value);
			break;
		case WSCONS_EVENT_MOUSE_UP:
			clear();
			break;
		default:
			break;
		}
		move(y, x);
		refresh();
	}
}
