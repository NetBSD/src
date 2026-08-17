/*
 * Privilege Separation for dhcpcd, control proxy
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2006-2025 Roy Marples <roy@marples.name>
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "control.h"
#include "dhcpcd.h"
#include "eloop.h"
#include "logerr.h"
#include "privsep.h"

#define PS_CTL_FD(ctx) (ctx)->ps_ctl->psp_fd

/* We expect to have open 2 privsep STREAM, 2 STREAM and 2 file STREAM fds */

static int
ps_ctl_startcb(struct ps_process *psp)
{
	struct dhcpcd_ctx *ctx = psp->psp_ctx;
	sa_family_t af;

	if (ctx->options & DHCPCD_MANAGER) {
#ifdef HAVE_SETPROCTITLE
		setproctitle("[control proxy]");
#endif
		af = AF_UNSPEC;
	} else {
#ifdef HAVE_SETPROCTITLE
		setproctitle("[control proxy] %s%s%s", ctx->ifv[0],
		    ctx->options & DHCPCD_IPV4 ? " [ip4]" : "",
		    ctx->options & DHCPCD_IPV6 ? " [ip6]" : "");
#endif
		if ((ctx->options & (DHCPCD_IPV4 | DHCPCD_IPV6)) == DHCPCD_IPV4)
			af = AF_INET;
		else if ((ctx->options & (DHCPCD_IPV4 | DHCPCD_IPV6)) ==
		    DHCPCD_IPV6)
			af = AF_INET6;
		else
			af = AF_UNSPEC;
	}

	return control_start(ctx,
	    ctx->options & DHCPCD_MANAGER ? NULL : *ctx->ifv, af);
}

static void
ps_ctl_recvmsg(void *arg, unsigned short events)
{
	struct ps_process *psp = arg;

	if (ps_recvpsmsg(psp->psp_ctx, psp->psp_fd, events, NULL, NULL) == -1)
		logerr(__func__);
}

ssize_t
ps_ctl_handleargs(struct fd_list *fd, const char *data, size_t len)
{
#define strclcmp(d, l, c) \
	((l) == (__arraycount((c))) ? strncmp((d), (c), (l)) : -1)

	/* Make any change here in dhcpcd.c as well.
	 * --version is NOT terminated with \n. */
	if (strclcmp(data, len, "--version") == 0)
		return control_queue(fd, VERSION, strlen(VERSION) + 1);
	else if (strclcmp(data, len, "--getconfigfile\n") == 0)
		return control_queue(fd, fd->ctx->cffile,
		    strlen(fd->ctx->cffile) + 1);
	else if (strclcmp(data, len, "--isprivileged\n") == 0) {
		const char *ret = fd->flags & FD_CONTROL ? "true" : "false";
		return control_queue(fd, ret, strlen(ret) + 1);
	} else if (strclcmp(data, len, "--listen\n") == 0)
		return control_handle_listen(fd);

	fd->flags |= FD_COMMAND;
	return 0;
}

static ssize_t
ps_ctl_dispatch(void *arg, struct ps_msghdr *psm, struct msghdr *msg)
{
	struct ps_process *psp = arg;
	struct dhcpcd_ctx *ctx = psp->psp_ctx;
	struct fd_list *fd;
	unsigned int fd_flags = 0;
	int err;

	switch (psm->ps_cmd) {
	case PS_CTL_CONTROL:
		fd_flags |= FD_CONTROL; /* FALLTHROUGH */
	case PS_CTL_READ:
		fd_flags |= FD_READ; /* FALLTHROUGH */
	case PS_CTL:
		if (msg->msg_iovlen != 1) {
			errno = EINVAL;
			return -1;
		}
		fd = control_new(ctx, ctx->ps_ctl->psp_work_fd, fd_flags);
		if (fd == NULL)
			return -1;
		fd->peer_id = (unsigned int)psm->ps_flags;
		err = control_recvmsg(fd, msg, psm->ps_datalen);
		if (err == -1 || err == 0)
			control_free(fd);
		break;
	default:
		errno = ENOTSUP;
		return -1;
	}
	return 0;
}

static void
ps_ctl_dodispatch(void *arg, unsigned short events)
{
	struct ps_process *psp = arg;

	if (ps_recvpsmsg(psp->psp_ctx, psp->psp_fd, events, ps_ctl_dispatch,
		psp) == -1)
		logerr(__func__);
}

static void
ps_ctl_recv(void *arg, unsigned short events)
{
	struct dhcpcd_ctx *ctx = arg;
	int fd;
	unsigned int peer_id;
	size_t msglen;
	/* Control messages for a peer are prefixed with fd and message len */
	struct iovec iov[] = {
		{
		    .iov_base = &peer_id,
		    .iov_len = sizeof(peer_id),
		},
		{
		    .iov_base = &msglen,
		    .iov_len = sizeof(msglen),
		},
	};
	struct msghdr msg = {
		.msg_iov = iov,
		.msg_iovlen = __arraycount(iov),
	};
	ssize_t rlen;
	struct fd_list *fdl;

	if (events & ELE_HANGUP) {
	hangup:
		eloop_exit(ctx->eloop, EXIT_SUCCESS);
		return;
	}

	if (!(events & ELE_READ))
		logerrx("%s: unexpected event 0x%04x", __func__, events);

	fd = ctx->ps_ctl->psp_work_fd;
	rlen = recvmsg(fd, &msg, MSG_WAITALL);
	if (rlen == 0)
		goto hangup;
	if (rlen == -1) {
		logerr("%s: recvmsg hdr", __func__);
		eloop_exit(ctx->eloop, EXIT_FAILURE);
		return;
	}
	if (rlen != sizeof(peer_id) + sizeof(msglen)) {
		errno = EINVAL;
		logerr("%s: recvmsg hdr", __func__);
		eloop_exit(ctx->eloop, EXIT_FAILURE);
		return;
	}

	if (msglen == 0) /* ulikely */
		return;

	if (ctx->io_buflen < msglen) {
		void *n = realloc(ctx->io_buf, msglen);
		if (n == NULL) {
			logerr(__func__);
			eloop_exit(ctx->eloop, EXIT_FAILURE);
			return;
		}
		ctx->io_buf = n;
		ctx->io_buflen = msglen;
	}

	iov[0].iov_base = ctx->io_buf;
	iov[0].iov_len = msglen;
	msg.msg_iovlen = 1;
	rlen = recvmsg(fd, &msg, MSG_WAITALL);
	if (rlen == 0)
		goto hangup;
	if (rlen == -1) {
		logerr("%s: recvmsg msg", __func__);
		eloop_exit(ctx->eloop, EXIT_FAILURE);
		return;
	}
	if ((size_t)rlen != msglen) {
		errno = EINVAL;
		logerr("%s: recvmsg msg", __func__);
		eloop_exit(ctx->eloop, EXIT_FAILURE);
		return;
	}

	/* Send to our peer */
	TAILQ_FOREACH(fdl, &ctx->control_fds, next) {
		if (fdl->id != peer_id)
			continue;
		if (control_queuef(fdl, ctx->io_buf, (size_t)msglen, 0) == -1)
			logerr("%s: control_queue", __func__);
		break;
	}
}

static void
ps_ctl_listen(void *arg, unsigned short events)
{
	struct dhcpcd_ctx *ctx = arg;
	ssize_t len;
	size_t msglen;
	struct iovec iov[] = { {
	    .iov_base = &msglen,
	    .iov_len = sizeof(msglen),
	} };
	struct msghdr msg = {
		.msg_iov = iov,
		.msg_iovlen = __arraycount(iov),
	};
	int fd;
	struct fd_list *fdl;

	if (events & ELE_HANGUP) {
	hangup:
		eloop_exit(ctx->eloop, EXIT_SUCCESS);
		return;
	}

	if (!(events & ELE_READ))
		logerrx("%s: unexpected event 0x%04x", __func__, events);

	fd = ctx->ps_control->fd;
	len = recvmsg(fd, &msg, MSG_WAITALL);
	if (len == 0)
		goto hangup;
	if (len != sizeof(msglen)) {
		logerr("%s: recvmsg len %zd", __func__, len);
		goto err;
	}

	if (ps_bufalloc(ctx, msglen) == -1) {
		logerr("%s: realloc", __func__);
		goto err;
	}

	iov->iov_base = ctx->ps_buf;
	iov->iov_len = msglen;
	len = recvmsg(fd, &msg, MSG_WAITALL);
	if (len == 0)
		goto hangup;
	if ((size_t)len != msglen) {
		logerr("%s: recvmsg", __func__);
		goto err;
	}

	/* Send to our listeners */
	TAILQ_FOREACH(fdl, &ctx->control_fds, next) {
		if (!(fdl->flags & FD_LISTEN))
			continue;
		if (control_queue(fdl, ctx->ps_buf, msglen) == -1)
			logerr("%s: control_queue", __func__);
	}

	return;

err:
	eloop_exit(ctx->eloop, EXIT_FAILURE);
}

pid_t
ps_ctl_start(struct dhcpcd_ctx *ctx)
{
	struct ps_id id = {
		.psi_ifindex = 0,
		.psi_cmd = PS_CTL,
	};
	struct ps_process *psp;
	int work_fd[2], listen_fd[2];
	pid_t pid;

	if_closesockets(ctx);

	if (xsocketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, work_fd) ==
		-1 ||
	    xsocketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, listen_fd) ==
		-1)
		return -1;
#ifdef PRIVSEP_RIGHTS
	if (ps_rights_limit_fdpair(work_fd) == -1 ||
	    ps_rights_limit_fdpair(listen_fd) == -1)
		return -1;
#endif

	psp = ctx->ps_ctl = ps_newprocess(ctx, &id);
	strlcpy(psp->psp_name, "control proxy", sizeof(psp->psp_name));
	pid = ps_startprocess(psp, ps_ctl_recvmsg, ps_ctl_dodispatch,
	    ps_ctl_startcb, PSF_DROPPRIVS);

	if (pid == -1)
		return -1;
	else if (pid != 0) {
		psp->psp_work_fd = work_fd[0];
		close(work_fd[1]);
		close(listen_fd[1]);
		ctx->ps_control = control_new(ctx, listen_fd[0], FD_LISTEN);
		if (ctx->ps_control == NULL)
			return -1;
		return pid;
	}

	close(work_fd[0]);
	close(listen_fd[0]);

	psp->psp_work_fd = work_fd[1];
	if (eloop_event_add(ctx->eloop, psp->psp_work_fd, ELE_READ, ps_ctl_recv,
		ctx) == -1)
		return -1;

	ctx->ps_control = control_new(ctx, listen_fd[1], 0);
	if (ctx->ps_control == NULL)
		return -1;
	if (eloop_event_add(ctx->eloop, ctx->ps_control->fd, ELE_READ,
		ps_ctl_listen, ctx) == -1)
		return -1;

	ps_entersandbox("stdio inet", NULL);
	return 0;
}

int
ps_ctl_stop(struct dhcpcd_ctx *ctx)
{
	return ps_stopprocess(ctx->ps_ctl);
}

ssize_t
ps_ctl_sendmsg(struct fd_list *fd, const struct msghdr *msg)
{
	struct dhcpcd_ctx *ctx = fd->ctx;
	uint16_t cmd;
	unsigned long flags = (unsigned long)fd->id;

	if (fd->flags & FD_CONTROL)
		cmd = PS_CTL_CONTROL;
	else if (fd->flags & FD_READ)
		cmd = PS_CTL_READ;
	else
		cmd = PS_CTL;
	return ps_sendmsg(ctx, PS_CTL_FD(ctx), cmd, flags, msg);
}
