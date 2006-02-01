/* $NetBSD: smdk2410.c,v 1.3.2.1 2006/02/01 14:51:26 yamt Exp $ */

/*
 * Copyright (c) 2003 By Noon Software, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Board initialization routines for the Samsung's SMDK2410.
 */

#include <sys/types.h>
#include <lib/libsa/stand.h>
#include <arch/arm/s3c2xx0/s3c2410reg.h>

#include "board.h"

void
board_init(void)
{

	mem_init();
}

void
board_fini(void)
{

}

void
mem_init(void)
{
	uint32_t start, size, heap;

	start = S3C2410_SDRAM_START;
	size = SDRAM_SIZE;

	heap = (start + size) - BOARD_HEAP_SIZE;

	printf(">> RAM 0x%x - 0x%x, heap at 0x%x\n",
	    start, (start + size) - 1, heap);
	setheap((void *)heap, (void *)(heap + BOARD_HEAP_SIZE - 1));
}
