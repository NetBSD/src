/* $NetBSD: wsmuxctl.c,v 1.1 2001/10/13 20:05:43 augustss Exp $ */

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author: Lennart Augustsson <augustss@carlstedt.se>
 *         Carlstedt Research & Technology
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <err.h>
#include <errno.h>

#include <dev/wscons/wsconsio.h>

static void usage(void);
int main(int, char**);

const char *devnames[] = { "?", "wsmouse", "wskbd", "wsmux" };

static void
usage(void)
{

	fprintf(stderr, "Usage: %s [-a dev] -f wsmux [-l] [-r dev]\n",
		    getprogname());
	exit(1);
}

static void
parsedev(const char *dev, struct wsmux_device *mdev)
{
	if (sscanf(dev, "wsmouse%d", &mdev->idx) == 1) {
		mdev->type = WSMUX_MOUSE;
		return;
	}
	if (sscanf(dev, "wskbd%d", &mdev->idx) == 1) {
		mdev->type = WSMUX_KBD;
		return;
	}
	if (sscanf(dev, "wsmux%d", &mdev->idx) == 1) {
		mdev->type = WSMUX_MUX;
		return;
	}
	errx(1, "bad device: `%s', use wsmouse, wskdb, or wsmux\n", dev);
}

int
main(int argc, char **argv)
{
	char *wsdev, *dev;
	int wsfd, list, c, i, add, rem;
	struct wsmux_device_list devs;
	struct wsmux_device mdev;

	wsdev = NULL;
	dev = NULL;
	add = 0;
	rem = 0;
	list = 0;

	while ((c = getopt(argc, argv, "a:f:lr:")) != -1) {
		switch (c) {
		case 'a':
			if (dev)
				usage();
			dev = optarg;
			add++;
			break;
		case 'r':
			if (dev)
				usage();
			dev = optarg;
			rem++;
			break;
		case 'f':
			wsdev = optarg;
			break;
		case 'l':
			list++;
			break;
		case '?':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (wsdev == NULL)
		usage();

	wsfd = open(wsdev, O_WRONLY, 0);
	if (wsfd < 0)
		err(2, "%s", wsdev);

	if (list) {
		if (add || rem)
			usage();
		if (ioctl(wsfd, WSMUX_LIST_DEVICES, &devs) < 0)
			err(1, "WSMUX_LIST_DEVICES");
		for (i = 0; i < devs.ndevices; i++)
			printf("%s%d\n", devnames[devs.devices[i].type],
			       devs.devices[i].idx);
		exit(0);
	}

	if (add) {
		parsedev(dev, &mdev);
		if (ioctl(wsfd, WSMUX_ADD_DEVICE, &mdev) < 0)
			err(1, "WSMUX_ADD_DEVICE");
	}

	if (rem) {
		parsedev(dev, &mdev);
		if (ioctl(wsfd, WSMUX_REMOVE_DEVICE, &mdev) < 0)
			err(1, "WSMUX_REMOVE_DEVICE");
	}

	close(wsfd);
	return (0);
}
