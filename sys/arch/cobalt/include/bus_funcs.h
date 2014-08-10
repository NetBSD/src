/*	$NetBSD: bus_funcs.h,v 1.1.26.1 2014/08/10 06:53:53 tls Exp $	*/

/*
 * Copyright (c) 1996, 1997, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#ifndef _COBALT_BUS_FUNCS_H_
#define _COBALT_BUS_FUNCS_H_

/*
 *	uintN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define	bus_space_read_1(t, h, o)					\
     ((void) t, (*(volatile uint8_t *)((h) + (o))))

#define	bus_space_read_2(t, h, o)					\
     ((void) t, (*(volatile uint16_t *)((h) + (o))))

#define	bus_space_read_4(t, h, o)					\
     ((void) t, (*(volatile uint32_t *)((h) + (o))))

#if 0	/* Cause a link error for bus_space_read_8 */
#define	bus_space_read_8(t, h, o)	!!! bus_space_read_8 unimplemented !!!
#endif

/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define	bus_space_write_1(t, h, o, v)					\
do {									\
	(void) t;							\
	*(volatile uint8_t *)((h) + (o)) = (v);			\
} while (0)

#define	bus_space_write_2(t, h, o, v)					\
do {									\
	(void) t;							\
	*(volatile uint16_t *)((h) + (o)) = (v);			\
} while (0)

#define	bus_space_write_4(t, h, o, v)					\
do {									\
	(void) t;							\
	*(volatile uint32_t *)((h) + (o)) = (v);			\
} while (0)

#if 0	/* Cause a link error for bus_space_write_8 */
#define	bus_space_write_8	!!! bus_space_write_8 not implemented !!!
#endif

/*
 * Operations which handle byte stream data on word access.
 *
 * These functions are defined to resolve endian mismatch, by either
 * - When normal (i.e. stream-less) operations perform byte swap
 *   to resolve endian mismatch, these functions bypass the byte swap.
 * or
 * - When bus bridge performs automatic byte swap, these functions
 *   perform byte swap once more, to cancel the bridge's behavior.
 *
 * Currently these are just same as normal operations, since all
 * supported buses are same endian with CPU (i.e. little-endian).
 *
 */
#define bus_space_read_stream_2(tag, bsh, offset)			\
	bus_space_read_2(tag, bsh, offset)
#define bus_space_read_stream_4(tag, bsh, offset)			\
	bus_space_read_4(tag, bsh, offset)
#define bus_space_read_stream_8(tag, bsh, offset)			\
	bus_space_read_8(tag, bsh, offset)
#define bus_space_read_multi_stream_2(tag, bsh, offset, datap, count)	\
	bus_space_read_multi_2(tag, bsh, offset, datap, count)
#define bus_space_read_multi_stream_4(tag, bsh, offset, datap, count)	\
	bus_space_read_multi_4(tag, bsh, offset, datap, count)
#define bus_space_read_multi_stream_8(tag, bsh, offset, datap, count)	\
	bus_space_read_multi_8(tag, bsh, offset, datap, count)
#define bus_space_read_region_stream_2(tag, bsh, offset, datap, count)	\
	bus_space_read_region_2(tag, bsh, offset, datap, count)
#define bus_space_read_region_stream_4(tag, bsh, offset, datap, count)	\
	bus_space_read_region_4(tag, bsh, offset, datap, count)
#define bus_space_read_region_stream_8(tag, bsh, offset, datap, count)	\
	bus_space_read_region_8(tag, bsh, offset, datap, count)
#define bus_space_write_stream_2(tag, bsh, offset, data)		\
	bus_space_write_2(tag, bsh, offset, data)
#define bus_space_write_stream_4(tag, bsh, offset, data)		\
	bus_space_write_4(tag, bsh, offset, data)
#define bus_space_write_stream_8(tag, bsh, offset, data)		\
	bus_space_write_8(tag, bsh, offset, data)
#define bus_space_write_multi_stream_2(tag, bsh, offset, datap, count)	\
	bus_space_write_multi_2(tag, bsh, offset, datap, count)
#define bus_space_write_multi_stream_4(tag, bsh, offset, datap, count)	\
	bus_space_write_multi_4(tag, bsh, offset, datap, count)
#define bus_space_write_multi_stream_8(tag, bsh, offset, datap, count)	\
	bus_space_write_multi_8(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_2(tag, bsh, offset, datap, count)	\
	bus_space_write_region_2(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_4(tag, bsh, offset, datap, count)	\
	bus_space_write_region_4(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_8(tag, bsh, offset, datap, count)	\
	bus_space_write_region_8(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_2(tag, bsh, offset, datap, count)	\
	bus_space_write_region_2(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_4(tag, bsh, offset, datap, count)	\
	bus_space_write_region_4(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_8(tag, bsh, offset, datap, count)	\
	bus_space_write_region_8(tag, bsh, offset, datap, count)
#define bus_space_set_multi_stream_2(tag, bsh, offset, data, count)	\
	bus_space_set_multi_2(tag, bsh, offset, data, count)
#define bus_space_set_multi_stream_4(tag, bsh, offset, data, count)	\
	bus_space_set_multi_4(tag, bsh, offset, data, count)
#define bus_space_set_multi_stream_8(tag, bsh, offset, data, count)	\
	bus_space_set_multi_8(tag, bsh, offset, data, count)
#define bus_space_set_region_stream_2(tag, bsh, offset, data, count)	\
	bus_space_set_region_2(tag, bsh, offset, data, count)
#define bus_space_set_region_stream_4(tag, bsh, offset, data, count)	\
	bus_space_set_region_4(tag, bsh, offset, data, count)
#define bus_space_set_region_stream_8(tag, bsh, offset, data, count)	\
	bus_space_set_region_8(tag, bsh, offset, data, count)

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags);
 *
 * On the MIPS, we just flush the write buffer.
 */
#define	bus_space_barrier(t, h, o, l, f)	\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f),	\
	 wbflush()))

#include <mips/bus_dma_funcs.h> 

#endif /* _COBALT_BUS_FUNCS_H_ */
