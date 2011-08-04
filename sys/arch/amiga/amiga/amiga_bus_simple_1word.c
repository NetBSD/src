/* $NetBSD: amiga_bus_simple_1word.c,v 1.4 2011/08/04 17:48:50 rkujawa Exp $ */

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
#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/pte.h>

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: amiga_bus_simple_1word.c,v 1.4 2011/08/04 17:48:50 rkujawa Exp $");

#define AMIGA_SIMPLE_BUS_STRIDE 1		/* 1 byte per byte */
#define AMIGA_SIMPLE_BUS_WORD_METHODS
#define AMIGA_SIMPLE_BUS_LONGWORD_METHODS

#include "simple_busfuncs.c"

bsr(oabs(bsr4_swap_), u_int32_t);
bsw(oabs(bsw4_swap_), u_int32_t);

bsrm(oabs(bsrm2_swap_), u_int16_t);
bswm(oabs(bswm2_swap_), u_int16_t);

int oabs(bsm_absolute_)(bus_space_tag_t, bus_addr_t, bus_size_t, int,
			bus_space_handle_t *);

/* ARGSUSED */
int
oabs(bsm_absolute_)(tag, address, size, flags, handlep)
	bus_space_tag_t tag;
	bus_addr_t address;
	bus_size_t size;
	int flags;
	bus_space_handle_t *handlep;
{
	uint32_t pa = kvtop((void*) tag->base);
	*handlep = tag->base + (address - pa) * AMIGA_SIMPLE_BUS_STRIDE;
	return 0;
}

/* ARGSUSED */
u_int32_t
oabs(bsr4_swap_)(handle, offset)
	bus_space_handle_t handle;
	bus_size_t offset;
{
	volatile u_int32_t *p;
	u_int32_t x;

	p = (volatile u_int32_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	x = *p;
	amiga_bus_reorder_protect();
	return bswap32(x);
}

/* ARGSUSED */
void
oabs(bsw4_swap_)(handle, offset, value)
	bus_space_handle_t handle;
	bus_size_t offset;
	unsigned value;
{
	volatile u_int32_t *p;

	p = (volatile u_int32_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	*p = bswap32( (u_int32_t)value );
	amiga_bus_reorder_protect();
}

/* ARGSUSED */
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

/* ARGSUSED */
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

const struct amiga_bus_space_methods amiga_bus_stride_1swap_abs = {

	oabs(bsm_absolute_),
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

	oabs(bsr2_),            /* XXX swap? */
	oabs(bsw2_),            /* XXX swap? */
	oabs(bsr2_),
	oabs(bsw2_),
	oabs(bsrm2_swap_),
	oabs(bswm2_swap_),
	oabs(bsrm2_),
	oabs(bswm2_),
	oabs(bsrr2_),           /* XXX swap? */
	oabs(bswr2_),           /* XXX swap? */
	oabs(bsrr2_),
	oabs(bswr2_),
	oabs(bssr2_),           /* XXX swap? */
	oabs(bscr2_),           /* XXX swap? */

	oabs(bsr4_swap_),
	oabs(bsw4_swap_),
	oabs(bsr4_),
	oabs(bsw4_),
	oabs(bsrm4_),		/* XXX swap? */
	oabs(bswm4_),		/* XXX swap? */
	oabs(bsrm4_),
	oabs(bswm4_),
	oabs(bsrr4_),		/* XXX swap? */
	oabs(bswr4_),		/* XXX swap? */
	oabs(bsrr4_),
	oabs(bswr4_),
	oabs(bssr4_),		/* XXX swap? */
	oabs(bscr4_)		/* XXX swap? */

};

const struct amiga_bus_space_methods amiga_bus_stride_1swap = {

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

	oabs(bsr2_),		/* XXX swap? */
	oabs(bsw2_),		/* XXX swap? */
	oabs(bsr2_),
	oabs(bsw2_),
	oabs(bsrm2_swap_),
	oabs(bswm2_swap_),
	oabs(bsrm2_),
	oabs(bswm2_),
	oabs(bsrr2_),		/* XXX swap? */
	oabs(bswr2_),		/* XXX swap? */
	oabs(bsrr2_),
	oabs(bswr2_),
	oabs(bssr2_),		/* XXX swap? */
	oabs(bscr2_),		/* XXX swap? */

	oabs(bsr4_swap_),
	oabs(bsw4_swap_),
	oabs(bsr4_),
	oabs(bsw4_),
	oabs(bsrm4_),		/* XXX swap? */
	oabs(bswm4_),		/* XXX swap? */
	oabs(bsrm4_),
	oabs(bswm4_),
	oabs(bsrr4_),		/* XXX swap? */
	oabs(bswr4_),		/* XXX swap? */
	oabs(bsrr4_),
	oabs(bswr4_),
	oabs(bssr4_),		/* XXX swap? */
	oabs(bscr4_)		/* XXX swap? */

};

