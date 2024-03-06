/* $NetBSD: gbus_io.c,v 1.1 2024/03/06 05:33:09 thorpej Exp $ */

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

/*
 * Bus space implementation for the Gbus: a general 8-bit bus found
 * on Laser / TurboLaser CPU modules.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: gbus_io.c,v 1.1 2024/03/06 05:33:09 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/vmem.h>

#include <sys/bus.h>

#include <alpha/gbus/gbusvar.h>

/*
 * While physically there is one Gbus per CPU module on the system,
 * logically there is only one (the one on the CPU module that holds
 * the primary CPU), and this describes it.
 */
static struct gbus_config {
	paddr_t		gc_sysbase;	/* system base address of Gbus */
	struct alpha_bus_space gc_st;	/* the Gbus's space tag */
} gbus_config;

/* Adjacent bytes on the Gbus are spaced 64 bytes apart. */
#define	GBUS_ADDR_SHIFT		6

static int
gbus_io_map(
	void *v,
	bus_addr_t addr,
	bus_size_t size			__unused,
	int flags			__unused,
	bus_space_handle_t *iohp,
	int acct			__unused)
{
	struct gbus_config * const gc = v;

	*iohp = ALPHA_PHYS_TO_K0SEG(gc->gc_sysbase + addr);

	return 0;
}

static void
gbus_io_unmap(
	void *v				__unused,
	bus_space_handle_t ioh		__unused,
	bus_size_t size			__unused,
	int acct			__unused)
{
	/* No work to do. */
}

static void
gbus_io_barrier(
	void *v				__unused,
	bus_space_handle_t ioh		__unused,
	bus_size_t off			__unused,
	bus_size_t len			__unused,
	int flags			__unused)
{
	if (flags & BUS_SPACE_BARRIER_READ) {
		alpha_mb();
	} else if (flags & BUS_SPACE_BARRIER_WRITE) {
		alpha_wmb();
	}
}

__CTASSERT(_BYTE_ORDER == _LITTLE_ENDIAN);

/*
 * A 32-bit pointer is used here because the bus transaction is going to
 * be 32-bit regardless, and this avoids having the compiler generate
 * extbl / insbl / mskbl unnecessarily (or r/m/w in the write case).
 */
#define	GBUS_ADDR(ioh, off)						\
	(uint32_t *)((ioh) + ((off) << GBUS_ADDR_SHIFT))

static uint8_t
gbus_io_read_1(
	void *v				__unused,
	bus_space_handle_t ioh,
	bus_size_t off)
{
	uint32_t *addr = GBUS_ADDR(ioh, off);

	alpha_mb();
	return (uint8_t)atomic_load_relaxed(addr);
}

static void
gbus_io_write_1(
	void *v				__unused,
	bus_space_handle_t ioh,
	bus_size_t off,
	uint8_t val)
{
	uint32_t *addr = GBUS_ADDR(ioh, off);

	atomic_store_relaxed(addr, (uint32_t)val);
	alpha_mb();
}

bus_space_tag_t
gbus_io_init(paddr_t sysbase)
{
	struct gbus_config * const gc = &gbus_config;
	bus_space_tag_t t = &gc->gc_st;

	if (t->abs_cookie == gc) {
		/* Already initialized. */
		KASSERT(gc->gc_sysbase == sysbase);
		return t;
	}

	gc->gc_sysbase = sysbase;
	memset(t, 0, sizeof(*t));

	/*
	 * Initialize the bus space tag.  We just support the bare
	 * minimum operations required by Gbus devices.
	 */

	/* cookie */
	t->abs_cookie =		gc;

	/* mapping/unmapping */
	t->abs_map =		gbus_io_map;
	t->abs_unmap =		gbus_io_unmap;

	/* barrier */
	t->abs_barrier =	gbus_io_barrier;

	/* read (single) */
	t->abs_r_1 =		gbus_io_read_1;

	/* write (single) */
	t->abs_w_1 =		gbus_io_write_1;

	return t;
}
