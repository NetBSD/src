/* $NetBSD: amiga_bus_simple_1word.c,v 1.8.10.1 2014/08/10 06:53:49 tls Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: amiga_bus_simple_1word.c,v 1.8.10.1 2014/08/10 06:53:49 tls Exp $");

#define AMIGA_SIMPLE_BUS_STRIDE 1		/* 1 byte per byte */
#define AMIGA_SIMPLE_BUS_WORD_METHODS
#define AMIGA_SIMPLE_BUS_LONGWORD_METHODS

#include "simple_busfuncs.c"

bsr(oabs(bsr2_swap_), u_int16_t);
bsw(oabs(bsw2_swap_), u_int16_t);
bsr(oabs(bsr4_swap_), u_int32_t);
bsw(oabs(bsw4_swap_), u_int32_t);

bsrm(oabs(bsrm2_swap_), u_int16_t);
bswm(oabs(bswm2_swap_), u_int16_t);

int oabs(bsm_absolute_)(bus_space_tag_t, bus_addr_t, bus_size_t, int,
			bus_space_handle_t *);

/* ARGSUSED */
int
oabs(bsm_absolute_)(bus_space_tag_t tag, bus_addr_t address,
	bus_size_t size, int flags, bus_space_handle_t *handlep)
{
	uint32_t pa = kvtop((void*) tag->base);
	*handlep = tag->base + (address - pa) * AMIGA_SIMPLE_BUS_STRIDE;
	return 0;
}

/* ARGSUSED */
u_int16_t
oabs(bsr2_swap_)(bus_space_handle_t handle, bus_size_t offset)
{
	volatile u_int16_t *p;
	u_int16_t x;

	p = (volatile u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	x = *p;
	amiga_bus_reorder_protect();
	return bswap16(x);
}

/* ARGSUSED */
void
oabs(bsw2_swap_)(bus_space_handle_t handle, bus_size_t offset, unsigned value)
{
	volatile u_int16_t *p;

	p = (volatile u_int16_t *)(handle + offset * AMIGA_SIMPLE_BUS_STRIDE);
	*p = bswap16( (u_int16_t)value );
	amiga_bus_reorder_protect();
}

/* ARGSUSED */
u_int32_t
oabs(bsr4_swap_)(bus_space_handle_t handle, bus_size_t offset)
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
oabs(bsw4_swap_)(bus_space_handle_t handle, bus_size_t offset, unsigned value)
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

	.bsm =		oabs(bsm_absolute_),
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

	.bsr2 =		oabs(bsr2_),            /* XXX swap? */
	.bsw2 =		oabs(bsw2_),            /* XXX swap? */
	.bsrs2 =	oabs(bsr2_),
	.bsws2 =	oabs(bsw2_),
	.bsrm2 =	oabs(bsrm2_swap_),
	.bswm2 =	oabs(bswm2_swap_),
	.bsrms2 =	oabs(bsrm2_),
	.bswms2 =	oabs(bswm2_),
	.bsrr2 =	oabs(bsrr2_),           /* XXX swap? */
	.bswr2 =	oabs(bswr2_),           /* XXX swap? */
	.bssr2 =	oabs(bssr2_),           /* XXX swap? */
	.bscr2 =	oabs(bscr2_),           /* XXX swap? */

	.bsr4 =		oabs(bsr4_swap_),
	.bsw4 =		oabs(bsw4_swap_),
	.bsrs4 =	oabs(bsr4_),
	.bsws4 =	oabs(bsw4_),
	.bsrm4 =	oabs(bsrm4_),		/* XXX swap? */
	.bswm4 =	oabs(bswm4_),		/* XXX swap? */
	.bsrms4 =	oabs(bsrm4_),
	.bswms4 =	oabs(bswm4_),
	.bsrr4 =	oabs(bsrr4_),		/* XXX swap? */
	.bswr4 =	oabs(bswr4_),		/* XXX swap? */
	.bsrrs4 =	oabs(bsrr4_),
	.bswrs4 =	oabs(bswr4_),
	.bssr4 =	oabs(bssr4_),		/* XXX swap? */
	.bscr4 =	oabs(bscr4_)		/* XXX swap? */
};

const struct amiga_bus_space_methods amiga_bus_stride_1swap = {

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

	.bsr2 =		oabs(bsr2_swap_),
	.bsw2 =		oabs(bsw2_swap_),
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

	.bsr4 =		oabs(bsr4_swap_),
	.bsw4 =		oabs(bsw4_swap_),
	.bsrs4 =	oabs(bsr4_),
	.bsws4 =	oabs(bsw4_),
	.bsrm4 =	oabs(bsrm4_),		/* XXX swap? */
	.bswm4 =	oabs(bswm4_),		/* XXX swap? */
	.bsrms4 =	oabs(bsrm4_),
	.bswms4 =	oabs(bswm4_),
	.bsrr4 =	oabs(bsrr4_),		/* XXX swap? */
	.bswr4 =	oabs(bswr4_),		/* XXX swap? */
	.bsrrs4 =	oabs(bsrr4_),
	.bswrs4 =	oabs(bswr4_),
	.bssr4 = 	oabs(bssr4_),		/* XXX swap? */
	.bscr4 =	oabs(bscr4_)		/* XXX swap? */
};
