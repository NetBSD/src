/* $NetBSD: universe_pci_var.h,v 1.1 2000/02/25 18:22:39 drochner Exp $ */

/*
 * Copyright (c) 1999
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Common data and functions for PCI-VME interfaces using the
 * Newbridge/Tundra Universe II (CA91C142).
 * The chip doesn't adhere to the PCI specs wrt address window usage and
 * interrupt routing, so we need implementation dependant front-ends.
 */
struct univ_pci_data {
	pci_chipset_tag_t pc;
	bus_space_tag_t csrt;
	bus_space_handle_t csrh;
	void *ih;
};

int univ_pci_attach __P((struct univ_pci_data *, struct pci_attach_args *));
int univ_pci_mapvme __P((struct univ_pci_data *, int, vme_addr_t, u_int32_t,
			 vme_am_t, vme_datasize_t, u_int32_t));
void univ_pci_unmapvme __P((struct univ_pci_data *, int));
