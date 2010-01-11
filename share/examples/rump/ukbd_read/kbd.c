/*      $NetBSD: kbd.c,v 1.1 2010/01/11 02:16:51 pooka Exp $	*/

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
 * Read keyboard events from a USB keyboard using rump drivers.
 */

#include <sys/types.h>
#include <sys/time.h>

#include <dev/wscons/wsconsio.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <err.h>
#include <paths.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SHIFT (-0x20)

int
main(int argc, char *argv[])
{
	struct wscons_event *wev;
	int shift = 0;
	char buf[128];
	int fd;

	rump_boot_sethowto(RUMP_AB_VERBOSE);
	rump_init();

	fd = rump_sys_open("/dev/wskbd", 0);
	if (fd == -1)
		err(1, "open");

	while (rump_sys_read(fd, buf, sizeof(buf)) > 0) {
		const char *typestr;

		/* XXX: timespec in 5.0 vs. -current */
		wev = (void *)buf;

		switch (wev->type) {
		case WSCONS_EVENT_KEY_UP:
			typestr = "up";
			if (wev->value == 0xe1 || wev->value == 0xe5)
				shift = 0;
			break;
		case WSCONS_EVENT_KEY_DOWN:
			typestr = "down";
			if (wev->value == 0xe1 || wev->value == 0xe5)
				shift = SHIFT;
			break;
		default:
			typestr = "unknown";
			break;
		}
		printf("event type: %d (%s)\n", wev->type, typestr);
		printf("value 0x%x", wev->value);
		/*
		 * There's probably a value-to-readable tool somewhere
		 * in the tree, but i'm not sure where or how to use it,
		 * so I'll just punt with the supersimple version for now.
		 */
		if (wev->value >= 0x04 && wev->value <= 0x1d)
			printf(" (%c)", wev->value - 0x04 + 'a' + shift);
		printf("\n");
	}
}
