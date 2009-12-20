/*	$NetBSD: tipsy.c,v 1.1 2009/12/20 19:50:29 pooka Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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
 * tip-on-booze.  Uses rump kernel for ucom driver + underlying
 * hardware.  Just shovels bits back and forth between the user
 * terminal and the ucom device.
 *
 * Seems to drop an occasional character for output here and there.
 * Haven't pinpointed the problem yet.  It happens commonly for certain
 * patterns such as ctrl-U followed by immediate typing.  Tipsy is quite
 * sober despite this "feature".
 */

#include <sys/types.h>
#include <sys/ioctl.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

/*
 * We use a shovel thread to get the bits from the rump kernel and
 * print them on the terminal.  The reason for a thread instead of
 * polling is that we currently have no way to poll two fd's when
 * they are in different kernels (stdin in the host, ucom in rump).
 */
static void *
shovel(void *arg)
{
	char buf[64];
	ssize_t n;
	int fd = (int)(intptr_t)arg;

	for (;;) {
		n = rump_sys_read(fd, buf, sizeof(buf));
		if (__predict_false(n <= 0)) {
			if (n == 0)
				errx(1, "ucom EOF");
			if (n == -1)
				err(1, "ucom read");
		}
		if (write(STDOUT_FILENO, buf, n) != n)
			err(1, "write to console");
	}
}

int
main(int argc, char *argv[])
{
	pthread_t pt;
        struct termios tios;
	int probeonly = 0;
	int com;

	if (argc > 1) {
		if (argc == 2 && strcmp(argv[1], "probe") == 0) {
			probeonly = 1;
		} else {
			fprintf(stderr, "mind the usage\n");
			exit(1);
		}
	}

	if (probeonly)
		rump_boot_sethowto(RUMP_AB_VERBOSE);
	rump_init();
	if (probeonly)
		exit(0);

	com = rump_sys_open("/dev/dtyU0", O_RDWR);
	if (com == -1)
		err(1, "rump ucom open failed");

	/*
	 * Setup the com port.  You might need to tweak this.
	 */
	if (rump_sys_ioctl(com, TIOCGETA, &tios) == -1)
		err(1, "rump get term");
	tios.c_cflag &= ~(CSIZE|PARENB);
	tios.c_cflag |= CS8;
	tios.c_cflag |= CLOCAL;
	tios.c_iflag &= ~(ISTRIP|ICRNL);
	tios.c_oflag &= ~OPOST;
	tios.c_lflag &= ~(ICANON|ISIG|IEXTEN|ECHO);
	tios.c_cc[VMIN] = 1;
	tios.c_cc[VTIME] = 0;
	if (rump_sys_ioctl(com, TIOCSETA, &tios) == -1)
		err(1, "rump set term");

	/* setup stdin */
	if (tcgetattr(STDIN_FILENO, &tios) == -1)
		err(1, "host get term");
	tios.c_oflag &= ~OPOST;
	tios.c_iflag &= ~(INPCK|ICRNL);
	tios.c_lflag &= ~(ICANON|IEXTEN|ECHO);
	tios.c_cc[VINTR] = tios.c_cc[VQUIT] = tios.c_cc[VSUSP]
	    = tios.c_cc[VDSUSP] = tios.c_cc[VDISCARD] = tios.c_cc[VREPRINT]
	    = tios.c_cc[VLNEXT] = _POSIX_VDISABLE;
        tios.c_cc[VMIN] = 1;
        tios.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSADRAIN, &tios) == -1)
		err(1, "host set term");

	if (pthread_create(&pt, NULL, shovel, (void *)(intptr_t)com) == -1)
		err(1, "pthread create");

	/* read stdin, feed that into the rump kernel */
	for (;;) {
		char ch;

		ch = getchar();
		if (rump_sys_write(com, &ch, 1) != 1)
			err(1, "rump write");
	}
}
