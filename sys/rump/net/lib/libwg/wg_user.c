/*	$NetBSD: wg_user.c,v 1.5 2026/07/04 22:22:33 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: wg_user.c,v 1.5 2026/07/04 22:22:33 riastradh Exp $");

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

	struct {
		union {
			struct sockaddr sa;
			struct sockaddr_in sin;
			struct sockaddr_in6 sin6;
		} addr;
		char payload[9018]; /* jumbo frame max len */
	} wgu_rcvbuf;
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

		/* rumpuser_wg_destroy notified us it's time */
		if (pfd[1].revents & POLLIN)
			continue;

		/* Receive user packets from tun */
		if (pfd[0].revents & POLLIN) {
			const struct sockaddr *dst;
			const void *pkt;
			size_t pktlen;

			nn = read(wgu->wgu_fd, &wgu->wgu_rcvbuf,
			    sizeof(wgu->wgu_rcvbuf));
			if (nn == -1 && errno == EAGAIN)
				continue;

			if (nn < 1) {
				/* XXX */
				fprintf(stderr, "%s: receive failed\n",
				    wgu->wgu_tun_name);
				sleep(1);
				continue;
			}

			dst = &wgu->wgu_rcvbuf.addr.sa;
			pkt = (const char *)dst + dst->sa_len;
			pktlen = (size_t)nn - dst->sa_len;

			rumpuser_component_schedule(NULL);
			rumpkern_wg_recv_user(wgu->wgu_sc, dst, pkt, pktlen);
			rumpuser_component_unschedule();
		}

		/* Receive wg UDP/IPv4 packets from a peer */
		if (pfd[2].revents & POLLIN) {
			struct sockaddr *src = &wgu->wgu_rcvbuf.addr.sa;
			socklen_t len = sizeof(wgu->wgu_rcvbuf.addr.sin);
			const void *pkt;
			size_t pktlen;

			nn = recvfrom(wgu->wgu_sock4, wgu->wgu_rcvbuf.payload,
			    sizeof(wgu->wgu_rcvbuf.payload), 0, src, &len);
			if (nn == -1)
				continue;
			if (len != sizeof(wgu->wgu_rcvbuf.addr.sin))
				continue;
			pkt = wgu->wgu_rcvbuf.payload;
			pktlen = (size_t)nn;

			rumpuser_component_schedule(NULL);
			rumpkern_wg_recv_peer(wgu->wgu_sc, src, pkt, pktlen);
			rumpuser_component_unschedule();
		}

		/* Receive wg UDP/IPv6 packets from a peer */
		if (pfd[3].revents & POLLIN) {
			struct sockaddr *src = &wgu->wgu_rcvbuf.addr.sa;
			socklen_t len = sizeof(wgu->wgu_rcvbuf.addr.sin6);
			const void *pkt;
			size_t pktlen;

			nn = recvfrom(wgu->wgu_sock6, wgu->wgu_rcvbuf.payload,
			    sizeof(wgu->wgu_rcvbuf.payload), 0, src, &len);
			if (nn == -1)
				continue;
			if (len != sizeof(wgu->wgu_rcvbuf.addr.sin6))
				continue;
			pkt = wgu->wgu_rcvbuf.payload;
			pktlen = (size_t)nn;

			rumpuser_component_schedule(NULL);
			rumpkern_wg_recv_peer(wgu->wgu_sc, src, pkt, pktlen);
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
rumpuser_wg_send_user(struct wg_user *wgu, const struct sockaddr *dst,
    const void *pkt, size_t pktlen)
{
	void *cookie = rumpuser_component_unschedule();
	struct iovec iov[2];
	int iovlen;
	ssize_t idontcare __attribute__((__unused__));

	memset(iov, 0, sizeof(iov));
	iov[0].iov_base = __UNCONST(dst);
	iov[0].iov_len = dst->sa_len;
	iov[1].iov_base = __UNCONST(pkt);
	iov[1].iov_len = pktlen;
	iovlen = 2;

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
rumpuser_wg_send_peer(struct wg_user *wgu, const struct sockaddr *dst,
    const void *pkt, size_t pktlen)
{
	void *cookie = rumpuser_component_unschedule();
	int s, error = 0;
	ssize_t sent;

	switch (dst->sa_family) {
	case AF_INET:
		s = wgu->wgu_sock4;
		break;
	case AF_INET6:
		s = wgu->wgu_sock6;
		break;
	default:
		error = EAFNOSUPPORT;
		goto out;
	}

	sent = sendto(s, pkt, pktlen, 0, dst, dst->sa_len);
	if (sent == -1)
		error = errno;
	else if ((size_t)sent != pktlen)
		error = EIO;

out:	rumpuser_component_schedule(cookie);

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
	union {
		struct sockaddr sa;
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
	} u;

	memset(&u.sin, 0, sizeof(u.sin));
	u.sin.sin_family = AF_INET;
	u.sin.sin_len = sizeof(u.sin);
	u.sin.sin_addr.s_addr = INADDR_ANY;
	u.sin.sin_port = htons(port);

	if (bind(wgu->wgu_sock4, &u.sa, sizeof(u.sin)) == -1)
		return errno;

	memset(&u.sin6, 0, sizeof(u.sin6));
	u.sin6.sin6_family = AF_INET6;
	u.sin6.sin6_len = sizeof(u.sin6);
	u.sin6.sin6_addr = in6addr_any;
	u.sin6.sin6_port = htons(port);

	if (bind(wgu->wgu_sock6, &u.sa, sizeof(u.sin6)) == -1)
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
