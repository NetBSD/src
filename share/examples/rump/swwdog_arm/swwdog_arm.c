/*	$NetBSD: swwdog_arm.c,v 1.1 2010/01/31 03:11:55 pooka Exp $	*/

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

#include <sys/types.h>
#include <sys/wdog.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(void)
{
	char wname[WDOG_NAMESIZE];
	struct wdog_conf wc;
	struct wdog_mode wm;
	int fd;
	extern int rumpns_swwdog_reboot;

	rumpns_swwdog_reboot = 1;
	rump_init();

	fd = rump_sys_open("/dev/watchdog", O_RDWR);
	if (fd == -1)
		err(1, "open watchdog");

	wc.wc_count = 1;
	wc.wc_names = wname;

	if (rump_sys_ioctl(fd, WDOGIOC_GWDOGS, &wc) == -1)
		err(1, "can't fetch watchdog names");

	if (wc.wc_count) {
		assert(wc.wc_count == 1);
		printf("watchdog available: %s\n", wc.wc_names);

		printf("enabling watchdog with 2s period\n");
		strlcpy(wm.wm_name, wc.wc_names, sizeof(wm.wm_name));
		wm.wm_mode = WDOG_MODE_ETICKLE;
		wm.wm_period = 2;
		if (rump_sys_ioctl(fd, WDOGIOC_SMODE, &wm) == -1)
			err(1, "failed to set tickle\n");

		printf("sleep 1s, then tickle tickle\n");
		sleep(1);
		rump_sys_ioctl(fd, WDOGIOC_TICKLE);

		printf("sleep 1s, then tickle tickle\n");
		sleep(1);
		rump_sys_ioctl(fd, WDOGIOC_TICKLE);

		printf("now, \"forgetting\" to tickle the doggie\n");
		sleep(3);
	} else {
		printf("no puppies\n");
	}

	return 0;
}
