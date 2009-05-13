/*	$NetBSD: ssl-bozo.c,v 1.4.6.1 2009/05/13 19:18:38 jym Exp $	*/

/*	$eterna: ssl-bozo.c,v 1.9 2008/11/06 05:08:11 mrg Exp $	*/

/*
 * Copyright (c) 1997-2008 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer and
 *    dedication in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* this code implements SSL for bozohttpd */

#ifndef NO_SSL_SUPPORT

#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "bozohttpd.h"

static	SSL_CTX		*ssl_context;
static	const SSL_METHOD	*ssl_method;
static	SSL		*bozossl;
static	char		*certificate_file;
static	char		*privatekey_file;

static	int	ssl_printf(const char *, ...);
static	ssize_t	ssl_read(int, void *, size_t);
static	ssize_t	ssl_write(int, const void *, size_t);
static	int	ssl_flush(FILE *);

void
ssl_init(void)
{
	if (!certificate_file)
		return;
	SSL_library_init();
	SSL_load_error_strings();

	ssl_method = SSLv23_server_method();
	ssl_context = SSL_CTX_new(ssl_method);

	/* XXX we need to learn how to check the SSL stack for more info */
	if (ssl_context == NULL)
		error(1, "SSL context initialization failed.");

	SSL_CTX_use_certificate_file(ssl_context, certificate_file,
	    SSL_FILETYPE_PEM);
	SSL_CTX_use_PrivateKey_file(ssl_context, privatekey_file,
	    SSL_FILETYPE_PEM);

	/* check consistency of key vs certificate */
	if (!SSL_CTX_check_private_key(ssl_context))
		error(1, "check private key failed");
}

void
ssl_accept()
{
	if (ssl_context) {
		bozossl = SSL_new(ssl_context); /* XXX global sucks */
		SSL_set_rfd(bozossl, 0);
		SSL_set_wfd(bozossl, 1);
		SSL_accept(bozossl);
	}
}

void
ssl_destroy()
{
	if (bozossl)
		SSL_free(bozossl);
}

void
ssl_set_opts(char *cert, char *priv)
{
	certificate_file = cert;
	privatekey_file = priv;
	debug((DEBUG_NORMAL, "using cert/priv files: %s & %s", certificate_file,
	    privatekey_file));
	if (Iflag_set == 0)
		Iflag = "https";
	bozoprintf = ssl_printf;
	bozoread = ssl_read;
	bozowrite = ssl_write;
	bozoflush = ssl_flush;
}

static int
ssl_printf(const char * fmt, ...)
{
	int nbytes;
	char *buf;
	va_list ap;

	/* XXX we need more elegant/proper handling of SSL_write return */
	va_start(ap, fmt);
	if ((nbytes = vasprintf(&buf, fmt, ap)) != -1) 
		SSL_write(bozossl, buf, nbytes);
	va_end(ap);

	return nbytes;
}

static ssize_t
ssl_read(int fd, void *buf, size_t nbytes)
{
	ssize_t rbytes;

	/* XXX we need elegant/proper handling of SSL_read return */
	rbytes = (ssize_t)SSL_read(bozossl, buf, nbytes);
	if (1 > rbytes) {
		if (SSL_get_error(bozossl, rbytes) == SSL_ERROR_WANT_READ)
			warning("SSL_ERROR_WANT_READ");
		else
			warning("SSL_ERROR OTHER");
	}

	return rbytes;
}

static ssize_t
ssl_write(int fd, const void *buf, size_t nbytes)
{
	ssize_t wbytes;

	/* XXX we need elegant/proper handling of SSL_write return */
	wbytes = (ssize_t)SSL_write(bozossl, buf, nbytes);

	return wbytes;
}

static int
ssl_flush(FILE *fp)
{
	/* nothing to see here, move right along */
	return 0;
}
#endif /* NO_SSL_SUPPORT */
