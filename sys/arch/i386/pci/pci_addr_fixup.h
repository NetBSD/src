/*	$NetBSD: pci_addr_fixup.h,v 1.6 2005/12/26 19:24:00 perry Exp $	*/

/*-
 * Copyright (c) 2000 UCHIYAMA Yasushi.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

struct pciaddr {
	struct extent *extent_mem;
	struct extent *extent_port;
	bus_addr_t mem_alloc_start;
	bus_addr_t port_alloc_start;
	int nbogus;
};

extern struct pciaddr pciaddr;

void	pci_addr_fixup(pci_chipset_tag_t, int);

/* for cardbus stuff */
typedef int (*pciaddr_resource_manage_func_t) 
	(pci_chipset_tag_t, pcitag_t, int, void *, int,
	 bus_addr_t *, bus_size_t);

void	pciaddr_resource_manage(pci_chipset_tag_t, pcitag_t,
				pciaddr_resource_manage_func_t,
				     void *);

void	pciaddr_print_devid(pci_chipset_tag_t, pcitag_t);

bus_addr_t pciaddr_ioaddr(uint32_t);
