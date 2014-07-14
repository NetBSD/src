#include <sys/cdefs.h>
 __RCSID("$NetBSD: control.c,v 1.1.1.7 2014/07/14 11:45:02 roy Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dhcpcd.h"
#include "control.h"
#include "eloop.h"

#ifndef SUN_LEN
#define SUN_LEN(su) \
            (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

static void
control_handle_data(void *arg)
{
	struct fd_list *l = arg, *lp, *last;
	char buffer[1024], *e, *p, *argvp[255], **ap, *a;
	ssize_t bytes;
	size_t len;
	int argc;

	bytes = read(l->fd, buffer, sizeof(buffer) - 1);
	if (bytes == -1 || bytes == 0) {
		/* Control was closed or there was an error.
		 * Remove it from our list. */
		last = NULL;
		for (lp = l->ctx->control_fds; lp; lp = lp->next) {
			if (lp == l) {
				eloop_event_delete(lp->ctx->eloop, lp->fd);
				close(lp->fd);
				if (last == NULL)
					lp->ctx->control_fds = lp->next;
				else
					last->next = lp->next;
				free(lp);
				break;
			}
			last = lp;
		}
		return;
	}
	buffer[bytes] = '\0';
	p = buffer;
	e = buffer + bytes;

	/* Each command is \n terminated
	 * Each argument is NULL separated */
	while (p < e) {
		argc = 0;
		ap = argvp;
		while (p < e) {
			argc++;
			if ((size_t)argc > sizeof(argvp)) {
				errno = ENOBUFS;
				return;
			}
			a = *ap++ = p;
			len = strlen(p);
			p += len + 1;
			if (a[len - 1] == '\n') {
				a[len - 1] = '\0';
				break;
			}
		}
		dhcpcd_handleargs(l->ctx, l, argc, argvp);
	}
}

static void
control_handle(void *arg)
{
	struct dhcpcd_ctx *ctx;
	struct sockaddr_un run;
	socklen_t len;
	struct fd_list *l;
	int fd, flags;

	ctx = arg;
	len = sizeof(run);
	if ((fd = accept(ctx->control_fd, (struct sockaddr *)&run, &len)) == -1)
		return;
	if ((flags = fcntl(fd, F_GETFD, 0)) == -1 ||
	    fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
	{
		close(fd);
	        return;
	}
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1 ||
	    fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		close(fd);
	        return;
	}
	l = malloc(sizeof(*l));
	if (l) {
		l->ctx = ctx;
		l->fd = fd;
		l->listener = 0;
		l->next = ctx->control_fds;
		ctx->control_fds = l;
		eloop_event_add(ctx->eloop, l->fd, control_handle_data, l);
	} else
		close(fd);
}

static socklen_t
make_sock(struct dhcpcd_ctx *ctx, struct sockaddr_un *sa, const char *ifname)
{

#ifdef SOCK_CLOEXEC
	if ((ctx->control_fd = socket(AF_UNIX,
	    SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0)) == -1)
		return 0;
#else
	int flags;

	if ((ctx->control_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return 0;
	if ((flags = fcntl(ctx->control_fd, F_GETFD, 0)) == -1 ||
	    fcntl(ctx->control_fd, F_SETFD, flags | FD_CLOEXEC) == -1)
	{
		close(ctx->control_fd);
		ctx->control_fd = -1;
	        return 0;
	}
	if ((flags = fcntl(ctx->control_fd, F_GETFL, 0)) == -1 ||
	    fcntl(ctx->control_fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		close(ctx->control_fd);
		ctx->control_fd = -1;
	        return 0;
	}
#endif
	memset(sa, 0, sizeof(*sa));
	sa->sun_family = AF_UNIX;
	snprintf(sa->sun_path, sizeof(sa->sun_path), CONTROLSOCKET,
	    ifname ? "-" : "", ifname ? ifname : "");
	strlcpy(ctx->control_sock, sa->sun_path, sizeof(ctx->control_sock));
	return (socklen_t)SUN_LEN(sa);
}

int
control_start(struct dhcpcd_ctx *ctx, const char *ifname)
{
	struct sockaddr_un sa;
	socklen_t len;

	if ((len = make_sock(ctx, &sa, ifname)) == 0)
		return -1;
	unlink(ctx->control_sock);
	if (bind(ctx->control_fd, (struct sockaddr *)&sa, len) == -1 ||
	    chmod(ctx->control_sock,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) == -1 ||
	    (ctx->control_group &&
	        chown(ctx->control_sock,
		geteuid(), ctx->control_group) == -1) ||
	    listen(ctx->control_fd, sizeof(ctx->control_fds)) == -1)
	{
		close(ctx->control_fd);
		ctx->control_fd = -1;
		unlink(ctx->control_sock);
		return -1;
	}
	eloop_event_add(ctx->eloop, ctx->control_fd, control_handle, ctx);
	return ctx->control_fd;
}

int
control_stop(struct dhcpcd_ctx *ctx)
{
	int retval = 0;
	struct fd_list *l;

	if (ctx->control_fd == -1)
		return 0;
	eloop_event_delete(ctx->eloop, ctx->control_fd);
	if (shutdown(ctx->control_fd, SHUT_RDWR) == -1)
		retval = 1;
	ctx->control_fd = -1;
	if (unlink(ctx->control_sock) == -1)
		retval = -1;

	l = ctx->control_fds;
	while (l != NULL) {
		ctx->control_fds = l->next;
		eloop_event_delete(ctx->eloop, l->fd);
		shutdown(l->fd, SHUT_RDWR);
		free(l);
		l = ctx->control_fds;
	}

	return retval;
}

int
control_open(struct dhcpcd_ctx *ctx, const char *ifname)
{
	struct sockaddr_un sa;
	socklen_t len;

	if ((len = make_sock(ctx, &sa, ifname)) == 0)
		return -1;
	if (connect(ctx->control_fd, (struct sockaddr *)&sa, len) == -1) {
		close(ctx->control_fd);
		ctx->control_fd = -1;
		return -1;
	}
	return 0;
}

ssize_t
control_send(struct dhcpcd_ctx *ctx, int argc, char * const *argv)
{
	char buffer[1024];
	int i;
	size_t len, l;

	if (argc > 255) {
		errno = ENOBUFS;
		return -1;
	}
	len = 0;
	for (i = 0; i < argc; i++) {
		l = strlen(argv[i]) + 1;
		if (len + l > sizeof(buffer)) {
			errno = ENOBUFS;
			return -1;
		}
		memcpy(buffer + len, argv[i], l);
		len += l;
	}
	return write(ctx->control_fd, buffer, len);
}

void
control_close(struct dhcpcd_ctx *ctx)
{

	if (ctx->control_fd != -1) {
		close(ctx->control_fd);
		ctx->control_fd = -1;
	}
}
