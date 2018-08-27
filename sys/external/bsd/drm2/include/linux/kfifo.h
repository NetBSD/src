/*	$NetBSD: kfifo.h,v 1.2 2018/08/27 14:00:26 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef	_LINUX_KFIFO_H_
#define	_LINUX_KFIFO_H_

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/mutex.h>

#include <linux/gfp.h>
#include <linux/slab.h>

struct kfifo_meta {
	kmutex_t	kfm_lock;
	size_t		kfm_head;
	size_t		kfm_tail;
	size_t		kfm_nbytes;
};

#define	_KFIFO_PTR_TYPE(TAG, TYPE)					      \
	struct TAG {							      \
		struct kfifo_meta	kf_meta;			      \
		TYPE			*kf_buf;			      \
	}

#define	DECLARE_KFIFO_PTR(FIFO, TYPE)	_KFIFO_PTR_TYPE(, TYPE) FIFO

_KFIFO_PTR_TYPE(kfifo, void);

#define	kfifo_alloc(FIFO, SIZE, GFP)					      \
	_kfifo_alloc(&(FIFO)->kf_meta, &(FIFO)->kf_buf, (SIZE), (GFP))

static inline int
_kfifo_alloc(struct kfifo_meta *meta, void *bufp, size_t nbytes, gfp_t gfp)
{
	void *buf;

	buf = kmalloc(nbytes, gfp);
	if (buf == NULL)
		return -ENOMEM;

	/* Type pun!  Hope void * == struct whatever *.  */
	memcpy(bufp, &buf, sizeof(void *));

	mutex_init(&meta->kfm_lock, MUTEX_DEFAULT, IPL_NONE);
	meta->kfm_head = 0;
	meta->kfm_tail = 0;
	meta->kfm_nbytes = nbytes;

	return 0;
}

#define	kfifo_free(FIFO)						      \
	_kfifo_free(&(FIFO)->kf_meta, &(FIFO)->kf_buf)

static inline void
_kfifo_free(struct kfifo_meta *meta, void *bufp)
{
	void *buf;

	mutex_destroy(&meta->kfm_lock);

	memcpy(&buf, bufp, sizeof(void *));
	kfree(buf);

	/* Paranoia.  */
	buf = NULL;
	memcpy(bufp, &buf, sizeof(void *));
}

#define	kfifo_is_empty(FIFO)	(kfifo_len(FIFO) == 0)
#define	kfifo_len(FIFO)		_kfifo_len(&(FIFO)->kf_meta)

static inline size_t
_kfifo_len(struct kfifo_meta *meta)
{
	const size_t head = meta->kfm_head;
	const size_t tail = meta->kfm_tail;
	const size_t nbytes = meta->kfm_nbytes;

	return (head <= tail ? tail - head : nbytes + tail - head);
}

#define	kfifo_out_peek(FIFO, PTR, SIZE)					      \
	_kfifo_out_peek(&(FIFO)->kf_meta, (FIFO)->kf_buf, (PTR), (SIZE))

static inline size_t
_kfifo_out_peek(struct kfifo_meta *meta, void *buf, void *ptr, size_t size)
{
	const char *src = buf;
	char *dst = ptr;
	size_t copied = 0;

	mutex_enter(&meta->kfm_lock);
	const size_t head = meta->kfm_head;
	const size_t tail = meta->kfm_tail;
	const size_t nbytes = meta->kfm_nbytes;
	if (head <= tail) {
		if (size <= tail - head) {
			memcpy(dst, src + head, size);
			copied = size;
		}
	} else {
		if (size <= nbytes - head) {
			memcpy(dst, src + head, size);
			copied = size;
		} else if (size <= nbytes + tail - head) {
			memcpy(dst, src + head, nbytes - head);
			memcpy(dst + nbytes - head, src,
			    size - (nbytes - head));
			copied = size;
		}
	}
	mutex_exit(&meta->kfm_lock);

	return copied;
}

#define	kfifo_out(FIFO, PTR, SIZE)					      \
	_kfifo_out(&(FIFO)->kf_meta, (FIFO)->kf_buf, (PTR), (SIZE))

static inline size_t
_kfifo_out(struct kfifo_meta *meta, const void *buf, void *ptr, size_t size)
{
	const char *src = buf;
	char *dst = ptr;
	size_t copied = 0;

	mutex_enter(&meta->kfm_lock);
	const size_t head = meta->kfm_head;
	const size_t tail = meta->kfm_tail;
	const size_t nbytes = meta->kfm_nbytes;
	if (head <= tail) {
		if (size <= tail - head) {
			memcpy(dst, src + head, size);
			meta->kfm_head = head + size;
			copied = size;
		}
	} else {
		if (size <= nbytes - head) {
			memcpy(dst, src + head, size);
			meta->kfm_head = head + size;
			copied = size;
		} else if (size <= nbytes + tail - head) {
			memcpy(dst, src + head, nbytes - head);
			memcpy(dst + nbytes - head, src,
			    size - (nbytes - head));
			meta->kfm_head = size - (nbytes - head);
			copied = size;
		}
	}
	mutex_exit(&meta->kfm_lock);

	return copied;
}

#define	kfifo_in(FIFO, PTR, SIZE)					      \
	_kfifo_in(&(FIFO)->kf_meta, (FIFO)->kf_buf, (PTR), (SIZE))

static inline size_t
_kfifo_in(struct kfifo_meta *meta, void *buf, const void *ptr, size_t size)
{
	const char *src = ptr;
	char *dst = buf;
	size_t copied = 0;

	mutex_enter(&meta->kfm_lock);
	const size_t head = meta->kfm_head;
	const size_t tail = meta->kfm_tail;
	const size_t nbytes = meta->kfm_nbytes;
	if (tail <= head) {
		if (size <= head - tail) {
			memcpy(dst + tail, src, size);
			meta->kfm_tail = tail + size;
			copied = size;
		}
	} else {
		if (size <= nbytes - tail) {
			memcpy(dst + tail, src, size);
			meta->kfm_tail = tail + size;
		} else if (size <= nbytes + tail - head) {
			memcpy(dst + tail, src, nbytes - tail);
			memcpy(dst, src + nbytes - tail,
			    size - (nbytes - tail));
			meta->kfm_tail = size - (nbytes - tail);
			copied = size;
		}
	}
	mutex_exit(&meta->kfm_lock);

	return copied;
}

#endif	/* _LINUX_KFIFO_H_ */
