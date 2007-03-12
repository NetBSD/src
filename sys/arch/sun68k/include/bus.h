/*	$NetBSD: bus.h,v 1.13.4.2 2007/03/12 05:51:11 rmind Exp $	*/

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

#ifndef _SUN68K_BUS_H_
#define _SUN68K_BUS_H_

#define	SUN68K_BUS_SPACE	0

/*
 * Bus address and size types
 */
typedef	u_long	bus_space_handle_t;
typedef u_long	bus_type_t;
typedef u_long	bus_addr_t;
typedef u_long	bus_size_t;

#define	BUS_ADDR_PADDR(x)	((x) & 0xffffffff)

/*
 * Access methods for bus resources and address space.
 */
typedef struct sun68k_bus_space_tag	*bus_space_tag_t;

struct sun68k_bus_space_tag {
	void		*cookie;
	bus_space_tag_t	parent;

	int	(*sun68k_bus_map)(
				bus_space_tag_t,
				bus_type_t,
				bus_addr_t,
				bus_size_t,
				int,			/*flags*/
				vaddr_t,		/*preferred vaddr*/
				bus_space_handle_t *);

	int	(*sun68k_bus_unmap)(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t);

	int	(*sun68k_bus_subregion)(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/*offset*/
				bus_size_t,		/*size*/
				bus_space_handle_t *);

	void	(*sun68k_bus_barrier)(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/*offset*/
				bus_size_t,		/*size*/
				int);			/*flags*/

	paddr_t	(*sun68k_bus_mmap)(
				bus_space_tag_t,
				bus_type_t,		/**/
				bus_addr_t,		/**/
				off_t,			/*offset*/
				int,			/*prot*/
				int);			/*flags*/

	void	*(*sun68k_intr_establish)(
				bus_space_tag_t,
				int,			/*bus-specific intr*/
				int,			/*device class level,
							  see machine/intr.h*/
				int,			/*flags*/
				int (*)(void *),	/*handler*/
				void *);		/*handler arg*/

	int	(*sun68k_bus_peek)(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/*offset*/
				size_t,			/*probe size*/
				void *);		/*result ptr*/

	int	(*sun68k_bus_poke)(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/*offset*/
				size_t,			/*probe size*/
				uint32_t);		/*value*/
};

#if 0
/*
 * The following macro could be used to generate the bus_space*() functions
 * but it uses a gcc extension and is ANSI-only.
#define PROTO_bus_space_xxx		(bus_space_tag_t t, ...)
#define RETURNTYPE_bus_space_xxx	void *
#define BUSFUN(name, returntype, t, args...)			\
	__inline RETURNTYPE_##name				\
	bus_##name PROTO_##name					\
	{							\
		while (t->sun68k_##name == NULL)			\
			t = t->parent;				\
		return (*(t)->sun68k_##name)(t, args);		\
	}
 */
#endif

/*
 * Bus space function prototypes.
 */
static int	bus_space_map(
				bus_space_tag_t,
				bus_addr_t,
				bus_size_t,
				int,			/*flags*/
				bus_space_handle_t *);
static int	bus_space_map2(
				bus_space_tag_t,
				bus_type_t,
				bus_addr_t,
				bus_size_t,
				int,			/*flags*/
				vaddr_t,		/*preferred vaddr*/
				bus_space_handle_t *);
static int	bus_space_unmap(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t);
static int	bus_space_subregion(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,
				bus_size_t,
				bus_space_handle_t *);
static void	bus_space_barrier(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,
				bus_size_t,
				int);
static paddr_t	bus_space_mmap(
				bus_space_tag_t,
				bus_addr_t,		/**/
				off_t,			/*offset*/
				int,			/*prot*/
				int);			/*flags*/
static paddr_t	bus_space_mmap2(
				bus_space_tag_t,
				bus_type_t,
				bus_addr_t,		/**/
				off_t,			/*offset*/
				int,			/*prot*/
				int);			/*flags*/
static void	*bus_intr_establish(
				bus_space_tag_t,
				int,			/*bus-specific intr*/
				int,			/*device class level,
							  see machine/intr.h*/
				int,			/*flags*/
				int (*)(void *),	/*handler*/
				void *);		/*handler arg*/
static int	_bus_space_peek(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/*offset*/
				size_t,			/*probe size*/
				void *);		/*result ptr*/
static int	_bus_space_poke(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/*offset*/
				size_t,			/*probe size*/
				uint32_t);		/*value*/

/* This macro finds the first "upstream" implementation of method `f' */
#define _BS_CALL(t,f)			\
	while (t->f == NULL)		\
		t = t->parent;		\
	return (*(t)->f)

__inline int
bus_space_map(bus_space_tag_t t, bus_addr_t a, bus_size_t s, int f,
    bus_space_handle_t *hp)
{
	_BS_CALL(t, sun68k_bus_map)((t), 0, (a), (s), (f), 0, (hp));
}

__inline int
bus_space_map2(bus_space_tag_t t, bus_type_t bt, bus_addr_t a, bus_size_t s,
    int f, vaddr_t v, bus_space_handle_t *hp)
{
	_BS_CALL(t, sun68k_bus_map)(t, bt, a, s, f, v, hp);
}

__inline int
bus_space_unmap(bus_space_tag_t t, bus_space_handle_t h, bus_size_t s)
{
	_BS_CALL(t, sun68k_bus_unmap)(t, h, s);
}

__inline int
bus_space_subregion(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    bus_size_t s, bus_space_handle_t *hp)
{
	_BS_CALL(t, sun68k_bus_subregion)(t, h, o, s, hp);
}

__inline paddr_t
bus_space_mmap(bus_space_tag_t t, bus_addr_t a, off_t o, int p, int f)
{
	_BS_CALL(t, sun68k_bus_mmap)(t, 0, a, o, p, f);
}

__inline paddr_t
bus_space_mmap2(bus_space_tag_t	t, bus_type_t bt, bus_addr_t a, off_t o, int p,
    int f)
{
	_BS_CALL(t, sun68k_bus_mmap)(t, bt, a, o, p, f);
}

__inline void *
bus_intr_establish(bus_space_tag_t t, int p, int l, int f, int (*h)(void *),
    void *a)
{
	_BS_CALL(t, sun68k_intr_establish)(t, p, l, f, h, a);
}

__inline void
bus_space_barrier(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    bus_size_t s, int f)
{
	_BS_CALL(t, sun68k_bus_barrier)(t, h, o, s, f);
}

__inline int
_bus_space_peek(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, size_t s,
    void *vp)
{
	_BS_CALL(t, sun68k_bus_peek)(t, h, o, s, vp);
}

__inline int
_bus_space_poke(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, size_t s,
    uint32_t v)
{
	_BS_CALL(t, sun68k_bus_poke)(t, h, o, s, v);
}

#if 0
int	bus_space_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t,
	    bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
void	bus_space_free(bus_space_tag_t, bus_space_handle_t, bus_size_t);
#endif

/*
 *	void *bus_space_vaddr(bus_space_tag_t, bus_space_handle_t);
 *
 * Get the kernel virtual address for the mapped bus space.
 * Only allowed for regions mapped with BUS_SPACE_MAP_LINEAR.
 *  (XXX not enforced)
 */
#define bus_space_vaddr(t, h)	((void)(t), (void *)(h))

/* flags for bus space map functions */
#define BUS_SPACE_MAP_CACHEABLE	0x0001
#define BUS_SPACE_MAP_LINEAR	0x0002
#define BUS_SPACE_MAP_PREFETCHABLE	0x0004
#define BUS_SPACE_MAP_BUS1	0x0100	/* placeholders for bus functions... */
#define BUS_SPACE_MAP_BUS2	0x0200
#define BUS_SPACE_MAP_BUS3	0x0400
#define BUS_SPACE_MAP_BUS4	0x0800

/* Internal flag: try to find and use a PROM maping for the device. */
#define	_SUN68K_BUS_MAP_USE_PROM		BUS_SPACE_MAP_BUS1

/* flags for intr_establish() */
#define BUS_INTR_ESTABLISH_FASTTRAP	1
#define BUS_INTR_ESTABLISH_SOFTINTR	2

/* flags for bus_space_barrier() */
#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

/*
 *	int bus_space_peek_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t *valuep);
 *
 * Cautiously read 1, 2, 4 or 8 byte quantity from bus space described
 * by tag/handle/offset.
 * If no hardware responds to the read access, the function returns a
 * non-zero value. Otherwise the value read is placed in `valuep'.
 */

#define	bus_space_peek_1(t, h, o, vp)					\
    _bus_space_peek(t, h, o, sizeof(uint8_t), (void *)vp)

#define	bus_space_peek_2(t, h, o, vp)					\
    _bus_space_peek(t, h, o, sizeof(uint16_t), (void *)vp)

#define	bus_space_peek_4(t, h, o, vp)					\
    _bus_space_peek(t, h, o, sizeof(uint32_t), (void *)vp)

/*
 *	int bus_space_poke_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t value);
 *
 * Cautiously write 1, 2, 4 or 8 byte quantity to bus space described
 * by tag/handle/offset.
 * If no hardware responds to the write access, the function returns a
 * non-zero value.
 */

#define	bus_space_poke_1(t, h, o, v)					\
    _bus_space_poke(t, h, o, sizeof(uint8_t), v)

#define	bus_space_poke_2(t, h, o, v)					\
    _bus_space_poke(t, h, o, sizeof(uint16_t), v)

#define	bus_space_poke_4(t, h, o, v)					\
    _bus_space_poke(t, h, o, sizeof(uint32_t), v)

/*
 *	uintN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define	bus_space_read_1(t, h, o)					\
	    ((void)t, *(volatile uint8_t *)((h) + (o)))

#define	bus_space_read_2(t, h, o)					\
	    ((void)t, *(volatile uint16_t *)((h) + (o)))

#define	bus_space_read_4(t, h, o)					\
	    ((void)t, *(volatile uint32_t *)((h) + (o)))

#define	bus_space_read_8(t, h, o)					\
	    ((void)t, *(volatile uint64_t *)((h) + (o)))


/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define	bus_space_write_1(t, h, o, v)	do {				\
	((void)t, (void)(*(volatile uint8_t *)((h) + (o)) = (v)));	\
} while (0)

#define	bus_space_write_2(t, h, o, v)	do {				\
	((void)t, (void)(*(volatile uint16_t *)((h) + (o)) = (v)));	\
} while (0)

#define	bus_space_write_4(t, h, o, v)	do {				\
	((void)t, (void)(*(volatile uint32_t *)((h) + (o)) = (v)));	\
} while (0)

#define	bus_space_write_8(t, h, o, v)	do {				\
	((void)t, (void)(*(volatile uint64_t *)((h) + (o)) = (v)));	\
} while (0)


/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

void bus_space_read_multi_1(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			    uint8_t *, bus_size_t);
void bus_space_read_multi_2(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			    uint16_t *, bus_size_t);
void bus_space_read_multi_4(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			    uint32_t *, bus_size_t);
void bus_space_read_multi_8(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			    uint64_t *, bus_size_t);

extern __inline void
bus_space_read_multi_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint8_t *a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_1(t, h, o);
}

extern __inline void
bus_space_read_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint16_t *a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_2(t, h, o);
}

extern __inline void
bus_space_read_multi_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint32_t *a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_4(t, h, o);
}

extern __inline void
bus_space_read_multi_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint64_t *a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_8(t, h, o);
}


/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */
void bus_space_write_multi_1(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     const uint8_t *, bus_size_t);
void bus_space_write_multi_2(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     const uint16_t *, bus_size_t);
void bus_space_write_multi_4(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     const uint32_t *, bus_size_t);
void bus_space_write_multi_8(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     const uint64_t *, bus_size_t);

extern __inline void
bus_space_write_multi_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint8_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_1(t, h, o, *a++);
}

extern __inline void
bus_space_write_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint16_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_2(t, h, o, *a++);
}

extern __inline void
bus_space_write_multi_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint32_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_4(t, h, o, *a++);
}

extern __inline void
bus_space_write_multi_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint64_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_8(t, h, o, *a++);
}

/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *	    bus_size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */
void bus_space_set_multi_1(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			   const uint8_t, bus_size_t);
void bus_space_set_multi_2(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			   const uint16_t, bus_size_t);
void bus_space_set_multi_4(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			   const uint32_t, bus_size_t);
void bus_space_set_multi_8(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			   const uint64_t, bus_size_t);

extern __inline void
bus_space_set_multi_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint8_t v, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_1(t, h, o, v);
}

extern __inline void
bus_space_set_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint16_t v, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_2(t, h, o, v);
}

extern __inline void
bus_space_set_multi_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint32_t v, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_4(t, h, o, v);
}

extern __inline void
bus_space_set_multi_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint64_t v, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_8(t, h, o, v);
}


/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    uintN_t *addr, bus_size_t count);
 *
 */
void bus_space_read_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     uint8_t *, bus_size_t);
void bus_space_read_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     uint16_t *, bus_size_t);
void bus_space_read_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     uint32_t *, bus_size_t);
void bus_space_read_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     uint64_t *, bus_size_t);

extern __inline void
bus_space_read_region_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint8_t *a, bus_size_t c)
{
	for (; c; a++, c--, o++)
		*a = bus_space_read_1(t, h, o);
}
extern __inline void
bus_space_read_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint16_t *a, bus_size_t c)
{
	for (; c; a++, c--, o += 2)
		*a = bus_space_read_2(t, h, o);
}
extern __inline void
bus_space_read_region_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint32_t *a, bus_size_t c)
{
	for (; c; a++, c--, o += 4)
		*a = bus_space_read_4(t, h, o);
}
extern __inline void
bus_space_read_region_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint64_t *a, bus_size_t c)
{
	for (; c; a++, c--, o += 8)
		*a = bus_space_read_8(t, h, o);
}

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    uintN_t *addr, bus_size_t count);
 *
 */
void bus_space_write_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			      const uint8_t *, bus_size_t);
void bus_space_write_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			      const uint16_t *, bus_size_t);
void bus_space_write_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			      const uint32_t *, bus_size_t);
void bus_space_write_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			      const uint64_t *, bus_size_t);

extern __inline void
bus_space_write_region_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint8_t *a, bus_size_t c)
{
	for (; c; a++, c--, o++)
		bus_space_write_1(t, h, o, *a);
}

extern __inline void
bus_space_write_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint16_t *a, bus_size_t c)
{
	for (; c; a++, c--, o += 2)
		bus_space_write_2(t, h, o, *a);
}

extern __inline void
bus_space_write_region_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint32_t *a, bus_size_t c)
{
	for (; c; a++, c--, o += 4)
		bus_space_write_4(t, h, o, *a);
}

extern __inline void
bus_space_write_region_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint64_t *a, bus_size_t c)
{
	for (; c; a++, c--, o += 8)
		bus_space_write_8(t, h, o, *a);
}


/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    uintN_t *addr, bus_size_t count);
 *
 */
void bus_space_set_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			    const uint8_t, bus_size_t);
void bus_space_set_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			    const uint16_t, bus_size_t);
void bus_space_set_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			    const uint32_t, bus_size_t);
void bus_space_set_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			    const uint64_t, bus_size_t);

extern __inline void
bus_space_set_region_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint8_t v, bus_size_t c)
{
	for (; c; c--, o++)
		bus_space_write_1(t, h, o, v);
}

extern __inline void
bus_space_set_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint16_t v, bus_size_t c)
{
	for (; c; c--, o += 2)
		bus_space_write_2(t, h, o, v);
}

extern __inline void
bus_space_set_region_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint32_t v, bus_size_t c)
{
	for (; c; c--, o += 4)
		bus_space_write_4(t, h, o, v);
}

extern __inline void
bus_space_set_region_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const uint64_t v, bus_size_t c)
{
	for (; c; c--, o += 8)
		bus_space_write_8(t, h, o, v);
}


/*
 *	void bus_space_copy_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    bus_size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */
void bus_space_copy_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     bus_space_handle_t, bus_size_t, bus_size_t);
void bus_space_copy_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     bus_space_handle_t, bus_size_t, bus_size_t);
void bus_space_copy_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     bus_space_handle_t, bus_size_t, bus_size_t);
void bus_space_copy_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t,
			     bus_space_handle_t, bus_size_t, bus_size_t);

extern __inline void
bus_space_copy_region_1(bus_space_tag_t t, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1++, o2++)
	    bus_space_write_1(t, h1, o1, bus_space_read_1(t, h2, o2));
}

extern __inline void
bus_space_copy_region_2(bus_space_tag_t t, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1 += 2, o2 += 2)
	    bus_space_write_2(t, h1, o1, bus_space_read_2(t, h2, o2));
}

extern __inline void
bus_space_copy_region_4(bus_space_tag_t t, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1 += 4, o2 += 4)
	    bus_space_write_4(t, h1, o1, bus_space_read_4(t, h2, o2));
}

extern __inline void
bus_space_copy_region_8(bus_space_tag_t t, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1 += 8, o2 += 8)
	    bus_space_write_8(t, h1, o1, bus_space_read_8(t, h2, o2));
}

/*
 *	void bus_space_copyin(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    void *addr, bus_size_t count);
 *
 * Copy `count' bytes from bus space starting at tag/bsh/off 
 * to kernel memory at addr using the most optimized transfer
 * possible for the bus.
 */

#define	bus_space_copyin(t, h, o, a, c)					\
	    ((void)t, w16copy((uint8_t *)((h) + (o)), (a), (c)))

/*
 *	void bus_space_copyout(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    const void *addr, bus_size_t count);
 *
 * Copy `count' bytes to bus space starting at tag/bsh/off 
 * from kernel memory at addr using the most optimized transfer
 * possible for the bus.
 */

#define	bus_space_copyout(t, h, o, a, c)				\
	    ((void)t, w16copy((a), (uint8_t *)((h) + (o)), (c)))

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

int find_prom_map(paddr_t, bus_type_t, int, vaddr_t *);

/*--------------------------------*/

/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_WAITOK		0x000	/* safe to sleep (pseudo-flag) */
#define	BUS_DMA_NOWAIT		0x001	/* not safe to sleep */
#define	BUS_DMA_ALLOCNOW	0x002	/* perform resource allocation now */
#define	BUS_DMA_COHERENT	0x004	/* hint: map memory DMA coherent */
#define	BUS_DMA_BUS1		0x010	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2		0x020
#define	BUS_DMA_BUS3		0x040
#define	BUS_DMA_BUS4		0x080
#define	BUS_DMA_READ		0x100	/* mapping is device -> memory only */
#define	BUS_DMA_WRITE		0x200	/* mapping is memory -> device only */
#define	BUS_DMA_NOCACHE		0x400	/* hint: map non-cached memory */

/* For devices that have a 24-bit address space */
#define BUS_DMA_24BIT		BUS_DMA_BUS1

/* Internal flag: current DVMA address is equal to the KVA buffer address */
#define _BUS_DMA_DIRECTMAP	BUS_DMA_BUS2

/*
 * Internal flag: current DVMA address has been double-mapped by hand
 * to the KVA buffer address (without the pmap's help).
 */
#define	_BUS_DMA_NOPMAP		BUS_DMA_BUS3

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

typedef struct sun68k_bus_dma_tag	*bus_dma_tag_t;
typedef struct sun68k_bus_dmamap	*bus_dmamap_t;

#define BUS_DMA_TAG_VALID(t)    ((t) != NULL)

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct sun68k_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DVMA address */
	bus_size_t	ds_len;		/* length of transfer */
	bus_size_t	_ds_sgsize;	/* size of allocated DVMA segment */
	void		*_ds_mlist;	/* page list when dmamem_alloc'ed */
	vaddr_t		_ds_va;		/* VA when dmamem_map'ed */
};
typedef struct sun68k_bus_dma_segment	bus_dma_segment_t;


/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */
struct sun68k_bus_dma_tag {
	void	*_cookie;		/* cookie used in the guts */

	/*
	 * DMA mapping methods.
	 */
	int	(*_dmamap_create)(bus_dma_tag_t, bus_size_t, int, bus_size_t,
		    bus_size_t, int, bus_dmamap_t *);
	void	(*_dmamap_destroy)(bus_dma_tag_t, bus_dmamap_t);
	int	(*_dmamap_load)(bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t,
		    struct proc *, int);
	int	(*_dmamap_load_mbuf)(bus_dma_tag_t, bus_dmamap_t, struct mbuf *,
		    int);
	int	(*_dmamap_load_uio)(bus_dma_tag_t, bus_dmamap_t, struct uio *,
		    int);
	int	(*_dmamap_load_raw)(bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);
	void	(*_dmamap_unload)(bus_dma_tag_t, bus_dmamap_t);
	void	(*_dmamap_sync)(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
		    bus_size_t, int);

	/*
	 * DMA memory utility functions.
	 */
	int	(*_dmamem_alloc)(bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*_dmamem_free)(bus_dma_tag_t, bus_dma_segment_t *, int);
	int	(*_dmamem_map)(bus_dma_tag_t, bus_dma_segment_t *, int, size_t,
		    void **, int);
	void	(*_dmamem_unmap)(bus_dma_tag_t, void *, size_t);
	paddr_t	(*_dmamem_mmap)(bus_dma_tag_t, bus_dma_segment_t *, int, off_t,
		    int, int);
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

#define bus_dmatag_subregion(t, mna, mxa, nt, f) EOPNOTSUPP
#define bus_dmatag_destroy(t)

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct sun68k_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use by machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxmaxsegsz; /* fixed largest possible segment */
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
	bus_size_t	dm_maxsegsz;	/* largest possible segment */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#ifdef _SUN68K_BUS_DMA_PRIVATE
int	_bus_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *);
void	_bus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int	_bus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t, struct mbuf *, int);
int	_bus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t, struct uio *, int);
int	_bus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t, bus_dma_segment_t *,
	    int, bus_size_t, int);
int	_bus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t,
	    struct proc *, int);
void	_bus_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t, bus_size_t,
	    int);

int	_bus_dmamem_alloc(bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t,
	    bus_dma_segment_t *, int, int *, int);
void	_bus_dmamem_free(bus_dma_tag_t, bus_dma_segment_t *, int);
int	_bus_dmamem_map(bus_dma_tag_t, bus_dma_segment_t *, int, size_t,
	    void **, int);
void	_bus_dmamem_unmap(bus_dma_tag_t, void *, size_t);
paddr_t	_bus_dmamem_mmap(bus_dma_tag_t, bus_dma_segment_t *, int, off_t, int,
	    int);

int	_bus_dmamem_alloc_range(bus_dma_tag_t, bus_size_t, bus_size_t,
	    bus_size_t, bus_dma_segment_t *, int, int *, int, vaddr_t, vaddr_t);

vaddr_t	_bus_dma_valloc_skewed(size_t, u_long, u_long, u_long);
#endif /* _SUN68K_BUS_DMA_PRIVATE */

#endif /* _SUN68K_BUS_H_ */
