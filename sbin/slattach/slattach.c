/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

/*
 * Hacked to change from sgtty to POSIX termio style serial line control
 * and added flag to enable cts/rts style flow control.
 *
 * blymn@awadi.com.au (Brett Lymn) 93.04.04
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)slattach.c	4.6 (Berkeley) 6/1/90";*/
static char rcsid[] = "$Id: slattach.c,v 1.11 1994/02/10 18:03:23 cgd Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <paths.h>

#define DEFAULT_BAUD	9600

static char usage_str[] = "\
usage: %s [-h] [-m] [-s <speed>] <device>\n\
	-h -- turn on CTS/RTS style flow control\n\
	-m -- maintain DTR after last close (no HUPCL)\n\
	-s -- baud rate (default 9600)\n";

int main(int argc, char **argv)
{
	struct termios tty;
	int option;
	int fd;
	char devname[32];
	char *dev = (char *)0;
	int slipdisc = SLIPDISC;
	int speed = DEFAULT_BAUD;
	int cflags = HUPCL;

	extern char *optarg;
	extern int optind;

	while ((option = getopt(argc, argv, "achmns:")) != EOF) {
		switch (option) {
		case 'h':
			cflags |= CRTSCTS;
			break;
		case 'm':
			cflags &= ~HUPCL;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case '?':
		default:
			fprintf(stderr, usage_str, argv[0]);
			exit(1);
		}
	}

	if (optind == argc - 1)
		dev = argv[optind];

	if (dev == (char *)0) {
		fprintf(stderr, usage_str, argv[0]);
		exit(2);
	}

	if (strncmp(_PATH_DEV, dev, sizeof(_PATH_DEV) - 1)) {
		strcpy(devname, _PATH_DEV);
		strcat(devname, "/");
		strncat(devname, dev, 10);
		dev = devname;
	}

	if ((fd = open(dev, O_RDWR | O_NDELAY)) < 0) {
		perror(dev);
		exit(1);
	}

	if (tcgetattr(fd, &tty) < 0) {
		perror("tcgetattr");
		close(fd);
		exit(1);
	}
	tty.c_iflag = 0;
	tty.c_oflag = 0;
	tty.c_cflag = CREAD | CS8 | cflags;
	tty.c_lflag = 0;
	tty.c_cc[VMIN] = 1; /* wait for one char */
	tty.c_cc[VTIME] = 0; /* wait forever for a char */
	cfsetispeed(&tty, speed);
	cfsetospeed(&tty, speed);
	if (tcsetattr(fd, TCSADRAIN, &tty) < 0) {
		perror("tcsetattr");
		close(fd);
		exit(1);
	}

	if (ioctl(fd, TIOCSDTR) < 0) {
                perror("ioctl(TIOCSDTR)");
                close(fd);
                exit(1);
        }

	if (ioctl(fd, TIOCSETD, &slipdisc) < 0) {
		perror("ioctl(TIOCSETD)");
		close(fd);
		exit(1);
	}

	if (fork() > 0)
		exit(0);

	for (;;)
		sigpause(0L);
}
