/*	$NetBSD: g1bus_bus_mem.c,v 1.1.18.2 2017/12/03 11:36:00 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 * Bus space implementation for the SEGA G1 bus, for GD-ROM and IDE port.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: g1bus_bus_mem.c,v 1.1.18.2 2017/12/03 11:36:00 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <machine/cpu.h>

#include <dreamcast/dev/g1/g1busvar.h>

int	g1bus_bus_mem_map(void *, bus_addr_t, bus_size_t, int,
	    bus_space_handle_t *);
void	g1bus_bus_mem_unmap(void *, bus_space_handle_t, bus_size_t);
int	g1bus_bus_mem_subregion(void *, bus_space_handle_t, bus_size_t,
	    bus_size_t, bus_space_handle_t *);
paddr_t	g1bus_bus_mem_mmap(void *, bus_addr_t, off_t, int, int);

uint8_t g1bus_bus_mem_read_1(void *, bus_space_handle_t, bus_size_t);
uint16_t g1bus_bus_mem_read_2(void *, bus_space_handle_t, bus_size_t);
uint32_t g1bus_bus_mem_read_4(void *, bus_space_handle_t, bus_size_t);

void	g1bus_bus_mem_write_1(void *, bus_space_handle_t, bus_size_t,
	    uint8_t);
void	g1bus_bus_mem_write_2(void *, bus_space_handle_t, bus_size_t,
	    uint16_t);
void	g1bus_bus_mem_write_4(void *, bus_space_handle_t, bus_size_t,
	    uint32_t);

void	g1bus_bus_mem_read_region_1(void *, bus_space_handle_t, bus_size_t,
	    uint8_t *, bus_size_t);
void	g1bus_bus_mem_read_region_2(void *, bus_space_handle_t, bus_size_t,
	    uint16_t *, bus_size_t);
void	g1bus_bus_mem_read_region_4(void *, bus_space_handle_t, bus_size_t,
	    uint32_t *, bus_size_t);

void	g1bus_bus_mem_write_region_1(void *, bus_space_handle_t, bus_size_t,
	    const uint8_t *, bus_size_t);
void	g1bus_bus_mem_write_region_2(void *, bus_space_handle_t, bus_size_t,
	    const uint16_t *, bus_size_t);
void	g1bus_bus_mem_write_region_4(void *, bus_space_handle_t, bus_size_t,
	    const uint32_t *, bus_size_t);

void	g1bus_bus_mem_set_region_4(void *, bus_space_handle_t, bus_size_t,
	    uint32_t, bus_size_t);

void	g1bus_bus_mem_read_multi_1(void *, bus_space_handle_t,
	    bus_size_t, uint8_t *, bus_size_t);
void	g1bus_bus_mem_read_multi_2(void *, bus_space_handle_t,
	    bus_size_t, uint16_t *, bus_size_t);

void	g1bus_bus_mem_write_multi_1(void *, bus_space_handle_t,
	    bus_size_t, const uint8_t *, bus_size_t);
void	g1bus_bus_mem_write_multi_2(void *, bus_space_handle_t,
	    bus_size_t, const uint16_t *, bus_size_t);

void
g1bus_bus_mem_init(struct g1bus_softc *sc)
{
	bus_space_tag_t t = &sc->sc_memt;

	memset(t, 0, sizeof(*t));

	t->dbs_map = g1bus_bus_mem_map;
	t->dbs_unmap = g1bus_bus_mem_unmap;
	t->dbs_subregion = g1bus_bus_mem_subregion;
	t->dbs_mmap = g1bus_bus_mem_mmap;

	t->dbs_r_1 = g1bus_bus_mem_read_1;
	t->dbs_r_2 = g1bus_bus_mem_read_2;
	t->dbs_r_4 = g1bus_bus_mem_read_4;

	t->dbs_w_1 = g1bus_bus_mem_write_1;
	t->dbs_w_2 = g1bus_bus_mem_write_2;
	t->dbs_w_4 = g1bus_bus_mem_write_4;

	t->dbs_rm_1 = g1bus_bus_mem_read_multi_1;
	t->dbs_rm_2 = g1bus_bus_mem_read_multi_2;

	t->dbs_wm_1 = g1bus_bus_mem_write_multi_1;
	t->dbs_wm_2 = g1bus_bus_mem_write_multi_2;

	t->dbs_rr_1 = g1bus_bus_mem_read_region_1;
	t->dbs_rr_2 = g1bus_bus_mem_read_region_2;
	t->dbs_rr_4 = g1bus_bus_mem_read_region_4;

	t->dbs_wr_1 = g1bus_bus_mem_write_region_1;
	t->dbs_wr_2 = g1bus_bus_mem_write_region_2;
	t->dbs_wr_4 = g1bus_bus_mem_write_region_4;

	t->dbs_sr_4 = g1bus_bus_mem_set_region_4;
}

int
g1bus_bus_mem_map(void *v, bus_addr_t addr, bus_size_t size, int flags,
    bus_space_handle_t *shp)
{

	KASSERT((addr & SH3_PHYS_MASK) == addr);
	*shp = SH3_PHYS_TO_P2SEG(addr);

	return 0;
}

void
g1bus_bus_mem_unmap(void *v, bus_space_handle_t sh, bus_size_t size)
{

	KASSERT(sh >= SH3_P2SEG_BASE && sh <= SH3_P2SEG_END);
	/* Nothing to do. */
}

int
g1bus_bus_mem_subregion(void *v, bus_space_handle_t handle, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nhandlep)
{

	*nhandlep = handle + offset;
	return 0;
}

paddr_t
g1bus_bus_mem_mmap(void *v, bus_addr_t addr, off_t offset, int prot, int flags)
{

	/* XXX not implemented */
	return -1;
}

uint8_t
g1bus_bus_mem_read_1(void *v, bus_space_handle_t sh, bus_size_t off)
{
	uint8_t rv;

	rv = *(volatile uint8_t *)(sh + off);

	return rv;
}

uint16_t
g1bus_bus_mem_read_2(void *v, bus_space_handle_t sh, bus_size_t off)
{
	uint16_t rv;

	rv = *(volatile uint16_t *)(sh + off);

	return rv;
}

uint32_t
g1bus_bus_mem_read_4(void *v, bus_space_handle_t sh, bus_size_t off)
{
	uint32_t rv;

	rv = *(volatile uint32_t *)(sh + off);

	return rv;
}

void
g1bus_bus_mem_write_1(void *v, bus_space_handle_t sh, bus_size_t off,
    uint8_t val)
{

	*(volatile uint8_t *)(sh + off) = val;
}

void
g1bus_bus_mem_write_2(void *v, bus_space_handle_t sh, bus_size_t off,
    uint16_t val)
{

	*(volatile uint16_t *)(sh + off) = val;
}

void
g1bus_bus_mem_write_4(void *v, bus_space_handle_t sh, bus_size_t off,
    uint32_t val)
{

	*(volatile uint32_t *)(sh + off) = val;
}

void
g1bus_bus_mem_read_multi_1(void *v, bus_space_handle_t sh,
    bus_size_t off, uint8_t *addr, bus_size_t len)
{
	volatile const uint8_t *baddr = (uint8_t *)(sh + off);

	while (len--)
		*addr++ = *baddr;
}

void
g1bus_bus_mem_read_multi_2(void *v, bus_space_handle_t sh,
    bus_size_t off, uint16_t *addr, bus_size_t len)
{
	volatile uint16_t *baddr = (uint16_t *)(sh + off);

	while (len--)
		*addr++ = *baddr;
}

void
g1bus_bus_mem_write_multi_1(void *v, bus_space_handle_t sh,
    bus_size_t off, const uint8_t *addr, bus_size_t len)
{
	volatile uint8_t *baddr = (uint8_t *)(sh + off);

	while (len--)
		*baddr = *addr++;
}

void
g1bus_bus_mem_write_multi_2(void *v, bus_space_handle_t sh,
    bus_size_t off, const uint16_t *addr, bus_size_t len)
{
	volatile uint16_t *baddr = (uint16_t *)(sh + off);

	while (len--)
		*baddr = *addr++;
}

void
g1bus_bus_mem_read_region_1(void *v, bus_space_handle_t sh, bus_size_t off,
    uint8_t *addr, bus_size_t len)
{
	volatile const uint8_t *baddr = (uint8_t *)(sh + off);

	while (len--)
		*addr++ = *baddr++;
}

void
g1bus_bus_mem_read_region_2(void *v, bus_space_handle_t sh, bus_size_t off,
    uint16_t *addr, bus_size_t len)
{
	volatile const uint16_t *baddr = (uint16_t *)(sh + off);

	while (len--)
		*addr++ = *baddr++;
}

void
g1bus_bus_mem_read_region_4(void *v, bus_space_handle_t sh, bus_size_t off,
    uint32_t *addr, bus_size_t len)
{
	volatile const uint32_t *baddr = (uint32_t *)(sh + off);

	while (len--)
		*addr++ = *baddr++;
}

void
g1bus_bus_mem_write_region_1(void *v, bus_space_handle_t sh, bus_size_t off,
    const uint8_t *addr, bus_size_t len)
{
	volatile uint8_t *baddr = (uint8_t *)(sh + off);

	while (len--)
		*baddr++ = *addr++;
}

void
g1bus_bus_mem_write_region_2(void *v, bus_space_handle_t sh, bus_size_t off,
    const uint16_t *addr, bus_size_t len)
{
	volatile uint16_t *baddr = (uint16_t *)(sh + off);

	while (len--)
		*baddr++ = *addr++;
}

void
g1bus_bus_mem_write_region_4(void *v, bus_space_handle_t sh, bus_size_t off,
    const uint32_t *addr, bus_size_t len)
{
	volatile uint32_t *baddr = (uint32_t *)(sh + off);

	while (len--)
		*baddr++ = *addr++;
}

void
g1bus_bus_mem_set_region_4(void *v, bus_space_handle_t sh, bus_size_t off,
    uint32_t val, bus_size_t len)
{
	volatile uint32_t *baddr = (uint32_t *)(sh + off);

	while (len--)
		*baddr++ = val;
}
