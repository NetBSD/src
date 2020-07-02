/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Privilege Separation for dhcpcd, control proxy
 * Copyright (c) 2006-2020 Roy Marples <roy@marples.name>
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

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "dhcpcd.h"
#include "control.h"
#include "eloop.h"
#include "logerr.h"
#include "privsep.h"

#ifdef HAVE_CAPSICUM
#include <sys/capsicum.h>
#endif

static int
ps_ctl_startcb(void *arg)
{
	struct dhcpcd_ctx *ctx = arg;
	sa_family_t af;

	if (ctx->options & DHCPCD_MASTER) {
		setproctitle("[control proxy]");
		af = AF_UNSPEC;
	} else {
		setproctitle("[control proxy] %s%s%s",
		    ctx->ifv[0],
		    ctx->options & DHCPCD_IPV4 ? " [ip4]" : "",
		    ctx->options & DHCPCD_IPV6 ? " [ip6]" : "");
		if ((ctx->options &
		    (DHCPCD_IPV4 | DHCPCD_IPV6)) == DHCPCD_IPV4)
			af = AF_INET;
		else if ((ctx->options &
		    (DHCPCD_IPV4 | DHCPCD_IPV6)) == DHCPCD_IPV6)
			af = AF_INET6;
		else
			af = AF_UNSPEC;
	}

	ctx->ps_control_pid = getpid();

	return control_start(ctx,
	    ctx->options & DHCPCD_MASTER ? NULL : *ctx->ifv, af);
}

static ssize_t
ps_ctl_recvmsgcb(void *arg, struct ps_msghdr *psm, __unused struct msghdr *msg)
{
	struct dhcpcd_ctx *ctx = arg;

	if (psm->ps_cmd != PS_CTL_EOF) {
		errno = ENOTSUP;
		return -1;
	}

	if (ctx->ps_control_client != NULL)
		ctx->ps_control_client = NULL;
	return 0;
}

static void
ps_ctl_recvmsg(void *arg)
{
	struct dhcpcd_ctx *ctx = arg;

	if (ps_recvpsmsg(ctx, ctx->ps_control_fd, ps_ctl_recvmsgcb, ctx) == -1)
		logerr(__func__);
}

static void
ps_ctl_signalcb(int sig, void *arg)
{
	struct dhcpcd_ctx *ctx = arg;

	if (sig != SIGTERM)
		return;

	shutdown(ctx->ps_control_fd, SHUT_RDWR);
	eloop_exit(ctx->eloop, EXIT_SUCCESS);
}

ssize_t
ps_ctl_handleargs(struct fd_list *fd, char *data, size_t len)
{

	/* Make any change here in dhcpcd.c as well. */
	if (strncmp(data, "--version",
	    MIN(strlen("--version"), len)) == 0) {
		return control_queue(fd, UNCONST(VERSION),
		    strlen(VERSION) + 1);
	} else if (strncmp(data, "--getconfigfile",
	    MIN(strlen("--getconfigfile"), len)) == 0) {
		return control_queue(fd, UNCONST(fd->ctx->cffile),
		    strlen(fd->ctx->cffile) + 1);
	} else if (strncmp(data, "--listen",
	    MIN(strlen("--listen"), len)) == 0) {
		fd->flags |= FD_LISTEN;
		return 0;
	}

	if (fd->ctx->ps_control_client != NULL &&
	    fd->ctx->ps_control_client != fd)
	{
		logerrx("%s: cannot handle another client", __func__);
		return 0;
	}
	return 1;
}

static ssize_t
ps_ctl_dispatch(void *arg, struct ps_msghdr *psm, struct msghdr *msg)
{
	struct dhcpcd_ctx *ctx = arg;
	struct iovec *iov = msg->msg_iov;
	struct fd_list *fd;
	unsigned int fd_flags = FD_SENDLEN;

	switch (psm->ps_flags) {
	case PS_CTL_PRIV:
		break;
	case PS_CTL_UNPRIV:
		fd_flags |= FD_UNPRIV;
		break;
	}

	switch (psm->ps_cmd) {
	case PS_CTL:
		if (msg->msg_iovlen != 1) {
			errno = EINVAL;
			return -1;
		}
		if (ctx->ps_control_client != NULL) {
			logerrx("%s: cannot handle another client", __func__);
			return 0;
		}
		fd = control_new(ctx, ctx->ps_control_data_fd, fd_flags);
		if (fd == NULL)
			return -1;
		ctx->ps_control_client = fd;
		control_recvdata(fd, iov->iov_base, iov->iov_len);
		break;
	case PS_CTL_EOF:
		control_free(ctx->ps_control_client);
		break;
	default:
		errno = ENOTSUP;
		return -1;
	}
	return 0;
}

static void
ps_ctl_dodispatch(void *arg)
{
	struct dhcpcd_ctx *ctx = arg;

	if (ps_recvpsmsg(ctx, ctx->ps_control_fd, ps_ctl_dispatch, ctx) == -1)
		logerr(__func__);
}

static void
ps_ctl_recv(void *arg)
{
	struct dhcpcd_ctx *ctx = arg;
	char buf[BUFSIZ];
	ssize_t len;

	errno = 0;
	len = read(ctx->ps_control_data_fd, buf, sizeof(buf));
	if (len == -1 || len == 0) {
		logerr("%s: read", __func__);
		eloop_exit(ctx->eloop, EXIT_FAILURE);
		return;
	}
	if (ctx->ps_control_client == NULL) /* client disconnected */
		return;
	errno = 0;
	if (control_queue(ctx->ps_control_client, buf, (size_t)len) == -1)
		logerr("%s: control_queue", __func__);
}

static void
ps_ctl_listen(void *arg)
{
	struct dhcpcd_ctx *ctx = arg;
	char buf[BUFSIZ];
	ssize_t len;
	struct fd_list *fd;

	errno = 0;
	len = read(ctx->ps_control->fd, buf, sizeof(buf));
	if (len == -1 || len == 0) {
		logerr("%s: read", __func__);
		eloop_exit(ctx->eloop, EXIT_FAILURE);
		return;
	}

	/* Send to our listeners */
	TAILQ_FOREACH(fd, &ctx->control_fds, next) {
		if (!(fd->flags & FD_LISTEN))
			continue;
		if (control_queue(fd, buf, (size_t)len)== -1)
			logerr("%s: control_queue", __func__);
	}
}

pid_t
ps_ctl_start(struct dhcpcd_ctx *ctx)
{
	int data_fd[2], listen_fd[2];
	pid_t pid;

	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CXNB, 0, data_fd) == -1)
		return -1;
	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CXNB, 0, listen_fd) == -1)
		return -1;
#ifdef PRIVSEP_RIGHTS
	if (ps_rights_limit_fdpair(data_fd) == -1)
		return -1;
	if (ps_rights_limit_fdpair(listen_fd) == -1)
		return -1;
#endif

	pid = ps_dostart(ctx, &ctx->ps_control_pid, &ctx->ps_control_fd,
	    ps_ctl_recvmsg, ps_ctl_dodispatch, ctx,
	    ps_ctl_startcb, ps_ctl_signalcb,
	    PSF_DROPPRIVS);

	if (pid == -1)
		return -1;
	else if (pid != 0) {
		ctx->ps_control_data_fd = data_fd[1];
		close(data_fd[0]);
		ctx->ps_control = control_new(ctx,
		    listen_fd[1], FD_SENDLEN | FD_LISTEN);
		if (ctx->ps_control == NULL)
			return -1;
		close(listen_fd[0]);
		return pid;
	}

	ctx->ps_control_data_fd = data_fd[0];
	close(data_fd[1]);
	if (eloop_event_add(ctx->eloop, ctx->ps_control_data_fd,
	    ps_ctl_recv, ctx) == -1)
		return -1;

	ctx->ps_control = control_new(ctx,
	    listen_fd[0], 0);
	close(listen_fd[1]);
	if (ctx->ps_control == NULL)
		return -1;
	if (eloop_event_add(ctx->eloop, ctx->ps_control->fd,
	    ps_ctl_listen, ctx) == -1)
		return -1;

#ifdef HAVE_CAPSICUM
	if (cap_enter() == -1 && errno != ENOSYS)
		logerr("%s: cap_enter", __func__);
#endif
#ifdef HAVE_PLEDGE
	if (pledge("stdio inet", NULL) == -1)
		logerr("%s: pledge", __func__);
#endif
	return 0;
}

int
ps_ctl_stop(struct dhcpcd_ctx *ctx)
{

	return ps_dostop(ctx, &ctx->ps_control_pid, &ctx->ps_control_fd);
}

ssize_t
ps_ctl_sendargs(struct fd_list *fd, void *data, size_t len)
{
	struct dhcpcd_ctx *ctx = fd->ctx;

	if (ctx->ps_control_client != NULL && ctx->ps_control_client != fd)
		logerrx("%s: cannot deal with another client", __func__);
	ctx->ps_control_client = fd;
	return ps_sendcmd(ctx, ctx->ps_control_fd, PS_CTL,
	    fd->flags & FD_UNPRIV ? PS_CTL_UNPRIV : PS_CTL_PRIV,
	    data, len);
}

ssize_t
ps_ctl_sendeof(struct fd_list *fd)
{
	struct dhcpcd_ctx *ctx = fd->ctx;

	return ps_sendcmd(ctx, ctx->ps_control_fd, PS_CTL_EOF, 0, NULL, 0);
}
