/*	$NetBSD: rbus_machdep.h,v 1.1 2003/03/22 06:36:18 nakayama Exp $	*/

/*
 * Copyright (c) 2003 Takeshi Nakayama.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SPARC64_RBUS_MACHDEP_H_
#define _SPARC64_RBUS_MACHDEP_H_

struct pci_attach_args;
rbus_tag_t rbus_pccbb_parent_io(struct pci_attach_args *);
rbus_tag_t rbus_pccbb_parent_mem(struct pci_attach_args *);
void pccbb_attach_hook(struct device *, struct device *,
		       struct pci_attach_args *);
#define __HAVE_PCCBB_ATTACH_HOOK

int md_space_map(bus_space_tag_t, bus_addr_t, bus_size_t, int,
		 bus_space_handle_t *);
void md_space_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t,
		    bus_addr_t *);

#endif /* _SPARC64_RBUS_MACHDEP_H_ */
