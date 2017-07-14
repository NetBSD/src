/*	$NetBSD: collate_locale.c,v 1.1.2.1 2017/07/14 15:53:08 perseant Exp $	*/
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

#define __SETLOCALE_SOURCE__
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "setlocale_local.h"
#include "collate_local.h"
#include "unicode_collation_data.h"

#include "citrus_module.h"

const _CollateLocale _DefaultCollateLocale = {
	__UNCONST("DUCET"),
	5,
	&ucd_collate_data[0],
	UCD_COLLATE_DATA_LENGTH
};

int
_collate_load(const char * __restrict var, size_t lenvar,
    _CollateLocale ** __restrict prl)
{
	int ret;

	_DIAGASSERT(var != NULL || lenvar < 1);
	_DIAGASSERT(prl != NULL);

	if (lenvar < 1)
		return EFTYPE;
	switch (*var) {
	case 'U':
#ifdef notyet
		ret = _collate_read_file(var, lenvar, prl);
#else
		*prl = (_CollateLocale *)malloc(sizeof(**prl));
		(*prl)->coll_variable = __UNCONST("FAKE");
		(*prl)->coll_variable_len = 4;
		(*prl)->coll_data = (struct ucd_coll *)malloc(sizeof(struct ucd_coll));
		(*prl)->coll_data_len = 0;
		ret = 0;
#endif
		break;
	default:
		ret = EFTYPE;
	}
	return ret;
}
