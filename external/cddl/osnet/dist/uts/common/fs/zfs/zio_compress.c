/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright (c) 2013 by Saso Kiselkov. All rights reserved.
 */

/*
 * Copyright (c) 2013 by Delphix. All rights reserved.
 */

#include <sys/zfs_context.h>
#include <sys/compress.h>
#include <sys/kstat.h>
#include <sys/spa.h>
#include <sys/zfeature.h>
#include <sys/zio.h>
#include <sys/zio_compress.h>

typedef struct zcomp_stats {
	kstat_named_t zcompstat_attempts;
	kstat_named_t zcompstat_empty;
	kstat_named_t zcompstat_skipped_insufficient_gain;
} zcomp_stats_t;

static zcomp_stats_t zcomp_stats = {
	{ "attempts",			KSTAT_DATA_UINT64 },
	{ "empty",			KSTAT_DATA_UINT64 },
	{ "skipped_insufficient_gain",	KSTAT_DATA_UINT64 }
};

#define	ZCOMPSTAT_INCR(stat, val) \
	atomic_add_64(&zcomp_stats.stat.value.ui64, (val));

#define	ZCOMPSTAT_BUMP(stat)		ZCOMPSTAT_INCR(stat, 1);

kstat_t		*zcomp_ksp;

/*
 * Compression vectors.
 */

zio_compress_info_t zio_compress_table[ZIO_COMPRESS_FUNCTIONS] = {
	{NULL,			NULL,			0,	"inherit"},
	{NULL,			NULL,			0,	"on"},
	{NULL,			NULL,			0,	"uncompressed"},
	{lzjb_compress,		lzjb_decompress,	0,	"lzjb"},
	{NULL,			NULL,			0,	"empty"},
	{gzip_compress,		gzip_decompress,	1,	"gzip-1"},
	{gzip_compress,		gzip_decompress,	2,	"gzip-2"},
	{gzip_compress,		gzip_decompress,	3,	"gzip-3"},
	{gzip_compress,		gzip_decompress,	4,	"gzip-4"},
	{gzip_compress,		gzip_decompress,	5,	"gzip-5"},
	{gzip_compress,		gzip_decompress,	6,	"gzip-6"},
	{gzip_compress,		gzip_decompress,	7,	"gzip-7"},
	{gzip_compress,		gzip_decompress,	8,	"gzip-8"},
	{gzip_compress,		gzip_decompress,	9,	"gzip-9"},
	{zle_compress,		zle_decompress,		64,	"zle"},
	{lz4_compress,		lz4_decompress,		0,	"lz4"},
};

enum zio_compress
zio_compress_select(spa_t *spa, enum zio_compress child,
    enum zio_compress parent)
{
	enum zio_compress result;

	ASSERT(child < ZIO_COMPRESS_FUNCTIONS);
	ASSERT(parent < ZIO_COMPRESS_FUNCTIONS);
	ASSERT(parent != ZIO_COMPRESS_INHERIT);

	result = child;
	if (result == ZIO_COMPRESS_INHERIT)
		result = parent;

	if (result == ZIO_COMPRESS_ON) {
		if (spa_feature_is_active(spa, SPA_FEATURE_LZ4_COMPRESS))
			result = ZIO_COMPRESS_LZ4_ON_VALUE;
		else
			result = ZIO_COMPRESS_LEGACY_ON_VALUE;
	}

	return (result);
}

size_t
zio_compress_data(enum zio_compress c, void *src, void *dst, size_t s_len)
{
	uint64_t *word, *word_end;
	size_t c_len, d_len;
	zio_compress_info_t *ci = &zio_compress_table[c];

	ASSERT((uint_t)c < ZIO_COMPRESS_FUNCTIONS);
	ASSERT((uint_t)c == ZIO_COMPRESS_EMPTY || ci->ci_compress != NULL);

	ZCOMPSTAT_BUMP(zcompstat_attempts);

	/*
	 * If the data is all zeroes, we don't even need to allocate
	 * a block for it.  We indicate this by returning zero size.
	 */
	word_end = (uint64_t *)((char *)src + s_len);
	for (word = src; word < word_end; word++)
		if (*word != 0)
			break;

	if (word == word_end) {
		ZCOMPSTAT_BUMP(zcompstat_empty);
 		return (0);
	}

	if (c == ZIO_COMPRESS_EMPTY)
		return (s_len);

	/* Compress at least 12.5% */
	d_len = s_len - (s_len >> 3);
	c_len = ci->ci_compress(src, dst, s_len, d_len, ci->ci_level);

	if (c_len > d_len) {
		ZCOMPSTAT_BUMP(zcompstat_skipped_insufficient_gain);
		return (s_len);
	}

	ASSERT3U(c_len, <=, d_len);
	return (c_len);
}

int
zio_decompress_data(enum zio_compress c, void *src, void *dst,
    size_t s_len, size_t d_len)
{
	zio_compress_info_t *ci = &zio_compress_table[c];

	if ((uint_t)c >= ZIO_COMPRESS_FUNCTIONS || ci->ci_decompress == NULL)
		return (SET_ERROR(EINVAL));

	return (ci->ci_decompress(src, dst, s_len, d_len, ci->ci_level));
}

void
zio_compress_init(void)
{

	zcomp_ksp = kstat_create("zfs", 0, "zcompstats", "misc",
	    KSTAT_TYPE_NAMED, sizeof (zcomp_stats) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);

	if (zcomp_ksp != NULL) {
		zcomp_ksp->ks_data = &zcomp_stats;
		kstat_install(zcomp_ksp);
	}
}

void
zio_compress_fini(void)
{
	if (zcomp_ksp != NULL) {
		kstat_delete(zcomp_ksp);
		zcomp_ksp = NULL;
	}
}
