/*	$NetBSD: bus.h,v 1.2 2005/12/24 21:44:59 perry Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Leo Weppelman for the
 *	NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * Lazyness macros for function declarations.
 */

#define bsr(what, typ) \
	typ (what)(bus_space_tag_t, bus_space_handle_t, bus_size_t)

#define bsw(what, typ) \
	void (what)(bus_space_tag_t, bus_space_handle_t, bus_size_t, typ)

#define bsrm(what, typ) \
	void (what)(bus_space_tag_t, bus_space_handle_t, bus_size_t, \
		typ *, bus_size_t)

#define bswm(what, typ) \
	void (what)(bus_space_tag_t, bus_space_handle_t, bus_size_t, \
		const typ *, bus_size_t)

#define bssr(what, typ) \
	void (what)(bus_space_tag_t, bus_space_handle_t, bus_size_t, \
		typ, bus_size_t)

#define bscr(what, typ) \
	void (what)(bus_space_tag_t, \
		bus_space_handle_t, bus_size_t, \
		bus_space_handle_t, bus_size_t, \
		bus_size_t)

/* declarations for _1 functions */

bsrm(bus_space_read_multi_1, u_int8_t);
bswm(bus_space_write_multi_1, u_int8_t);
bsrm(bus_space_read_region_1, u_int8_t);
bswm(bus_space_write_region_1, u_int8_t);
bsrm(bus_space_read_region_stream_1, u_int8_t);
bswm(bus_space_write_region_stream_1, u_int8_t);
bssr(bus_space_set_region_1, u_int8_t);
bscr(bus_space_copy_region_1, u_int8_t);

/*
 * Implementation specific structures.
 * XXX Don't use outside of bus_space definitions!
 * XXX maybe this should be encapsuled in a non-global .h file?
 */ 

struct bus_space_tag {
	bus_addr_t	base;
	u_int8_t	stride;
	u_int8_t	dum[3];
	const struct amiga_bus_space_methods *absm;
};

struct amiga_bus_space_methods {

	/* 16bit methods */

	bsr(*bsr2, u_int16_t);
	bsw(*bsw2, u_int16_t);
	bsrm(*bsrm2, u_int16_t);
	bswm(*bswm2, u_int16_t);
	bsrm(*bsrr2, u_int16_t);
	bswm(*bswr2, u_int16_t);
	bsrm(*bsrrs2, u_int16_t);
	bswm(*bswrs2, u_int16_t);
	bssr(*bssr2, u_int16_t);
	bscr(*bscr2, u_int16_t);

	/* add 32bit methods here */
};

const struct amiga_bus_space_methods amiga_contiguous_methods;
const struct amiga_bus_space_methods amiga_interleaved_methods;
const struct amiga_bus_space_methods amiga_interleaved_wordaccess_methods;

/*
 * Macro definition of map, unmap, etc.
 */

#define bus_space_map(tag,off,size,cache,handle) 			\
	(*(handle) = (tag)->base + ((off)<<(tag)->stride), 0)

#define bus_space_subregion(tag, handle, offset, size, nhandlep) \
	(*(nhandlep) = (handle) + ((offset)<<(tag)->stride), 0)

#define bus_space_unmap(tag,handle,size)	(void)0

/*
 * Macro definition of some _1 functions:
 */

#define	bus_space_read_1(t, h, o) \
    ((void) t, (*(volatile u_int8_t *)((h) + ((o)<<(t)->stride))))

#define	bus_space_write_1(t, h, o, v) \
    ((void) t, ((void)(*(volatile u_int8_t *)((h) + ((o)<<(t)->stride)) = (v))))

/*
 * Inline definition of other _1 functions:
 */

extern inline void
bus_space_read_multi_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int8_t		*a;
{
	for (; c; a++, c--)
		*a = bus_space_read_1(t, h, o);
}

extern inline void
bus_space_write_multi_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int8_t		*a;
{
	for (; c; a++, c--)
		bus_space_write_1(t, h, o, *a);
}

extern inline void
bus_space_read_region_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int8_t		*a;
{
	for (; c; a++, c--)
		*a = bus_space_read_1(t, h, o++);
}

extern inline void
bus_space_write_region_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int8_t		*a;
{
	for (; c; a++, c--)
		 bus_space_write_1(t, h, o++, *a);
}

extern inline void
bus_space_set_region_1(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int8_t		v;
{
	while (c--)
		 bus_space_write_1(t, h, o++, v);
}

extern inline void
bus_space_copy_region_1(t, srch, srco, dsth, dsto, c)
	bus_space_tag_t		t;
	bus_space_handle_t	srch, dsth;
	bus_size_t		srco, dsto, c;
{
	u_int8_t		v;

	while (c--) {
		 v = bus_space_read_1(t, srch, srco++);
		 bus_space_write_1(t, dsth, dsto++, v);
	}
}

extern inline void
bus_space_read_region_stream_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int8_t		*a;
{
	for (; c; a++, c--)
		*a = bus_space_read_1(t, h, o++);
}

extern inline void
bus_space_write_region_stream_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int8_t		*a;
{
	for (; c; a++, c--)
		 bus_space_write_1(t, h, o++, *a);
}

/*
 * Macro definition of _2 functions as indirect method array calls
 */

#define bus_space_read_2(t, h, o)	((t)->absm->bsr2)((t), (h), (o))
#define bus_space_write_2(t, h, o, v)	((t)->absm->bsw2)((t), (h), (o), (v))

#define bus_space_read_multi_2(t, h, o, p, c) \
	((t)->absm->bsrm2)((t), (h), (o), (p), (c))

#define bus_space_write_multi_2(t, h, o, p, c) \
	((t)->absm->bswm2)((t), (h), (o), (p), (c))

#define bus_space_read_region_2(t, h, o, p, c) \
	((t)->absm->bsrr2)((t), (h), (o), (p), (c))

#define bus_space_write_region_2(t, h, o, p, c) \
	((t)->absm->bswr2)((t), (h), (o), (p), (c))

#define bus_space_read_region_stream_2(t, h, o, p, c) \
	((t)->absm->bsrrs2)((t), (h), (o), (p), (c))

#define bus_space_write_region_stream_2(t, h, o, p, c) \
	((t)->absm->bswrs2)((t), (h), (o), (p), (c))

#define bus_space_set_region_2(t, h, o, v, c) \
	((t)->absm->bssr2)((t), (h), (o), (v), (c))

#define bus_space_copy_region_2(t, srch, srco, dsth, dsto, c) \
	((t)->absm->bscr2)((t), (srch), (srco), (dsth), (dsto), (c))

/* 
 * Bus read/write barrier methods.
 * 
 *      void bus_space_barrier __P((bus_space_tag_t tag,
 *          bus_space_handle_t bsh, bus_size_t offset,
 *          bus_size_t len, int flags));
 *    
 * Note: the 680x0 does not currently require barriers, but we must
 * provide the flags to MI code.
 */   
#define bus_space_barrier(t, h, o, l, f)        \
        ((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)))
#define BUS_SPACE_BARRIER_READ  0x01            /* force read barrier */
#define BUS_SPACE_BARRIER_WRITE 0x02            /* force write barrier */
 
#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)
#endif /* _AMIGA_BUS_H_ */
