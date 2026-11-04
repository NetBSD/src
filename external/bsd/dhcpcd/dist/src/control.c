/*
 * dhcpcd - DHCP client daemon
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

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/un.h>

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h" // IWYU pragma: keep
#include "common.h"
#include "control.h"
#include "dhcpcd.h"
#include "eloop.h"
#include "if.h"
#include "logerr.h"
#include "privsep.h"

#ifndef SUN_LEN
#define SUN_LEN(su) \
	(sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

#define SUN_MODE_USR   (S_IRUSR | S_IWUSR)
#define SUN_MODE_GRP   (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define SUN_MODE_ALL   (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define GID_SET(gid)   ((gid) != (gid_t) - 1)

#define LISTEN_BACKLOG 5

static void control_handle_data(void *, unsigned short);

static void
control_queue_free(struct fd_list *fd)
{
	struct fd_data *fdp;

	while ((fdp = TAILQ_FIRST(&fd->queue))) {
		TAILQ_REMOVE(&fd->queue, fdp, next);
		if (fdp->data_size != 0)
			free(fdp->data);
		free(fdp);
	}

#ifdef CTL_FREE_LIST
	while ((fdp = TAILQ_FIRST(&fd->free_queue))) {
		TAILQ_REMOVE(&fd->free_queue, fdp, next);
		if (fdp->data_size != 0)
			free(fdp->data);
		free(fdp);
	}
#endif
}

void
control_free(struct fd_list *fd)
{
	eloop_event_delete(fd->ctx->eloop, fd->fd);
	close(fd->fd);
	TAILQ_REMOVE(&fd->ctx->control_fds, fd, next);
	control_queue_free(fd);
	free(fd);
}

static void
control_hangup(struct fd_list *fd)
{
	control_free(fd);
}

static ssize_t
control_handle_read(struct fd_list *fd)
{
	uid_t uid;
	gid_t gid, in_gid;
	char buf[BUFSIZ];
	struct iovec iov[] = { {
	    .iov_base = buf,
	    .iov_len = sizeof(buf),
	} };
	struct msghdr msg = {
		.msg_iov = iov,
		.msg_iovlen = __arraycount(iov),
	};
	ssize_t bytes, err;

	bytes = recvmsg(fd->fd, &msg, 0);
	if (bytes == -1)
		logerr(__func__);
	if (bytes == -1 || bytes == 0)
		return bytes;

	if (getpeereid(fd->fd, &uid, &gid) == -1) {
		logerr("%s: getpeereid", __func__);
		return -1;
	}

#ifdef PRIVSEP
	if (IN_PRIVSEP(fd->ctx)) {
		in_gid = fd->ctx->control_group;
		err = ps_root_user_ingroup(fd->ctx, uid, gid, in_gid);
		if (err == -1) {
			logerr(__func__);
			return -1;
		}
		if (err == 0) {
			fd->flags &= ~FD_CONTROL;
			in_gid = fd->ctx->read_group;
			err = ps_root_user_ingroup(fd->ctx, uid, gid, in_gid);
			if (err == -1) {
				logerr(__func__);
				return -1;
			}
			if (err == 0)
				fd->flags &= ~FD_READ;
			else
				fd->flags |= FD_READ;
		} else
			fd->flags |= FD_CONTROL | FD_READ;

		err = ps_ctl_handleargs(fd, buf, (size_t)bytes);
		if (err == -1) {
			logerr(__func__);
			return 0;
		}

		if (fd->flags & FD_LISTEN)
			fd->flags &= ~FD_COMMAND;
		else
			fd->flags |= FD_COMMAND;

		iov[0].iov_len = (size_t)bytes;
		/* If ps_ctl_handleargs returns 0 that means it didn't
		 * do anything with the command, so pass it to the manager. */
		if (err == 0 && ps_ctl_sendmsg(fd, &msg) == -1) {
			logerr(__func__);
			return -1;
		}
		return 1;
	}
#endif

	in_gid = fd->ctx->control_group;
	err = control_user_ingroup(uid, gid, in_gid);
	if (err == -1) {
		logerr(__func__);
		return -1;
	}
	if (err == 0) {
		fd->flags &= ~FD_CONTROL;
		in_gid = fd->ctx->read_group;
		err = control_user_ingroup(uid, gid, in_gid);
		if (err == -1) {
			logerr(__func__);
			return -1;
		}
		if (err == 0)
			fd->flags &= ~FD_READ;
		else
			fd->flags |= FD_READ;
	} else
		fd->flags |= FD_CONTROL | FD_READ;
	return control_recvmsg(fd, &msg, (size_t)bytes);
}

static ssize_t
control_handle_write(struct fd_list *fd)
{
	struct iovec iov[4];
	struct msghdr msg = { .msg_iov = iov };
	struct fd_data *data;
	ssize_t len;
#ifdef PRIVSEP
	size_t peer_len;
#endif

	data = TAILQ_FIRST(&fd->queue);

#ifdef PRIVSEP
	if (data->data_peer_id != 0) {
		/* Control messages for a peer are prefixed with it's fd
		 * fd and message length. */
		iov[msg.msg_iovlen].iov_base = &data->data_peer_id;
		iov[msg.msg_iovlen].iov_len = sizeof(data->data_peer_id);
		msg.msg_iovlen++;

		peer_len = data->data_len;
		if (data->data_flags & FD_DATA_SENDLEN)
			peer_len += sizeof(data->data_len);
		iov[msg.msg_iovlen].iov_base = &peer_len;
		iov[msg.msg_iovlen].iov_len = sizeof(peer_len);
		msg.msg_iovlen++;
	}
#endif

	if (data->data_flags & FD_DATA_SENDLEN) {
		iov[msg.msg_iovlen].iov_base = &data->data_len;
		iov[msg.msg_iovlen].iov_len = sizeof(data->data_len);
		msg.msg_iovlen++;
	}

	iov[msg.msg_iovlen].iov_base = data->data;
	iov[msg.msg_iovlen].iov_len = data->data_len;
	msg.msg_iovlen++;

	len = sendmsg(fd->fd, &msg, 0);
	if (len == -1) {
		if (errno != EPIPE && errno != ENOTCONN) {
			// We don't get ELE_HANGUP for some reason
			logerr("%s: write", __func__);
		}
		control_hangup(fd);
		return -1;
	}

	TAILQ_REMOVE(&fd->queue, data, next);
#ifdef CTL_FREE_LIST
	TAILQ_INSERT_TAIL(&fd->free_queue, data, next);
#else
	if (data->data_size != 0)
		free(data->data);
	free(data);
#endif

	if (TAILQ_FIRST(&fd->queue) != NULL)
		return len;

	/* Done sending data, stop watching write to fd */
	if (eloop_event_add(fd->ctx->eloop, fd->fd, ELE_READ,
		control_handle_data, fd) == -1)
		logerr("%s: eloop_event_add", __func__);
	return len;
}

static void
control_handle_data(void *arg, unsigned short events)
{
	struct fd_list *fd = arg;
	ssize_t err;

	if (events & ELE_HANGUP)
		goto hangup;

	if (!(events & (ELE_READ | ELE_WRITE | ELE_HANGUP)))
		logerrx("%s: unexpected event 0x%04x", __func__, events);

	if (events & ELE_WRITE && !(events & ELE_HANGUP)) {
		err = control_handle_write(fd);
		if (err == -1)
			goto hangup;
	}
	if (events & ELE_READ) {
		err = control_handle_read(fd);
		if ((err == -1 && errno != EPERM) || err == 0)
			goto hangup;
	}

	return;

hangup:
	control_hangup(fd);
}

int
control_recvmsg(struct fd_list *fd, struct msghdr *msg, size_t len)
{
	struct iovec *iov;
	char *p, *e;
	char *argvp[255], **ap;
	int argc;

	if (msg->msg_iovlen == 0) {
		errno = EINVAL;
		return -1;
	}

	iov = msg->msg_iov;
	p = (char *)iov->iov_base;

	/* Each command is \n terminated
	 * Each argument is NULL separated */
	while (len != 0) {
		argc = 0;
		ap = argvp;
		while (len != 0) {
			if (*p == '\0') {
				p++;
				len--;
				continue;
			}
			e = memchr(p, '\0', len);
			if (e == NULL) {
				errno = EINVAL;
				logerrx("%s: no terminator", __func__);
				return -1;
			}
			if ((size_t)argc + 1 >=
			    sizeof(argvp) / sizeof(argvp[0])) {
				errno = ENOBUFS;
				logerrx("%s: no arg buffer", __func__);
				return -1;
			}
			*ap++ = p;
			argc++;
			e++;
			len -= (size_t)(e - p);
			p = e;
			e--;
			if (*(--e) == '\n') {
				*e = '\0';
				break;
			}
		}
		if (argc == 0) {
			logerrx("%s: no args", __func__);
			continue;
		}
		*ap = NULL;
		if (dhcpcd_handleargs(fd->ctx, fd, argc, argvp) == -1) {
			logerr("%s: dhcpcd_handleargs", __func__);
			if (errno != EINTR && errno != EAGAIN && errno != EPERM)
				return -1;
		}
		if (!(fd->flags & FD_LISTEN))
			fd->flags |= FD_COMMAND;
	}

	return 1;
}

struct fd_list *
control_find(struct dhcpcd_ctx *ctx, int fd)
{
	struct fd_list *l;

	TAILQ_FOREACH(l, &ctx->control_fds, next) {
		if (l->fd == fd)
			return l;
	}

	errno = ESRCH;
	return NULL;
}

struct fd_list *
control_new(struct dhcpcd_ctx *ctx, int fd, unsigned int flags)
{
	struct fd_list *l;
	size_t cnt;
	struct fd_list *n;
#ifdef PRIVSEP
	unsigned int id;
#endif

	l = control_find(ctx, fd);
	if (l != NULL) {
		l->flags = flags;
		return l;
	}

#ifdef PRIVSEP
again:
	id = ++ctx->ps_control_id;
	if (id == 0) /* wrapped */
		id = ++ctx->ps_control_id;
#endif
	cnt = 0;
	TAILQ_FOREACH(n, &ctx->control_fds, next) {
		if (++cnt >= CONTROL_PEER_MAX) {
			errno = ENOBUFS;
			return NULL;
		}
#ifdef PRIVSEP
		if (n->id == id)
			goto again;
#endif
	}

	l = malloc(sizeof(*l));
	if (l == NULL)
		return NULL;

	l->ctx = ctx;
	l->fd = fd;
	l->flags = flags;
	TAILQ_INIT(&l->queue);
#ifdef CTL_FREE_LIST
	TAILQ_INIT(&l->free_queue);
#endif
#ifdef PRIVSEP
	l->peer_id = 0;
	l->id = id;
#endif
	TAILQ_INSERT_TAIL(&ctx->control_fds, l, next);
	return l;
}

static void
control_handle1(struct dhcpcd_ctx *ctx, int lfd, unsigned int fd_flags,
    unsigned short events)
{
	struct sockaddr_un run;
	socklen_t len;
	struct fd_list *l;
	int fd, flags = 1;

	if (events != ELE_READ)
		logerrx("%s: unexpected event 0x%04x", __func__, events);

	len = sizeof(run);
	if ((fd = accept(lfd, (struct sockaddr *)&run, &len)) == -1)
		goto error;
	if ((flags = fcntl(fd, F_GETFD, 0)) == -1 ||
	    fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
		goto error;
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1 ||
	    fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		goto error;

	l = control_new(ctx, fd, fd_flags);
	if (l == NULL)
		goto error;

	if (eloop_event_add(ctx->eloop, l->fd, ELE_READ, control_handle_data,
		l) == -1)
		logerr("%s: eloop_event_add", __func__);

	return;

error:
	logerr(__func__);
	if (fd != -1)
		close(fd);
}

static void
control_handle(void *arg, unsigned short events)
{
	struct dhcpcd_ctx *ctx = arg;

	control_handle1(ctx, ctx->control_fd, 0, events);
}

static int
make_path(char *path, size_t len, const char *ifname, sa_family_t family)
{
	const char *per;

	switch (family) {
	case AF_INET:
		per = "-4";
		break;
	case AF_INET6:
		per = "-6";
		break;
	default:
		per = "";
		break;
	}
	return snprintf(path, len, CONTROLSOCKET, ifname ? ifname : "",
	    ifname ? per : "", "", ifname ? "." : "");
}

static int
make_sock(struct sockaddr_un *sa, const char *ifname, sa_family_t family)
{
	int fd;

	if ((fd = xsocket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) == -1)
		return -1;
	memset(sa, 0, sizeof(*sa));
	sa->sun_family = AF_UNIX;
	make_path(sa->sun_path, sizeof(sa->sun_path), ifname, family);
	return fd;
}

static int
control_start1(struct dhcpcd_ctx *ctx, const char *ifname, sa_family_t family)
{
	struct sockaddr_un sa;
	mode_t mode;
	gid_t grp = getgid();
	int fd;
	socklen_t len;

	fd = make_sock(&sa, ifname, family);
	if (fd == -1)
		return -1;

	len = (socklen_t)SUN_LEN(&sa);
	if (GID_SET(ctx->control_group) && GID_SET(ctx->read_group)) {
		if (ctx->control_group == ctx->read_group) {
			mode = SUN_MODE_GRP;
			grp = ctx->control_group;
		} else
			mode = SUN_MODE_ALL;
	} else if (!GID_SET(ctx->control_group) && !GID_SET(ctx->read_group))
		mode = SUN_MODE_USR;
	else if (GID_SET(ctx->control_group)) {
		mode = SUN_MODE_GRP;
		grp = ctx->control_group;
	} else {
		mode = SUN_MODE_GRP;
		grp = ctx->read_group;
	}

	unlink(sa.sun_path);
	if (bind(fd, (struct sockaddr *)&sa, len) == -1 ||
	    chmod(sa.sun_path, mode) == -1 ||
	    chown(sa.sun_path, geteuid(), grp) == -1 ||
	    listen(fd, LISTEN_BACKLOG) == -1)
		goto err;

#ifdef PRIVSEP_RIGHTS
	if (IN_PRIVSEP(ctx) && ps_rights_limit_fd_getsockopt(fd) == -1)
		goto err;
#endif

	strlcpy(ctx->control_sock, sa.sun_path, sizeof(ctx->control_sock));
	return fd;

err:
	close(fd);
	unlink(sa.sun_path);
	return -1;
}

int
control_start(struct dhcpcd_ctx *ctx, const char *ifname, sa_family_t family)
{
	int fd;

#ifdef PRIVSEP
	if (IN_PRIVSEP_SE(ctx)) {
		make_path(ctx->control_sock, sizeof(ctx->control_sock), ifname,
		    family);
		return 0;
	}
#endif

	if ((fd = control_start1(ctx, ifname, family)) == -1)
		return -1;

	ctx->control_fd = fd;
	if (eloop_event_add(ctx->eloop, fd, ELE_READ, control_handle, ctx) ==
	    -1)
		logerr("%s: eloop_event_add", __func__);

	return ctx->control_fd;
}

static int
control_unlink(struct dhcpcd_ctx *ctx, const char *file)
{
	int retval = 0;

	errno = 0;
#ifdef PRIVSEP
	if (IN_PRIVSEP(ctx))
		retval = (int)ps_root_unlink(ctx, file);
	else
#else
	UNUSED(ctx);
#endif
		retval = unlink(file);

	return retval == -1 && errno != ENOENT ? -1 : 0;
}

int
control_stop(struct dhcpcd_ctx *ctx)
{
	int retval = 0;
	struct fd_list *l;

	while ((l = TAILQ_FIRST(&ctx->control_fds)) != NULL) {
		control_free(l);
	}

#ifdef PRIVSEP
	if (IN_PRIVSEP_SE(ctx)) {
		if (ctx->control_sock[0] != '\0' &&
		    ps_root_unlink(ctx, ctx->control_sock) == -1)
			retval = -1;
		return retval;
	} else if (ctx->options & DHCPCD_FORKED)
		return retval;
#endif

	if (ctx->control_fd != -1) {
		eloop_event_delete(ctx->eloop, ctx->control_fd);
		close(ctx->control_fd);
		ctx->control_fd = -1;
		if (control_unlink(ctx, ctx->control_sock) == -1)
			retval = -1;
	}

	return retval;
}

int
control_open(const char *ifname, sa_family_t family)
{
	struct sockaddr_un sa;
	int fd;

	if ((fd = make_sock(&sa, ifname, family)) != -1) {
		socklen_t len;

		len = (socklen_t)SUN_LEN(&sa);
		if (connect(fd, (struct sockaddr *)&sa, len) == -1) {
			close(fd);
			fd = -1;
		}
	}
	return fd;
}

ssize_t
control_send(struct dhcpcd_ctx *ctx, int argc, char *const *argv)
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

ssize_t
control_queuef(struct fd_list *fd, const void *data, size_t data_len,
    unsigned int flags)
{
	struct fd_data *d;
	unsigned short events;

	if (data_len == 0) {
		errno = EINVAL;
		return -1;
	}

#ifdef CTL_FREE_LIST
	struct fd_data *df;

	d = NULL;
	TAILQ_FOREACH(df, &fd->free_queue, next) {
		if (d == NULL || d->data_size < df->data_size) {
			d = df;
			if (d->data_size <= data_len)
				break;
		}
	}
	if (d != NULL)
		TAILQ_REMOVE(&fd->free_queue, d, next);
	else
#endif
	{
		d = calloc(1, sizeof(*d));
		if (d == NULL)
			return -1;
	}

	if (d->data_size == 0)
		d->data = NULL;
	if (d->data_size < data_len) {
		void *nbuf = realloc(d->data, data_len);
		if (nbuf == NULL) {
			free(d->data);
			free(d);
			return -1;
		}
		d->data = nbuf;
		d->data_size = data_len;
	}
	memcpy(d->data, data, data_len);
	d->data_len = data_len;
	d->data_flags = flags;
#ifdef PRIVSEP
	d->data_peer_id = fd->peer_id;
#endif

	TAILQ_INSERT_TAIL(&fd->queue, d, next);
	events = ELE_WRITE;
	if (fd->flags & FD_LISTEN)
		events |= ELE_READ;
	if (eloop_event_add(fd->ctx->eloop, fd->fd, events, control_handle_data,
		fd) == -1)
		return -1;
	return (ssize_t)d->data_len;
}

ssize_t
control_queue(struct fd_list *fd, const void *data, size_t data_len)
{
	return control_queuef(fd, data, data_len, FD_DATA_SENDLEN);
}

int
control_user_ingroup(uid_t uid, gid_t gid, gid_t grpid)
{
#ifdef __APPLE__
	/* why is getgrouplist API is different ..... */
	int *groups = NULL, *gp;
#define GID (int)
#else
	gid_t *groups = NULL, *gp;
#define GID
#endif
	struct passwd *pw;
	int ngroups = 10, err = -1;

	/* Allow root */
	if (uid == 0)
		return 1;
	if (!GID_SET(grpid))
		return 0;

	pw = getpwuid(uid);
	if (pw == NULL)
		return 0; /* unknown user is not privileged */

	groups = reallocarray(groups, (size_t)ngroups, sizeof(*groups));
	if (groups == NULL)
		return -1;

	if (getgrouplist(pw->pw_name, GID gid, groups, &ngroups) == -1) {
		gp = reallocarray(groups, (size_t)ngroups, sizeof(*groups));
		if (gp == NULL)
			goto out;
		groups = gp;
		if (getgrouplist(pw->pw_name, GID gid, groups, &ngroups) == -1)
			goto out;
	}

	for (gp = groups; ngroups != 0; ngroups--, gp++) {
		if (*gp == GID grpid) {
			err = 1;
			goto out;
		}
	}
	err = 0;

out:
	free(groups);
	return err;
}

ssize_t
control_handle_listen(struct fd_list *fd)
{
	int err;

	if (fd->flags & FD_READ) {
		err = 0;
		if (!(fd->flags & FD_COMMAND))
			fd->flags |= FD_LISTEN;
	} else {
		err = errno = EPERM;
		logwarn(__func__);
	}
	return control_queuef(fd, &err, sizeof(err), 0);
}
