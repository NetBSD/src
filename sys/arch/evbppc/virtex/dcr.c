/* 	$NetBSD: dcr.c,v 1.3 2022/03/10 00:14:16 riastradh Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * DCR bus space size & base addresses are hardcoded and runtime-checked,
 * so there's no point in maintaning extents etc -- just provide placebo
 * implementations so that we can use bus_space as usual (add more as
 * needed).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dcr.c,v 1.3 2022/03/10 00:14:16 riastradh Exp $");

#include <sys/types.h>
#include <sys/bus.h>
#include <evbppc/virtex/dcr.h>

int
dcr_map(bus_space_tag_t bst, bus_addr_t addr, bus_size_t size, int flags,
    bus_space_handle_t *bsh)
{
	*bsh = addr;

	return (0);
}

void
dcr_unmap(bus_space_tag_t bst, bus_space_handle_t bsh, bus_size_t size)
{
	/* Nothing to do. */
}

int
dcr_subregion(bus_space_tag_t bst, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *bshp)
{
	*bshp = bsh + offset;

	return (0);
}

void
dcr_barrier(bus_space_tag_t bst, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, int flags)
{
	/* XXX EIEIO? */
}
