/*	$NetBSD: empb_bsm.c,v 1.3 2012/06/04 12:56:48 rkujawa Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * Special bus space methods handling PCI memory window. Used only by empb.
 *
 * XXX: Handle ops on window boundary! Currently these are broken!
 */

#include <sys/bus.h>
#include <sys/null.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <amiga/pci/empbreg.h>
#include <amiga/pci/empbvar.h>
#include <amiga/pci/emmemvar.h>

static bool empb_bsm_init(void);

/*
 * The bus_space functions are prototyped below. Due to macro-ridden 
 * maddness of amiga port bus_space implementation, these prototypes look
 * somewhat different than what you can read in bus_space(9) man page.
 */

static int 	empb_bsm(bus_space_tag_t space, bus_addr_t address, 
		    bus_size_t size, int flags, bus_space_handle_t *handlep);
static int 	empb_bsms(bus_space_handle_t handle, bus_size_t offset, 
		    bus_size_t size, bus_space_handle_t *nhandlep);
static void	empb_bsu(bus_space_handle_t handle,
		    bus_size_t size);

static bsr(empb_bsr1, u_int8_t);
static bsw(empb_bsw1, u_int8_t);
static bsrm(empb_bsrm1, u_int8_t);
static bswm(empb_bswm1, u_int8_t);
static bsrm(empb_bsrr1, u_int8_t);
static bswm(empb_bswr1, u_int8_t);
static bssr(empb_bssr1, u_int8_t);
static bscr(empb_bscr1, u_int8_t);

static bsr(empb_bsr2_swap, u_int16_t);
static bsw(empb_bsw2_swap, u_int16_t);
static bsr(empb_bsr2, u_int16_t);
static bsw(empb_bsw2, u_int16_t);
static bsrm(empb_bsrm2_swap, u_int16_t);
static bswm(empb_bswm2_swap, u_int16_t);
static bsrm(empb_bsrm2, u_int16_t);
static bswm(empb_bswm2, u_int16_t);
static bsrm(empb_bsrr2_swap, u_int16_t);
static bswm(empb_bswr2_swap, u_int16_t);
static bsrm(empb_bsrr2, u_int16_t);
static bswm(empb_bswr2, u_int16_t);
static bssr(empb_bssr2_swap, u_int16_t);
static bscr(empb_bscr2, u_int16_t);

/*static bsr(empb_bsr4_swap, u_int32_t);
static bsw(empb_bsw4_swap, u_int32_t);
static bsr(empb_bsr4, u_int32_t);
static bsw(empb_bsw4, u_int32_t);
static bsrm(empb_bsrm4_swap, u_int32_t);
static bswm(empb_bswm4_swap, u_int32_t);
static bsrm(empb_bsrm4, u_int32_t);
static bswm(empb_bswm4, u_int32_t);
static bsrm(empb_bsrr4_swap, u_int32_t);
static bswm(empb_bswr4_swap, u_int32_t);
static bsrm(empb_bsrr4, u_int32_t);
static bswm(empb_bswr4, u_int32_t);
static bssr(empb_bssr4_swap, u_int32_t);
static bscr(empb_bscr4, u_int32_t);*/
/* 
 * Hold pointer to bridge driver here. We need to access it to switch 
 * window position. Perhaps it should be stored in bus_space_tag instead...
 */
static struct empb_softc *empb_sc = NULL;

static bool 
empb_bsm_init(void) 
{
	device_t dev;

	/* We can't have more than one Mediator anyway. */
	if (!(dev = device_find_by_xname("empb0"))) {
		aprint_error("empb: can't find bridge device\n");
		return false;
	}

	if (!(empb_sc = device_private(dev))) {
		aprint_error_dev(dev, "can't obtain bridge softc\n");
		return false;
	}

	if (empb_sc->pci_mem_win_size == 0) {
		aprint_error_dev(dev, "no PCI memory window found\n");
		return false;
	}

	return true;
}

/* === common bus space methods === */

static int
empb_bsm(bus_space_tag_t space, bus_addr_t address, bus_size_t size,
    int flags, bus_space_handle_t *handlep) 
{

	/* Check for bridge driver softc. */
	if (empb_sc==NULL)
		if(empb_bsm_init() == false)
			return -1; 
		
	/* Fail miserably if the driver wants linear space. */	
	if (flags & BUS_SPACE_MAP_LINEAR)
	{
		aprint_error("empb: linear space mapping not possible\n");
		return -1;
	}

	/* 
	 * Just store the desired PCI bus address as handlep. Don't make things 
	 * more complicated than they need to be.
	 */
	*handlep = address;

	return 0;
}

static int
empb_bsms(bus_space_handle_t handle,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nhandlep)
{
	*nhandlep = handle + offset;
	return 0;
}

static void
empb_bsu(bus_space_handle_t handle, bus_size_t size) 
{
	return;
}

/* === 8-bit methods === */

static uint8_t
empb_bsr1(bus_space_handle_t handle, bus_size_t offset)
{
	uint8_t *p;
	uint8_t x;
	bus_addr_t wp; /* window position */

	wp = empb_switch_window(empb_sc, handle);	

	/* window address + (PCI mem address - window position) */
	p = (uint8_t*)((empb_sc->pci_mem_win_t->base) + (handle - wp));
	x = *p;

	return x;
}

void
empb_bsw1(bus_space_handle_t handle, bus_size_t offset, unsigned value) 
{
	uint8_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (uint8_t*)((empb_sc->pci_mem_win_t->base) + (handle - wp));
	*p = value;
}

void
empb_bsrm1(bus_space_handle_t handle, bus_size_t offset, u_int8_t *pointer,
    bus_size_t count)
{
	volatile uint8_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
  	p = (volatile u_int8_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

	while (count > 0) {
		*pointer++ = *p;
		amiga_bus_reorder_protect();
 		--count;
	}
}

void
empb_bswm1(bus_space_handle_t handle, bus_size_t offset, 
    const u_int8_t *pointer, bus_size_t count)
{
	volatile uint8_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int8_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

	while (count > 0) {
		*p = *pointer++;
 		amiga_bus_reorder_protect();
		--count;
	}
}

void
empb_bsrr1(bus_space_handle_t handle, bus_size_t offset, u_int8_t *pointer,
    bus_size_t count)
{
	volatile uint8_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int8_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

	while (count > 0) {
		*pointer++ = *p;
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

void
empb_bswr1(bus_space_handle_t handle, bus_size_t offset, 
    const u_int8_t *pointer, bus_size_t count)
{
	volatile uint8_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int8_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

 	while (count > 0) {
		*p = *pointer++;
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

void
empb_bssr1(bus_space_handle_t handle, bus_size_t offset, unsigned value,
    bus_size_t count)
{
	volatile uint8_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int8_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

 	while (count > 0) {
 		*p = value;
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

/* XXX: this is broken, rewrite */
void
empb_bscr1(bus_space_handle_t handlefrom, bus_size_t from, 
    bus_space_handle_t handleto, bus_size_t to, bus_size_t count)
{
	volatile uint8_t *p, *q;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handlefrom);

	p = (volatile u_int8_t *)
	    ( ((empb_sc->pci_mem_win_t->base)+(handlefrom - wp)) + from );
	q = (volatile u_int8_t *)
	    ( ((empb_sc->pci_mem_win_t->base)+(handleto - wp)) + to );

	while (count > 0) {
		*q = *p;
		amiga_bus_reorder_protect();
		p ++; q++;
		--count;
	}
}

/* === 16-bit methods === */

static uint16_t
empb_bsr2(bus_space_handle_t handle, bus_size_t offset)
{
	uint16_t *p;
	uint16_t x;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);	

	p = (uint16_t*)((empb_sc->pci_mem_win_t->base) + (handle - wp));
	x = *p;

	return x;
}

static uint16_t
empb_bsr2_swap(bus_space_handle_t handle, bus_size_t offset)
{
	uint16_t *p;
	uint16_t x;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);	

	p = (uint16_t*)((empb_sc->pci_mem_win_t->base) + (handle - wp));
	x = *p;

	return bswap16(x);
}


void
empb_bsw2(bus_space_handle_t handle, bus_size_t offset, unsigned value) 
{
	uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (uint16_t*)((empb_sc->pci_mem_win_t->base) + (handle - wp));
	*p = value;
}

void
empb_bsw2_swap(bus_space_handle_t handle, bus_size_t offset, unsigned value) 
{
	uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (uint16_t*)((empb_sc->pci_mem_win_t->base) + (handle - wp));
	*p = bswap16(value);
}

void
empb_bsrm2(bus_space_handle_t handle, bus_size_t offset, u_int16_t *pointer,
    bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
  	p = (volatile u_int16_t *) ((empb_sc->pci_mem_win_t->base) + 
	    (handle - wp));

	while (count > 0) {
		*pointer++ = *p;
		amiga_bus_reorder_protect();
 		--count;
	}
}

void
empb_bsrm2_swap(bus_space_handle_t handle, bus_size_t offset, 
    u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
  	p = (volatile u_int16_t *) ((empb_sc->pci_mem_win_t->base) + 
	    (handle - wp));

	while (count > 0) {
		*pointer++ = bswap16(*p);
		amiga_bus_reorder_protect();
 		--count;
	}
}

void
empb_bswm2(bus_space_handle_t handle, bus_size_t offset, 
    const u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int16_t *) ((empb_sc->pci_mem_win_t->base) + 
	    (handle - wp));

	while (count > 0) {
		*p = *pointer++;
 		amiga_bus_reorder_protect();
		--count;
	}
}

void
empb_bswm2_swap(bus_space_handle_t handle, bus_size_t offset, 
    const u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int16_t *) ((empb_sc->pci_mem_win_t->base) + 
	    (handle - wp));

	while (count > 0) {
		*p = bswap16(*pointer++);
 		amiga_bus_reorder_protect();
		--count;
	}
}

void
empb_bsrr2(bus_space_handle_t handle, bus_size_t offset, u_int16_t *pointer,
    bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile uint16_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

	while (count > 0) {
		*pointer++ = *p;
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

void
empb_bsrr2_swap(bus_space_handle_t handle, bus_size_t offset, 
    u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile uint16_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

	while (count > 0) {
		*pointer++ = bswap16(*p);
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

void
empb_bssr2_swap(bus_space_handle_t handle, bus_size_t offset, unsigned value,
    bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile uint16_t *) ((empb_sc->pci_mem_win_t->base) + 
	    (handle - wp));

 	while (count > 0) {
 		*p = bswap16(value);
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

void
empb_bswr2(bus_space_handle_t handle, bus_size_t offset, 
    const u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int16_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

 	while (count > 0) {
		*p = *pointer++;
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

void
empb_bswr2_swap(bus_space_handle_t handle, bus_size_t offset, 
    const u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int16_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

 	while (count > 0) {
		*p = bswap16(*pointer++);
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

/* XXX: this is broken, rewrite, XXX 2: should we swap here? */
void
empb_bscr2(bus_space_handle_t handlefrom, bus_size_t from, 
    bus_space_handle_t handleto, bus_size_t to, bus_size_t count)
{
	volatile uint16_t *p, *q;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handlefrom);

	p = (volatile uint16_t *)
	    ( ((empb_sc->pci_mem_win_t->base)+(handlefrom - wp)) + from );
	q = (volatile uint16_t *)
	    ( ((empb_sc->pci_mem_win_t->base)+(handleto - wp)) + to );

	while (count > 0) {
		*q = *p;
		amiga_bus_reorder_protect();
		p++; q++;
		--count;
	}
}

/* === 32-bit methods === */
/*
static uint32_t
empb_bsr4(bus_space_handle_t handle, bus_size_t offset)
{
	uint32_t *p;
	uint32_t x;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);	

	p = (uint32_t*)((empb_sc->pci_mem_win_t->base) + (handle - wp));
	x = *p;

	return x;
}

static uint32_t
empb_bsr4_swap(bus_space_handle_t handle, bus_size_t offset)
{
	uint32_t *p;
	uint32_t x;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);	

	p = (uint32_t*)((empb_sc->pci_mem_win_t->base) + (handle - wp));
	x = *p;

	return bswap32(x);
}


void
empb_bsw4(bus_space_handle_t handle, bus_size_t offset, unsigned value) 
{
	uint32_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (uint16_t*)((empb_sc->pci_mem_win_t->base) + (handle - wp));
	*p = value;
}

void
empb_bsw2_swap(bus_space_handle_t handle, bus_size_t offset, unsigned value) 
{
	uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (uint16_t*)((empb_sc->pci_mem_win_t->base) + (handle - wp));
	*p = bswap16(value);
}

void
empb_bsrm2(bus_space_handle_t handle, bus_size_t offset, u_int16_t *pointer,
    bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
  	p = (volatile u_int16_t *) ((empb_sc->pci_mem_win_t->base) + 
	    (handle - wp));

	while (count > 0) {
		*pointer++ = *p;
		amiga_bus_reorder_protect();
 		--count;
	}
}

void
empb_bsrm2_swap(bus_space_handle_t handle, bus_size_t offset, 
    u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
  	p = (volatile u_int16_t *) ((empb_sc->pci_mem_win_t->base) + 
	    (handle - wp));

	while (count > 0) {
		*pointer++ = bswap16(*p);
		amiga_bus_reorder_protect();
 		--count;
	}
}

void
empb_bswm2(bus_space_handle_t handle, bus_size_t offset, 
    const u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int16_t *) ((empb_sc->pci_mem_win_t->base) + 
	    (handle - wp));

	while (count > 0) {
		*p = *pointer++;
 		amiga_bus_reorder_protect();
		--count;
	}
}

void
empb_bswm2_swap(bus_space_handle_t handle, bus_size_t offset, 
    const u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int16_t *) ((empb_sc->pci_mem_win_t->base) + 
	    (handle - wp));

	while (count > 0) {
		*p = bswap16(*pointer++);
 		amiga_bus_reorder_protect();
		--count;
	}
}

void
empb_bsrr2(bus_space_handle_t handle, bus_size_t offset, u_int16_t *pointer,
    bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile uint16_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

	while (count > 0) {
		*pointer++ = *p;
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

void
empb_bsrr2_swap(bus_space_handle_t handle, bus_size_t offset, 
    u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile uint16_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

	while (count > 0) {
		*pointer++ = bswap16(*p);
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

void
empb_bssr2_swap(bus_space_handle_t handle, bus_size_t offset, unsigned value,
    bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile uint16_t *) ((empb_sc->pci_mem_win_t->base) + 
	    (handle - wp));

 	while (count > 0) {
 		*p = bswap16(value);
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

void
empb_bswr2(bus_space_handle_t handle, bus_size_t offset, 
    const u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int16_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

 	while (count > 0) {
		*p = *pointer++;
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

void
empb_bswr2_swap(bus_space_handle_t handle, bus_size_t offset, 
    const u_int16_t *pointer, bus_size_t count)
{
	volatile uint16_t *p;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handle);
	p = (volatile u_int16_t *)((empb_sc->pci_mem_win_t->base)+(handle - wp));

 	while (count > 0) {
		*p = bswap16(*pointer++);
		amiga_bus_reorder_protect();
		p++;
		--count;
	}
}

// XXX: this is broken, rewrite, XXX 2: should we swap here? 
void
empb_bscr2(bus_space_handle_t handlefrom, bus_size_t from, 
    bus_space_handle_t handleto, bus_size_t to, bus_size_t count)
{
	volatile uint16_t *p, *q;
	bus_addr_t wp; 

	wp = empb_switch_window(empb_sc, handlefrom);

	p = (volatile uint16_t *)
	    ( ((empb_sc->pci_mem_win_t->base)+(handlefrom - wp)) + from );
	q = (volatile uint16_t *)
	    ( ((empb_sc->pci_mem_win_t->base)+(handleto - wp)) + to );

	while (count > 0) {
		*q = *p;
		amiga_bus_reorder_protect();
		p++; q++;
		--count;
	}
} */
/* === end of implementation === */

const struct amiga_bus_space_methods empb_bus_swap = {

	.bsm =		empb_bsm,
	.bsms =		empb_bsms,
	.bsu =		empb_bsu,
 	.bsa =		NULL,
	.bsf =		NULL,

	/* 8-bit methods */
	.bsr1 =		empb_bsr1,
	.bsw1 =		empb_bsw1,
	.bsrm1 =	empb_bsrm1,
	.bswm1 =	empb_bswm1,
	.bsrr1 =	empb_bsrr1,
	.bswr1 =	empb_bswr1,
	.bssr1 =	empb_bssr1,
	.bscr1 =	empb_bscr1,

	/* 16-bit methods */
	.bsr2 =		empb_bsr2_swap,	
	.bsw2 =		empb_bsw2_swap,	
	.bsrs2 =	empb_bsr2,
	.bsws2 =	empb_bsw2,
	.bsrm2 =	empb_bsrm2_swap,
	.bswm2 =	empb_bswm2_swap,
	.bsrms2 =	empb_bsrm2,
	.bswms2 =	empb_bswm2,
	.bsrr2 =	empb_bsrr2_swap,
	.bswr2 =	empb_bswr2_swap,
	.bsrrs2 =	empb_bsrr2,
	.bswrs2 =	empb_bswr2,
	.bssr2 =	empb_bssr2_swap,
	.bscr2 =	empb_bscr2 /*

	//32-bit methods 
	.bsr4 =		empb_bsr4_swap,
	.bsw4 =		empb_bsw4_swap,
	.bsrs4 =	empb_bsr4,
	.bsws4 =	empb_bsw4,
	.bsrm4 =	empb_bsrm4_swap,
	.bswm4 =	empb_bswm4_swap,
	.bsrms4 =	empb_bsrm4,
	.bswms4 =	empb_bswm4,
	.bsrr4 =	empb_bsrr4_swap, 
	.bswr4 =	empb_bswr4_swap,
	.bsrrs4 =	empb_bsrr4,
	.bswrs4 =	empb_bswr4,
	.bssr4 = 	empb_bssr4_swap,
	.bscr4 =	empb_bscr4_swap  */
};


