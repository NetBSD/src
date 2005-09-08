/* $NetBSD: sha384hl.c,v 1.3.2.2 2005/09/08 19:15:44 tron Exp $ */

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dkuug.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 */

#include "namespace.h"

#include <sys/param.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <crypto/sha2.h>

/* ARGSUSED */
char *
SHA384_End(SHA384_CTX *ctx, char *buf)
{
	int i;
	u_int8_t digest[SHA384_DIGEST_LENGTH];
	static const char hex[] = "0123456789abcdef";

	if (buf == NULL && (buf = malloc(SHA384_DIGEST_STRING_LENGTH)) == NULL)
		return (NULL);

	SHA384_Final(digest, ctx);
	for (i = 0; i < SHA384_DIGEST_LENGTH; i++) {
		buf[i + i] = hex[(u_int32_t)digest[i] >> 4];
		buf[i + i + 1] = hex[digest[i] & 0x0f];
	}
	buf[i + i] = '\0';
	memset(digest, 0, sizeof(digest));
	return (buf);
}

char *
SHA384_FileChunk(const char *filename, char *buf, off_t off, off_t len)
{
	struct stat sb;
	u_char buffer[BUFSIZ];
	SHA384_CTX ctx;
	int fd, save_errno;
	ssize_t nr;

	SHA384_Init(&ctx);

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
		SHA384_Update(&ctx, buffer, (size_t)nr);
		if (len > 0 && (len -= nr) == 0)
			break;
	}

	save_errno = errno;
	close(fd);
	errno = save_errno;
	return (nr < 0 ? NULL : SHA384_End(&ctx, buf));
}

char *
SHA384_File(const char *filename, char *buf)
{
	return (SHA384_FileChunk(filename, buf, (off_t)0, (off_t)0));
}

char *
SHA384_Data(const u_char *data, size_t len, char *buf)
{
	SHA384_CTX ctx;

	SHA384_Init(&ctx);
	SHA384_Update(&ctx, data, len);
	return (SHA384_End(&ctx, buf));
}
