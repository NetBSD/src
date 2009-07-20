/*	$NetBSD: bus_space.c,v 1.1 2009/07/20 04:41:36 kiyohara Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.1 2009/07/20 04:41:36 kiyohara Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>


int
ia64_bus_space_map(bus_space_tag_t t, bus_addr_t addr,
    bus_size_t size, int flags, bus_space_handle_t *bshp)
{
	if (t == IA64_BUS_SPACE_IO) {
		if (addr > 0xffff)
			return 1;
		*bshp = addr;
	} else {
		/* t == IA64_BUS_SPACE_MEM */

		if (flags & BUS_SPACE_MAP_CACHEABLE)
			*bshp = addr + IA64_RR_BASE(7);
		else
			*bshp = addr + IA64_RR_BASE(6);
	}

	return 0;
}

void
ia64_bus_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t size)
{
	/* nop */
}

int
ia64_bus_space_subregion(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *bshp)
{
	*bshp = bsh + offset;
	return 0;
}

paddr_t
ia64_bus_space_mmap(bus_space_tag_t t, bus_addr_t addr, off_t offset,
    int prot, int flags)
{
	/* Can't mmap I/O space */
	if (t == IA64_BUS_SPACE_IO)
		return -1;

	return addr + offset;
}

int
ia64_bus_space_alloc(bus_space_tag_t t, bus_addr_t reg_start,
    bus_addr_t reg_end, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, int flags, bus_addr_t *addrp, bus_space_handle_t *bshp)
{
	bus_addr_t addr;

	addr = (reg_start + PAGE_SIZE - 1) & ~PAGE_MASK;

	if ((addr % alignment) == 0) {
		*addrp = addr;
		*bshp = (bus_space_handle_t)addr;
		return 0;
	} else {
		return 1;
	}
}

void
ia64_bus_space_free(bus_space_tag_t t, bus_space_handle_t handle,
    bus_size_t size)
{
	/* nop */
}
