/*	$NetBSD: bus_space.c,v 1.1 2002/07/05 13:32:03 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
 * Implementation of bus_space for sh5.
 * Derived from the mvme68k bus_space implementation.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <uvm/uvm.h>
#include <sh5/pmap.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/vmparam.h>
#include <machine/trap.h>

static int _bus_space_map(void *, bus_addr_t, bus_size_t, int,
		bus_space_handle_t *);
static void _bus_space_unmap(void *, bus_space_handle_t, bus_size_t);
static int _bus_space_alloc(void *, bus_addr_t, bus_addr_t, bus_size_t,
		bus_size_t, bus_size_t, int, bus_addr_t *,bus_space_handle_t *);
static void _bus_space_free(void *, bus_space_handle_t, bus_size_t);
static int _bus_space_subregion(void *, bus_space_handle_t, bus_size_t,
		bus_size_t, bus_space_handle_t *);
static paddr_t _bus_space_mmap(void *, bus_addr_t, off_t, int, int);
static int _bus_space_peek_1(void *, bus_space_handle_t, bus_size_t,
		u_int8_t *);
static int _bus_space_peek_2(void *, bus_space_handle_t, bus_size_t,
		u_int16_t *);
static int _bus_space_peek_4(void *, bus_space_handle_t, bus_size_t,
		u_int32_t *);
static int _bus_space_peek_8(void *, bus_space_handle_t, bus_size_t,
		u_int64_t *);
static int _bus_space_poke_1(void *, bus_space_handle_t, bus_size_t, u_int8_t);
static int _bus_space_poke_2(void *, bus_space_handle_t, bus_size_t, u_int16_t);
static int _bus_space_poke_4(void *, bus_space_handle_t, bus_size_t, u_int32_t);
static int _bus_space_poke_8(void *, bus_space_handle_t, bus_size_t, u_int64_t);

static void peek1(void *, void *);
static void peek2(void *, void *);
static void peek4(void *, void *);
static void peek8(void *, void *);
static int do_peek(void (*)(void *, void *), void *, void *);

union poke_u {
	u_int8_t	u8;
	u_int16_t	u16;
	u_int32_t	u32;
	u_int64_t	u64;
};
static void poke1(void *, union poke_u *);
static void poke2(void *, union poke_u *);
static void poke4(void *, union poke_u *);
static void poke8(void *, union poke_u *);
static int do_poke(void (*)(void *, union poke_u *), void *, union poke_u *);


extern u_int8_t  _bus_space_read_1(void *, bus_space_handle_t, bus_size_t);
extern u_int16_t _bus_space_read_2(void *, bus_space_handle_t, bus_size_t);
extern u_int32_t _bus_space_read_4(void *, bus_space_handle_t, bus_size_t);
extern u_int64_t _bus_space_read_8(void *, bus_space_handle_t, bus_size_t);
extern u_int8_t  _bus_space_read_stream_1(void *, bus_space_handle_t,
		    bus_size_t);
extern u_int16_t _bus_space_read_stream_2(void *, bus_space_handle_t,
		    bus_size_t);
extern u_int32_t _bus_space_read_stream_4(void *, bus_space_handle_t,
		    bus_size_t);
extern u_int64_t _bus_space_read_stream_8(void *, bus_space_handle_t,
		    bus_size_t);
extern void	 _bus_space_write_1(void *, bus_space_handle_t,
		    bus_size_t, u_int8_t);
extern void	 _bus_space_write_2(void *, bus_space_handle_t,
		    bus_size_t, u_int16_t);
extern void	 _bus_space_write_4(void *, bus_space_handle_t,
		    bus_size_t, u_int32_t);
extern void	 _bus_space_write_8(void *, bus_space_handle_t,
		    bus_size_t, u_int64_t);
extern void	 _bus_space_write_stream_1(void *, bus_space_handle_t,
		    bus_size_t, u_int8_t);
extern void	 _bus_space_write_stream_2(void *, bus_space_handle_t,
		    bus_size_t, u_int16_t);
extern void	 _bus_space_write_stream_4(void *, bus_space_handle_t,
		    bus_size_t, u_int32_t);
extern void	 _bus_space_write_stream_8(void *, bus_space_handle_t,
		    bus_size_t, u_int64_t);


const struct sh5_bus_space_tag _sh5_bus_space_tag = {
	NULL,
	_bus_space_map,
	_bus_space_unmap,
	_bus_space_alloc,
	_bus_space_free,
	_bus_space_subregion,
	_bus_space_mmap,
	_bus_space_read_1,
	_bus_space_read_2,
	_bus_space_read_4,
	_bus_space_read_8,
	_bus_space_write_1,
	_bus_space_write_2,
	_bus_space_write_4,
	_bus_space_write_8,
	_bus_space_read_stream_2,
	_bus_space_read_stream_4,
	_bus_space_read_stream_8,
	_bus_space_write_stream_2,
	_bus_space_write_stream_4,
	_bus_space_write_stream_8,
	_bus_space_peek_1,
	_bus_space_peek_2,
	_bus_space_peek_4,
	_bus_space_peek_8,
	_bus_space_poke_1,
	_bus_space_poke_2,
	_bus_space_poke_4,
	_bus_space_poke_8
};

struct bootmapping {
	int bm_valid;
	bus_addr_t bm_start;
	bus_size_t bm_size;
	vaddr_t bm_va;
};

static struct bootmapping bootmapping[_BUS_SPACE_NUM_BOOT_MAPPINGS];


/* ARGSUSED */
static int
_bus_space_map(void *cookie, bus_addr_t addr, bus_size_t size,
    int flags, bus_space_handle_t *bushp)
{
	bus_dma_segment_t seg;
	vaddr_t va;
	int i;

	for (i = 0; i < _BUS_SPACE_NUM_BOOT_MAPPINGS; i++) {
		if (bootmapping[i].bm_valid == 0)
			continue;
		if (addr < bootmapping[i].bm_start ||
		    addr >= (bootmapping[i].bm_start + bootmapping[i].bm_size))
			continue;

		if ((addr + size) >
		    (bootmapping[i].bm_start + bootmapping[i].bm_size)) {
			panic("bus_space_map: bootmap overlap: %lx:%lx",
			    addr, size);
			/*NOTREACHED*/
		}

		/*
		 * The requested region can be satisfied from
		 * a mapping set up during bootstrap.
		 */
		*bushp = (bus_space_handle_t)(bootmapping[i].bm_va +
		    (vaddr_t)(seg.ds_addr - bootmapping[i].bm_start));
		return (0);
	}

	seg.ds_addr = seg._ds_cpuaddr = sh5_trunc_page(addr);
	seg.ds_len = sh5_round_page(size);

	if (pmap_initialized) {
		caddr_t kva;
		/*
		 * Set up a new mapping in the normal way
		 */
		if (bus_dmamem_map(&_sh5_bus_dma_tag, &seg, 1, seg.ds_len, &kva,
		    flags | BUS_DMA_COHERENT))
			return (EIO);

		va = (vaddr_t)kva;
	} else {
		/*
		 * We're being called before the pmap (and presumably UVM)
		 * have been fully initialised.
		 * At this time, pmap_bootstrap() has already been called,
		 * so we are able to steal KVA to make these mappings.
		 */
		for (i = 0; i < _BUS_SPACE_NUM_BOOT_MAPPINGS; i++) {
			if (bootmapping[i].bm_valid == 0)
				break;
		}

		if (i == _BUS_SPACE_NUM_BOOT_MAPPINGS) {
			panic("_bus_space_map: out of bootstrap maps!");
			/*NOTREACHED*/
		}

		va = pmap_bootstrap_mapping((paddr_t)seg.ds_addr,
		    (u_int)seg.ds_len);
		bootmapping[i].bm_valid = 1;
		bootmapping[i].bm_start = seg.ds_addr;
		bootmapping[i].bm_size = seg.ds_len;
		bootmapping[i].bm_va = va;
	}

	/*
	 * The handle is really the virtual address we just mapped
	 */
	*bushp = (bus_space_handle_t) (va + sh5_page_offset(addr));

	return (0);
}

/* ARGSUSED */
static void
_bus_space_unmap(void *cookie, bus_space_handle_t bush, bus_size_t size)
{
	vaddr_t va;
	int i;

	va = sh5_trunc_page(bush);
	size = sh5_round_page(size);

	for (i = 0; i < _BUS_SPACE_NUM_BOOT_MAPPINGS; i++) {
		if (bootmapping[i].bm_valid == 0)
			continue;
		if (va < bootmapping[i].bm_va ||
		    va >= (bootmapping[i].bm_va + bootmapping[i].bm_size))
			continue;

		/*
		 * The mapping was setup during bootstrap. Just ignore the
		 * unmap request. The KVA can never be reclaimed, so there's
		 * no point trying to forget about it.
		 */
		return;
	}

	bus_dmamem_unmap(&_sh5_bus_dma_tag, (caddr_t) va, size);
}

/*ARGSUSED*/
static int
_bus_space_alloc(void *cookie, bus_addr_t reg_start, bus_addr_t reg_end,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary,
    int flags, bus_addr_t *addrp, bus_space_handle_t *bushp)
{

	/* Not required for the cpu bus */
	return (EIO);
}

/*ARGSUSED*/
static void
_bus_space_free(void *cookie, bus_space_handle_t bush, bus_size_t size)
{

	_bus_space_unmap(cookie, bush, size);
}

/*ARGSUSED*/
static int
_bus_space_subregion(void *cookie, bus_space_handle_t bush,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *bushp)
{

	*bushp = bush + offset;
	return (0);
}

/*ARGSUSED*/
static paddr_t
_bus_space_mmap(void *cookie, bus_addr_t addr, off_t off, int prot, int flags)
{

	return (sh5_btop(addr + off));
}

/* ARGSUSED */
static int
_bus_space_peek_1(void *cookie, bus_space_handle_t bush,
    bus_size_t offset, u_int8_t *valuep)
{
	u_int8_t v;

	if (valuep == NULL)
		valuep = &v;

	return (do_peek(&peek1, (void *)(bush + offset), (void *)valuep));
}

/* ARGSUSED */
static int
_bus_space_peek_2(void *cookie, bus_space_handle_t bush,
    bus_size_t offset, u_int16_t *valuep)
{
	u_int16_t v;

	if (valuep == NULL)
		valuep = &v;

	return (do_peek(&peek2, (void *)(bush + offset), (void *)valuep));
}

/* ARGSUSED */
static int
_bus_space_peek_4(void *cookie, bus_space_handle_t bush,
    bus_size_t offset, u_int32_t *valuep)
{
	u_int32_t v;

	if (valuep == NULL)
		valuep = &v;

	return (do_peek(&peek4, (void *)(bush + offset), (void *)valuep));
}

/* ARGSUSED */
static int
_bus_space_peek_8(void *cookie, bus_space_handle_t bush,
    bus_size_t offset, u_int64_t *valuep)
{
	u_int64_t v;

	if (valuep == NULL)
		valuep = &v;

	return (do_peek(&peek8, (void *)(bush + offset), (void *)valuep));
}

/* ARGSUSED */
static int
_bus_space_poke_1(void *cookie, bus_space_handle_t bush,
    bus_size_t offset, u_int8_t value)
{
	union poke_u pu;

	pu.u8 = value;

	return (do_poke(&poke1, (void *)(bush + offset), &pu));
}

/* ARGSUSED */
static int
_bus_space_poke_2(void *cookie, bus_space_handle_t bush,
    bus_size_t offset, u_int16_t value)
{
	union poke_u pu;

	pu.u16 = value;

	return (do_poke(&poke2, (void *)(bush + offset), &pu));
}

/* ARGSUSED */
static int
_bus_space_poke_4(void *cookie, bus_space_handle_t bush,
    bus_size_t offset, u_int32_t value)
{
	union poke_u pu;

	pu.u32 = value;

	return (do_poke(&poke4, (void *)(bush + offset), &pu));
}

/* ARGSUSED */
static int
_bus_space_poke_8(void *cookie, bus_space_handle_t bush,
    bus_size_t offset, u_int64_t value)
{
	union poke_u pu;

	pu.u64 = value;

	return (do_poke(&poke8, (void *)(bush + offset), &pu));
}

static void
peek1(void *addr, void *vp)
{

	*((u_int8_t *)vp) =  *((u_int8_t *)addr);
}

static void
peek2(void *addr, void *vp)
{

	*((u_int16_t *)vp) = *((u_int16_t *)addr);
}

static void
peek4(void *addr, void *vp)
{

	*((u_int32_t *)vp) = *((u_int32_t *)addr);
}

static void
peek8(void *addr, void *vp)
{

	*((u_int64_t *)vp) = *((u_int64_t *)addr);
}

static void
poke1(void *addr, union poke_u *pu)
{

	*((u_int8_t *)addr) = pu->u8;
}

static void
poke2(void *addr, union poke_u *pu)
{

	*((u_int16_t *)addr) = pu->u16;
}

static void
poke4(void *addr, union poke_u *pu)
{

	*((u_int32_t *)addr) = pu->u32;
}

static void
poke8(void *addr, union poke_u *pu)
{

	*((u_int64_t *)addr) = pu->u64;
}

static int
do_peek(void (*peekfn)(void *, void *), void *addr, void *valuep)
{
	label_t faultbuf;

	onfault = &faultbuf;
	if (setjmp(&faultbuf)) {
		onfault = NULL;
		return (1);
	}

	(*peekfn)(addr, valuep);

	onfault = NULL;
	return (0);
}

static int
do_poke(void (*pokefn)(void *, union poke_u *), void *addr, union poke_u *pu)
{
	label_t faultbuf;

	onfault = &faultbuf;
	if (setjmp(&faultbuf)) {
		onfault = NULL;
		return (1);
	}

	(*pokefn)(addr, pu);

	onfault = NULL;
	return (0);
}
