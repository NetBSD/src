/*	$NetBSD: pciidevar.h,v 1.7 2001/06/08 04:48:58 simonb Exp $	*/

/*
 * Copyright (c) 1998 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * PCI IDE driver exported software structures.
 *
 * Author: Christopher G. Demetriou, March 2, 1998.
 */

#include <dev/ata/atavar.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>

struct device;

struct pciide_softc {
	struct wdc_softc	sc_wdcdev;	/* common wdc definitions */
	pci_chipset_tag_t	sc_pc;		/* PCI registers info */
	pcitag_t		sc_tag;
	void			*sc_pci_ih;	/* PCI interrupt handle */
	int			sc_dma_ok;	/* bus-master DMA info */
	bus_space_tag_t		sc_dma_iot;
	bus_space_handle_t	sc_dma_ioh;
	bus_dma_tag_t		sc_dmat;

	/* For Cypress */
	const struct cy82c693_handle *sc_cy_handle;
	int sc_cy_compatchan;

	/* Chip description */
	const struct pciide_product_desc *sc_pp;
	/* common definitions */
	struct channel_softc *wdc_chanarray[PCIIDE_NUM_CHANNELS];
	/* internal bookkeeping */
	struct pciide_channel {			/* per-channel data */
		struct channel_softc wdc_channel; /* generic part */
		char		*name;
		int		hw_ok;	/* hardware mapped & OK? */
		int		compat;	/* is it compat? */
		void		*ih;	/* compat or pci handle */
		bus_space_handle_t ctl_baseioh; /* ctrl regs blk, native mode */
		/* DMA tables and DMA map for xfer, for each drive */
		struct pciide_dma_maps {
			bus_dmamap_t    dmamap_table;
			struct idedma_table *dma_table;
			bus_dmamap_t    dmamap_xfer;
			int dma_flags;
		} dma_maps[2];
	} pciide_channels[PCIIDE_NUM_CHANNELS];
};

/*
 * Functions defined by machine-dependent code.
 */

/* Attach compat interrupt handler, returning handle or NULL if failed. */
#ifdef __HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH
void	*pciide_machdep_compat_intr_establish __P((struct device *,
	    struct pci_attach_args *, int, int (*)(void *), void *));
#endif
