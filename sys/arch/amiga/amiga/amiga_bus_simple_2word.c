/* $NetBSD: amiga_bus_simple_2word.c,v 1.1 1999/12/30 20:56:44 is Exp $ */

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
 * We combine this simple_bus byte access with the word access methods
 * needed for if_ne_zbus, that is, the _stream_ variants are the same
 * as the non_stream ones.
 */

#define AMIGA_SIMPLE_BUS_STRIDE 2		/* 1 byte per word */
#define AMIGA_SIMPLE_BUS_NO_ARRAY

/* byte methods */

#include "simple_busfuncs.c"

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
oabs(bsr2_) (handle, offset)
	bus_space_handle_t handle;
	bus_size_t offset;
{
	u_int16_t *p;

	p = (u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	return (*p);
}

void
oabs(bsw2_)(handle, offset, value)
	bus_space_handle_t handle;
	bus_size_t offset;
	unsigned value;
{
	u_int16_t *p;

	p = (u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	*p = (u_int16_t)value;
}

void
oabs(bsrm2_)(handle, offset, pointer, count)
	bus_space_handle_t handle;
	bus_size_t offset;
	u_int16_t *pointer;
	bus_size_t count;
{
	volatile u_int16_t *p;

	p = (volatile u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	
	while (count > 0) {
		*pointer++ = *p;
		--count;
	}
}

void
oabs(bswm2_)(handle, offset, pointer, count)
	bus_space_handle_t handle;
	bus_size_t offset;
	const u_int16_t *pointer;
	bus_size_t count;
{
	volatile u_int16_t *p;

	p = (volatile u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	
	while (count > 0) {
		*p = *pointer++;
		--count;
	}
}

void
oabs(bsrr2_)(handle, offset, pointer, count)
	bus_space_handle_t handle;
	bus_size_t offset;
	u_int16_t *pointer;
	bus_size_t count;
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	
	while (count > 0) {
		*pointer++ = *(volatile u_int16_t *)p;
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int16_t);
		--count;
	}
}

void
oabs(bswr2_)(handle, offset, pointer, count)
	bus_space_handle_t handle;
	bus_size_t offset;
	const u_int16_t *pointer;
	bus_size_t count;
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	
	while (count > 0) {
		*(volatile u_int16_t *)p = *pointer++;
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int16_t);
		--count;
	}
}

void
oabs(bssr2_)(handle, offset, value, count)
	bus_space_handle_t handle;
	bus_size_t offset;
	unsigned value;
	bus_size_t count;
{
	volatile u_int8_t *p;

	p = (volatile u_int8_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	
	while (count > 0) {
		*(volatile u_int16_t *)p = (unsigned)value;
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int16_t);
		--count;
	}
}

void
oabs(bscr2_)(handlefrom, from, handleto, to, count)
	bus_space_handle_t handlefrom, handleto;
	bus_size_t from, to;
	bus_size_t count;
{
	volatile u_int8_t *p, *q;

	p = (volatile u_int8_t *)(handlefrom + from * AMIGA_SIMPLE_BUS_STRIDE);
	q = (volatile u_int8_t *)(handleto   +   to * AMIGA_SIMPLE_BUS_STRIDE);
	
	while (count > 0) {
		*(volatile u_int16_t *)q = *(volatile u_int16_t *)p;
		p += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int16_t);
		q += AMIGA_SIMPLE_BUS_STRIDE * sizeof(u_int16_t);
		--count;
	}
}

/* method array */
 
const struct amiga_bus_space_methods amiga_bus_stride_2word = {

        oabs(bsm_),
        oabs(bsms_),
        oabs(bsu_),
        0,
        0,
 
        oabs(bsr1_),
        oabs(bsw1_),
        oabs(bsrm1_),
        oabs(bswm1_),
        oabs(bsrr1_),
        oabs(bswr1_),
        oabs(bssr1_),
        oabs(bscr1_),

        oabs(bsr2_),
        oabs(bsw2_),
        oabs(bsr2_),
        oabs(bsw2_),
        oabs(bsrm2_),
        oabs(bswm2_),
        oabs(bsrm2_),
        oabs(bswm2_),
        oabs(bsrr2_),
        oabs(bswr2_),
        oabs(bsrr2_),
        oabs(bswr2_),
        oabs(bssr2_),
        oabs(bscr2_)
};
