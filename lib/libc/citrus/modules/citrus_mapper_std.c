/*	$NetBSD: citrus_mapper_std.c,v 1.3 2003/07/12 15:39:20 tshiozak Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
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
__RCSID("$NetBSD: citrus_mapper_std.c,v 1.3 2003/07/12 15:39:20 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/endian.h>
#include <sys/queue.h>

#include "citrus_namespace.h"
#include "citrus_types.h"
#include "citrus_bcs.h"
#include "citrus_region.h"
#include "citrus_mmap.h"
#include "citrus_module.h"
#include "citrus_hash.h"
#include "citrus_mapper.h"
#include "citrus_db.h"
#include "citrus_db_hash.h"

#include "citrus_mapper_std.h"
#include "citrus_mapper_std_file.h"

/* ---------------------------------------------------------------------- */

_CITRUS_MAPPER_DECLS(mapper_std);
_CITRUS_MAPPER_DEF_OPS(mapper_std);


/* ---------------------------------------------------------------------- */

int
_citrus_mapper_std_mapper_getops(struct _citrus_mapper_ops *ops, size_t lenops,
				 u_int32_t expected_version)
{
	if (expected_version<_CITRUS_MAPPER_ABI_VERSION || lenops<sizeof(*ops))
		return (EINVAL);

	memcpy(ops, &_citrus_mapper_std_mapper_ops,
	       sizeof(_citrus_mapper_std_mapper_ops));

	return (0);
}

/* ---------------------------------------------------------------------- */

static int
/*ARGSUSED*/
rowcol_convert(struct _citrus_mapper_std * __restrict ms,
	       _index_t * __restrict dst, _index_t src,
	       void * __restrict ps)
{
	struct _citrus_mapper_std_rowcol *rc = &ms->ms_rowcol;
	_index_t row, col, idx;
	u_int32_t conv;

	if (rc->rc_src_col_bits == 32) {
		row = 0;
		col = src;
	} else {
		row = src >> rc->rc_src_col_bits;
		col = src & ((1U<<rc->rc_src_col_bits)-1);
	}
	if (row < rc->rc_src_row_begin || row > rc->rc_src_row_end ||
	    col < rc->rc_src_col_begin || col > rc->rc_src_col_end) {
		switch (rc->rc_oob_mode) {
		case _CITRUS_MAPPER_STD_OOB_NONIDENTICAL:
			*dst = rc->rc_dst_invalid;
			return _MAPPER_CONVERT_NONIDENTICAL;
		case _CITRUS_MAPPER_STD_OOB_ILSEQ:
			return _MAPPER_CONVERT_ILSEQ;
		default:
			return _MAPPER_CONVERT_FATAL;
		}
	}

	idx  =
	    (row - rc->rc_src_row_begin)*rc->rc_src_col_width +
	    (col - rc->rc_src_col_begin);

	switch (rc->rc_dst_unit_bits) {
	case 8:
		conv = _region_peek8(&rc->rc_table, idx);
		break;
	case 16:
		conv = be16toh(_region_peek16(&rc->rc_table, idx*2));
		break;
	case 32:
		conv = be32toh(_region_peek32(&rc->rc_table, idx*4));
		break;
	}

	if (conv == rc->rc_dst_invalid) {
		*dst = rc->rc_dst_invalid;
		return _MAPPER_CONVERT_NONIDENTICAL;
	}
	if (conv == rc->rc_dst_ilseq)
		return _MAPPER_CONVERT_ILSEQ;

	*dst = conv;

	return _MAPPER_CONVERT_SUCCESS;
}


static int
rowcol_init(struct _citrus_mapper_std *ms)
{
	int ret;
	struct _citrus_mapper_std_rowcol *rc = &ms->ms_rowcol;
	const struct _citrus_mapper_std_rowcol_info_x *rcx;
	const struct _citrus_mapper_std_rowcol_ext_ilseq_info_x *eix;
	struct _region r;
	u_int64_t table_size;

	ms->ms_convert = &rowcol_convert;
	ms->ms_uninit = NULL;

	/* get table region */
	ret = _db_lookup_by_s(ms->ms_db, _CITRUS_MAPPER_STD_SYM_TABLE,
			      &rc->rc_table, NULL);
	if (ret) {
		if (ret==ENOENT)
			ret = EFTYPE;
		return ret;
	}

	/* get table information */
	ret = _db_lookup_by_s(ms->ms_db, _CITRUS_MAPPER_STD_SYM_INFO, &r, NULL);
	if (ret) {
		if (ret==ENOENT)
			ret = EFTYPE;
		return ret;
	}
	if (_region_size(&r) < sizeof(*rcx))
		return EFTYPE;
	rcx = _region_head(&r);

	/* convert */
#define CONV_ROWCOL(rc, rcx, elem)			\
do {							\
	(rc)->rc_##elem = be32toh((rcx)->rcx_##elem);	\
} while (/*CONSTCOND*/0)
	CONV_ROWCOL(rc, rcx, src_col_bits);
	CONV_ROWCOL(rc, rcx, dst_invalid);
	CONV_ROWCOL(rc, rcx, src_row_begin);
	CONV_ROWCOL(rc, rcx, src_row_end);
	CONV_ROWCOL(rc, rcx, src_col_begin);
	CONV_ROWCOL(rc, rcx, src_col_end);
	CONV_ROWCOL(rc, rcx, dst_unit_bits);

	/* ilseq extension */
	rc->rc_oob_mode = _CITRUS_MAPPER_STD_OOB_NONIDENTICAL;
	rc->rc_dst_ilseq = rc->rc_dst_invalid;
	ret = _db_lookup_by_s(ms->ms_db,
			      _CITRUS_MAPPER_STD_SYM_ROWCOL_EXT_ILSEQ,
			      &r, NULL);
	if (ret && ret != ENOENT)
		return ret;
	if (_region_size(&r) < sizeof(*eix))
		return EFTYPE;
	if (ret == 0) {
		eix = _region_head(&r);
		rc->rc_oob_mode = be32toh(eix->eix_oob_mode);
		rc->rc_dst_ilseq = be32toh(eix->eix_dst_ilseq);
	}
	rc->rc_src_col_width = rc->rc_src_col_end - rc->rc_src_col_begin +1;

	/* validation checks */
	if (rc->rc_src_col_end < rc->rc_src_col_begin ||
	    rc->rc_src_row_end < rc->rc_src_row_begin ||
	    !(rc->rc_dst_unit_bits==8 || rc->rc_dst_unit_bits==16 ||
	      rc->rc_dst_unit_bits==32) ||
	    !(rc->rc_src_col_bits >= 0 && rc->rc_src_col_bits <= 32))
		return EFTYPE;

	/* calcurate expected table size */
	table_size  = rc->rc_src_row_end - rc->rc_src_row_begin + 1;
	table_size *= rc->rc_src_col_width;
	table_size *= rc->rc_dst_unit_bits/8;

	if (table_size > UINT32_MAX ||
	    _region_size(&rc->rc_table) < table_size)
		return EFTYPE;

	return 0;
}

typedef int (*initfunc_t)(struct _citrus_mapper_std *);
static struct {
	const char			*t_name;
	initfunc_t			t_init;
} types[] = {
	{ _CITRUS_MAPPER_STD_TYPE_ROWCOL, &rowcol_init },
};
#define NUM_OF_TYPES ((int)(sizeof(types)/sizeof(types[0])))

static int
/*ARGSUSED*/
_citrus_mapper_std_mapper_init(struct _citrus_mapper_area *__restrict ma,
			       struct _citrus_mapper * __restrict cm,
			       const char * __restrict curdir,
			       const void * __restrict var, size_t lenvar,
			       struct _citrus_mapper_traits * __restrict mt,
			       size_t lenmt)
{
	char path[PATH_MAX];
	const char *type;
	int ret, id;
	struct _citrus_mapper_std *ms;

	/* set traits */
	if (lenmt<sizeof(*mt)) {
		ret = EINVAL;
		goto err0;
	}
	mt->mt_src_max = mt->mt_dst_max = 1;	/* 1:1 converter */
	mt->mt_state_size = 0;			/* stateless */

	/* alloc mapper std structure */
	ms = malloc(sizeof(*ms));
	if (ms==NULL) {
		ret = errno;
		goto err0;
	}

	/* open mapper file */
	snprintf(path, sizeof(path),
		 "%s/%.*s", curdir, (int)lenvar, (const char *)var);
	ret = _map_file(&ms->ms_file, path);
	if (ret)
		goto err1;

	ret = _db_open(&ms->ms_db, &ms->ms_file, _CITRUS_MAPPER_STD_MAGIC,
		       &_db_hash_std, NULL);
	if (ret)
		goto err2;

	/* get mapper type */
	ret = _db_lookupstr_by_s(ms->ms_db, _CITRUS_MAPPER_STD_SYM_TYPE,
				 &type, NULL);
	if (ret) {
		if (ret==ENOENT)
			ret = EFTYPE;
		goto err3;
	}
	for (id=0; id<NUM_OF_TYPES; id++)
		if (_bcs_strcasecmp(type, types[id].t_name) == 0)
			break;

	if (id == NUM_OF_TYPES)
		goto err3;

	/* init the per-type structure */
	ret = (*types[id].t_init)(ms);
	if (ret)
		goto err3;

	cm->cm_closure = ms;

	return 0;

err3:
	_db_close(ms->ms_db);
err2:
	_unmap_file(&ms->ms_file);
err1:
	free(ms);
err0:
	return ret;
}

static void
/*ARGSUSED*/
_citrus_mapper_std_mapper_uninit(struct _citrus_mapper *cm)
{
	struct _citrus_mapper_std *ms;

	_DIAGASSERT(cm!=NULL & cm->cm_closure!=NULL);

	ms = cm->cm_closure;
	if (ms->ms_uninit)
		(*ms->ms_uninit)(ms);
	_db_close(ms->ms_db);
	_unmap_file(&ms->ms_file);
	free(ms);
}

static void
/*ARGSUSED*/
_citrus_mapper_std_mapper_init_state(struct _citrus_mapper * __restrict cm,
				     void * __restrict ps)
{
}

static int
/*ARGSUSED*/
_citrus_mapper_std_mapper_convert(struct _citrus_mapper * __restrict cm,
				  _index_t * __restrict dst, _index_t src,
				  void * __restrict ps)
{
	struct _citrus_mapper_std *ms;

	_DIAGASSERT(cm!=NULL && cm->cm_closure!=NULL);

	ms = cm->cm_closure;

	_DIAGASSERT(ms->ms_convert != NULL);

	return (*ms->ms_convert)(ms, dst, src, ps);
}
