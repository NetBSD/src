/* $NetBSD: citrus_lc_ctype.c,v 1.8 2010/06/13 04:14:56 tnozaki Exp $ */

/*-
 * Copyright (c)2008 Citrus Project,
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_lc_ctype.c,v 1.8 2010/06/13 04:14:56 tnozaki Exp $");
#endif /* LIBC_SCCS and not lint */

#include "reentrant.h"
#include <sys/types.h>
#include <sys/ctype_bits.h>
#include <sys/queue.h>
#include <assert.h>
#include <errno.h>
#include <langinfo.h>
#include <limits.h>
#define __SETLOCALE_SOURCE__
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "setlocale_local.h"

#include "citrus_namespace.h"
#include "citrus_bcs.h"
#include "citrus_region.h"
#include "citrus_lookup.h"
#include "citrus_aliasname_local.h"
#include "citrus_module.h"
#include "citrus_ctype.h"

#include "runetype_local.h"
#include "multibyte.h"

/*
 * macro required by all template headers
 */
#define _PREFIX(name)		__CONCAT(_citrus_LC_CTYPE_, name)

/*
 * macro required by nb_lc_template(_decl).h
 */
#define _CATEGORY_TYPE          _RuneLocale

#include "nb_lc_template_decl.h"

static int
_citrus_LC_CTYPE_create_impl(const char * __restrict root,
    const char * __restrict name, _RuneLocale ** __restrict pdata)
{
	char path[PATH_MAX + 1];
	FILE *fp;
	_RuneLocale *data;
	int ret;

	_DIAGASSERT(root != NULL);
	_DIAGASSERT(name != NULL);
	_DIAGASSERT(pdata != NULL);

	snprintf(path, sizeof(path),
	    "%s/%s/LC_CTYPE", root, name);
	fp = fopen(path, "r");
	if (fp == NULL)
		return ENOENT;
	data = _Read_RuneMagi(fp);
	if (data == NULL) {
		data = _Read_CTypeAsRune(fp);
		if (data == NULL) {
			fclose(fp);
			return EFTYPE;
		}
	}
	fclose(fp);
	ret = _citrus_ctype_open(&data->rl_citrus_ctype, data->rl_encoding,
	   data->rl_variable, data->rl_variable_len, _PRIVSIZE);
	if (!ret)
		ret = __runetable_to_netbsd_ctype(data);
	if (ret || __mb_len_max_runtime <
	    _citrus_ctype_get_mb_cur_max(data->rl_citrus_ctype)) {
		_NukeRune(data);
		return EINVAL;
	}
	data->rl_wctrans[_WCTRANS_INDEX_LOWER].te_name = "tolower";
	data->rl_wctrans[_WCTRANS_INDEX_LOWER].te_cached = &data->rl_maplower[0];
	data->rl_wctrans[_WCTRANS_INDEX_LOWER].te_extmap = &data->rl_maplower_ext;

	data->rl_wctrans[_WCTRANS_INDEX_UPPER].te_name = "toupper";
	data->rl_wctrans[_WCTRANS_INDEX_UPPER].te_cached = &data->rl_mapupper[0];
	data->rl_wctrans[_WCTRANS_INDEX_UPPER].te_extmap = &data->rl_mapupper_ext;

	*pdata = data;
	return 0;
}

static __inline void
_PREFIX(build_cache)(struct _locale_cache_t * __restrict cache,
    _RuneLocale * __restrict data)
{
	_DIAGASSERT(cache != NULL);
	_DIAGASSERT(cache->items != NULL);
	_DIAGASSERT(data != NULL);

	cache->ctype_tab = data->rl_ctype_tab;
	cache->tolower_tab = data->rl_tolower_tab;
	cache->toupper_tab = data->rl_toupper_tab;
	cache->mb_cur_max = _citrus_ctype_get_mb_cur_max(data->rl_citrus_ctype);
	cache->items[(size_t)CODESET] = data->rl_codeset;
}

static __inline void
_PREFIX(fixup)(_RuneLocale *data)
{
	_DIAGASSERT(data != NULL);

	__mb_cur_max = _citrus_ctype_get_mb_cur_max(data->rl_citrus_ctype);
	_ctype_ = data->rl_ctype_tab;
	_tolower_tab_ = data->rl_tolower_tab;
	_toupper_tab_ = data->rl_toupper_tab;
	_CurrentRuneLocale = data;
}

/*
 * macro required by nb_lc_template.h
 */
#define _CATEGORY_ID		LC_CTYPE
#define _CATEGORY_NAME		"LC_CTYPE"
#define _CATEGORY_DEFAULT	_DefaultRuneLocale

#include "nb_lc_template.h"
_LOCALE_CATEGORY_ENTRY(_citrus_LC_CTYPE_);
