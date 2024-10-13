/*	$NetBSD: ssl.c,v 1.12.2.5 2024/10/13 16:06:36 martin Exp $	*/

/*-
 * Copyright (c) 1998-2004 Dag-Erling Coïdan Smørgrav
 * Copyright (c) 2008, 2010 Joerg Sonnenberger <joerg@NetBSD.org>
 * Copyright (c) 2015 Thomas Klausner <wiz@NetBSD.org>
 * Copyright (c) 2023 Michael van Elst <mlelstv@NetBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: common.c,v 1.53 2007/12/19 00:26:36 des Exp $
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ssl.c,v 1.12.2.5 2024/10/13 16:06:36 martin Exp $");
#endif

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/uio.h>

#include <netinet/tcp.h>
#include <netinet/in.h>

#ifdef WITH_SSL
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#include "ssl.h"
#include "ftp_var.h"

extern int quit_time, verbose, ftp_debug;
extern FILE *ttyout;

struct fetch_connect {
	int			 sd;		/* file/socket descriptor */
	char			*buf;		/* buffer */
	size_t			 bufsize;	/* buffer size */
	size_t			 bufpos;	/* position of buffer */
	size_t			 buflen;	/* length of buffer contents */
	struct {				/* data cached after an
						   interrupted read */
		char	*buf;
		size_t	 size;
		size_t	 pos;
		size_t	 len;
	} cache;
	int 			 issock;
	int			 iserr;
	int			 iseof;
#ifdef WITH_SSL
	SSL			*ssl;		/* SSL handle */
#endif
};

/*
 * Write a vector to a connection w/ timeout
 * Note: can modify the iovec.
 */
static ssize_t
fetch_writev(struct fetch_connect *conn, struct iovec *iov, int iovcnt)
{
	struct timeval timeout, now, delta;
	ssize_t len, total;
	int fd = conn->sd;
	int rv, timeout_secs;
	struct pollfd pfd[1];

	pfd[0].fd = fd;
	pfd[0].events = POLLOUT;
	gettimeofday(&timeout, NULL);
	timeout.tv_sec += quit_time;

	total = 0;
	while (iovcnt > 0) {
		if (quit_time > 0) {	/* enforce timeout */
			do {
				(void)gettimeofday(&now, NULL);
				timersub(&timeout, &now, &delta);
				timeout_secs = (int)(delta.tv_sec * 1000
				    + delta.tv_usec / 1000);
				if (timeout_secs < 0)
					timeout_secs = 0;
				rv = ftp_poll(pfd, 1, timeout_secs);
					/* loop until poll !EINTR && !EAGAIN */
			} while (rv == -1 && (errno == EINTR || errno == EAGAIN));
			if (rv == -1)
				return -1;
			if (rv == 0) {
				errno = ETIMEDOUT;
				return -1;
			}
		}
		errno = 0;
#ifdef WITH_SSL
		if (conn->ssl != NULL)
			len = SSL_write(conn->ssl, iov->iov_base, (int)iov->iov_len);
		else
#endif
			len = writev(fd, iov, iovcnt);
		if (len == 0) {
			/* we consider a short write a failure */
			/* XXX perhaps we shouldn't in the SSL case */
			errno = EPIPE;
			return -1;
		}
		if (len < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return -1;
		}
		total += len;
		while (iovcnt > 0 && len >= (ssize_t)iov->iov_len) {
			len -= iov->iov_len;
			iov++;
			iovcnt--;
		}
		if (iovcnt > 0) {
			iov->iov_len -= len;
			iov->iov_base = (char *)iov->iov_base + len;
		}
	}
	return total;
}

static ssize_t
fetch_write(const void *str, size_t len, struct fetch_connect *conn)
{
	struct iovec iov[1];

	iov[0].iov_base = (char *)__UNCONST(str);
	iov[0].iov_len = len;
	return fetch_writev(conn, iov, 1);
}

/*
 * Send a formatted line; optionally echo to terminal
 */
int
fetch_printf(struct fetch_connect *conn, const char *fmt, ...)
{
	va_list ap;
	size_t len;
	char *msg;
	ssize_t r;

	va_start(ap, fmt);
	len = vasprintf(&msg, fmt, ap);
	va_end(ap);

	if (msg == NULL) {
		errno = ENOMEM;
		return -1;
	}

	r = fetch_write(msg, len, conn);
	free(msg);
	return (int)r;
}

int
fetch_fileno(struct fetch_connect *conn)
{

	return conn->sd;
}

int
fetch_error(struct fetch_connect *conn)
{

	return conn->iserr;
}

static void
fetch_clearerr(struct fetch_connect *conn)
{

	conn->iserr = 0;
}

int
fetch_flush(struct fetch_connect *conn)
{

	if (conn->issock) {
		int fd = conn->sd;
		int v;
#ifdef TCP_NOPUSH
		v = 0;
		setsockopt(fd, IPPROTO_TCP, TCP_NOPUSH, &v, sizeof(v));
#endif
		v = 1;
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));
	}
	return 0;
}

/*ARGSUSED*/
struct fetch_connect *
fetch_open(const char *fname, const char *fmode)
{
	struct fetch_connect *conn;
	int fd;

	fd = open(fname, O_RDONLY); /* XXX: fmode */
	if (fd < 0)
		return NULL;

	if ((conn = calloc(1, sizeof(*conn))) == NULL) {
		close(fd);
		return NULL;
	}

	conn->sd = fd;
	conn->issock = 0;
	return conn;
}

/*ARGSUSED*/
struct fetch_connect *
fetch_fdopen(int sd, const char *fmode)
{
	struct fetch_connect *conn;
#if defined(SO_NOSIGPIPE) || defined(TCP_NOPUSH)
	int opt = 1;
#endif

	if ((conn = calloc(1, sizeof(*conn))) == NULL)
		return NULL;

	conn->sd = sd;
	conn->issock = 1;
	fcntl(sd, F_SETFD, FD_CLOEXEC);
#ifdef SO_NOSIGPIPE
	setsockopt(sd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#endif
#ifdef TCP_NOPUSH
	setsockopt(sd, IPPROTO_TCP, TCP_NOPUSH, &opt, sizeof(opt));
#endif
	return conn;
}

int
fetch_close(struct fetch_connect *conn)
{
	if (conn == NULL)
		return 0;

	fetch_flush(conn);
#ifdef WITH_SSL
	SSL_free(conn->ssl);
#endif
	close(conn->sd);
	free(conn->cache.buf);
	free(conn->buf);
	free(conn);
	return 0;
}

#define FETCH_WRITE_WAIT	-3
#define FETCH_READ_WAIT		-2
#define FETCH_READ_ERROR	-1

#ifdef WITH_SSL
static ssize_t
fetch_ssl_read(SSL *ssl, void *buf, size_t len)
{
	int rlen;
	rlen = SSL_read(ssl, buf, (int)len);
	if (rlen >= 0)
		return rlen;

	switch (SSL_get_error(ssl, rlen)) {
	case SSL_ERROR_WANT_READ:
		return FETCH_READ_WAIT;
	case SSL_ERROR_WANT_WRITE:
		return FETCH_WRITE_WAIT;
	default:
		ERR_print_errors_fp(ttyout);
		return FETCH_READ_ERROR;
	}
}
#endif /* WITH_SSL */

static ssize_t
fetch_nonssl_read(int sd, void *buf, size_t len)
{
	ssize_t rlen;

	rlen = read(sd, buf, len);
	if (rlen == -1) {
		if (errno == EINTR || errno == EAGAIN)
			return FETCH_READ_WAIT;
		return FETCH_READ_ERROR;
	}
	return rlen;
}

/*
 * Cache some data that was read from a socket but cannot be immediately
 * returned because of an interrupted system call.
 */
static int
fetch_cache_data(struct fetch_connect *conn, char *src, size_t nbytes)
{

	if (conn->cache.size < nbytes) {
		char *tmp = realloc(conn->cache.buf, nbytes);
		if (tmp == NULL)
			return -1;

		conn->cache.buf = tmp;
		conn->cache.size = nbytes;
	}

	memcpy(conn->cache.buf, src, nbytes);
	conn->cache.len = nbytes;
	conn->cache.pos = 0;
	return 0;
}

static int
fetch_wait(struct fetch_connect *conn, ssize_t rlen, struct timeval *timeout)
{
	struct timeval now, delta;
	int fd = conn->sd;
	int rv, timeout_secs;
	struct pollfd pfd[1];

	pfd[0].fd = fd;
	if (rlen == FETCH_READ_WAIT) {
		pfd[0].events = POLLIN;
	} else if (rlen == FETCH_WRITE_WAIT) {
		pfd[0].events = POLLOUT;
	} else {
		pfd[0].events = 0;
	}

	do {
		if (quit_time > 0) {
			gettimeofday(&now, NULL);
			timersub(timeout, &now, &delta);
			timeout_secs = (int)(delta.tv_sec * 1000
			    + delta.tv_usec / 1000);
			if (timeout_secs < 0)
				timeout_secs = 0;
		} else {
			timeout_secs = INFTIM;
		}
		errno = 0;
		rv = ftp_poll(pfd, 1, timeout_secs);
				/* loop until poll !EINTR && !EAGAIN */
	} while (rv == -1 && (errno == EINTR || errno == EAGAIN));
	if (rv == 0) {		/* poll timeout */
		fprintf(ttyout, "\r\n%s: transfer aborted"
		    " because stalled for %lu sec.\r\n",
		    getprogname(), (unsigned long)quit_time);
		errno = ETIMEDOUT;
		conn->iserr = ETIMEDOUT;
		return -1;
	}
	if (rv == -1) {		/* poll error */
		conn->iserr = errno;
		return -1;
	}
	return 0;
}

size_t
fetch_read(void *ptr, size_t size, size_t nmemb, struct fetch_connect *conn)
{
	ssize_t rlen, total;
	size_t len;
	char *start, *buf;
	struct timeval timeout;

	if (quit_time > 0) {
		gettimeofday(&timeout, NULL);
		timeout.tv_sec += quit_time;
	}

	total = 0;
	start = buf = ptr;
	len = size * nmemb;

	if (conn->cache.len > 0) {
		/*
		 * The last invocation of fetch_read was interrupted by a
		 * signal after some data had been read from the socket. Copy
		 * the cached data into the supplied buffer before trying to
		 * read from the socket again.
		 */
		total = (conn->cache.len < len) ? conn->cache.len : len;
		memcpy(buf, conn->cache.buf, total);

		conn->cache.len -= total;
		conn->cache.pos += total;
		len -= total;
		buf += total;
	}

	while (len > 0) {
		/*
		 * The socket is non-blocking.  Instead of the canonical
		 * poll() -> read(), we do the following:
		 *
		 * 1) call read() or SSL_read().
		 * 2) if an error occurred, return -1.
		 * 3) if we received data but we still expect more,
		 *    update our counters and loop.
		 * 4) if read() or SSL_read() signaled EOF, return.
		 * 5) if we did not receive any data but we're not at EOF,
		 *    call poll().
		 *
		 * In the SSL case, this is necessary because if we
		 * receive a close notification, we have to call
		 * SSL_read() one additional time after we've read
		 * everything we received.
		 *
		 * In the non-SSL case, it may improve performance (very
		 * slightly) when reading small amounts of data.
		 */
#ifdef WITH_SSL
		if (conn->ssl != NULL)
			rlen = fetch_ssl_read(conn->ssl, buf, len);
		else
#endif
			rlen = fetch_nonssl_read(conn->sd, buf, len);
		switch (rlen) {
		case 0:
			conn->iseof = 1;
			return total;
		case FETCH_READ_ERROR:
			conn->iserr = errno;
			if (errno == EINTR || errno == EAGAIN)
				fetch_cache_data(conn, start, total);
			return 0;
		case FETCH_READ_WAIT:
		case FETCH_WRITE_WAIT:
			if (fetch_wait(conn, rlen, &timeout) == -1)
				return 0;
			break;
		default:
			len -= rlen;
			buf += rlen;
			total += rlen;
			break;
		}
	}
	return total;
}

#define MIN_BUF_SIZE 1024

/*
 * Read a line of text from a connection w/ timeout
 */
char *
fetch_getln(char *str, int size, struct fetch_connect *conn)
{
	size_t tmpsize;
	size_t len;
	char c;

	if (conn->buf == NULL) {
		if ((conn->buf = malloc(MIN_BUF_SIZE)) == NULL) {
			errno = ENOMEM;
			conn->iserr = 1;
			return NULL;
		}
		conn->bufsize = MIN_BUF_SIZE;
	}

	if (conn->iserr || conn->iseof)
		return NULL;

	if (conn->buflen - conn->bufpos > 0)
		goto done;

	conn->buf[0] = '\0';
	conn->bufpos = 0;
	conn->buflen = 0;
	do {
		len = fetch_read(&c, sizeof(c), 1, conn);
		if (len == 0) {
			if (conn->iserr)
				return NULL;
			if (conn->iseof)
				break;
			abort();
		}
		conn->buf[conn->buflen++] = c;
		if (conn->buflen == conn->bufsize) {
			char *tmp = conn->buf;
			tmpsize = conn->bufsize * 2 + 1;
			if ((tmp = realloc(tmp, tmpsize)) == NULL) {
				errno = ENOMEM;
				conn->iserr = 1;
				return NULL;
			}
			conn->buf = tmp;
			conn->bufsize = tmpsize;
		}
	} while (c != '\n');

	if (conn->buflen == 0)
		return NULL;
 done:
	tmpsize = MIN(size - 1, (int)(conn->buflen - conn->bufpos));
	memcpy(str, conn->buf + conn->bufpos, tmpsize);
	str[tmpsize] = '\0';
	conn->bufpos += tmpsize;
	return str;
}

int
fetch_getline(struct fetch_connect *conn, char *buf, size_t buflen,
    const char **errormsg)
{
	size_t len;
	int rv;

	if (fetch_getln(buf, (int)buflen, conn) == NULL) {
		if (conn->iseof) {	/* EOF */
			rv = -2;
			if (errormsg)
				*errormsg = "\nEOF received";
		} else {		/* error */
			rv = -1;
			if (errormsg)
				*errormsg = "Error encountered";
		}
		fetch_clearerr(conn);
		return rv;
	}
	len = strlen(buf);
	if (buf[len - 1] == '\n') {	/* clear any trailing newline */
		buf[--len] = '\0';
	} else if (len == buflen - 1) {	/* line too long */
		for (;;) {
			char c;
			size_t rlen = fetch_read(&c, sizeof(c), 1, conn);
			if (rlen == 0 || c == '\n')
				break;
		}
		if (errormsg)
			*errormsg = "Input line is too long (specify -b > 16384)";
		fetch_clearerr(conn);
		return -3;
	}
	if (errormsg)
		*errormsg = NULL;
	return (int)len;
}

#ifdef WITH_SSL
/*
 * Start the SSL/TLS negotiation.
 * Socket fcntl flags are temporarily updated to include O_NONBLOCK;
 * these will not be reverted on connection failure.
 * Returns pointer to allocated SSL structure on success,
 * or NULL upon failure.
 */
void *
fetch_start_ssl(int sock, const char *servername)
{
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;
	X509_VERIFY_PARAM *param;
	int ret, ssl_err, flags, rv, timeout_secs;
	int verify = !ftp_truthy("sslnoverify", getoptionvalue("sslnoverify"), 0);
	struct timeval timeout, now, delta;
	struct pollfd pfd[1];

	/* Init the SSL library and context */
	if (!SSL_library_init()){
		warnx("SSL library init failed");
		goto cleanup_start_ssl;
	}

	SSL_load_error_strings();

	ctx = SSL_CTX_new(SSLv23_client_method());
	SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
	if (verify) {
		SSL_CTX_set_default_verify_paths(ctx);
		SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	}

	ssl = SSL_new(ctx);
	if (ssl == NULL){
		warnx("SSL context creation failed");
		goto cleanup_start_ssl;
	}

	if (verify) {
		param = SSL_get0_param(ssl);
		if (!X509_VERIFY_PARAM_set1_host(param, servername,
		    strlen(servername))) {
			warnx("SSL verification setup failed");
			goto cleanup_start_ssl;
		}

		/* Enable peer verification, (using the default callback) */
		SSL_set_verify(ssl, SSL_VERIFY_PEER, NULL);
	}
#ifdef SSL_OP_IGNORE_UNEXPECTED_EOF
	SSL_set_options(ssl, SSL_OP_IGNORE_UNEXPECTED_EOF);
#endif

						/* save current socket flags */
	if ((flags = fcntl(sock, F_GETFL, 0)) == -1) {
		warn("Can't %s socket flags for SSL connect to `%s'",
		    "save", servername);
		goto cleanup_start_ssl;
	}
						/* set non-blocking connect */
	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
		warn("Can't set socket non-blocking for SSL connect to `%s'",
		    servername);
		goto cleanup_start_ssl;
	}

	/* NOTE: we now must restore socket flags on successful connection */

	(void)gettimeofday(&timeout, NULL);	/* setup SSL_connect() timeout */
	timeout.tv_sec += (quit_time > 0) ? quit_time: 60;
						/* without -q, default to 60s */

	SSL_set_fd(ssl, sock);
	if (!SSL_set_tlsext_host_name(ssl, __UNCONST(servername))) {
		warnx("SSL hostname setting failed");
		goto cleanup_start_ssl;
	}
	pfd[0].fd = sock;
	pfd[0].events = 0;
	while ((ret = SSL_connect(ssl)) <= 0) {
		ssl_err = SSL_get_error(ssl, ret);
		DPRINTF("%s: SSL_connect() ret=%d ssl_err=%d\n",
		    __func__, ret, ssl_err);
		if (ret == 0) { /* unsuccessful handshake */
			ERR_print_errors_fp(ttyout);
			goto cleanup_start_ssl;
		}
		if (ssl_err == SSL_ERROR_WANT_READ) {
			pfd[0].events = POLLIN;
		} else if (ssl_err == SSL_ERROR_WANT_WRITE) {
			pfd[0].events = POLLOUT;
		} else {
			ERR_print_errors_fp(ttyout);
			goto cleanup_start_ssl;
		}
		(void)gettimeofday(&now, NULL);
		timersub(&timeout, &now, &delta);
		timeout_secs = (int)(delta.tv_sec * 1000
		    + delta.tv_usec / 1000);
		if (timeout_secs < 0)
			timeout_secs = 0;
		rv = ftp_poll(pfd, 1, timeout_secs);
		if (rv == 0) {		/* poll for SSL_connect() timed out */
			fprintf(ttyout, "Timeout establishing SSL connection to `%s'\n",
			    servername);
			goto cleanup_start_ssl;
		} else if (rv == -1 && errno != EINTR && errno != EAGAIN) {
			warn("Error polling for SSL connect to `%s'", servername);
			goto cleanup_start_ssl;
		}
	}

	if (fcntl(sock, F_SETFL, flags) == -1) {
						/* restore socket flags */
		warn("Can't %s socket flags for SSL connect to `%s'",
		    "restore", servername);
		goto cleanup_start_ssl;
	}

	if (ftp_debug && verbose) {
		X509 *cert;
		X509_NAME *name;
		char *str;

		fprintf(ttyout, "SSL connection established using %s\n",
		    SSL_get_cipher(ssl));
		cert = SSL_get_peer_certificate(ssl);
		name = X509_get_subject_name(cert);
		str = X509_NAME_oneline(name, 0, 0);
		fprintf(ttyout, "Certificate subject: %s\n", str);
		free(str);
		name = X509_get_issuer_name(cert);
		str = X509_NAME_oneline(name, 0, 0);
		fprintf(ttyout, "Certificate issuer: %s\n", str);
		free(str);
	}

	return ssl;

 cleanup_start_ssl:
	if (ssl)
		SSL_free(ssl);
	if (ctx)
		SSL_CTX_free(ctx);
	return NULL;
}
#endif /* WITH_SSL */


void
fetch_set_ssl(struct fetch_connect *conn, void *ssl)
{
#ifdef WITH_SSL
	conn->ssl = ssl;
#endif
}
