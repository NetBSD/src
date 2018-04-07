/* $NetBSD: platform.h,v 1.2.2.2 2018/04/07 04:12:12 pgoyette Exp $ */

/*-
 * Copyright (c) 2015-2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _EVBARM_FDT_PLATFORM_H
#define _EVBARM_FDT_PLATFORM_H

#ifndef _LOCORE
void fdt_add_reserved_memory_range(uint64_t, uint64_t);
#endif

#ifdef __aarch64__

#define KERNEL_IO_VBASE		VM_KERNEL_IO_ADDRESS

#define KERNEL_VM_BASE		VM_MIN_KERNEL_ADDRESS
#define KERNEL_VM_SIZE		(VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS)

#else /* __aarch64__ */

#define KERNEL_IO_VBASE		0xf0000000
#define KERNEL_IO_VSIZE		(KERNEL_IO_VBASE - VM_MAX_KERNEL_ADDRESS)

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
#define KERNEL_VM_BASE		0xc0000000
#define KERNEL_VM_SIZE		0x20000000 /* 0x20000000 = 512MB */
#else
#define KERNEL_VM_BASE		0x90000000
#define KERNEL_VM_SIZE		0x50000000 /* 0x50000000 = 1.25GB */
#endif

#endif /* !__aarch64 */

#endif /* _EVBARM_FDT_PLATFORM_H */
