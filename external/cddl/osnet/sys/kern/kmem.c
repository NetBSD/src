/*	$NetBSD: kmem.c,v 1.1.2.2 2018/06/25 07:25:25 pgoyette Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
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

#include <sys/kmem.h>

#undef kmem_alloc
#undef kmem_zalloc
#undef kmem_free

/*
 * Solaris allows allocating zero bytes (which returns NULL)
 * and freeing a NULL pointer (which does nothing),
 * so allow that here with wrappers around the native functions
 * (which do not allow those things).
 */

void *
solaris_kmem_alloc(size_t size, int flags)
{

	if (size == 0)
		return NULL;
	return kmem_alloc(size, flags);
}

void *
solaris_kmem_zalloc(size_t size, int flags)
{

	if (size == 0)
		return NULL;
	return kmem_zalloc(size, flags);
}

void
solaris_kmem_free(void *buf, size_t size)
{

	if (buf == NULL)
		return;
	kmem_free(buf, size);
}
