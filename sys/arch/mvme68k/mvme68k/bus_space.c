/*	$NetBSD: bus_space.c,v 1.5 2003/07/15 02:43:48 lukem Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford and Jason R. Thorpe.
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
 * Implementation of bus_space mapping for the mvme68k.
 * Derived from the hp300 bus_space implementation by Jason Thorpe.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.5 2003/07/15 02:43:48 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#define _MVME68K_BUS_DMA_PRIVATE    /* For _bus_dmamem_map/_bus_dmamem_unmap */
#define _MVME68K_BUS_SPACE_PRIVATE
#include <machine/bus.h>
#undef _MVME68K_BUS_DMA_PRIVATE
#undef _MVME68K_BUS_SPACE_PRIVATE

static	void	peek1(void *, void *);
static	void	peek2(void *, void *);
static	void	peek4(void *, void *);
static	void	poke1(void *, u_int);
static	void	poke2(void *, u_int);
static	void	poke4(void *, u_int);
static	int	do_peek(void (*)(void *, void *), void *, void *);
static	int	do_poke(void (*)(void *, u_int), void *, u_int);

/*
 * Used in locore.s/trap.c to determine if faults are being trapped.
 */
label_t *nofault;


/* ARGSUSED */
int
_bus_space_map(cookie, addr, size, flags, bushp)
	void *cookie;
	bus_addr_t addr;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bushp;
{
	bus_dma_segment_t seg;
	caddr_t va;

	if (addr >= intiobase_phys && addr < intiotop_phys) {
#ifdef DEBUG
		if ((addr + size) >= intiotop_phys)
			panic("mvme68k_bus_space_map: Invalid INTIO range!");
#endif
		/*
		 * Intio space is direct-mapped in pmap_bootstrap(); just
		 * do the translation.
		 */
		addr -= intiobase_phys;
		*bushp = (bus_space_handle_t) &(intiobase[addr]);
		return (0);
	}

	/*
	 * Otherwise, set up a VM mapping for the requested address range
	 */
	seg.ds_addr = seg._ds_cpuaddr = m68k_trunc_page(addr);
	seg.ds_len = m68k_round_page(size);

	if (_bus_dmamem_map(NULL, &seg, 1, seg.ds_len, &va,
	    flags | BUS_DMA_COHERENT))
		return EIO;

	/*
	 * The handle is really the virtual address we just mapped
	 */
	*bushp = (bus_space_handle_t) (va + m68k_page_offset(addr));

	return (0);
}

/* ARGSUSED */
void
_bus_space_unmap(cookie, bush, size)
	void *cookie;
	bus_space_handle_t bush;
	bus_size_t size;
{
	/* Nothing to do for INTIO space */
	if ((char *)bush >= intiobase && (char *)bush < intiolimit)
		return;

	bush = m68k_trunc_page(bush);
	size = m68k_round_page(size);

	_bus_dmamem_unmap(NULL, (caddr_t) bush, size);
}

/* ARGSUSED */
int
_bus_space_peek_1(cookie, bush, offset, valuep)
	void *cookie;
	bus_space_handle_t bush;
	bus_size_t offset;
	u_int8_t *valuep;
{
	u_int8_t v;

	if (valuep == NULL)
		valuep = &v;

	return (do_peek(&peek1, (void *)(bush + offset), (void *)valuep));
}

/* ARGSUSED */
int
_bus_space_peek_2(cookie, bush, offset, valuep)
	void *cookie;
	bus_space_handle_t bush;
	bus_size_t offset;
	u_int16_t *valuep;
{
	u_int16_t v;

	if (valuep == NULL)
		valuep = &v;

	return (do_peek(&peek2, (void *)(bush + offset), (void *)valuep));
}

/* ARGSUSED */
int
_bus_space_peek_4(cookie, bush, offset, valuep)
	void *cookie;
	bus_space_handle_t bush;
	bus_size_t offset;
	u_int32_t *valuep;
{
	u_int32_t v;

	if (valuep == NULL)
		valuep = &v;

	return (do_peek(&peek4, (void *)(bush + offset), (void *)valuep));
}

/* ARGSUSED */
int
_bus_space_poke_1(cookie, bush, offset, value)
	void *cookie;
	bus_space_handle_t bush;
	bus_size_t offset;
	u_int8_t value;
{
	return (do_poke(&poke1, (void *)(bush + offset), (u_int)value));
}

/* ARGSUSED */
int
_bus_space_poke_2(cookie, bush, offset, value)
	void *cookie;
	bus_space_handle_t bush;
	bus_size_t offset;
	u_int16_t value;
{
	return (do_poke(&poke2, (void *)(bush + offset), (u_int)value));
}

/* ARGSUSED */
int
_bus_space_poke_4(cookie, bush, offset, value)
	void *cookie;
	bus_space_handle_t bush;
	bus_size_t offset;
	u_int32_t value;
{
	return (do_poke(&poke4, (void *)(bush + offset), (u_int)value));
}

static void
peek1(addr, vp)
	void *addr;
	void *vp;
{
	*((u_int8_t *)vp) =  *((u_int8_t *)addr);
}

static void
peek2(addr, vp)
	void *addr;
	void *vp;
{
	*((u_int16_t *)vp) = *((u_int16_t *)addr);
}

static void
peek4(addr, vp)
	void *addr;
	void *vp;
{
	*((u_int32_t *)vp) = *((u_int32_t *)addr);
}

static void
poke1(addr, value)
	void *addr;
	u_int value;
{
	*((u_int8_t *)addr) = value;
}

static void
poke2(addr, value)
	void *addr;
	u_int value;
{
	*((u_int16_t *)addr) = value;
}

static void
poke4(addr, value)
	void *addr;
	u_int value;
{
	*((u_int32_t *)addr) = value;
}

static int
do_peek(peekfn, addr, valuep)
	void (*peekfn)(void *, void *);
	void *addr;
	void *valuep;
{
	label_t faultbuf;

	nofault = &faultbuf;
	if (setjmp(&faultbuf)) {
		nofault = NULL;
		return (1);
	}

	(*peekfn)(addr, valuep);

	nofault = NULL;
	return (0);
}

static int
do_poke(pokefn, addr, value)
	void (*pokefn)(void *, u_int);
	void *addr;
	u_int value;
{
	label_t faultbuf;

	nofault = &faultbuf;
	if (setjmp(&faultbuf)) {
		nofault = NULL;
		return (1);
	}

	(*pokefn)(addr, value);

	nofault = NULL;
	return (0);
}
