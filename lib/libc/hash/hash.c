/* $NetBSD */

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dkuug.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 */

/*
 * Modified September 24, 2005 by Elad Efrat <elad@NetBSD.org>
 * Modified April 29, 1997 by Jason R. Thorpe <thorpej@NetBSD.org>
 */

#include "namespace.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#ifndef MIN
#define	MIN(x,y)	((x)<(y)?(x):(y))
#endif /* !MIN */

#define	CONCAT(x,y)	__CONCAT(x,y)

#ifndef HASH_FNPREFIX
#define	HASH_FNPREFIX	HASH_ALGORITHM
#endif /* !HASH_FNPREFIX */

#define	FNPREFIX(x)	CONCAT(HASH_FNPREFIX,x)
#define	HASH_CTX	CONCAT(HASH_ALGORITHM,_CTX)
#define	HASH_LEN	CONCAT(HASH_ALGORITHM,_DIGEST_LENGTH)
#define	HASH_STRLEN	CONCAT(HASH_ALGORITHM,_DIGEST_STRING_LENGTH)

char *
FNPREFIX(End)(HASH_CTX *ctx, char *buf)
{
	int i;
	unsigned char digest[HASH_LEN];
	static const char hex[]="0123456789abcdef";

	_DIAGASSERT(ctx != 0);

	if (buf == NULL)
		buf = malloc((size_t)HASH_STRLEN);
	if (buf == NULL)
		return (NULL);

	FNPREFIX(Final)(digest, ctx);

	for (i = 0; i < HASH_LEN; i++) {
		buf[i+i] = hex[(u_int32_t)digest[i] >> 4];
		buf[i+i+1] = hex[digest[i] & 0x0f];
	}

	buf[i+i] = '\0';
	return (buf);
}

char *
FNPREFIX(FileChunk)(const char *filename, char *buf, off_t off, off_t len)
{
	struct stat sb;
	u_char buffer[BUFSIZ];
	HASH_CTX ctx;
	int fd, save_errno;
	ssize_t nr;

	FNPREFIX(Init)(&ctx);

	if ((fd = open(filename, O_RDONLY)) < 0)
		return (NULL);
	if (len == 0) {
		if (fstat(fd, &sb) == -1) {
			close(fd);
			return (NULL);
		}
		len = sb.st_size;
	}
	if (off > 0 && lseek(fd, off, SEEK_SET) < 0)
		return (NULL);

	while ((nr = read(fd, buffer, (size_t) MIN(sizeof(buffer), len)))
	    > 0) {
		FNPREFIX(Update)(&ctx, buffer, (unsigned int)nr);
		if (len > 0 && (len -= nr) == 0)
			break;
	}

	save_errno = errno;
	close(fd);
	errno = save_errno;
	return (nr < 0 ? NULL : FNPREFIX(End)(&ctx, buf));
}

char *
FNPREFIX(File)(const char *filename, char *buf)
{
	return (FNPREFIX(FileChunk)(filename, buf, (off_t)0, (off_t)0));
}

char *
FNPREFIX(Data)(const unsigned char *data, size_t len, char *buf)
{
	HASH_CTX ctx;

	_DIAGASSERT(data != 0);

	FNPREFIX(Init)(&ctx);
	FNPREFIX(Update)(&ctx, data, (unsigned int)len);
	return (FNPREFIX(End)(&ctx, buf));
}
