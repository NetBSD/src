/* $NetBSD: a12c_bus_mem.c,v 1.5.148.1 2009/05/13 17:16:05 jym Exp $ */

/* [Notice revision 2.0]
 * Copyright (c) 1997 Avalon Computer Systems, Inc.
 * All rights reserved.
 *
 * Author: Ross Harvey
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright and
 *    author notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Avalon Computer Systems, Inc. nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. This copyright will be assigned to The NetBSD Foundation on
 *    1/1/2000 unless these terms (including possibly the assignment
 *    date) are updated in writing by Avalon prior to the latest specified
 *    assignment date.
 *
 * THIS SOFTWARE IS PROVIDED BY AVALON COMPUTER SYSTEMS, INC. AND CONTRIBUTORS
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

#include "opt_avalon_a12.h"		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <alpha/pci/a12creg.h>
#include <alpha/pci/a12cvar.h>

#define	A12C_BUS_MEM()	/* Generate ctags(1) key */

__KERNEL_RCSID(0, "$NetBSD: a12c_bus_mem.c,v 1.5.148.1 2009/05/13 17:16:05 jym Exp $");

/* Memory barrier */
void		pci_a12c_mem_barrier(void *, bus_space_handle_t,
		    bus_size_t, bus_size_t, int);

/* Memory read (single) */
u_int8_t	pci_a12c_mem_read_1(void *, bus_space_handle_t,
		    bus_size_t);
u_int16_t	pci_a12c_mem_read_2(void *, bus_space_handle_t,
		    bus_size_t);
u_int32_t	pci_a12c_mem_read_4(void *, bus_space_handle_t,
		    bus_size_t);
u_int64_t	pci_a12c_mem_read_8(void *, bus_space_handle_t,
		    bus_size_t);

/* Memory read multiple */
void		pci_a12c_mem_read_multi_1(void *, bus_space_handle_t,
		    bus_size_t, u_int8_t *, bus_size_t);
void		pci_a12c_mem_read_multi_2(void *, bus_space_handle_t,
		    bus_size_t, u_int16_t *, bus_size_t);
void		pci_a12c_mem_read_multi_4(void *, bus_space_handle_t,
		    bus_size_t, u_int32_t *, bus_size_t);
void		pci_a12c_mem_read_multi_8(void *, bus_space_handle_t,
		    bus_size_t, u_int64_t *, bus_size_t);

/* Memory read region */
void		pci_a12c_mem_read_region_1(void *, bus_space_handle_t,
		    bus_size_t, u_int8_t *, bus_size_t);
void		pci_a12c_mem_read_region_2(void *, bus_space_handle_t,
		    bus_size_t, u_int16_t *, bus_size_t);
void		pci_a12c_mem_read_region_4(void *, bus_space_handle_t,
		    bus_size_t, u_int32_t *, bus_size_t);
void		pci_a12c_mem_read_region_8(void *, bus_space_handle_t,
		    bus_size_t, u_int64_t *, bus_size_t);

/* Memory write (single) */
void		pci_a12c_mem_write_1(void *, bus_space_handle_t,
		    bus_size_t, u_int8_t);
void		pci_a12c_mem_write_2(void *, bus_space_handle_t,
		    bus_size_t, u_int16_t);
void		pci_a12c_mem_write_4(void *, bus_space_handle_t,
		    bus_size_t, u_int32_t);
void		pci_a12c_mem_write_8(void *, bus_space_handle_t,
		    bus_size_t, u_int64_t);

/* Memory write multiple */
void		pci_a12c_mem_write_multi_1(void *, bus_space_handle_t,
		    bus_size_t, const u_int8_t *, bus_size_t);
void		pci_a12c_mem_write_multi_2(void *, bus_space_handle_t,
		    bus_size_t, const u_int16_t *, bus_size_t);
void		pci_a12c_mem_write_multi_4(void *, bus_space_handle_t,
		    bus_size_t, const u_int32_t *, bus_size_t);
void		pci_a12c_mem_write_multi_8(void *, bus_space_handle_t,
		    bus_size_t, const u_int64_t *, bus_size_t);

/* Memory write region */
void		pci_a12c_mem_write_region_1(void *, bus_space_handle_t,
		    bus_size_t, const u_int8_t *, bus_size_t);
void		pci_a12c_mem_write_region_2(void *, bus_space_handle_t,
		    bus_size_t, const u_int16_t *, bus_size_t);
void		pci_a12c_mem_write_region_4(void *, bus_space_handle_t,
		    bus_size_t, const u_int32_t *, bus_size_t);
void		pci_a12c_mem_write_region_8(void *, bus_space_handle_t,
		    bus_size_t, const u_int64_t *, bus_size_t);

/* Memory set multiple */
void		pci_a12c_mem_set_multi_1(void *, bus_space_handle_t,
		    bus_size_t, u_int8_t, bus_size_t);
void		pci_a12c_mem_set_multi_2(void *, bus_space_handle_t,
		    bus_size_t, u_int16_t, bus_size_t);
void		pci_a12c_mem_set_multi_4(void *, bus_space_handle_t,
		    bus_size_t, u_int32_t, bus_size_t);
void		pci_a12c_mem_set_multi_8(void *, bus_space_handle_t,
		    bus_size_t, u_int64_t, bus_size_t);

/* Memory set region */
void		pci_a12c_mem_set_region_1(void *, bus_space_handle_t,
		    bus_size_t, u_int8_t, bus_size_t);
void		pci_a12c_mem_set_region_2(void *, bus_space_handle_t,
		    bus_size_t, u_int16_t, bus_size_t);
void		pci_a12c_mem_set_region_4(void *, bus_space_handle_t,
		    bus_size_t, u_int32_t, bus_size_t);
void		pci_a12c_mem_set_region_8(void *, bus_space_handle_t,
		    bus_size_t, u_int64_t, bus_size_t);

/* Memory copy */
void		pci_a12c_mem_copy_region_1(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void		pci_a12c_mem_copy_region_2(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void		pci_a12c_mem_copy_region_4(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void		pci_a12c_mem_copy_region_8(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);

#define	__S(S)		__STRING(S)

/* mapping/unmapping */
int		pci_a12c_mem_map(void *, bus_addr_t, bus_size_t, int,
		    bus_space_handle_t *, int);
void		pci_a12c_mem_unmap(void *, bus_space_handle_t,
		    bus_size_t, int);
int		pci_a12c_mem_subregion(void *, bus_space_handle_t,
		    bus_size_t, bus_size_t, bus_space_handle_t *);

/* allocation/deallocation */
int		pci_a12c_mem_alloc(void *, bus_addr_t, bus_addr_t,
		    bus_size_t, bus_size_t, bus_addr_t, int, bus_addr_t *,
                    bus_space_handle_t *);
void		pci_a12c_mem_free(void *, bus_space_handle_t,
		    bus_size_t);

/* get kernel virtual address*/
void		*pci_a12c_mem_vaddr(void *, bus_space_handle_t);

static struct alpha_bus_space pci_a12c_mem_space = {
	/* cookie */
	NULL,

	/* mapping/unmapping */
	pci_a12c_mem_map,
	pci_a12c_mem_unmap,
	pci_a12c_mem_subregion,

	/* allocation/deallocation */
	pci_a12c_mem_alloc,
	pci_a12c_mem_free,

	/* get kernel virtual address */
	pci_a12c_mem_vaddr,

	/* barrier */
	pci_a12c_mem_barrier,
	
	/* read (single) */
	pci_a12c_mem_read_1,
	pci_a12c_mem_read_2,
	pci_a12c_mem_read_4,
	pci_a12c_mem_read_8,
	
	/* read multiple */
	pci_a12c_mem_read_multi_1,
	pci_a12c_mem_read_multi_2,
	pci_a12c_mem_read_multi_4,
	pci_a12c_mem_read_multi_8,
	
	/* read region */
	pci_a12c_mem_read_region_1,
	pci_a12c_mem_read_region_2,
	pci_a12c_mem_read_region_4,
	pci_a12c_mem_read_region_8,
	
	/* write (single) */
	pci_a12c_mem_write_1,
	pci_a12c_mem_write_2,
	pci_a12c_mem_write_4,
	pci_a12c_mem_write_8,
	
	/* write multiple */
	pci_a12c_mem_write_multi_1,
	pci_a12c_mem_write_multi_2,
	pci_a12c_mem_write_multi_4,
	pci_a12c_mem_write_multi_8,
	
	/* write region */
	pci_a12c_mem_write_region_1,
	pci_a12c_mem_write_region_2,
	pci_a12c_mem_write_region_4,
	pci_a12c_mem_write_region_8,

	/* set multiple */
	pci_a12c_mem_set_multi_1,
	pci_a12c_mem_set_multi_2,
	pci_a12c_mem_set_multi_4,
	pci_a12c_mem_set_multi_8,
	
	/* set region */
	pci_a12c_mem_set_region_1,
	pci_a12c_mem_set_region_2,
	pci_a12c_mem_set_region_4,
	pci_a12c_mem_set_region_8,

	/* copy */
	pci_a12c_mem_copy_region_1,
	pci_a12c_mem_copy_region_2,
	pci_a12c_mem_copy_region_4,
	pci_a12c_mem_copy_region_8,
};

bus_space_tag_t
a12c_bus_mem_init(void *v)
{
        bus_space_tag_t t;

	t = &pci_a12c_mem_space;
	t->abs_cookie = v;
	return (t);
}

int
pci_a12c_mem_map(void *v, bus_addr_t memaddr, bus_size_t memsize, int flags, bus_space_handle_t *memhp, int acct)
{
	if(flags & BUS_SPACE_MAP_LINEAR)
		printf("warning, linear a12 pci map requested\n");
	if(memaddr >= 1L<<28 || memaddr<0)
		return -1;
	*memhp = memaddr;
	return 0;
}

void
pci_a12c_mem_unmap(void *v, bus_space_handle_t memh, bus_size_t memsize, int acct)
{
}

int
pci_a12c_mem_subregion(void *v, bus_space_handle_t memh, bus_size_t offset, bus_size_t size, bus_space_handle_t *nmemh)
{
	*nmemh = memh + offset;
	return 0;
}

int
pci_a12c_mem_alloc(v, rstart, rend, size, align, boundary, flags,
    addrp, bshp)
	void *v;
	bus_addr_t rstart, rend, *addrp;
	bus_size_t size, align, boundary;
	int flags;
	bus_space_handle_t *bshp;
{
	return -1;
}

void
pci_a12c_mem_free(void *v, bus_space_handle_t bsh, bus_size_t size)
{
}

void *
pci_a12c_mem_vaddr(void *v, bus_space_handle_t bsh)
{
	/* not supported (could panic() if pci_a12c_mem_map() caught it) */
	return (0);
}

void
pci_a12c_mem_barrier(void *v, bus_space_handle_t h, bus_size_t o, bus_size_t l, int f)
{
	/* optimize for wmb-only case but fall back to the more general mb */

	if (f == BUS_SPACE_BARRIER_WRITE)
		alpha_wmb();
	else	alpha_mb();
}
/*
 * Don't worry about calling small subroutines on the alpha. In the
 * time it takes to do a PCI bus target transfer, the 21164-400 can execute
 * 160 cycles, and up to 320 integer instructions.
 */
static void *
setup_target_transfer(bus_space_handle_t memh, bus_size_t off)
{
	register u_int64_t addr,t;

	alpha_mb();
	addr = memh + off;
	t = REGVAL(A12_OMR) & ~A12_OMR_PCIAddr2;
	if (addr & 4)
		t |= A12_OMR_PCIAddr2;
	REGVAL(A12_OMR) = t;
	alpha_mb();
	return (void *)ALPHA_PHYS_TO_K0SEG(A12_PCITarget | (addr & ~4L));
}

#define	TARGET_EA(t,memh,off)  ((t *)setup_target_transfer((memh),(off)))

#define	TARGET_READ(n,t) 				\
							\
t							\
__CONCAT(pci_a12c_mem_read_,n)(void *v,			\
			bus_space_handle_t memh,	\
			bus_size_t off)			\
{							\
		return *TARGET_EA(t,memh,off);		\
}

TARGET_READ(1, u_int8_t)
TARGET_READ(2, u_int16_t)
TARGET_READ(4, u_int32_t)
TARGET_READ(8, u_int64_t)

#define pci_a12c_mem_read_multi_N(BYTES,TYPE)				\
void									\
__CONCAT(pci_a12c_mem_read_multi_,BYTES)(v, h, o, a, c)			\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	TYPE *a;							\
{									\
									\
	while (c-- > 0) {						\
		alpha_mb();						\
		*a++ = __CONCAT(pci_a12c_mem_read_,BYTES)(v, h, o);	\
	}								\
}

pci_a12c_mem_read_multi_N(1,u_int8_t)
pci_a12c_mem_read_multi_N(2,u_int16_t)
pci_a12c_mem_read_multi_N(4,u_int32_t)
pci_a12c_mem_read_multi_N(8,u_int64_t)
/* 

 * In this case, we _really_ don't want all those barriers that the
 * bus_space_read's once did, and that we have carried over into
 * the A12 version. Perhaps we need a gratuitous barrier-on mode.
 * The only reason to have the bus_space_r/w functions do barriers is
 * to make up for call sites that incorrectly omit the bus_space_barrier
 * calls.
 */

#define pci_a12c_mem_read_region_N(BYTES,TYPE)				\
void									\
__CONCAT(pci_a12c_mem_read_region_,BYTES)(v, h, o, a, c)			\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	TYPE *a;							\
{									\
									\
	while (c-- > 0) {						\
		*a++ = __CONCAT(pci_a12c_mem_read_,BYTES)(v, h, o);	\
		o += sizeof *a;						\
	}								\
}
pci_a12c_mem_read_region_N(1,u_int8_t)
pci_a12c_mem_read_region_N(2,u_int16_t)
pci_a12c_mem_read_region_N(4,u_int32_t)
pci_a12c_mem_read_region_N(8,u_int64_t)

#define	TARGET_WRITE(n,t)						\
void									\
__CONCAT(pci_a12c_mem_write_,n)(v, memh, off, val)			\
	void *v;							\
	bus_space_handle_t memh;					\
	bus_size_t off;							\
	t val;								\
{									\
	alpha_wmb();							\
	*TARGET_EA(t,memh,off) = val;					\
	alpha_wmb();							\
}

TARGET_WRITE(1,u_int8_t)
TARGET_WRITE(2,u_int16_t)
TARGET_WRITE(4,u_int32_t)
TARGET_WRITE(8,u_int64_t)

#define pci_a12c_mem_write_multi_N(BYTES,TYPE)				\
void									\
__CONCAT(pci_a12c_mem_write_multi_,BYTES)(v, h, o, a, c)		\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	const TYPE *a;							\
{									\
									\
	while (c-- > 0) {						\
		__CONCAT(pci_a12c_mem_write_,BYTES)(v, h, o, *a++);	\
		alpha_wmb();						\
	}								\
}

pci_a12c_mem_write_multi_N(1,u_int8_t)
pci_a12c_mem_write_multi_N(2,u_int16_t)
pci_a12c_mem_write_multi_N(4,u_int32_t)
pci_a12c_mem_write_multi_N(8,u_int64_t)

#define pci_a12c_mem_write_region_N(BYTES,TYPE)				\
void									\
__CONCAT(pci_a12c_mem_write_region_,BYTES)(v, h, o, a, c)		\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	const TYPE *a;							\
{									\
									\
	while (c-- > 0) {						\
		__CONCAT(pci_a12c_mem_write_,BYTES)(v, h, o, *a++);	\
		o += sizeof *a;						\
	}								\
}

pci_a12c_mem_write_region_N(1,u_int8_t)
pci_a12c_mem_write_region_N(2,u_int16_t)
pci_a12c_mem_write_region_N(4,u_int32_t)
pci_a12c_mem_write_region_N(8,u_int64_t)

#define pci_a12c_mem_set_multi_N(BYTES,TYPE)				\
void									\
__CONCAT(pci_a12c_mem_set_multi_,BYTES)(v, h, o, val, c)			\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	TYPE val;							\
{									\
									\
	while (c-- > 0) {						\
		__CONCAT(pci_a12c_mem_write_,BYTES)(v, h, o, val);	\
		alpha_wmb();						\
	}								\
}

pci_a12c_mem_set_multi_N(1,u_int8_t)
pci_a12c_mem_set_multi_N(2,u_int16_t)
pci_a12c_mem_set_multi_N(4,u_int32_t)
pci_a12c_mem_set_multi_N(8,u_int64_t)

#define pci_a12c_mem_set_region_N(BYTES,TYPE)				\
void									\
__CONCAT(pci_a12c_mem_set_region_,BYTES)(v, h, o, val, c)			\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	TYPE val;							\
{									\
									\
	while (c-- > 0) {						\
		__CONCAT(pci_a12c_mem_write_,BYTES)(v, h, o, val);	\
		o += sizeof val;					\
	}								\
}

pci_a12c_mem_set_region_N(1,u_int8_t)
pci_a12c_mem_set_region_N(2,u_int16_t)
pci_a12c_mem_set_region_N(4,u_int32_t)
pci_a12c_mem_set_region_N(8,u_int64_t)

#define	pci_a12c_mem_copy_region_N(BYTES)				\
void									\
__CONCAT(pci_a12c_mem_copy_region_,BYTES)(v, h1, o1, h2, o2, c)		\
	void *v;							\
	bus_space_handle_t h1, h2;					\
	bus_size_t o1, o2, c;						\
{									\
	bus_size_t o;							\
									\
	if ((h1 >> 63) != 0 && (h2 >> 63) != 0) {			\
		memmove((void *)(h2 + o2), (void *)(h1 + o1), c * BYTES); \
		return;							\
	}								\
									\
	if (h1 + o1 >= h2 + o2)						\
		/* src after dest: copy forward */			\
		for (o = 0; c > 0; c--, o += BYTES)			\
			__CONCAT(pci_a12c_mem_write_,BYTES)(v, h2, o2 + o,	\
			    __CONCAT(pci_a12c_mem_read_,BYTES)(v, h1, o1 + o)); \
	else								\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * BYTES; c > 0; c--, o -= BYTES)	\
			__CONCAT(pci_a12c_mem_write_,BYTES)(v, h2, o2 + o,	\
			    __CONCAT(pci_a12c_mem_read_,BYTES)(v, h1, o1 + o)); \
}
pci_a12c_mem_copy_region_N(1)
pci_a12c_mem_copy_region_N(2)
pci_a12c_mem_copy_region_N(4)
pci_a12c_mem_copy_region_N(8)
