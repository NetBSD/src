/*	$NetBSD: maltavar.h,v 1.2 2002/03/18 01:21:12 simonb Exp $	*/

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

#include <machine/bus.h>
#include <dev/pci/pcivar.h>
#include <dev/isa/isavar.h>

#include <evbmips/malta/dev/gtvar.h>

struct malta_config {
	struct gt_config mc_gt;

	struct mips_bus_space mc_iot;
	struct mips_bus_space mc_memt;

	struct mips_bus_dma_tag mc_pci_dmat;
	struct mips_bus_dma_tag mc_isa_dmat;

	struct evbmips_pci_chipset mc_pc;
	struct evbmips_isa_chipset mc_ic;

	struct extent *mc_io_ex;
	struct extent *mc_mem_ex;

	int	mc_mallocsafe;
};

#ifdef _KERNEL
extern struct malta_config malta_configuration;

void	malta_bus_io_init(bus_space_tag_t, void *);
void	malta_bus_mem_init(bus_space_tag_t, void *);
void	malta_cal_timer(bus_space_tag_t, bus_space_handle_t);
void	malta_dma_init(struct malta_config *);
int	malta_get_ethaddr(int, u_int8_t *);
void	malta_intr_init(struct malta_config *);
void	malta_pci_init(pci_chipset_tag_t, struct malta_config *);
#endif /* _KERNEL */
