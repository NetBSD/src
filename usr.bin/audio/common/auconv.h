/*	$NetBSD: auconv.h,v 1.4 2005/12/24 21:48:16 perry Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/audioio.h>

/* Convert between signed and unsigned. */
static inline void change_sign8(u_char *, int);
static inline void change_sign16_le(u_char *, int);
static inline void change_sign16_be(u_char *, int);
static inline void change_sign32_le(u_char *, int);
static inline void change_sign32_be(u_char *, int);
/* Convert between little and big endian. */
static inline void swap_bytes(u_char *, int);
static inline void swap_bytes32(u_char *, int);
static inline void swap_bytes_change_sign16_le(u_char *, int);
static inline void swap_bytes_change_sign16_be(u_char *, int);
static inline void change_sign16_swap_bytes_le(u_char *, int);
static inline void change_sign16_swap_bytes_be(u_char *, int);
static inline void swap_bytes_change_sign32_le(u_char *, int);
static inline void swap_bytes_change_sign32_be(u_char *, int);
static inline void change_sign32_swap_bytes_le(u_char *, int);
static inline void change_sign32_swap_bytes_be(u_char *, int);

static inline void
change_sign8(u_char *p, int cc)
{
	while (--cc >= 0) {
		*p ^= 0x80;
		++p;
	}
}

static inline void
change_sign16_le(u_char *p, int cc)
{
	while ((cc -= 2) >= 0) {
		p[1] ^= 0x80;
		p += 2;
	}
}

static inline void
change_sign16_be(u_char *p, int cc)
{
	while ((cc -= 2) >= 0) {
		p[0] ^= 0x80;
		p += 2;
	}
}

static inline void
change_sign32_le(u_char *p, int cc)
{
	while ((cc -= 4) >= 0) {
		p[3] ^= 0x80;
		p += 4;
	}
}

static inline void
change_sign32_be(u_char *p, int cc)
{
	while ((cc -= 4) >= 0) {
		p[0] ^= 0x80;
		p += 4;
	}
}

static inline void
swap_bytes(u_char *p, int cc)
{
	u_char t;

	while ((cc -= 2) >= 0) {
		t = p[0];
		p[0] = p[1];
		p[1] = t;
		p += 2;
	}
}

static inline void
swap_bytes32(u_char *p, int cc)
{
	u_char t;

	while ((cc -= 4) >= 0) {
		t = p[0];
		p[0] = p[3];
		p[3] = t;
		t = p[1];
		p[1] = p[2];
		p[2] = t;
		p += 4;
	}
}

static inline void
swap_bytes_change_sign16_le(u_char *p, int cc)
{
	u_char t;

	while ((cc -= 2) >= 0) {
		t = p[1];
		p[1] = p[0] ^ 0x80;
		p[0] = t;
		p += 2;
	}
}

static inline void
swap_bytes_change_sign16_be(u_char *p, int cc)
{
	u_char t;

	while ((cc -= 2) >= 0) {
		t = p[0];
		p[0] = p[1] ^ 0x80;
		p[1] = t;
		p += 2;
	}
}

static inline void
change_sign16_swap_bytes_le(u_char *p, int cc)
{
	swap_bytes_change_sign16_be(p, cc);
}

static inline void
change_sign16_swap_bytes_be(u_char *p, int cc)
{
	swap_bytes_change_sign16_le(p, cc);
}

static inline void
swap_bytes_change_sign32_le(u_char *p, int cc)
{
	u_char t;

	while ((cc -= 4) >= 0) {
		t = p[3];
		p[3] = p[0] ^ 0x80;
		p[0] = t;
		t = p[1];
		p[1] = p[2];
		p[2] = t;
		p += 4;
	}
}

static inline void
swap_bytes_change_sign32_be(u_char *p, int cc)
{
	u_char t;

	while ((cc -= 4) >= 0) {
		t = p[0];
		p[0] = p[3] ^ 0x80;
		p[3] = t;
		t = p[1];
		p[1] = p[2];
		p[2] = t;
		p += 4;
	}
}

static inline void
change_sign32_swap_bytes_le(u_char *p, int cc)
{
	swap_bytes_change_sign32_be(p, cc);
}

static inline void
change_sign32_swap_bytes_be(u_char *p, int cc)
{
	swap_bytes_change_sign32_le(p, cc);
}
