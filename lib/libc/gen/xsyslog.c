/*	$NetBSD: xsyslog.c,v 1.2.4.3 2017/04/26 02:52:54 pgoyette Exp $	*/

/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)syslog.c	8.5 (Berkeley) 4/29/95";
#else
__RCSID("$NetBSD: xsyslog.c,v 1.2.4.3 2017/04/26 02:52:54 pgoyette Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/uio.h>
#include <sys/un.h>

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <paths.h>
#include "syslog_private.h"
#include "reentrant.h"
#include "extern.h"

static void
disconnectlog_r(struct syslog_data *data)
{
	/*
	 * If the user closed the FD and opened another in the same slot,
	 * that's their problem.  They should close it before calling on
	 * system services.
	 */
	if (data->log_file != -1) {
		(void)close(data->log_file);
		data->log_file = -1;
	}
	data->log_connected = 0;		/* retry connect */
}

static void
connectlog_r(struct syslog_data *data)
{
	/* AF_UNIX address of local logger */
	static const struct sockaddr_un sun = {
		.sun_family = AF_LOCAL,
		.sun_len = sizeof(sun),
		.sun_path = _PATH_LOG,
	};

	if (data->log_file == -1 || fcntl(data->log_file, F_GETFL, 0) == -1) {
		if ((data->log_file = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC,
		    0)) == -1)
			return;
		data->log_connected = 0;
	}
	if (!data->log_connected) {
		if (connect(data->log_file,
		    (const struct sockaddr *)(const void *)&sun,
		    (socklen_t)sizeof(sun)) == -1) {
			(void)close(data->log_file);
			data->log_file = -1;
		} else
			data->log_connected = 1;
	}
}

void
_openlog_unlocked_r(const char *ident, int logstat, int logfac,
    struct syslog_data *data)
{
	if (ident != NULL)
		data->log_tag = ident;
	data->log_stat = logstat;
	if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0)
		data->log_fac = logfac;

	if (data->log_stat & LOG_NDELAY)	/* open immediately */
		connectlog_r(data);

	data->log_opened = 1;
}

void
_closelog_unlocked_r(struct syslog_data *data)
{
	(void)close(data->log_file);
	data->log_file = -1;
	data->log_connected = 0;
}

static void
_xsyslogp_r(int pri, struct syslog_fun *fun,
    struct syslog_data *data, const char *msgid,
    const char *sdfmt, const char *msgfmt, ...)
{
	va_list ap;
	va_start(ap, msgfmt);
	_vxsyslogp_r(pri, fun, data, msgid, sdfmt, msgfmt, ap);
	va_end(ap);
}

void
_vxsyslogp_r(int pri, struct syslog_fun *fun,
    struct syslog_data *data, const char *msgid,
    const char *sdfmt, const char *msgfmt, va_list ap)
{
	static const char BRCOSP[] = "]: ";
	static const char CRLF[] = "\r\n";
	size_t cnt, prlen, tries;
	char ch, *p, *t;
	int fd, saved_errno;
#define TBUF_LEN	2048
#define FMT_LEN		1024
#define MAXTRIES	10
	char tbuf[TBUF_LEN], fmt_cpy[FMT_LEN], fmt_cat[FMT_LEN] = "";
	size_t tbuf_left, fmt_left, msgsdlen;
	char *fmt = fmt_cat;
	struct iovec iov[7];	/* prog + [ + pid + ]: + fmt + crlf */
	int opened, iovcnt;

	iovcnt = opened = 0;

#define INTERNALLOG	LOG_ERR|LOG_CONS|LOG_PERROR|LOG_PID
	/* Check for invalid bits. */
	if (pri & ~(LOG_PRIMASK|LOG_FACMASK)) {
		_xsyslogp_r(INTERNALLOG, &_syslog_ss_fun, data, NULL, NULL,
		    "%s: unknown facility/priority: %x", pri);
		pri &= LOG_PRIMASK|LOG_FACMASK;
	}

	/* Check priority against setlogmask values. */
	if (!(LOG_MASK(LOG_PRI(pri)) & data->log_mask))
		return;

	saved_errno = errno;

	/* Set default facility if none specified. */
	if ((pri & LOG_FACMASK) == 0)
		pri |= data->log_fac;

	/* Build the message. */
	p = tbuf;
	tbuf_left = TBUF_LEN;

	prlen = snprintf_ss(p, tbuf_left, "<%d>1 ", pri);
	DEC();

	prlen = (*fun->timefun)(p, tbuf_left);

	(*fun->lock)(data);

	if (data->log_hostname[0] == '\0' && gethostname(data->log_hostname,
	    sizeof(data->log_hostname)) == -1) {
		/* can this really happen? */
		data->log_hostname[0] = '-';
		data->log_hostname[1] = '\0';
	}

	DEC();
	prlen = snprintf_ss(p, tbuf_left, " %s ", data->log_hostname);

	if (data->log_tag == NULL)
		data->log_tag = getprogname();

	DEC();
	prlen = snprintf_ss(p, tbuf_left, "%s ",
	    data->log_tag ? data->log_tag : "-");

	(*fun->unlock)(data);

	if (data->log_stat & (LOG_PERROR|LOG_CONS)) {
		iov[iovcnt].iov_base = p;
		iov[iovcnt].iov_len = prlen - 1;
		iovcnt++;
	}
	DEC();

	if (data->log_stat & LOG_PID) {
		prlen = snprintf_ss(p, tbuf_left, "%d ", getpid());
		if (data->log_stat & (LOG_PERROR|LOG_CONS)) {
			iov[iovcnt].iov_base = __UNCONST("[");
			iov[iovcnt].iov_len = 1;
			iovcnt++;
			iov[iovcnt].iov_base = p;
			iov[iovcnt].iov_len = prlen - 1;
			iovcnt++;
			iov[iovcnt].iov_base = __UNCONST(BRCOSP);
			iov[iovcnt].iov_len = 3;
			iovcnt++;
		}
	} else {
		prlen = snprintf_ss(p, tbuf_left, "- ");
		if (data->log_stat & (LOG_PERROR|LOG_CONS)) {
			iov[iovcnt].iov_base = __UNCONST(BRCOSP + 1);
			iov[iovcnt].iov_len = 2;
			iovcnt++;
		}
	}
	DEC();

	/*
	 * concat the format strings, then use one vsnprintf()
	 */
	if (msgid != NULL && *msgid != '\0') {
		strlcat(fmt_cat, msgid, FMT_LEN);
		strlcat(fmt_cat, " ", FMT_LEN);
	} else
		strlcat(fmt_cat, "- ", FMT_LEN);

	if (sdfmt != NULL && *sdfmt != '\0') {
		strlcat(fmt_cat, sdfmt, FMT_LEN);
	} else
		strlcat(fmt_cat, "-", FMT_LEN);

	if (data->log_stat & (LOG_PERROR|LOG_CONS))
		msgsdlen = strlen(fmt_cat) + 1;
	else
		msgsdlen = 0;	/* XXX: GCC */

	if (msgfmt != NULL && *msgfmt != '\0') {
		strlcat(fmt_cat, " ", FMT_LEN);
		strlcat(fmt_cat, msgfmt, FMT_LEN);
	}

	/*
	 * We wouldn't need this mess if printf handled %m, or if
	 * strerror() had been invented before syslog().
	 */
	for (t = fmt_cpy, fmt_left = FMT_LEN; (ch = *fmt) != '\0'; ++fmt) {
		if (ch == '%' && fmt[1] == 'm') {
			char buf[256];

			if ((*fun->errfun)(saved_errno, buf, sizeof(buf)) != 0)
				prlen = snprintf_ss(t, fmt_left, "Error %d",
				    saved_errno);
			else
				prlen = strlcpy(t, buf, fmt_left);
			if (prlen >= fmt_left)
				prlen = fmt_left - 1;
			t += prlen;
			fmt++;
			fmt_left -= prlen;
		} else if (ch == '%' && fmt[1] == '%' && fmt_left > 2) {
			*t++ = '%';
			*t++ = '%';
			fmt++;
			fmt_left -= 2;
		} else {
			if (fmt_left > 1) {
				*t++ = ch;
				fmt_left--;
			}
		}
	}
	*t = '\0';

	prlen = (*fun->prfun)(p, tbuf_left, fmt_cpy, ap);
	if (data->log_stat & (LOG_PERROR|LOG_CONS)) {
		iov[iovcnt].iov_base = p + msgsdlen;
		iov[iovcnt].iov_len = prlen - msgsdlen;
		iovcnt++;
	}

	DEC();
	cnt = p - tbuf;

	/* Output to stderr if requested. */
	if (data->log_stat & LOG_PERROR) {
		struct iovec *piov;
		int piovcnt;

		iov[iovcnt].iov_base = __UNCONST(CRLF + 1);
		iov[iovcnt].iov_len = 1;
		if (data->log_stat & LOG_PTRIM) {
			piov = &iov[iovcnt - 1];
			piovcnt = 2;
		} else {
			piov = iov;
			piovcnt = iovcnt + 1;
		}
		(void)writev(STDERR_FILENO, piov, piovcnt);
	}

	if (data->log_stat & LOG_NLOG)
		goto out;

	/* Get connected, output the message to the local logger. */
	(*fun->lock)(data);
	opened = !data->log_opened;
	if (opened)
		_openlog_unlocked_r(data->log_tag, data->log_stat, 0, data);
	connectlog_r(data);

	/*
	 * If the send() failed, there are two likely scenarios:
	 *  1) syslogd was restarted
	 *  2) /dev/log is out of socket buffer space
	 * We attempt to reconnect to /dev/log to take care of
	 * case #1 and keep send()ing data to cover case #2
	 * to give syslogd a chance to empty its socket buffer.
	 */
	for (tries = 0; tries < MAXTRIES; tries++) {
		if (send(data->log_file, tbuf, cnt, 0) != -1)
			break;
		if (errno != ENOBUFS) {
			disconnectlog_r(data);
			connectlog_r(data);
		} else
			(void)usleep(1);
	}

	/*
	 * Output the message to the console; try not to block
	 * as a blocking console should not stop other processes.
	 * Make sure the error reported is the one from the syslogd failure.
	 */
	if (tries == MAXTRIES && (data->log_stat & LOG_CONS) &&
	    (fd = open(_PATH_CONSOLE,
		O_WRONLY | O_NONBLOCK | O_CLOEXEC, 0)) >= 0) {
		iov[iovcnt].iov_base = __UNCONST(CRLF);
		iov[iovcnt].iov_len = 2;
		(void)writev(fd, iov, iovcnt + 1);
		(void)close(fd);
	}

out:
	if (!(*fun->unlock)(data) && opened)
		_closelog_unlocked_r(data);
}

int
setlogmask_r(int pmask, struct syslog_data *data)
{
	int omask;

	omask = data->log_mask;
	if (pmask != 0)
		data->log_mask = pmask;
	return omask;
}
