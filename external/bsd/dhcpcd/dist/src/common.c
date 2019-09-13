/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2019 Roy Marples <roy@marples.name>
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

#include <sys/statvfs.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "dhcpcd.h"
#include "if-options.h"
#include "logerr.h"

/* Most route(4) messages are less than 256 bytes. */
#define IOVEC_BUFSIZ	256

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

int
is_root_local(void)
{
#ifdef ST_LOCAL
	struct statvfs vfs;

	if (statvfs("/", &vfs) == -1)
		return -1;
	return vfs.f_flag & ST_LOCAL ? 1 : 0;
#else
	errno = ENOTSUP;
	return -1;
#endif
}
