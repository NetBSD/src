/*	$NetBSD: bus_defs.h,v 1.5.12.1 2014/08/20 00:02:44 tls Exp $	*/

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

#ifndef _AMIGAPPC_BUS_DEFS_H_
#define _AMIGAPPC_BUS_DEFS_H_

/*
 * Memory addresses (in bus space)
 */

typedef uint32_t bus_addr_t;
typedef uint32_t bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
typedef struct bus_space_tag *bus_space_tag_t;
typedef u_long	bus_space_handle_t;

struct amigappc_bus_dma_segment;
struct amigappc_bus_dma_tag;
struct amigappc_bus_dmamap;
typedef struct amigappc_bus_dma_segment bus_dma_segment_t;
typedef struct amigappc_bus_dma_tag bus_dma_tag_t;
typedef struct amigappc_bus_dmamap bus_dmamap_t;

struct amigappc_bus_dma_tag {
	int dummy;
};

struct amigappc_bus_dma_segment {
	bus_addr_t	ds_addr;
	bus_size_t	ds_len;
};

struct amigappc_bus_dmamap {
	bus_size_t		dm_maxsegsz;
	bus_size_t		dm_mapsize;
	int			dm_nsegs;
	bus_dma_segment_t	*dm_segs;
	/* TBD */
};

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

	bsr(*bsr1, uint8_t);
	bsw(*bsw1, uint8_t);
	bsrm(*bsrm1, uint8_t);
	bswm(*bswm1, uint8_t);
	bsrm(*bsrr1, uint8_t);
	bswm(*bswr1, uint8_t);
	bssr(*bssr1, uint8_t);
	bscr(*bscr1, uint8_t);

	/* 16bit methods */

	bsr(*bsr2, uint16_t);
	bsw(*bsw2, uint16_t);
	bsr(*bsrs2, uint16_t);
	bsw(*bsws2, uint16_t);
	bsrm(*bsrm2, uint16_t);
	bswm(*bswm2, uint16_t);
	bsrm(*bsrms2, uint16_t);
	bswm(*bswms2, uint16_t);
	bsrm(*bsrr2, uint16_t);
	bswm(*bswr2, uint16_t);
	bsrm(*bsrrs2, uint16_t);
	bswm(*bswrs2, uint16_t);
	bssr(*bssr2, uint16_t);
	bscr(*bscr2, uint16_t);

	/* 32bit methods */

	bsr(*bsr4, uint32_t);
	bsw(*bsw4, uint32_t);
	bsr(*bsrs4, uint32_t);
	bsw(*bsws4, uint32_t);
	bsrm(*bsrm4, uint32_t);
	bswm(*bswm4, uint32_t);
	bsrm(*bsrms4, uint32_t);
	bswm(*bswms4, uint32_t);
	bsrm(*bsrr4, uint32_t);
	bswm(*bswr4, uint32_t);
	bsrm(*bsrrs4, uint32_t);
	bswm(*bswrs4, uint32_t);
	bssr(*bssr4, uint32_t);
	bscr(*bscr4, uint32_t);
};

#define BUS_SPACE_BARRIER_READ  0x01            /* force read barrier */
#define BUS_SPACE_BARRIER_WRITE 0x02            /* force write barrier */

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

#define __BUS_SPACE_HAS_STREAM_METHODS

extern const struct amiga_bus_space_methods amiga_bus_stride_1;
extern const struct amiga_bus_space_methods amiga_bus_stride_2;
extern const struct amiga_bus_space_methods amiga_bus_stride_4;
extern const struct amiga_bus_space_methods amiga_bus_stride_4swap;
extern const struct amiga_bus_space_methods amiga_bus_stride_16;
extern const struct amiga_bus_space_methods amiga_bus_stride_0x1000;

#endif /* _AMIGAPPC_BUS_DEFS_H_ */
