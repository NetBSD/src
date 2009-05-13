/*	$NetBSD: irdaattach.c,v 1.7.8.1 2009/05/13 19:20:25 jym Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Inspired by slattach.c.
 */

#include <sys/ioctl.h>
#include <dev/ir/irdaio.h>

#include <err.h>
#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <util.h>

int	main(int, char **);
void	usage(void);

int
main(int argc, char **argv)
{
	int fd;
	char *dev, devbuf[100];
	const char *donglename = "none";
	struct termios tty;
	tcflag_t cflag = HUPCL;
	int ch;
	sigset_t nsigset;
	int opt_detach = 1;
	int pr_pid = 0;
	int pr_frame = 0;
	int frdev;
	int dongle;

	while ((ch = getopt(argc, argv, "d:fhHlmnp")) != -1) {
		switch (ch) {
		case 'd':
			donglename = optarg;
			break;
		case 'f':
			pr_frame = 1;
			break;
		case 'h':
			cflag |= CRTSCTS;
			break;
		case 'H':
			cflag |= CDTRCTS;
			break;
		case 'l':
			cflag |= CLOCAL;
			break;
		case 'm':
			cflag &= ~HUPCL;
			break;
		case 'n':
			opt_detach = 0;
			break;
		case 'p':
			pr_pid = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	dev = *argv;
	if (strncmp(_PATH_DEV, dev, sizeof(_PATH_DEV) - 1)) {
		(void)snprintf(devbuf, sizeof(devbuf),
		    "%s%s", _PATH_DEV, dev);
		dev = devbuf;
	}
	if ((fd = open(dev, O_RDWR | O_NDELAY)) < 0)
		err(1, "%s", dev);
	tty.c_cflag = CREAD | CS8 | cflag;
	tty.c_iflag = 0;
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;
	cfsetspeed(&tty, 9600);
	if (tcsetattr(fd, TCSADRAIN, &tty) < 0)
		err(1, "tcsetattr");
	if (ioctl(fd, TIOCSDTR, 0) < 0)
		err(1, "TIOCSDTR");
	if (ioctl(fd, TIOCSLINED, "irframe") < 0)
		err(1, "TIOCSLINED");
	if (pr_frame) {
		if (ioctl(fd, IRFRAMETTY_GET_DEVICE, &frdev) < 0)
			err(1, "IRFRAMETTY_GET_DEVICE");
		printf("%sirframe%d\n", _PATH_DEV, frdev);
	}
	if (strcmp(donglename, "none") == 0)
		dongle = DONGLE_NONE;
	else if (strcmp(donglename, "tekram") == 0)
		dongle = DONGLE_TEKRAM;
	else if (strcmp(donglename, "jeteye") == 0)
		dongle = DONGLE_JETEYE;
	else if (strcmp(donglename, "actisys") == 0)
		dongle = DONGLE_ACTISYS;
	else if (strcmp(donglename, "actisys+") == 0)
		dongle = DONGLE_ACTISYS_PLUS;
	else if (strcmp(donglename, "litelink") == 0)
		dongle = DONGLE_LITELINK;
	else if (strcmp(donglename, "girbil") == 0)
		dongle = DONGLE_GIRBIL;
	else
		errx(1, "Unknown dongle");
	if (ioctl(fd, IRFRAMETTY_SET_DONGLE, &dongle) < 0)
		err(1, "IRFRAMETTY_SET_DONGLE");

	fflush(stdout);
	if (opt_detach && daemon(0, 0) != 0)
		err(1, "couldn't detach");
	if (pr_pid)
		pidfile(NULL);
	sigemptyset(&nsigset);
	for (;;)
		sigsuspend(&nsigset);
}

void
usage()
{

	fprintf(stderr, "usage: %s [-d donglename] [-fhHlmnp] ttyname\n",
		getprogname());
	exit(1);
}
