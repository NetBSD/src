/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Privilege Separation for dhcpcd
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

/*
 * The current design is this:
 * Spawn a priv process to carry out privileged actions and
 * spawning unpriv process to initate network connections such as BPF
 * or address specific listener.
 * Spawn an unpriv process to send/receive common network data.
 * Then drop all privs and start running.
 * Every process aside from the privileged actioneer is chrooted.
 * All privsep processes ignore signals - only the master process accepts them.
 *
 * dhcpcd will maintain the config file in the chroot, no need to handle
 * this in a script or something.
 */

#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef AF_LINK
#include <net/if_dl.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <stddef.h>	/* For offsetof, struct padding debug */
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arp.h"
#include "common.h"
#include "control.h"
#include "dev.h"
#include "dhcp.h"
#include "dhcp6.h"
#include "eloop.h"
#include "ipv6nd.h"
#include "logerr.h"
#include "privsep.h"

#ifdef HAVE_CAPSICUM
#include <sys/capsicum.h>
#include <capsicum_helpers.h>
#endif
#ifdef HAVE_UTIL_H
#include <util.h>
#endif

int
ps_init(struct dhcpcd_ctx *ctx)
{
	struct passwd *pw;
	struct stat st;

	errno = 0;
	if ((ctx->ps_user = pw = getpwnam(PRIVSEP_USER)) == NULL) {
		ctx->options &= ~DHCPCD_PRIVSEP;
		if (errno == 0) {
			logerrx("no such user %s", PRIVSEP_USER);
			/* Just incase logerrx caused an error... */
			errno = 0;
		} else
			logerr("getpwnam");
		return -1;
	}

	if (stat(pw->pw_dir, &st) == -1 || !S_ISDIR(st.st_mode)) {
		ctx->options &= ~DHCPCD_PRIVSEP;
		logerrx("refusing chroot: %s: %s",
		    PRIVSEP_USER, pw->pw_dir);
		errno = 0;
		return -1;
	}

	ctx->options |= DHCPCD_PRIVSEP;
	return 0;
}

static int
ps_dropprivs(struct dhcpcd_ctx *ctx)
{
	struct passwd *pw = ctx->ps_user;

	if (ctx->options & DHCPCD_LAUNCHER)
		logdebugx("chrooting as %s to %s", pw->pw_name, pw->pw_dir);
	if (chroot(pw->pw_dir) == -1 &&
	    (errno != EPERM || ctx->options & DHCPCD_FORKED))
		logerr("%s: chroot: %s", __func__, pw->pw_dir);
	if (chdir("/") == -1)
		logerr("%s: chdir: /", __func__);

	if ((setgroups(1, &pw->pw_gid) == -1 ||
	     setgid(pw->pw_gid) == -1 ||
	     setuid(pw->pw_uid) == -1) &&
	     (errno != EPERM || ctx->options & DHCPCD_FORKED))
	{
		logerr("failed to drop privileges");
		return -1;
	}

	struct rlimit rzero = { .rlim_cur = 0, .rlim_max = 0 };

	if (ctx->ps_control_pid != getpid()) {
		/* Prohibit new files, sockets, etc */
#if defined(__linux__) || defined(__sun) || defined(__OpenBSD__)
		/*
		 * If poll(2) is called with nfds > RLIMIT_NOFILE
		 * then it returns EINVAL.
		 * This blows.
		 * Do the best we can and limit to what we need.
		 * An attacker could potentially close a file and
		 * open a new one still, but that cannot be helped.
		 */
		unsigned long maxfd;
		maxfd = (unsigned long)eloop_event_count(ctx->eloop);
		if (IN_PRIVSEP_SE(ctx))
			maxfd++; /* XXX why? */

		struct rlimit rmaxfd = {
		    .rlim_cur = maxfd,
		    .rlim_max = maxfd
		};
		if (setrlimit(RLIMIT_NOFILE, &rmaxfd) == -1)
			logerr("setrlimit RLIMIT_NOFILE");
#else
		if (setrlimit(RLIMIT_NOFILE, &rzero) == -1)
			logerr("setrlimit RLIMIT_NOFILE");
#endif
	}

#define DHC_NOCHKIO	(DHCPCD_STARTED | DHCPCD_DAEMONISE)
	/* Prohibit writing to files.
	 * Obviously this won't work if we are using a logfile
	 * or redirecting stderr to a file. */
	if ((ctx->options & DHC_NOCHKIO) == DHC_NOCHKIO ||
	    (ctx->logfile == NULL &&
	    (!ctx->stderr_valid || isatty(STDERR_FILENO) == 1)))
	{
		if (setrlimit(RLIMIT_FSIZE, &rzero) == -1)
			logerr("setrlimit RLIMIT_FSIZE");
	}

#ifdef RLIMIT_NPROC
	/* Prohibit forks */
	if (setrlimit(RLIMIT_NPROC, &rzero) == -1)
		logerr("setrlimit RLIMIT_NPROC");
#endif

	return 0;
}

static int
ps_setbuf0(int fd, int ctl, int minlen)
{
	int len;
	socklen_t slen;

	slen = sizeof(len);
	if (getsockopt(fd, SOL_SOCKET, ctl, &len, &slen) == -1)
		return -1;

#ifdef __linux__
	len /= 2;
#endif
	if (len >= minlen)
		return 0;

	return setsockopt(fd, SOL_SOCKET, ctl, &minlen, sizeof(minlen));
}

static int
ps_setbuf(int fd)
{
	/* Ensure we can receive a fully sized privsep message.
	 * Double the send buffer. */
	int minlen = (int)sizeof(struct ps_msg);

	if (ps_setbuf0(fd, SO_RCVBUF, minlen) == -1 ||
	    ps_setbuf0(fd, SO_SNDBUF, minlen * 2) == -1)
	{
		logerr(__func__);
		return -1;
	}
	return 0;
}

int
ps_setbuf_fdpair(int fd[])
{

	if (ps_setbuf(fd[0]) == -1 || ps_setbuf(fd[1]) == -1)
		return -1;
	return 0;
}

#ifdef PRIVSEP_RIGHTS
int
ps_rights_limit_ioctl(int fd)
{
	cap_rights_t rights;

	cap_rights_init(&rights, CAP_IOCTL);
	if (cap_rights_limit(fd, &rights) == -1 && errno != ENOSYS)
		return -1;
	return 0;
}

int
ps_rights_limit_fd_fctnl(int fd)
{
	cap_rights_t rights;

	cap_rights_init(&rights, CAP_READ, CAP_WRITE, CAP_EVENT,
	    CAP_ACCEPT, CAP_FCNTL);
	if (cap_rights_limit(fd, &rights) == -1 && errno != ENOSYS)
		return -1;
	return 0;
}

int
ps_rights_limit_fd(int fd)
{
	cap_rights_t rights;

	cap_rights_init(&rights, CAP_READ, CAP_WRITE, CAP_EVENT, CAP_SHUTDOWN);
	if (cap_rights_limit(fd, &rights) == -1 && errno != ENOSYS)
		return -1;
	return 0;
}

int
ps_rights_limit_fd_sockopt(int fd)
{
	cap_rights_t rights;

	cap_rights_init(&rights, CAP_READ, CAP_WRITE, CAP_EVENT,
	    CAP_GETSOCKOPT, CAP_SETSOCKOPT);
	if (cap_rights_limit(fd, &rights) == -1 && errno != ENOSYS)
		return -1;
	return 0;
}

int
ps_rights_limit_fd_rdonly(int fd)
{
	cap_rights_t rights;

	cap_rights_init(&rights, CAP_READ, CAP_EVENT);
	if (cap_rights_limit(fd, &rights) == -1 && errno != ENOSYS)
		return -1;
	return 0;
}

int
ps_rights_limit_fdpair(int fd[])
{

	if (ps_rights_limit_fd(fd[0]) == -1 || ps_rights_limit_fd(fd[1]) == -1)
		return -1;
	return 0;
}

static int
ps_rights_limit_stdio(struct dhcpcd_ctx *ctx)
{
	const int iebadf = CAPH_IGNORE_EBADF;
	int error = 0;

	if (ctx->stdin_valid &&
	    caph_limit_stream(STDIN_FILENO, CAPH_READ | iebadf) == -1)
		error = -1;
	if (ctx->stdout_valid &&
	    caph_limit_stream(STDOUT_FILENO, CAPH_WRITE | iebadf) == -1)
		error = -1;
	if (ctx->stderr_valid &&
	    caph_limit_stream(STDERR_FILENO, CAPH_WRITE | iebadf) == -1)
		error = -1;

	return error;
}
#endif

pid_t
ps_dostart(struct dhcpcd_ctx *ctx,
    pid_t *priv_pid, int *priv_fd,
    void (*recv_msg)(void *), void (*recv_unpriv_msg),
    void *recv_ctx, int (*callback)(void *), void (*signal_cb)(int, void *),
    unsigned int flags)
{
	int fd[2];
	pid_t pid;

	if (xsocketpair(AF_UNIX, SOCK_DGRAM | SOCK_CXNB, 0, fd) == -1) {
		logerr("%s: socketpair", __func__);
		return -1;
	}
	if (ps_setbuf_fdpair(fd) == -1) {
		logerr("%s: ps_setbuf_fdpair", __func__);
		return -1;
	}
#ifdef PRIVSEP_RIGHTS
	if (ps_rights_limit_fdpair(fd) == -1) {
		logerr("%s: ps_rights_limit_fdpair", __func__);
		return -1;
	}
#endif

	switch (pid = fork()) {
	case -1:
		logerr("fork");
		return -1;
	case 0:
		*priv_fd = fd[1];
		close(fd[0]);
		break;
	default:
		*priv_pid = pid;
		*priv_fd = fd[0];
		close(fd[1]);
		if (recv_unpriv_msg == NULL)
			;
		else if (eloop_event_add(ctx->eloop, *priv_fd,
		    recv_unpriv_msg, recv_ctx) == -1)
		{
			logerr("%s: eloop_event_add", __func__);
			return -1;
		}
		return pid;
	}

	ctx->options |= DHCPCD_FORKED;
	if (ctx->fork_fd != -1) {
		close(ctx->fork_fd);
		ctx->fork_fd = -1;
	}
	pidfile_clean();
	eloop_clear(ctx->eloop);

	/* We are not root */
	if (priv_fd != &ctx->ps_root_fd) {
		ps_freeprocesses(ctx, recv_ctx);
		if (ctx->ps_root_fd != -1) {
			close(ctx->ps_root_fd);
			ctx->ps_root_fd = -1;
		}

#ifdef PRIVSEP_RIGHTS
		/* We cannot limit the root process in any way. */
		if (ps_rights_limit_stdio(ctx) == -1) {
			logerr("ps_rights_limit_stdio");
			goto errexit;
		}
#endif
	}

	if (priv_fd != &ctx->ps_inet_fd && ctx->ps_inet_fd != -1) {
		close(ctx->ps_inet_fd);
		ctx->ps_inet_fd = -1;
	}

	eloop_signal_set_cb(ctx->eloop,
	    dhcpcd_signals, dhcpcd_signals_len, signal_cb, ctx);

	/* ctx->sigset aready has the initial sigmask set in main() */
	if (eloop_signal_mask(ctx->eloop, NULL) == -1) {
		logerr("%s: eloop_signal_mask", __func__);
		goto errexit;
	}

	if (eloop_event_add(ctx->eloop, *priv_fd, recv_msg, recv_ctx) == -1)
	{
		logerr("%s: eloop_event_add", __func__);
		goto errexit;
	}

	if (callback(recv_ctx) == -1)
		goto errexit;

	if (flags & PSF_DROPPRIVS)
		ps_dropprivs(ctx);

	return 0;

errexit:
	/* Failure to start root or inet processes is fatal. */
	if (priv_fd == &ctx->ps_root_fd || priv_fd == &ctx->ps_inet_fd)
		(void)ps_sendcmd(ctx, *priv_fd, PS_STOP, 0, NULL, 0);
	shutdown(*priv_fd, SHUT_RDWR);
	*priv_fd = -1;
	eloop_exit(ctx->eloop, EXIT_FAILURE);
	return -1;
}

int
ps_dostop(struct dhcpcd_ctx *ctx, pid_t *pid, int *fd)
{
	int err = 0;

#ifdef PRIVSEP_DEBUG
	logdebugx("%s: pid=%d fd=%d", __func__, *pid, *fd);
#endif

	if (*fd != -1) {
		eloop_event_delete(ctx->eloop, *fd);
		if (ps_sendcmd(ctx, *fd, PS_STOP, 0, NULL, 0) == -1) {
			logerr(__func__);
			err = -1;
		}
		(void)shutdown(*fd, SHUT_RDWR);
		close(*fd);
		*fd = -1;
	}

	/* Don't wait for the process as it may not respond to the shutdown
	 * request. We'll reap the process on receipt of SIGCHLD. */
	*pid = 0;
	return err;
}

int
ps_start(struct dhcpcd_ctx *ctx)
{
	pid_t pid;

	TAILQ_INIT(&ctx->ps_processes);

	switch (pid = ps_root_start(ctx)) {
	case -1:
		logerr("ps_root_start");
		return -1;
	case 0:
		return 0;
	default:
		logdebugx("spawned privileged actioneer on PID %d", pid);
	}

	/* No point in spawning the generic network listener if we're
	 * not going to use it. */
	if (!ps_inet_canstart(ctx))
		goto started_net;

	switch (pid = ps_inet_start(ctx)) {
	case -1:
		return -1;
	case 0:
		return 0;
	default:
		logdebugx("spawned network proxy on PID %d", pid);
	}

started_net:
	if (!(ctx->options & DHCPCD_TEST)) {
		switch (pid = ps_ctl_start(ctx)) {
		case -1:
			return -1;
		case 0:
			return 0;
		default:
			logdebugx("spawned controller proxy on PID %d", pid);
		}
	}

#ifdef ARC4RANDOM_H
	/* Seed the random number generator early incase it needs /dev/urandom
	 * which won't be available in the chroot. */
	arc4random();
#endif

	return 1;
}

int
ps_entersandbox(const char *_pledge, const char **sandbox)
{

#if !defined(HAVE_PLEDGE)
	UNUSED(_pledge);
#endif

#if defined(HAVE_CAPSICUM)
	if (sandbox != NULL)
		*sandbox = "capsicum";
	return cap_enter();
#elif defined(HAVE_PLEDGE)
	if (sandbox != NULL)
		*sandbox = "pledge";
	return pledge(_pledge, NULL);
#elif defined(HAVE_SECCOMP)
	if (sandbox != NULL)
		*sandbox = "seccomp";
	return ps_seccomp_enter();
#else
	if (sandbox != NULL)
		*sandbox = "posix resource limited";
	return 0;
#endif
}

int
ps_mastersandbox(struct dhcpcd_ctx *ctx, const char *_pledge)
{
	const char *sandbox = NULL;
	bool forked;
	int dropped;

	forked = ctx->options & DHCPCD_FORKED;
	ctx->options &= ~DHCPCD_FORKED;
	dropped = ps_dropprivs(ctx);
	if (forked)
		ctx->options |= DHCPCD_FORKED;

	/*
	 * If we don't have a root process, we cannot use syslog.
	 * If it cannot be opened before chrooting then syslog(3) will fail.
	 * openlog(3) does not return an error which doubly sucks.
	 */
	if (ctx->ps_root_fd == -1) {
		unsigned int logopts = loggetopts();

		logopts &= ~LOGERR_LOG;
		logsetopts(logopts);
	}

	if (dropped == -1) {
		logerr("%s: ps_dropprivs", __func__);
		return -1;
	}

#ifdef PRIVSEP_RIGHTS
	if ((ctx->pf_inet_fd != -1 &&
	    ps_rights_limit_ioctl(ctx->pf_inet_fd) == -1) ||
	     ps_rights_limit_stdio(ctx) == -1)
	{
		logerr("%s: cap_rights_limit", __func__);
		return -1;
	}
#endif

	if (_pledge == NULL)
		_pledge = "stdio";
	if (ps_entersandbox(_pledge, &sandbox) == -1) {
		if (errno == ENOSYS) {
			if (sandbox != NULL)
				logwarnx("sandbox unavailable: %s", sandbox);
			return 0;
		}
		logerr("%s: %s", __func__, sandbox);
		return -1;
	} else if (ctx->options & DHCPCD_LAUNCHER ||
		  ((!(ctx->options & DHCPCD_DAEMONISE)) &&
		   ctx->options & DHCPCD_MASTER))
		logdebugx("sandbox: %s", sandbox);
	return 0;
}

int
ps_stop(struct dhcpcd_ctx *ctx)
{
	int r, ret = 0;

	if (!(ctx->options & DHCPCD_PRIVSEP) ||
	    ctx->options & DHCPCD_FORKED ||
	    ctx->eloop == NULL)
		return 0;

	r = ps_ctl_stop(ctx);
	if (r != 0)
		ret = r;

	r = ps_inet_stop(ctx);
	if (r != 0)
		ret = r;

	/* We've been chrooted, so we need to tell the
	 * privileged actioneer to remove the pidfile. */
	ps_root_unlink(ctx, ctx->pidfile);

	r = ps_root_stop(ctx);
	if (r != 0)
		ret = r;

	ctx->options &= ~DHCPCD_PRIVSEP;
	return ret;
}

void
ps_freeprocess(struct ps_process *psp)
{

	TAILQ_REMOVE(&psp->psp_ctx->ps_processes, psp, next);
	if (psp->psp_fd != -1) {
		eloop_event_delete(psp->psp_ctx->eloop, psp->psp_fd);
		close(psp->psp_fd);
	}
	if (psp->psp_work_fd != -1) {
		eloop_event_delete(psp->psp_ctx->eloop, psp->psp_work_fd);
		close(psp->psp_work_fd);
	}
#ifdef INET
	if (psp->psp_bpf != NULL)
		bpf_close(psp->psp_bpf);
#endif
	free(psp);
}

static void
ps_free(struct dhcpcd_ctx *ctx)
{
	struct ps_process *psp;
	bool stop = ctx->ps_root_pid == getpid();

	while ((psp = TAILQ_FIRST(&ctx->ps_processes)) != NULL) {
		if (stop)
			ps_dostop(ctx, &psp->psp_pid, &psp->psp_fd);
		ps_freeprocess(psp);
	}
}

int
ps_unrollmsg(struct msghdr *msg, struct ps_msghdr *psm,
    const void *data, size_t len)
{
	uint8_t *datap, *namep, *controlp;

	namep = UNCONST(data);
	controlp = namep + psm->ps_namelen;
	datap = controlp + psm->ps_controllen;

	if (psm->ps_namelen != 0) {
		if (psm->ps_namelen > len) {
			errno = EINVAL;
			return -1;
		}
		msg->msg_name = namep;
		len -= psm->ps_namelen;
	} else
		msg->msg_name = NULL;
	msg->msg_namelen = psm->ps_namelen;

	if (psm->ps_controllen != 0) {
		if (psm->ps_controllen > len) {
			errno = EINVAL;
			return -1;
		}
		msg->msg_control = controlp;
		len -= psm->ps_controllen;
	} else
		msg->msg_control = NULL;
	msg->msg_controllen = psm->ps_controllen;

	if (len != 0) {
		msg->msg_iovlen = 1;
		msg->msg_iov[0].iov_base = datap;
		msg->msg_iov[0].iov_len = len;
	} else {
		msg->msg_iovlen = 0;
		msg->msg_iov[0].iov_base = NULL;
		msg->msg_iov[0].iov_len = 0;
	}
	return 0;
}

ssize_t
ps_sendpsmmsg(struct dhcpcd_ctx *ctx, int fd,
    struct ps_msghdr *psm, const struct msghdr *msg)
{
	struct iovec iov[] = {
		{ .iov_base = UNCONST(psm), .iov_len = sizeof(*psm) },
		{ .iov_base = NULL, },	/* name */
		{ .iov_base = NULL, },	/* control */
		{ .iov_base = NULL, },	/* payload 1 */
		{ .iov_base = NULL, },	/* payload 2 */
		{ .iov_base = NULL, },	/* payload 3 */
	};
	int iovlen;
	ssize_t len;

	if (msg != NULL) {
		struct iovec *iovp = &iov[1];
		int i;

		psm->ps_namelen = msg->msg_namelen;
		psm->ps_controllen = (socklen_t)msg->msg_controllen;

		iovp->iov_base = msg->msg_name;
		iovp->iov_len = msg->msg_namelen;
		iovp++;
		iovp->iov_base = msg->msg_control;
		iovp->iov_len = msg->msg_controllen;
		iovlen = 3;

		for (i = 0; i < (int)msg->msg_iovlen; i++) {
			if ((size_t)(iovlen + i) > __arraycount(iov)) {
				errno =	ENOBUFS;
				return -1;
			}
			iovp++;
			iovp->iov_base = msg->msg_iov[i].iov_base;
			iovp->iov_len = msg->msg_iov[i].iov_len;
		}
		iovlen += i;
	} else
		iovlen = 1;

	len = writev(fd, iov, iovlen);
	if (len == -1) {
		logerr(__func__);
		if (ctx->options & DHCPCD_FORKED &&
		    !(ctx->options & DHCPCD_PRIVSEPROOT))
			eloop_exit(ctx->eloop, EXIT_FAILURE);
	}
	return len;
}

ssize_t
ps_sendpsmdata(struct dhcpcd_ctx *ctx, int fd,
    struct ps_msghdr *psm, const void *data, size_t len)
{
	struct iovec iov[] = {
		{ .iov_base = UNCONST(data), .iov_len = len },
	};
	struct msghdr msg = {
		.msg_iov = iov, .msg_iovlen = 1,
	};

	return ps_sendpsmmsg(ctx, fd, psm, &msg);
}


ssize_t
ps_sendmsg(struct dhcpcd_ctx *ctx, int fd, uint16_t cmd, unsigned long flags,
    const struct msghdr *msg)
{
	struct ps_msghdr psm = {
		.ps_cmd = cmd,
		.ps_flags = flags,
		.ps_namelen = msg->msg_namelen,
		.ps_controllen = (socklen_t)msg->msg_controllen,
	};
	size_t i;

	for (i = 0; i < (size_t)msg->msg_iovlen; i++)
		psm.ps_datalen += msg->msg_iov[i].iov_len;

#if 0	/* For debugging structure padding. */
	logerrx("psa.family %lu %zu", offsetof(struct ps_addr, psa_family), sizeof(psm.ps_id.psi_addr.psa_family));
	logerrx("psa.pad %lu %zu", offsetof(struct ps_addr, psa_pad), sizeof(psm.ps_id.psi_addr.psa_pad));
	logerrx("psa.psa_u %lu %zu", offsetof(struct ps_addr, psa_u), sizeof(psm.ps_id.psi_addr.psa_u));
	logerrx("psa %zu", sizeof(psm.ps_id.psi_addr));

	logerrx("psi.addr %lu %zu", offsetof(struct ps_id, psi_addr), sizeof(psm.ps_id.psi_addr));
	logerrx("psi.index %lu %zu", offsetof(struct ps_id, psi_ifindex), sizeof(psm.ps_id.psi_ifindex));
	logerrx("psi.cmd %lu %zu", offsetof(struct ps_id, psi_cmd), sizeof(psm.ps_id.psi_cmd));
	logerrx("psi.pad %lu %zu", offsetof(struct ps_id, psi_pad), sizeof(psm.ps_id.psi_pad));
	logerrx("psi %zu", sizeof(struct ps_id));

	logerrx("ps_cmd %lu", offsetof(struct ps_msghdr, ps_cmd));
	logerrx("ps_pad %lu %zu", offsetof(struct ps_msghdr, ps_pad), sizeof(psm.ps_pad));
	logerrx("ps_flags %lu %zu", offsetof(struct ps_msghdr, ps_flags), sizeof(psm.ps_flags));

	logerrx("ps_id %lu %zu", offsetof(struct ps_msghdr, ps_id), sizeof(psm.ps_id));

	logerrx("ps_namelen %lu %zu", offsetof(struct ps_msghdr, ps_namelen), sizeof(psm.ps_namelen));
	logerrx("ps_controllen %lu %zu", offsetof(struct ps_msghdr, ps_controllen), sizeof(psm.ps_controllen));
	logerrx("ps_pad2 %lu %zu", offsetof(struct ps_msghdr, ps_pad2), sizeof(psm.ps_pad2));
	logerrx("ps_datalen %lu %zu", offsetof(struct ps_msghdr, ps_datalen), sizeof(psm.ps_datalen));
	logerrx("psm %zu", sizeof(psm));
#endif

	return ps_sendpsmmsg(ctx, fd, &psm, msg);
}

ssize_t
ps_sendcmd(struct dhcpcd_ctx *ctx, int fd, uint16_t cmd, unsigned long flags,
    const void *data, size_t len)
{
	struct ps_msghdr psm = {
		.ps_cmd = cmd,
		.ps_flags = flags,
	};
	struct iovec iov[] = {
		{ .iov_base = UNCONST(data), .iov_len = len }
	};
	struct msghdr msg = {
		.msg_iov = iov, .msg_iovlen = 1,
	};

	return ps_sendpsmmsg(ctx, fd, &psm, &msg);
}

static ssize_t
ps_sendcmdmsg(int fd, uint16_t cmd, const struct msghdr *msg)
{
	struct ps_msghdr psm = { .ps_cmd = cmd };
	uint8_t data[PS_BUFLEN], *p = data;
	struct iovec iov[] = {
		{ .iov_base = &psm, .iov_len = sizeof(psm) },
		{ .iov_base = data, .iov_len = 0 },
	};
	size_t dl = sizeof(data);

	if (msg->msg_namelen != 0) {
		if (msg->msg_namelen > dl)
			goto nobufs;
		psm.ps_namelen = msg->msg_namelen;
		memcpy(p, msg->msg_name, msg->msg_namelen);
		p += msg->msg_namelen;
		dl -= msg->msg_namelen;
	}

	if (msg->msg_controllen != 0) {
		if (msg->msg_controllen > dl)
			goto nobufs;
		psm.ps_controllen = (socklen_t)msg->msg_controllen;
		memcpy(p, msg->msg_control, msg->msg_controllen);
		p += msg->msg_controllen;
		dl -= msg->msg_controllen;
	}

	psm.ps_datalen = msg->msg_iov[0].iov_len;
	if (psm.ps_datalen > dl)
		goto nobufs;

	iov[1].iov_len = psm.ps_namelen + psm.ps_controllen + psm.ps_datalen;
	if (psm.ps_datalen != 0)
		memcpy(p, msg->msg_iov[0].iov_base, psm.ps_datalen);
	return writev(fd, iov, __arraycount(iov));

nobufs:
	errno = ENOBUFS;
	return -1;
}

ssize_t
ps_recvmsg(struct dhcpcd_ctx *ctx, int rfd, uint16_t cmd, int wfd)
{
	struct sockaddr_storage ss = { .ss_family = AF_UNSPEC };
	uint8_t controlbuf[sizeof(struct sockaddr_storage)] = { 0 };
	uint8_t databuf[64 * 1024];
	struct iovec iov[] = {
	    { .iov_base = databuf, .iov_len = sizeof(databuf) }
	};
	struct msghdr msg = {
		.msg_name = &ss, .msg_namelen = sizeof(ss),
		.msg_control = controlbuf, .msg_controllen = sizeof(controlbuf),
		.msg_iov = iov, .msg_iovlen = 1,
	};

	ssize_t len = recvmsg(rfd, &msg, 0);

	if (len == -1)
		logerr("%s: recvmsg", __func__);
	if (len == -1 || len == 0) {
		if (ctx->options & DHCPCD_FORKED &&
		    !(ctx->options & DHCPCD_PRIVSEPROOT))
			eloop_exit(ctx->eloop,
			    len == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
		return len;
	}

	iov[0].iov_len = (size_t)len;
	len = ps_sendcmdmsg(wfd, cmd, &msg);
	if (len == -1) {
		logerr("ps_sendcmdmsg");
		if (ctx->options & DHCPCD_FORKED &&
		    !(ctx->options & DHCPCD_PRIVSEPROOT))
			eloop_exit(ctx->eloop, EXIT_FAILURE);
	}
	return len;
}

ssize_t
ps_recvpsmsg(struct dhcpcd_ctx *ctx, int fd,
    ssize_t (*callback)(void *, struct ps_msghdr *, struct msghdr *),
    void *cbctx)
{
	struct ps_msg psm;
	ssize_t len;
	size_t dlen;
	struct iovec iov[1];
	struct msghdr msg = { .msg_iov = iov, .msg_iovlen = 1 };
	bool stop = false;

	len = read(fd, &psm, sizeof(psm));
#ifdef PRIVSEP_DEBUG
	logdebugx("%s: %zd", __func__, len);
#endif

	if (len == -1 || len == 0)
		stop = true;
	else {
		dlen = (size_t)len;
		if (dlen < sizeof(psm.psm_hdr)) {
			errno = EINVAL;
			return -1;
		}

		if (psm.psm_hdr.ps_cmd == PS_STOP) {
			stop = true;
			len = 0;
		}
	}

	if (stop) {
#ifdef PRIVSEP_DEBUG
		logdebugx("process %d stopping", getpid());
#endif
		ps_free(ctx);
#ifdef PLUGIN_DEV
		dev_stop(ctx);
#endif
		eloop_exit(ctx->eloop, len != -1 ? EXIT_SUCCESS : EXIT_FAILURE);
		return len;
	}
	dlen -= sizeof(psm.psm_hdr);

	if (ps_unrollmsg(&msg, &psm.psm_hdr, psm.psm_data, dlen) == -1)
		return -1;

	if (callback == NULL)
		return 0;

	errno = 0;
	return callback(cbctx, &psm.psm_hdr, &msg);
}

struct ps_process *
ps_findprocess(struct dhcpcd_ctx *ctx, struct ps_id *psid)
{
	struct ps_process *psp;

	TAILQ_FOREACH(psp, &ctx->ps_processes, next) {
		if (memcmp(&psp->psp_id, psid, sizeof(psp->psp_id)) == 0)
			return psp;
	}
	errno = ESRCH;
	return NULL;
}

struct ps_process *
ps_newprocess(struct dhcpcd_ctx *ctx, struct ps_id *psid)
{
	struct ps_process *psp;

	psp = calloc(1, sizeof(*psp));
	if (psp == NULL)
		return NULL;
	psp->psp_ctx = ctx;
	memcpy(&psp->psp_id, psid, sizeof(psp->psp_id));
	psp->psp_work_fd = -1;
	TAILQ_INSERT_TAIL(&ctx->ps_processes, psp, next);
	return psp;
}

void
ps_freeprocesses(struct dhcpcd_ctx *ctx, struct ps_process *notthis)
{
	struct ps_process *psp, *psn;

	TAILQ_FOREACH_SAFE(psp, &ctx->ps_processes, next, psn) {
		if (psp == notthis)
			continue;
		ps_freeprocess(psp);
	}
}
