/*	$NetBSD: citrus_region.h,v 1.3 2003/07/21 07:10:28 thorpej Exp $	*/

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
 *
 */

#ifndef _CITRUS_REGION_H_
#define _CITRUS_REGION_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

struct _citrus_region {
/* private: */
	void	*r_head;
	size_t	r_size;
};

static __inline void
_citrus_region_init(struct _citrus_region *r, void *h, size_t sz)
{
	_DIAGASSERT(r);
	r->r_head = h;
	r->r_size = sz;
}

static __inline void *
_citrus_region_head(const struct _citrus_region *r)
{
	return r->r_head;
}

static __inline size_t
_citrus_region_size(const struct _citrus_region *r)
{
	return r->r_size;
}

static __inline int
_citrus_region_check(const struct _citrus_region *r, size_t ofs, size_t sz)
{
	return r->r_size >= ofs + sz ? 0 : -1;
}

static __inline void *
_citrus_region_offset(const struct _citrus_region *r, size_t pos)
{
	return (void *)((u_int8_t *)r->r_head + pos);
}

static __inline u_int8_t
_citrus_region_peek8(const struct _citrus_region *r, size_t pos)
{
	return *(u_int8_t *)_citrus_region_offset(r, pos);
}

static __inline u_int16_t
_citrus_region_peek16(const struct _citrus_region *r, size_t pos)
{
	u_int16_t val;
	memcpy(&val, _citrus_region_offset(r, pos), 2);
	return val;
}

static __inline u_int32_t
_citrus_region_peek32(const struct _citrus_region *r, size_t pos)
{
	u_int32_t val;
	memcpy(&val, _citrus_region_offset(r, pos), 4);
	return val;
}

#endif
