/* $NetBSD: tc_bus_mem.c,v 1.35.6.1 2016/10/05 20:55:23 skrll Exp $ */

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Common TurboChannel Chipset "bus memory" functions.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: tc_bus_mem.c,v 1.35.6.1 2016/10/05 20:55:23 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <dev/tc/tcvar.h>

#define	__C(A,B)	__CONCAT(A,B)

/* mapping/unmapping */
int		tc_mem_map(void *, bus_addr_t, bus_size_t, int,
		    bus_space_handle_t *, int);
void		tc_mem_unmap(void *, bus_space_handle_t, bus_size_t, int);
int		tc_mem_subregion(void *, bus_space_handle_t, bus_size_t,
		    bus_size_t, bus_space_handle_t *);

int		tc_mem_translate(void *, bus_addr_t, bus_size_t,
		    int, struct alpha_bus_space_translation *);
int		tc_mem_get_window(void *, int,
		    struct alpha_bus_space_translation *);

/* allocation/deallocation */
int		tc_mem_alloc(void *, bus_addr_t, bus_addr_t, bus_size_t,
		    bus_size_t, bus_addr_t, int, bus_addr_t *,
		    bus_space_handle_t *);
void		tc_mem_free(void *, bus_space_handle_t, bus_size_t);

/* get kernel virtual address */
void *		tc_mem_vaddr(void *, bus_space_handle_t);

/* mmap for user */
paddr_t		tc_mem_mmap(void *, bus_addr_t, off_t, int, int);

/* barrier */
static inline void	tc_mem_barrier(void *, bus_space_handle_t,
		    bus_size_t, bus_size_t, int);

/* read (single) */
static inline uint8_t	tc_mem_read_1(void *, bus_space_handle_t, bus_size_t);
static inline uint16_t tc_mem_read_2(void *, bus_space_handle_t, bus_size_t);
static inline uint32_t tc_mem_read_4(void *, bus_space_handle_t, bus_size_t);
static inline uint64_t tc_mem_read_8(void *, bus_space_handle_t, bus_size_t);

/* read multiple */
void		tc_mem_read_multi_1(void *, bus_space_handle_t,
		    bus_size_t, uint8_t *, bus_size_t);
void		tc_mem_read_multi_2(void *, bus_space_handle_t,
		    bus_size_t, uint16_t *, bus_size_t);
void		tc_mem_read_multi_4(void *, bus_space_handle_t,
		    bus_size_t, uint32_t *, bus_size_t);
void		tc_mem_read_multi_8(void *, bus_space_handle_t,
		    bus_size_t, uint64_t *, bus_size_t);

/* read region */
void		tc_mem_read_region_1(void *, bus_space_handle_t,
		    bus_size_t, uint8_t *, bus_size_t);
void		tc_mem_read_region_2(void *, bus_space_handle_t,
		    bus_size_t, uint16_t *, bus_size_t);
void		tc_mem_read_region_4(void *, bus_space_handle_t,
		    bus_size_t, uint32_t *, bus_size_t);
void		tc_mem_read_region_8(void *, bus_space_handle_t,
		    bus_size_t, uint64_t *, bus_size_t);

/* write (single) */
static inline void	tc_mem_write_1(void *, bus_space_handle_t, bus_size_t,
		    uint8_t);
static inline void	tc_mem_write_2(void *, bus_space_handle_t, bus_size_t,
		    uint16_t);
static inline void	tc_mem_write_4(void *, bus_space_handle_t, bus_size_t,
		    uint32_t);
static inline void	tc_mem_write_8(void *, bus_space_handle_t, bus_size_t,
		    uint64_t);

/* write multiple */
void		tc_mem_write_multi_1(void *, bus_space_handle_t,
		    bus_size_t, const uint8_t *, bus_size_t);
void		tc_mem_write_multi_2(void *, bus_space_handle_t,
		    bus_size_t, const uint16_t *, bus_size_t);
void		tc_mem_write_multi_4(void *, bus_space_handle_t,
		    bus_size_t, const uint32_t *, bus_size_t);
void		tc_mem_write_multi_8(void *, bus_space_handle_t,
		    bus_size_t, const uint64_t *, bus_size_t);

/* write region */
void		tc_mem_write_region_1(void *, bus_space_handle_t,
		    bus_size_t, const uint8_t *, bus_size_t);
void		tc_mem_write_region_2(void *, bus_space_handle_t,
		    bus_size_t, const uint16_t *, bus_size_t);
void		tc_mem_write_region_4(void *, bus_space_handle_t,
		    bus_size_t, const uint32_t *, bus_size_t);
void		tc_mem_write_region_8(void *, bus_space_handle_t,
		    bus_size_t, const uint64_t *, bus_size_t);

/* set multiple */
void		tc_mem_set_multi_1(void *, bus_space_handle_t,
		    bus_size_t, uint8_t, bus_size_t);
void		tc_mem_set_multi_2(void *, bus_space_handle_t,
		    bus_size_t, uint16_t, bus_size_t);
void		tc_mem_set_multi_4(void *, bus_space_handle_t,
		    bus_size_t, uint32_t, bus_size_t);
void		tc_mem_set_multi_8(void *, bus_space_handle_t,
		    bus_size_t, uint64_t, bus_size_t);

/* set region */
void		tc_mem_set_region_1(void *, bus_space_handle_t,
		    bus_size_t, uint8_t, bus_size_t);
void		tc_mem_set_region_2(void *, bus_space_handle_t,
		    bus_size_t, uint16_t, bus_size_t);
void		tc_mem_set_region_4(void *, bus_space_handle_t,
		    bus_size_t, uint32_t, bus_size_t);
void		tc_mem_set_region_8(void *, bus_space_handle_t,
		    bus_size_t, uint64_t, bus_size_t);

/* copy */
void		tc_mem_copy_region_1(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void		tc_mem_copy_region_2(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void		tc_mem_copy_region_4(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void		tc_mem_copy_region_8(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);

static struct alpha_bus_space tc_mem_space = {
	/* cookie */
	NULL,

	/* mapping/unmapping */
	tc_mem_map,
	tc_mem_unmap,
	tc_mem_subregion,

	tc_mem_translate,
	tc_mem_get_window,

	/* allocation/deallocation */
	tc_mem_alloc,
	tc_mem_free,

	/* get kernel virtual address */
	tc_mem_vaddr,

	/* mmap for user */
	tc_mem_mmap,

	/* barrier */
	tc_mem_barrier,

	/* read (single) */
	tc_mem_read_1,
	tc_mem_read_2,
	tc_mem_read_4,
	tc_mem_read_8,

	/* read multiple */
	tc_mem_read_multi_1,
	tc_mem_read_multi_2,
	tc_mem_read_multi_4,
	tc_mem_read_multi_8,

	/* read region */
	tc_mem_read_region_1,
	tc_mem_read_region_2,
	tc_mem_read_region_4,
	tc_mem_read_region_8,

	/* write (single) */
	tc_mem_write_1,
	tc_mem_write_2,
	tc_mem_write_4,
	tc_mem_write_8,

	/* write multiple */
	tc_mem_write_multi_1,
	tc_mem_write_multi_2,
	tc_mem_write_multi_4,
	tc_mem_write_multi_8,

	/* write region */
	tc_mem_write_region_1,
	tc_mem_write_region_2,
	tc_mem_write_region_4,
	tc_mem_write_region_8,

	/* set multiple */
	tc_mem_set_multi_1,
	tc_mem_set_multi_2,
	tc_mem_set_multi_4,
	tc_mem_set_multi_8,

	/* set region */
	tc_mem_set_region_1,
	tc_mem_set_region_2,
	tc_mem_set_region_4,
	tc_mem_set_region_8,

	/* copy */
	tc_mem_copy_region_1,
	tc_mem_copy_region_2,
	tc_mem_copy_region_4,
	tc_mem_copy_region_8,
};

bus_space_tag_t
tc_bus_mem_init(void *memv)
{
	bus_space_tag_t h = &tc_mem_space;

	h->abs_cookie = memv;
	return (h);
}

/* ARGSUSED */
int
tc_mem_translate(void *v, bus_addr_t memaddr, bus_size_t memlen, int flags, struct alpha_bus_space_translation *abst)
{

	return (EOPNOTSUPP);
}

/* ARGSUSED */
int
tc_mem_get_window(void *v, int window, struct alpha_bus_space_translation *abst)
{

	return (EOPNOTSUPP);
}

/* ARGSUSED */
int
tc_mem_map(void *v, bus_addr_t memaddr, bus_size_t memsize, int flags, bus_space_handle_t *memhp, int acct)
{
	int cacheable = flags & BUS_SPACE_MAP_CACHEABLE;
	int linear = flags & BUS_SPACE_MAP_LINEAR;

	/* Requests for linear uncacheable space can't be satisfied. */
	if (linear && !cacheable)
		return (EOPNOTSUPP);

	if (memaddr & 0x7)
		panic("%s: need 8 byte alignment", __func__);
	if (cacheable)
		*memhp = ALPHA_PHYS_TO_K0SEG(memaddr);
	else
		*memhp = ALPHA_PHYS_TO_K0SEG(TC_DENSE_TO_SPARSE(memaddr));
	return (0);
}

/* ARGSUSED */
void
tc_mem_unmap(void *v, bus_space_handle_t memh, bus_size_t memsize, int acct)
{

	/* XXX XX XXX nothing to do. */
}

int
tc_mem_subregion(void *v, bus_space_handle_t memh, bus_size_t offset, bus_size_t size, bus_space_handle_t *nmemh)
{

	/* Disallow subregioning that would make the handle unaligned. */
	if ((offset & 0x7) != 0)
		return (1);

	if ((memh & TC_SPACE_SPARSE) != 0)
		*nmemh = memh + (offset << 1);
	else
		*nmemh = memh + offset;

	return (0);
}

int
tc_mem_alloc(void *v, bus_addr_t rstart, bus_addr_t rend, bus_size_t size, bus_size_t align, bus_size_t boundary, int flags, bus_addr_t *addrp, bus_space_handle_t *bshp)
{

	/* XXX XXX XXX XXX XXX XXX */
	panic("%s: unimplemented", __func__);
}

void
tc_mem_free(void *v, bus_space_handle_t bsh, bus_size_t size)
{

	/* XXX XXX XXX XXX XXX XXX */
	panic("%s: unimplemented", __func__);
}

void *
tc_mem_vaddr(void *v, bus_space_handle_t bsh)
{
#ifdef DIAGNOSTIC
	if ((bsh & TC_SPACE_SPARSE) != 0) {
		/*
		 * tc_mem_map() catches linear && !cacheable,
		 * so we shouldn't come here
		 */
		panic("%s: can't do sparse", __func__);
	}
#endif
	return ((void *)bsh);
}

paddr_t
tc_mem_mmap(void *v, bus_addr_t addr, off_t off, int prot, int flags)
{
	int linear = flags & BUS_SPACE_MAP_LINEAR;
	bus_addr_t rv;

	if (linear)
		rv = addr + off;
	else
		rv = TC_DENSE_TO_SPARSE(addr + off);

	return (alpha_btop(rv));
}

static inline void
tc_mem_barrier(void *v, bus_space_handle_t h, bus_size_t o, bus_size_t l, int f)
{

	if ((f & BUS_SPACE_BARRIER_READ) != 0)
		alpha_mb();
	else if ((f & BUS_SPACE_BARRIER_WRITE) != 0)
		alpha_wmb();
}

/*
 * https://web-docs.gsi.de/~kraemer/COLLECTION/DEC/d3syspmb.pdf
 * http://h20565.www2.hpe.com/hpsc/doc/public/display?docId=emr_na-c04623255
 */
#define TC_SPARSE_PTR(memh, off) \
    ((void *)((memh) + ((off & ((bus_size_t)-1 << 2)) << 1)))

static inline uint8_t
tc_mem_read_1(void *v, bus_space_handle_t memh, bus_size_t off)
{

	alpha_mb();		/* XXX XXX XXX */

	if ((memh & TC_SPACE_SPARSE) != 0) {
		volatile uint32_t *p;

		p = TC_SPARSE_PTR(memh, off);
		return ((*p >> ((off & 3) << 3)) & 0xff);
	} else {
		volatile uint8_t *p;

		p = (uint8_t *)(memh + off);
		return (*p);
	}
}

static inline uint16_t
tc_mem_read_2(void *v, bus_space_handle_t memh, bus_size_t off)
{

	alpha_mb();		/* XXX XXX XXX */

	if ((memh & TC_SPACE_SPARSE) != 0) {
		volatile uint32_t *p;

		p = TC_SPARSE_PTR(memh, off);
		return ((*p >> ((off & 2) << 3)) & 0xffff);
	} else {
		volatile uint16_t *p;

		p = (uint16_t *)(memh + off);
		return (*p);
	}
}

static inline uint32_t
tc_mem_read_4(void *v, bus_space_handle_t memh, bus_size_t off)
{
	volatile uint32_t *p;

	alpha_mb();		/* XXX XXX XXX */

	if ((memh & TC_SPACE_SPARSE) != 0)
		/* Nothing special to do for 4-byte sparse space accesses */
		p = (uint32_t *)(memh + (off << 1));
	else
		/*
		 * LDL to a dense space address always results in two
		 * TURBOchannel I/O read transactions to consecutive longword
		 * addresses. Use caution in dense space if the option has
		 * registers with read side effects.
		 */
		p = (uint32_t *)(memh + off);
	return (*p);
}

static inline uint64_t
tc_mem_read_8(void *v, bus_space_handle_t memh, bus_size_t off)
{
	volatile uint64_t *p;

	alpha_mb();		/* XXX XXX XXX */

	if ((memh & TC_SPACE_SPARSE) != 0)
		panic("%s: not implemented for sparse space", __func__);

	p = (uint64_t *)(memh + off);
	return (*p);
}

#define	tc_mem_read_multi_N(BYTES,TYPE)					\
void									\
__C(tc_mem_read_multi_,BYTES)(						\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	TYPE *a,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		tc_mem_barrier(v, h, o, sizeof *a, BUS_SPACE_BARRIER_READ); \
		*a++ = __C(tc_mem_read_,BYTES)(v, h, o);		\
	}								\
}
tc_mem_read_multi_N(1,uint8_t)
tc_mem_read_multi_N(2,uint16_t)
tc_mem_read_multi_N(4,uint32_t)
tc_mem_read_multi_N(8,uint64_t)

#define	tc_mem_read_region_N(BYTES,TYPE)				\
void									\
__C(tc_mem_read_region_,BYTES)(						\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	TYPE *a,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		*a++ = __C(tc_mem_read_,BYTES)(v, h, o);		\
		o += sizeof *a;						\
	}								\
}
tc_mem_read_region_N(1,uint8_t)
tc_mem_read_region_N(2,uint16_t)
tc_mem_read_region_N(4,uint32_t)
tc_mem_read_region_N(8,uint64_t)

#define TC_SPARSE_WR_PVAL(msk, b, v) \
    ((UINT64_C(msk) << (32 + (b))) | ((uint64_t)(v) << ((b) << 3)))

static inline void
tc_mem_write_1(void *v, bus_space_handle_t memh, bus_size_t off, uint8_t val)
{

	if ((memh & TC_SPACE_SPARSE) != 0) {
		volatile uint64_t *p;

		p = TC_SPARSE_PTR(memh, off);
		*p = TC_SPARSE_WR_PVAL(0x1, off & 3, val);
	} else
		panic("%s: not implemented for dense space", __func__);

	alpha_mb();		/* XXX XXX XXX */
}

static inline void
tc_mem_write_2(void *v, bus_space_handle_t memh, bus_size_t off, uint16_t val)
{

	if ((memh & TC_SPACE_SPARSE) != 0) {
		volatile uint64_t *p;

		p = TC_SPARSE_PTR(memh, off);
		*p = TC_SPARSE_WR_PVAL(0x3, off & 2, val);
	} else
		panic("%s: not implemented for dense space", __func__);

	alpha_mb();		/* XXX XXX XXX */
}

static inline void
tc_mem_write_4(void *v, bus_space_handle_t memh, bus_size_t off, uint32_t val)
{
	volatile uint32_t *p;

	if ((memh & TC_SPACE_SPARSE) != 0)
		/* Nothing special to do for 4-byte sparse space accesses */
		p = (uint32_t *)(memh + (off << 1));
	else
		p = (uint32_t *)(memh + off);
	*p = val;

	alpha_mb();		/* XXX XXX XXX */
}

static inline void
tc_mem_write_8(void *v, bus_space_handle_t memh, bus_size_t off, uint64_t val)
{
	volatile uint64_t *p;

	if ((memh & TC_SPACE_SPARSE) != 0)
		panic("%s: not implemented for sparse space", __func__);

	p = (uint64_t *)(memh + off);
	*p = val;

	alpha_mb();		/* XXX XXX XXX */
}

#define	tc_mem_write_multi_N(BYTES,TYPE)				\
void									\
__C(tc_mem_write_multi_,BYTES)(						\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	const TYPE *a,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		__C(tc_mem_write_,BYTES)(v, h, o, *a++);		\
		tc_mem_barrier(v, h, o, sizeof *a, BUS_SPACE_BARRIER_WRITE); \
	}								\
}
tc_mem_write_multi_N(1,uint8_t)
tc_mem_write_multi_N(2,uint16_t)
tc_mem_write_multi_N(4,uint32_t)
tc_mem_write_multi_N(8,uint64_t)

#define	tc_mem_write_region_N(BYTES,TYPE)				\
void									\
__C(tc_mem_write_region_,BYTES)(					\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	const TYPE *a,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		__C(tc_mem_write_,BYTES)(v, h, o, *a++);		\
		o += sizeof *a;						\
	}								\
}
tc_mem_write_region_N(1,uint8_t)
tc_mem_write_region_N(2,uint16_t)
tc_mem_write_region_N(4,uint32_t)
tc_mem_write_region_N(8,uint64_t)

#define	tc_mem_set_multi_N(BYTES,TYPE)					\
void									\
__C(tc_mem_set_multi_,BYTES)(						\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	TYPE val,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		__C(tc_mem_write_,BYTES)(v, h, o, val);			\
		tc_mem_barrier(v, h, o, sizeof val, BUS_SPACE_BARRIER_WRITE); \
	}								\
}
tc_mem_set_multi_N(1,uint8_t)
tc_mem_set_multi_N(2,uint16_t)
tc_mem_set_multi_N(4,uint32_t)
tc_mem_set_multi_N(8,uint64_t)

#define	tc_mem_set_region_N(BYTES,TYPE)					\
void									\
__C(tc_mem_set_region_,BYTES)(						\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	TYPE val,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		__C(tc_mem_write_,BYTES)(v, h, o, val);			\
		o += sizeof val;					\
	}								\
}
tc_mem_set_region_N(1,uint8_t)
tc_mem_set_region_N(2,uint16_t)
tc_mem_set_region_N(4,uint32_t)
tc_mem_set_region_N(8,uint64_t)

#define	tc_mem_copy_region_N(BYTES)					\
void									\
__C(tc_mem_copy_region_,BYTES)(						\
	void *v,							\
	bus_space_handle_t h1,						\
	bus_size_t o1,							\
	bus_space_handle_t h2,						\
	bus_size_t o2,							\
	bus_size_t c)							\
{									\
	bus_size_t o;							\
									\
	if ((h1 & TC_SPACE_SPARSE) != 0 &&				\
	    (h2 & TC_SPACE_SPARSE) != 0) {				\
		memmove((void *)(h2 + o2), (void *)(h1 + o1), c * BYTES); \
		return;							\
	}								\
									\
	if (h1 + o1 >= h2 + o2)						\
		/* src after dest: copy forward */			\
		for (o = 0; c > 0; c--, o += BYTES)			\
			__C(tc_mem_write_,BYTES)(v, h2, o2 + o,		\
			    __C(tc_mem_read_,BYTES)(v, h1, o1 + o));	\
	else								\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * BYTES; c > 0; c--, o -= BYTES)	\
			__C(tc_mem_write_,BYTES)(v, h2, o2 + o,		\
			    __C(tc_mem_read_,BYTES)(v, h1, o1 + o));	\
}
tc_mem_copy_region_N(1)
tc_mem_copy_region_N(2)
tc_mem_copy_region_N(4)
tc_mem_copy_region_N(8)
