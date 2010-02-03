/*	$NetBSD: busfuncs.c,v 1.11 2010/02/03 13:56:53 phx Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: busfuncs.c,v 1.11 2010/02/03 13:56:53 phx Exp $");

/*
 * Amiga bus access methods for data widths > 1
 * XXX currently, only 16bit methods are defined
 */

#include <machine/bus.h>

bsr(amiga_contiguous_read_2, u_int16_t);
bsw(amiga_contiguous_write_2, u_int16_t);
bsrm(amiga_contiguous_read_multi_2, u_int16_t);
bswm(amiga_contiguous_write_multi_2, u_int16_t);
bsrm(amiga_contiguous_read_region_2, u_int16_t);
bswm(amiga_contiguous_write_region_2, u_int16_t);
bssr(amiga_contiguous_set_region_2, u_int16_t);
bscr(amiga_contiguous_copy_region_2, u_int16_t);

bsr(amiga_interleaved_read_2, u_int16_t);
bsw(amiga_interleaved_write_2, u_int16_t);
bsrm(amiga_interleaved_read_multi_2, u_int16_t);
bswm(amiga_interleaved_write_multi_2, u_int16_t);
bsrm(amiga_interleaved_read_region_2, u_int16_t);
bswm(amiga_interleaved_write_region_2, u_int16_t);
bssr(amiga_interleaved_set_region_2, u_int16_t);
bscr(amiga_interleaved_copy_region_2, u_int16_t);

bsr(amiga_interleaved_wordaccess_read_2, u_int16_t);
bsw(amiga_interleaved_wordaccess_write_2, u_int16_t);
bsrm(amiga_interleaved_wordaccess_read_multi_2, u_int16_t);
bswm(amiga_interleaved_wordaccess_write_multi_2, u_int16_t);
bsrm(amiga_interleaved_wordaccess_read_region_2, u_int16_t);
bswm(amiga_interleaved_wordaccess_write_region_2, u_int16_t);
bssr(amiga_interleaved_wordaccess_set_region_2, u_int16_t);
bscr(amiga_interleaved_wordaccess_copy_region_2, u_int16_t);

const struct amiga_bus_space_methods amiga_contiguous_methods = {
	amiga_contiguous_read_2,
	amiga_contiguous_write_2,
	amiga_contiguous_read_multi_2,
	amiga_contiguous_write_multi_2,
	amiga_contiguous_read_region_2,
	amiga_contiguous_write_region_2,
	/* next two identical to the above here */
	amiga_contiguous_read_region_2,
	amiga_contiguous_write_region_2,
	amiga_contiguous_set_region_2,
	amiga_contiguous_copy_region_2
};

const struct amiga_bus_space_methods amiga_interleaved_methods = {
	amiga_interleaved_read_2,
	amiga_interleaved_write_2,
	amiga_interleaved_read_multi_2,
	amiga_interleaved_write_multi_2,
	amiga_interleaved_read_region_2,
	amiga_interleaved_write_region_2,
	/* next two identical to the above here */
	amiga_interleaved_read_region_2,
	amiga_interleaved_write_region_2,
	amiga_interleaved_set_region_2,
	amiga_interleaved_copy_region_2
};

const struct amiga_bus_space_methods amiga_interleaved_wordaccess_methods = {
	amiga_interleaved_wordaccess_read_2,
	amiga_interleaved_wordaccess_write_2,
	amiga_interleaved_wordaccess_read_multi_2,
	amiga_interleaved_wordaccess_write_multi_2,
	amiga_interleaved_wordaccess_read_region_2,
	amiga_interleaved_wordaccess_write_region_2,
	/* next two identical to the above here */
	amiga_interleaved_wordaccess_read_region_2,	/* region_stream */
	amiga_interleaved_wordaccess_write_region_2,
	amiga_interleaved_wordaccess_set_region_2,
	amiga_interleaved_wordaccess_copy_region_2
};

/*
 * Contiguous methods
 * Be sure to only create word accesses.
 * For busses that _need_ split byte accesses, use the interleaved functions.
 */

u_int16_t
amiga_contiguous_read_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{
	/* ARGSUSED */
	u_int16_t x;

	x  = * (u_int16_t *) (h + o); /* only used if t->stride == 0 */
	amiga_bus_reorder_protect();
	return x;
}

void
amiga_contiguous_write_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v)
{
	/* ARGSUSED */
	* (u_int16_t *) (h + o) = v;
	amiga_bus_reorder_protect();
}

void
amiga_contiguous_read_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *p, bus_size_t s)
{
	/* ARGSUSED */
	volatile u_int16_t *q = (volatile u_int16_t *)(h + o);

	while (s-- > 0) {
		*p++ =  *q;
		amiga_bus_reorder_protect();
	}
}

void
amiga_contiguous_write_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *p, bus_size_t s)
{
	/* ARGSUSED */
	volatile u_int16_t *q = (volatile u_int16_t *)(h + o);

	while (s-- > 0) {
		*q = *p++;
		amiga_bus_reorder_protect();
	}
}

void
amiga_contiguous_read_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *p, bus_size_t s)
{
	/* ARGSUSED */
	volatile u_int16_t *q = (volatile u_int16_t *)(h + o);

	while (s-- > 0) {
		*p++ =  *q++;
		amiga_bus_reorder_protect();
	}
}

void
amiga_contiguous_write_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *p, bus_size_t s)
{
	/* ARGSUSED */
	volatile u_int16_t *q = (volatile u_int16_t *)(h + o);

	while (s-- > 0) {
		*q++ = *p++;
		amiga_bus_reorder_protect();
	}
}

void
amiga_contiguous_set_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v, bus_size_t s)
{
	/* ARGSUSED */
	volatile u_int16_t *q = (volatile u_int16_t *)(h + o);

	while (s-- > 0) {
		*q++ = v;
		amiga_bus_reorder_protect();
	}
}

void
amiga_contiguous_copy_region_2(bus_space_tag_t t, bus_space_handle_t srch, bus_size_t srco, bus_space_handle_t dsth, bus_size_t dsto, bus_size_t s)
{
	/* ARGSUSED */
	volatile u_int16_t *p = (volatile u_int16_t *)(srch + srco);
	volatile u_int16_t *q = (volatile u_int16_t *)(dsth + dsto);

	while (s-- > 0) {
		*q++ = *p++;
		amiga_bus_reorder_protect();
	}
}

/*
 * Interleaved methods.
 * These use single-byte acceses. In case of stride = 0, the contiguous
 * methods are preferred, as they only create word accesses.
 */

u_int16_t
amiga_interleaved_read_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{
	volatile u_int8_t *q;
	u_int16_t x;
	int step;

	step = 1 << t->stride;
	q = (volatile u_int8_t *)(h + (o << t->stride));
	x = ((*q) << 8) | *(q + step);
	amiga_bus_reorder_protect();
	return x;
}

void
amiga_interleaved_write_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v)
{
	volatile u_int8_t *q;
	int step;

	step = 1 << t->stride;
	q = (volatile u_int8_t *)(h + (o << t->stride));

	*q = v >> 8;
	*(q+step) = v;
	amiga_bus_reorder_protect();
}

void
amiga_interleaved_read_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *p, bus_size_t s)
{
	volatile u_int8_t *q;
	int step;

	step = 1 << t->stride;
	q = (volatile u_int8_t *)(h + (o << t->stride));

	while (s-- > 0) {
		*p++ =  ((*q)<<8) | *(q+step);
		amiga_bus_reorder_protect();
	}
}

void
amiga_interleaved_write_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *p, bus_size_t s)
{
	volatile u_int8_t *q;
	int step;
	u_int16_t v;

	step = 1 << t->stride;
	q = (volatile u_int8_t *)(h + (o << t->stride));

	while (s-- > 0) {
		v = *p++;
		*q 		= v>>8;
		*(q + step)	= v;
		amiga_bus_reorder_protect();
	}
}

void
amiga_interleaved_read_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *p, bus_size_t s)
{
	volatile u_int8_t *q;
	int step;
	u_int16_t v;

	step = 1 << t->stride;
	q = (volatile u_int8_t *)(h + (o << t->stride));

	while (s-- > 0) {
		v = (*q) << 8;
		q += step;
		v |= *q;
		amiga_bus_reorder_protect();
		q += step;
		*p++ =  v;
	}
}

void
amiga_interleaved_write_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *p, bus_size_t s)
{
	volatile u_int8_t *q;
	int step;
	u_int16_t v;

	step = 1 << t->stride;
	q = (volatile u_int8_t *)(h + (o << t->stride));

	while (s-- > 0) {
		v = *p++;
		*q = v >> 8;
		q += step;
		*q = v;
		amiga_bus_reorder_protect();
		q += step;
	}
}

void
amiga_interleaved_set_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v, bus_size_t s)
{
	int step;
	volatile u_int16_t *q = (volatile u_int16_t *)(h + o);

	step = 1 << t->stride;

	while (s-- > 0) {
		*q = v;
		amiga_bus_reorder_protect();
		q += step;
	}
}

void
amiga_interleaved_copy_region_2(bus_space_tag_t t, bus_space_handle_t srch, bus_size_t srco, bus_space_handle_t dsth, bus_size_t dsto, bus_size_t s)
{
	int step;
	volatile u_int16_t *p = (volatile u_int16_t *)(srch + srco);
	volatile u_int16_t *q = (volatile u_int16_t *)(dsth + dsto);

	step = 1 << t->stride;

	while (s-- > 0) {
		*q = *p;
		amiga_bus_reorder_protect();
		p += step;
		q += step;
	}
}

/*
 * Interleaved_wordaccess methods. Have a stride, but translate
 * word accesses to word accesses at the target address.
 */

u_int16_t
amiga_interleaved_wordaccess_read_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{
	/* ARGSUSED */
	u_int16_t x;

	x = * (u_int16_t *) (h + (o << t->stride));
	amiga_bus_reorder_protect();
	return x;
}

void
amiga_interleaved_wordaccess_write_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v)
{
	/* ARGSUSED */
	* (u_int16_t *) (h + (o << t->stride)) = v;
	amiga_bus_reorder_protect();
}

void
amiga_interleaved_wordaccess_read_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *p, bus_size_t s)
{
	/* ARGSUSED */
	volatile u_int16_t *q;

	q = (volatile u_int16_t *)(h + (o << t->stride));

	while (s-- > 0) {
		*p++ =  *q;
		amiga_bus_reorder_protect();
	}
}

void
amiga_interleaved_wordaccess_write_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *p, bus_size_t s)
{
	/* ARGSUSED */
	volatile u_int16_t *q;

	q = (volatile u_int16_t *)(h + (o << t->stride));

	while (s-- > 0) {
		*q = *p++;
		amiga_bus_reorder_protect();
	}
}

void
amiga_interleaved_wordaccess_read_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *p, bus_size_t s)
{
	/* ARGSUSED */
	volatile u_int16_t *q;
	int step;

	q = (volatile u_int16_t *)(h + (o << t->stride));
	step = (1 << t->stride) / sizeof(u_int16_t);

	while (s-- > 0) {
		*p++ =  *q;
		amiga_bus_reorder_protect();
		q += step;
	}
}

void
amiga_interleaved_wordaccess_write_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *p, bus_size_t s)
{
	/* ARGSUSED */
	volatile u_int16_t *q;
	int step;

	q = (volatile u_int16_t *)(h + (o << t->stride));
	step = (1 << t->stride) / sizeof(u_int16_t);

	while (s-- > 0) {
		*q = *p++;
		amiga_bus_reorder_protect();
		q += step;
	}
}

void
amiga_interleaved_wordaccess_set_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v, bus_size_t s)
{
	/* ARGSUSED */
	volatile u_int16_t *q;
	int step;

	q = (volatile u_int16_t *)(h + (o << t->stride));
	step = (1 << t->stride) / sizeof(u_int16_t);

	while (s-- > 0) {
		*q = v;
		amiga_bus_reorder_protect();
		q += step;
	}
}

void
amiga_interleaved_wordaccess_copy_region_2(bus_space_tag_t t, bus_space_handle_t srch, bus_size_t srco, bus_space_handle_t dsth, bus_size_t dsto, bus_size_t s)
{
	int step;
	/* ARGSUSED */
	volatile u_int16_t *p;
	volatile u_int16_t *q;

	p = (volatile u_int16_t *)(srch + (srco << t->stride));
	q = (volatile u_int16_t *)(dsth + (dsto << t->stride));
	step = (1 << t->stride) / sizeof(u_int16_t);

	while (s-- > 0) {
		*q = *p;
		amiga_bus_reorder_protect();
		q += step;
		p += step;
	}
}
