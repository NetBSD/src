/* $NetBSD: pci_machdep.h,v 1.3.38.1 2007/05/08 20:24:54 rjs Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <powerpc/pci_machdep.h>

#ifdef _KERNEL
/*
 * ibmnws-specific PCI functions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */
int ibmnws_pci_bus_maxdevs(pci_chipset_tag_t, int);
int ibmnws_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
int ibmnws_pci_conf_hook(pci_chipset_tag_t, int, int, int, pcireg_t);

void ibmnws_pci_get_chipset_tag_indirect(pci_chipset_tag_t);
void ibmnws_pci_get_chipset_tag(pci_chipset_tag_t pc);

/*
 * ibmnws-specific PCI data.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */
extern struct powerpc_bus_dma_tag pci_bus_dma_tag;
#endif
