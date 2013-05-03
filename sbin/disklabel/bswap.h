/*	$NetBSD: bswap.h,v 1.2 2013/05/03 16:05:12 matt Exp $	*/

/*-
 * Copyright (c) 2009 Izumi Tsutsui.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>

#if HAVE_NBTOOL_CONFIG_H
#ifndef BYTE_ORDER
#ifdef WORDS_BIGENDIAN
#define BYTE_ORDER		BIG_ENDIAN
#else
#define BYTE_ORDER		LITTLE_ENDIAN
#endif
#endif
#else
#include <sys/endian.h>
#endif

extern int bswap_p;
extern u_int maxpartitions;

#define htotarget16(x)		(bswap_p ? bswap16(x) : (x))
#define target16toh(x)		(bswap_p ? bswap16(x) : (x))
#define htotarget32(x)		(bswap_p ? bswap32(x) : (x))
#define target32toh(x)		(bswap_p ? bswap32(x) : (x))

void htotargetlabel(struct disklabel *, const struct disklabel *);
void targettohlabel(struct disklabel *, const struct disklabel *);
uint16_t dkcksum_target(struct disklabel *);
