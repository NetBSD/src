/* $NetBSD: buf.h,v 1.1 2022/01/22 07:54:57 pho Exp $ */

/*
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#if !defined(_FUSE_BUF_H_)
#define _FUSE_BUF_H_

#include <sys/types.h>

/* Data buffer API, appeared on FUSE 2.9. */

#if !defined(FUSE_H_)
#  error Do not include this header directly. Include <fuse.h> instead.
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum fuse_buf_flags {
	/* The buffer is actually an fd: .mem is invalid but .fd and
	 * .pos have valid values. */
	FUSE_BUF_IS_FD		= (1 << 1),
	/* The fd is seekable. */
	FUSE_BUF_FD_SEEK	= (1 << 2),
	/* Keep reading from/writing to the fd until reaching EOF. */
	FUSE_BUF_FD_RETRY	= (1 << 3),
};
enum fuse_buf_copy_flags {
	/* These flags are always ignored because splice(2) is a
	 * Linux-specific syscall and there are no alternatives on
	 * NetBSD atm. */
	FUSE_BUF_NO_SPLICE		= (1 << 1),
	FUSE_BUF_FORCE_SPLICE		= (1 << 2),
	FUSE_BUF_SPLICE_MOVE		= (1 << 3),
	FUSE_BUF_SPLICE_NONBLOCK	= (1 << 4),
};
struct fuse_buf {
	size_t			size;
	enum fuse_buf_flags	flags;
	void			*mem;
	int			fd;
	off_t			pos;
};
struct fuse_bufvec {
	/* The number of buffers in the vector. */
	size_t		count;
	/* The index of the current buffer in the vector. */
	size_t		idx;
	/* The current offset in the current buffer. */
	size_t		off;
	/* The vector of buffers. */
	struct fuse_buf	buf[1];
};
#define FUSE_BUFVEC_INIT(size_)				\
	((struct fuse_bufvec) {				\
		/* .count = */ 1,			\
		/* .idx = */ 0,				\
		/* .off = */ 0,				\
		/* .buf = */ {				\
			/* [0] = */ {			\
				/* .size = */ (size_)	\
				/* .flags = */ (enum fuse_buf_flags)0,	\
				/* .mem = */ NULL,	\
				/* .fd = */ -1,		\
				/* .pos = */ 0,		\
			}				\
		}					\
	})

/* Get the total size of data in a buffer vector. */
size_t fuse_buf_size(const struct fuse_bufvec *bufv);

/* Copy data from one buffer vector to another, and return the number
 * of octets that have been copied. */
ssize_t fuse_buf_copy(struct fuse_bufvec *dstv, struct fuse_bufvec *srcv,
					  enum fuse_buf_copy_flags flags);

#ifdef __cplusplus
}
#endif

#endif
