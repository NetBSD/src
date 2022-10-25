/*	$NetBSD: dmapool.h,v 1.1 2022/10/25 23:32:37 riastradh Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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

#ifndef	_LINUX_DMAPOOL_H_
#define	_LINUX_DMAPOOL_H_

#include <sys/types.h>

#include <sys/bus.h>

#include <linux/gfp.h>
#include <linux/types.h>

/* namespace */
#define	dma_pool_create		linux_dma_pool_create
#define	dma_pool_destroy	linux_dma_pool_destroy
#define	dma_pool_free		linux_dma_pool_free
#define	dma_pool_zalloc		linux_dma_pool_zalloc

struct dma_pool;

struct dma_pool *dma_pool_create(const char *, bus_dma_tag_t, size_t, size_t,
    size_t);
void dma_pool_destroy(struct dma_pool *);

void *dma_pool_zalloc(struct dma_pool *, gfp_t, dma_addr_t *);
void dma_pool_free(struct dma_pool *, void *, dma_addr_t);

#endif	/* _LINUX_DMAPOOL_H_ */
