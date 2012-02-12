/* $NetBSD: simple_busfuncs.c,v 1.11 2012/02/12 16:34:06 matt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: simple_busfuncs.c,v 1.11 2012/02/12 16:34:06 matt Exp $");

/*
 * Do NOT use this standalone.
 * Instead define the stride (linear, not logarithmic!) in the enclosing
 * file (AMIGA_SIMPLE_BUS_STRIDE), then include this one.
 * If you want to use it only to provide the functions without creating the
 * method array, also define AMIGA_SIMPLE_BUS_NO_ARRAY
 */

#ifndef AMIGA_SIMPLE_BUS_STRIDE
Error AMIGA_SIMPLE_BUS_STRIDE not defined in __FILE__, line __LINE__ .
#endif

#include <sys/bus.h>
#include <sys/null.h>

#define MKN2(x,y) __CONCAT(x, y)
#define MKN1(x,y) MKN2(x, y)
#define oabs(n) MKN1(n, AMIGA_SIMPLE_BUS_STRIDE)

/* function declarations */

int oabs(bsm_)(bus_space_tag_t, bus_addr_t, bus_size_t, int,
	bus_space_handle_t *);

int oabs(bsms_)(bus_space_handle_t, bus_size_t, bus_size_t,
	bus_space_handle_t *);

void oabs(bsu_)(bus_space_handle_t, bus_size_t);

bsr (oabs(bsr1_), u_int8_t);
bsw (oabs(bsw1_), u_int8_t);
bsrm(oabs(bsrm1_), u_int8_t);
bswm(oabs(bswm1_), u_int8_t);
bsrm(oabs(bsrr1_), u_int8_t);
bswm(oabs(bswr1_), u_int8_t);
bssr(oabs(bssr1_), u_int8_t);
bscr(oabs(bscr1_), u_int8_t);

/* function definitions */
/* ARGSUSED */
int
oabs(bsm_)(
	bus_space_tag_t tag,
	bus_addr_t address,
	bus_size_t size,
	int flags,
	bus_space_handle_t *handlep)
{
	*handlep = tag->base + address * AMIGA_SIMPLE_BUS_STRIDE;
	return 0;
}

/* ARGSUSED */
int
oabs(bsms_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	bus_size_t size,
	bus_space_handle_t *nhandlep)
{
	*nhandlep = handle + offset * AMIGA_SIMPLE_BUS_STRIDE;
	return 0;
}

/* ARGSUSED */
void
oabs(bsu_)(
	bus_space_handle_t handle,
	bus_size_t size)
{
	return;
}

u_int8_t
oabs(bsr1_)(
	bus_space_handle_t handle,
	bus_size_t offset)
{
	u_int8_t *p;
	u_int8_t x;

	p = (u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	x = *p;
	amiga_bus_reorder_protect();
	return x;
}

void
oabs(bsw1_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	unsigned value)
{
	u_int8_t *p;

	p = (u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	*p = (u_int8_t)value;
	amiga_bus_reorder_protect();
}

void
oabs(bsrm1_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	u_int8_t *pointer,
	bus_size_t count)
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*pointer++ = *p;
		amiga_bus_reorder_protect();
		--count;
	}
}

void
oabs(bswm1_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	const u_int8_t *pointer,
	bus_size_t count)
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*p = *pointer++;
		amiga_bus_reorder_protect();
		--count;
	}
}

void
oabs(bsrr1_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	u_int8_t *pointer,
	bus_size_t count)
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*pointer++ = *p;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE;
		--count;
	}
}

void
oabs(bswr1_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	const u_int8_t *pointer,
	bus_size_t count)
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*p = *pointer++;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE;
		--count;
	}
}

void
oabs(bssr1_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	unsigned value,
	bus_size_t count)
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*p = value;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE;
		--count;
	}
}

void
oabs(bscr1_)(
	bus_space_handle_t handlefrom,
	bus_size_t from,
	bus_space_handle_t handleto,
	bus_size_t to,
	bus_size_t count)
{
	volatile u_int8_t *p, *q;

	p = (volatile u_int8_t *)(handlefrom + from * AMIGA_SIMPLE_BUS_STRIDE);
	q = (volatile u_int8_t *)(handleto   +   to * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*q = *p;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE;
		q += AMIGA_SIMPLE_BUS_STRIDE;
		--count;
	}
}


#ifdef AMIGA_SIMPLE_BUS_WORD_METHODS

/* word methods */

bsr (oabs(bsr2_), u_int16_t);
bsw (oabs(bsw2_), u_int16_t);
bsrm(oabs(bsrm2_), u_int16_t);
bswm(oabs(bswm2_), u_int16_t);
bsrm(oabs(bsrr2_), u_int16_t);
bswm(oabs(bswr2_), u_int16_t);
bssr(oabs(bssr2_), u_int16_t);
bscr(oabs(bscr2_), u_int16_t);

u_int16_t
oabs(bsr2_)(
	bus_space_handle_t handle,
	bus_size_t offset)
{
	u_int16_t *p;
	u_int16_t x;

	p = (u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	x = *p;
	amiga_bus_reorder_protect();
	return x;
}

void
oabs(bsw2_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	unsigned value)
{
	u_int16_t *p;

	p = (u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	*p = (u_int16_t)value;
	amiga_bus_reorder_protect();
}

void
oabs(bsrm2_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	u_int16_t *pointer,
	bus_size_t count)
{
	volatile u_int16_t *p;

	p = (volatile u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*pointer++ = *p;
		amiga_bus_reorder_protect();
		--count;
	}
}

void
oabs(bswm2_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	const u_int16_t *pointer,
	bus_size_t count)
{
	volatile u_int16_t *p;

	p = (volatile u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*p = *pointer++;
		amiga_bus_reorder_protect();
		--count;
	}
}

void
oabs(bsrr2_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	u_int16_t *pointer,
	bus_size_t count)
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*pointer++ = *(volatile u_int16_t *)p;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int16_t);
		--count;
	}
}

void
oabs(bswr2_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	const u_int16_t *pointer,
	bus_size_t count)
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*(volatile u_int16_t *)p = *pointer++;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int16_t);
		--count;
	}
}

void
oabs(bssr2_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	unsigned value,
	bus_size_t count)
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*(volatile u_int16_t *)p = (unsigned)value;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int16_t);
		--count;
	}
}

void
oabs(bscr2_)(
	bus_space_handle_t handlefrom,
	bus_size_t from,
	bus_space_handle_t handleto,
	bus_size_t to,
	bus_size_t count)
{
	volatile u_int8_t *p, *q;

	p = (volatile u_int8_t *)(handlefrom + from * AMIGA_SIMPLE_BUS_STRIDE);
	q = (volatile u_int8_t *)(handleto   +   to * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*(volatile u_int16_t *)q = *(volatile u_int16_t *)p;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int16_t);
		q += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int16_t);
		--count;
	}
}
#endif /* AMIGA_SIMPLE_BUS_WORD_METHODS */

#ifdef AMIGA_SIMPLE_BUS_LONGWORD_METHODS

/* longword methods */

bsr (oabs(bsr4_), u_int32_t);
bsw (oabs(bsw4_), u_int32_t);
bsrm(oabs(bsrm4_), u_int32_t);
bswm(oabs(bswm4_), u_int32_t);
bsrm(oabs(bsrr4_), u_int32_t);
bswm(oabs(bswr4_), u_int32_t);
bssr(oabs(bssr4_), u_int32_t);
bscr(oabs(bscr4_), u_int32_t);

u_int32_t
oabs(bsr4_)(
	bus_space_handle_t handle,
	bus_size_t offset)
{
	u_int32_t *p;
	u_int32_t x;

	p = (u_int32_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	x = *p;
	amiga_bus_reorder_protect();
	return x;
}

void
oabs(bsw4_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	unsigned value)
{
	u_int32_t *p;

	p = (u_int32_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	*p = (u_int32_t)value;
	amiga_bus_reorder_protect();
}


void
oabs(bsrm4_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	u_int32_t *pointer,
	bus_size_t count)
{
	volatile u_int32_t *p;

	p = (volatile u_int32_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*pointer++ = *p;
		amiga_bus_reorder_protect();
		--count;
	}
}

void
oabs(bswm4_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	const u_int32_t *pointer,
	bus_size_t count)
{
	volatile u_int32_t *p;

	p = (volatile u_int32_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*p = *pointer++;
		amiga_bus_reorder_protect();
		--count;
	}
}

void
oabs(bsrr4_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	u_int32_t *pointer,
	bus_size_t count)
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*pointer++ = *(volatile u_int32_t *)p;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int32_t);
		--count;
	}
}

void
oabs(bswr4_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	const u_int32_t *pointer,
	bus_size_t count)
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*(volatile u_int32_t *)p = *pointer++;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int32_t);
		--count;
	}
}

void
oabs(bssr4_)(
	bus_space_handle_t handle,
	bus_size_t offset,
	unsigned value,
	bus_size_t count)
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*(volatile u_int32_t *)p = (unsigned)value;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int32_t);
		--count;
	}
}

void
oabs(bscr4_)(
	bus_space_handle_t handlefrom,
	bus_size_t from,
	bus_space_handle_t handleto,
	bus_size_t to,
	bus_size_t count)
{
	volatile u_int8_t *p, *q;

	p = (volatile u_int8_t *)(handlefrom + from * AMIGA_SIMPLE_BUS_STRIDE);
	q = (volatile u_int8_t *)(handleto   +   to * AMIGA_SIMPLE_BUS_STRIDE);

	while (count > 0) {
		*(volatile u_int32_t *)q = *(volatile u_int32_t *)p;
		amiga_bus_reorder_protect();
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int32_t);
		q += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int32_t);
		--count;
	}
}
#endif /* AMIGA_SIMPLE_BUS_LONGWORD_METHODS */

#ifndef AMIGA_SIMPLE_BUS_NO_ARRAY
/* method array */

const struct amiga_bus_space_methods oabs(amiga_bus_stride_) = {

	.bsm =		oabs(bsm_),
	.bsms =		oabs(bsms_),
	.bsu =		oabs(bsu_),
	.bsa =		NULL,
	.bsf =		NULL,

	.bsr1 =	oabs(bsr1_),
	.bsw1 =		oabs(bsw1_),
	.bsrm1 =	oabs(bsrm1_),
	.bswm1 =	oabs(bswm1_),
	.bsrr1 =	oabs(bsrr1_),
	.bswr1 =	oabs(bswr1_),
	.bssr1 =	oabs(bssr1_),
	.bscr1 =	oabs(bscr1_),

#ifdef AMIGA_SIMPLE_BUS_WORD_METHODS
	.bsr2 =		oabs(bsr2_),
	.bsw2 =		oabs(bsw2_),
	.bsrs2 =	oabs(bsr2_),
	.bsws2 =	oabs(bsw2_),
	.bsrm2 =	oabs(bsrm2_),
	.bswm2 =	oabs(bswm2_),
	.bsrms2 =	oabs(bsrm2_),
	.bswms2 =	oabs(bswm2_),
	.bsrr2 =	oabs(bsrr2_),
	.bswr2 =	oabs(bswr2_),
	.bsrrs2 =	oabs(bsrr2_),
	.bswrs2 =	oabs(bswr2_),
	.bssr2 =	oabs(bssr2_),
	.bscr2 =	oabs(bscr2_),
#endif /* AMIGA_SIMPLE_BUS_WORD_METHODS */

#ifdef AMIGA_SIMPLE_BUS_LONGWORD_METHODS
	.bsr4 =		oabs(bsr4_),
	.bsw4 =		oabs(bsw4_),
	.bsrs4 =	oabs(bsr4_),
	.bsws4 =	oabs(bsw4_),
	.bsrm4 =	oabs(bsrm4_),
	.bswm4 =	oabs(bswm4_),
	.bsrms4 =	oabs(bsrm4_),
	.bswms4 =	oabs(bswm4_),
	.bsrr4 =	oabs(bsrr4_),
	.bswr4 =	oabs(bswr4_),
	.bsrrs4 =	oabs(bsrr4_),
	.bswrs4 =	oabs(bswr4_),
	.bssr4 =	oabs(bssr4_),
	.bscr4 =	oabs(bscr4_)
#endif /* AMIGA_SIMPLE_BUS_LONGWORD_METHODS */
};
#endif
