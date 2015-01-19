/*	$NetBSD: bl.c,v 1.4 2015/01/19 18:52:55 christos Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
#include <sys/cdefs.h>
__RCSID("$NetBSD: bl.c,v 1.4 2015/01/19 18:52:55 christos Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "bl.h"

static void
bl_reset(bl_t b)
{
	close(b->b_fd);
	b->b_fd = -1;
	b->b_connected = false;
}

static int
bl_init(bl_t b, bool srv)
{
	static int one = 1;
	/* AF_UNIX address of local logger */
	struct sockaddr_un sun = {
		.sun_family = AF_LOCAL,
		.sun_len = sizeof(sun),
	};
	strlcpy(sun.sun_path, b->b_path, sizeof(sun.sun_path));
	if (srv)
		(void)unlink(b->b_path);

	if (b->b_fd == -1) {
		b->b_fd = socket(PF_LOCAL,
		    SOCK_DGRAM|SOCK_CLOEXEC|SOCK_NONBLOCK|SOCK_NOSIGPIPE, 0);
		if (b->b_fd == -1) {
			(*b->b_fun)(LOG_ERR, "%s: socket failed (%m)",
			    __func__);
			return 0;
		}
	}

	if (b->b_connected)
		return 0;

	if ((srv ? bind : connect)(b->b_fd, (const void *)&sun,
	    (socklen_t)sizeof(sun)) == -1) {
		(*b->b_fun)(LOG_ERR, "%s: %s failed (%m)", __func__,
		    srv ? "bind" : "connect");
		goto out;
	}

	b->b_connected = true;
	if (setsockopt(b->b_fd, 0, LOCAL_CREDS,
	    &one, sizeof(one)) == -1) {
		(*b->b_fun)(LOG_ERR, "%s: setsockopt LOCAL_CREDS "
		    "failed (%m)", __func__);
		goto out;
	}

	if (srv)
		if (listen(b->b_fd, 5) == -1) {
			(*b->b_fun)(LOG_ERR, "%s: listen failed (%m)",
			    __func__);
			goto out;
		}
	return 0;
out:
	bl_reset(b);
	return -1;
}

bl_t
bl_create2(bool srv, const char *path, void (*fun)(int, const char *, ...))
{
	bl_t b = malloc(sizeof(*b));
	bl_info_t *bi;
	if (b == NULL)
		goto out;
	bi = &b->b_info;
	bi->bi_fd = malloc(2 * sizeof(int));
	if (bi->bi_fd == NULL)
		goto out1;
	bi->bi_cred = malloc(SOCKCREDSIZE(NGROUPS_MAX));
	if (bi->bi_cred == NULL)
		goto out2;

	b->b_fun = fun == NULL ? syslog : fun;
	b->b_fd = -1;
	b->b_path = strdup(path ? path : _PATH_BLSOCK);
	if (b->b_path == NULL)
		goto out3;
	b->b_connected = false;
	bl_init(b, srv);
	return b;
out3:
	free(bi->bi_cred);
out2:
	free(bi->bi_fd);
out1:
	free(b);
out:
	(*fun)(LOG_ERR, "%s: malloc failed (%m)", __func__);
	return NULL;
}

bl_t
bl_create(void)
{
	return bl_create2(false, NULL, NULL);
}

void
bl_destroy(bl_t b)
{
	bl_reset(b);
	free(__UNCONST(b->b_path));
	free(b->b_info.bi_cred);
	free(b->b_info.bi_fd);
	free(b);
}

int
bl_send(bl_t b, bl_type_t e, int lfd, int pfd, const char *ctx)
{
	struct msghdr   msg;
	struct iovec    iov;
	union {
		char ctrl[CMSG_SPACE(2 * sizeof(int))];
		uint32_t fd[2];
	} ua;
	struct cmsghdr *cmsg;
	union {
		bl_message_t bl;
		char buf[512];
	} ub;
	int *fd;
	size_t ctxlen, tried;
#define NTRIES	5

	ctxlen = strlen(ctx);
	if (ctxlen > 256)
		ctxlen = 256;

	iov.iov_base = ub.buf;
	iov.iov_len = sizeof(bl_message_t) + ctxlen;
	ub.bl.bl_len = iov.iov_len;
	ub.bl.bl_version = BL_VERSION;
	ub.bl.bl_type = (uint32_t)e;
	memcpy(ub.bl.bl_data, ctx, ctxlen);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	msg.msg_control = ua.ctrl;
	msg.msg_controllen = sizeof(ua.ctrl);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(2 * sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;

	fd = (void *)CMSG_DATA(cmsg);
	fd[0] = lfd;
	fd[1] = pfd;

	tried = 0;
again:
	if (bl_init(b, false) == -1)
		return -1;

	if ((sendmsg(b->b_fd, &msg, 0) == -1) && tried++ < NTRIES) {
		bl_reset(b);
		goto again;
	}
	return tried >= NTRIES ? -1 : 0;
}

bl_info_t *
bl_recv(bl_t b)
{
        struct msghdr   msg;
        struct iovec    iov;
	union {
		char ctrl[CMSG_SPACE(2 * sizeof(int)) +
			CMSG_SPACE(SOCKCREDSIZE(NGROUPS_MAX))];
		uint32_t fd[2];
		struct sockcred sc;
	} ua;
	struct cmsghdr *cmsg;
	struct sockcred *sc;
	union {
		bl_message_t bl;
		char buf[512];
	} ub;
	int *fd;
	ssize_t rlen;
	bl_info_t *bi = &b->b_info;

	iov.iov_base = ub.buf;
	iov.iov_len = sizeof(ub);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	msg.msg_control = ua.ctrl;
	msg.msg_controllen = sizeof(ua.ctrl) + 100;

        rlen = recvmsg(b->b_fd, &msg, 0);
        if (rlen == -1) {
		(*b->b_fun)(LOG_ERR, "%s: recvmsg failed (%m)", __func__);
		return NULL;
        }

	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level != SOL_SOCKET) {
			(*b->b_fun)(LOG_ERR, "%s: unexpected cmsg_level %d",
			    __func__, cmsg->cmsg_level);
			continue;
		}
		switch (cmsg->cmsg_type) {
		case SCM_RIGHTS:
			if (cmsg->cmsg_len != CMSG_LEN(2 * sizeof(int))) {
				(*b->b_fun)(LOG_ERR,
				    "%s: unexpected cmsg_len %d != %zu",
				    __func__, cmsg->cmsg_len,
				    CMSG_LEN(2 * sizeof(int)));
				continue;
			}
			fd = (void *)CMSG_DATA(cmsg);
			memcpy(bi->bi_fd, fd, sizeof(bi->bi_fd));
			break;
		case SCM_CREDS:
			sc = (void *)CMSG_DATA(cmsg);
			if (sc->sc_ngroups > NGROUPS_MAX)
				sc->sc_ngroups = NGROUPS_MAX;
			memcpy(bi->bi_cred, sc, SOCKCREDSIZE(sc->sc_ngroups));
			break;
		default:
			(*b->b_fun)(LOG_ERR, "%s: unexpected cmsg_type %d",
			    __func__, cmsg->cmsg_type);
			continue;
		}

	}

	if (rlen <= sizeof(ub.bl)) {
		(*b->b_fun)(LOG_ERR, "message too short %zd", rlen);
		return NULL;
	}

	if (ub.bl.bl_version != BL_VERSION) {
		(*b->b_fun)(LOG_ERR, "bad version %d", ub.bl.bl_version);
		return NULL;
	}

	bi->bi_type = ub.bl.bl_type;
	strlcpy(bi->bi_msg, ub.bl.bl_data, MIN(sizeof(bi->bi_msg),
	    rlen - sizeof(ub.bl) + 1));
	return bi;
}
