/*	$NetBSD: byteorder.c,v 1.1 2001/10/30 23:35:33 thorpej Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "byteorder.h"

typedef union {
	uint16_t val;
	uint8_t bytes[2];
} un16;

typedef union {
	uint32_t val;
	uint8_t bytes[4];
} un32;

/* 16-bit */

uint16_t
sa_htobe16(uint16_t val)
{
	un16 un;

	un.bytes[1] = val & 0xff;
	un.bytes[0] = (val >> 8) & 0xff;

	return (un.val);
}

uint16_t
sa_htole16(uint16_t val)
{
	un16 un;

	un.bytes[0] = val & 0xff;
	un.bytes[1] = (val >> 8) & 0xff;

	return (un.val);
}

uint16_t
sa_be16toh(uint16_t val)
{
	un16 un;

	un.val = val;

	return ((un.bytes[0] << 8) |
		 un.bytes[1]);
}

uint16_t
sa_le16toh(uint16_t val)
{
	un16 un;

	un.val = val;

	return ((un.bytes[1] << 8) |
		 un.bytes[0]);
}

/* 32-bit */

uint32_t
sa_htobe32(uint32_t val)
{
	un32 un;

	un.bytes[3] = val & 0xff;
	un.bytes[2] = (val >> 8) & 0xff;
	un.bytes[1] = (val >> 16) & 0xff;
	un.bytes[0] = (val >> 24) & 0xff;

	return (un.val);
}

uint32_t
sa_htole32(uint32_t val)
{
	un32 un;

	un.bytes[0] = val & 0xff;
	un.bytes[1] = (val >> 8) & 0xff;
	un.bytes[2] = (val >> 16) & 0xff;
	un.bytes[3] = (val >> 24) & 0xff;

	return (un.val);
}

uint32_t
sa_be32toh(uint32_t val)
{
	un32 un;

	un.val = val;

	return ((un.bytes[0] << 24) |
		(un.bytes[1] << 16) |
		(un.bytes[2] << 8) |
		 un.bytes[3]);
}

uint32_t
sa_le32toh(uint32_t val)
{
	un32 un;

	un.val = val;

	return ((un.bytes[3] << 24) |
		(un.bytes[2] << 16) |
		(un.bytes[1] << 8) |
		 un.bytes[0]);
}
