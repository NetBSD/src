/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2009 Roy Marples <roy@marples.name>
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

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "getline.h"

/* Redefine a small buffer for our simple text config files */
#undef BUFSIZ
#define BUFSIZ 128

ssize_t
getline(char ** __restrict buf, size_t * __restrict buflen,
    FILE * __restrict fp)
{
	size_t bytes, newlen;
	char *newbuf, *p;
	
	if (buf == NULL || buflen == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (*buf == NULL)
		*buflen = 0;
	
	bytes = 0;
	do {
		if (feof(fp))
			break;
		if (*buf == NULL || bytes != 0) {
			newlen = *buflen + BUFSIZ;
			newbuf = realloc(*buf, newlen);
			if (newbuf == NULL)
				return -1;
			*buf = newbuf;
			*buflen = newlen;
		}
		p = *buf + bytes;
		memset(p, 0, BUFSIZ);
		if (fgets(p, BUFSIZ, fp) == NULL)
			break;
		bytes += strlen(p);
	} while (bytes == 0 || *(*buf + (bytes - 1)) != '\n');
	if (bytes == 0)
		return -1;
	return bytes;
}
