/*	$NetBSD: rumpcomp_user.c,v 1.6.4.1 2013/08/28 23:59:37 rmind Exp $	*/

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

#ifndef _KERNEL
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/uio.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __linux__
#include <net/if.h>
#include <linux/if_tun.h>
#endif

#include <rump/rumpuser_component.h>

#include "if_virt.h"
#include "rumpcomp_user.h"

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

int
VIFHYPER_CREATE(int devnum, struct virtif_user **viup)
{
	struct virtif_user *viu = NULL;
	void *cookie;
	int rv;

	cookie = rumpuser_component_unschedule();

	viu = malloc(sizeof(*viu));
	if (viu == NULL) {
		rv = errno;
		goto out;
	}

	viu->viu_fd = opentapdev(devnum);
	if (viu->viu_fd == -1) {
		rv = errno;
		free(viu);
		goto out;
	}
	viu->viu_dying = 0;
	rv = 0;

 out:
	rumpuser_component_schedule(cookie);

	*viup = viu;
	return rumpuser_component_errtrans(rv);
}

void
VIFHYPER_SEND(struct virtif_user *viu,
	struct iovec *iov, size_t iovlen)
{
	void *cookie = rumpuser_component_unschedule();
	ssize_t idontcare __attribute__((__unused__));

	/*
	 * no need to check for return value; packets may be dropped
	 *
	 * ... sorry, I spoke too soon.  We need to check it because
	 * apparently gcc reinvented const poisoning and it's very
	 * hard to say "thanks, I know I'm not using the result,
	 * but please STFU and let's get on with something useful".
	 * So let's trick gcc into letting us share the compiler
	 * experience.
	 */
	idontcare = writev(viu->viu_fd, iov, iovlen);

	rumpuser_component_schedule(cookie);
}

/* how often to check for interface going south */
#define POLLTIMO_MS 10
int
VIFHYPER_RECV(struct virtif_user *viu,
	void *data, size_t dlen, size_t *rcv)
{
	void *cookie = rumpuser_component_unschedule();
	struct pollfd pfd;
	ssize_t nn = 0;
	int rv, prv;

	pfd.fd = viu->viu_fd;
	pfd.events = POLLIN;

	for (;;) {
		if (viu->viu_dying) {
			rv = 0;
			*rcv = 0;
			break;
		}

		prv = poll(&pfd, 1, POLLTIMO_MS);
		if (prv == 0)
			continue;
		if (prv == -1) {
			rv = errno;
			break;
		}

		nn = read(viu->viu_fd, data, dlen);
		if (nn == -1) {
			if (errno == EAGAIN)
				continue;
			rv = errno;
		} else {
			*rcv = (size_t)nn;
			rv = 0;
		}

		break;
	}

	rumpuser_component_schedule(cookie);
	return rumpuser_component_errtrans(rv);
}
#undef POLLTIMO_MS

void
VIFHYPER_DYING(struct virtif_user *viu)
{

	/* no locking necessary.  it'll be seen eventually */
	viu->viu_dying = 1;
}

void
VIFHYPER_DESTROY(struct virtif_user *viu)
{
	void *cookie = rumpuser_component_unschedule();

	close(viu->viu_fd);
	free(viu);

	rumpuser_component_schedule(cookie);
}
#endif
