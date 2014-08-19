/*	$NetBSD: bootinfo.h,v 1.1.10.2 2014/08/20 00:03:05 tls Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan Stone, Michael Hitch and Simon Burge.
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

#include <machine/bootinfo.h>

#define BOOTINFO_MAXSIZE 1024
#define BOOTINFO_DATASIZE (BOOTINFO_MAXSIZE - 2 * sizeof(int))

/*
 * Structure that holds the information passed by the boot loader.
 */
struct bootinfo {
	int	bi_nentries;	/* Number of bootinfo_* entries in bi_data. */
	int	bi_offset;	/* Offset into bi_data for next bootinfo_* */

	/* Raw data of bootinfo entries.  The first one (if any) is
	 * found at &bi_data[0] and can be cast to (bootinfo_common *).
	 * Once this is done, the following entry is found at 'len'
	 * offset as specified by the previous entry. */
	char	bi_data[BOOTINFO_DATASIZE];
};

extern struct bootinfo bootinfo;

#define	BI_ADD(x, type, size) bi_add((struct btinfo_common *)(x), type, size)

void	bi_init(void);
void	bi_add(struct btinfo_common *, int, size_t);
