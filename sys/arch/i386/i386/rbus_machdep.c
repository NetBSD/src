/*	$NetBSD: rbus_machdep.c,v 1.9 2000/06/29 08:44:54 mrg Exp $	*/

/*
 * Copyright (c) 1999
 *     HAYAKAWA Koichi.  All rights reserved.
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
 *	This product includes software developed by HAYAKAWA Koichi.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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


#include "opt_pcibios.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/bus.h>
#include <dev/cardbus/rbus.h>

#include <sys/device.h>
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/pci/pcivar.h>
#ifdef PCIBIOS_ADDR_FIXUP
#include <arch/i386/pci/pci_addr_fixup.h>
#endif

/*
 * void _i386_memio_unmap(bus_space_tag bst, bus_space_handle bsh,
 *                        bus_size_t size, bus_addr_t *adrp)
 *
 *   This function unmaps memory- or io-space mapped by the function
 *   _i386_memio_map().  This function works nearly as same as
 *   i386_memio_unmap(), but this function does not ask kernel
 *   built-in extents and returns physical address of the bus space,
 *   for the convenience of the extra extent manager.
 *
 *   I suppose this function should be in arch/i386/i386/machdep.c,
 *   but it is not.
 */
void
_i386_memio_unmap(t, bsh, size, adrp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
	bus_addr_t *adrp;
{
	u_long va, endva;
	bus_addr_t bpa;

	/*
	 * Find the correct extent and bus physical address.
	 */
	if (t == I386_BUS_SPACE_IO) {
		bpa = bsh;
	} else if (t == I386_BUS_SPACE_MEM) {
		if (bsh >= atdevbase && (bsh + size) <= (atdevbase + IOM_SIZE)) {
			bpa = (bus_addr_t)ISA_PHYSADDR(bsh);
		} else {

			va = i386_trunc_page(bsh);
			endva = i386_round_page(bsh + size);

#ifdef DIAGNOSTIC
			if (endva <= va) {
				panic("_i386_memio_unmap: overflow");
			}
#endif

#if __NetBSD_Version__ > 104050000
			if (pmap_extract(pmap_kernel(), va, &bpa) == FALSE) {
				panic("_i386_memio_unmap:"
				    "i386/rbus_machdep.c wrong virtual address");
			}
			bpa += (bsh & PGOFSET);
#else
			bpa = pmap_extract(pmap_kernel(), va) + (bsh & PGOFSET);
#endif

			/*
			 * Free the kernel virtual mapping.
			 */
			uvm_km_free(kernel_map, va, endva - va);
		}
	} else {
		panic("_i386_memio_unmap: bad bus space tag");
	}

	if (adrp != NULL) {
		*adrp = bpa;
	}
}




/*
 * rbus_tag_t rbus_fakeparent_mem(struct pci_attach_args *pa)
 *
 *   This function makes an rbus tag for memory space.  This rbus tag
 *   shares the all memory region of ex_iomem.
 */
rbus_tag_t
rbus_pccbb_parent_mem(pa)
	struct pci_attach_args *pa;
{
	bus_addr_t start, offset;
	bus_size_t size;
	struct extent *ex;
#ifdef PCIBIOS_ADDR_FIXUP
	ex = pciaddr.extent_mem;
#else
	extern struct extent *iomem_ex;
	ex = iomem_ex;
#endif

	start = ex->ex_start;

	/*
	 * XXX: unfortunately, iomem_ex cannot be used for the dynamic
	 * bus_space allocatoin.  There are some hidden memory (or
	 * some obstacles which do not recognised by the kernel) in
	 * the region governed by iomem_ex.  So I decide to use only
	 * very high address region.
	 *
	 * if defined PCIBIOS_ADDR_FIXUP, PCI device using area
	 * which do not recognised by the kernel are already reserved.
	 */

	if (start < 0x40000000) {
		start = 0x40000000;	/* 1GB */
	}

	size = ex->ex_end - start;
	offset = 0;
  
	return rbus_new_root_share(pa->pa_memt, ex, start, size, 0);
}


/*
 * rbus_tag_t rbus_pccbb_parent_io(struct pci_attach_args *pa)
 */
rbus_tag_t
rbus_pccbb_parent_io(pa)
	struct pci_attach_args *pa;
{
	struct extent *ex;
	bus_addr_t start;
	bus_size_t size;
#ifdef PCIBIOS_ADDR_FIXUP
	ex = pciaddr.extent_port;
#else
	extern struct extent *ioport_ex;
	ex = ioport_ex;
#endif
	start =  0x2000;
	size =  0x1000;

	return rbus_new_root_share(pa->pa_iot, ex, start, size, 0);
}
