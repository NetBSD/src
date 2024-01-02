/*	$NetBSD: bus_space.c,v 1.1 2024/01/02 07:41:02 thorpej Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.1 2024/01/02 07:41:02 thorpej Exp $");

#define _VIRT68K_BUS_DMA_PRIVATE    /* For _bus_dmamem_map/_bus_dmamem_unmap */
#define _VIRT68K_BUS_SPACE_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/bus.h>

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
_bus_space_map(void *cookie, bus_addr_t addr, bus_size_t size, int flags,
    bus_space_handle_t *bushp)
{
	/*
	 * All devices on virt68k are direct-mapped by TT registers.
	 */
	if (addr < VIRT68K_IO_BASE) {
		return EINVAL;
	}

	*bushp = (bus_space_handle_t)addr;
	return 0;
}

/* ARGSUSED */
void
_bus_space_unmap(void *cookie, bus_space_handle_t bush, bus_size_t size)
{
	KASSERT((bus_addr_t)bush >= VIRT68K_IO_BASE);
}

/* ARGSUSED */
int
_bus_space_peek_1(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint8_t *valuep)
{
	uint8_t v;

	if (valuep == NULL)
		valuep = &v;

	return do_peek(&peek1, (void *)(bush + offset), (void *)valuep);
}

/* ARGSUSED */
int
_bus_space_peek_2(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint16_t *valuep)
{
	uint16_t v;

	if (valuep == NULL)
		valuep = &v;

	return do_peek(&peek2, (void *)(bush + offset), (void *)valuep);
}

/* ARGSUSED */
int
_bus_space_peek_4(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint32_t *valuep)
{
	uint32_t v;

	if (valuep == NULL)
		valuep = &v;

	return do_peek(&peek4, (void *)(bush + offset), (void *)valuep);
}

/* ARGSUSED */
int
_bus_space_poke_1(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint8_t value)
{

	return do_poke(&poke1, (void *)(bush + offset), (u_int)value);
}

/* ARGSUSED */
int
_bus_space_poke_2(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint16_t value)
{

	return do_poke(&poke2, (void *)(bush + offset), (u_int)value);
}

/* ARGSUSED */
int
_bus_space_poke_4(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint32_t value)
{

	return do_poke(&poke4, (void *)(bush + offset), (u_int)value);
}

static void
peek1(void *addr, void *vp)
{

	*((uint8_t *)vp) =  *((uint8_t *)addr);
}

static void
peek2(void *addr, void *vp)
{

	*((uint16_t *)vp) = *((uint16_t *)addr);
}

static void
peek4(void *addr, void *vp)
{

	*((uint32_t *)vp) = *((uint32_t *)addr);
}

static void
poke1(void *addr, u_int value)
{

	*((uint8_t *)addr) = value;
}

static void
poke2(void *addr, u_int value)
{

	*((uint16_t *)addr) = value;
}

static void
poke4(void *addr, u_int value)
{

	*((uint32_t *)addr) = value;
}

static int
do_peek(void (*peekfn)(void *, void *), void *addr, void *valuep)
{
	label_t faultbuf;

	nofault = &faultbuf;
	if (setjmp(&faultbuf)) {
		nofault = NULL;
		return 1;
	}

	(*peekfn)(addr, valuep);

	nofault = NULL;
	return 0;
}

static int
do_poke(void (*pokefn)(void *, u_int), void *addr, u_int value)
{
	label_t faultbuf;

	nofault = &faultbuf;
	if (setjmp(&faultbuf)) {
		nofault = NULL;
		return 1;
	}

	(*pokefn)(addr, value);

	nofault = NULL;
	return 0;
}
