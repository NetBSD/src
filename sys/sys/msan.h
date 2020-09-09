/*	$NetBSD: msan.h,v 1.2 2020/09/09 16:29:59 maxv Exp $	*/

/*
 * Copyright (c) 2019-2020 Maxime Villard, m00nbsd.net
 * All rights reserved.
 *
 * This code is part of the KMSAN subsystem of the NetBSD kernel.
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

#ifndef _SYS_MSAN_H_
#define _SYS_MSAN_H_

#ifdef _KERNEL_OPT
#include "opt_kmsan.h"
#endif

#ifdef KMSAN
#include <sys/types.h>
#include <sys/bus.h>

#define KMSAN_STATE_UNINIT	0xFF
#define KMSAN_STATE_INITED	0x00

#define KMSAN_TYPE_STACK	0
#define KMSAN_TYPE_KMEM		1
#define KMSAN_TYPE_MALLOC	2
#define KMSAN_TYPE_POOL		3
#define KMSAN_TYPE_UVM		4

#define KMSAN_DMA_LINEAR	1
#define KMSAN_DMA_MBUF		2
#define KMSAN_DMA_UIO		3
#define KMSAN_DMA_RAW		4

#define __RET_ADDR		(uintptr_t)__builtin_return_address(0)

void kmsan_init(void *);
void kmsan_shadow_map(void *, size_t);

void kmsan_lwp_alloc(struct lwp *);
void kmsan_lwp_free(struct lwp *);

void kmsan_dma_sync(bus_dmamap_t, bus_addr_t, bus_size_t, int);
void kmsan_dma_load(bus_dmamap_t, void *, bus_size_t, int);

void kmsan_orig(void *, size_t, int, uintptr_t);
void kmsan_mark(void *, size_t, uint8_t);
void kmsan_check_mbuf(void *);
void kmsan_check_buf(void *);
#else
#define kmsan_init(u)			__nothing
#define kmsan_shadow_map(a, s)		__nothing
#define kmsan_lwp_alloc(l)		__nothing
#define kmsan_lwp_free(l)		__nothing
#define kmsan_dma_sync(m, a, s, o)	__nothing
#define kmsan_dma_load(m, b, s, o)	__nothing
#define kmsan_orig(p, l, c, a)		__nothing
#define kmsan_mark(p, l, c)		__nothing
#define kmsan_check_mbuf(m)		__nothing
#define kmsan_check_buf(b)		__nothing
#endif

#endif /* !_SYS_MSAN_H_ */
