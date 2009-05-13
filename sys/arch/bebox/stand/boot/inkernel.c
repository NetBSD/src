/*	$NetBSD: inkernel.c,v 1.8.12.1 2009/05/13 17:16:34 jym Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Kazuki Sakamoto.
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

#include <lib/libsa/stand.h>
#include <sys/param.h>
#include "boot.h"
#include "magic.h"

#define	KERNENTRY	(RELOC - 0x200000)

int inopen(struct open_file *);
int inclose(struct open_file *);
int instrategy(void *, int, daddr_t, size_t, void *, size_t *);

void
init_in(void)
{
	int p;

	for (p = endaddr(); p < KERNENTRY; p += sizeof (int)) {
		if (*(int *)p != BEBOX_ENTRY ||
		    *(int *)(p + sizeof (int)) != 0 ||
		    *(int *)(p + sizeof (int) * 2) != 0)
			continue;

		p += sizeof (int) * 3;
		if (memcmp((char *)p, bebox_magic, BEBOX_MAGICSIZE) == 0) {
			kern_len = *(int *)(p + BEBOX_MAGICSIZE);
			memcpy((char *)KERNENTRY,
				(char *)(p + BEBOX_MAGICSIZE + KERNLENSIZE),
				kern_len);
			break;
		}
	}
}

int
inopen(struct open_file *p)
{

	if (kern_len)
		return (0);
	return (EINVAL);
}

int
inclose(struct open_file *p)
{

	return (0);
}

int
instrategy(void *devdata, int func, daddr_t blk, size_t size, void *buf,
	   size_t *rsize)
{

	memcpy(buf, (char *)KERNENTRY + blk * DEV_BSIZE, size);
	*rsize = size;
	return (0);
}
