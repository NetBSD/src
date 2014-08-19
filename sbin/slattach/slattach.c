/*	$NetBSD: slattach.c,v 1.32.6.1 2014/08/20 00:02:28 tls Exp $	*/

/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Adams.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1988, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)slattach.c	8.2 (Berkeley) 1/7/94";
#else
__RCSID("$NetBSD: slattach.c,v 1.32.6.1 2014/08/20 00:02:28 tls Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static int	speed = 9600;
static const char	*ldisc = "slip";

static char	devicename[32];

__dead static void	usage(void);

int
main(int argc, char *argv[])
{
	int fd;
	char *dev = argv[1];
	struct termios tty;
	tcflag_t cflag = HUPCL;
	int ch;
	sigset_t nsigset;
	int opt_detach = 1;

	while ((ch = getopt(argc, argv, "hHlmns:t:")) != -1) {
		switch (ch) {
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
		case 's':
			speed = atoi(optarg);
			break;
		case 't':
			ldisc = optarg;
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
		(void)snprintf(devicename, sizeof(devicename),
		    "%s%s", _PATH_DEV, dev);
		dev = devicename;
	}
	if ((fd = open(dev, O_RDWR | O_NDELAY)) < 0)
		err(1, "%s", dev);
	tty.c_cflag = CREAD | CS8 | cflag;
	tty.c_iflag = 0;
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;
	cfsetspeed(&tty, speed);
	if (tcsetattr(fd, TCSADRAIN, &tty) < 0)
		err(1, "tcsetattr");
	if (ioctl(fd, TIOCSDTR, 0) < 0 && errno != ENOTTY)
		err(1, "TIOCSDTR");
	if (ioctl(fd, TIOCSLINED, ldisc) < 0)
		err(1, "TIOCSLINED");
	if (opt_detach && daemon(0, 0) != 0)
		err(1, "couldn't detach");
	sigemptyset(&nsigset);
	for (;;)
		sigsuspend(&nsigset);
}

static void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: %s [-t ldisc] [-hHlmn] [-s baudrate] ttyname\n",
		getprogname());
	exit(1);
}
