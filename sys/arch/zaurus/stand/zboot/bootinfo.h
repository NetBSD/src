/*	$NetBSD: bootinfo.h,v 1.1.6.2 2009/05/13 17:18:54 jym Exp $	*/

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

#ifndef	_STAND_BOOTINFO_H_
#define	_STAND_BOOTINFO_H_

#include <machine/bootinfo.h>

struct btinfo {
	int nentries;
	u_long entry[1];
};

extern struct btinfo *bootinfo;

#define BI_ALLOC(max) (bootinfo = ALLOC(sizeof(struct btinfo) \
                                        + ((max) - 1) * sizeof(u_long))) \
                      ->nentries = 0

#define BI_FREE() DEALLOC(bootinfo, 0)

#define BI_ADD(x, type, size) bi_add((struct btinfo_common *)(x), type, size)
#define BI_DEL(type) bi_del(type)

void bi_add(struct btinfo_common *, int, int);
void bi_del(int);

#endif	/* _STAND_BOOTINFO_H_ */
