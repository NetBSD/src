/*	$NetBSD: dlz_pthread.h,v 1.4 2022/09/23 12:15:28 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: ISC
 *
 * Permission to use, copy, modify, and distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright
 * notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DLZ_PTHREAD_H
#define DLZ_PTHREAD_H 1

#ifndef PTHREADS
#define PTHREADS 1
#endif /* ifndef PTHREADS */

#ifdef PTHREADS
#include <pthread.h>
#define dlz_mutex_t	  pthread_mutex_t
#define dlz_mutex_init	  pthread_mutex_init
#define dlz_mutex_destroy pthread_mutex_destroy
#define dlz_mutex_lock	  pthread_mutex_lock
#define dlz_mutex_trylock pthread_mutex_trylock
#define dlz_mutex_unlock  pthread_mutex_unlock
#else /* !PTHREADS */
#define dlz_mutex_t	     void
#define dlz_mutex_init(a, b) (0)
#define dlz_mutex_destroy(a) (0)
#define dlz_mutex_lock(a)    (0)
#define dlz_mutex_trylock(a) (0)
#define dlz_mutex_unlock(a)  (0)
#endif /* ifdef PTHREADS */

#endif /* DLZ_PTHREAD_H */
