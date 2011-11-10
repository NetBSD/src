/*	$NetBSD: md5hl.c,v 1.2.6.1 2011/11/10 14:31:22 yamt Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@netbsd.org>, April 29, 1997.
 * Public domain.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define	MDALGORITHM	iSCSI_MD5

/* #include "namespace.h" */
#include "iscsi-md5.h"

#ifndef _DIAGASSERT
#define _DIAGASSERT(cond)	assert(cond)
#endif

/*	$NetBSD: md5hl.c,v 1.2.6.1 2011/11/10 14:31:22 yamt Exp $	*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dkuug.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * from FreeBSD Id: mdXhl.c,v 1.8 1996/10/25 06:48:12 bde Exp
 */

/*
 * Modifed April 29, 1997 by Jason R. Thorpe <thorpej@netbsd.org>
 */

#include <assert.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define	CONCAT(x,y)	__CONCAT(x,y)
#define	MDNAME(x)	CONCAT(MDALGORITHM,x)

char *
MDNAME(End)(MDNAME(_CTX) *ctx, char *buf)
{
	int i;
	unsigned char digest[16];
	static const char hex[]="0123456789abcdef";

	_DIAGASSERT(ctx != 0);

	if (buf == NULL)
		buf = malloc(33);
	if (buf == NULL)
		return (NULL);

	MDNAME(Final)(digest, ctx);

	for (i = 0; i < 16; i++) {
		buf[i+i] = hex[(uint32_t)digest[i] >> 4];
		buf[i+i+1] = hex[digest[i] & 0x0f];
	}

	buf[i+i] = '\0';
	return (buf);
}

char *
MDNAME(File)(filename, buf)
	const char *filename;
	char *buf;
{
	unsigned char buffer[BUFSIZ];
	MDNAME(_CTX) ctx;
	int f, j;
	ssize_t i;

	_DIAGASSERT(filename != 0);
	/* buf may be NULL */

	MDNAME(Init)(&ctx);
	f = open(filename, O_RDONLY, 0666);
	if (f < 0)
		return NULL;

	while ((i = read(f, buffer, sizeof(buffer))) > 0)
		MDNAME(Update)(&ctx, buffer, (size_t)i);

	j = errno;
	close(f);
	errno = j;

#if 0
	if (i < 0)
		return NULL;
#endif

	return (MDNAME(End)(&ctx, buf));
}

char *
MDNAME(Data)(const uint8_t *data, size_t len, char *buf)
{
	MDNAME(_CTX) ctx;

	_DIAGASSERT(data != 0);

	MDNAME(Init)(&ctx);
	MDNAME(Update)(&ctx, data, len);
	return (MDNAME(End)(&ctx, buf));
}
