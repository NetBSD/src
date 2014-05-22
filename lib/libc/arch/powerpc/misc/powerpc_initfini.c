/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__RCSID("$NetBSD: powerpc_initfini.c,v 1.1.4.2 2014/05/22 11:36:48 yamt Exp $");

#include "namespace.h"

/*
 * Grab the cache_info at load time.
 */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/queue.h>
#include <sys/cpu.h>

#include <stdbool.h>
#include <stddef.h>

__dso_hidden struct cache_info _libc_powerpc_cache_info;

static void _libc_cache_info_init(void)
    __attribute__((__constructor__, __used__));

void __section(".text.startup")
_libc_cache_info_init(void)
{
	static bool initialized;
	if (!initialized) {
		const int name[2] = { CTL_MACHDEP, CPU_CACHEINFO };
		size_t len = sizeof(_libc_powerpc_cache_info);
		(void)sysctl(name, __arraycount(name),
		    &_libc_powerpc_cache_info, &len, NULL, 0);
		initialized = 1;
	}
}
