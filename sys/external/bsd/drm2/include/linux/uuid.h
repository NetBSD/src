/*	$NetBSD: uuid.h,v 1.3 2021/12/19 11:36:57 riastradh Exp $	*/

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
