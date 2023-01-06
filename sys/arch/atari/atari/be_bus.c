/*	$NetBSD: be_bus.c,v 1.18 2023/01/06 10:28:27 tsutsui Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Leo Weppelman.
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
__KERNEL_RCSID(0, "$NetBSD: be_bus.c,v 1.18 2023/01/06 10:28:27 tsutsui Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <machine/cpu.h>
#include <sys/bus.h>

/*
 * This file contains the common functions for using a big endian (linear)
 * bus on a big endian atari.
 */

	/* Autoconf detection stuff */
static int		beb_bus_space_peek_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static int		beb_bus_space_peek_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static int		beb_bus_space_peek_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static int		beb_bus_space_peek_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);

	/* read (single) */
static uint8_t		beb_bus_space_read_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static uint16_t		beb_bus_space_read_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static uint32_t		beb_bus_space_read_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static uint64_t		beb_bus_space_read_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);

	/* write (single) */
static void		beb_bus_space_write_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint8_t);
static void		beb_bus_space_write_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint16_t);
static void		beb_bus_space_write_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint32_t);
static void		beb_bus_space_write_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint64_t);

	/* read multiple */
static void		beb_bus_space_read_multi_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint8_t *,
				bus_size_t);
static void		beb_bus_space_read_multi_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint16_t *,
				bus_size_t);
static void		beb_bus_space_read_multi_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint32_t *,
				bus_size_t);
static void		beb_bus_space_read_multi_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint64_t *,
				bus_size_t);

	/* write multiple */
static void		beb_bus_space_write_multi_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint8_t *, bus_size_t);
static void		beb_bus_space_write_multi_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint16_t *, bus_size_t);
static void		beb_bus_space_write_multi_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint32_t *, bus_size_t);
static void		beb_bus_space_write_multi_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint64_t *, bus_size_t);

	/* read region */
static void		beb_bus_space_read_region_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint8_t *,
				bus_size_t);
static void		beb_bus_space_read_region_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint16_t *,
				bus_size_t);
static void		beb_bus_space_read_region_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint32_t *,
				bus_size_t);
static void		beb_bus_space_read_region_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint64_t *,
				bus_size_t);

	/* read region */
static void		beb_bus_space_write_region_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint8_t *, bus_size_t);
static void		beb_bus_space_write_region_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint16_t *, bus_size_t);
static void		beb_bus_space_write_region_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint32_t *, bus_size_t);
static void		beb_bus_space_write_region_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint64_t *, bus_size_t);

	/* set multi */
static void		beb_bus_space_set_multi_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint8_t,
				bus_size_t);
static void		beb_bus_space_set_multi_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint16_t,
				bus_size_t);
static void		beb_bus_space_set_multi_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint32_t,
				bus_size_t);
static void		beb_bus_space_set_multi_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint64_t,
				bus_size_t);

	/* set region */
static void		beb_bus_space_set_region_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint8_t,
				bus_size_t);
static void		beb_bus_space_set_region_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint16_t,
				bus_size_t);
static void		beb_bus_space_set_region_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint32_t,
				bus_size_t);
static void		beb_bus_space_set_region_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint64_t,
				bus_size_t);

/*
 * Don't force a function call overhead on these primitives...
 */
#define __read_1(h, o)		*((volatile uint8_t  *)((h) + (o)))
#define __read_2(h, o)		*((volatile uint16_t *)((h) + (o)))
#define __read_4(h, o)		*((volatile uint32_t *)((h) + (o)))
#define __read_8(h, o)		*((volatile uint64_t *)((h) + (o)))

#define __write_1(h, o, v)	*((volatile uint8_t  *)((h) + (o))) = (v)
#define __write_2(h, o, v)	*((volatile uint16_t *)((h) + (o))) = (v)
#define __write_4(h, o, v)	*((volatile uint32_t *)((h) + (o))) = (v)
#define __write_8(h, o, v)	*((volatile uint64_t *)((h) + (o))) = (v)

bus_space_tag_t
beb_alloc_bus_space_tag(bus_space_tag_t storage)
{
	bus_space_tag_t	beb_t;

	/*
	 * Allow the caller to specify storage space for the tag. This
	 * is used during console config (when kmem_alloc() can't be used).
	 */
	if (storage != NULL)
		beb_t = storage;
	else {
		beb_t = kmem_alloc(sizeof(*beb_t), KM_SLEEP);
	}
	memset(beb_t, 0, sizeof(*beb_t));

	beb_t->abs_p_1   = beb_bus_space_peek_1;
	beb_t->abs_p_2   = beb_bus_space_peek_2;
	beb_t->abs_p_4   = beb_bus_space_peek_4;
	beb_t->abs_p_8   = beb_bus_space_peek_8;
	beb_t->abs_r_1   = beb_bus_space_read_1;
	beb_t->abs_r_2   = beb_bus_space_read_2;
	beb_t->abs_r_4   = beb_bus_space_read_4;
	beb_t->abs_r_8   = beb_bus_space_read_8;
	beb_t->abs_rs_1  = beb_bus_space_read_1;
	beb_t->abs_rs_2  = beb_bus_space_read_2;
	beb_t->abs_rs_4  = beb_bus_space_read_4;
	beb_t->abs_rs_8  = beb_bus_space_read_8;
	beb_t->abs_rm_1  = beb_bus_space_read_multi_1;
	beb_t->abs_rm_2  = beb_bus_space_read_multi_2;
	beb_t->abs_rm_4  = beb_bus_space_read_multi_4;
	beb_t->abs_rm_8  = beb_bus_space_read_multi_8;
	beb_t->abs_rms_1 = beb_bus_space_read_multi_1;
	beb_t->abs_rms_2 = beb_bus_space_read_multi_2;
	beb_t->abs_rms_4 = beb_bus_space_read_multi_4;
	beb_t->abs_rms_8 = beb_bus_space_read_multi_8;
	beb_t->abs_rr_1  = beb_bus_space_read_region_1;
	beb_t->abs_rr_2  = beb_bus_space_read_region_2;
	beb_t->abs_rr_4  = beb_bus_space_read_region_4;
	beb_t->abs_rr_8  = beb_bus_space_read_region_8;
	beb_t->abs_rrs_1 = beb_bus_space_read_region_1;
	beb_t->abs_rrs_2 = beb_bus_space_read_region_2;
	beb_t->abs_rrs_4 = beb_bus_space_read_region_4;
	beb_t->abs_rrs_8 = beb_bus_space_read_region_8;
	beb_t->abs_w_1   = beb_bus_space_write_1;
	beb_t->abs_w_2   = beb_bus_space_write_2;
	beb_t->abs_w_4   = beb_bus_space_write_4;
	beb_t->abs_w_8   = beb_bus_space_write_8;
	beb_t->abs_ws_1  = beb_bus_space_write_1;
	beb_t->abs_ws_2  = beb_bus_space_write_2;
	beb_t->abs_ws_4  = beb_bus_space_write_4;
	beb_t->abs_ws_8  = beb_bus_space_write_8;
	beb_t->abs_wm_1  = beb_bus_space_write_multi_1;
	beb_t->abs_wm_2  = beb_bus_space_write_multi_2;
	beb_t->abs_wm_4  = beb_bus_space_write_multi_4;
	beb_t->abs_wm_8  = beb_bus_space_write_multi_8;
	beb_t->abs_wms_1 = beb_bus_space_write_multi_1;
	beb_t->abs_wms_2 = beb_bus_space_write_multi_2;
	beb_t->abs_wms_4 = beb_bus_space_write_multi_4;
	beb_t->abs_wms_8 = beb_bus_space_write_multi_8;
	beb_t->abs_wr_1  = beb_bus_space_write_region_1;
	beb_t->abs_wr_2  = beb_bus_space_write_region_2;
	beb_t->abs_wr_4  = beb_bus_space_write_region_4;
	beb_t->abs_wr_8  = beb_bus_space_write_region_8;
	beb_t->abs_wrs_1 = beb_bus_space_write_region_1;
	beb_t->abs_wrs_2 = beb_bus_space_write_region_2;
	beb_t->abs_wrs_4 = beb_bus_space_write_region_4;
	beb_t->abs_wrs_8 = beb_bus_space_write_region_8;
	beb_t->abs_sm_1  = beb_bus_space_set_multi_1;
	beb_t->abs_sm_2  = beb_bus_space_set_multi_2;
	beb_t->abs_sm_4  = beb_bus_space_set_multi_4;
	beb_t->abs_sm_8  = beb_bus_space_set_multi_8;
	beb_t->abs_sr_1  = beb_bus_space_set_region_1;
	beb_t->abs_sr_2  = beb_bus_space_set_region_2;
	beb_t->abs_sr_4  = beb_bus_space_set_region_4;
	beb_t->abs_sr_8  = beb_bus_space_set_region_8;

	return beb_t;
}


/*
 * The various access functions
 */

/*
 *	int bus_space_peek_N(bus_space_tag_t tag,
 *		bus_space_handle_t sh, bus_size_t offset);
 *
 * Check if the address is suitable for reading N-byte quantities.
 */
static int
beb_bus_space_peek_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return !badbaddr((void *)(h + o), 1);
}

static int
beb_bus_space_peek_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return !badbaddr((void *)(h + o), 2);
}

static int
beb_bus_space_peek_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return !badbaddr((void *)(h + o), 4);
}

static int
beb_bus_space_peek_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return !badbaddr((void *)(h + o), 8);
}

/*
 *	uintX_t bus_space_read_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset);
 *
 * Return an 1, 2, 4, or 8 byte value read from the bus_space described
 * by tag/handle at `offset'. The value is converted to host-endian.
 */
static uint8_t
beb_bus_space_read_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return __read_1(h, o);
}

static uint16_t
beb_bus_space_read_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return __read_2(h, o);
}

static uint32_t
beb_bus_space_read_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return __read_4(h, o);
}

static uint64_t
beb_bus_space_read_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return __read_8(h, o);
}

/*
 *	uintX_t bus_space_write_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset, uintX_t val);
 *
 * Write an 1, 2, 4, or 8 byte value to the bus_space described by tag/handle
 * at `offset'. The value `val' is converted from host to bus endianness
 * before being written.
 */
static void
beb_bus_space_write_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint8_t v)
{

	__write_1(h, o, v);
}

static void
beb_bus_space_write_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint16_t v)
{

	__write_2(h, o, v);
}

static void
beb_bus_space_write_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint32_t v)
{

	__write_4(h, o, v);
}

static void
beb_bus_space_write_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint64_t v)
{

	__write_8(h, o, v);
}

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset, uintX_t *address,
 *		bus_size_t count);
 *
 * Read 'count' 1, 2, 4, or 8 byte values from the bus_space described by
 * tag/handle at `offset' and store them in the address range starting at
 * 'address'. The values are converted to CPU endian order before being
 * being stored.
 */
static void
beb_bus_space_read_multi_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t *a, bus_size_t c)
{

	for (; c; a++, c--)
		*a = __read_1(h, o);
}

static void
beb_bus_space_read_multi_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t *a, bus_size_t c)
{

	for (; c; a++, c--)
		*a = __read_2(h, o);
}

static void
beb_bus_space_read_multi_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t *a, bus_size_t c)
{

	for (; c; a++, c--)
		*a = __read_4(h, o);
}

static void
beb_bus_space_read_multi_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t *a, bus_size_t c)
{

	for (; c; a++, c--)
		*a = __read_8(h, o);
}

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset,
 *		const uintX_t *address, bus_size_t count);
 *
 * Write 'count' 1, 2, 4, or 8 byte values from the address range starting
 * at 'address' to the bus_space described by tag/handle at `offset'.
 * The values are converted to bus endian order before being written to
 * the bus.
 */
static void
beb_bus_space_write_multi_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint8_t *a, bus_size_t c)
{

	for (; c; a++, c--)
		__write_1(h, o, *a);
}

static void
beb_bus_space_write_multi_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint16_t *a, bus_size_t c)
{

	for (; c; a++, c--)
		__write_2(h, o, *a);
}

static void
beb_bus_space_write_multi_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint32_t *a, bus_size_t c)
{

	for (; c; a++, c--)
		__write_4(h, o, *a);
}

static void
beb_bus_space_write_multi_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint64_t *a, bus_size_t c)
{

	for (; c; a++, c--)
		__write_8(h, o, *a);
}

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset,
 *		uintN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */
static void
beb_bus_space_read_region_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t *a, bus_size_t c)
{

	for (; c; a++, o++, c--)
		*a = __read_1(h, o);
}

static void
beb_bus_space_read_region_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t *a, bus_size_t c)
{

	for (; c; a++, o += 2, c--)
		*a = __read_2(h, o);
}

static void
beb_bus_space_read_region_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t *a, bus_size_t c)
{

	for (; c; a++, o += 4, c--)
		*a = __read_4(h, o);
}

static void
beb_bus_space_read_region_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t *a, bus_size_t c)
{

	for (; c; a++, o += 8, c--)
		*a = __read_8(h, o);
}

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset,
 *		uintN_t *addr, bus_size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * into the bus space described by tag/handle and starting at `offset'.
 */
static void
beb_bus_space_write_region_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint8_t *a, bus_size_t c)
{

	for (; c; a++, o++, c--)
		__write_1(h, o, *a);
}

static void
beb_bus_space_write_region_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint16_t *a, bus_size_t c)
{

	for (; c; a++, o += 2, c--)
		__write_2(h, o, *a);
}

static void
beb_bus_space_write_region_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint32_t *a, bus_size_t c)
{

	for (; c; a++, o += 4, c--)
		__write_4(h, o, *a);
}

static void
beb_bus_space_write_region_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint64_t *a, bus_size_t c)
{

	for (; c; a++, o += 8, c--)
		__write_8(h, o, *a);
}

/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *		bus_size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */

static void
beb_bus_space_set_multi_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t v, bus_size_t c)
{

	for (; c; c--)
		__write_1(h, o, v);
}

static void
beb_bus_space_set_multi_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t v, bus_size_t c)
{

	for (; c; c--)
		__write_2(h, o, v);
}

static void
beb_bus_space_set_multi_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t v, bus_size_t c)
{

	for (; c; c--)
		__write_4(h, o, v);
}

static void
beb_bus_space_set_multi_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t v, bus_size_t c)
{

	for (; c; c--)
		__write_8(h, o, v);
}

/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *		bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
static void
beb_bus_space_set_region_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t v, bus_size_t c)
{

	for (; c; o++, c--)
		__write_1(h, o, v);
}

static void
beb_bus_space_set_region_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t v, bus_size_t c)
{

	for (; c; o += 2, c--)
		__write_2(h, o, v);
}

static void
beb_bus_space_set_region_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t v, bus_size_t c)
{

	for (; c; o += 4, c--)
		__write_4(h, o, v);
}

static void
beb_bus_space_set_region_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t v, bus_size_t c)
{

	for (; c; o += 8, c--)
		__write_8(h, o, v);
}
