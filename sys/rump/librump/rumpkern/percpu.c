/*	$NetBSD: percpu.c,v 1.4.4.1 2009/05/13 17:22:57 jym Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: percpu.c,v 1.4.4.1 2009/05/13 17:22:57 jym Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/percpu.h>

#include "rump_private.h"

/*
 * A poor-man's userspace percpu emulation.  Since we can't disable
 * preemption currently, use a mutex.  Not the world's most efficient
 * method, but quite enough.  Hence, we can have only one cpu.
 */

static kmutex_t pcmtx;

void
percpu_init(void)
{

	mutex_init(&pcmtx, MUTEX_DEFAULT, IPL_NONE);
}

void *
percpu_getref(percpu_t *pc)
{

	mutex_enter(&pcmtx);
	return pc;
}

void
percpu_putref(percpu_t *pc)
{

	mutex_exit(&pcmtx);
}

percpu_t *
percpu_alloc(size_t size)
{

	return kmem_alloc(size, KM_SLEEP);
}

void
percpu_free(percpu_t *pc, size_t size)
{

	kmem_free(pc, size);
}

void
percpu_foreach(percpu_t *pc, percpu_callback_t cb, void *arg)
{

	cb(pc, arg, &rump_cpu);
}
