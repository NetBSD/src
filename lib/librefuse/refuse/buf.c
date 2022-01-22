/* $NetBSD: buf.c,v 1.2 2022/01/22 13:25:55 pho Exp $ */

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

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: buf.c,v 1.2 2022/01/22 13:25:55 pho Exp $");
#endif /* !lint */

#include <assert.h>
#include <errno.h>
#include <fuse_internal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h> /* for MIN(a, b) */
#include <unistd.h>

size_t
fuse_buf_size(const struct fuse_bufvec *bufv) {
    size_t i;
    size_t total = 0;

    for (i = 0; i < bufv->count; i++) {
        total += bufv->buf[i].size;
    }

    return total;
}

/* Return the pointer to the current buffer in a buffer vector, or
 * NULL if we have reached the end of the vector. */
static const struct fuse_buf*
fuse_buf_current(const struct fuse_bufvec *bufv) {
    if (bufv->idx < bufv->count)
        return &bufv->buf[bufv->idx];
    else
        return NULL;
}

/* Copy data from one fd to a memory buffer, and return the number of
 * octets that have been copied, or -1 on failure. */
static ssize_t
fuse_buf_read_fd_to_mem(const struct fuse_buf *dst, size_t dst_off,
                        const struct fuse_buf *src, size_t src_off,
                        size_t len) {
    ssize_t total = 0;

    while (len > 0) {
        ssize_t n_read;

        if (src->flags & FUSE_BUF_FD_SEEK)
            n_read = pread(src->fd, (uint8_t*)dst->mem + dst_off, len,
                           src->pos + (off_t)src_off);
        else
            n_read = read(src->fd, (uint8_t*)dst->mem + dst_off, len);

        if (n_read == -1) {
            if (errno == EINTR)
                continue;
            else if (total == 0)
                return -1;
            else
                /* The last pread(2) or read(2) failed but we have
                 * already copied some data. */
                break;
        }
        else if (n_read == 0) {
            /* Reached EOF */
            break;
        }
        else {
            total   += n_read;
            dst_off += (size_t)n_read;
            src_off += (size_t)n_read;
            len     -= (size_t)n_read;

            if (src->flags & FUSE_BUF_FD_RETRY)
                continue;
        }
    }

    return total;
}

/* Copy data from one memory buffer to an fd, and return the number of
 * octets that have been copied, or -1 on failure. */
static ssize_t
fuse_buf_write_mem_to_fd(const struct fuse_buf *dst, size_t dst_off,
                         const struct fuse_buf *src, size_t src_off,
                         size_t len) {
    ssize_t total = 0;

    while (len > 0) {
        ssize_t n_wrote;

        if (dst->flags & FUSE_BUF_FD_SEEK)
            n_wrote = pwrite(dst->fd, (uint8_t*)src->mem + src_off, len,
                             dst->pos + (off_t)dst_off);
        else
            n_wrote = write(dst->fd, (uint8_t*)src->mem + src_off, len);

        if (n_wrote == -1) {
            if (errno == EINTR)
                continue;
            else if (total == 0)
                return -1;
            else
                /* The last pwrite(2) or write(2) failed but we have
                 * already copied some data. */
                break;
        }
        else if (n_wrote == 0) {
            break;
        }
        else {
            total   += n_wrote;
            dst_off += (size_t)n_wrote;
            src_off += (size_t)n_wrote;
            len     -= (size_t)n_wrote;

            if (dst->flags & FUSE_BUF_FD_RETRY)
                continue;
        }
    }

    return total;
}

/* Copy data from one fd to another, and return the number of octets
 * that have been copied, or -1 on failure. */
static ssize_t
fuse_buf_copy_fd_to_fd(const struct fuse_buf *dst, size_t dst_off,
                       const struct fuse_buf *src, size_t src_off,
                       size_t len) {
    ssize_t total = 0;
    struct fuse_buf tmp;

    tmp.size  = (size_t)sysconf(_SC_PAGESIZE);
    tmp.flags = (enum fuse_buf_flags)0;
    tmp.mem   = malloc(tmp.size);

    if (tmp.mem == NULL) {
        return -1;
    }

    while (len) {
        size_t n_to_read = MIN(tmp.size, len);
        ssize_t n_read;
        ssize_t n_wrote;

        n_read = fuse_buf_read_fd_to_mem(&tmp, 0, src, src_off, n_to_read);
        if (n_read == -1) {
            if (total == 0) {
                free(tmp.mem);
                return -1;
            }
            else {
                /* We have already copied some data. */
                break;
            }
        }
        else if (n_read == 0) {
            /* Reached EOF */
            break;
        }

        n_wrote = fuse_buf_write_mem_to_fd(dst, dst_off, &tmp, 0, (size_t)n_read);
        if (n_wrote == -1) {
            if (total == 0) {
                free(tmp.mem);
                return -1;
            }
            else {
                /* We have already copied some data. */
                break;
            }
        }
        else if (n_wrote == 0) {
            break;
        }

        total   += n_wrote;
        dst_off += (size_t)n_wrote;
        src_off += (size_t)n_wrote;
        len     -= (size_t)n_wrote;

        if (n_wrote < n_read)
            /* Now we have some data that were read but couldn't be
             * written, and can't do anything about it. */
            break;
    }

    free(tmp.mem);
    return total;
}

/* Copy data from one buffer to another, and return the number of
 * octets that have been copied. */
static ssize_t
fuse_buf_copy_one(const struct fuse_buf *dst, size_t dst_off,
                  const struct fuse_buf *src, size_t src_off,
                  size_t len,
                  enum fuse_buf_copy_flags flags __attribute__((__unused__))) {

    const bool dst_is_fd = !!(dst->flags & FUSE_BUF_IS_FD);
    const bool src_is_fd = !!(src->flags & FUSE_BUF_IS_FD);

    if (!dst_is_fd && !src_is_fd) {
        void* dst_mem = (uint8_t*)dst->mem + dst_off;
        void* src_mem = (uint8_t*)src->mem + src_off;

        memmove(dst_mem, src_mem, len);

        return (ssize_t)len;
    }
    else if (!dst_is_fd) {
        return fuse_buf_read_fd_to_mem(dst, dst_off, src, src_off, len);
    }
    else if (!src_is_fd) {
        return fuse_buf_write_mem_to_fd(dst, dst_off, src, src_off, len);
    }
    else {
        return fuse_buf_copy_fd_to_fd(dst, dst_off, src, src_off, len);
    }
}

/* Advance the buffer by a given number of octets. Return 0 on
 * success, or -1 otherwise. Reaching the end of the buffer vector
 * counts as a failure. */
static int
fuse_buf_advance(struct fuse_bufvec *bufv, size_t len) {
    const struct fuse_buf *buf = fuse_buf_current(bufv);

    assert(bufv->off + len <= buf->size);
    bufv->off += len;
    if (bufv->off == buf->size) {
        /* Done with the current buffer. Advance to the next one. */
        assert(bufv->idx < bufv->count);
        bufv->idx++;
        if (bufv->idx == bufv->count)
            return -1; /* No more buffers in the vector. */
        bufv->off = 0;
    }
    return 0;
}

ssize_t
fuse_buf_copy(struct fuse_bufvec *dstv, struct fuse_bufvec *srcv,
                      enum fuse_buf_copy_flags flags) {
    ssize_t total = 0;

    while (true) {
        const struct fuse_buf* dst;
        const struct fuse_buf* src;
        size_t src_len;
        size_t dst_len;
        size_t len;
        ssize_t n_copied;

        dst = fuse_buf_current(dstv);
        src = fuse_buf_current(srcv);
        if (src == NULL || dst == NULL)
            break;

        src_len = src->size - srcv->off;
        dst_len = dst->size - dstv->off;
        len = MIN(src_len, dst_len);

        n_copied = fuse_buf_copy_one(dst, dstv->off, src, srcv->off, len, flags);
        if (n_copied == -1) {
            if (total == 0)
                return -1;
            else
                /* Failed to copy the current buffer but we have
                 * already copied some part of the vector. It is
                 * therefore inappropriate to return an error. */
                break;
        }
        total += n_copied;

        if (fuse_buf_advance(srcv, (size_t)n_copied) != 0 ||
            fuse_buf_advance(dstv, (size_t)n_copied) != 0)
            break;

        if ((size_t)n_copied < len)
            break;
    }

    return total;
}
