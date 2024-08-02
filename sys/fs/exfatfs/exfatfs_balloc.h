/* $NetBSD: exfatfs_balloc.h,v 1.1.2.4 2024/08/02 00:16:55 perseant Exp $ */

/*-
 * Copyright (c) 2022, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#ifndef EXFATFS_BALLOC_H_
#define EXFATFS_BALLOC_H_

#include <fs/exfatfs/exfatfs.h>

/* Convert cluster number to logical block and offset in bitmap */
#define NBBYSHIFT		3 /* 1 << NBBYSHIFT == NBBY == 8 */
#define BITMAPSHIFT(fs)		(EXFATFS_LSHIFT(fs) + NBBYSHIFT)
#define BITMAPLBN(fs, cn)	(((cn) - 2) >> BITMAPSHIFT(fs))
#define BITMAPOFF(fs, cn)	(((cn) - 2) & ((1 << BITMAPSHIFT(fs)) - 1))
#define LBNOFF2CLUSTER(fs, lbn, off) ((lbn << BITMAPSHIFT(fs)) + (off) + 2)
#define INVALID			(~(uint32_t)0)

int exfatfs_bitmap_init(struct exfatfs *);
void exfatfs_bitmap_destroy(struct exfatfs *);
int exfatfs_bitmap_alloc(struct exfatfs *, uint32_t, uint32_t *);
int exfatfs_bitmap_dealloc(struct exfatfs *, uint32_t, int);

#endif /* EXFATFS_BALLOC_H_ */
