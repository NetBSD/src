/*
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1996 Jason R. Thorpe.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>

#include <machine/bat.h>
#include <machine/bus.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>

/*
 * bus space read/write function
 *  need endian swap
 */
#define	SWAP_16(p) ((u_int16_t)((p[1]<<8)|(p[0]<<0)))
#define	SWAP_32(p) ((u_int32_t)((p[3]<<24)|(p[2]<<16)|(p[1]<<8)|(p[0]<<0)))

/*
 *	u_intN_t bus_space_read_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset));
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */
u_int8_t bus_space_read_1(tag, bsh, offset)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return (*(u_int8_t *)(tag + bsh + offset));
}

u_int16_t bus_space_read_2(tag, bsh, offset)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	u_int16_t val = *(u_int16_t *)(tag + bsh + offset);
	u_int8_t *p = (u_int8_t *)&val;

	return (SWAP_16(p));
}

u_int32_t bus_space_read_4(tag, bsh, offset)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	u_int32_t val = *(u_int32_t *)(tag + bsh + offset);
	u_int8_t *p = (u_int8_t *)&val;

	return (SWAP_32(p));
}

/*
 *	void bus_space_write_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value));
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

void	bus_space_write_1(tag, bsh, offset, value)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t value;
{
	*(u_int8_t *)(tag + bsh + offset) = value;
}

void	bus_space_write_2(tag, bsh, offset, value)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t value;
{
	u_int8_t *p = (u_int8_t *)&value;
	u_int16_t val = SWAP_16(p);

	*(u_int16_t *)(tag + bsh + offset) = val;
}

void	bus_space_write_4(tag, bsh, offset, value)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t value;
{
	u_int8_t *p = (u_int8_t *)&value;
	u_int32_t val = SWAP_32(p);

	*(u_int32_t *)(tag + bsh + offset) = val;
}

/*
 * Access methods for bus resources and address space.
 */
int
bus_space_map(t, bpa, size, cacheable, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int cacheable;
	bus_space_handle_t *bshp;
{
	*bshp = bpa;
	return (0);
}

void
bus_space_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
}

int
bus_space_subregion(t, bsh, offset, size, nbshp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	bus_size_t size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return (0);
}
