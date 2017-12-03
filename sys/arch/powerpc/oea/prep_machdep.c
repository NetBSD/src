/* $NetBSD: prep_machdep.c,v 1.10.6.1 2017/12/03 11:36:37 jdolecek Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
 * This is a set of general routines that most PReP-similar machines have
 * in common.  Machines that use these routines do not have to be fully
 * PReP compliant, the only requirement is that they use a PReP memory map.
 * IE, io at 0x80000000 and mem at 0xc0000000.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: prep_machdep.c,v 1.10.6.1 2017/12/03 11:36:37 jdolecek Exp $");

#include "opt_modular.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>
#include <machine/powerpc.h>
#include <sys/bus.h>
#include <machine/pmap.h>
#include <powerpc/oea/bat.h>

#include "opt_ddb.h"
#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
extern void *endsym, *startsym;
#endif
extern struct mem_region physmemr[2], availmemr[2];

struct powerpc_bus_space prep_io_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	.pbs_offset = PREP_BUS_SPACE_IO,
	.pbs_base = 0x00000000,
	.pbs_limit = PREP_PHYS_SIZE_IO,
};

struct powerpc_bus_space genppc_isa_io_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	.pbs_offset = PREP_BUS_SPACE_IO,
	.pbs_base = 0x00000000,
	.pbs_limit = PREP_ISA_SIZE_IO,
};

struct powerpc_bus_space prep_mem_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	.pbs_offset = PREP_BUS_SPACE_MEM,
	.pbs_base = 0x00000000,
	.pbs_limit = PREP_PHYS_SIZE_MEM,
};

struct powerpc_bus_space genppc_isa_mem_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	.pbs_offset = PREP_BUS_SPACE_MEM,
	.pbs_base = 0x00000000,
	.pbs_limit = PREP_ISA_SIZE_MEM,
};

static char ex_storage[2][EXTENT_FIXED_STORAGE_SIZE(8)]
	__attribute__((aligned(8)));

void
prep_bus_space_init(void)
{
	int error;

	error = bus_space_init(&prep_io_space_tag, "ioport",
	    ex_storage[0], sizeof(ex_storage[0]));
	if (error)
		panic("prep_bus_space_init: can't init io tag");

	error = extent_alloc_region(prep_io_space_tag.pbs_extent,
	    PREP_PHYS_RESVD_START_IO, PREP_PHYS_RESVD_SIZE_IO, EX_NOWAIT);
	if (error)
		panic("prep_bus_space_init: can't block out reserved I/O"
		    " space 0x10000-0x7fffff: error=%d", error);

	error = bus_space_init(&prep_mem_space_tag, "iomem",
	    ex_storage[1], sizeof(ex_storage[1]));
	if (error)
		panic("prep_bus_space_init: can't init mem tag");

	genppc_isa_io_space_tag.pbs_extent = prep_io_space_tag.pbs_extent;
	error = bus_space_init(&genppc_isa_io_space_tag, "isa-ioport", NULL, 0);	if (error)
		panic("prep_bus_space_init: can't init isa io tag");

	genppc_isa_mem_space_tag.pbs_extent = prep_mem_space_tag.pbs_extent;
	error = bus_space_init(&genppc_isa_mem_space_tag, "isa-iomem", NULL, 0);
	if (error)
		panic("prep_bus_space_init: can't init isa mem tag");
}

/*
 * just a few calls used in initppc() all in one place so everything gets
 * done the same across the ports
 */

void
prep_initppc(u_long startkernel, u_long endkernel, u_int args)
{

	/*
	 * Now setup fixed bat registers
	 * We setup the memory BAT, the IO space BAT, and a special
	 * BAT for certain machines that have rs6k style PCI bridges
	 * (only on port-prep)
	 */
	oea_batinit(
	    PREP_BUS_SPACE_MEM, BAT_BL_256M,
	    PREP_BUS_SPACE_IO,  BAT_BL_256M,
#if defined(bebox)
	    0x7ffff000, BAT_BL_8M,	/* BeBox Mainboard Registers (4KB) */
#elif defined(prep)
	    0xbf800000, BAT_BL_8M,
#endif
	    0);

	/* Install vectors and interrupt handler. */
	oea_init(NULL);

	/* Initialize bus_space. */
	prep_bus_space_init();

	/* Initialize the console asap. */
	consinit();

	/* Set the page size */
	uvm_md_init();

	/* Initialize pmap module */
	pmap_bootstrap(startkernel, endkernel);

#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((int)((u_long)endsym - (u_long)startsym), startsym, endsym);
#endif

#ifdef DDB
	boothowto = args;
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{

	*mem = physmemr;
	*avail = availmemr;
}
