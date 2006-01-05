/*	$NetBSD: pciide_common.c,v 1.8.2.3.2.2 2006/01/05 22:39:22 riz Exp $	*/


/*
 * Copyright (c) 1999, 2000, 2001, 2003 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 */


/*
 * Copyright (c) 1996, 1998 Christopher G. Demetriou.  All rights reserved.
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
 * PCI IDE controller driver.
 *
 * Author: Christopher G. Demetriou, March 2, 1998 (derived from NetBSD
 * sys/dev/pci/ppb.c, revision 1.16).
 *
 * See "PCI IDE Controller Specification, Revision 1.0 3/4/94" and
 * "Programming Interface for Bus Master IDE Controller, Revision 1.0
 * 5/16/94" from the PCI SIG.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pciide_common.c,v 1.8.2.3.2.2 2006/01/05 22:39:22 riz Exp $");

#include <sys/param.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <dev/ic/wdcreg.h>

#ifdef WDCDEBUG
int wdcdebug_pciide_mask = 0;
#endif

static const char dmaerrfmt[] = 
    "%s:%d: unable to %s table DMA map for drive %d, error=%d\n";

/* Default product description for devices not known from this controller */
const struct pciide_product_desc default_product_desc = {
	0,
	0,
	"Generic PCI IDE controller",
	default_chip_map,
};      

const struct pciide_product_desc *
pciide_lookup_product(id, pp)
	pcireg_t id;
	const struct pciide_product_desc *pp;
{
	for (; pp->chip_map != NULL; pp++)
		if (PCI_PRODUCT(id) == pp->ide_product)
			break;
    
	if (pp->chip_map == NULL)
		return NULL;
	return pp;
}

void
pciide_common_attach(sc, pa, pp)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
	const struct pciide_product_desc *pp;
{
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t csr;
	char devinfo[256];
	const char *displaydev;

	aprint_naive(": disk controller\n");
	aprint_normal("\n");

	sc->sc_pci_id = pa->pa_id;
	if (pp == NULL) {
		/* should only happen for generic pciide devices */
		sc->sc_pp = &default_product_desc;
		pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
		displaydev = devinfo;
	} else {
		sc->sc_pp = pp;
		displaydev = sc->sc_pp->ide_name;
	}

	/* if displaydev == NULL, printf is done in chip-specific map */
	if (displaydev)
		aprint_normal("%s: %s (rev. 0x%02x)\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, displaydev,
		    PCI_REVISION(pa->pa_class));

	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;

	/* Set up DMA defaults; these might be adjusted by chip_map. */
	sc->sc_dma_maxsegsz = IDEDMA_BYTE_COUNT_MAX;
	sc->sc_dma_boundary = IDEDMA_BYTE_COUNT_ALIGN;

#ifdef WDCDEBUG
	if (wdcdebug_pciide_mask & DEBUG_PROBE)
		pci_conf_print(sc->sc_pc, sc->sc_tag, NULL);
#endif
	sc->sc_pp->chip_map(sc, pa);

	if (sc->sc_dma_ok) {
		csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		csr |= PCI_COMMAND_MASTER_ENABLE;
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);
	}
	WDCDEBUG_PRINT(("pciide: command/status register=%x\n",
	    pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG)), DEBUG_PROBE);
}

/* tell whether the chip is enabled or not */
int
pciide_chipen(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	pcireg_t csr;

	if ((pa->pa_flags & PCI_FLAGS_IO_ENABLED) == 0) {
		csr = pci_conf_read(sc->sc_pc, sc->sc_tag,
		    PCI_COMMAND_STATUS_REG);
		aprint_normal("%s: device disabled (at %s)\n",
		    sc->sc_wdcdev.sc_dev.dv_xname,
		   (csr & PCI_COMMAND_IO_ENABLE) == 0 ?
		   "device" : "bridge");
		return 0;
	}
	return 1;
}

void
pciide_mapregs_compat(pa, cp, compatchan, cmdsizep, ctlsizep)
	struct pci_attach_args *pa;
	struct pciide_channel *cp;
	int compatchan;
	bus_size_t *cmdsizep, *ctlsizep;
{
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.ch_wdc;
	struct wdc_channel *wdc_cp = &cp->wdc_channel;
	int i;

	cp->compat = 1;
	*cmdsizep = PCIIDE_COMPAT_CMD_SIZE;
	*ctlsizep = PCIIDE_COMPAT_CTL_SIZE;

	wdc_cp->cmd_iot = pa->pa_iot;
	if (bus_space_map(wdc_cp->cmd_iot, PCIIDE_COMPAT_CMD_BASE(compatchan),
	    PCIIDE_COMPAT_CMD_SIZE, 0, &wdc_cp->cmd_baseioh) != 0) {
		aprint_error("%s: couldn't map %s channel cmd regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		goto bad;
	}

	wdc_cp->ctl_iot = pa->pa_iot;
	if (bus_space_map(wdc_cp->ctl_iot, PCIIDE_COMPAT_CTL_BASE(compatchan),
	    PCIIDE_COMPAT_CTL_SIZE, 0, &wdc_cp->ctl_ioh) != 0) {
		aprint_error("%s: couldn't map %s channel ctl regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		bus_space_unmap(wdc_cp->cmd_iot, wdc_cp->cmd_baseioh,
		    PCIIDE_COMPAT_CMD_SIZE);
		goto bad;
	}

	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdc_cp->cmd_iot, wdc_cp->cmd_baseioh, i,
		    i == 0 ? 4 : 1, &wdc_cp->cmd_iohs[i]) != 0) {
			aprint_error("%s: couldn't subregion %s channel "
				     "cmd regs\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
			goto bad;
		}
	}
	wdc_cp->data32iot = wdc_cp->cmd_iot;
	wdc_cp->data32ioh = wdc_cp->cmd_iohs[0];
	return;

bad:
	cp->wdc_channel.ch_flags |= WDCF_DISABLED;
	return;
}

void
pciide_mapregs_native(pa, cp, cmdsizep, ctlsizep, pci_intr)
	struct pci_attach_args * pa;
	struct pciide_channel *cp;
	bus_size_t *cmdsizep, *ctlsizep;
	int (*pci_intr) __P((void *));
{
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.ch_wdc;
	struct wdc_channel *wdc_cp = &cp->wdc_channel;
	const char *intrstr;
	pci_intr_handle_t intrhandle;
	int i;

	cp->compat = 0;

	if (sc->sc_pci_ih == NULL) {
		if (pci_intr_map(pa, &intrhandle) != 0) {
			aprint_error("%s: couldn't map native-PCI interrupt\n",
			    sc->sc_wdcdev.sc_dev.dv_xname);
			goto bad;
		}	
		intrstr = pci_intr_string(pa->pa_pc, intrhandle);
		sc->sc_pci_ih = pci_intr_establish(pa->pa_pc,
		    intrhandle, IPL_BIO, pci_intr, sc);
		if (sc->sc_pci_ih != NULL) {
			aprint_normal("%s: using %s for native-PCI interrupt\n",
			    sc->sc_wdcdev.sc_dev.dv_xname,
			    intrstr ? intrstr : "unknown interrupt");
		} else {
			aprint_error(
			    "%s: couldn't establish native-PCI interrupt",
			    sc->sc_wdcdev.sc_dev.dv_xname);
			if (intrstr != NULL)
				aprint_normal(" at %s", intrstr);
			aprint_normal("\n");
			goto bad;
		}
	}
	cp->ih = sc->sc_pci_ih;
	if (pci_mapreg_map(pa, PCIIDE_REG_CMD_BASE(wdc_cp->ch_channel),
	    PCI_MAPREG_TYPE_IO, 0,
	    &wdc_cp->cmd_iot, &wdc_cp->cmd_baseioh, NULL, cmdsizep) != 0) {
		aprint_error("%s: couldn't map %s channel cmd regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		goto bad;
	}

	if (pci_mapreg_map(pa, PCIIDE_REG_CTL_BASE(wdc_cp->ch_channel),
	    PCI_MAPREG_TYPE_IO, 0,
	    &wdc_cp->ctl_iot, &cp->ctl_baseioh, NULL, ctlsizep) != 0) {
		aprint_error("%s: couldn't map %s channel ctl regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		bus_space_unmap(wdc_cp->cmd_iot, wdc_cp->cmd_baseioh,
		    *cmdsizep);
		goto bad;
	}
	/*
	 * In native mode, 4 bytes of I/O space are mapped for the control
	 * register, the control register is at offset 2. Pass the generic
	 * code a handle for only one byte at the right offset.
	 */
	if (bus_space_subregion(wdc_cp->ctl_iot, cp->ctl_baseioh, 2, 1,
	    &wdc_cp->ctl_ioh) != 0) {
		aprint_error("%s: unable to subregion %s channel ctl regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		bus_space_unmap(wdc_cp->cmd_iot, wdc_cp->cmd_baseioh,
		     *cmdsizep);
		bus_space_unmap(wdc_cp->cmd_iot, cp->ctl_baseioh, *ctlsizep);
		goto bad;
	}

	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdc_cp->cmd_iot, wdc_cp->cmd_baseioh, i,
		    i == 0 ? 4 : 1, &wdc_cp->cmd_iohs[i]) != 0) {
			aprint_error("%s: couldn't subregion %s channel "
				     "cmd regs\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
			goto bad;
		}
	}
	wdc_cp->data32iot = wdc_cp->cmd_iot;
	wdc_cp->data32ioh = wdc_cp->cmd_iohs[0];
	return;

bad:
	cp->wdc_channel.ch_flags |= WDCF_DISABLED;
	return;
}

void
pciide_mapreg_dma(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	pcireg_t maptype;
	bus_addr_t addr;
	struct pciide_channel *pc;
	int reg, chan;
	bus_size_t size;

	/*
	 * Map DMA registers
	 *
	 * Note that sc_dma_ok is the right variable to test to see if
	 * DMA can be done.  If the interface doesn't support DMA,
	 * sc_dma_ok will never be non-zero.  If the DMA regs couldn't
	 * be mapped, it'll be zero.  I.e., sc_dma_ok will only be
	 * non-zero if the interface supports DMA and the registers
	 * could be mapped.
	 *
	 * XXX Note that despite the fact that the Bus Master IDE specs
	 * XXX say that "The bus master IDE function uses 16 bytes of IO
	 * XXX space," some controllers (at least the United
	 * XXX Microelectronics UM8886BF) place it in memory space.
	 */
	maptype = pci_mapreg_type(pa->pa_pc, pa->pa_tag,
	    PCIIDE_REG_BUS_MASTER_DMA);

	switch (maptype) {
	case PCI_MAPREG_TYPE_IO:
		sc->sc_dma_ok = (pci_mapreg_info(pa->pa_pc, pa->pa_tag,
		    PCIIDE_REG_BUS_MASTER_DMA, PCI_MAPREG_TYPE_IO,
		    &addr, NULL, NULL) == 0);
		if (sc->sc_dma_ok == 0) {
			aprint_normal(
			    ", but unused (couldn't query registers)");
			break;
		}
		if ((sc->sc_pp->ide_flags & IDE_16BIT_IOSPACE)
		    && addr >= 0x10000) {
			sc->sc_dma_ok = 0;
			aprint_normal(
			    ", but unused (registers at unsafe address "
			    "%#lx)", (unsigned long)addr);
			break;
		}
		/* FALLTHROUGH */
	
	case PCI_MAPREG_MEM_TYPE_32BIT:
		sc->sc_dma_ok = (pci_mapreg_map(pa,
		    PCIIDE_REG_BUS_MASTER_DMA, maptype, 0,
		    &sc->sc_dma_iot, &sc->sc_dma_ioh, NULL, NULL) == 0);
		sc->sc_dmat = pa->pa_dmat;
		if (sc->sc_dma_ok == 0) {
			aprint_normal(", but unused (couldn't map registers)");
		} else {
			sc->sc_wdcdev.dma_arg = sc;
			sc->sc_wdcdev.dma_init = pciide_dma_init;
			sc->sc_wdcdev.dma_start = pciide_dma_start;
			sc->sc_wdcdev.dma_finish = pciide_dma_finish;
		}

		if (sc->sc_wdcdev.sc_dev.dv_cfdata->cf_flags &
		    PCIIDE_OPTIONS_NODMA) {
			aprint_normal(
			    ", but unused (forced off by config file)");
			sc->sc_dma_ok = 0;
		}
		break;

	default:
		sc->sc_dma_ok = 0;
		aprint_normal(
		    ", but unsupported register maptype (0x%x)", maptype);
	}

	if (sc->sc_dma_ok == 0)
		return;

	/*
	 * Set up the default handles for the DMA registers.
	 * Just reserve 32 bits for each handle, unless space
	 * doesn't permit it.
	 */
	for (chan = 0; chan < PCIIDE_NUM_CHANNELS; chan++) {
		pc = &sc->pciide_channels[chan];
		for (reg = 0; reg < IDEDMA_NREGS; reg++) {
			size = 4;
			if (size > (IDEDMA_SCH_OFFSET - reg))
				size = IDEDMA_SCH_OFFSET - reg;
			if (bus_space_subregion(sc->sc_dma_iot, sc->sc_dma_ioh,
			    IDEDMA_SCH_OFFSET * chan + reg, size,
			    &pc->dma_iohs[reg]) != 0) {
				sc->sc_dma_ok = 0;
				aprint_normal(", but can't subregion offset %d "
					      "size %lu", reg, (u_long)size);
				return;
			}
		}
	}
}

int
pciide_compat_intr(arg)
	void *arg;
{
	struct pciide_channel *cp = arg;

#ifdef DIAGNOSTIC
	/* should only be called for a compat channel */
	if (cp->compat == 0)
		panic("pciide compat intr called for non-compat chan %p", cp);
#endif
	return (wdcintr(&cp->wdc_channel));
}

int
pciide_pci_intr(arg)
	void *arg;
{
	struct pciide_softc *sc = arg;
	struct pciide_channel *cp;
	struct wdc_channel *wdc_cp;
	int i, rv, crv;

	rv = 0;
	for (i = 0; i < sc->sc_wdcdev.nchannels; i++) {
		cp = &sc->pciide_channels[i];
		wdc_cp = &cp->wdc_channel;

		/* If a compat channel skip. */
		if (cp->compat)
			continue;
		/* if this channel not waiting for intr, skip */
		if ((wdc_cp->ch_flags & WDCF_IRQ_WAIT) == 0)
			continue;

		crv = wdcintr(wdc_cp);
		if (crv == 0)
			;		/* leave rv alone */
		else if (crv == 1)
			rv = 1;		/* claim the intr */
		else if (rv == 0)	/* crv should be -1 in this case */
			rv = crv;	/* if we've done no better, take it */
	}
	return (rv);
}

void
pciide_channel_dma_setup(cp)
	struct pciide_channel *cp;
{
	int drive;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.ch_wdc;
	struct ata_drive_datas *drvp;

	for (drive = 0; drive < 2; drive++) {
		drvp = &cp->wdc_channel.ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		/* setup DMA if needed */
		if (((drvp->drive_flags & DRIVE_DMA) == 0 &&
		    (drvp->drive_flags & DRIVE_UDMA) == 0) ||
		    sc->sc_dma_ok == 0) {
			drvp->drive_flags &= ~(DRIVE_DMA | DRIVE_UDMA);
			continue;
		}
		if (pciide_dma_table_setup(sc, cp->wdc_channel.ch_channel,
					   drive) != 0) {
			/* Abort DMA setup */
			drvp->drive_flags &= ~(DRIVE_DMA | DRIVE_UDMA);
			continue;
		}
	}
}

int
pciide_dma_table_setup(sc, channel, drive)
	struct pciide_softc *sc;
	int channel, drive;
{
	bus_dma_segment_t seg;
	int error, rseg;
	const bus_size_t dma_table_size =
	    sizeof(struct idedma_table) * NIDEDMA_TABLES;
	struct pciide_dma_maps *dma_maps =
	    &sc->pciide_channels[channel].dma_maps[drive];

	/* If table was already allocated, just return */
	if (dma_maps->dma_table)
		return 0;

	/* Allocate memory for the DMA tables and map it */
	if ((error = bus_dmamem_alloc(sc->sc_dmat, dma_table_size,
	    IDEDMA_TBL_ALIGN, IDEDMA_TBL_ALIGN, &seg, 1, &rseg,
	    BUS_DMA_NOWAIT)) != 0) {
		aprint_error(dmaerrfmt, sc->sc_wdcdev.sc_dev.dv_xname, channel,
		    "allocate", drive, error);
		return error;
	}
	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    dma_table_size,
	    (caddr_t *)&dma_maps->dma_table,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		aprint_error(dmaerrfmt, sc->sc_wdcdev.sc_dev.dv_xname, channel,
		    "map", drive, error);
		return error;
	}
	WDCDEBUG_PRINT(("pciide_dma_table_setup: table at %p len %lu, "
	    "phy 0x%lx\n", dma_maps->dma_table, (u_long)dma_table_size,
	    (unsigned long)seg.ds_addr), DEBUG_PROBE);
	/* Create and load table DMA map for this disk */
	if ((error = bus_dmamap_create(sc->sc_dmat, dma_table_size,
	    1, dma_table_size, IDEDMA_TBL_ALIGN, BUS_DMA_NOWAIT,
	    &dma_maps->dmamap_table)) != 0) {
		aprint_error(dmaerrfmt, sc->sc_wdcdev.sc_dev.dv_xname, channel,
		    "create", drive, error);
		return error;
	}
	if ((error = bus_dmamap_load(sc->sc_dmat,
	    dma_maps->dmamap_table,
	    dma_maps->dma_table,
	    dma_table_size, NULL, BUS_DMA_NOWAIT)) != 0) {
		aprint_error(dmaerrfmt, sc->sc_wdcdev.sc_dev.dv_xname, channel,
		    "load", drive, error);
		return error;
	}
	WDCDEBUG_PRINT(("pciide_dma_table_setup: phy addr of table 0x%lx\n",
	    (unsigned long)dma_maps->dmamap_table->dm_segs[0].ds_addr),
	    DEBUG_PROBE);
	/* Create a xfer DMA map for this drive */
	if ((error = bus_dmamap_create(sc->sc_dmat, IDEDMA_BYTE_COUNT_MAX,
	    NIDEDMA_TABLES, sc->sc_dma_maxsegsz, sc->sc_dma_boundary,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
	    &dma_maps->dmamap_xfer)) != 0) {
		aprint_error(dmaerrfmt, sc->sc_wdcdev.sc_dev.dv_xname, channel,
		    "create xfer", drive, error);
		return error;
	}
	return 0;
}

int
pciide_dma_dmamap_setup(sc, channel, drive, databuf, datalen, flags)
	struct pciide_softc *sc;
	int channel, drive;
	void *databuf;
	size_t datalen;
	int flags;
{
	int error, seg;
	struct pciide_channel *cp = &sc->pciide_channels[channel];
	struct pciide_dma_maps *dma_maps = &cp->dma_maps[drive];

	error = bus_dmamap_load(sc->sc_dmat,
	    dma_maps->dmamap_xfer,
	    databuf, datalen, NULL, BUS_DMA_NOWAIT | BUS_DMA_STREAMING |
	    ((flags & WDC_DMA_READ) ? BUS_DMA_READ : BUS_DMA_WRITE));
	if (error) {
		printf(dmaerrfmt, sc->sc_wdcdev.sc_dev.dv_xname, channel,
		    "load xfer", drive, error);
		return error;
	}

	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_xfer, 0,
	    dma_maps->dmamap_xfer->dm_mapsize,
	    (flags & WDC_DMA_READ) ?
	    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	for (seg = 0; seg < dma_maps->dmamap_xfer->dm_nsegs; seg++) {
#ifdef DIAGNOSTIC
		/* A segment must not cross a 64k boundary */
		{
		u_long phys = dma_maps->dmamap_xfer->dm_segs[seg].ds_addr;
		u_long len = dma_maps->dmamap_xfer->dm_segs[seg].ds_len;
		if ((phys & ~IDEDMA_BYTE_COUNT_MASK) !=
		    ((phys + len - 1) & ~IDEDMA_BYTE_COUNT_MASK)) {
			printf("pciide_dma: segment %d physical addr 0x%lx"
			    " len 0x%lx not properly aligned\n",
			    seg, phys, len);
			panic("pciide_dma: buf align");
		}
		}
#endif
		dma_maps->dma_table[seg].base_addr =
		    htole32(dma_maps->dmamap_xfer->dm_segs[seg].ds_addr);
		dma_maps->dma_table[seg].byte_count =
		    htole32(dma_maps->dmamap_xfer->dm_segs[seg].ds_len &
		    IDEDMA_BYTE_COUNT_MASK);
		WDCDEBUG_PRINT(("\t seg %d len %d addr 0x%x\n",
		   seg, le32toh(dma_maps->dma_table[seg].byte_count),
		   le32toh(dma_maps->dma_table[seg].base_addr)), DEBUG_DMA);

	}
	dma_maps->dma_table[dma_maps->dmamap_xfer->dm_nsegs -1].byte_count |=
	    htole32(IDEDMA_BYTE_COUNT_EOT);

	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_table, 0,
	    dma_maps->dmamap_table->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

#ifdef DIAGNOSTIC
	if (dma_maps->dmamap_table->dm_segs[0].ds_addr & ~IDEDMA_TBL_MASK) {
		printf("pciide_dma_dmamap_setup: addr 0x%lx "
		    "not properly aligned\n",
		    (u_long)dma_maps->dmamap_table->dm_segs[0].ds_addr);
		panic("pciide_dma_init: table align");
	}
#endif
	/* remember flags */
	dma_maps->dma_flags = flags;

	return 0;
}

int
pciide_dma_init(v, channel, drive, databuf, datalen, flags)
	void *v;
	int channel, drive;
	void *databuf;
	size_t datalen;
	int flags;
{
	struct pciide_softc *sc = v;
	int error;
	struct pciide_channel *cp = &sc->pciide_channels[channel];
	struct pciide_dma_maps *dma_maps = &cp->dma_maps[drive];

	if ((error = pciide_dma_dmamap_setup(sc, channel, drive,
	    databuf, datalen, flags)) != 0)
		return error;
	/* Maps are ready. Start DMA function */
	/* Clear status bits */
	bus_space_write_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CTL], 0,
	    bus_space_read_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CTL], 0));
	/* Write table addr */
	bus_space_write_4(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_TBL], 0,
	    dma_maps->dmamap_table->dm_segs[0].ds_addr);
	/* set read/write */
	bus_space_write_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CMD], 0,
	    ((flags & WDC_DMA_READ) ? IDEDMA_CMD_WRITE : 0) | cp->idedma_cmd);
	return 0;
}

void
pciide_dma_start(v, channel, drive)
	void *v;
	int channel, drive;
{
	struct pciide_softc *sc = v;
	struct pciide_channel *cp = &sc->pciide_channels[channel];

	WDCDEBUG_PRINT(("pciide_dma_start\n"),DEBUG_XFERS);
	bus_space_write_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CMD], 0,
	    bus_space_read_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CMD], 0)
		| IDEDMA_CMD_START);
}

int
pciide_dma_finish(v, channel, drive, force)
	void *v;
	int channel, drive;
	int force;
{
	struct pciide_softc *sc = v;
	u_int8_t status;
	int error = 0;
	struct pciide_channel *cp = &sc->pciide_channels[channel];
	struct pciide_dma_maps *dma_maps = &cp->dma_maps[drive];

	status = bus_space_read_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CTL], 0);
	WDCDEBUG_PRINT(("pciide_dma_finish: status 0x%x\n", status),
	    DEBUG_XFERS);

	if (force == 0 && (status & IDEDMA_CTL_INTR) == 0)
		return WDC_DMAST_NOIRQ;

	/* stop DMA channel */
	bus_space_write_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CMD], 0,
	    bus_space_read_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CMD], 0)
		& ~IDEDMA_CMD_START);

	/* Unload the map of the data buffer */
	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_xfer, 0,
	    dma_maps->dmamap_xfer->dm_mapsize,
	    (dma_maps->dma_flags & WDC_DMA_READ) ?
	    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, dma_maps->dmamap_xfer);

	if ((status & IDEDMA_CTL_ERR) != 0) {
		printf("%s:%d:%d: bus-master DMA error: status=0x%x\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, channel, drive, status);
		error |= WDC_DMAST_ERR;
	}

	if ((status & IDEDMA_CTL_INTR) == 0) {
		printf("%s:%d:%d: bus-master DMA error: missing interrupt, "
		    "status=0x%x\n", sc->sc_wdcdev.sc_dev.dv_xname, channel,
		    drive, status);
		error |= WDC_DMAST_NOIRQ;
	}

	if ((status & IDEDMA_CTL_ACT) != 0) {
		/* data underrun, may be a valid condition for ATAPI */
		error |= WDC_DMAST_UNDER;
	}
	return error;
}

void
pciide_irqack(chp)
	struct wdc_channel *chp;
{
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.ch_wdc;

	/* clear status bits in IDE DMA registers */
	bus_space_write_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CTL], 0,
	    bus_space_read_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CTL], 0));
}

/* some common code used by several chip_map */
int
pciide_chansetup(sc, channel, interface)
	struct pciide_softc *sc;
	int channel;
	pcireg_t interface;
{
	struct pciide_channel *cp = &sc->pciide_channels[channel];
	sc->wdc_chanarray[channel] = &cp->wdc_channel;
	cp->name = PCIIDE_CHANNEL_NAME(channel);
	cp->wdc_channel.ch_channel = channel;
	cp->wdc_channel.ch_wdc = &sc->sc_wdcdev;
	cp->wdc_channel.ch_queue =
	    malloc(sizeof(struct ata_queue), M_DEVBUF, M_NOWAIT);
	if (cp->wdc_channel.ch_queue == NULL) {
		aprint_error("%s %s channel: "
		    "can't allocate memory for command queue",
		sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		return 0;
	}
	aprint_normal("%s: %s channel %s to %s mode\n",
	    sc->sc_wdcdev.sc_dev.dv_xname, cp->name,
	    (interface & PCIIDE_INTERFACE_SETTABLE(channel)) ?
	    "configured" : "wired",
	    (interface & PCIIDE_INTERFACE_PCI(channel)) ?
	    "native-PCI" : "compatibility");
	return 1;
}

/* some common code used by several chip channel_map */
void
pciide_mapchan(pa, cp, interface, cmdsizep, ctlsizep, pci_intr)
	struct pci_attach_args *pa;
	struct pciide_channel *cp;
	pcireg_t interface;
	bus_size_t *cmdsizep, *ctlsizep;
	int (*pci_intr) __P((void *));
{
	struct wdc_channel *wdc_cp = &cp->wdc_channel;

	if (interface & PCIIDE_INTERFACE_PCI(wdc_cp->ch_channel))
		pciide_mapregs_native(pa, cp, cmdsizep, ctlsizep, pci_intr);
	else {
		pciide_mapregs_compat(pa, cp, wdc_cp->ch_channel, cmdsizep,
		    ctlsizep);
		if ((cp->wdc_channel.ch_flags & WDCF_DISABLED) == 0)
			pciide_map_compat_intr(pa, cp, wdc_cp->ch_channel);
	}
	wdcattach(wdc_cp);
}

/*
 * generic code to map the compat intr.
 */
void
pciide_map_compat_intr(pa, cp, compatchan)
	struct pci_attach_args *pa;
	struct pciide_channel *cp;
	int compatchan;
{
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.ch_wdc;

#ifdef __HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH
	cp->ih = pciide_machdep_compat_intr_establish(&sc->sc_wdcdev.sc_dev,
	    pa, compatchan, pciide_compat_intr, cp);
	if (cp->ih == NULL) {
#endif
		aprint_error("%s: no compatibility interrupt for use by %s "
		    "channel\n", sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		cp->wdc_channel.ch_flags |= WDCF_DISABLED;
#ifdef __HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH
	}
#endif
}

void
default_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	struct pciide_channel *cp;
	pcireg_t interface = PCI_INTERFACE(pa->pa_class);
	pcireg_t csr;
	int channel, drive;
	u_int8_t idedma_ctl;
	bus_size_t cmdsize, ctlsize;
	char *failreason;

	if (pciide_chipen(sc, pa) == 0)
		return;

	if (interface & PCIIDE_INTERFACE_BUS_MASTER_DMA) {
		aprint_normal("%s: bus-master DMA support present",
		    sc->sc_wdcdev.sc_dev.dv_xname);
		if (sc->sc_pp == &default_product_desc &&
		    (sc->sc_wdcdev.sc_dev.dv_cfdata->cf_flags &
		    PCIIDE_OPTIONS_DMA) == 0) {
			aprint_normal(", but unused (no driver support)");
			sc->sc_dma_ok = 0;
		} else {
			pciide_mapreg_dma(sc, pa);
			if (sc->sc_dma_ok != 0)
				aprint_normal(", used without full driver "
				    "support");
		}
	} else {
		aprint_normal("%s: hardware does not support DMA",
		    sc->sc_wdcdev.sc_dev.dv_xname);
		sc->sc_dma_ok = 0;
	}
	aprint_normal("\n");
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA | WDC_CAPABILITY_IRQACK;
		sc->sc_wdcdev.irqack = pciide_irqack;
	}
	sc->sc_wdcdev.PIO_cap = 0;
	sc->sc_wdcdev.DMA_cap = 0;

	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		if (pciide_chansetup(sc, channel, interface) == 0)
			continue;
		if (interface & PCIIDE_INTERFACE_PCI(channel))
			pciide_mapregs_native(pa, cp, &cmdsize, &ctlsize,
			    pciide_pci_intr);
		else
			pciide_mapregs_compat(pa, cp,
			    cp->wdc_channel.ch_channel, &cmdsize, &ctlsize);
		if (cp->wdc_channel.ch_flags & WDCF_DISABLED)
			continue;
		/*
		 * Check to see if something appears to be there.
		 */
		failreason = NULL;
		/*
		 * In native mode, always enable the controller. It's
		 * not possible to have an ISA board using the same address
		 * anyway.
		 */
		if (interface & PCIIDE_INTERFACE_PCI(channel)) {
			wdcattach(&cp->wdc_channel);
			continue;
		}
		if (!wdcprobe(&cp->wdc_channel)) {
			failreason = "not responding; disabled or no drives?";
			goto next;
		}
		/*
		 * Now, make sure it's actually attributable to this PCI IDE
		 * channel by trying to access the channel again while the
		 * PCI IDE controller's I/O space is disabled.  (If the
		 * channel no longer appears to be there, it belongs to
		 * this controller.)  YUCK!
		 */
		csr = pci_conf_read(sc->sc_pc, sc->sc_tag,
		    PCI_COMMAND_STATUS_REG);
		pci_conf_write(sc->sc_pc, sc->sc_tag, PCI_COMMAND_STATUS_REG,
		    csr & ~PCI_COMMAND_IO_ENABLE);
		if (wdcprobe(&cp->wdc_channel))
			failreason = "other hardware responding at addresses";
		pci_conf_write(sc->sc_pc, sc->sc_tag,
		    PCI_COMMAND_STATUS_REG, csr);
next:
		if (failreason) {
			aprint_error("%s: %s channel ignored (%s)\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name,
			    failreason);
			cp->wdc_channel.ch_flags |= WDCF_DISABLED;
			bus_space_unmap(cp->wdc_channel.cmd_iot,
			    cp->wdc_channel.cmd_baseioh, cmdsize);
			bus_space_unmap(cp->wdc_channel.ctl_iot,
			    cp->wdc_channel.ctl_ioh, ctlsize);
		} else {
			pciide_map_compat_intr(pa, cp,
			    cp->wdc_channel.ch_channel);
			wdcattach(&cp->wdc_channel);
		}
	}

	if (sc->sc_dma_ok == 0)
		return;

	/* Allocate DMA maps */
	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		idedma_ctl = 0;
		cp = &sc->pciide_channels[channel];
		for (drive = 0; drive < 2; drive++) {
			/*
			 * we have not probed the drives yet, allocate
			 * ressources for all of them.
			 */
			if (pciide_dma_table_setup(sc, channel, drive) != 0) {
				/* Abort DMA setup */
				aprint_error(
				    "%s:%d:%d: can't allocate DMA maps, "
				    "using PIO transfers\n",
				    sc->sc_wdcdev.sc_dev.dv_xname,
				    channel, drive);
				sc->sc_dma_ok = 0;
				sc->sc_wdcdev.cap &= ~(WDC_CAPABILITY_DMA |
				    WDC_CAPABILITY_IRQACK);
				break;
			}
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		}
		if (idedma_ctl != 0) {
			/* Add software bits in status register */
			bus_space_write_1(sc->sc_dma_iot,
			    cp->dma_iohs[IDEDMA_CTL], 0, idedma_ctl);
		}
	}
}

void
sata_setup_channel(chp)
	struct wdc_channel *chp;
{
	struct ata_drive_datas *drvp;
	int drive;
	u_int32_t idedma_ctl;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc*)cp->wdc_channel.ch_wdc;

	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	idedma_ctl = 0;

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		if (drvp->drive_flags & DRIVE_UDMA) {
			/* use Ultra/DMA */
			drvp->drive_flags &= ~DRIVE_DMA;
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		} else if (drvp->drive_flags & DRIVE_DMA) {
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		}
	}

	/*
	 * Nothing to do to setup modes; it is meaningless in S-ATA
	 * (but many S-ATA drives still want to get the SET_FEATURE
	 * command).
	 */
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CTL], 0,
		    idedma_ctl);
	}
}
