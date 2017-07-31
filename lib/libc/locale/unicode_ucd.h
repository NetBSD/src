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

#ifndef _UNICODE_UCD_H
#define _UNICODE_UCD_H

#ifdef COLLATION_TEST
#include "collation_test.h"
#endif

#include <sys/types.h>
#include <sys/queue.h>
#include <locale.h>

#include "collate_local.h"
#include "collate.h"

typedef int32_t ucd_codepoint_t;

TAILQ_HEAD(collation_set, collation_element);
SLIST_HEAD(ucd_nfd_qc_head, ucd_nfd_qc);
SLIST_HEAD(ucd_ccc_head, ucd_ccc);
SLIST_HEAD(ucd_coll_head, ucd_coll);
SLIST_HEAD(ucd_resv_cp_head, ucd_resv_cp);
SLIST_HEAD(ucd_resv_range_head, ucd_resv_range);

#define _COLLATE_LOCALE(loc) \
  ((struct xlocale_collate *)((loc)->part_impl[(size_t)LC_COLLATE]))

/* Data definition */
#if 0
struct ucd_decomp {
	ucd_codepoint_t cp;
	int n;
	ucd_codepoint_t dm[20]; /* At present, no more than 18 */
	SLIST_ENTRY(ucd_decomp) hash_next;
};
#endif

struct ucd_decomp_single {
	ucd_codepoint_t cp;
	ucd_codepoint_t dm;
};

struct ucd_decomp_pair {
	ucd_codepoint_t cp;
	ucd_codepoint_t dm[2];
};

struct ucd_decomp_misc {
	ucd_codepoint_t cp;
	int n;
	ucd_codepoint_t dm[20]; /* At present, no more than 18 */
};

struct ucd_ccc {
	ucd_codepoint_t cp;
	uint8_t ccc;
	SLIST_ENTRY(ucd_ccc) hash_next;
};

struct ucd_nfd_qc {
	ucd_codepoint_t cp;
	uint8_t nfd_qc;
	SLIST_ENTRY(ucd_nfd_qc) hash_next;
};

struct ucd_resv_cp {
	ucd_codepoint_t cp;
	SLIST_ENTRY(ucd_resv_cp) hash_next;
};

struct ucd_resv_range {
	ucd_codepoint_t firstcp;
	ucd_codepoint_t lastcp;
	SLIST_ENTRY(ucd_resv_range) hash_next;
};

struct collation_element {
	const weight_t *pri;
	int len;
#define CE_FLAGS_NEEDSFREE 0x00000001
	u_int32_t flags;
	TAILQ_ENTRY(collation_element) tailq;
};

/* depends on these data-using functions */
uint8_t unicode_get_ccc(wchar_t);
uint8_t unicode_get_nfd_qc(wchar_t);
wchar_t *unicode_decomposition(wchar_t, int *);
int unicode_get_collation_element(struct collation_set *, wchar_t *, locale_t loc);
int ucd_check_unassigned(wchar_t);

#endif /* _UNICODE_UCD_H */
