/*	$NetBSD: os.c,v 1.1.1.1 2018/08/12 12:07:51 christos Exp $	*/

/*
 * Copyright (C) 2011 - 2015 Nominum, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND NOMINUM DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL NOMINUM BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <sys/select.h>

#include <isc/result.h>
#include <isc/types.h>

#include "log.h"
#include "os.h"
#include "util.h"

void
perf_os_blocksignal(int sig, isc_boolean_t block)
{
	sigset_t sset;
	int op;

	op = block ? SIG_BLOCK : SIG_UNBLOCK;

	if (sigemptyset(&sset) < 0 ||
	    sigaddset(&sset, sig) < 0 ||
	    pthread_sigmask(op, &sset, NULL) < 0)
		perf_log_fatal("pthread_sigmask: %s", strerror(errno));
}

void
perf_os_handlesignal(int sig, void (*handler)(int))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;

	if (sigfillset(&sa.sa_mask) < 0 ||
	    sigaction(sig, &sa, NULL) < 0)
		perf_log_fatal("sigaction: %s", strerror(errno));
}

isc_result_t
perf_os_waituntilreadable(int fd, int pipe_fd, isc_int64_t timeout)
{
	fd_set read_fds;
	int maxfd;
	struct timeval tv, *tvp;
	int n;

	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);
	FD_SET(pipe_fd, &read_fds);
	maxfd = pipe_fd > fd ? pipe_fd : fd;
	if (timeout < 0) {
		tvp = NULL;
	} else {
		tv.tv_sec = timeout / MILLION;
		tv.tv_usec = timeout % MILLION;
		tvp = &tv;
	}
	n = select(maxfd + 1, &read_fds, NULL, NULL, tvp);
	if (n < 0) {
		if (errno != EINTR)
			perf_log_fatal("select() failed");
		return (ISC_R_CANCELED);
	} else if (FD_ISSET(fd, &read_fds)) {
		return (ISC_R_SUCCESS);
	} else if (FD_ISSET(pipe_fd, &read_fds)) {
		return (ISC_R_CANCELED);
	} else {
		return (ISC_R_TIMEDOUT);
	}
}

isc_result_t
perf_os_waituntilanyreadable(int *fds, unsigned int nfds, int pipe_fd,
			     isc_int64_t timeout)
{
	fd_set read_fds;
	unsigned int i;
	int maxfd;
	struct timeval tv, *tvp;
	int n;

	FD_ZERO(&read_fds);
	maxfd = 0;
	for (i = 0; i < nfds; i++) {
		FD_SET(fds[i], &read_fds);
		if (fds[i] > maxfd)
			maxfd = fds[i];
	}
	FD_SET(pipe_fd, &read_fds);
	if (pipe_fd > maxfd)
		maxfd = pipe_fd;

	if (timeout < 0) {
		tvp = NULL;
	} else {
		tv.tv_sec = timeout / MILLION;
		tv.tv_usec = timeout % MILLION;
		tvp = &tv;
	}
	n = select(maxfd + 1, &read_fds, NULL, NULL, tvp);
	if (n < 0) {
		if (errno != EINTR)
			perf_log_fatal("select() failed");
		return (ISC_R_CANCELED);
	} else if (n == 0) {
		return (ISC_R_TIMEDOUT);
	} else if (FD_ISSET(pipe_fd, &read_fds)) {
		return (ISC_R_CANCELED);
	} else {
		return (ISC_R_SUCCESS);
	}
}
