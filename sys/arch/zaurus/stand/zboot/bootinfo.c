/*	$NetBSD: bootinfo.c,v 1.1.6.2 2009/05/13 17:18:54 jym Exp $	*/

/*
 * Copyright (c) 1997
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

#include "boot.h"
#include "bootinfo.h"

struct btinfo *bootinfo;

static int
bi_find(int type)
{
	struct btinfo_common *btinfo;
	int i;
	
	for (i = 0; i < bootinfo->nentries; i++) {
		btinfo = (struct btinfo_common *)(bootinfo->entry[i]);
		if (btinfo->type == type) {
			return i;
		}
	}
	return -1;
}

void
bi_add(struct btinfo_common *what, int type, int size)
{
	int idx;

	what->len = size;
	what->type = type;

	if (bootinfo) {
		idx = bi_find(type);
		if (idx < 0) {
			idx = bootinfo->nentries++;
		}
		bootinfo->entry[idx] = (u_long)what;
	}
}

void
bi_del(int type)
{
	int idx;
	int i;

	if (bootinfo) {
		idx = bi_find(type);
		if (idx >= 0) {
			for (i = idx + 1; i < bootinfo->nentries; i++) {
				bootinfo->entry[i - 1] = bootinfo->entry[i];
			}
			bootinfo->entry[--bootinfo->nentries] = 0L;
		}
	}
}
