/*	$NetBSD: gtpcivar.h,v 1.2 2003/03/16 07:05:34 matt Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_MARVELL_GTPCIVAR_H
#define _DEV_MARVELL_GTPCIVAR_H

/*
 */
struct gtpci_chipset {
	struct pci_chipset gtpc_pc;
	int gtpc_busno;
	bus_space_tag_t gtpc_io_bs;
	bus_space_tag_t gtpc_mem_bs;
	bus_space_tag_t gtpc_gt_memt;
	bus_space_handle_t gtpc_gt_memh;
	bus_size_t gtpc_cfgaddr;
	bus_size_t gtpc_cfgdata;
	bus_size_t gtpc_syncreg;
};


#ifdef _KERNEL
static uint32_t __inline
gtpci_read(struct gtpci_chipset *gtpc, bus_size_t reg)
{
	uint32_t rv;
	(void) bus_space_read_4(gtpc->gtpc_gt_memt, gtpc->gtpc_gt_memh,
	    gtpc->gtpc_syncreg);
	rv = bus_space_read_4(gtpc->gtpc_gt_memt, gtpc->gtpc_gt_memh, reg);
	(void) bus_space_read_4(gtpc->gtpc_gt_memt, gtpc->gtpc_gt_memh,
	    gtpc->gtpc_syncreg);
	return rv;
}

static void __inline
gtpci_write(struct gtpci_chipset *gtpc, bus_size_t reg, uint32_t val)
{
	(void) bus_space_read_4(gtpc->gtpc_gt_memt, gtpc->gtpc_gt_memh,
	    gtpc->gtpc_syncreg);
	bus_space_write_4(gtpc->gtpc_gt_memt, gtpc->gtpc_gt_memh, reg, val);
	(void) bus_space_read_4(gtpc->gtpc_gt_memt, gtpc->gtpc_gt_memh,
	    gtpc->gtpc_syncreg);
}

pcitag_t gtpci_make_tag(pci_chipset_tag_t, int, int, int);
void	gtpci_decompose_tag(pci_chipset_tag_t, pcitag_t, int *, int *, int *);
pcireg_t gtpci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void	gtpci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);

void	gtpci_md_bus_devorder(pci_chipset_tag_t, int, char []);
int	gtpci_md_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
int	gtpci_md_conf_hook(pci_chipset_tag_t, int, int, int, pcireg_t);
void	gtpci_md_conf_interrupt(pci_chipset_tag_t, int, int, int, int, int *);
#endif /* _KERNEL */

#endif /* _DEV_MARVELL_GTPCIVAR_H */
