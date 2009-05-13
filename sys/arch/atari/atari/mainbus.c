/*	$NetBSD: mainbus.c,v 1.8.2.1 2009/05/13 17:16:21 jym Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.8.2.1 2009/05/13 17:16:21 jym Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <machine/cpu.h>
#include <machine/bus.h>

static int		mb_bus_space_peek_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static int		mb_bus_space_peek_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static int		mb_bus_space_peek_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static int		mb_bus_space_peek_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static uint8_t		mb_bus_space_read_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static uint16_t		mb_bus_space_read_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static uint32_t		mb_bus_space_read_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static uint64_t		mb_bus_space_read_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t);
static void		mb_bus_space_write_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint8_t);
static void		mb_bus_space_write_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint16_t);
static void		mb_bus_space_write_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint32_t);
static void		mb_bus_space_write_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint64_t);
static void		mb_bus_space_read_multi_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint8_t *,
				bus_size_t);
static void		mb_bus_space_read_multi_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint16_t *,
				bus_size_t);
static void		mb_bus_space_read_multi_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint32_t *,
				bus_size_t);
static void		mb_bus_space_read_multi_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint64_t *,
				bus_size_t);
static void		mb_bus_space_write_multi_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint8_t *, bus_size_t);
static void		mb_bus_space_write_multi_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint16_t *, bus_size_t);
static void		mb_bus_space_write_multi_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint32_t *, bus_size_t);
static void		mb_bus_space_write_multi_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint64_t *, bus_size_t);
static void		mb_bus_space_read_region_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint8_t *,
				bus_size_t);
static void		mb_bus_space_read_region_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint16_t *,
				bus_size_t);
static void		mb_bus_space_read_region_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint32_t *,
				bus_size_t);
static void		mb_bus_space_read_region_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint64_t *,
				bus_size_t);
static void		mb_bus_space_write_region_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint8_t *, bus_size_t);
static void		mb_bus_space_write_region_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint16_t *, bus_size_t);
static void		mb_bus_space_write_region_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint32_t *, bus_size_t);
static void		mb_bus_space_write_region_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t,
				const uint64_t *, bus_size_t);
static void		mb_bus_space_set_multi_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint8_t,
				bus_size_t);
static void		mb_bus_space_set_multi_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint16_t,
				bus_size_t);
static void		mb_bus_space_set_multi_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint32_t,
				bus_size_t);
static void		mb_bus_space_set_multi_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint64_t,
				bus_size_t);
static void		mb_bus_space_set_region_1(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint8_t,
				bus_size_t);
static void		mb_bus_space_set_region_2(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint16_t,
				bus_size_t);
static void		mb_bus_space_set_region_4(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint32_t,
				bus_size_t);
static void		mb_bus_space_set_region_8(bus_space_tag_t,
				bus_space_handle_t, bus_size_t, uint64_t,
				bus_size_t);
/*
 * Calculate offset on the mainbus given a stride_shift and width_offset
 */
#define calc_addr(base, off, stride, wm)	\
	((u_long)(base) + ((off) << (stride)) + (wm))

#define __read_1(t, h, o)	\
	(*((volatile uint8_t *)(calc_addr(h, o, (t)->stride, (t)->wo_1))))
#define __read_2(t, h, o)	\
	(*((volatile uint16_t *)(calc_addr(h, o, (t)->stride, (t)->wo_2))))
#define	__read_4(t, h, o)	\
	(*((volatile uint32_t *)(calc_addr(h, o, (t)->stride, (t)->wo_4))))
#define	__read_8(t, h, o)	\
	(*((volatile uint64_t *)(calc_addr(h, o, (t)->stride, (t)->wo_8))))

#define __write_1(t, h, o, v)	\
    *((volatile uint8_t *)(calc_addr(h, o, (t)->stride, (t)->wo_1))) = v

#define __write_2(t, h, o, v)	\
    *((volatile uint16_t *)(calc_addr(h, o, (t)->stride, (t)->wo_2))) = v

#define __write_4(t, h, o, v)	\
    *((volatile uint32_t *)(calc_addr(h, o, (t)->stride, (t)->wo_4))) = v

#define __write_8(t, h, o, v)	\
    *((volatile uint64_t *)(calc_addr(h, o, (t)->stride, (t)->wo_8))) = v

bus_space_tag_t
mb_alloc_bus_space_tag(void)
{
	bus_space_tag_t	mb_t;

	/* Not really M_TEMP, is it.. */
	mb_t = malloc(sizeof(*mb_t), M_TEMP, M_NOWAIT|M_ZERO);
	if (mb_t == NULL)
		return NULL;
	
	mb_t->abs_p_1   = mb_bus_space_peek_1;
	mb_t->abs_p_2   = mb_bus_space_peek_2;
	mb_t->abs_p_4   = mb_bus_space_peek_4;
	mb_t->abs_p_8   = mb_bus_space_peek_8;
	mb_t->abs_r_1   = mb_bus_space_read_1;
	mb_t->abs_r_2   = mb_bus_space_read_2;
	mb_t->abs_r_4   = mb_bus_space_read_4;
	mb_t->abs_r_8   = mb_bus_space_read_8;
	mb_t->abs_rs_1  = mb_bus_space_read_1;
	mb_t->abs_rs_2  = mb_bus_space_read_2;
	mb_t->abs_rs_4  = mb_bus_space_read_4;
	mb_t->abs_rs_8  = mb_bus_space_read_8;
	mb_t->abs_rm_1  = mb_bus_space_read_multi_1;
	mb_t->abs_rm_2  = mb_bus_space_read_multi_2;
	mb_t->abs_rm_4  = mb_bus_space_read_multi_4;
	mb_t->abs_rm_8  = mb_bus_space_read_multi_8;
	mb_t->abs_rms_1 = mb_bus_space_read_multi_1;
	mb_t->abs_rms_2 = mb_bus_space_read_multi_2;
	mb_t->abs_rms_4 = mb_bus_space_read_multi_4;
	mb_t->abs_rms_8 = mb_bus_space_read_multi_8;
	mb_t->abs_rr_1  = mb_bus_space_read_region_1;
	mb_t->abs_rr_2  = mb_bus_space_read_region_2;
	mb_t->abs_rr_4  = mb_bus_space_read_region_4;
	mb_t->abs_rr_8  = mb_bus_space_read_region_8;
	mb_t->abs_rrs_1 = mb_bus_space_read_region_1;
	mb_t->abs_rrs_2 = mb_bus_space_read_region_2;
	mb_t->abs_rrs_4 = mb_bus_space_read_region_4;
	mb_t->abs_rrs_8 = mb_bus_space_read_region_8;
	mb_t->abs_w_1   = mb_bus_space_write_1;
	mb_t->abs_w_2   = mb_bus_space_write_2;
	mb_t->abs_w_4   = mb_bus_space_write_4;
	mb_t->abs_w_8   = mb_bus_space_write_8;
	mb_t->abs_ws_1  = mb_bus_space_write_1;
	mb_t->abs_ws_2  = mb_bus_space_write_2;
	mb_t->abs_ws_4  = mb_bus_space_write_4;
	mb_t->abs_ws_8  = mb_bus_space_write_8;
	mb_t->abs_wm_1  = mb_bus_space_write_multi_1;
	mb_t->abs_wm_2  = mb_bus_space_write_multi_2;
	mb_t->abs_wm_4  = mb_bus_space_write_multi_4;
	mb_t->abs_wm_8  = mb_bus_space_write_multi_8;
	mb_t->abs_wms_1 = mb_bus_space_write_multi_1;
	mb_t->abs_wms_2 = mb_bus_space_write_multi_2;
	mb_t->abs_wms_4 = mb_bus_space_write_multi_4;
	mb_t->abs_wms_8 = mb_bus_space_write_multi_8;
	mb_t->abs_wr_1  = mb_bus_space_write_region_1;
	mb_t->abs_wr_2  = mb_bus_space_write_region_2;
	mb_t->abs_wr_4  = mb_bus_space_write_region_4;
	mb_t->abs_wr_8  = mb_bus_space_write_region_8;
	mb_t->abs_wrs_1 = mb_bus_space_write_region_1;
	mb_t->abs_wrs_2 = mb_bus_space_write_region_2;
	mb_t->abs_wrs_4 = mb_bus_space_write_region_4;
	mb_t->abs_wrs_8 = mb_bus_space_write_region_8;
	mb_t->abs_sm_1  = mb_bus_space_set_multi_1;
	mb_t->abs_sm_2  = mb_bus_space_set_multi_2;
	mb_t->abs_sm_4  = mb_bus_space_set_multi_4;
	mb_t->abs_sm_8  = mb_bus_space_set_multi_8;
	mb_t->abs_sr_1  = mb_bus_space_set_region_1;
	mb_t->abs_sr_2  = mb_bus_space_set_region_2;
	mb_t->abs_sr_4  = mb_bus_space_set_region_4;
	mb_t->abs_sr_8  = mb_bus_space_set_region_8;

	return mb_t;
}

void
mb_free_bus_space_tag(bus_space_tag_t mb_t)
{

	/* Not really M_TEMP, is it.. */
	free(mb_t, M_TEMP);
}

static int
mb_bus_space_peek_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return !badbaddr((void *)(calc_addr(h, o, t->stride, t->wo_1)), 1);
}

static int 
mb_bus_space_peek_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return !badbaddr((void *)(calc_addr(h, o, t->stride, t->wo_2)), 2);
}

static int 
mb_bus_space_peek_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return !badbaddr((void *)(calc_addr(h, o, t->stride, t->wo_4)), 4);
}

static int 
mb_bus_space_peek_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return !badbaddr((void *)(calc_addr(h, o, t->stride, t->wo_8)), 8);
}

static uint8_t
mb_bus_space_read_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return __read_1(t, h, o);
}

static uint16_t
mb_bus_space_read_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return __read_2(t, h, o);
}

static uint32_t
mb_bus_space_read_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return __read_4(t, h, o);
}

static uint64_t
mb_bus_space_read_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	return __read_8(t, h, o);
}

static void
mb_bus_space_write_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint8_t v)
{

	__write_1(t, h, o, v);
}

static void
mb_bus_space_write_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint16_t v)
{

	__write_2(t, h, o, v);
}

static void
mb_bus_space_write_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint32_t v)
{

	__write_4(t, h, o, v);
}

static void
mb_bus_space_write_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint64_t v)
{

	__write_8(t, h, o, v);
}


static void
mb_bus_space_read_multi_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t *a, bus_size_t c)
{
	volatile uint8_t	*ba;

	ba = (volatile uint8_t *)calc_addr(h, o, t->stride, t->wo_1);
	for (; c; a++, c--)
		*a = *ba;
}

static void
mb_bus_space_read_multi_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t *a, bus_size_t c)
{
	volatile uint16_t	*ba;

	ba = (volatile uint16_t *)calc_addr(h, o, t->stride, t->wo_2);
	for (; c; a++, c--)
		*a = *ba;
}

static void
mb_bus_space_read_multi_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t *a, bus_size_t c)
{
	volatile uint32_t	*ba;

	ba = (volatile uint32_t *)calc_addr(h, o, t->stride, t->wo_4);
	for (; c; a++, c--)
		*a = *ba;
}

static void
mb_bus_space_read_multi_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t *a, bus_size_t c)
{
	volatile uint64_t	*ba;

	ba = (volatile uint64_t *)calc_addr(h, o, t->stride, t->wo_8);
	for (; c; a++, c--)
		*a = *ba;
}

static void
mb_bus_space_write_multi_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint8_t *a, bus_size_t c)
{
	volatile uint8_t	*ba;

	ba = (volatile uint8_t *)calc_addr(h, o, t->stride, t->wo_1);
	for (; c; a++, c--)
		*ba = *a;
}

static void
mb_bus_space_write_multi_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint16_t *a, bus_size_t c)
{
	volatile uint16_t	*ba;

	ba = (volatile uint16_t *)calc_addr(h, o, t->stride, t->wo_2);
	for (; c; a++, c--)
		*ba = *a;
}

static void
mb_bus_space_write_multi_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint32_t *a, bus_size_t c)
{
	volatile uint32_t	*ba;

	ba = (volatile uint32_t *)calc_addr(h, o, t->stride, t->wo_4);
	for (; c; a++, c--)
		*ba = *a;
}

static void
mb_bus_space_write_multi_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint64_t *a, bus_size_t c)
{
	volatile uint64_t	*ba;

	ba = (volatile uint64_t *)calc_addr(h, o, t->stride, t->wo_8);
	for (; c; a++, c--)
		*ba = *a;
}

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset,
 *		uintN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */
static void
mb_bus_space_read_region_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t *a, bus_size_t c)
{

	for (; c; a++, o++, c--)
		*a = __read_1(t, h, o);
}

static void
mb_bus_space_read_region_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t *a, bus_size_t c)
{

	for (; c; a++, o += 2, c--)
		*a = __read_2(t, h, o);
}

static void
mb_bus_space_read_region_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t *a, bus_size_t c)
{

	for (; c; a++, o += 4, c--)
		*a = __read_4(t, h, o);
}

static void
mb_bus_space_read_region_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t *a, bus_size_t c)
{

	for (; c; a++, o += 8, c--)
		*a = __read_8(t, h, o);
}

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset,
 *		uintN_t *addr, size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * into the bus space described by tag/handle and starting at `offset'.
 */
static void
mb_bus_space_write_region_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint8_t *a, bus_size_t c)
{

	for (; c; a++, o++, c--)
		__write_1(t, h, o, *a);
}

static void
mb_bus_space_write_region_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint16_t *a, bus_size_t c)
{

	for (; c; a++, o += 2, c--)
		__write_2(t, h, o, *a);
}

static void
mb_bus_space_write_region_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint32_t *a, bus_size_t c)
{

	for (; c; a++, o += 4, c--)
		__write_4(t, h, o, *a);
}

static void
mb_bus_space_write_region_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const uint64_t *a, bus_size_t c)
{

	for (; c; a++, o += 8, c--)
		__write_8(t, h, o, *a);
}

/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *		size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */

static void
mb_bus_space_set_multi_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t v, bus_size_t c)
{
	volatile uint8_t	*ba;

	ba = (volatile uint8_t *)calc_addr(h, o, t->stride, t->wo_1);
	for (; c; c--)
		*ba = v;
}

static void
mb_bus_space_set_multi_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t v, bus_size_t c)
{
	volatile uint16_t	*ba;

	ba = (volatile uint16_t *)calc_addr(h, o, t->stride, t->wo_2);
	for (; c; c--)
		*ba = v;
}

static void
mb_bus_space_set_multi_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t v, bus_size_t c)
{
	volatile uint32_t	*ba;

	ba = (volatile uint32_t *)calc_addr(h, o, t->stride, t->wo_4);
	for (; c; c--)
		*ba = v;
}

static void
mb_bus_space_set_multi_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t v, bus_size_t c)
{
	volatile uint64_t	*ba;

	ba = (volatile uint64_t *)calc_addr(h, o, t->stride, t->wo_8);
	for (; c; c--)
		*ba = v;
}

/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *		size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
static void
mb_bus_space_set_region_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t v, bus_size_t c)
{

	for (; c; o++, c--)
		__write_1(t, h, o, v);
}

static void
mb_bus_space_set_region_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t v, bus_size_t c)
{

	for (; c; o += 2, c--)
		__write_2(t, h, o, v);
}

static void
mb_bus_space_set_region_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t v, bus_size_t c)
{

	for (; c; o += 4, c--)
		__write_4(t, h, o, v);
}

static void
mb_bus_space_set_region_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t v, bus_size_t c)
{

	for (; c; o += 8, c--)
		__write_8(t, h, o, v);
}
