/*	$NetBSD: bus_space_through.c,v 1.2 2002/04/14 07:59:59 takemura Exp $	*/

/*-
 * Copyright (c) 2001 TAKEMRUA Shin. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/systm.h>
#include <machine/bus.h>

bus_space_protos(bs_through);
/*
 * Mapping and unmapping operations.
 */
int
bs_through_bs_map(bus_space_tag_t t, bus_addr_t addr,
    bus_size_t size, int cacheable, bus_space_handle_t *bshp)
{
	return bus_space_map(t->bs_base, addr, size, cacheable, bshp);
}

void
bs_through_bs_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	bus_space_unmap(t->bs_base, bsh, size);
}

int
bs_through_bs_subregion(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{
	return bus_space_subregion(t->bs_base, bsh, offset, size, nbshp);
}


/*
 * Allocation and deallocation operations.
 */
int
bs_through_bs_alloc(bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t align, bus_size_t boundary, int cacheable,
    bus_addr_t *addrp, bus_space_handle_t *bshp)
{
	return bus_space_alloc(t->bs_base, rstart, rend, size, align, boundary,
	    cacheable, addrp, bshp);
}

void
bs_through_bs_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	bus_space_free(t->bs_base, bsh, size);
}


/*
 * Get kernel virtual address for ranges mapped BUS_SPACE_MAP_LINEAR.
 */
void *
bs_through_bs_vaddr(bus_space_tag_t t, bus_space_handle_t bsh)
{
	return bus_space_vaddr(t->bs_base, bsh);
}


/*
 * MMap bus space for a user application.
 */
paddr_t
bs_through_bs_mmap(bus_space_tag_t t, bus_addr_t addr, off_t offset,
    int prot, int flags)
{
	return bus_space_mmap(t->bs_base, addr, offset, prot, flags);
}


/*
 * Bus barrier operations.
 */
void
bs_through_bs_barrier(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t len, int flags)
{
	bus_space_barrier(t->bs_base, bsh, offset, len, flags);
}


/*
 * Bus probe operations.
 */
int
bs_through_bs_peek(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, size_t size, void *ptr)
{
	return bus_space_peek(t->bs_base, bsh, offset, size, ptr);
}

int
bs_through_bs_poke(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, size_t size, u_int32_t val)
{
	return bus_space_poke(t->bs_base, bsh, offset, size, val);
}


/*
 * Bus read (single) operations.
 */
u_int8_t
bs_through_bs_r_1(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset)
{
	return bus_space_read_1(t->bs_base, bsh, offset);
}

u_int16_t
bs_through_bs_r_2(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset)
{
	return bus_space_read_2(t->bs_base, bsh, offset);
}

u_int32_t
bs_through_bs_r_4(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset)
{
	return bus_space_read_4(t->bs_base, bsh, offset);
}

u_int64_t
bs_through_bs_r_8(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset)
{
	return bus_space_read_8(t->bs_base, bsh, offset);
}


/*
 * Bus read multiple operations.
 */
void
bs_through_bs_rm_1(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int8_t *addr, bus_size_t count)
{
	bus_space_read_multi_1(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rm_2(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int16_t *addr, bus_size_t count)
{
	bus_space_read_multi_2(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rm_4(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int32_t *addr, bus_size_t count)
{
	bus_space_read_multi_4(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rm_8(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int64_t *addr, bus_size_t count)
{
	bus_space_read_multi_8(t->bs_base, bsh, offset, addr, count);
}


/*
 * Bus read region operations.
 */
void
bs_through_bs_rr_1(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int8_t *addr, bus_size_t count)
{
	bus_space_read_region_1(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rr_2(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int16_t *addr, bus_size_t count)
{
	bus_space_read_region_2(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rr_4(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int32_t *addr, bus_size_t count)
{
	bus_space_read_region_4(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rr_8(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int64_t *addr, bus_size_t count)
{
	bus_space_read_region_8(t->bs_base, bsh, offset, addr, count);
}


/*
 * Bus write (single) operations.
 */
void
bs_through_bs_w_1(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int8_t value)
{
	bus_space_write_1(t->bs_base, bsh, offset, value);
}

void
bs_through_bs_w_2(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int16_t value)
{
	bus_space_write_2(t->bs_base, bsh, offset, value);
}

void
bs_through_bs_w_4(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int32_t value)
{
	bus_space_write_4(t->bs_base, bsh, offset, value);
}

void
bs_through_bs_w_8(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int64_t value)
{
	bus_space_write_8(t->bs_base, bsh, offset, value);
}


/*
 * Bus write multiple operations.
 */
void
bs_through_bs_wm_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int8_t *addr, bus_size_t count)
{
	bus_space_write_multi_1(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wm_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int16_t *addr, bus_size_t count)
{
	bus_space_write_multi_2(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wm_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int32_t *addr, bus_size_t count)
{
	bus_space_write_multi_4(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wm_8(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int64_t *addr, bus_size_t count)
{
	bus_space_write_multi_8(t->bs_base, bsh, offset, addr, count);
}


/*
 * Bus write region operations.
 */
void
bs_through_bs_wr_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int8_t *addr, bus_size_t count)
{
	bus_space_write_region_1(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wr_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int16_t *addr, bus_size_t count)
{
	bus_space_write_region_2(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wr_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int32_t *addr, bus_size_t count)
{
	bus_space_write_region_4(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wr_8(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int64_t *addr, bus_size_t count)
{
	bus_space_write_region_8(t->bs_base, bsh, offset, addr, count);
}

#ifdef BUS_SPACE_HAS_REAL_STREAM_METHODS
/*
 * Bus read (single) operations.
 */
u_int8_t
bs_through_bs_rs_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset)
{
	return bus_space_read_stream_1(t->bs_base, bsh, offset);
}

u_int16_t
bs_through_bs_rs_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset)
{
	return bus_space_read_stream_2(t->bs_base, bsh, offset);
}

u_int32_t
bs_through_bs_rs_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset)
{
	return bus_space_read_stream_4(t->bs_base, bsh, offset);
}

u_int64_t
bs_through_bs_rs_8(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset)
{
	return bus_space_read_stream_8(t->bs_base, bsh, offset);
}


/*
 * Bus read multiple operations.
 */
void
bs_through_bs_rms_1(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int8_t *addr, bus_size_t count)
{
	bus_space_read_multi_stream_1(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rms_2(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int16_t *addr, bus_size_t count)
{
	bus_space_read_multi_stream_2(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rms_4(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int32_t *addr, bus_size_t count)
{
	bus_space_read_multi_stream_4(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rms_8(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int64_t *addr, bus_size_t count)
{
	bus_space_read_multi_stream_8(t->bs_base, bsh, offset, addr, count);
}


/*
 * Bus read region operations.
 */
void
bs_through_bs_rrs_1(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int8_t *addr, bus_size_t count)
{
	bus_space_read_reagion_stream_1(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rrs_2(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int16_t *addr, bus_size_t count)
{
	bus_space_read_reagion_stream_2(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rrs_4(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int32_t *addr, bus_size_t count)
{
	bus_space_read_reagion_stream_4(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_rrs_8(bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, u_int64_t *addr, bus_size_t count)
{
	bus_space_read_reagion_stream_8(t->bs_base, bsh, offset, addr, count);
}


/*
 * Bus write (single) operations.
 */
void
bs_through_bs_ws_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t value)
{
	bus_space_write_stream_1(t->bs_base, bsh, offset, value);
}

void
bs_through_bs_ws_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t value)
{
	bus_space_write_stream_2(t->bs_base, bsh, offset, value);
}

void
bs_through_bs_ws_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t value)
{
	bus_space_write_stream_4(t->bs_base, bsh, offset, value);
}

void
bs_through_bs_ws_8(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int64_t value)
{
	bus_space_write_stream_8(t->bs_base, bsh, offset, value);
}


/*
 * Bus write multiple operations.
 */
void
bs_through_bs_wms_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int8_t *addr, bus_size_t count)
{
	bus_space_write_multi_stream_1(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wms_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int16_t *addr, bus_size_t count)
{
	bus_space_write_multi_stream_2(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wms_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int32_t *addr, bus_size_t count)
{
	bus_space_write_multi_stream_4(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wms_8(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int64_t *addr, bus_size_t count)
{
	bus_space_write_multi_stream_8(t->bs_base, bsh, offset, addr, count);
}


/*
 * Bus write region operations.
 */
void
bs_through_bs_wrs_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int8_t *addr, bus_size_t count)
{
	bus_space_write_region_stream_1(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wrs_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int16_t *addr, bus_size_t count)
{
	bus_space_write_region_stream_2(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wrs_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int32_t *addr, bus_size_t count)
{
	bus_space_write_region_stream_4(t->bs_base, bsh, offset, addr, count);
}

void
bs_through_bs_wrs_8(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const u_int64_t *addr, bus_size_t count)
{
	bus_space_write_region_stream_8(t->bs_base, bsh, offset, addr, count);
}
#endif /* ! BUS_SPACE_HAS_REAL_STREAM_METHODS */


/*
 * Set multiple operations.
 */
void
bs_through_bs_sm_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t value, bus_size_t count)
{
	bus_space_set_multi_1(t->bs_base, bsh, offset, value, count);
}

void
bs_through_bs_sm_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t value, bus_size_t count)
{
	bus_space_set_multi_2(t->bs_base, bsh, offset, value, count);
}

void
bs_through_bs_sm_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t value, bus_size_t count)
{
	bus_space_set_multi_4(t->bs_base, bsh, offset, value, count);
}

void
bs_through_bs_sm_8(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int64_t value, bus_size_t count)
{
	bus_space_set_multi_8(t->bs_base, bsh, offset, value, count);
}

/*
 * Set region operations.
 */
void
bs_through_bs_sr_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t value, bus_size_t count)
{
	bus_space_set_region_1(t->bs_base, bsh, offset, value, count);
}

void
bs_through_bs_sr_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t value, bus_size_t count)
{
	bus_space_set_region_2(t->bs_base, bsh, offset, value, count);
}

void
bs_through_bs_sr_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t value, bus_size_t count)
{
	bus_space_set_region_4(t->bs_base, bsh, offset, value, count);
}

void
bs_through_bs_sr_8(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int64_t value, bus_size_t count)
{
	bus_space_set_region_8(t->bs_base, bsh, offset, value, count);
}


/*
 * Copy operations.
 */
void
bs_through_bs_c_1(bus_space_tag_t t, bus_space_handle_t bsh1,
    bus_size_t offset1, bus_space_handle_t bsh2,
    bus_size_t offset2, bus_size_t n)
{
	bus_space_copy_region_1(t->bs_base, bsh1, offset1, bsh2, offset2, n);
}

void
bs_through_bs_c_2(bus_space_tag_t t, bus_space_handle_t bsh1,
    bus_size_t offset1, bus_space_handle_t bsh2,
    bus_size_t offset2, bus_size_t n)
{
	bus_space_copy_region_2(t->bs_base, bsh1, offset1, bsh2, offset2, n);
}

void
bs_through_bs_c_4(bus_space_tag_t t, bus_space_handle_t bsh1,
    bus_size_t offset1, bus_space_handle_t bsh2,
    bus_size_t offset2, bus_size_t n)
{
	bus_space_copy_region_4(t->bs_base, bsh1, offset1, bsh2, offset2, n);
}

void
bs_through_bs_c_8(bus_space_tag_t t, bus_space_handle_t bsh1,
    bus_size_t offset1, bus_space_handle_t bsh2,
    bus_size_t offset2, bus_size_t n)
{
	bus_space_copy_region_8(t->bs_base, bsh1, offset1, bsh2, offset2, n);
}
