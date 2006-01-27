/*	$NetBSD: bootinfo.c,v 1.1 2006/01/27 18:31:12 cdi Exp $	*/

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

#include <sys/param.h>
#include <machine/types.h>
#include <machine/bootinfo.h>
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "boot.h"

static char *bootinfo = NULL;
static char *bi_next;
static int bi_size;

u_long
bi_init(addr)
	u_long addr;
{
	struct btinfo_common *bi;
	struct btinfo_magic bi_magic;
	uint64_t *endp;

	endp = (uint64_t *)ALIGN(addr);

	/*
	 * Allocate and zero-fill bootinfo structure.
	 */
	sparc64_memset(endp, 0, BOOTINFO_SIZE);

	/*
	 * For the first word we load our end address,
	 * giving enough space for the bootinfo structure.
	 */
	endp[0] = (uint64_t)(u_long)((char *)endp + BOOTINFO_SIZE);
	bootinfo = (char *)ALIGN(&endp[2]);
	endp[1] = (uint64_t)(u_long)bootinfo;
	bi = (struct btinfo_common *)bootinfo;
	bi->next = bi->type = 0;
	bi_next = bootinfo;
	bi_size = 0;

	bi_magic.magic = BOOTINFO_MAGIC;
	bi_add(&bi_magic, BTINFO_MAGIC, sizeof(bi_magic));
	return (u_long) endp;
}

void
bi_add(new, type, size)
	void *new;
	int type;
	size_t size;
{
	struct btinfo_common *bi;

	if (bi_size + size > BOOTINFO_SIZE)
		return;				/* XXX error? */

	bi = new;
	bi->next = ALIGN(size);
	bi->type = type;
	sparc64_memcpy(bi_next, new, size);
	bi_next += ALIGN(size);

	bi = (struct btinfo_common *)bi_next;
	bi->next = bi->type = 0;
}
