/*	$NetBSD: dvma.h,v 1.11.26.1 2007/03/12 05:51:09 rmind Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * DVMA (Direct Virtual Memory Access)
 *
 * For the unfamiliar, this is just DMA where the device doing DMA
 * operates in a virtual address space.  The virtual to physical
 * translations are controlled by some sort of I/O MMU, which may
 * be the same used by the CPU (sun3) or not (sun3x).  Usually, the
 * virtual space accessed by DVMA devices is a small sub-range of
 * the CPU virtual space, and that range is known as DVMA space.
 */

#ifdef	_SUN3_
#include <machine/dvma3.h>
#endif	/* SUN3 */
#ifdef	_SUN3X_
#include <machine/dvma3x.h>
#endif	/* SUN3X */

void dvma_init(void);

/* Allocate/free actual pages of DVMA space. */
void *dvma_malloc(size_t);
void dvma_free(void *, size_t);

/* Remap/unmap kernel memory in DVMA space. */
void *dvma_mapin(void *, int, int);
void dvma_mapout(void *, int);

/* Convert a kernel DVMA pointer to a slave address. */
u_long dvma_kvtopa(void *, int);

