/*	$NetBSD: rumpcomp_user.c,v 1.1 2013/03/13 21:13:45 pooka Exp $	*/

/*
 * Copyright (c) 2013 Antti Kantee.  All Rights Reserved.
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
#include <sys/ioctl.h>
#include <sys/uio.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <net/if.h>
#include <linux/if_tun.h>
#endif

#include <rump/rumpuser_component.h>

struct virtif_user {
	int viu_fd;
	int viu_dying;
};

static int
opentapdev(int devnum)
{
	int fd = -1;

#if defined(__NetBSD__) || defined(__DragonFly__)
	char tapdev[64];

	snprintf(tapdev, sizeof(tapdev), "/dev/tap%d", devnum);
	fd = open(tapdev, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "rumpcomp_virtif_create: can't open %s: "
		    "%s\n", tapdev, strerror(errno));
	}

#elif defined(__linux__)
	struct ifreq ifr;
	char devname[16];

	fd = open("/dev/net/tun", O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "rumpcomp_virtif_create: can't open %s: "
		    "%s\n", "/dev/net/tun", strerror(errno));
		return -1;
	}

	snprintf(devname, sizeof(devname), "tun%d", devnum);
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strncpy(ifr.ifr_name, devname, sizeof(ifr.ifr_name)-1);

	if (ioctl(fd, TUNSETIFF, &ifr) == -1) {
		close(fd);
		fd = -1;
	}

#else
	fprintf(stderr, "virtif not supported on this platform\n");
#endif

	return fd;
}

struct virtif_user *
rumpcomp_virtif_create(int devnum)
{
	struct virtif_user *viu;
	void *cookie;

	cookie = rumpuser_component_unschedule();

	viu = malloc(sizeof(*viu));
	viu->viu_fd = opentapdev(devnum);
	viu->viu_dying = 0;

 out:
	if (viu->viu_fd == -1) {
		free(viu);
		viu = NULL;
	}

	rumpuser_component_schedule(cookie);
	return viu;
}

void
rumpcomp_virtif_send(struct virtif_user *viu,
	struct iovec *iov, size_t iovlen)
{
	void *cookie = rumpuser_component_unschedule();

	/* no need to check for return value; packets may be dropped */
	writev(viu->viu_fd, iov, iovlen);

	rumpuser_component_schedule(cookie);
}

/* how often to check for interface going south */
#define POLLTIMO_MS 10
size_t
rumpcomp_virtif_recv(struct virtif_user *viu, void *data, size_t dlen)
{
	void *cookie = rumpuser_component_unschedule();
	struct pollfd pfd;
	ssize_t nn = 0;
	int rv;

	pfd.fd = viu->viu_fd;
	pfd.events = POLLIN;

	for (;;) {
		if (viu->viu_dying)
			break;

		rv = poll(&pfd, 1, POLLTIMO_MS);
		if (rv == 0)
			continue;
		if (rv == -1)
			break;

		nn = read(viu->viu_fd, data, dlen);
		if (nn == -1 && errno == EAGAIN)
			continue;

		break;
	}

	rumpuser_component_schedule(cookie);
	return nn;
}
#undef POLLTIMO_MS

void
rumpcomp_virtif_dying(struct virtif_user *viu)
{

	/* no locking necessary.  it'll be seen eventually */
	viu->viu_dying = 1;
}

void
rumpcomp_virtif_destroy(struct virtif_user *viu)
{
	void *cookie = rumpuser_component_unschedule();

	close(viu->viu_fd);
	free(viu);

	rumpuser_component_schedule(cookie);
}
