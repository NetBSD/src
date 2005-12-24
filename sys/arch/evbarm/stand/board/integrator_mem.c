/*	$NetBSD: integrator_mem.c,v 1.3 2005/12/24 20:07:03 perry Exp $	*/

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

#define	INL(x)		*((volatile uint32_t *) (x))

void
mem_init(void)
{
	uint32_t cm_sdram;
	uint32_t heap, size;

	cm_sdram = INL(0x10000020);

	switch ((cm_sdram >> 2) & 0x7) {
	case 0:
		size = 16 * 1024 * 1024;
		break;

	case 1:
		size = 32 * 1024 * 1024;
		break;

	case 2:
		size = 64 * 1024 * 1024;
		break;

	case 3:
		size = 128 * 1024 * 1024;
		break;

	case 4:
		size = 256 * 1024 * 1024;
		break;

	default:
		printf("** CM_SDRAM retuns unknown value, using 16M\n");
		size = 16 * 1024 * 1024;
		break;
	}

	/* Start is always 0. */
	heap = size - HEAP_SIZE;

	printf(">> RAM 0x%x - 0x%x, heap at 0x%x\n", 0, size - 1, heap);
	setheap((void *)heap, (void *)(size - 1));
}
