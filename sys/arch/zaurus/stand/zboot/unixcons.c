/*	$NetBSD: unixcons.c,v 1.1.28.1 2014/08/20 00:03:30 tls Exp $	*/

/*
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "boot.h"
#include "bootinfo.h"
#include "unixdev.h"

#include "compat_linux.h"
#include "termios.h"

struct btinfo_console bi_cons;

static int iodev = CONSDEV_GLASS;
static int infd = 0;
static int outfd = 1;

static const char *comdevname[] = {
	"/dev/ttyS0",
};

static void common_putc(int fd, int c);
static int common_getc(int fd, int timo);

void
consinit(int dev, int speed)
{
	struct linux_termios termios;
	int fd;

	switch (dev) {
	case CONSDEV_COM0:
		iodev = dev;
		break;

	case CONSDEV_GLASS:
	default:
 glass_console:
		iodev = CONSDEV_GLASS;
		break;
	}

	if (infd >= 0 && infd == outfd) {
		uclose(infd);
		infd = 0;
		outfd = 1;
	}

	if (iodev == CONSDEV_GLASS) {
		infd = 0;
		outfd = 1;

		strlcpy(bi_cons.devname, "glass", sizeof(bi_cons.devname));
		bi_cons.addr = -1;
		bi_cons.speed = -1;
	} else {
		fd = uopen(comdevname[iodev - CONSDEV_COM0], LINUX_O_RDWR);
		if (fd < 0)
			goto glass_console;
		infd = outfd = fd;

		/* set speed */
		linux_tcgetattr(fd, &termios);
		if (linux_cfsetspeed(&termios, speed) < 0) {
			speed = 9600;
			if (linux_cfsetspeed(&termios, speed) < 0)
				goto glass_console;
		}
		if (linux_tcsetattr(fd, LINUX_TCSETS, &termios) < 0)
			goto glass_console;

		snprintf(bi_cons.devname, sizeof(bi_cons.devname), "com%d",
		    iodev - CONSDEV_COM0);
		bi_cons.addr = -1;
		bi_cons.speed = speed;
	}
	BI_ADD(&bi_cons, BTINFO_CONSDEV, sizeof(bi_cons));
}

void
putchar(int c)
{

	common_putc(outfd, c);
}

int
getchar(void)
{

	return common_getc(infd, 1);
}

static void
common_putc(int fd, int c)
{

	(void)uwrite(fd, &c, 1);
}

static int
common_getc(int fd, int timo)
{
	struct linux_timeval tv;
	fd_set fdset;
	int nfds, n;
	char c;

	for (; timo < 0 || timo > 0; --timo) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&fdset);

		nfds = 1;
		FD_SET(fd, &fdset);

		n = uselect(nfds, &fdset, NULL, NULL, &tv);
		if (n > 0)
			break;
	}

	if (timo > 0) {
		for (fd = 0; fd < nfds; fd++) {
			if (FD_ISSET(fd, &fdset)) {
				return (uread(fd, &c, 1) < 1 ? -1 : c);
			}
		}
	}
	return -1;
}

int
awaitkey(int timeout, int tell)
{
	struct linux_termios orig_termios, raw_termios;
	int c = 0;
	int i;

	/* set raw mode */
	linux_tcgetattr(infd, &orig_termios);
	raw_termios = orig_termios;
	linux_cfmakeraw(&raw_termios);
	linux_tcsetattr(infd, LINUX_TCSETS, &raw_termios);

	for (i = timeout; i > 0; i--) {
		if (tell) {
			char numbuf[20];
			int len, j;

			len = snprintf(numbuf, sizeof(numbuf), "%d ", i);
			for (j = 0; j < len; j++)
				numbuf[len + j] = '\b';
			numbuf[len + j] = '\0';
			printf("%s", numbuf);
		}
		c = common_getc(infd, 1);
		if (c == 0)
			c = -1;
		if (c >= 0)
			break;
	}
	if (i == 0)
		c = '\0';

	/* set original mode */
	linux_tcsetattr(infd, LINUX_TCSETS, &orig_termios);

	if (tell)
		printf("0 \n");

	return c;
}

void dummycall2(void);
void
dummycall2(void)
{

	(void)linux_termio_to_bsd_termios;
	(void)bsd_termios_to_linux_termio;
	(void)linux_termios_to_bsd_termios;
	(void)bsd_termios_to_linux_termios;
}
