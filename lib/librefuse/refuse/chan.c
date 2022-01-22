/* $NetBSD: chan.c,v 1.1 2022/01/22 08:09:40 pho Exp $ */

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
__RCSID("$NetBSD: chan.c,v 1.1 2022/01/22 08:09:40 pho Exp $");
#endif /* !lint */

#include <assert.h>
#include <err.h>
#include <fuse_internal.h>
#if defined(MULTITHREADED_REFUSE)
#  include <pthread.h>
#endif
#include <stdlib.h>
#include <string.h>

/* The communication channel API is a part of the public interface of
 * FUSE. However, it is actually an implementation detail and we can't
 * really emulate its semantics. We only use it for implementing
 * pre-3.0 fuse_mount(), i.e. the mount-before-new thingy.
 */

struct fuse_chan {
    char* mountpoint;
    struct fuse_args* args;
    struct fuse* fuse;
    bool is_to_be_destroyed;
};

struct refuse_chan_storage {
    size_t n_alloc;
    struct fuse_chan** vec;
};

#if defined(MULTITHREADED_REFUSE)
static pthread_mutex_t storage_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static struct refuse_chan_storage storage;


struct fuse_chan* fuse_chan_new(const char* mountpoint, const struct fuse_args* args) {
    struct fuse_chan* chan;

    chan = calloc(1, sizeof(*chan));
    if (!chan) {
        warn("%s", __func__);
        return NULL;
    }

    chan->mountpoint = strdup(mountpoint);
    if (!chan->mountpoint) {
        warn("%s", __func__);
        free(chan);
        return NULL;
    }

    chan->args = fuse_opt_deep_copy_args(args->argc, args->argv);
    if (!chan->args) {
        warn("%s", __func__);
        free(chan->mountpoint);
        free(chan);
        return NULL;
    }

    return chan;
}

void
fuse_chan_destroy(struct fuse_chan* chan) {
    free(chan->mountpoint);
    fuse_opt_free_args(chan->args);
    free(chan->args);
    free(chan);
}

int
fuse_chan_stash(struct fuse_chan* chan) {
    int idx;
#if defined(MULTITHREADED_REFUSE)
    int rv;

    rv = pthread_mutex_lock(&storage_mutex);
    assert(rv == 0);
#endif

    /* Find the first empty slot in the storage. */
    for (idx = 0; idx < (int)storage.n_alloc; idx++) {
        if (storage.vec[idx] == NULL) {
            storage.vec[idx] = chan;
            goto done;
        }
    }

    /* Allocate more space */
    storage.n_alloc = (storage.n_alloc + 8) * 2;
    storage.vec     = realloc(storage.vec, sizeof(struct fuse_chan*) * storage.n_alloc);
    if (!storage.vec) {
        warn("%s", __func__);
        idx = -1;
        goto done;
    }

    storage.vec[idx] = chan;
    memset(&storage.vec[idx+1], 0, sizeof(struct fuse_chan*) * (storage.n_alloc - (size_t)idx - 1));

  done:
#if defined(MULTITHREADED_REFUSE)
    rv = pthread_mutex_unlock(&storage_mutex);
    assert(rv == 0);
#endif
    return idx;
}

/* Acquire a pointer to a stashed channel with a given index. */
struct fuse_chan* fuse_chan_peek(int idx) {
    struct fuse_chan* chan = NULL;
#if defined(MULTITHREADED_REFUSE)
    int rv;

    rv = pthread_mutex_lock(&storage_mutex);
    assert(rv == 0);
#endif

    if (idx >= 0 && idx < (int)storage.n_alloc) {
        chan = storage.vec[idx];
    }

#if defined(MULTITHREADED_REFUSE)
    rv = pthread_mutex_unlock(&storage_mutex);
    assert(rv == 0);
#endif
    return chan;
}

/* Like fuse_chan_peek() but also removes the channel from the
 * storage. */
struct fuse_chan* fuse_chan_take(int idx) {
    struct fuse_chan* chan = NULL;
#if defined(MULTITHREADED_REFUSE)
    int rv;

    rv = pthread_mutex_lock(&storage_mutex);
    assert(rv == 0);
#endif

    if (idx >= 0 && idx < (int)storage.n_alloc) {
        chan = storage.vec[idx];
        storage.vec[idx] = NULL;
    }

#if defined(MULTITHREADED_REFUSE)
    rv = pthread_mutex_unlock(&storage_mutex);
    assert(rv == 0);
#endif
    return chan;
}

/* Find the first stashed channel satisfying a given predicate in the
 * storage, or NULL if no channels satisfy it. */
struct fuse_chan*
fuse_chan_find(bool (*pred)(struct fuse_chan*, void*),
               int* found_idx, void* priv) {
    int idx;
    struct fuse_chan* chan = NULL;
#if defined(MULTITHREADED_REFUSE)
    int rv;

    rv = pthread_mutex_lock(&storage_mutex);
    assert(rv == 0);
#endif

    for (idx = 0; idx < (int)storage.n_alloc; idx++) {
        if (storage.vec[idx] != NULL) {
            if (pred(storage.vec[idx], priv)) {
                chan = storage.vec[idx];
                if (found_idx)
                    *found_idx = idx;
                goto done;
            }
        }
    }

  done:
#if defined(MULTITHREADED_REFUSE)
    rv = pthread_mutex_unlock(&storage_mutex);
    assert(rv == 0);
#endif
    return chan;
}

void
fuse_chan_set_fuse(struct fuse_chan* chan, struct fuse* fuse) {
    chan->fuse = fuse;
}

void
fuse_chan_set_to_be_destroyed(struct fuse_chan* chan, bool is_to_be_destroyed) {
    chan->is_to_be_destroyed = is_to_be_destroyed;
}

const char*
fuse_chan_mountpoint(const struct fuse_chan* chan) {
    return chan->mountpoint;
}

struct fuse_args*
fuse_chan_args(struct fuse_chan* chan) {
    return chan->args;
}

struct fuse*
fuse_chan_fuse(struct fuse_chan* chan) {
    return chan->fuse;
}

bool
fuse_chan_is_to_be_destroyed(const struct fuse_chan* chan) {
    return chan->is_to_be_destroyed;
}
