/*	$NetBSD: compat.c,v 1.1.2.2 2018/10/20 06:58:22 pgoyette Exp $	*/
/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger.
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

/*
 * Early ELF support in NetBSD up to April 2nd, 2000 exposed dlopen and
 * friends via function pointers in the main object that is passed to the
 * startup code in crt0.o. Later versions kept the magic and version number
 * checks, but depend on normal symbol interposition to get the symbols from
 * rtld. The compatibility object build here contains just enough fields to
 * make either routine happy without polluting the rest of rtld.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: compat.c,v 1.1.2.2 2018/10/20 06:58:22 pgoyette Exp $");

#include "rtld.h"

#if defined(__alpha__)
#define RTLD_OBJ_DLOPEN_OFFSET 264
#elif defined(__i386__)
#define RTLD_OBJ_DLOPEN_OFFSET 140
#elif defined(__mips) && defined(_ABIO32)
/* The MIPS legacy support is required for O32 only, N32 is not affected. */
#define RTLD_OBJ_DLOPEN_OFFSET 152
#elif defined(__powerpc__) && !defined(__powerpc64__)
/* Only 32bit PowerPC is affected. */
#define RTLD_OBJ_DLOPEN_OFFSET 140
#elif defined(__sparc) && !defined(__sparc64__)
/* Only 32bit SPARC is affected, 64bit SPARC ELF support was incomplete. */
#define RTLD_OBJ_DLOPEN_OFFSET 140
#endif

#define RTLD_MAGIC	0xd550b87a
#define RTLD_VERSION	1

#ifdef RTLD_OBJ_DLOPEN_OFFSET
const uintptr_t _rtld_compat_obj[] = {
#  ifdef _LP64
#    if BYTE_ORDER == BIG_ENDIAN
	[0] = (((uint64_t)RTLD_MAGIC) << 32) | RTLD_VERSION,
#    else
	[0] = (((uint64_t)RTLD_VERSION) << 32) | RTLD_MAGIC,
#    endif
#  else
	[0] = RTLD_MAGIC,
	[1] = RTLD_VERSION,
#  endif
	[(RTLD_OBJ_DLOPEN_OFFSET / sizeof(uintptr_t)) + 0] = (uintptr_t)dlopen,
	[(RTLD_OBJ_DLOPEN_OFFSET / sizeof(uintptr_t)) + 1] = (uintptr_t)dlsym,
	[(RTLD_OBJ_DLOPEN_OFFSET / sizeof(uintptr_t)) + 2] = (uintptr_t)dlerror,
	[(RTLD_OBJ_DLOPEN_OFFSET / sizeof(uintptr_t)) + 3] = (uintptr_t)dlclose,
	[(RTLD_OBJ_DLOPEN_OFFSET / sizeof(uintptr_t)) + 4] = (uintptr_t)dladdr,
};
#else
const uintptr_t _rtld_compat_obj[] = {
#  ifdef _LP64
#    if BYTE_ORDER == BIG_ENDIAN
	[0] = (((uint64_t)RTLD_MAGIC) << 32) | RTLD_VERSION,
#    else
	[0] = (((uint64_t)RTLD_VERSION) << 32) | RTLD_MAGIC,
#    endif
#  else
	[0] = RTLD_MAGIC,
	[1] = RTLD_VERSION,
#  endif
};
#endif /* RTLD_OBJ_DLOPEN_OFFSET */
