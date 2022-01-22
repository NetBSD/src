/* $NetBSD: chan.h,v 1.1 2022/01/22 08:09:40 pho Exp $ */

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
#if !defined(_FUSE_CHAN_H_)
#define _FUSE_CHAN_H_

/*
 * Fuse communication channel API, appeared on FUSE 2.4.
 */

#if !defined(FUSE_H_)
#  error Do not include this header directly. Include <fuse.h> instead.
#endif

#include <stdbool.h>
#include <stddef.h>
#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* An opaque object representing a communication channel. */
struct fuse_chan;

/* Implementation details. User code should never call these functions
 * directly. */
struct fuse;
struct fuse_args;
__BEGIN_HIDDEN_DECLS
struct fuse_chan* fuse_chan_new(const char* mountpoint, const struct fuse_args* args);
void              fuse_chan_destroy(struct fuse_chan* chan);

int               fuse_chan_stash(struct fuse_chan* chan);
struct fuse_chan* fuse_chan_peek(int idx);
struct fuse_chan* fuse_chan_take(int idx);
struct fuse_chan* fuse_chan_find(bool (*pred)(struct fuse_chan* chan, void* priv),
                                 int* found_idx, void* priv);

void              fuse_chan_set_fuse(struct fuse_chan* chan, struct fuse* fuse);
void              fuse_chan_set_to_be_destroyed(struct fuse_chan* chan, bool is_to_be_destroyed);

const char*       fuse_chan_mountpoint(const struct fuse_chan* chan);
struct fuse_args* fuse_chan_args(struct fuse_chan* chan);
struct fuse*      fuse_chan_fuse(struct fuse_chan* chan);
bool              fuse_chan_is_to_be_destroyed(const struct fuse_chan* chan);
__END_HIDDEN_DECLS

#ifdef __cplusplus
}
#endif

#endif
