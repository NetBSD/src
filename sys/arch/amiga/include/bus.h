/*	$NetBSD: bus.h,v 1.23 2010/02/03 13:56:53 phx Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AMIGA_BUS_H_
#define _AMIGA_BUS_H_

#include <sys/types.h>

/* for public use: */

/*
 * Memory addresses (in bus space)
 */

typedef u_int32_t bus_addr_t;
typedef u_int32_t bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
typedef struct bus_space_tag *bus_space_tag_t;
typedef u_long	bus_space_handle_t;

/* unpublic, but needed by method implementors */

/*
 * Lazyness macros for function declarations.
 */

#define bsr(what, typ) \
	typ (what)(bus_space_handle_t, bus_addr_t)

#define bsw(what, typ) \
	void (what)(bus_space_handle_t, bus_addr_t, unsigned)

#define bsrm(what, typ) \
	void (what)(bus_space_handle_t, bus_size_t, typ *, bus_size_t)

#define bswm(what, typ) \
	void (what)(bus_space_handle_t, bus_size_t, const typ *, bus_size_t)

#define bssr(what, typ) \
	void (what)(bus_space_handle_t, bus_size_t, unsigned, bus_size_t)

#define bscr(what, typ) \
	void (what)(bus_space_handle_t, bus_size_t, \
		    bus_space_handle_t, bus_size_t, bus_size_t)

/*
 * Implementation specific structures.
 * XXX Don't use outside of bus_space definitions!
 * XXX maybe this should be encapsuled in a non-global .h file?
 */ 

struct bus_space_tag {
	bus_addr_t	base;
	const struct amiga_bus_space_methods *absm;
};

struct amiga_bus_space_methods {

	/* map, unmap, etc */

	int (*bsm)(bus_space_tag_t,
		bus_addr_t, bus_size_t, int, bus_space_handle_t *);

	int (*bsms)(bus_space_handle_t,
		bus_size_t, bus_size_t, bus_space_handle_t *);

	void (*bsu)(bus_space_handle_t, bus_size_t);

	/* placeholders for currently not implemented alloc and free */

	void *bsa;
	void *bsf;

	/* 8 bit methods */

	bsr(*bsr1, u_int8_t);
	bsw(*bsw1, u_int8_t);
	bsrm(*bsrm1, u_int8_t);
	bswm(*bswm1, u_int8_t);
	bsrm(*bsrr1, u_int8_t);
	bswm(*bswr1, u_int8_t);
	bssr(*bssr1, u_int8_t);
	bscr(*bscr1, u_int8_t);

	/* 16bit methods */

	bsr(*bsr2, u_int16_t);
	bsw(*bsw2, u_int16_t);
	bsr(*bsrs2, u_int16_t);
	bsw(*bsws2, u_int16_t);
	bsrm(*bsrm2, u_int16_t);
	bswm(*bswm2, u_int16_t);
	bsrm(*bsrms2, u_int16_t);
	bswm(*bswms2, u_int16_t);
	bsrm(*bsrr2, u_int16_t);
	bswm(*bswr2, u_int16_t);
	bsrm(*bsrrs2, u_int16_t);
	bswm(*bswrs2, u_int16_t);
	bssr(*bssr2, u_int16_t);
	bscr(*bscr2, u_int16_t);

	/* add 32bit methods here */
};

/*
 * Macro definition of map, unmap, etc.
 */

#define bus_space_map(t, o, s, f, hp) \
	((t)->absm->bsm)((t), (o), (s), (f), (hp))

#define bus_space_subregion(t, h, o, s, hp) \
	((t)->absm->bsms)((h), (o), (s), (hp))

#define bus_space_unmap(t, h, s) \
	((t)->absm->bsu)((h), (s))

/*
 * Macro definition of _2 functions as indirect method array calls
 */

/* 0: Helper macros */

#define dbsdr(n, t, h, o)	((t)->absm->n)((h), (o))
#define dbsdw(n, t, h, o, v)	((t)->absm->n)((h), (o), (v))
#define dbsm(n, t, h, o, p, c)	((t)->absm->n)((h), (o), (p), (c))
#define dbss(n, t, h, o, v, c)	((t)->absm->n)((h), (o), (v), (c))
#define dbsc(n, t, h, o, v, c)	((t)->absm->n)((h), (o), (v), (c))

/* 1: byte-wide "functions" */

#define bus_space_read_1(t, h, o)		  dbsdr(bsr1, t, h, o)
#define bus_space_write_1(t, h, o, v)		  dbsdw(bsw1, t, h, o, v)

#define bus_space_read_multi_1(t, h, o, p, c)	  dbsm(bsrm1, t, h, o, p, c)
#define bus_space_write_multi_1(t, h, o, p, c)	  dbsm(bswm1, t, h, o, p, c)

#define bus_space_read_region_1(t, h, o, p, c)	  dbsm(bsrr1, t, h, o, p, c)
#define bus_space_write_region_1(t, h, o, p, c)	  dbsm(bswr1, t, h, o, p, c)

#define bus_space_set_region_1(t, h, o, v, c)	  dbss(bssr1, t, h, o, v, c)
#define bus_space_copy_region_1(t, h, o, g, q, c) dbss(bscr1, t, h, o, g, q, c)


/* 2: word-wide "functions" */

#define bus_space_read_2(t, h, o)		  dbsdr(bsr2, t, h, o)
#define bus_space_write_2(t, h, o, v)		  dbsdw(bsw2, t, h, o, v)
#define bus_space_read_stream_2(t, h, o)	  dbsdr(bsrs2, t, h, o)
#define bus_space_write_stream_2(t, h, o, v)	  dbsdw(bsws2, t, h, o, v)

#define bus_space_read_multi_2(t, h, o, p, c)	  dbsm(bsrm2, t, h, o, p, c)
#define bus_space_write_multi_2(t, h, o, p, c)	  dbsm(bswm2, t, h, o, p, c)

#define bus_space_read_multi_stream_2(t, h, o, p, c) \
						  dbsm(bsrms2, t, h, o, p, c)

#define bus_space_write_multi_stream_2(t, h, o, p, c) \
						  dbsm(bswms2, t, h, o, p, c)

#define bus_space_read_region_2(t, h, o, p, c)	  dbsm(bsrr2, t, h, o, p, c)
#define bus_space_write_region_2(t, h, o, p, c)	  dbsm(bswr2, t, h, o, p, c)

#define bus_space_read_region_stream_2(t, h, o, p, c) \
						  dbsm(bsrrs2, t, h, o, p, c)

#define bus_space_write_region_stream_2(t, h, o, p, c) \
						  dbsm(bswrs2, t, h, o, p, c)

#define bus_space_set_region_2(t, h, o, v, c)	  dbss(bssr2, t, h, o, v, c)
#define bus_space_copy_region_2(t, h, o, g, q, c) dbss(bscr2, t, h, o, g, q, c)

/* 4: Fake 32-bit macros */

#define bus_space_read_4(t, h, o) \
	(panic("bus_space_read_4 not implemented"), 0)

#define bus_space_write_4(t, h, o, v) \
	panic("bus_space_write_4 not implemented")

#define bus_space_read_stream_4(t, h, o) \
	(panic("bus_space_read_stream_4 not implemented"), 0)

#define bus_space_write_stream_4(t, h, o, v) \
	panic("bus_space_read_stream_4 not implemented")

#define bus_space_read_multi_4(t, h, o, p, c) \
	panic("bus_space_read_multi_4 not implemented")

#define bus_space_write_multi_4(t, h, o, p, c) \
	panic("bus_space_write_multi_4 not implemented")

#define bus_space_read_multi_stream_4(t, h, o, p, c) \
	panic("bus_space_read_multi_stream_4 not implemented")

#define bus_space_write_multi_stream_4(t, h, o, p, c) \
	panic("bus_space_write_multi_stream_4 not implemented")

#define bus_space_read_region_stream_4(t, h, o, p, c) \
	panic("bus_space_read_region_stream_4 not implemented")

#define bus_space_write_region_stream_4(t, h, o, p, c) \
	panic("bus_space_write_region_stream_4 not implemented")

/* 
 * Bus read/write barrier methods.
 * 
 *      void bus_space_barrier(bus_space_tag_t tag,
 *          bus_space_handle_t bsh, bus_size_t offset,
 *          bus_size_t len, int flags);
 *    
 * Note: the 680x0 does not currently require barriers, but we must
 * provide the flags to MI code.
 */   
#define bus_space_barrier(t, h, o, l, f)        \
        ((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)))
#define BUS_SPACE_BARRIER_READ  0x01            /* force read barrier */
#define BUS_SPACE_BARRIER_WRITE 0x02            /* force write barrier */
 
#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

#define __BUS_SPACE_HAS_STREAM_METHODS

/* Instruction for enforcing reorder protection. Nothing for 68k. */
#define amiga_bus_reorder_protect()

extern const struct amiga_bus_space_methods amiga_bus_stride_1;
extern const struct amiga_bus_space_methods amiga_bus_stride_2;
extern const struct amiga_bus_space_methods amiga_bus_stride_4;
extern const struct amiga_bus_space_methods amiga_bus_stride_4swap;
extern const struct amiga_bus_space_methods amiga_bus_stride_16;

#endif /* _AMIGA_BUS_H_ */
