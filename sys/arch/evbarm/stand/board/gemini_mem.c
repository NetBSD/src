/*	$NetBSD: gemini_mem.c,v 1.2.4.2 2008/12/13 01:13:08 haad Exp $	*/

/* adapted from:
 *	NetBSD: integrator_mem.c,v 1.4 2006/01/16 19:34:53 he Exp
 */

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file provides the mem_init() function for ARM Integrator
 * core modules.
 */

#include <sys/types.h>
#include <lib/libsa/stand.h>

#include "board.h"

/*
 * constrain MEMSIZE to avoid stepping out of smallest-case
 * Gemini CPU Remap Control remapped-private memory size
 * "bad" things can happen if the gzboot heap is in
 * non-core-private memory.
 */
#define MEMSIZE	(32 * 1024 * 1024)	/* 32MB */

void
mem_init(void)
{
	uint32_t heap, size;

	size = MEMSIZE;

	/* Start is always 0. */
	heap = size - BOARD_HEAP_SIZE;

	printf(">> RAM 0x%x - 0x%x, heap at 0x%x\n", 0, size - 1, heap);
	setheap((void *)heap, (void *)(size - 1));
}
