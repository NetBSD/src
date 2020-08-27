/*	$NetBSD: wg_user.c,v 1.3 2020/08/27 02:51:15 riastradh Exp $	*/

/*
 * Copyright (C) Ryota Ozaki <ozaki.ryota@gmail.com>
 * All rights reserved.
 *
 * Based on wg_user.c by Antti Kantee.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wg_user.c,v 1.3 2020/08/27 02:51:15 riastradh Exp $");

#ifndef _KERNEL
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/param.h>

#include <net/if.h>
#include <net/if_tun.h>

#include <netinet/in.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rumpuser_component.h>

#include "wg_user.h"

struct wg_user {
	struct wg_softc *wgu_sc;
	int wgu_devnum;
	char wgu_tun_name[IFNAMSIZ];

	int wgu_fd;
	int wgu_sock4;
	int wgu_sock6;
	int wgu_pipe[2];
	pthread_t wgu_rcvthr;

	int wgu_dying;

	char wgu_rcvbuf[9018]; /* jumbo frame max len */
};

static int
open_tun(const char *tun_name)
{
	char tun_path[MAXPATHLEN];
	int n, fd, error;

	n = snprintf(tun_path, sizeof(tun_path), "/dev/%s", tun_name);
	if (n == MAXPATHLEN)
		return E2BIG;

	fd = open(tun_path, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "%s: can't open %s: %s\n",
		    __func__, tun_name, strerror(errno));
	}

	int i = 1;
	error = ioctl(fd, TUNSLMODE, &i);
	if (error == -1) {
		close(fd);
		fd = -1;
	}

	return fd;
}

static void
close_tun(struct wg_user *wgu)
{
	int s;
	struct ifreq ifr = {};

	close(wgu->wgu_fd);

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		return; /* XXX */
	strcpy(ifr.ifr_name, wgu->wgu_tun_name);
	(void)ioctl(s, SIOCIFDESTROY, &ifr);
	close(s);
}

static void *
wg_user_rcvthread(void *aaargh)
{
	struct wg_user *wgu = aaargh;
	struct pollfd pfd[4];
	ssize_t nn = 0;
	int prv;

	rumpuser_component_kthread();

	pfd[0].fd = wgu->wgu_fd;
	pfd[0].events = POLLIN;
	pfd[1].fd = wgu->wgu_pipe[0];
	pfd[1].events = POLLIN;
	pfd[2].fd = wgu->wgu_sock4;
	pfd[2].events = POLLIN;
	pfd[3].fd = wgu->wgu_sock6;
	pfd[3].events = POLLIN;

	while (!wgu->wgu_dying) {
		struct iovec iov[2];

		prv = poll(pfd, 4, -1);
		if (prv == 0)
			continue;
		if (prv == -1) {
			/* XXX */
			fprintf(stderr, "%s: poll error: %d\n",
			    wgu->wgu_tun_name, errno);
			sleep(1);
			continue;
		}
		if (pfd[1].revents & POLLIN)
			continue;

		/* Receive user packets from tun */
		if (pfd[0].revents & POLLIN) {
			nn = read(wgu->wgu_fd, wgu->wgu_rcvbuf, sizeof(wgu->wgu_rcvbuf));
			if (nn == -1 && errno == EAGAIN)
				continue;

			if (nn < 1) {
				/* XXX */
				fprintf(stderr, "%s: receive failed\n",
				    wgu->wgu_tun_name);
				sleep(1);
				continue;
			}

			iov[0].iov_base = wgu->wgu_rcvbuf;
			iov[0].iov_len = ((struct sockaddr *)wgu->wgu_rcvbuf)->sa_len;

			iov[1].iov_base = (char *)wgu->wgu_rcvbuf + iov[0].iov_len;
			iov[1].iov_len = nn - iov[0].iov_len;

			rumpuser_component_schedule(NULL);
			rumpkern_wg_recv_user(wgu->wgu_sc, iov, 2);
			rumpuser_component_unschedule();
		}

		/* Receive wg UDP/IPv4 packets from a peer */
		if (pfd[2].revents & POLLIN) {
			struct sockaddr_in sin;
			socklen_t len = sizeof(sin);
			nn = recvfrom(wgu->wgu_sock4, wgu->wgu_rcvbuf,
			    sizeof(wgu->wgu_rcvbuf), 0, (struct sockaddr *)&sin,
			    &len);
			if (nn == -1 && errno == EAGAIN)
				continue;
			if (len != sizeof(sin))
				continue;
			iov[0].iov_base = &sin;
			iov[0].iov_len = sin.sin_len;

			iov[1].iov_base = wgu->wgu_rcvbuf;
			iov[1].iov_len = nn;

			rumpuser_component_schedule(NULL);
			rumpkern_wg_recv_peer(wgu->wgu_sc, iov, 2);
			rumpuser_component_unschedule();
		}

		/* Receive wg UDP/IPv6 packets from a peer */
		if (pfd[3].revents & POLLIN) {
			struct sockaddr_in6 sin6;
			socklen_t len = sizeof(sin6);
			nn = recvfrom(wgu->wgu_sock6, wgu->wgu_rcvbuf,
			    sizeof(wgu->wgu_rcvbuf), 0, (struct sockaddr *)&sin6,
			    &len);
			if (nn == -1 && errno == EAGAIN)
				continue;
			if (len != sizeof(sin6))
				continue;
			iov[0].iov_base = &sin6;
			iov[0].iov_len = sin6.sin6_len;

			iov[1].iov_base = wgu->wgu_rcvbuf;
			iov[1].iov_len = nn;

			rumpuser_component_schedule(NULL);
			rumpkern_wg_recv_peer(wgu->wgu_sc, iov, 2);
			rumpuser_component_unschedule();
		}
	}

	assert(wgu->wgu_dying);

	rumpuser_component_kthread_release();
	return NULL;
}

int
rumpuser_wg_create(const char *tun_name, struct wg_softc *wg,
    struct wg_user **wgup)
{
	struct wg_user *wgu = NULL;
	void *cookie;
	int rv;

	cookie = rumpuser_component_unschedule();

	wgu = malloc(sizeof(*wgu));
	if (wgu == NULL) {
		rv = errno;
		goto oerr1;
	}

	if (strlcpy(wgu->wgu_tun_name, tun_name, sizeof(wgu->wgu_tun_name))
	    >= sizeof(wgu->wgu_tun_name)) {
		rv = EINVAL;
		goto oerr2;
	}
	wgu->wgu_sc = wg;

	wgu->wgu_fd = open_tun(tun_name);
	if (wgu->wgu_fd == -1) {
		rv = errno;
		goto oerr2;
	}

	if (pipe(wgu->wgu_pipe) == -1) {
		rv = errno;
		goto oerr3;
	}

	wgu->wgu_sock4 = socket(AF_INET, SOCK_DGRAM, 0);
	wgu->wgu_sock6 = socket(AF_INET6, SOCK_DGRAM, 0);
	if (wgu->wgu_sock4 == -1 || wgu->wgu_sock6 == -1) {
		rv = errno;
		goto oerr4;
	}

	rv = pthread_create(&wgu->wgu_rcvthr, NULL, wg_user_rcvthread, wgu);
	if (rv != 0)
		goto oerr5;

	rumpuser_component_schedule(cookie);
	*wgup = wgu;
	return 0;

 oerr5:
	if (wgu->wgu_sock4 != -1)
		close(wgu->wgu_sock4);
	if (wgu->wgu_sock6 != -1)
		close(wgu->wgu_sock6);
 oerr4:
	close(wgu->wgu_pipe[0]);
	close(wgu->wgu_pipe[1]);
 oerr3:
	close_tun(wgu);
 oerr2:
	free(wgu);
 oerr1:
	rumpuser_component_schedule(cookie);
	return rumpuser_component_errtrans(rv);
}

/*
 * Send decrypted packets to users via a tun.
 */
void
rumpuser_wg_send_user(struct wg_user *wgu, struct iovec *iov, size_t iovlen)
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
	idontcare = writev(wgu->wgu_fd, iov, iovlen);

	rumpuser_component_schedule(cookie);
}

/*
 * Send wg messages to a peer.
 */
int
rumpuser_wg_send_peer(struct wg_user *wgu, struct sockaddr *sa,
    struct iovec *iov, size_t iovlen)
{
	void *cookie = rumpuser_component_unschedule();
	int s, error = 0;
	size_t i;
	ssize_t sent;

	if (sa->sa_family == AF_INET)
		s = wgu->wgu_sock4;
	else
		s = wgu->wgu_sock6;

	for (i = 0; i < iovlen; i++) {
		sent = sendto(s, iov[i].iov_base, iov[i].iov_len, 0, sa,
		    sa->sa_len);
		if (sent == -1 || (size_t)sent != iov[i].iov_len) {
			error = errno;
			break;
		}
	}

	rumpuser_component_schedule(cookie);

	return error;
}

int
rumpuser_wg_ioctl(struct wg_user *wgu, u_long cmd, void *data, int af)
{
	void *cookie = rumpuser_component_unschedule();
	int s, error;

	s = socket(af, SOCK_DGRAM, 0);
	if (s == -1)
		return errno;
	error = ioctl(s, cmd, data);
	close(s);

	rumpuser_component_schedule(cookie);

	return error == -1 ? errno : 0;
}

int
rumpuser_wg_sock_bind(struct wg_user *wgu, const uint16_t port)
{
	int error;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(sin);
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	error = bind(wgu->wgu_sock4, (struct sockaddr *)&sin, sizeof(sin));
	if (error == -1)
		return errno;

	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_len = sizeof(sin6);
	sin6.sin6_addr = in6addr_any;
	sin6.sin6_port = htons(port);

	error = bind(wgu->wgu_sock6, (struct sockaddr *)&sin6, sizeof(sin6));
	if (error == -1)
		return errno;

	return 0;
}

void
rumpuser_wg_destroy(struct wg_user *wgu)
{
	void *cookie = rumpuser_component_unschedule();

	wgu->wgu_dying = 1;
	if (write(wgu->wgu_pipe[1],
	    &wgu->wgu_dying, sizeof(wgu->wgu_dying)) == -1) {
		/*
		 * this is here mostly to avoid a compiler warning
		 * about ignoring the return value of write()
		 */
		fprintf(stderr, "%s: failed to signal thread\n",
		    wgu->wgu_tun_name);
	}
	pthread_join(wgu->wgu_rcvthr, NULL);
	close_tun(wgu);
	close(wgu->wgu_pipe[0]);
	close(wgu->wgu_pipe[1]);
	free(wgu);

	rumpuser_component_schedule(cookie);
}

char *
rumpuser_wg_get_tunname(struct wg_user *wgu)
{

	return wgu->wgu_tun_name;
}
#endif
