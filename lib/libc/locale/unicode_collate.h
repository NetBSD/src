/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/queue.h>
#include "unicode_ucd.h"

#define COLL_BACKWARDS 0x0001
#define COLLATE_MAX_LEVEL(x) (4)
#define COLLATE_FLAGS(x, y) (0)

static wchar_t *
unicode_recursive_decompose(const wchar_t *, wchar_t **);
static void
unicode_wchar2coll(struct collation_set *, const wchar_t *, locale_t);
#if 0
static unsigned char *
unicode_coll2key(struct collation_set *, int, locale_t);
#endif
static uint16_t *
unicode_coll2numkey(struct collation_set *, int *, locale_t);
static void
unicode_wcstokey(uint16_t **, size_t *, const wchar_t *, wchar_t **, locale_t);

int 
unicode_wcscoll_l(const wchar_t *, const wchar_t *, locale_t);
size_t
unicode_wcsxfrm_l(wchar_t *, const wchar_t *, size_t, locale_t);

