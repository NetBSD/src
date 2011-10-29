/* $NetBSD: amiga_bus_simple_4.c,v 1.9 2011/10/29 19:25:19 rkujawa Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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
__KERNEL_RCSID(1, "$NetBSD: amiga_bus_simple_4.c,v 1.9 2011/10/29 19:25:19 rkujawa Exp $");

#define AMIGA_SIMPLE_BUS_STRIDE 4		/* 1 byte per long */
#define AMIGA_SIMPLE_BUS_WORD_METHODS
#define AMIGA_SIMPLE_BUS_LONGWORD_METHODS

#include "simple_busfuncs.c"

/*
 * Little-endian word methods.
 * Stream access does not swap, used for 16-bit wide transfers of byte streams.
 * Non-stream access swaps bytes.
 * XXX Only *_multi_2 transfers currently swap bytes XXX
 */

bsrm(oabs(bsrm2_swap_), u_int16_t);
bswm(oabs(bswm2_swap_), u_int16_t);
bsrm(oabs(bsrm4_swap_), u_int32_t);
bswm(oabs(bswm4_swap_), u_int32_t);

void
oabs(bsrm2_swap_)(bus_space_handle_t handle, bus_size_t offset,
		  u_int16_t *pointer, bus_size_t count)
{
	volatile u_int16_t *p;

	p = (volatile u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*pointer++ = bswap16(*p);
		amiga_bus_reorder_protect();
		--count;
	}
}

void
oabs(bswm2_swap_)(bus_space_handle_t handle, bus_size_t offset,
		  const u_int16_t *pointer, bus_size_t count)
{
	volatile u_int16_t *p;

	p = (volatile u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*p = bswap16(*pointer);
		amiga_bus_reorder_protect();
		++pointer;
		--count;
	}
}

void
oabs(bsrm4_swap_)(bus_space_handle_t handle, bus_size_t offset,
		  u_int32_t *pointer, bus_size_t count)
{
	volatile u_int32_t *p;

	p = (volatile u_int32_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*pointer++ = bswap32(*p);
		amiga_bus_reorder_protect();
		--count;
	}
}

void
oabs(bswm4_swap_)(bus_space_handle_t handle, bus_size_t offset,
		  const u_int32_t *pointer, bus_size_t count)
{
	volatile u_int32_t *p;

	p = (volatile u_int32_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*p = bswap32(*pointer);
		amiga_bus_reorder_protect();
		++pointer;
		--count;
	}
}

const struct amiga_bus_space_methods amiga_bus_stride_4swap = {

	.bsm =		oabs(bsm_),
	.bsms =		oabs(bsms_),
	.bsu =		oabs(bsu_),
	.bsa =		NULL,
	.bsf =		NULL,

	.bsr1 =		oabs(bsr1_),
	.bsw1 =		oabs(bsw1_),
	.bsrm1 =	oabs(bsrm1_),
	.bswm1 =	oabs(bswm1_),
	.bsrr1 =	oabs(bsrr1_),
	.bswr1 =	oabs(bswr1_),
	.bssr1 =	oabs(bssr1_),
	.bscr1 =	oabs(bscr1_),

	.bsr2 =		oabs(bsr2_),		/* XXX swap? */
	.bsw2 =		oabs(bsw2_),		/* XXX swap? */
	.bsrs2 =	oabs(bsr2_),
	.bsws2 =	oabs(bsw2_),
	.bsrm2 =	oabs(bsrm2_swap_),
	.bswm2 =	oabs(bswm2_swap_),
	.bsrms2 =	oabs(bsrm2_),
	.bswms2 =	oabs(bswm2_),
	.bsrr2 =	oabs(bsrr2_),		/* XXX swap? */
	.bswr2 =	oabs(bswr2_),		/* XXX swap? */
	.bsrrs2 =	oabs(bsrr2_),
	.bswrs2 =	oabs(bswr2_),
	.bssr2 =	oabs(bssr2_),		/* XXX swap? */
	.bscr2 =	oabs(bscr2_),		/* XXX swap? */

	.bsr4 =		oabs(bsr4_),		/* XXX swap? */
	.bsw4 =		oabs(bsw4_),		/* XXX swap? */
	.bsrs4 =	oabs(bsr4_),
	.bsws4 =	oabs(bsw4_),
	.bsrm4 =	oabs(bsrm4_swap_),
	.bswm4 =	oabs(bswm4_swap_),
	.bsrms4 =	oabs(bsrm4_),
	.bswms4 =	oabs(bswm4_),
	.bsrr4 =	oabs(bsrr4_),		/* XXX swap? */
	.bswr4 =	oabs(bswr4_),		/* XXX swap? */
	.bsrrs4 =	oabs(bsrr4_),
	.bswrs4 =	oabs(bswr4_),
	.bssr4 =	oabs(bssr4_),		/* XXX swap? */
	.bscr4 =	oabs(bscr4_)		/* XXX swap? */
};

