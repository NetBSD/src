/*	$NetBSD: loongson_bus_io.c,v 1.2 2012/03/02 13:20:57 nonaka Exp $	*/

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
 * Platform-specific PCI bus I/O support for loongson-based systems
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: loongson_bus_io.c,v 1.2 2012/03/02 13:20:57 nonaka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/locore.h>

#include <mips/bonito/bonitoreg.h>

#include <sys/bus.h>

#include <evbmips/loongson/loongson_bus_defs.h>
#include <evbmips/loongson/autoconf.h>

#define	CHIP			bonito
#define CHIP_IO			/* defined */

#define	CHIP_EX_MALLOC_SAFE(v)	(ex_mallocsafe)
#define	CHIP_EXTENT(v)		(loongson_io_ex)

/* IO region 1 */
#define	CHIP_W1_BUS_START(v)	0x00000000UL
#define	CHIP_W1_BUS_END(v)	0x000fffffUL
#define	CHIP_W1_SYS_START(v)	((u_long)BONITO_PCIIO_BASE)
#define	CHIP_W1_SYS_END(v)	((u_long)BONITO_PCIIO_BASE + 0x000fffffUL)

#include <mips/mips/bus_space_alignstride_chipdep.c>

int
bonito_bus_io_legacy_map(void *v, bus_addr_t addr, bus_size_t size, int flags,
    bus_space_handle_t *hp, int acct)
{
	const struct legacy_io_range *r;
	bus_addr_t rend;

	if (addr < BONITO_PCIIO_LEGACY) {
		r = sys_platform->legacy_io_ranges;
		if (r == NULL)
			return (ENXIO);

		rend = addr + size - 1;
		for (; r->start != 0; r++)
			if (addr >= r->start && rend <= r->end)
				break;
		if (r->end == 0)
			return (ENXIO);
	}
	return __BS(map)(v, addr, size, flags, hp, acct);
}
