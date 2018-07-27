/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2018 Roy Marples <roy@marples.name>
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

#include <sys/param.h>
#include <sys/time.h>
#ifdef __sun
#include <sys/sysmacros.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#ifdef BSD
#  include <paths.h>
#endif
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "dhcpcd.h"
#include "if-options.h"
#include "logerr.h"

/* Most route(4) messages are less than 256 bytes. */
#define IOVEC_BUFSIZ	256

ssize_t
setvar(char **e, const char *prefix, const char *var, const char *value)
{
	size_t len = strlen(var) + strlen(value) + 3;

	if (prefix)
		len += strlen(prefix) + 1;
	if ((*e = malloc(len)) == NULL) {
		logerr(__func__);
		return -1;
	}
	if (prefix)
		snprintf(*e, len, "%s_%s=%s", prefix, var, value);
	else
		snprintf(*e, len, "%s=%s", var, value);
	return (ssize_t)len;
}

ssize_t
setvard(char **e, const char *prefix, const char *var, size_t value)
{

	char buffer[32];

	snprintf(buffer, sizeof(buffer), "%zu", value);
	return setvar(e, prefix, var, buffer);
}

ssize_t
addvar(char ***e, const char *prefix, const char *var, const char *value)
{
	ssize_t len;

	len = setvar(*e, prefix, var, value);
	if (len != -1)
		(*e)++;
	return (ssize_t)len;
}

ssize_t
addvard(char ***e, const char *prefix, const char *var, size_t value)
{
	char buffer[32];

	snprintf(buffer, sizeof(buffer), "%zu", value);
	return addvar(e, prefix, var, buffer);
}

const char *
hwaddr_ntoa(const void *hwaddr, size_t hwlen, char *buf, size_t buflen)
{
	const unsigned char *hp, *ep;
	char *p;

	if (buf == NULL)
		return NULL;

	if (hwlen * 3 > buflen) {
		errno = ENOBUFS;
		return NULL;
	}

	hp = hwaddr;
	ep = hp + hwlen;
	p = buf;

	while (hp < ep) {
		if (hp != hwaddr)
			*p ++= ':';
		p += snprintf(p, 3, "%.2x", *hp++);
	}
	*p ++= '\0';
	return buf;
}

size_t
hwaddr_aton(uint8_t *buffer, const char *addr)
{
	char c[3];
	const char *p = addr;
	uint8_t *bp = buffer;
	size_t len = 0;

	c[2] = '\0';
	while (*p != '\0') {
		/* Skip separators */
		c[0] = *p++;
		switch (c[0]) {
		case '\n':	/* long duid split on lines */
		case ':':	/* typical mac address */
		case '-':	/* uuid */
			continue;
		}
		c[1] = *p++;
		/* Ensure that digits are hex */
		if (isxdigit((unsigned char)c[0]) == 0 ||
		    isxdigit((unsigned char)c[1]) == 0)
		{
			errno = EINVAL;
			return 0;
		}
		/* We should have at least two entries 00:01 */
		if (len == 0 && *p == '\0') {
			errno = EINVAL;
			return 0;
		}
		if (bp)
			*bp++ = (uint8_t)strtol(c, NULL, 16);
		len++;
	}
	return len;
}

size_t
read_hwaddr_aton(uint8_t **data, const char *path)
{
	FILE *fp;
	char *buf;
	size_t buf_len, len;

	if ((fp = fopen(path, "r")) == NULL)
		return 0;

	buf = NULL;
	buf_len = len = 0;
	*data = NULL;
	while (getline(&buf, &buf_len, fp) != -1) {
		if ((len = hwaddr_aton(NULL, buf)) != 0) {
			if (buf_len >= len)
				*data = (uint8_t *)buf;
			else {
				if ((*data = malloc(len)) == NULL)
					len = 0;
			}
			if (len != 0)
				(void)hwaddr_aton(*data, buf);
			if (buf_len < len)
				free(buf);
			break;
		}
	}
	fclose(fp);
	return len;
}

ssize_t
recvmsg_realloc(int fd, struct msghdr *msg, int flags)
{
	struct iovec *iov;
	ssize_t slen;
	size_t len;
	void *n;

	assert(msg != NULL);
	assert(msg->msg_iov != NULL && msg->msg_iovlen > 0);
	assert((flags & (MSG_PEEK | MSG_TRUNC)) == 0);

	/* Assume we are reallocing the last iovec. */
	iov = &msg->msg_iov[msg->msg_iovlen - 1];

	for (;;) {
		/* Passing MSG_TRUNC should return the actual size needed. */
		slen = recvmsg(fd, msg, flags | MSG_PEEK | MSG_TRUNC);
		if (slen == -1)
			return -1;
		if (!(msg->msg_flags & MSG_TRUNC))
			break;

		len = (size_t)slen;

		/* Some kernels return the size of the receive buffer
		 * on truncation, not the actual size needed.
		 * So grow the buffer and try again. */
		if (iov->iov_len == len)
			len++;
		else if (iov->iov_len > len)
			break;
		len = roundup(len, IOVEC_BUFSIZ);
		if ((n = realloc(iov->iov_base, len)) == NULL)
			return -1;
		iov->iov_base = n;
		iov->iov_len = len;
	}

	slen = recvmsg(fd, msg, flags);
	if (slen != -1 && msg->msg_flags & MSG_TRUNC) {
		/* This should not be possible ... */
		errno = ENOBUFS;
		return -1;
	}
	return slen;
}
