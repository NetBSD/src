/*	$NetBSD: bootinfo_memmap.c,v 1.6.26.1 2024/09/03 15:23:40 martin Exp $	*/

/*
 * Copyright (c) 1999
 *	Matthias Drochner.  All rights reserved.
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
 *
 */

#include <lib/libsa/stand.h>
#include "libi386.h"
#include "bootinfo.h"

struct btinfo_memmap *btinfo_memmap = NULL;

extern int getmementry(int *, int *);

void
bi_getmemmap(void)
{
	/*
	 * Allocate 6 words, not 5, to work around buggy firmware --
	 * see comment on getmementry in biosmemx.S.
	 */
	int buf[6], i, nranges, n;

	nranges = 0;
	i = 0;
	do {
		if (getmementry(&i, buf))
			break;
		nranges++;
	} while (i);

	btinfo_memmap = alloc(sizeof(struct btinfo_memmap)
		+ (nranges - 1) * sizeof(struct bi_memmap_entry));

	i = 0;
	for (n = 0; n < nranges; n++) {
		getmementry(&i, buf);
		memcpy(&btinfo_memmap->entry[n], buf,
		       sizeof(struct bi_memmap_entry));
	}
	btinfo_memmap->num = nranges;

	BI_ADD(btinfo_memmap, BTINFO_MEMMAP, sizeof(struct btinfo_memmap)
	       + (nranges - 1) * sizeof(struct bi_memmap_entry));
}
