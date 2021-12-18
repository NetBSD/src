/*	$NetBSD: mock_dmabuf.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2016 Intel Corporation
 */

#ifndef __MOCK_DMABUF_H__
#define __MOCK_DMABUF_H__

#include <linux/dma-buf.h>

struct mock_dmabuf {
	int npages;
	struct page *pages[];
};

static inline struct mock_dmabuf *to_mock(struct dma_buf *buf)
{
	return buf->priv;
}

#endif /* !__MOCK_DMABUF_H__ */
