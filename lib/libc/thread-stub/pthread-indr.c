/*	$NetBSD: pthread-indr.c,v 1.1.2.3 2002/03/25 03:40:37 nathanw Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Indirect references for pthread stubs in pthread-stub.c.  This
 * is required for a.out, which doesn't have __weak_alias.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(_libc_pthread_mutex_init, pthread_mutex_init)
__indr_reference(_libc_pthread_mutex_lock, pthread_mutex_lock)
__indr_reference(_libc_pthread_mutex_unlock, pthread_mutex_unlock)
#if 0
__indr_reference(_libc_pthread_rwlock_init, pthread_rwlock_init)
__indr_reference(_libc_pthread_rwlock_rdlock, pthread_rwlock_rdlock)
__indr_reference(_libc_pthread_rwlock_wrlock, pthread_rwlock_wrlock)
__indr_reference(_libc_pthread_rwlock_unlock, pthread_rwlock_unlock)
#endif
__indr_reference(_flockfile, flockfile)
__indr_reference(_ftrylockfile, ftrylockfile)
__indr_reference(_funlockfile, funlockfile)
__indr_reference(_libc_pthread_once, pthread_once)
__indr_reference(_libc_pthread_cond_init, pthread_cond_init)
__indr_reference(_libc_pthread_cond_wait, pthread_cond_wait)
__indr_reference(_libc_pthread_cond_signal, pthread_cond_signal)
__indr_reference(_libc_pthread_spinlock, pthread_spinlock)
__indr_reference(_libc_pthread_spinunlock, pthread_spinunlock)
__indr_reference(_libc_pthread_key_create, pthread_key_create)
__indr_reference(_libc_pthread_setspecific, pthread_setspecific)
__indr_reference(_libc_pthread_getspecific, pthread_getspecific)
__indr_reference(_libc_pthread_self, pthread_self)
__indr_reference(_libc_pthread_sigmask, pthread_sigmask)
__indr_reference(_libc_pthread__errno, pthread__errno)
#endif
