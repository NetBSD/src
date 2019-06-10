/* $NetBSD: wsmuxctl.c,v 1.11.42.1 2019/06/10 22:10:44 christos Exp $ */

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
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <err.h>
#include <errno.h>

#include <dev/wscons/wsconsio.h>

__dead static void usage(void);

static const char *ctlpath = "/dev/wsmuxctl";

static void
usage(void)
{

	fprintf(stderr, "usage: %s [-a dev] -f wsmux [-l] [-L] [-r dev]\n",
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
	if (sscanf(dev, "wsbell%d", &mdev->idx) == 1) {
		mdev->type = WSMUX_BELL;
		return;
	}
	errx(1, "bad device: `%s', use wsmouse, wskdb, wsmux or wsbell", dev);
}

static void
listdevs(int fd, int rec, int ind)
{
	int i, rfd;
	char buf[100];
	struct wsmux_device_list devs;
	const char *name;

	if (ioctl(fd, WSMUXIO_LIST_DEVICES, &devs) < 0)
		err(1, "WSMUXIO_LIST_DEVICES");
	for (i = 0; i < devs.ndevices; i++) {
		switch (devs.devices[i].type) {
		case WSMUX_MOUSE:   name = "wsmouse"; break;
		case WSMUX_KBD:     name = "wskbd"; break;
		case WSMUX_MUX:     name = "wsmux"; break;
		case WSMUX_BELL:    name = "wsbell"; break;
		default:            name = "?"; break;
		}
		printf("%*s%s%d\n", ind, "", name, devs.devices[i].idx);
		if (rec && devs.devices[i].type == WSMUX_MUX) {
			snprintf(buf, sizeof(buf), "%s%d", ctlpath,
			    devs.devices[i].idx);
			rfd = open(buf, O_WRONLY, 0);
			if (rfd < 0)
				warn("%s", buf);
			listdevs(rfd, rec, ind+2);
			close(rfd);
		}
	}
}

int
main(int argc, char **argv)
{
	char *wsdev, *dev;
	int wsfd, list, c, add, rem, recursive;
	struct wsmux_device mdev;
	char buf[100];

	wsdev = NULL;
	dev = NULL;
	add = 0;
	rem = 0;
	list = 0;
	recursive = 0;

	while ((c = getopt(argc, argv, "a:f:lLr:")) != -1) {
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
		case 'L':
			recursive++;
			/* FALLTHROUGH */
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
	if (wsfd < 0) {
		if (isdigit((unsigned char)wsdev[0])) {
			snprintf(buf, sizeof(buf), "%s%s", ctlpath, wsdev);
			wsdev = buf;
			wsfd = open(wsdev, O_WRONLY, 0);
		}
		if (wsfd < 0)
			err(2, "%s", wsdev);
	}

	if (list) {
		if (add || rem)
			usage();
		listdevs(wsfd, recursive, 0);
		exit(0);
	}

	if (add) {
		parsedev(dev, &mdev);
		if (ioctl(wsfd, WSMUXIO_ADD_DEVICE, &mdev) < 0)
			err(1, "WSMUXIO_ADD_DEVICE");
	}

	if (rem) {
		parsedev(dev, &mdev);
		if (ioctl(wsfd, WSMUXIO_REMOVE_DEVICE, &mdev) < 0)
			err(1, "WSMUXIO_REMOVE_DEVICE");
	}

	close(wsfd);
	return (0);
}
