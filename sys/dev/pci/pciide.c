/*	$NetBSD: pciide.c,v 1.10 1998/10/13 08:59:46 bouyer Exp $	*/

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

#define DEBUG_DMA   0x01
#define DEBUG_XFERS  0x02
#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#ifdef WDCDEBUG
int wdcdebug_pciide_mask = DEBUG_PROBE;
#define WDCDEBUG_PRINT(args, level) \
	if (wdcdebug_pciide_mask & (level)) printf args
#else
#define WDCDEBUG_PRINT(args, level)
#endif
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_kern.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>
#include <dev/pci/pciide_piix_reg.h>
#include <dev/pci/pciide_apollo_reg.h>
#include <dev/pci/pciide_cmd_reg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>

struct pciide_softc {
	struct wdc_softc	sc_wdcdev;	/* common wdc definitions */

	void			*sc_pci_ih;	/* PCI interrupt handle */
	int			sc_dma_ok;	/* bus-master DMA info */
	bus_space_tag_t		sc_dma_iot;
	bus_space_handle_t	sc_dma_ioh;
	bus_dma_tag_t		sc_dmat;
	/* Chip description */
	const struct pciide_product_desc *sc_pp;
	/* common definitions */
	struct channel_softc wdc_channels[PCIIDE_NUM_CHANNELS];
	/* internal bookkeeping */
	struct pciide_channel {			/* per-channel data */
		int		hw_ok;		/* hardware mapped & OK? */
		int		compat;		/* is it compat? */
		void		*ih;		/* compat or pci handle */
		/* DMA tables and DMA map for xfer, for each drive */
		struct pciide_dma_maps {
			bus_dmamap_t    dmamap_table;
			struct idedma_table *dma_table;
			bus_dmamap_t    dmamap_xfer;
		} dma_maps[2];
	} pciide_channels[PCIIDE_NUM_CHANNELS];
};

void default_setup_cap __P((struct pciide_softc*));
void default_setup_chip __P((struct pciide_softc*,
		pci_chipset_tag_t, pcitag_t));
const char *default_channel_probe __P((struct pciide_softc *,
		struct pci_attach_args *, int));
int default_channel_disable __P((struct pciide_softc *,
		struct pci_attach_args *, int));


void piix_setup_cap __P((struct pciide_softc*));
void piix_setup_chip __P((struct pciide_softc*,
		pci_chipset_tag_t, pcitag_t));
void piix3_4_setup_chip __P((struct pciide_softc*,
		pci_chipset_tag_t, pcitag_t));
const char *piix_channel_probe __P((struct pciide_softc *,
		struct pci_attach_args *, int));
int piix_channel_disable __P((struct pciide_softc *,
		struct pci_attach_args *, int));
static u_int32_t piix_setup_idetim_timings __P((u_int8_t, u_int8_t, u_int8_t));
static u_int32_t piix_setup_idetim_drvs __P((struct ata_drive_datas*));
static u_int32_t piix_setup_sidetim_timings __P((u_int8_t, u_int8_t, u_int8_t));

void apollo_setup_cap __P((struct pciide_softc*));
void apollo_setup_chip __P((struct pciide_softc*,
		pci_chipset_tag_t, pcitag_t));
const char *apollo_channel_probe __P((struct pciide_softc *,
		struct pci_attach_args *, int));
int apollo_channel_disable __P((struct pciide_softc *,
		struct pci_attach_args *, int));

const char *cmd_channel_probe __P((struct pciide_softc *,
		struct pci_attach_args *, int));
int cmd_channel_disable __P((struct pciide_softc *,
		struct pci_attach_args *, int));

int  pciide_dma_table_setup __P((struct pciide_softc*, int, int));
int  pciide_dma_init __P((void*, int, int, void *, size_t, int));
void pciide_dma_start __P((void*, int, int, int));
int  pciide_dma_finish __P((void*, int, int, int));

struct pciide_product_desc {
    u_int32_t ide_product;
    int ide_flags;
    const char *ide_name;
    /* init controller's capabilities for drives probe */
    void (*setup_cap) __P((struct pciide_softc*));
    /* init controller after drives probe */
    void (*setup_chip) __P((struct pciide_softc*, pci_chipset_tag_t, pcitag_t));
    /* Probe for compat channel enabled/disabled */
    const char * (*channel_probe) __P((struct pciide_softc *,
		struct pci_attach_args *, int));
    int  (*channel_disable) __P((struct pciide_softc *,
		struct pci_attach_args *, int));
};

/* Flags for ide_flags */
#define CMD_PCI064x_IOEN 0x01 /* CMD-style PCI_COMMAND_IO_ENABLE */
#define ONE_QUEUE         0x02 /* device need serialised access */

/* Default product description for devices not known from this controller */
const struct pciide_product_desc default_product_desc = {
    0,
    0,
    "Generic PCI IDE controller",
    default_setup_cap,
    default_setup_chip,
    default_channel_probe,
    default_channel_disable
};


const struct pciide_product_desc pciide_intel_products[] =  {
    { PCI_PRODUCT_INTEL_82092AA,
      0,
      "Intel 82092AA IDE controller",
      default_setup_cap,
      default_setup_chip,
      default_channel_probe,
      default_channel_disable
    },
    { PCI_PRODUCT_INTEL_82371FB_IDE,
      0,
      "Intel 82371FB IDE controller (PIIX)",
      piix_setup_cap,
      piix_setup_chip,
      piix_channel_probe,
      piix_channel_disable
    },
    { PCI_PRODUCT_INTEL_82371SB_IDE,
      0,
      "Intel 82371SB IDE Interface (PIIX3)",
      piix_setup_cap,
      piix3_4_setup_chip,
      piix_channel_probe,
      piix_channel_disable
    },
    { PCI_PRODUCT_INTEL_82371AB_IDE,
      0,
      "Intel 82371AB IDE controller (PIIX4)",
      piix_setup_cap,
      piix3_4_setup_chip,
      piix_channel_probe,
      piix_channel_disable
    },
    { 0,
      0,
      NULL,
    }
};
const struct pciide_product_desc pciide_cmd_products[] =  {
    { PCI_PRODUCT_CMDTECH_640,
      ONE_QUEUE | CMD_PCI064x_IOEN,
      "CMD Technology PCI0640",
      default_setup_cap,
      default_setup_chip,
      cmd_channel_probe,
      cmd_channel_disable
    },
    { 0,
      0,
      NULL,
    }
};

const struct pciide_product_desc pciide_via_products[] =  {
    { PCI_PRODUCT_VIATECH_VT82C586_IDE,
      0,
      "VT82C586 (Apollo VP) IDE Controller",
      apollo_setup_cap,
      apollo_setup_chip,
      apollo_channel_probe,
      apollo_channel_disable
     },
     { 0,
       0,
       NULL,
     }
};

struct pciide_vendor_desc {
    u_int32_t ide_vendor;
    const struct pciide_product_desc *ide_products;
};

const struct pciide_vendor_desc pciide_vendors[] = {
    { PCI_VENDOR_INTEL, pciide_intel_products },
    { PCI_VENDOR_CMDTECH, pciide_cmd_products },
    { PCI_VENDOR_VIATECH, pciide_via_products },
    { 0, NULL }
};


#define	PCIIDE_CHANNEL_NAME(chan)	((chan) == 0 ? "primary" : "secondary")

int	pciide_match __P((struct device *, struct cfdata *, void *));
void	pciide_attach __P((struct device *, struct device *, void *));

struct cfattach pciide_ca = {
	sizeof(struct pciide_softc), pciide_match, pciide_attach
};

int	pciide_map_channel_compat __P((struct pciide_softc *,
	    struct pci_attach_args *, int));
int	pciide_map_channel_native __P((struct pciide_softc *,
	    struct pci_attach_args *, int));
int	pciide_print __P((void *, const char *pnp));
int	pciide_compat_intr __P((void *));
int	pciide_pci_intr __P((void *));
const struct pciide_product_desc* pciide_lookup_product __P((u_int32_t));

const struct pciide_product_desc*
pciide_lookup_product(id)
    u_int32_t id;
{
    const struct pciide_product_desc *pp;
    const struct pciide_vendor_desc *vp;

    for (vp = pciide_vendors; vp->ide_products != NULL; vp++)
	if (PCI_VENDOR(id) == vp->ide_vendor)
	    break;

    if ((pp = vp->ide_products) == NULL)
	return NULL;

    for (; pp->ide_name != NULL; pp++)
	if (PCI_PRODUCT(id) == pp->ide_product)
	    break;
    
    if (pp->ide_name == NULL)
	return NULL;
    return pp;
}

int
pciide_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	/*
	 * Check the ID register to see that it's a PCI IDE controller.
	 * If it is, we assume that we can deal with it; it _should_
	 * work in a standardized way...
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_MASS_STORAGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_IDE) {
		return (1);
	}

	return (0);
}

void
pciide_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	struct pciide_softc *sc = (struct pciide_softc *)self;
	struct pciide_channel *cp;
	pcireg_t class, interface, csr;
	pci_intr_handle_t intrhandle;
	const char *intrstr;
	char devinfo[256];
	int i;

        sc->sc_pp = pciide_lookup_product(pa->pa_id);
	if (sc->sc_pp == NULL) {
		sc->sc_pp = &default_product_desc;
		pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
		printf(": %s (rev. 0x%02x)\n", devinfo,
		    PCI_REVISION(pa->pa_class));
	} else {
		printf(": %s\n", sc->sc_pp->ide_name);
	}

	if ((pa->pa_flags & PCI_FLAGS_IO_ENABLED) == 0) {
		csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		/*
		 * For a CMD PCI064x, the use of PCI_COMMAND_IO_ENABLE
		 * and base adresses registers can be disabled at
		 * hardware level. In this case, the device is wired
		 * in compat mode and its first channel is always enabled,
		 * but we can't rely on PCI_COMMAND_IO_ENABLE.
		 * In fact, it seems that the first channel of the CMD PCI0640
		 * can't be disabled.
		 */
		if ((sc->sc_pp->ide_flags & CMD_PCI064x_IOEN) == 0) {
			printf("%s: device disabled (at %s)\n",
		 	   sc->sc_wdcdev.sc_dev.dv_xname,
		  	  (csr & PCI_COMMAND_IO_ENABLE) == 0 ?
			  "device" : "bridge");
			return;
		}
	}

	class = pci_conf_read(pc, tag, PCI_CLASS_REG);
	interface = PCI_INTERFACE(class);

	/*
	 * Set up PCI interrupt only if at last one channel is in native mode.
	 * At last one device (CMD PCI0640) has a default value of 14, which
	 * will be mapped even if both channels are in compat-only mode.
	 */
	if (interface & (PCIIDE_INTERFACE_PCI(0) | PCIIDE_INTERFACE_PCI(1))) {
		if (pci_intr_map(pa->pa_pc, pa->pa_intrtag, pa->pa_intrpin,
		    pa->pa_intrline, &intrhandle) != 0) {
			printf("%s: couldn't map native-PCI interrupt\n",
			    sc->sc_wdcdev.sc_dev.dv_xname);
		} else {
			intrstr = pci_intr_string(pa->pa_pc, intrhandle);
			sc->sc_pci_ih = pci_intr_establish(pa->pa_pc,
			    intrhandle, IPL_BIO, pciide_pci_intr, sc);
			if (sc->sc_pci_ih != NULL) {
				printf("%s: using %s for native-PCI "
				    "interrupt\n",
				    sc->sc_wdcdev.sc_dev.dv_xname,
				    intrstr ? intrstr : "unknown interrupt");
			} else {
				printf("%s: couldn't establish native-PCI "
				    "interrupt",
				    sc->sc_wdcdev.sc_dev.dv_xname);
				if (intrstr != NULL)
					printf(" at %s", intrstr); 
				printf("\n");
			}
		}
	}

	/*
	 * Map DMA registers, if DMA is supported.
	 *
	 * Note that sc_dma_ok is the right variable to test to see if
	 * DMA can be done.  If the interface doesn't support DMA,
	 * sc_dma_ok will never be non-zero.  If the DMA regs couldn't
	 * be mapped, it'll be zero.  I.e., sc_dma_ok will only be
	 * non-zero if the interface supports DMA and the registers
	 * could be mapped.
	 *
	 * XXX Note that despite the fact that the Bus Master IDE specs
	 * XXX say that "The bus master IDE functoin uses 16 bytes of IO
	 * XXX space," some controllers (at least the United
	 * XXX Microelectronics UM8886BF) place it in memory space.
	 * XXX eventually, we should probably read the register and check
	 * XXX which type it is.  Either that or 'quirk' certain devices.
	 */
	if (interface & PCIIDE_INTERFACE_BUS_MASTER_DMA) {
		sc->sc_dma_ok = (pci_mapreg_map(pa,
		    PCIIDE_REG_BUS_MASTER_DMA, PCI_MAPREG_TYPE_IO, 0,
		    &sc->sc_dma_iot, &sc->sc_dma_ioh, NULL, NULL) == 0);
		sc->sc_dmat = pa->pa_dmat;
		printf("%s: bus-master DMA support present",
		    sc->sc_wdcdev.sc_dev.dv_xname);
		if (sc->sc_dma_ok == 0) {
			printf(", but unused (couldn't map registers)");
		} else {
			sc->sc_wdcdev.dma_arg = sc;
			sc->sc_wdcdev.dma_init = pciide_dma_init;
			sc->sc_wdcdev.dma_start = pciide_dma_start;
			sc->sc_wdcdev.dma_finish = pciide_dma_finish;
		}
		printf("\n");
	}
	sc->sc_pp->setup_cap(sc);
	sc->sc_wdcdev.channels = sc->wdc_channels;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;

	for (i = 0; i < PCIIDE_NUM_CHANNELS; i++) {
		cp = &sc->pciide_channels[i];

		sc->wdc_channels[i].channel = i;
		sc->wdc_channels[i].wdc = &sc->sc_wdcdev;
		if (i > 0 && (sc->sc_pp->ide_flags & ONE_QUEUE)) {
		    sc->wdc_channels[i].ch_queue =
			sc->wdc_channels[0].ch_queue;
		} else {
		    sc->wdc_channels[i].ch_queue =
		        malloc(sizeof(struct channel_queue), M_DEVBUF,
			M_NOWAIT);
		}
		if (sc->wdc_channels[i].ch_queue == NULL) {
		    printf("%s %s channel: "
			"can't allocate memory for command queue",
			sc->sc_wdcdev.sc_dev.dv_xname,
			PCIIDE_CHANNEL_NAME(i));
			continue;
		}
		printf("%s: %s channel %s to %s mode\n",
		    sc->sc_wdcdev.sc_dev.dv_xname,
		    PCIIDE_CHANNEL_NAME(i),
		    (interface & PCIIDE_INTERFACE_SETTABLE(i)) ?
		      "configured" : "wired",
		    (interface & PCIIDE_INTERFACE_PCI(i)) ? "native-PCI" :
		      "compatibility");

		/*
		 * pciide_map_channel_native() and pciide_map_channel_compat()
		 * will also call wdcattach. Eventually the channel will be
		 * disabled if there's no drive present
		 */
		if (interface & PCIIDE_INTERFACE_PCI(i))
			cp->hw_ok = pciide_map_channel_native(sc, pa, i);
		else
			cp->hw_ok = pciide_map_channel_compat(sc, pa, i);
			
	}
	sc->sc_pp->setup_chip(sc, pc, tag);
	WDCDEBUG_PRINT(("pciide: command/status register=%x\n",
	    pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG)), DEBUG_PROBE);
}

int
pciide_map_channel_compat(sc, pa, chan)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
	int chan;
{
	struct pciide_channel *cp = &sc->pciide_channels[chan];
	struct channel_softc *wdc_cp = &sc->wdc_channels[chan];
	const char *probe_fail_reason;
	int rv = 1;

	cp->compat = 1;

	wdc_cp->cmd_iot = pa->pa_iot;
	if (bus_space_map(wdc_cp->cmd_iot, PCIIDE_COMPAT_CMD_BASE(chan),
	    PCIIDE_COMPAT_CMD_SIZE, 0, &wdc_cp->cmd_ioh) != 0) {
		printf("%s: couldn't map %s channel cmd regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname,
		    PCIIDE_CHANNEL_NAME(chan));
		rv = 0;
	}

	wdc_cp->ctl_iot = pa->pa_iot;
	if (bus_space_map(wdc_cp->ctl_iot, PCIIDE_COMPAT_CTL_BASE(chan),
	    PCIIDE_COMPAT_CTL_SIZE, 0, &wdc_cp->ctl_ioh) != 0) {
		printf("%s: couldn't map %s channel ctl regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname,
		    PCIIDE_CHANNEL_NAME(chan));
		rv = 0;
	}

	/*
	 * If we weren't able to map the device successfully,
	 * we just give up now.  Something else has already
	 * occupied those ports, indicating that the device has
	 * (probably) been completely disabled (by some nonstandard
	 * mechanism).
	 *
	 * XXX If we successfully map some ports, but not others,
	 * XXX it might make sense to unmap the ones that we mapped.
	 */
	if (rv == 0)
		goto out;

	/*
	 * If we were able to map the device successfully, check if
	 * the channel is enabled. For "known" device, a chip-specific
	 * routine will be used (which read the rigth PCI register).
	 * For unknow device, a generic routine using "standart" wdc probe
	 * will try to guess it.
	 *
	 * If the channel has been disabled, other devices are free to use
	 * its ports.
	 */
	probe_fail_reason = sc->sc_pp->channel_probe(sc, pa, chan);
	if (probe_fail_reason != NULL) {
		printf("%s: %s channel ignored (%s)\n",
		    sc->sc_wdcdev.sc_dev.dv_xname,
		    PCIIDE_CHANNEL_NAME(chan), probe_fail_reason);
		rv = 0;

		bus_space_unmap(wdc_cp->cmd_iot, wdc_cp->cmd_ioh,
		    PCIIDE_COMPAT_CMD_SIZE);
		bus_space_unmap(wdc_cp->ctl_iot, wdc_cp->ctl_ioh,
		    PCIIDE_COMPAT_CTL_SIZE);

		goto out;
	}
	wdc_cp->data32iot = wdc_cp->cmd_iot;
	wdc_cp->data32ioh = wdc_cp->cmd_ioh;
	wdcattach(&sc->wdc_channels[chan]);
	/*
	 * If drive not present, try to disable the channel and
	 * free the resources.
	 */
	if ((sc->wdc_channels[chan].ch_drive[0].drive_flags & DRIVE) == 0 &&
	    (sc->wdc_channels[chan].ch_drive[1].drive_flags & DRIVE) == 0) {
		if (sc->sc_pp->channel_disable(sc, pa, chan)) {
			printf("%s: disabling %s channel (no drives)\n",
			    sc->sc_wdcdev.sc_dev.dv_xname,
			    PCIIDE_CHANNEL_NAME(chan));
			bus_space_unmap(wdc_cp->cmd_iot, wdc_cp->cmd_ioh,
			    PCIIDE_COMPAT_CMD_SIZE);
			bus_space_unmap(wdc_cp->ctl_iot, wdc_cp->ctl_ioh,
			    PCIIDE_COMPAT_CTL_SIZE);
			rv = 0;
			goto out;
		}
	}

	/*
	 * If we're here, we were able to map the device successfully
	 * and it really looks like there's a controller there.
	 *
	 * Unless those conditions are true, we don't map the
	 * compatibility interrupt.  The spec indicates that if a
	 * channel is configured for compatibility mode and the PCI
	 * device's I/O space is enabled, the channel will be enabled.
	 * Hoewver, some devices seem to be able to disable invididual
	 * compatibility channels (via non-standard mechanisms).  If
	 * the channel is disabled, the interrupt line can (probably)
	 * be used by other devices (and may be assigned to other
	 * devices by the BIOS).  If we mapped the interrupt we might
	 * conflict with another interrupt assignment.
	 */
	cp->ih = pciide_machdep_compat_intr_establish(&sc->sc_wdcdev.sc_dev,
	    pa, chan, pciide_compat_intr, wdc_cp);
	if (cp->ih == NULL) {
		printf("%s: no compatibility interrupt for use by %s channel\n",
		    sc->sc_wdcdev.sc_dev.dv_xname,
		    PCIIDE_CHANNEL_NAME(chan));
		rv = 0;
	}

out:
	return (rv);
}

int
pciide_map_channel_native(sc, pa, chan)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
	int chan;
{
	struct pciide_channel *cp = &sc->pciide_channels[chan];
	struct channel_softc *wdc_cp = &sc->wdc_channels[chan];
	int rv = 1;

	cp->compat = 0;

	if (pci_mapreg_map(pa, PCIIDE_REG_CMD_BASE(chan), PCI_MAPREG_TYPE_IO,
	    0, &wdc_cp->cmd_iot, &wdc_cp->cmd_ioh, NULL, NULL) != 0) {
		printf("%s: couldn't map %s channel cmd regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname,
		    PCIIDE_CHANNEL_NAME(chan));
		rv = 0;
	}

	if (pci_mapreg_map(pa, PCIIDE_REG_CTL_BASE(chan), PCI_MAPREG_TYPE_IO,
	    0, &wdc_cp->ctl_iot, &wdc_cp->ctl_ioh, NULL, NULL) != 0) {
		printf("%s: couldn't map %s channel ctl regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname,
		    PCIIDE_CHANNEL_NAME(chan));
		rv = 0;
	}

	if ((cp->ih = sc->sc_pci_ih) == NULL) {
		printf("%s: no native-PCI interrupt for use by %s channel\n",
		    sc->sc_wdcdev.sc_dev.dv_xname,
		    PCIIDE_CHANNEL_NAME(chan));
		rv = 0;
	}
	wdc_cp->data32iot = wdc_cp->cmd_iot;
	wdc_cp->data32ioh = wdc_cp->cmd_ioh;
	if (rv) {
		wdcattach(&sc->wdc_channels[chan]);
		/*
		 * If drive not present, try to disable the channel and
		 * free the resources.
		 */
		/* XXX How do we unmap regs mapped with pci_mapreg_map ?*/
#if 0
		if ((sc->wdc_channels[chan].ch_drive[0].drive_flags & DRIVE)
		    == 0 &&
		    (sc->wdc_channels[chan].ch_drive[1].drive_flags & DRIVE)
		    == 0) {
			if (sc->sc_pp->channel_disable(sc, pa, chan)) {
				printf("%s: disabling %s channel (no drives)\n",
				    sc->sc_wdcdev.sc_dev.dv_xname,
				    PCIIDE_CHANNEL_NAME(chan));
				pci_mapreg_map(xxx);
				rv = 0;
			}
		}
#endif
	}
	return (rv);
}

int
pciide_compat_intr(arg)
	void *arg;
{
	struct channel_softc *wdc_cp = arg;

#ifdef DIAGNOSTIC
	struct pciide_softc *sc = (struct pciide_softc*)wdc_cp->wdc;
	struct pciide_channel *cp = &sc->pciide_channels[wdc_cp->channel];
	/* should only be called for a compat channel */
	if (cp->compat == 0)
		panic("pciide compat intr called for non-compat chan %p\n", cp);
#endif
	return (wdcintr(wdc_cp));
}

int
pciide_pci_intr(arg)
	void *arg;
{
	struct pciide_softc *sc = arg;
	struct pciide_channel *cp;
	struct channel_softc *wdc_cp;
	int i, rv, crv;

	rv = 0;
	for (i = 0; i < PCIIDE_NUM_CHANNELS; i++) {
		cp = &sc->pciide_channels[i];
		wdc_cp = &sc->wdc_channels[i];

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
default_setup_cap(sc)
	struct pciide_softc *sc;
{
	if (sc->sc_dma_ok)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA;
	sc->sc_wdcdev.pio_mode = 0;
	sc->sc_wdcdev.dma_mode = 0;
}

void
default_setup_chip(sc, pc, tag)
	struct pciide_softc *sc;
	pci_chipset_tag_t pc;
	pcitag_t tag;
{
	int channel, drive, idedma_ctl;
	struct channel_softc *chp;
	struct ata_drive_datas *drvp;

	if (sc->sc_dma_ok == 0)
		return; /* nothing to do */

	/* Allocate DMA maps */
	for (channel = 0; channel < PCIIDE_NUM_CHANNELS; channel++) {
		idedma_ctl = 0;
		chp = &sc->wdc_channels[channel];
		for (drive = 0; drive < 2; drive++) {
			drvp = &chp->ch_drive[drive];
			/* If no drive, skip */
			if ((drvp->drive_flags & DRIVE) == 0)
				continue;
			if ((drvp->drive_flags & DRIVE_DMA) == 0)
				continue;
			if (pciide_dma_table_setup(sc, channel, drive) != 0) {
				/* Abort DMA setup */
				printf("%s:%d:%d: can't allocate DMA maps, "
				    "using PIO transferts\n",
				    sc->sc_wdcdev.sc_dev.dv_xname,
				    channel, drive);
				drvp->drive_flags &= ~DRIVE_DMA;
			}
			printf("%s:%d:%d: using DMA mode %d\n",
			    sc->sc_wdcdev.sc_dev.dv_xname,
			    channel, drive,
			    drvp->DMA_mode);
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		}
		if (idedma_ctl != 0) {
			/* Add software bits in status register */
			bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
			    IDEDMA_CTL + (IDEDMA_SCH_OFFSET * channel),
			    idedma_ctl);
		}
	}

}

const char *
default_channel_probe(sc, pa, chan)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	pcireg_t csr;
	const char *failreason = NULL;

	/*
	 * Check to see if something appears to be there.
	 */
	if (!wdcprobe(&sc->wdc_channels[chan])) {
		failreason = "not responding; disabled or no drives?";
		goto out;
	}

	/*
	 * Now, make sure it's actually attributable to this PCI IDE
	 * channel by trying to access the channel again while the
	 * PCI IDE controller's I/O space is disabled.  (If the
	 * channel no longer appears to be there, it belongs to
	 * this controller.)  YUCK!
	 */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    csr & ~PCI_COMMAND_IO_ENABLE);
	if (wdcprobe(&sc->wdc_channels[chan]))
		failreason = "other hardware responding at addresses";
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, csr);

out:
	return (failreason);
}

int
default_channel_disable(sc, pa, chan)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	/* don't know how to disable a channel */
	return 0;
}

void
piix_setup_cap(sc)
	struct pciide_softc *sc;
{
	if (sc->sc_pp->ide_product == PCI_PRODUCT_INTEL_82371AB_IDE)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_UDMA;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA32 | WDC_CAPABILITY_MODE |
	    WDC_CAPABILITY_DMA;
	sc->sc_wdcdev.pio_mode = 4;
	sc->sc_wdcdev.dma_mode = 2;
}

void
piix_setup_chip(sc, pc, tag)
	struct pciide_softc *sc;
	pci_chipset_tag_t pc;
	pcitag_t tag;
{
	struct channel_softc *chp;
	u_int8_t mode[2];
	u_int8_t channel, drive;
	u_int32_t oidetim, idetim, sidetim, idedma_ctl;
	struct ata_drive_datas *drvp;

	oidetim = pci_conf_read(pc, tag, PIIX_IDETIM);
	idetim = sidetim = 0;

	WDCDEBUG_PRINT(("piix_setup_chip: old idetim=0x%x, sidetim=0x%x\n",
	    oidetim,
	    pci_conf_read(pc, tag, PIIX_SIDETIM)), DEBUG_PROBE);

	for (channel = 0; channel < PCIIDE_NUM_CHANNELS; channel++) {
		chp = &sc->wdc_channels[channel];
		drvp = chp->ch_drive;
		idedma_ctl = 0;
		/* If channel disabled, no need to go further */
		if ((PIIX_IDETIM_READ(oidetim, channel) & PIIX_IDETIM_IDE) == 0)
			continue;
		/* set up new idetim: Enable IDE registers decode */
		idetim = PIIX_IDETIM_SET(idetim, PIIX_IDETIM_IDE,
		    channel);

		/* setup DMA if needed */
		for (drive = 0; drive < 2; drive++) {
			if (drvp[drive].drive_flags & DRIVE_DMA &&
			    pciide_dma_table_setup(sc, channel, drive) != 0) {
				drvp[drive].drive_flags &= ~DRIVE_DMA;
			}
		}

		/*
		 * Here we have to mess up with drives mode: PIIX can't have
		 * different timings for master and slave drives.
		 * We need to find the best combination.
		 */

		/* If both drives supports DMA, takes the lower mode */
		if ((drvp[0].drive_flags & DRIVE_DMA) &&
		    (drvp[1].drive_flags & DRIVE_DMA)) {
			mode[0] = mode[1] =
			    min(drvp[0].DMA_mode, drvp[1].DMA_mode);
			    drvp[0].DMA_mode = mode[0];
			goto ok;
		}
		/*
		 * If only one drive supports DMA, use its mode, and
		 * put the other one in PIO mode 0 if mode not compatible
		 */
		if (drvp[0].drive_flags & DRIVE_DMA) {
			mode[0] = drvp[0].DMA_mode;
			mode[1] = drvp[1].PIO_mode;
			if (piix_isp_pio[mode[1]] != piix_isp_dma[mode[0]] ||
			    piix_rtc_pio[mode[1]] != piix_rtc_dma[mode[0]])
				mode[1] = 0;
			goto ok;
		}
		if (drvp[1].drive_flags & DRIVE_DMA) {
			mode[1] = drvp[1].DMA_mode;
			mode[0] = drvp[0].PIO_mode;
			if (piix_isp_pio[mode[0]] != piix_isp_dma[mode[1]] ||
			    piix_rtc_pio[mode[0]] != piix_rtc_dma[mode[1]])
				mode[0] = 0;
			goto ok;
		}
		/*
		 * If both drives are not DMA, takes the lower mode, unless
		 * one of them is PIO mode < 2
		 */
		if (drvp[0].PIO_mode < 2) {
			mode[0] = 0;
			mode[1] = drvp[1].PIO_mode;
		} else if (drvp[1].PIO_mode < 2) {
			mode[1] = 0;
			mode[0] = drvp[0].PIO_mode;
		} else {
			mode[0] = mode[1] =
			    min(drvp[1].PIO_mode, drvp[0].PIO_mode);
		}
ok:		/* The modes are setup */
		for (drive = 0; drive < 2; drive++) {
			if (drvp[drive].drive_flags & DRIVE_DMA) {
				drvp[drive].DMA_mode = mode[drive];
				idetim |= piix_setup_idetim_timings(
				    mode[drive], 1, channel);
				goto end;
			} else
				drvp[drive].PIO_mode = mode[drive];
		}
		/* If we are there, none of the drives are DMA */
		if (mode[0] >= 2)
			idetim |= piix_setup_idetim_timings(
			    mode[0], 0, channel);
		else 
			idetim |= piix_setup_idetim_timings(
			    mode[1], 0, channel);
end:		/*
		 * timing mode is now set up in the controller. Enable
		 * it per-drive
		 */
		for (drive = 0; drive < 2; drive++) {
			/* If no drive, skip */
			if ((drvp[drive].drive_flags & DRIVE) == 0)
				continue;
			idetim |= piix_setup_idetim_drvs(&drvp[drive]);
			printf("%s(%s:%d:%d): using PIO mode %d",
			    drvp[drive].drv_softc->dv_xname,
			    sc->sc_wdcdev.sc_dev.dv_xname,
			    channel, drive, drvp[drive].PIO_mode);
			if (drvp[drive].drive_flags & DRIVE_DMA) {
				idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
				printf(", DMA mode %d", drvp[drive].DMA_mode);
			}
			printf("\n");
		}
		if (idedma_ctl != 0) {
			/* Add software bits in status register */
			bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
			    IDEDMA_CTL + (IDEDMA_SCH_OFFSET * channel),
			    idedma_ctl);
		}
	}
	WDCDEBUG_PRINT(("piix_setup_chip: idetim=0x%x, sidetim=0x%x\n",
	    idetim, sidetim), DEBUG_PROBE);
	pci_conf_write(pc, tag, PIIX_IDETIM, idetim);
	pci_conf_write(pc, tag, PIIX_SIDETIM, sidetim);
}

void
piix3_4_setup_chip(sc, pc, tag)
	struct pciide_softc *sc;
	pci_chipset_tag_t pc;
	pcitag_t tag;
{
	int channel, drive;
	struct channel_softc *chp;
	struct ata_drive_datas *drvp;
	u_int32_t oidetim, idetim, sidetim, udmareg, idedma_ctl;

	idetim = sidetim = udmareg = 0;
	oidetim = pci_conf_read(pc, tag, PIIX_IDETIM);

	WDCDEBUG_PRINT(("piix3_4_setup_chip: old idetim=0x%x, sidetim=0x%x",
	    oidetim,
	    pci_conf_read(pc, tag, PIIX_SIDETIM)), DEBUG_PROBE);
	if (sc->sc_wdcdev.cap & WDC_CAPABILITY_UDMA) {
		WDCDEBUG_PRINT((", udamreg 0x%x",
		    pci_conf_read(pc, tag, PIIX_UDMAREG)),
		    DEBUG_PROBE);
	}
	WDCDEBUG_PRINT(("\n"), DEBUG_PROBE);

	for (channel = 0; channel < PCIIDE_NUM_CHANNELS; channel++) {
		chp = &sc->wdc_channels[channel];
		idedma_ctl = 0;
		/* If channel disabled, no need to go further */
		if ((PIIX_IDETIM_READ(oidetim, channel) & PIIX_IDETIM_IDE) == 0)
			continue;
		/* set up new idetim: Enable IDE registers decode */
		idetim = PIIX_IDETIM_SET(idetim, PIIX_IDETIM_IDE,
		    channel);
		for (drive = 0; drive < 2; drive++) {
			drvp = &chp->ch_drive[drive];
			/* If no drive, skip */
			if ((drvp->drive_flags & DRIVE) == 0)
				continue;
			/* add timing values, setup DMA if needed */
			if (((drvp->drive_flags & DRIVE_DMA) == 0 &&
			    (drvp->drive_flags & DRIVE_UDMA) == 0) ||
			    sc->sc_dma_ok == 0) {
				drvp->drive_flags &= ~(DRIVE_DMA | DRIVE_UDMA);
				goto pio;
			}
			if (pciide_dma_table_setup(sc, channel, drive) != 0) {
				/* Abort DMA setup */
				drvp->drive_flags &= ~(DRIVE_DMA | DRIVE_UDMA);
				goto pio;
			}
			if ((chp->wdc->cap & WDC_CAPABILITY_UDMA) &&
			    (drvp->drive_flags & DRIVE_UDMA)) {
				/* use Ultra/DMA */
				drvp->drive_flags &= ~DRIVE_DMA;
				udmareg |= PIIX_UDMACTL_DRV_EN(
				    channel, drive);
				udmareg |= PIIX_UDMATIM_SET(
				    piix4_sct_udma[drvp->UDMA_mode],
				    channel, drive);
			} else {
				/* use Multiword DMA */
				drvp->drive_flags &= ~DRIVE_UDMA;
				if (drive == 0) {
					idetim |= piix_setup_idetim_timings(
					    drvp->DMA_mode, 1, channel);
				} else {
					sidetim |= piix_setup_sidetim_timings(
						drvp->DMA_mode, 1, channel);
					idetim =PIIX_IDETIM_SET(idetim,
					    PIIX_IDETIM_SITRE, channel);
				}
			}
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		
pio:			/* use PIO mode */
			idetim |= piix_setup_idetim_drvs(drvp);
			if (drive == 0) {
				idetim |= piix_setup_idetim_timings(
				    drvp->PIO_mode, 0, channel);
			} else {
				sidetim |= piix_setup_sidetim_timings(
					drvp->PIO_mode, 0, channel);
				idetim =PIIX_IDETIM_SET(idetim,
				    PIIX_IDETIM_SITRE, channel);
			}
			printf("%s(%s:%d:%d): using PIO mode %d",
			    drvp->drv_softc->dv_xname,
			    sc->sc_wdcdev.sc_dev.dv_xname,
			    channel, drive, drvp->PIO_mode);
			if (drvp->drive_flags & DRIVE_DMA)
			    printf(", DMA mode %d", drvp->DMA_mode);
			if (drvp->drive_flags & DRIVE_UDMA)
			    printf(", UDMA mode %d", drvp->UDMA_mode);
			printf("\n");
		}
		if (idedma_ctl != 0) {
			/* Add software bits in status register */
			bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
			    IDEDMA_CTL + (IDEDMA_SCH_OFFSET * channel),
			    idedma_ctl);
		}
	}

	WDCDEBUG_PRINT(("piix3_4_setup_chip: idetim=0x%x, sidetim=0x%x",
	    idetim, sidetim), DEBUG_PROBE);
	if (sc->sc_wdcdev.cap & WDC_CAPABILITY_UDMA) {
		WDCDEBUG_PRINT((", udmareg=0x%x", udmareg), DEBUG_PROBE);
		pci_conf_write(pc, tag, PIIX_UDMAREG, udmareg);
	}
	WDCDEBUG_PRINT(("\n"), DEBUG_PROBE);
	pci_conf_write(pc, tag, PIIX_IDETIM, idetim);
	pci_conf_write(pc, tag, PIIX_SIDETIM, sidetim);
}

/* setup ISP and RTC fields, based on mode */
static u_int32_t
piix_setup_idetim_timings(mode, dma, channel)
	u_int8_t mode;
	u_int8_t dma;
	u_int8_t channel;
{
	
	if (dma)
		return PIIX_IDETIM_SET(0,
		    PIIX_IDETIM_ISP_SET(piix_isp_dma[mode]) | 
		    PIIX_IDETIM_RTC_SET(piix_rtc_dma[mode]),
		    channel);
	else 
		return PIIX_IDETIM_SET(0,
		    PIIX_IDETIM_ISP_SET(piix_isp_pio[mode]) | 
		    PIIX_IDETIM_RTC_SET(piix_rtc_pio[mode]),
		    channel);
}

/* setup DTE, PPE, IE and TIME field based on PIO mode */
static u_int32_t
piix_setup_idetim_drvs(drvp)
	struct ata_drive_datas *drvp;
{
	u_int32_t ret = 0;
	struct channel_softc *chp = drvp->chnl_softc;
	u_int8_t channel = chp->channel;
	u_int8_t drive = drvp->drive;

	/*
	 * If drive is using UDMA, timings setups are independant
	 * So just check DMA and PIO here.
	 */
	if (drvp->drive_flags & DRIVE_DMA) {
		/* if mode = DMA mode 0, use compatible timings */
		if ((drvp->drive_flags & DRIVE_DMA) &&
		    drvp->DMA_mode == 0) {
			drvp->PIO_mode = 0;
			return ret;
		}
		ret = PIIX_IDETIM_SET(ret, PIIX_IDETIM_TIME(drive), channel);
		/*
		 * PIO and DMA timings are the same, use fast timings for PIO
		 * too, else use compat timings.
		 */
		if ((piix_isp_pio[drvp->PIO_mode] !=
		    piix_isp_dma[drvp->DMA_mode]) ||
		    (piix_rtc_pio[drvp->PIO_mode] !=
		    piix_rtc_dma[drvp->DMA_mode]))
			drvp->PIO_mode = 0;
		/* if PIO mode <= 2, use compat timings for PIO */
		if (drvp->PIO_mode <= 2) {
			ret = PIIX_IDETIM_SET(ret, PIIX_IDETIM_DTE(drive),
			    channel);
			return ret;
		}
	}

	/*
	 * Now setup PIO modes. If mode < 2, use compat timings.
	 * Else enable fast timings. Enable IORDY and prefetch/post
	 * if PIO mode >= 3.
	 */

	if (drvp->PIO_mode < 2)
		return ret;

	ret = PIIX_IDETIM_SET(ret, PIIX_IDETIM_TIME(drive), channel);
	if (drvp->PIO_mode >= 3) {
		ret = PIIX_IDETIM_SET(ret, PIIX_IDETIM_IE(drive), channel);
		ret = PIIX_IDETIM_SET(ret, PIIX_IDETIM_PPE(drive), channel);
	}
	return ret;
}

/* setup values in SIDETIM registers, based on mode */
static u_int32_t
piix_setup_sidetim_timings(mode, dma, channel)
	u_int8_t mode;
	u_int8_t dma;
	u_int8_t channel;
{
	if (dma)
		return PIIX_SIDETIM_ISP_SET(piix_isp_dma[mode], channel) |
		    PIIX_SIDETIM_RTC_SET(piix_rtc_dma[mode], channel);
	else 
		return PIIX_SIDETIM_ISP_SET(piix_isp_pio[mode], channel) |
		    PIIX_SIDETIM_RTC_SET(piix_rtc_pio[mode], channel);
}

const char*
piix_channel_probe(sc, pa, chan)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
	int chan;
{
	u_int32_t idetim = pci_conf_read(pa->pa_pc, pa->pa_tag, PIIX_IDETIM);

	if (PIIX_IDETIM_READ(idetim, chan) & PIIX_IDETIM_IDE)
		return NULL;
	else
		return "disabled";
}

int
piix_channel_disable(sc, pa, chan)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	u_int32_t idetim = pci_conf_read(pa->pa_pc, pa->pa_tag, PIIX_IDETIM);
	idetim = PIIX_IDETIM_CLEAR(idetim, PIIX_IDETIM_IDE, chan);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PIIX_IDETIM, idetim);
	return 1;
}

void
apollo_setup_cap(sc)
	struct pciide_softc *sc;
{
	if (sc->sc_pp->ide_product == PCI_PRODUCT_VIATECH_VT82C586_IDE)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_UDMA;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA32 | WDC_CAPABILITY_MODE |
	    WDC_CAPABILITY_DMA;
	sc->sc_wdcdev.pio_mode = 4;
	sc->sc_wdcdev.dma_mode = 2;

}
void
apollo_setup_chip(sc, pc, tag)
	struct pciide_softc *sc;
	pci_chipset_tag_t pc;
	pcitag_t tag;
{
	u_int32_t udmatim_reg, datatim_reg;
	u_int8_t idedma_ctl;
	int mode;
	int channel, drive;
	struct channel_softc *chp;
	struct ata_drive_datas *drvp;

	WDCDEBUG_PRINT(("apollo_setup_chip: old APO_IDECONF=0x%x, "
	    "APO_CTLMISC=0x%x, APO_DATATIM=0x%x, APO_UDMA=0x%x\n",
	    pci_conf_read(pc, tag, APO_IDECONF),
	    pci_conf_read(pc, tag, APO_CTLMISC),
	    pci_conf_read(pc, tag, APO_DATATIM),
	    pci_conf_read(pc, tag, APO_UDMA)),
	    DEBUG_PROBE);

	datatim_reg = 0;
	udmatim_reg = 0;
	for (channel = 0; channel < PCIIDE_NUM_CHANNELS; channel++) {
		chp = &sc->wdc_channels[channel];
		idedma_ctl = 0;
		for (drive = 0; drive < 2; drive++) {
			drvp = &chp->ch_drive[drive];
			/* If no drive, skip */
			if ((drvp->drive_flags & DRIVE) == 0)
				continue;
			/* add timing values, setup DMA if needed */
			if (((drvp->drive_flags & DRIVE_DMA) == 0 &&
			    (drvp->drive_flags & DRIVE_UDMA) == 0) ||
			    sc->sc_dma_ok == 0) {
				drvp->drive_flags &= ~(DRIVE_DMA | DRIVE_UDMA);
				mode = drvp->PIO_mode;
				goto pio;
			}
			if (pciide_dma_table_setup(sc, channel, drive) != 0) {
				/* Abort DMA setup */
				drvp->drive_flags &= ~(DRIVE_DMA | DRIVE_UDMA);
				mode = drvp->PIO_mode;
				goto pio;
			}
			if ((chp->wdc->cap & WDC_CAPABILITY_UDMA) &&
			    (drvp->drive_flags & DRIVE_UDMA)) {
				/* use Ultra/DMA */
				drvp->drive_flags &= ~DRIVE_DMA;
				udmatim_reg |= APO_UDMA_EN(channel, drive) |
				    APO_UDMA_EN_MTH(channel, drive) |
				    APO_UDMA_TIME(channel, drive,
					apollo_udma_tim[drvp->UDMA_mode]);
				/* can use PIO timings, MW DMA unused */
				mode = drvp->PIO_mode;
			} else {
				/* use Multiword DMA */
				drvp->drive_flags &= ~DRIVE_UDMA;
				/* mode = min(pio, dma+2) */
				if (drvp->PIO_mode <= (drvp->DMA_mode +2))
					mode = drvp->PIO_mode;
				else
					mode = drvp->DMA_mode;
			}
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);

pio:			/* setup PIO mode */
			datatim_reg |=
			    APO_DATATIM_PULSE(channel, drive,
				apollo_pio_set[mode]) |
			    APO_DATATIM_RECOV(channel, drive,
				apollo_pio_rec[mode]);
			drvp->PIO_mode = mode;
			drvp->DMA_mode = mode + 2;
			printf("%s(%s:%d:%d): using PIO mode %d",
			    drvp->drv_softc->dv_xname,
			    sc->sc_wdcdev.sc_dev.dv_xname,
			    channel, drive, drvp->PIO_mode);
			if (drvp[drive].drive_flags & DRIVE_DMA)
			    printf(", DMA mode %d", drvp->DMA_mode);
			if (drvp->drive_flags & DRIVE_UDMA)
			    printf(", UDMA mode %d", drvp->UDMA_mode);
			printf("\n");
		}
		if (idedma_ctl != 0) {
			/* Add software bits in status register */
			bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
			    IDEDMA_CTL + (IDEDMA_SCH_OFFSET * channel),
			    idedma_ctl);
		}
	}
	WDCDEBUG_PRINT(("apollo_setup_chip: APO_DATATIM=0x%x, APO_UDMA=0x%x\n",
	    datatim_reg, udmatim_reg), DEBUG_PROBE);
	pci_conf_write(pc, tag, APO_DATATIM, datatim_reg);
	pci_conf_write(pc, tag, APO_UDMA, udmatim_reg);
}

const char*
apollo_channel_probe(sc, pa, chan)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
	int chan;
{

	u_int32_t ideconf = pci_conf_read(pa->pa_pc, pa->pa_tag, APO_IDECONF);

	if (ideconf & APO_IDECONF_EN(chan))
		return NULL;
	else
		return "disabled";
	
}

int
apollo_channel_disable(sc, pa, chan)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	u_int32_t ideconf = pci_conf_read(pa->pa_pc, pa->pa_tag, APO_IDECONF);
	ideconf &= ~APO_IDECONF_EN(chan);
	pci_conf_write(pa->pa_pc, pa->pa_tag, APO_IDECONF, ideconf);
	return 1;
}

const char*
cmd_channel_probe(sc, pa, chan)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
	int chan;
{

	/*
	 * with a CMD PCI64x, if we get here, the first channel is enabled:
	 * there's no way to disable the first channel without disabling
	 * the whole device
	 */
	if (chan == 0)
		return NULL;

	/* Second channel is enabled if CMD_CONF_2PORT is set */
	if ((pci_conf_read(pa->pa_pc, pa->pa_tag, CMD_CONF_CTRL0) &
	    CMD_CONF_2PORT) == 0)
		return "disabled";

	return NULL;
}

int
cmd_channel_disable(sc, pa, chan)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	u_int32_t ctrl0;
	/* with a CMD PCI64x, the first channel is always enabled */
	if (chan == 0)
		return 0;
	ctrl0 = pci_conf_read(pa->pa_pc, pa->pa_tag, CMD_CONF_CTRL0);
	ctrl0 &= ~CMD_CONF_2PORT;
	pci_conf_write(pa->pa_pc, pa->pa_tag, CMD_CONF_CTRL0, ctrl0);
	return 1;
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

	/* Allocate memory for the DMA tables and map it */
	if ((error = bus_dmamem_alloc(sc->sc_dmat, dma_table_size,
	    IDEDMA_TBL_ALIGN, IDEDMA_TBL_ALIGN, &seg, 1, &rseg,
	    BUS_DMA_NOWAIT)) != 0) {
		printf("%s:%d: unable to allocate table DMA for "
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}
	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    dma_table_size,
	    (caddr_t *)&dma_maps->dma_table,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s:%d: unable to map table DMA for"
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}
	WDCDEBUG_PRINT(("pciide_dma_table_setup: table at %p len %ld, "
	    "phy 0x%lx\n", dma_maps->dma_table, dma_table_size,
	    seg.ds_addr), DEBUG_PROBE);

	/* Create and load table DMA map for this disk */
	if ((error = bus_dmamap_create(sc->sc_dmat, dma_table_size,
	    1, dma_table_size, IDEDMA_TBL_ALIGN, BUS_DMA_NOWAIT,
	    &dma_maps->dmamap_table)) != 0) {
		printf("%s:%d: unable to create table DMA map for "
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}
	if ((error = bus_dmamap_load(sc->sc_dmat,
	    dma_maps->dmamap_table,
	    dma_maps->dma_table,
	    dma_table_size, NULL, BUS_DMA_NOWAIT)) != 0) {
		printf("%s:%d: unable to load table DMA map for "
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}
	WDCDEBUG_PRINT(("pciide_dma_table_setup: phy addr of table 0x%lx\n",
	    dma_maps->dmamap_table->dm_segs[0].ds_addr), DEBUG_PROBE);
	/* Create a xfer DMA map for this drive */
	if ((error = bus_dmamap_create(sc->sc_dmat, IDEDMA_BYTE_COUNT_MAX,
	    NIDEDMA_TABLES, IDEDMA_BYTE_COUNT_MAX, IDEDMA_BYTE_COUNT_ALIGN,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
	    &dma_maps->dmamap_xfer)) != 0) {
		printf("%s:%d: unable to create xfer DMA map for "
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}
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
	int error, seg;
	struct pciide_dma_maps *dma_maps =
	    &sc->pciide_channels[channel].dma_maps[drive];

	error = bus_dmamap_load(sc->sc_dmat,
	    dma_maps->dmamap_xfer,
	    databuf, datalen, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s:%d: unable to load xfer DMA map for"
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}

	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_xfer, 0,
	    dma_maps->dmamap_xfer->dm_mapsize,
	    (flags & WDC_DMA_READ) ?
	    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	WDCDEBUG_PRINT(("pciide_dma_init: %d segs for %p len %d (phy 0x%x)\n",
	    dma_maps->dmamap_xfer->dm_nsegs, databuf, datalen,
	    vtophys(databuf)), DEBUG_DMA|DEBUG_XFERS);
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
		    dma_maps->dmamap_xfer->dm_segs[seg].ds_addr;
		dma_maps->dma_table[seg].byte_count =
		    dma_maps->dmamap_xfer->dm_segs[seg].ds_len &
		    IDEDMA_BYTE_COUNT_MASK;
		WDCDEBUG_PRINT(("\t seg %d len %d addr 0x%x\n",
		   seg, dma_maps->dma_table[seg].byte_count,
		   dma_maps->dma_table[seg].base_addr), DEBUG_DMA);

	}
	dma_maps->dma_table[dma_maps->dmamap_xfer->dm_nsegs -1].byte_count |=
		IDEDMA_BYTE_COUNT_EOT;

	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_table, 0,
	    dma_maps->dmamap_table->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	/* Maps are ready. Start DMA function */
#ifdef DIAGNOSTIC
	if (dma_maps->dmamap_table->dm_segs[0].ds_addr & ~IDEDMA_TBL_MASK) {
		printf("pciide_dma_init: addr 0x%lx not properly aligned\n",
		    dma_maps->dmamap_table->dm_segs[0].ds_addr);
		panic("pciide_dma_init: table align");
	}
#endif

	WDCDEBUG_PRINT(("phy addr of table at %p = 0x%lx len %ld (%d segs, "
	    "phys 0x%x)\n",
	    dma_maps->dma_table,
	    dma_maps->dmamap_table->dm_segs[0].ds_addr,
	    dma_maps->dmamap_table->dm_segs[0].ds_len,
	    dma_maps->dmamap_table->dm_nsegs,
	    vtophys(dma_maps->dma_table)), DEBUG_DMA);
	/* Clear status bits */
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CTL + IDEDMA_SCH_OFFSET * channel,
	    bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		IDEDMA_CTL + IDEDMA_SCH_OFFSET * channel));
	/* Write table addr */
	bus_space_write_4(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_TBL + IDEDMA_SCH_OFFSET * channel,
	    dma_maps->dmamap_table->dm_segs[0].ds_addr);
	/* set read/write */
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CMD + IDEDMA_SCH_OFFSET * channel,
	    (flags & WDC_DMA_READ) ? IDEDMA_CMD_WRITE: 0);
	return 0;
}

void
pciide_dma_start(v, channel, drive, flags)
	void *v;
	int channel, drive, flags;
{
	struct pciide_softc *sc = v;

	WDCDEBUG_PRINT(("pciide_dma_start\n"),DEBUG_XFERS);
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CMD + IDEDMA_SCH_OFFSET * channel,
	    bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		IDEDMA_CMD + IDEDMA_SCH_OFFSET * channel) | IDEDMA_CMD_START);
}

int
pciide_dma_finish(v, channel, drive, flags)
	void *v;
	int channel, drive;
	int flags;
{
	struct pciide_softc *sc = v;
	u_int8_t status;
	struct pciide_dma_maps *dma_maps =
	    &sc->pciide_channels[channel].dma_maps[drive];

	/* Unload the map of the data buffer */
	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_xfer, 0,
	    dma_maps->dmamap_xfer->dm_mapsize,
	    (flags & WDC_DMA_READ) ?
	    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, dma_maps->dmamap_xfer);

	status = bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CTL + IDEDMA_SCH_OFFSET * channel);
	WDCDEBUG_PRINT(("pciide_dma_finish: status 0x%x\n", status),
	    DEBUG_XFERS);

	/* stop DMA channel */
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CMD + IDEDMA_SCH_OFFSET * channel,
	    bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		IDEDMA_CMD + IDEDMA_SCH_OFFSET * channel) & ~IDEDMA_CMD_START);

	/* Clear status bits */
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CTL + IDEDMA_SCH_OFFSET * channel,
	    status);

	if ((status & IDEDMA_CTL_ERR) != 0) {
		printf("%s:%d:%d: Bus-Master DMA error: status=0x%x\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, channel, drive, status);
		return -1;
	}

	if ((flags & WDC_DMA_POLL) == 0 && (status & IDEDMA_CTL_INTR) == 0) {
		printf("%s:%d:%d: Bus-Master DMA error: missing interrupt, "
		    "status=0x%x\n", sc->sc_wdcdev.sc_dev.dv_xname, channel,
		    drive, status);
		return -1;
	}

	if ((status & IDEDMA_CTL_ACT) != 0) {
		/* data underrun, may be a valid condition for ATAPI */
		return 1;
	}

	return 0;
}
