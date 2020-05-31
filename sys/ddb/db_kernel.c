/*	$NetBSD: db_kernel.c,v 1.4 2020/05/31 23:34:34 rin Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
__KERNEL_RCSID(0, "$NetBSD: db_kernel.c,v 1.4 2020/05/31 23:34:34 rin Exp $");

#include <sys/param.h>
#include <sys/kmem.h>

#include <ddb/ddb.h>

/*
 * XXX
 * These routines are *not* intended for a DDB session:
 *
 * - It disturbes on-going debugged kernel datastructures.
 *
 * - DDB can be running in the interrupt context, e.g., when activated from
 *   console. This results in assertion failures in the allocator.
 *
 * Use these only for permanent data storage when initializing DDB.
 */

void *
db_alloc(size_t sz)
{

	return kmem_alloc(sz, KM_SLEEP);
}

void *
db_zalloc(size_t sz)
{

	return kmem_zalloc(sz, KM_SLEEP);
}

void
db_free(void *p, size_t sz)
{

	kmem_free(p, sz);
}
