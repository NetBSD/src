/*	$NetBSD: bus.h,v 1.31 2002/05/07 00:37:47 uwe Exp $	*/

/*-
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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

#ifndef _SPARC_BUS_H_
#define _SPARC_BUS_H_

#define	SPARC_BUS_SPACE	0

/*
 * Bus address and size types
 */
typedef	u_long		bus_space_handle_t;
typedef u_int64_t	bus_addr_t;
typedef u_long		bus_size_t;

/* bus_addr_t is extended to 64-bits and has the iospace encoded in it */
#define	BUS_ADDR_IOSPACE(x)	((x)>>32)
#define	BUS_ADDR_PADDR(x)	((x)&0xffffffff)
#define	BUS_ADDR(io, pa)	\
	((((u_int64_t)(u_int32_t)(io))<<32) | (u_int32_t)(pa))

/*
 * Access methods for bus resources and address space.
 */
typedef struct sparc_bus_space_tag	*bus_space_tag_t;

struct sparc_bus_space_tag {
	void		*cookie;
	bus_space_tag_t	parent;

	int	(*sparc_bus_map) __P((
				bus_space_tag_t,
				bus_addr_t,
				bus_size_t,
				int,			/*flags*/
				vaddr_t,		/*preferred vaddr*/
				bus_space_handle_t *));
	int	(*sparc_bus_unmap) __P((
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t));
	int	(*sparc_bus_subregion) __P((
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/*offset*/
				bus_size_t,		/*size*/
				bus_space_handle_t *));

	void	(*sparc_bus_barrier) __P((
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/*offset*/
				bus_size_t,		/*size*/
				int));			/*flags*/

	paddr_t	(*sparc_bus_mmap) __P((
				bus_space_tag_t,
				bus_addr_t,
				off_t,
				int,			/*prot*/
				int));			/*flags*/

	void	*(*sparc_intr_establish) __P((
				bus_space_tag_t,
				int,			/*bus-specific intr*/
				int,			/*device class level,
							  see machine/intr.h*/
				int,			/*flags*/
				int (*) __P((void *)),	/*handler*/
				void *));		/*handler arg*/

};

#if 0
/*
 * The following macro could be used to generate the bus_space*() functions
 * but it uses a gcc extension and is ANSI-only.
#define PROTO_bus_space_xxx		__P((bus_space_tag_t t, ...))
#define RETURNTYPE_bus_space_xxx	void *
#define BUSFUN(name, returntype, t, args...)			\
	__inline__ RETURNTYPE_##name				\
	bus_##name PROTO_##name					\
	{							\
		while (t->sparc_##name == NULL)			\
			t = t->parent;				\
		return (*(t)->sparc_##name)(t, args);		\
	}
 */
#endif

/*
 * Bus space function prototypes.
 * In bus_space_map2(), supply a special virtual address only if you
 * get it from ../sparc/vaddrs.h.
 */
static int	bus_space_map __P((
				bus_space_tag_t,
				bus_addr_t,
				bus_size_t,
				int,			/*flags*/
				bus_space_handle_t *));
static int	bus_space_map2 __P((
				bus_space_tag_t,
				bus_addr_t,
				bus_size_t,
				int,			/*flags*/
				vaddr_t,		/*preferred vaddr*/
				bus_space_handle_t *));
static int	bus_space_unmap __P((
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t));
static int	bus_space_subregion __P((
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,
				bus_size_t,
				bus_space_handle_t *));
static void	bus_space_barrier __P((
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,
				bus_size_t,
				int));
static paddr_t	bus_space_mmap __P((
				bus_space_tag_t,
				bus_addr_t,		/**/
				off_t,
				int,			/*prot*/
				int));			/*flags*/
static void	*bus_intr_establish __P((
				bus_space_tag_t,
				int,			/*bus-specific intr*/
				int,			/*device class level,
							  see machine/intr.h*/
				int,			/*flags*/
				int (*) __P((void *)),	/*handler*/
				void *));		/*handler arg*/


/* This macro finds the first "upstream" implementation of method `f' */
#define _BS_CALL(t,f)			\
	while (t->f == NULL)		\
		t = t->parent;		\
	return (*(t)->f)

static __inline__ int
bus_space_map(t, a, s, f, hp)
	bus_space_tag_t	t;
	bus_addr_t	a;
	bus_size_t	s;
	int		f;
	bus_space_handle_t *hp;
{
	_BS_CALL(t, sparc_bus_map)(t, a, s, f, 0, hp);
}

static __inline__ int
bus_space_map2(t, a, s, f, v, hp)
	bus_space_tag_t	t;
	bus_addr_t	a;
	bus_size_t	s;
	int		f;
	vaddr_t		v;
	bus_space_handle_t *hp;
{
	_BS_CALL(t, sparc_bus_map)(t, a, s, f, v, hp);
}

static __inline__ int
bus_space_unmap(t, h, s)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t	s;
{
	_BS_CALL(t, sparc_bus_unmap)(t, h, s);
}

static __inline__ int
bus_space_subregion(t, h, o, s, hp)
	bus_space_tag_t	t;
	bus_space_handle_t h;
	bus_size_t	o;
	bus_size_t	s;
	bus_space_handle_t *hp;
{
	_BS_CALL(t, sparc_bus_subregion)(t, h, o, s, hp);
}

static __inline__ paddr_t
bus_space_mmap(t, a, o, p, f)
	bus_space_tag_t	t;
	bus_addr_t	a;
	off_t		o;
	int		p;
	int		f;
{
	_BS_CALL(t, sparc_bus_mmap)(t, a, o, p, f);
}

static __inline__ void *
bus_intr_establish(t, p, l, f, h, a)
	bus_space_tag_t t;
	int	p;
	int	l;
	int	f;
	int	(*h)__P((void *));
	void	*a;
{
	_BS_CALL(t, sparc_intr_establish)(t, p, l, f, h, a);
}

static __inline__ void
bus_space_barrier(t, h, o, s, f)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	bus_size_t s;
	int f;
{
	_BS_CALL(t, sparc_bus_barrier)(t, h, o, s, f);
}


#if 0
int	bus_space_alloc __P((bus_space_tag_t t, bus_addr_t rstart,
	    bus_addr_t rend, bus_size_t size, bus_size_t align,
	    bus_size_t boundary, int flags, bus_addr_t *addrp,
	    bus_space_handle_t *bshp));
void	bus_space_free __P((bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t size));
#endif

#define	bus_space_vaddr(t, h)	((void *)(h))

/* flags for bus space map functions */
#define BUS_SPACE_MAP_CACHEABLE	0x0001
#define BUS_SPACE_MAP_LINEAR	0x0002
#define BUS_SPACE_MAP_PREFETCHABLE	0x0004
#define BUS_SPACE_MAP_BUS1	0x0100	/* placeholders for bus functions... */
#define BUS_SPACE_MAP_BUS2	0x0200
#define BUS_SPACE_MAP_BUS3	0x0400
#define BUS_SPACE_MAP_BUS4	0x0800


/* flags for intr_establish() */
#define BUS_INTR_ESTABLISH_FASTTRAP	1
#define BUS_INTR_ESTABLISH_SOFTINTR	2

/* flags for bus_space_barrier() */
#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

/*
 * Device space probe assistant.
 * The optional callback function's arguments are:
 *	the temporary virtual address
 *	the passed `arg' argument
 */
int bus_space_probe __P((
		bus_space_tag_t,
		bus_addr_t,
		bus_size_t,			/* probe size */
		size_t,				/* offset */
		int,				/* flags */
		int (*) __P((void *, void *)),	/* callback function */
		void *));			/* callback arg */


/*
 *	u_intN_t bus_space_read_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset));
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define	bus_space_read_1(t, h, o)					\
	    ((void)t, *(volatile u_int8_t *)((h) + (o)))

#define	bus_space_read_2(t, h, o)					\
	    ((void)t, *(volatile u_int16_t *)((h) + (o)))

#define	bus_space_read_4(t, h, o)					\
	    ((void)t, *(volatile u_int32_t *)((h) + (o)))

#define	bus_space_read_8(t, h, o)					\
	    ((void)t, *(volatile u_int64_t *)((h) + (o)))

#define bus_space_read_stream_1 bus_space_read_1
#define bus_space_read_stream_2 bus_space_read_2
#define bus_space_read_stream_4 bus_space_read_4
#define bus_space_read_stream_8 bus_space_read_8


/*
 *	void bus_space_write_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value));
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define	bus_space_write_1(t, h, o, v)	do {				\
	((void)t, (void)(*(volatile u_int8_t *)((h) + (o)) = (v)));	\
} while (0)

#define	bus_space_write_2(t, h, o, v)	do {				\
	((void)t, (void)(*(volatile u_int16_t *)((h) + (o)) = (v)));	\
} while (0)

#define	bus_space_write_4(t, h, o, v)	do {				\
	((void)t, (void)(*(volatile u_int32_t *)((h) + (o)) = (v)));	\
} while (0)

#define	bus_space_write_8(t, h, o, v)	do {				\
	((void)t, (void)(*(volatile u_int64_t *)((h) + (o)) = (v)));	\
} while (0)

#define bus_space_write_stream_1 bus_space_write_1
#define bus_space_write_stream_2 bus_space_write_2
#define bus_space_write_stream_4 bus_space_write_4
#define bus_space_write_stream_8 bus_space_write_8


/*
 *	void bus_space_read_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, bus_size_t count));
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

static void bus_space_read_multi_1 __P((bus_space_tag_t,
					bus_space_handle_t,
					bus_size_t,
					u_int8_t *,
					bus_size_t));

static void bus_space_read_multi_2 __P((bus_space_tag_t,
					bus_space_handle_t,
					bus_size_t,
					u_int16_t *,
					bus_size_t));

static void bus_space_read_multi_4 __P((bus_space_tag_t,
					bus_space_handle_t,
					bus_size_t,
					u_int32_t *,
					bus_size_t));

static void bus_space_read_multi_8 __P((bus_space_tag_t,
					bus_space_handle_t,
					bus_size_t,
					u_int64_t *,
					bus_size_t));

static __inline__ void
bus_space_read_multi_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int8_t		*a;
{
	while (c-- > 0)
		*a++ = bus_space_read_1(t, h, o);
}

static __inline__ void
bus_space_read_multi_2(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int16_t		*a;
{
	while (c-- > 0)
		*a++ = bus_space_read_2(t, h, o);
}

static __inline__ void
bus_space_read_multi_4(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int32_t		*a;
{
	while (c-- > 0)
		*a++ = bus_space_read_4(t, h, o);
}

static __inline__ void
bus_space_read_multi_8(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int64_t		*a;
{
	while (c-- > 0)
		*a++ = bus_space_read_8(t, h, o);
}

#define bus_space_read_multi_stream_1 bus_space_read_multi_1
#define bus_space_read_multi_stream_2 bus_space_read_multi_2
#define bus_space_read_multi_stream_4 bus_space_read_multi_4
#define bus_space_read_multi_stream_8 bus_space_read_multi_8


/*
 *	void bus_space_write_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, bus_size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */
static void bus_space_write_multi_1 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 const u_int8_t *,
					 bus_size_t));
static void bus_space_write_multi_2 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 const u_int16_t *,
					 bus_size_t));
static void bus_space_write_multi_4 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 const u_int32_t *,
					 bus_size_t));
static void bus_space_write_multi_8 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 const u_int64_t *,
					 bus_size_t));
static __inline__ void
bus_space_write_multi_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int8_t		*a;
{
	while (c-- > 0)
		bus_space_write_1(t, h, o, *a++);
}

static __inline__ void
bus_space_write_multi_2(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int16_t		*a;
{
	while (c-- > 0)
		bus_space_write_2(t, h, o, *a++);
}

static __inline__ void
bus_space_write_multi_4(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int32_t		*a;
{
	while (c-- > 0)
		bus_space_write_4(t, h, o, *a++);
}

static __inline__ void
bus_space_write_multi_8(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int64_t		*a;
{
	while (c-- > 0)
		bus_space_write_8(t, h, o, *a++);
}

#define bus_space_write_multi_stream_1 bus_space_write_multi_1
#define bus_space_write_multi_stream_2 bus_space_write_multi_2
#define bus_space_write_multi_stream_4 bus_space_write_multi_4
#define bus_space_write_multi_stream_8 bus_space_write_multi_8


/*
 *	void bus_space_set_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    bus_size_t count));
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */
static void bus_space_set_multi_1 __P((bus_space_tag_t,
				       bus_space_handle_t,
				       bus_size_t,
				       const u_int8_t,
				       bus_size_t));
static void bus_space_set_multi_2 __P((bus_space_tag_t,
				       bus_space_handle_t,
				       bus_size_t,
				       const u_int16_t,
				       bus_size_t));
static void bus_space_set_multi_4 __P((bus_space_tag_t,
				       bus_space_handle_t,
				       bus_size_t,
				       const u_int32_t,
				       bus_size_t));
static void bus_space_set_multi_8 __P((bus_space_tag_t,
				       bus_space_handle_t,
				       bus_size_t,
				       const u_int64_t,
				       bus_size_t));

static __inline__ void
bus_space_set_multi_1(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int8_t		v;
{
	while (c-- > 0)
		bus_space_write_1(t, h, o, v);
}

static __inline__ void
bus_space_set_multi_2(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int16_t		v;
{
	while (c-- > 0)
		bus_space_write_2(t, h, o, v);
}

static __inline__ void
bus_space_set_multi_4(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int32_t		v;
{
	while (c-- > 0)
		bus_space_write_4(t, h, o, v);
}

static __inline__ void
bus_space_set_multi_8(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int64_t		v;
{
	while (c-- > 0)
		bus_space_write_8(t, h, o, v);
}


/*
 *	void bus_space_read_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    u_intN_t *addr, bus_size_t count));
 *
 */
static void bus_space_read_region_1 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 u_int8_t *,
					 bus_size_t));
static void bus_space_read_region_2 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 u_int16_t *,
					 bus_size_t));
static void bus_space_read_region_4 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 u_int32_t *,
					 bus_size_t));
static void bus_space_read_region_8 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 u_int64_t *,
					 bus_size_t));

static __inline__ void
bus_space_read_region_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int8_t		*a;
{
	for (; c; a++, c--, o++)
		*a = bus_space_read_1(t, h, o);
}

static __inline__ void
bus_space_read_region_2(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int16_t		*a;
{
	for (; c; a++, c--, o+=2)
		*a = bus_space_read_2(t, h, o);
}

static __inline__ void
bus_space_read_region_4(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int32_t		*a;
{
	for (; c; a++, c--, o+=4)
		*a = bus_space_read_4(t, h, o);
}

static __inline__ void
bus_space_read_region_8(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	u_int64_t		*a;
{
	for (; c; a++, c--, o+=8)
		*a = bus_space_read_8(t, h, o);
}

/*
 *	void bus_space_write_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    u_intN_t *addr, bus_size_t count));
 *
 */
static void bus_space_write_region_1 __P((bus_space_tag_t,
					  bus_space_handle_t,
					  bus_size_t,
					  const u_int8_t *,
					  bus_size_t));
static void bus_space_write_region_2 __P((bus_space_tag_t,
					  bus_space_handle_t,
					  bus_size_t,
					  const u_int16_t *,
					  bus_size_t));
static void bus_space_write_region_4 __P((bus_space_tag_t,
					  bus_space_handle_t,
					  bus_size_t,
					  const u_int32_t *,
					  bus_size_t));
static void bus_space_write_region_8 __P((bus_space_tag_t,
					  bus_space_handle_t,
					  bus_size_t,
					  const u_int64_t *,
					  bus_size_t));
static __inline__ void
bus_space_write_region_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int8_t		*a;
{
	for (; c; a++, c--, o++)
		bus_space_write_1(t, h, o, *a);
}

static __inline__ void
bus_space_write_region_2(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int16_t		*a;
{
	for (; c; a++, c--, o+=2)
		bus_space_write_2(t, h, o, *a);
}

static __inline__ void
bus_space_write_region_4(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int32_t		*a;
{
	for (; c; a++, c--, o+=4)
		bus_space_write_4(t, h, o, *a);
}

static __inline__ void
bus_space_write_region_8(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int64_t		*a;
{
	for (; c; a++, c--, o+=8)
		bus_space_write_8(t, h, o, *a);
}


/*
 *	void bus_space_set_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    u_intN_t *addr, bus_size_t count));
 *
 */
static void bus_space_set_region_1 __P((bus_space_tag_t,
					bus_space_handle_t,
					bus_size_t,
					const u_int8_t,
					bus_size_t));
static void bus_space_set_region_2 __P((bus_space_tag_t,
					bus_space_handle_t,
					bus_size_t,
					const u_int16_t,
					bus_size_t));
static void bus_space_set_region_4 __P((bus_space_tag_t,
					bus_space_handle_t,
					bus_size_t,
					const u_int32_t,
					bus_size_t));
static void bus_space_set_region_8 __P((bus_space_tag_t,
					bus_space_handle_t,
					bus_size_t,
					const u_int64_t,
					bus_size_t));

static __inline__ void
bus_space_set_region_1(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int8_t		v;
{
	for (; c; c--, o++)
		bus_space_write_1(t, h, o, v);
}

static __inline__ void
bus_space_set_region_2(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int16_t		v;
{
	for (; c; c--, o+=2)
		bus_space_write_2(t, h, o, v);
}

static __inline__ void
bus_space_set_region_4(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int32_t		v;
{
	for (; c; c--, o+=4)
		bus_space_write_4(t, h, o, v);
}

static __inline__ void
bus_space_set_region_8(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o, c;
	const u_int64_t		v;
{
	for (; c; c--, o+=8)
		bus_space_write_8(t, h, o, v);
}


/*
 *	void bus_space_copy_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    bus_size_t count));
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */
static void bus_space_copy_region_1 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 bus_space_handle_t,
					 bus_size_t,
					 bus_size_t));
static void bus_space_copy_region_2 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 bus_space_handle_t,
					 bus_size_t,
					 bus_size_t));
static void bus_space_copy_region_4 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 bus_space_handle_t,
					 bus_size_t,
					 bus_size_t));
static void bus_space_copy_region_8 __P((bus_space_tag_t,
					 bus_space_handle_t,
					 bus_size_t,
					 bus_space_handle_t,
					 bus_size_t,
					 bus_size_t));


static __inline__ void
bus_space_copy_region_1(t, h1, o1, h2, o2, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h1, h2;
	bus_size_t		o1, o2;
	bus_size_t		c;
{
	for (; c; c--, o1++, o2++)
	    bus_space_write_1(t, h1, o1, bus_space_read_1(t, h2, o2));
}

static __inline__ void
bus_space_copy_region_2(t, h1, o1, h2, o2, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h1, h2;
	bus_size_t		o1, o2;
	bus_size_t		c;
{
	for (; c; c--, o1+=2, o2+=2)
	    bus_space_write_2(t, h1, o1, bus_space_read_2(t, h2, o2));
}

static __inline__ void
bus_space_copy_region_4(t, h1, o1, h2, o2, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h1, h2;
	bus_size_t		o1, o2;
	bus_size_t		c;
{
	for (; c; c--, o1+=4, o2+=4)
	    bus_space_write_4(t, h1, o1, bus_space_read_4(t, h2, o2));
}

static __inline__ void
bus_space_copy_region_8(t, h1, o1, h2, o2, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h1, h2;
	bus_size_t		o1, o2;
	bus_size_t		c;
{
	for (; c; c--, o1+=8, o2+=8)
	    bus_space_write_8(t, h1, o1, bus_space_read_8(t, h2, o2));
}

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

/*--------------------------------*/

/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_WAITOK		0x000	/* safe to sleep (pseudo-flag) */
#define	BUS_DMA_NOWAIT		0x001	/* not safe to sleep */
#define	BUS_DMA_ALLOCNOW	0x002	/* perform resource allocation now */
#define	BUS_DMA_COHERENT	0x004	/* hint: map memory DMA coherent */
#define	BUS_DMA_STREAMING	0x008	/* hint: sequential, unidirectional */
#define	BUS_DMA_BUS1		0x010	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2		0x020
#define	BUS_DMA_BUS3		0x040
#define	BUS_DMA_BUS4		0x080
#define	BUS_DMA_READ		0x100	/* mapping is device -> memory only */
#define	BUS_DMA_WRITE		0x200	/* mapping is memory -> device only */

/* For devices that have a 24-bit address space */
#define BUS_DMA_24BIT		BUS_DMA_BUS1

/* Internal flag: current DVMA address is equal to the KVA buffer address */
#define _BUS_DMA_DIRECTMAP	BUS_DMA_BUS2

/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;

/*
 * Operations performed by bus_dmamap_sync().
 */
#define	BUS_DMASYNC_PREREAD	0x01	/* pre-read synchronization */
#define	BUS_DMASYNC_POSTREAD	0x02	/* post-read synchronization */
#define	BUS_DMASYNC_PREWRITE	0x04	/* pre-write synchronization */
#define	BUS_DMASYNC_POSTWRITE	0x08	/* post-write synchronization */

typedef struct sparc_bus_dma_tag	*bus_dma_tag_t;
typedef struct sparc_bus_dmamap		*bus_dmamap_t;

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct sparc_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DVMA address */
	bus_size_t	ds_len;		/* length of transfer */
	bus_size_t	_ds_sgsize;	/* size of allocated DVMA segment */
	void		*_ds_mlist;	/* page list when dmamem_alloc'ed */
	vaddr_t		_ds_va;		/* VA when dmamem_map'ed */
};
typedef struct sparc_bus_dma_segment	bus_dma_segment_t;


/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */
struct sparc_bus_dma_tag {
	void	*_cookie;		/* cookie used in the guts */

	/*
	 * DMA mapping methods.
	 */
	int	(*_dmamap_create) __P((bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *));
	void	(*_dmamap_destroy) __P((bus_dma_tag_t, bus_dmamap_t));
	int	(*_dmamap_load) __P((bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int));
	int	(*_dmamap_load_mbuf) __P((bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int));
	int	(*_dmamap_load_uio) __P((bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int));
	int	(*_dmamap_load_raw) __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int));
	void	(*_dmamap_unload) __P((bus_dma_tag_t, bus_dmamap_t));
	void	(*_dmamap_sync) __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int));

	/*
	 * DMA memory utility functions.
	 */
	int	(*_dmamem_alloc) __P((bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int));
	void	(*_dmamem_free) __P((bus_dma_tag_t,
		    bus_dma_segment_t *, int));
	int	(*_dmamem_map) __P((bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, caddr_t *, int));
	void	(*_dmamem_unmap) __P((bus_dma_tag_t, caddr_t, size_t));
	paddr_t	(*_dmamem_mmap) __P((bus_dma_tag_t, bus_dma_segment_t *,
		    int, off_t, int, int));
};

#define	bus_dmamap_create(t, s, n, m, b, f, p)			\
	(*(t)->_dmamap_create)((t), (s), (n), (m), (b), (f), (p))
#define	bus_dmamap_destroy(t, p)				\
	(*(t)->_dmamap_destroy)((t), (p))
#define	bus_dmamap_load(t, m, b, s, p, f)			\
	(*(t)->_dmamap_load)((t), (m), (b), (s), (p), (f))
#define	bus_dmamap_load_mbuf(t, m, b, f)			\
	(*(t)->_dmamap_load_mbuf)((t), (m), (b), (f))
#define	bus_dmamap_load_uio(t, m, u, f)				\
	(*(t)->_dmamap_load_uio)((t), (m), (u), (f))
#define	bus_dmamap_load_raw(t, m, sg, n, s, f)			\
	(*(t)->_dmamap_load_raw)((t), (m), (sg), (n), (s), (f))
#define	bus_dmamap_unload(t, p)					\
	(*(t)->_dmamap_unload)((t), (p))
#define	bus_dmamap_sync(t, p, o, l, ops)			\
	(void)((t)->_dmamap_sync ?				\
	    (*(t)->_dmamap_sync)((t), (p), (o), (l), (ops)) : (void)0)

#define	bus_dmamem_alloc(t, s, a, b, sg, n, r, f)		\
	(*(t)->_dmamem_alloc)((t), (s), (a), (b), (sg), (n), (r), (f))
#define	bus_dmamem_free(t, sg, n)				\
	(*(t)->_dmamem_free)((t), (sg), (n))
#define	bus_dmamem_map(t, sg, n, s, k, f)			\
	(*(t)->_dmamem_map)((t), (sg), (n), (s), (k), (f))
#define	bus_dmamem_unmap(t, k, s)				\
	(*(t)->_dmamem_unmap)((t), (k), (s))
#define	bus_dmamem_mmap(t, sg, n, o, p, f)			\
	(*(t)->_dmamem_mmap)((t), (sg), (n), (o), (p), (f))

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct sparc_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use by machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxsegsz;	/* largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_flags;	/* misc. flags */

	void		*_dm_cookie;	/* cookie for bus-specific functions */

	u_long		_dm_align;	/* DVMA alignment; must be a
					   multiple of the page size */
	u_long		_dm_ex_start;	/* constraints on DVMA map */
	u_long		_dm_ex_end;	/* allocations; used by the VME bus
					   driver and by the IOMMU driver
					   when mapping 24-bit devices */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#ifdef _SPARC_BUS_DMA_PRIVATE
int	_bus_dmamap_create __P((bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *));
void	_bus_dmamap_destroy __P((bus_dma_tag_t, bus_dmamap_t));
int	_bus_dmamap_load_mbuf __P((bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int));
int	_bus_dmamap_load_uio __P((bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int));
int	_bus_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int));
void	_bus_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
void	_bus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int));

int	_bus_dmamem_alloc __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags));
void	_bus_dmamem_free __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs));
void	_bus_dmamem_unmap __P((bus_dma_tag_t tag, caddr_t kva,
	    size_t size));
paddr_t	_bus_dmamem_mmap __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, off_t off, int prot, int flags));

int	_bus_dmamem_alloc_range __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    vaddr_t low, vaddr_t high));

vaddr_t	_bus_dma_valloc_skewed(size_t, u_long, u_long, u_long);
#endif /* _SPARC_BUS_DMA_PRIVATE */

#endif /* _SPARC_BUS_H_ */
