/* $NetBSD: jensenio_bus_intio.c,v 1.4.2.1 2012/04/17 00:05:56 yamt Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: jensenio_bus_intio.c,v 1.4.2.1 2012/04/17 00:05:56 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <sys/bus.h>

#include <dev/eisa/eisavar.h>

#include <dev/isa/isavar.h>

#include <alpha/jensenio/jensenioreg.h>
#include <alpha/jensenio/jenseniovar.h>

/* mapping/unmapping */
int		jensenio_intio_map(void *, bus_addr_t, bus_size_t, int,
		    bus_space_handle_t *, int);
void		jensenio_intio_unmap(void *, bus_space_handle_t,
		    bus_size_t, int);
int		jensenio_intio_subregion(void *, bus_space_handle_t,
		    bus_size_t, bus_size_t, bus_space_handle_t *);

/* allocation/deallocation */
	/* Not supported for Internal space */

/* barrier */
static inline void	jensenio_intio_barrier(void *, bus_space_handle_t,
		    bus_size_t, bus_size_t, int);

/* read (single) */
static inline uint8_t	jensenio_intio_read_1(void *, bus_space_handle_t, bus_size_t);

/* read multiple */
void		jensenio_intio_read_multi_1(void *, bus_space_handle_t,
		    bus_size_t, uint8_t *, bus_size_t);

/* read region */
	/* Not supported for Internal space */

/* write (single) */
static inline void	jensenio_intio_write_1(void *, bus_space_handle_t,
		    bus_size_t, uint8_t);

/* write multiple */
void		jensenio_intio_write_multi_1(void *, bus_space_handle_t,
		    bus_size_t, const uint8_t *, bus_size_t);

/* write region */
	/* Not supported for Internal space */

/* set multiple */
void		jensenio_intio_set_multi_1(void *, bus_space_handle_t,
		    bus_size_t, uint8_t, bus_size_t);

/* set region */
	/* Not supported for Internal space */

/* copy */
	/* Not supported for Internal space */

void
jensenio_bus_intio_init(bus_space_tag_t t, void *v)
{

	/*
	 * Initialize the bus space tag.
	 */

	memset(t, 0, sizeof(*t));

	/* cookie */
	t->abs_cookie =		v;

	/* mapping/unmapping */
	t->abs_map =		jensenio_intio_map;
	t->abs_unmap =		jensenio_intio_unmap;
	t->abs_subregion =	jensenio_intio_subregion;

	/* barrier */
	t->abs_barrier =	jensenio_intio_barrier;

	/* read (single) */
	t->abs_r_1 =		jensenio_intio_read_1;

	/* read multiple */
	t->abs_rm_1 =		jensenio_intio_read_multi_1;

	/* write (single) */
	t->abs_w_1 =		jensenio_intio_write_1;

	/* write multiple */
	t->abs_wm_1 =		jensenio_intio_write_multi_1;

	/* set multiple */
	t->abs_sm_1 =		jensenio_intio_set_multi_1;

	/*
	 * Extent map is already set up.
	 */
}

int
jensenio_intio_map(void *v, bus_addr_t ioaddr, bus_size_t iosize, int flags,
    bus_space_handle_t *iohp, int acct)
{
	struct jensenio_config *jcp = v;
	int linear = flags & BUS_SPACE_MAP_LINEAR;
	int error;

	/*
	 * Can't map i/o space linearly.
	 */
	if (linear)
		return (EOPNOTSUPP);

	if (acct) {
#ifdef EXTENT_DEBUG
		printf("intio: allocating 0x%lx to 0x%lx\n", ioaddr,
		    ioaddr + iosize - 1);
#endif
		error = extent_alloc_region(jcp->jc_io_ex, ioaddr, iosize,
		    EX_NOWAIT | (jcp->jc_mallocsafe ? EX_MALLOCOK : 0));
		if (error) {
#ifdef EXTENT_DEBUG
			printf("intio: allocation failed (%d)\n", error);
			extent_print(jcp->jc_io_ex);
#endif
			return (error);
		}
	}

	*iohp = ALPHA_PHYS_TO_K0SEG((ioaddr << 9) + JENSEN_VL82C106);
	return (0);
}

void
jensenio_intio_unmap(void *v, bus_space_handle_t ioh, bus_size_t iosize,
    int acct)
{
	struct jensenio_config *jcp = v;
	bus_addr_t ioaddr;
	int error;

	if (acct == 0)
		return;

#ifdef EXTENT_DEBUG
	printf("intio: freeing handle 0x%lx for 0x%lx\n", ioh, iosize);
#endif

	ioh = ALPHA_K0SEG_TO_PHYS(ioh);

	ioaddr = (ioh - JENSEN_VL82C106) >> 9;

#ifdef EXTENT_DEBUG
	printf("intio: freeing 0x%lx to 0x%lx\n", ioaddr, ioaddr + iosize - 1);
#endif
	error = extent_free(jcp->jc_io_ex, ioaddr, iosize,
	    EX_NOWAIT | (jcp->jc_mallocsafe ? EX_MALLOCOK : 0));
	if (error) {
		printf("WARNING: could not unmap 0x%lx-0x%lx (error %d)\n",
		    ioaddr, ioaddr + iosize - 1, error);
#ifdef EXTENT_DEBUG
		extent_print(jcp->jc_io_ex);
#endif
	}
}

int
jensenio_intio_subregion(void *v, bus_space_handle_t ioh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nioh)
{

	*nioh = ioh + (offset << 9);
	return (0);
}

static inline void
jensenio_intio_barrier(void *v, bus_space_handle_t h, bus_size_t o,
    bus_size_t l, int f)
{

	if ((f & BUS_SPACE_BARRIER_READ) != 0)
		alpha_mb();
	else if ((f & BUS_SPACE_BARRIER_WRITE) != 0)
		alpha_wmb();
}

static inline uint8_t
jensenio_intio_read_1(void *v, bus_space_handle_t ioh, bus_size_t off)
{
	register uint32_t *port;

	alpha_mb();

	port = (uint32_t *)(ioh + (off << 9));
	return (*port & 0xff);
}

void
jensenio_intio_read_multi_1(void *v, bus_space_handle_t h, bus_size_t o,
    uint8_t *a, bus_size_t c)
{

	while (c-- > 0) {
		jensenio_intio_barrier(v, h, o, sizeof *a,
		    BUS_SPACE_BARRIER_READ);
		*a++ = jensenio_intio_read_1(v, h, o);
	}
}

static inline void
jensenio_intio_write_1(void *v, bus_space_handle_t ioh, bus_size_t off,
    uint8_t val)
{
	register uint32_t *port;

	port = (uint32_t *)(ioh + (off << 9));
	*port = val;
	alpha_mb();
}

void
jensenio_intio_write_multi_1(void *v, bus_space_handle_t h, bus_size_t o,
    const uint8_t *a, bus_size_t c)
{

	while (c-- > 0) {
		jensenio_intio_write_1(v, h, o, *a++);
		jensenio_intio_barrier(v, h, o, sizeof *a,
		    BUS_SPACE_BARRIER_WRITE);
	}
}

void
jensenio_intio_set_multi_1(void *v, bus_space_handle_t h, bus_size_t o,
    uint8_t val, bus_size_t c)
{

	while (c-- > 0) {
		jensenio_intio_write_1(v, h, o, val);
		jensenio_intio_barrier(v, h, o, sizeof val,
		    BUS_SPACE_BARRIER_WRITE);
	}
}
