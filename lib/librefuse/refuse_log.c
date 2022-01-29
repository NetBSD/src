/* $NetBSD: refuse_log.c,v 1.2 2022/01/29 00:03:41 tnn Exp $ */

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
__RCSID("$NetBSD: refuse_log.c,v 1.2 2022/01/29 00:03:41 tnn Exp $");
#endif /* !lint */

#include <assert.h>
#include <fuse_log.h>
#if defined(MULTITHREADED_REFUSE)
#	include <pthread.h>
#endif
#include <stdio.h>

static void  __printflike(2, 0)
default_log_func(enum fuse_log_level level __attribute__((__unused__)),
                 const char *fmt, va_list ap) {
    /* This function needs to be thread-safe. Calling vfprintf(3)
     * should be okay because POSIX mandates locking FILE* objects
     * internally. */
    vfprintf(stderr, fmt, ap);
}

#if defined(MULTITHREADED_REFUSE)
static pthread_mutex_t	log_func_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static fuse_log_func_t	log_func = default_log_func;

void
fuse_set_log_func(fuse_log_func_t func) {
#if defined(MULTITHREADED_REFUSE)
    /* What we really need here is merely a memory barrier, but
     * locking a mutex is the easiest way to achieve that. */
    int rv;

    rv = pthread_mutex_lock(&log_func_mutex);
    assert(rv == 0);
#endif

    if (func)
        log_func = func;
    else
        log_func = default_log_func;

#if defined(MULTITHREADED_REFUSE)
    rv = pthread_mutex_unlock(&log_func_mutex);
    assert(rv == 0);
#endif
}

void
fuse_log(enum fuse_log_level level, const char *fmt, ...) {
    va_list ap;
#if defined(MULTITHREADED_REFUSE)
    /* What we really need here is merely a memory barrier, but
     * locking a mutex is the easiest way to achieve that. */
    int rv;

    rv = pthread_mutex_lock(&log_func_mutex);
    assert(rv == 0);
#endif

    va_start(ap, fmt);
    log_func(level, fmt, ap);
    va_end(ap);

#if defined(MULTITHREADED_REFUSE)
    rv = pthread_mutex_unlock(&log_func_mutex);
    assert(rv == 0);
#endif
}
