/*	$NetBSD: ssl.c,v 1.8 2019/04/07 00:44:54 christos Exp $	*/

/*-
 * Copyright (c) 1998-2004 Dag-Erling Coïdan Smørgrav
 * Copyright (c) 2008, 2010 Joerg Sonnenberger <joerg@NetBSD.org>
 * Copyright (c) 2015 Thomas Klausner <wiz@NetBSD.org>
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
__RCSID("$NetBSD: ssl.c,v 1.8 2019/04/07 00:44:54 christos Exp $");
#endif

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/param.h>
#include <sys/select.h>
#include <sys/uio.h>

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "ssl.h"

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
	SSL			*ssl;		/* SSL handle */
};

/*
 * Write a vector to a connection w/ timeout
 * Note: can modify the iovec.
 */
static ssize_t
fetch_writev(struct fetch_connect *conn, struct iovec *iov, int iovcnt)
{
	struct timeval now, timeout, delta;
	fd_set writefds;
	ssize_t len, total;
	int fd = conn->sd;
	int r;

	if (quit_time > 0) {
		FD_ZERO(&writefds);
		gettimeofday(&timeout, NULL);
		timeout.tv_sec += quit_time;
	}

	total = 0;
	while (iovcnt > 0) {
		while (quit_time > 0 && !FD_ISSET(fd, &writefds)) {
			FD_SET(fd, &writefds);
			gettimeofday(&now, NULL);
			delta.tv_sec = timeout.tv_sec - now.tv_sec;
			delta.tv_usec = timeout.tv_usec - now.tv_usec;
			if (delta.tv_usec < 0) {
				delta.tv_usec += 1000000;
				delta.tv_sec--;
			}
			if (delta.tv_sec < 0) {
				errno = ETIMEDOUT;
				return -1;
			}
			errno = 0;
			r = select(fd + 1, NULL, &writefds, NULL, &delta);
			if (r == -1) {
				if (errno == EINTR)
					continue;
				return -1;
			}
		}
		errno = 0;
		if (conn->ssl != NULL)
			len = SSL_write(conn->ssl, iov->iov_base, iov->iov_len);
		else
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
	int r;

	va_start(ap, fmt);
	len = vasprintf(&msg, fmt, ap);
	va_end(ap);

	if (msg == NULL) {
		errno = ENOMEM;
		return -1;
	}

	r = fetch_write(msg, len, conn);
	free(msg);
	return r;
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
	SSL_free(conn->ssl);
	close(conn->sd);
	free(conn->cache.buf);
	free(conn->buf);
	free(conn);
	return 0;
}

#define FETCH_WRITE_WAIT	-3
#define FETCH_READ_WAIT		-2
#define FETCH_READ_ERROR	-1

static ssize_t
fetch_ssl_read(SSL *ssl, void *buf, size_t len)
{
	ssize_t rlen;
	rlen = SSL_read(ssl, buf, len);
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

static ssize_t
fetch_nonssl_read(int sd, void *buf, size_t len)
{
	ssize_t rlen;

	rlen = read(sd, buf, len);
	if (rlen == -1) {
		if (errno == EAGAIN || errno == EINTR)
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
	fd_set fds;

	FD_ZERO(&fds);
	while (!FD_ISSET(fd, &fds)) {
		FD_SET(fd, &fds);
		if (quit_time > 0) {
			gettimeofday(&now, NULL);
			if (!timercmp(timeout, &now, >)) {
				conn->iserr = ETIMEDOUT;
				return -1;
			}
			timersub(timeout, &now, &delta);
		}
		errno = 0;
		if (select(fd + 1,
			rlen == FETCH_READ_WAIT ? &fds : NULL,
			rlen == FETCH_WRITE_WAIT ? &fds : NULL,
			NULL, quit_time > 0 ? &delta : NULL) < 0) {
			if (errno == EINTR)
				continue;
			conn->iserr = errno;
			return -1;
		}
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
		 * select() -> read(), we do the following:
		 *
		 * 1) call read() or SSL_read().
		 * 2) if an error occurred, return -1.
		 * 3) if we received data but we still expect more,
		 *    update our counters and loop.
		 * 4) if read() or SSL_read() signaled EOF, return.
		 * 5) if we did not receive any data but we're not at EOF,
		 *    call select().
		 *
		 * In the SSL case, this is necessary because if we
		 * receive a close notification, we have to call
		 * SSL_read() one additional time after we've read
		 * everything we received.
		 *
		 * In the non-SSL case, it may improve performance (very
		 * slightly) when reading small amounts of data.
		 */
		if (conn->ssl != NULL)
			rlen = fetch_ssl_read(conn->ssl, buf, len);
		else
			rlen = fetch_nonssl_read(conn->sd, buf, len);
		switch (rlen) {
		case 0:
			conn->iseof = 1;
			return total;
		case FETCH_READ_ERROR:
			conn->iserr = errno;
			if (errno == EINTR)
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

	if (fetch_getln(buf, buflen, conn) == NULL) {
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
		while (1) {
			char c;
			size_t rlen = fetch_read(&c, sizeof(c), 1, conn);
			if (rlen == 0 || c == '\n')
				break;
		}
		if (errormsg)
			*errormsg = "Input line is too long";
		fetch_clearerr(conn);
		return -3;
	}
	if (errormsg)
		*errormsg = NULL;
	return len;
}

void *
fetch_start_ssl(int sock, const char *servername)
{
	SSL *ssl;
	SSL_CTX *ctx;
	int ret, ssl_err;

	/* Init the SSL library and context */
	if (!SSL_library_init()){
		fprintf(ttyout, "SSL library init failed\n");
		return NULL;
	}

	SSL_load_error_strings();

	ctx = SSL_CTX_new(SSLv23_client_method());
	SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

	ssl = SSL_new(ctx);
	if (ssl == NULL){
		fprintf(ttyout, "SSL context creation failed\n");
		SSL_CTX_free(ctx);
		return NULL;
	}
	SSL_set_fd(ssl, sock);
	if (!SSL_set_tlsext_host_name(ssl, __UNCONST(servername))) {
		fprintf(ttyout, "SSL hostname setting failed\n");
		SSL_CTX_free(ctx);
		return NULL;
	}
	while ((ret = SSL_connect(ssl)) == -1) {
		ssl_err = SSL_get_error(ssl, ret);
		if (ssl_err != SSL_ERROR_WANT_READ &&
		    ssl_err != SSL_ERROR_WANT_WRITE) {
			ERR_print_errors_fp(ttyout);
			SSL_free(ssl);
			return NULL;
		}
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
}


void
fetch_set_ssl(struct fetch_connect *conn, void *ssl)
{
	conn->ssl = ssl;
}
