/*	$NetBSD: dvma3x.h,v 1.3 1997/10/16 15:45:46 gwr Exp $	*/

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
 * DVMA (Direct Virtual Memory Access - like DMA)
 *
 * (Most of the comments pertaining to the sun3x DVMA are now
 *  in dvma.c)
 *
 * Relevant parts of virtual memory map are:
 *
 * FE00.0000  monitor map (devices)
 * FF00.0000  Begining of DVMA space
 * FFFF.FFFF  End of DVMA space
 */

/*
 * To convert an address in DVMA space to a slave address,
 * just use a logical AND with one of the following masks.
 * To convert back, just logical OR with the base address.
 */
#define DVMA_SLAVE_BASE 	0xFF000000
#define DVMA_SLAVE_MASK 	0x00FFffff	/* 16MB */

/* Compatibility with the sun3... */
#define DVMA_OBIO_SLAVE_BASE DVMA_SLAVE_BASE

/*
 * DVMA is the last 16MB, but the PROM owns the last 1M.
 * See mon.h: MON_DVMA_BASE, MON_DVMA_SIZE.
 */
#define	DVMA_SPACE_START	0xFF000000
#define DVMA_SPACE_LENGTH  	  0xF00000

void dvma_init __P((void));

/* Allocate/free actual pages of DVMA space. */
void * dvma_malloc __P((size_t bytes));
void dvma_free(void *addr, size_t bytes);

/* Remap/unmap kernel memory in DVMA space. */
void * dvma_mapin __P((void *kva, int len, int canwait));
void dvma_mapout __P((void *dvma_addr, int len));

/* Convert a kernel DVMA pointer to a slave address. */
u_long dvma_kvtopa __P((void *kva, int bus));

