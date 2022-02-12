/*	$NetBSD: pthread_makelwp_netbsd.c,v 1.3 2022/02/12 14:59:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2003, 2006, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams and Andrew Doran.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_makelwp_netbsd.c,v 1.3 2022/02/12 14:59:32 riastradh Exp $");

/* Need to use libc-private names for atomic operations. */
#include "../../common/lib/libc/atomic/atomic_op_namespace.h"

#include <sys/param.h>

#include <lwp.h>
#include <pthread.h>
#include <string.h>

#include "pthread_int.h"
#include "pthread_makelwp.h"

int
pthread__makelwp(void (*start_routine)(void *), void *arg, void *priv,
	void *stack_base, size_t stack_size,
	unsigned long flag, lwpid_t *newlid)
{
	ucontext_t uc;

	/*
	 * XXX: most of the following chunk is these days
	 * performed also by _lwp_makecontext(), but there may
	 * be MD differences, so lug everything along for now.
	 */
	memset(&uc, 0, sizeof(uc));
	_INITCONTEXT_U(&uc);
	uc.uc_stack.ss_sp = stack_base;
	uc.uc_stack.ss_size = stack_size;
	uc.uc_stack.ss_flags = 0;
	uc.uc_link = NULL;

	_lwp_makecontext(&uc, start_routine, arg, priv, stack_base, stack_size);
	return _lwp_create(&uc, flag, newlid);
}
