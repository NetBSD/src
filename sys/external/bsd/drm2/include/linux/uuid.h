/*	$NetBSD: uuid.h,v 1.4 2021/12/19 11:38:04 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX_UUID_H_
#define _LINUX_UUID_H_

typedef struct {
	unsigned char guid_bytes[16];
} guid_t;

#define	GUID_INIT(x, y, z, v0, v1, v2, v3, v4, v5, v6, v7) ((guid_t)	      \
{									      \
	.guid_bytes = {							      \
		[0] = (x) & 0xff,					      \
		[1] = ((x) >> 8) & 0xff,				      \
		[2] = ((x) >> 16) & 0xff,				      \
		[3] = ((x) >> 24) & 0xff,				      \
		[4] = (y) & 0xff,					      \
		[5] = ((y) >> 8) & 0xff,				      \
		[6] = (z) & 0xff,					      \
		[7] = ((z) >> 8) & 0xff,				      \
		[8] = (v0),						      \
		[9] = (v1),						      \
		[10] = (v2),						      \
		[11] = (v3),						      \
		[12] = (v4),						      \
		[13] = (v5),						      \
		[14] = (v6),						      \
		[15] = (v7),						      \
	}								      \
})

#define	UUID_STRING_LEN		36

static inline int
uuid_is_valid(const char uuid[static 36])
{
	unsigned i;

	for (i = 0; i < 36; i++) {
		switch (i) {
		case 8:		/* xxxxxxxx[-]xxxx-xxxx-xxxx-xxxxxxxxxxxx */
		case 12 + 1:	/* xxxxxxxx-xxxx[-]xxxx-xxxx-xxxxxxxxxxxx */
		case 16 + 2:	/* xxxxxxxx-xxxx-xxxx[-]xxxx-xxxxxxxxxxxx */
		case 20 + 3:	/* xxxxxxxx-xxxx-xxxx-xxxx[-]xxxxxxxxxxxx */
			if (uuid[i] == '-')
				continue;
			return 0;
		default:
			if ('0' <= uuid[i] && uuid[i] <= '9')
				continue;
			if ('a' <= uuid[i] && uuid[i] <= 'f')
				continue;
			if ('A' <= uuid[i] && uuid[i] <= 'F')
				continue;
			return 0;
		}
	}

	return 1;
}

#endif  /* _LINUX_UUID_H_ */
