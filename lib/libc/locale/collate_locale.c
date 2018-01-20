/*	$NetBSD: collate_locale.c,v 1.1.2.3 2018/01/20 19:36:29 perseant Exp $	*/
/*-
 * Copyright (c)2010 Citrus Project,
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

#ifdef COLLATION_TEST
#include "collation_test.h"
#endif

#define __SETLOCALE_SOURCE__
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "collate.h"

#include "setlocale_local.h"
#include "collate_local.h"
#ifdef DUCET_IS_DEFAULT
# include "ducet_collation_data.h"
#else
# define DUCET_COLLATE_CHAINS_LENGTH 0
# define DUCET_COLLATE_LARGE_LENGTH  0
# define DUCET_COLLATE_RCHAINS_LENGTH  0
collate_char_t ducet_collate_chars[0] = { };
collate_large_t ducet_collate_large[0] = { };
collate_chain_t ducet_collate_chains[0] = { };
collate_rchain_t ducet_collate_rchains[0] = { };
#endif

#ifndef COLLATION_TEST
#include "citrus_module.h"
#endif

const struct collate_info _DefaultCollateInfo = {
	4,     /* uint8_t directive_count;               */
	{},    /* uint8_t directive[COLL_WEIGHTS_MAX];   */
	{},    /* int32_t pri_count[COLL_WEIGHTS_MAX];   */
	0,     /* int32_t flags;                         */
	DUCET_COLLATE_CHAINS_LENGTH, /* int32_t chain_count;       */
	DUCET_COLLATE_LARGE_LENGTH, /* int32_t large_count;       */
	{ 0 }, /* int32_t subst_count[COLL_WEIGHTS_MAX]; */
	{ 0 }, /* int32_t undef_pri[COLL_WEIGHTS_MAX];   */
	DUCET_COLLATE_RCHAINS_LENGTH, /* int32_t rchain_count;       */
};

const struct xlocale_collate _DefaultCollateLocale = {
	0,    /* int __collate_load_error; */
	NULL, /* char * map; */
	0,    /* size_t maplen; */

	&_DefaultCollateInfo, /* collate_info_t	*info; */
	ducet_collate_chars, /* collate_char_t	*char_pri_table; */
	ducet_collate_large, /* collate_large_t	*large_pri_table; */
	ducet_collate_chains, /* collate_chain_t	*chain_pri_table; */
	ducet_collate_rchains, /* collate_rchain_t	*rchain_pri_table; */
	NULL, /* collate_subst_t	*subst_table[COLL_WEIGHTS_MAX]; */
};

static int
_collate_read_file(const char * __restrict, size_t,
		   struct xlocale_collate ** __restrict);

int
_collate_load(const char * __restrict var, size_t lenvar,
	      struct xlocale_collate ** __restrict prl)
{
	_DIAGASSERT(var != NULL || lenvar < 1);
	_DIAGASSERT(prl != NULL);

	if (lenvar < COLLATE_STR_LEN)
		return EFTYPE;

	if (strncmp(var, COLLATE_VERSION, COLLATE_STR_LEN))
		return EFTYPE;

	var += COLLATE_STR_LEN;
	lenvar -= COLLATE_STR_LEN;

	return _collate_read_file(var, lenvar, prl);
	}

static int
_collate_read_file(const char * __restrict var, size_t lenvar,
		   struct xlocale_collate ** __restrict prl)
{
	struct xlocale_collate *clp;
	size_t section_length;
	char *ci;

	if (lenvar < sizeof(struct collate_info))
		return EFTYPE;

	clp = (struct xlocale_collate *)malloc(sizeof(*clp));
	ci = (char *)malloc(lenvar);
	memcpy(ci, var, lenvar);

	/* File header */
	clp->info = (const struct collate_info *)ci;
	ci += sizeof(*clp->info);
	lenvar -= sizeof(*clp->info);

	/* Table of narrow character priorities */
	clp->char_pri_table = (const collate_char_t *)ci;
	section_length = 0x80 * sizeof(collate_chain_t);
	if (lenvar < section_length)
		goto errout;
	ci += section_length;
	lenvar -= section_length;

	/* Collation elements ("chains") */
	clp->chain_pri_table = (const collate_chain_t *)ci;
	section_length = clp->info->chain_count * sizeof(collate_chain_t);
	if (lenvar < section_length)
		goto errout;
	ci += section_length;
	lenvar -= section_length;

	/* Collation weights for characters > 0x80 */
	clp->large_pri_table = (const collate_large_t *)ci;
	section_length = clp->info->large_count * sizeof(collate_large_t);
	if (lenvar < section_length)
		goto errout;
	ci += section_length;
	lenvar -= section_length;

#if 0
	/* Substitutions */
	clp->subst_table = (collate_subst_t *)ci;
	section_length = clp->info->subst_count * sizeof(collate_subst_t);
	if (lenvar < section_length)
		goto errout;
	ci += section_length;
	lenvar -= section_length;
#endif

	/* Characters that have more than one associated weight (reverse chains) */
	clp->rchain_pri_table = (const collate_rchain_t *)ci;
	section_length = clp->info->rchain_count * sizeof(collate_rchain_t);
	if (lenvar < section_length)
		goto errout;
	ci += section_length;
	lenvar -= section_length;

	*prl = clp;
	return 0;

errout:
	free(clp);
	return EFTYPE;
}
