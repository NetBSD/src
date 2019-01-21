/*	$NetBSD: cpufunc.c,v 1.1 2019/01/21 00:30:14 alnsn Exp $	*/

/*-
 * Copyright (c) 2019 Alexander Nasonov.
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
__KERNEL_RCSID(0, "$NetBSD: cpufunc.c,v 1.1 2019/01/21 00:30:14 alnsn Exp $");

/*
 * Barebone implementation of arm cpufunc routines for rump.
 */

#include <machine/types.h>
#include <sys/types.h>

#include "sljit_rump.h"

void aarch64_icache_sync_range(vaddr_t, vsize_t);

void
aarch64_icache_sync_range(vaddr_t va, vsize_t sz)
{

	// XXX MIPS and 32-bit ARM make this call:
	// (void)rumpcomp_sync_icache((void *)va, (uint64_t)sz);
	// but it doesn't link. Fix Makefiles to make it link.
	__builtin___clear_cache((void *)va, (char *)va + sz);
}
