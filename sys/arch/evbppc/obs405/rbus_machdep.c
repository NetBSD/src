/*	$NetBSD: rbus_machdep.c,v 1.8.2.1 2012/04/17 00:06:18 yamt Exp $	*/

/*
 * Copyright (c) 2003
 *     KIYOHARA Takashi.  All rights reserved.
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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/bus.h>


#include <uvm/uvm_extern.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/cardbus/rbus.h>

#include <uvm/uvm_extern.h>

#include "opt_pci.h"

#ifndef PCI_NETBSD_CONFIGURE
#error requird macro PCI_NETBSD_CONFIGURE
#endif

#ifdef RBUS_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

int
md_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size, int flags, bus_space_handle_t *bshp)
{
	DPRINTF("md_space_map: 0x%x, 0x%x, 0x%x\n", t->pbs_base, bpa, size);

	return bus_space_map(t, bpa, size, flags, bshp);
}

void
md_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size, bus_addr_t *adrp)
{
	paddr_t pa;

	DPRINTF("md_space_unmap: 0x%x 0x%x\n", t->pbs_base, bsh);

	if (adrp != NULL) {
        	pmap_extract(pmap_kernel(), bsh, &pa);
		*adrp = pa - t->pbs_offset;
	}
	bus_space_unmap(t, bsh, size);
}

rbus_tag_t
rbus_pccbb_parent_mem(struct pci_attach_args *pa)
{
	bus_space_tag_t bst = pa->pa_memt;

	return rbus_new_root_delegate(bst, bst->pbs_base,
	    bst->pbs_limit - bst->pbs_base, 0);
}

rbus_tag_t
rbus_pccbb_parent_io(struct pci_attach_args *pa)
{
	bus_space_tag_t bst = pa->pa_iot;

	return rbus_new_root_delegate(bst, bst->pbs_base,
	    bst->pbs_limit - bst->pbs_base, 0);
}
