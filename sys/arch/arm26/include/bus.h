/* $NetBSD: bus.h,v 1.2.4.3 2000/12/13 14:49:40 bouyer Exp $ */

/*-
 * Copyright (c) 2000 Ben Harris
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * bus.h - Bus space functions for brivers
 */

#ifndef _ARM26_BUS_H_
#define _ARM26_BUS_H_

/*
 * I believe that there's only one sensible bus space on the Archimedes,
 * which corresponds to the system I/O bus.
 *
 * Some IDE cards have an odd shift applied to their registers.  This is
 * contained in the bus_space_tag_t.
 */

/* Addresses (in bus space). */
typedef u_int bus_addr_t;
typedef u_int bus_size_t;

/* Access methods for bus space. */
typedef int bus_space_tag_t;
typedef bus_addr_t bus_space_handle_t;

/* Mapping and unmapping operations. */
#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02
extern int bus_space_map(bus_space_tag_t, bus_addr_t, bus_size_t, int,
			 bus_space_handle_t *);
#define bus_space_unmap(t,h,s) /* Do nothing */
extern int bus_space_subregion(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			       bus_size_t, bus_space_handle_t *);
extern int bus_space_shift(bus_space_tag_t, bus_space_handle_t, int,
			   bus_space_tag_t *, bus_space_handle_t *);

/* Allocating and freeing bus space */
#define bus_space_alloc(t, rs, re, s, a, b, c, ap, hp) (-1)
#define bus_space_free(t, h, s) /* Do nothing */

/* Used by ne2000.c */
#define BUS_SPACE_ALIGNED_POINTER ALIGNED_POINTER

/* Bus barrier operations. */
#define BUS_SPACE_BARRIER_READ	0
#define BUS_SPACE_BARRIER_WRITE	0
#define	bus_space_barrier(t, h, o, l, f) /* Do nothing */

/* Bus read (single) operations. */
#define	bus_space_read_1(t, h, o) (*(volatile u_int8_t *)((h) + ((o) << (t))))
#define	bus_space_read_2(t, h, o) (*(volatile u_int16_t *)((h) + ((o) << (t))))

/* Bus write (single) operations. */
#define	bus_space_write_1(t, h, o, v)					\
    (*(volatile u_int8_t *)((h) + ((o) << (t))) = (v))
#define	bus_space_write_2(t, h, o, v)					\
    (*(volatile u_int32_t *)((h) + ((o) << (t))) = (v) | ((v) << 16))

/* Bus read multiple operations. */
extern void bus_space_read_multi_1(bus_space_tag_t, bus_space_handle_t,
				   bus_size_t, u_int8_t *, bus_size_t);
extern void bus_space_read_multi_2(bus_space_tag_t, bus_space_handle_t,
				   bus_size_t, u_int16_t *, bus_size_t);
#define bus_space_read_multi_4(t, h, o, d, s) panic("bus_space_read_multi_4")

/* Bus write multiple operations. */
extern void bus_space_write_multi_1(bus_space_tag_t, bus_space_handle_t,
				    bus_size_t, u_int8_t const *, bus_size_t);
extern void bus_space_write_multi_2(bus_space_tag_t, bus_space_handle_t,
				    bus_size_t, u_int16_t const *, bus_size_t);
#define bus_space_write_multi_4(t, h, o, d, s) panic("bus_space_write_multi_4")

/* Bus set multiple operations. */
extern void bus_space_set_multi_1(bus_space_tag_t, bus_space_handle_t,
				  bus_size_t, u_int8_t, bus_size_t);
extern void bus_space_set_multi_2(bus_space_tag_t, bus_space_handle_t,
				  bus_size_t, u_int16_t, bus_size_t);

/* Bus read region operations. */
extern void bus_space_read_region_1(bus_space_tag_t, bus_space_handle_t,
				    bus_size_t, u_int8_t *, bus_size_t);
extern void bus_space_read_region_2(bus_space_tag_t, bus_space_handle_t,
				    bus_size_t, u_int16_t *, bus_size_t);

/* Bus write region operations. */
extern void bus_space_write_region_1(bus_space_tag_t, bus_space_handle_t,
				     bus_size_t, u_int8_t const *, bus_size_t);
extern void bus_space_write_region_2(bus_space_tag_t, bus_space_handle_t,
				     bus_size_t, u_int16_t const *,
				     bus_size_t);

/* Bus set region operations. */
extern void bus_space_set_region_1(bus_space_tag_t, bus_space_handle_t,
				   bus_size_t, u_int8_t, bus_size_t);
extern void bus_space_set_region_2(bus_space_tag_t, bus_space_handle_t,
				   bus_size_t, u_int16_t, bus_size_t);

/* Copy operations. */
extern void bus_space_copy_1(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     bus_space_handle_t, bus_size_t, bus_size_t);
extern void bus_space_copy_2(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     bus_space_handle_t, bus_size_t, bus_size_t);

/*
 * bus_dma stuff.  We don't support bus_dma yet, but some drivers need
 * this.
 */

typedef void *bus_dmamap_t;

#endif /* _ARM26_BUS_H_ */
