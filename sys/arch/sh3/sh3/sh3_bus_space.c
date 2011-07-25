/*	$NetBSD: sh3_bus_space.c,v 1.1 2011/07/25 21:12:23 dyoung Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
__KERNEL_RCSID(0, "$NetBSD: sh3_bus_space.c,v 1.1 2011/07/25 21:12:23 dyoung Exp $");

#include <sys/types.h>
#include <sys/bus.h>

/*
 *	u_intN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

uint8_t
bus_space_read_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return *(volatile uint8_t *)(bsh + offset);
}

uint16_t
bus_space_read_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return bswap16(*(volatile uint16_t *)(bsh + offset));
}

uint32_t
bus_space_read_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return bswap32(*(volatile uint32_t *)(bsh + offset));
}

uint16_t
bus_space_read_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return *(volatile uint16_t *)(bsh + offset);
}

uint32_t
bus_space_read_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return *(volatile uint32_t *)(bsh + offset);
}

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */
void
bus_space_read_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_1(tag, bsh, offset);
}

void
bus_space_read_multi_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_2(tag, bsh, offset);
}

void
bus_space_read_multi_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_4(tag, bsh, offset);
}

void
bus_space_read_multi_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = *(volatile uint16_t *)(bsh + offset);
}

void
bus_space_read_multi_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = *(volatile uint32_t *)(bsh + offset);
}

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */
void
bus_space_read_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{
	uint8_t *p = (uint8_t *)(bsh + offset);

	while (count--)
		*addr++ = *p++;
}

void
bus_space_read_region_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{
	uint16_t *p = (uint16_t *)(bsh + offset);

	while (count--)
		*addr++ = bswap16(*p++);
}

void
bus_space_read_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{
	uint32_t *p = (uint32_t *)(bsh + offset);

	while (count--)
		*addr++ = bswap32(*p++);
}

/*
 *	void bus_space_read_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */
void
bus_space_read_region_stream_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{
	uint8_t *p = (uint8_t *)(bsh + offset);

	while (count--)
		*addr++ = *p++;
}

void
bus_space_read_region_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{
	uint16_t *p = (uint16_t *)(bsh + offset);

	while (count--)
		*addr++ = *p++;
}

void
bus_space_read_region_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{
	uint32_t *p = (uint32_t *)(bsh + offset);

	while (count--)
		*addr++ = *p++;
}

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */
void
bus_space_write_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{
	uint8_t *p = (uint8_t *)(bsh + offset);

	while (count--)
		*p++ = *addr++;
}

void
bus_space_write_region_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{
	uint16_t *p = (uint16_t *)(bsh + offset);

	while (count--)
		*p++ = bswap16(*addr++);
}

void
bus_space_write_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{
	uint32_t *p = (uint32_t *)(bsh + offset);

	while (count--)
		*p++ = bswap32(*addr++);
}

/*
 *	void bus_space_write_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */
void
bus_space_write_region_stream_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{
	uint8_t *p = (uint8_t *)(bsh + offset);

	while (count--)
		*p++ = *addr++;
}

void
bus_space_write_region_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{
	uint16_t *p = (uint16_t *)(bsh + offset);

	while (count--)
		*p++ = *addr++;
}

void
bus_space_write_region_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{
	uint32_t *p = (uint32_t *)(bsh + offset);

	while (count--)
		*p++ = *addr++;
}

/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */
void
bus_space_write_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value)
{

	*(volatile uint8_t *)(bsh + offset) = value;
}

void
bus_space_write_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value)
{

	*(volatile uint16_t *)(bsh + offset) = bswap16(value);
}

void
bus_space_write_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value)
{

	*(volatile uint32_t *)(bsh + offset) = bswap32(value);
}

void
bus_space_write_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value)
{

	*(volatile uint16_t *)(bsh + offset) = value;
}

void
bus_space_write_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value)
{

	*(volatile uint32_t *)(bsh + offset) = value;
}

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */
void
bus_space_write_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_1(tag, bsh, offset, *addr++);
}

void
bus_space_write_multi_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_2(tag, bsh, offset, *addr++);
}

void
bus_space_write_multi_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_4(tag, bsh, offset, *addr++);
}

void
bus_space_write_multi_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_stream_2(tag, bsh, offset, *addr++);
}

void
bus_space_write_multi_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_stream_4(tag, bsh, offset, *addr++);
}

/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    bus_size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */
void
bus_space_set_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count)
{

	while (count--)
		bus_space_write_1(tag, bsh, offset, val);
}

void
bus_space_set_multi_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count)
{

	while (count--)
		bus_space_write_2(tag, bsh, offset, val);
}

void
bus_space_set_multi_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count)
{

	while (count--)
		bus_space_write_4(tag, bsh, offset, val);
}

/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
void
bus_space_set_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count)
{
	volatile uint8_t *addr = (void *)(bsh + offset);

	while (count--)
		*addr++ = val;
}

void
bus_space_set_region_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count)
{
	volatile uint16_t *addr = (void *)(bsh + offset);

	val = bswap16(val);
	while (count--)
		*addr++ = val;
}

void
bus_space_set_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count)
{
	volatile uint32_t *addr = (void *)(bsh + offset);

	val = bswap32(val);
	while (count--)
		*addr++ = val;
}

/*
 *	void bus_space_set_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
void
bus_space_set_region_stream_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count)
{
	volatile uint8_t *addr = (void *)(bsh + offset);

	while (count--)
		*addr++ = val;
}

void
bus_space_set_region_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count)
{
	volatile uint16_t *addr = (void *)(bsh + offset);

	while (count--)
		*addr++ = val;
}

void
bus_space_set_region_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count)
{
	volatile uint32_t *addr = (void *)(bsh + offset);

	while (count--)
		*addr++ = val;
}

/*
 *	void bus_space_copy_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    bus_size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */
void
bus_space_copy_region_1(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	volatile uint8_t *addr1 = (void *)(h1 + o1);
	volatile uint8_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (c--)
			*addr2++ = *addr1++;
	} else {		/* dest after src: copy backwards */
		addr1 += c - 1;
		addr2 += c - 1;
		while (c--)
			*addr2-- = *addr1--;
	}
}

void
bus_space_copy_region_2(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	volatile uint16_t *addr1 = (void *)(h1 + o1);
	volatile uint16_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (c--)
			*addr2++ = *addr1++;
	} else {		/* dest after src: copy backwards */
		addr1 += c - 1;
		addr2 += c - 1;
		while (c--)
			*addr2-- = *addr1--;
	}
}

void
bus_space_copy_region_4(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	volatile uint32_t *addr1 = (void *)(h1 + o1);
	volatile uint32_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (c--)
			*addr2++ = *addr1++;
	} else {		/* dest after src: copy backwards */
		addr1 += c - 1;
		addr2 += c - 1;
		while (c--)
			*addr2-- = *addr1--;
	}
}

