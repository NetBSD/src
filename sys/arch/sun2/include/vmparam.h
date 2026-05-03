/*	$NetBSD: vmparam.h,v 1.23 2026/05/03 19:10:41 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

/*
 * Machine dependent constants for Sun2
 *
 * The Sun2 has limited total kernel virtual space (14MB) and
 * can not use main memory for page tables.  (All active PTEs
 * must be installed in special translation RAM in the MMU).
 * Therefore, parameters that would normally configure the
 * size of various page tables are irrelevant.  Only things
 * that consume portions of kernel virtual (KV) space matter,
 * and those things should be chosen to conserve KV space.
 */

#if defined(_KERNEL) && !defined(_MODULE)

#define VM_MIN_KERNEL_ADDRESS	((vaddr_t)KERNBASE)
#define VM_MAX_KERNEL_ADDRESS	((vaddr_t)KERN_END)

/*
 * Size of phys_map, used for mapping user I/O buffer into kernel
 * space for physio.
 *
 * The actual limitation for physio requests will be the DVMA space,
 * and that is fixed by hardware design at 256K.  We could make the
 * physio map larger than that, but it would not buy us much.
 */
#define	VM_PHYS_SIZE		(256 * 1024)

/*
 * We definitely need a small pager map.
 */
#define	PAGER_MAP_DEFAULT_SIZE (1 * 1024 * 1024)

#endif /* _KERNEL && !_MODULE */

#include <m68k/vmparam.h>
