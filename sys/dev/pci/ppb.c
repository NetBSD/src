/*	$NetBSD: ppb.c,v 1.63.2.1 2019/02/01 11:25:13 martin Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ppb.c,v 1.63.2.1 2019/02/01 11:25:13 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/evcnt.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/ppbreg.h>
#include <dev/pci/ppbvar.h>
#include <dev/pci/pcidevs.h>

#define	PCIE_SLCSR_ENABLE_MASK					\
	(PCIE_SLCSR_ABE | PCIE_SLCSR_PFE | PCIE_SLCSR_MSE |	\
	 PCIE_SLCSR_PDE | PCIE_SLCSR_CCE | PCIE_SLCSR_HPE |	\
	 PCIE_SLCSR_DLLSCE)

#define	PCIE_SLCSR_STATCHG_MASK					\
	(PCIE_SLCSR_ABP | PCIE_SLCSR_PFD | PCIE_SLCSR_MSC |	\
	 PCIE_SLCSR_PDC | PCIE_SLCSR_CC | PCIE_SLCSR_LACS)

static const char pcie_linkspeed_strings[4][5] = {
	"1.25", "2.5", "5.0", "8.0",
};

int	ppb_printevent = 0; /* Print event type if the value is not 0 */

static int	ppbmatch(device_t, cfdata_t, void *);
static void	ppbattach(device_t, device_t, void *);
static int	ppbdetach(device_t, int);
static void	ppbchilddet(device_t, device_t);
#ifdef PPB_USEINTR
static int	ppb_intr(void *);
#endif
static bool	ppb_resume(device_t, const pmf_qual_t *);
static bool	ppb_suspend(device_t, const pmf_qual_t *);

CFATTACH_DECL3_NEW(ppb, sizeof(struct ppb_softc),
    ppbmatch, ppbattach, ppbdetach, NULL, NULL, ppbchilddet,
    DVF_DETACH_SHUTDOWN);

static int
ppbmatch(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	/*
	 * Check the ID register to see that it's a PCI bridge.
	 * If it is, we assume that we can deal with it; it _should_
	 * work in a standardized way...
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_PCI)
		return 1;

#ifdef __powerpc__
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_PROCESSOR &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_PROCESSOR_POWERPC) {
		pcireg_t bhlc = pci_conf_read(pa->pa_pc, pa->pa_tag,
		    PCI_BHLC_REG);
		if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_FREESCALE
		    && PCI_HDRTYPE(bhlc) == PCI_HDRTYPE_RC)
		return 1;
	}
#endif

#ifdef _MIPS_PADDR_T_64BIT
	/* The LDT HB acts just like a PPB.  */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SIBYTE
	    && PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SIBYTE_BCM1250_LDTHB)
		return 1;
#endif

	return 0;
}

static void
ppb_print_pcie(device_t self)
{
	struct ppb_softc *sc = device_private(self);
	pcireg_t reg;
	int off, capversion, devtype;

	if (!pci_get_capability(sc->sc_pc, sc->sc_tag, PCI_CAP_PCIEXPRESS,
				&off, &reg))
		return; /* Not a PCIe device */

	capversion = PCIE_XCAP_VER(reg);
	devtype = PCIE_XCAP_TYPE(reg);
	aprint_normal_dev(self, "PCI Express capability version ");
	switch (capversion) {
	case PCIE_XCAP_VER_1:
		aprint_normal("1");
		break;
	case PCIE_XCAP_VER_2:
		aprint_normal("2");
		break;
	default:
		aprint_normal_dev(self, "unsupported (%d)\n", capversion);
		return;
	}
	aprint_normal(" <");
	switch (devtype) {
	case PCIE_XCAP_TYPE_PCIE_DEV:
		aprint_normal("PCI-E Endpoint device");
		break;
	case PCIE_XCAP_TYPE_PCI_DEV:
		aprint_normal("Legacy PCI-E Endpoint device");
		break;
	case PCIE_XCAP_TYPE_ROOT:
		aprint_normal("Root Port of PCI-E Root Complex");
		break;
	case PCIE_XCAP_TYPE_UP:
		aprint_normal("Upstream Port of PCI-E Switch");
		break;
	case PCIE_XCAP_TYPE_DOWN:
		aprint_normal("Downstream Port of PCI-E Switch");
		break;
	case PCIE_XCAP_TYPE_PCIE2PCI:
		aprint_normal("PCI-E to PCI/PCI-X Bridge");
		break;
	case PCIE_XCAP_TYPE_PCI2PCIE:
		aprint_normal("PCI/PCI-X to PCI-E Bridge");
		break;
	default:
		aprint_normal("Device/Port Type %x", devtype);
		break;
	}

	switch (devtype) {
	case PCIE_XCAP_TYPE_ROOT:
	case PCIE_XCAP_TYPE_DOWN:
	case PCIE_XCAP_TYPE_PCI2PCIE:
		reg = pci_conf_read(sc->sc_pc, sc->sc_tag, off + PCIE_LCAP);
		u_int mlw = __SHIFTOUT(reg, PCIE_LCAP_MAX_WIDTH);
		u_int mls = __SHIFTOUT(reg, PCIE_LCAP_MAX_SPEED);

		if (mls < __arraycount(pcie_linkspeed_strings)) {
			aprint_normal("> x%d @ %sGT/s\n",
			    mlw, pcie_linkspeed_strings[mls]);
		} else {
			aprint_normal("> x%d @ %d.%dGT/s\n",
			    mlw, (mls * 25) / 10, (mls * 25) % 10);
		}

		reg = pci_conf_read(sc->sc_pc, sc->sc_tag, off + PCIE_LCSR);
		if (reg & PCIE_LCSR_DLACTIVE) {	/* DLLA */
			u_int lw = __SHIFTOUT(reg, PCIE_LCSR_NLW);
			u_int ls = __SHIFTOUT(reg, PCIE_LCSR_LINKSPEED);

			if (lw != mlw || ls != mls) {
				if (ls < __arraycount(pcie_linkspeed_strings)) {
					aprint_normal_dev(self,
					    "link is x%d @ %sGT/s\n",
					    lw, pcie_linkspeed_strings[ls]);
				} else {
					aprint_normal_dev(self,
					    "link is x%d @ %d.%dGT/s\n",
					    lw, (ls * 25) / 10, (ls * 25) % 10);
				}
			}
		}
		break;
	default:
		aprint_normal(">\n");
		break;
	}
}

static void
ppbattach(device_t parent, device_t self, void *aux)
{
	struct ppb_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	struct pcibus_attach_args pba;
#ifdef PPB_USEINTR
	char const *intrstr;
	char intrbuf[PCI_INTRSTR_LEN];
#endif
	pcireg_t busdata, reg;
	bool second_configured = false;

	pci_aprint_devinfo(pa, NULL);

	sc->sc_pc = pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_dev = self;

	busdata = pci_conf_read(pc, pa->pa_tag, PPB_REG_BUSINFO);

	if (PPB_BUSINFO_SECONDARY(busdata) == 0) {
		aprint_normal_dev(self, "not configured by system firmware\n");
		return;
	}

	ppb_print_pcie(self);

#if 0
	/*
	 * XXX can't do this, because we're not given our bus number
	 * (we shouldn't need it), and because we've no way to
	 * decompose our tag.
	 */
	/* sanity check. */
	if (pa->pa_bus != PPB_BUSINFO_PRIMARY(busdata))
		panic("ppbattach: bus in tag (%d) != bus in reg (%d)",
		    pa->pa_bus, PPB_BUSINFO_PRIMARY(busdata));
#endif

	/* Check for PCI Express capabilities and setup hotplug support. */
	if (pci_get_capability(pc, pa->pa_tag, PCI_CAP_PCIEXPRESS,
	    &sc->sc_pciecapoff, &reg) && (reg & PCIE_XCAP_SI)) {
		/*
		 * First, disable all interrupts because BIOS might
		 * enable them.
		 */
		reg = pci_conf_read(sc->sc_pc, sc->sc_tag,
		    sc->sc_pciecapoff + PCIE_SLCSR);
		if (reg & PCIE_SLCSR_ENABLE_MASK) {
			reg &= ~PCIE_SLCSR_ENABLE_MASK;
			pci_conf_write(sc->sc_pc, sc->sc_tag,
			    sc->sc_pciecapoff + PCIE_SLCSR, reg);
		}
#ifdef PPB_USEINTR
#if 0 /* notyet */
		/*
		 * XXX Initialize workqueue or something else for
		 * HotPlug support.
		 */
#endif	
		if (pci_intr_alloc(pa, &sc->sc_pihp, NULL, 0) == 0)
			sc->sc_intrhand = pci_intr_establish_xname(pc,
			    sc->sc_pihp[0], IPL_BIO, ppb_intr, sc,
			    device_xname(sc->sc_dev));
#endif
	}

#ifdef PPB_USEINTR
	if (sc->sc_intrhand != NULL) {
		pcireg_t slcap, slcsr, val;

		intrstr = pci_intr_string(pc, sc->sc_pihp[0], intrbuf,
		    sizeof(intrbuf));
		aprint_normal_dev(self, "%s\n", intrstr);

		/* Clear any pending events */
		slcsr = pci_conf_read(pc, pa->pa_tag,
		    sc->sc_pciecapoff + PCIE_SLCSR);
		pci_conf_write(pc, pa->pa_tag,
		    sc->sc_pciecapoff + PCIE_SLCSR, slcsr);

		/* Enable interrupt. */
		val = 0;
		slcap = pci_conf_read(pc, pa->pa_tag,
		    sc->sc_pciecapoff + PCIE_SLCAP);
		if (slcap & PCIE_SLCAP_ABP)
			val |= PCIE_SLCSR_ABE;
		if (slcap & PCIE_SLCAP_PCP)
			val |= PCIE_SLCSR_PFE;
		if (slcap & PCIE_SLCAP_MSP)
			val |= PCIE_SLCSR_MSE;
#if 0
		/*
		 * XXX Disable for a while because setting
		 * PCIE_SLCSR_CCE makes break device access on
		 * some environment.
		 */
		if ((slcap & PCIE_SLCAP_NCCS) == 0)
			val |= PCIE_SLCSR_CCE;
#endif
		/* Attention indicator off by default */
		if (slcap & PCIE_SLCAP_AIP) {
			val |= __SHIFTIN(PCIE_SLCSR_IND_OFF,
			    PCIE_SLCSR_AIC);
		}
		/* Power indicator */
		if (slcap & PCIE_SLCAP_PIP) {
			/*
			 * Indicator off:
			 *  a) card not present
			 *  b) power fault
			 *  c) MRL sensor off
			 */
			if (((slcsr & PCIE_SLCSR_PDS) == 0)
			    || ((slcsr & PCIE_SLCSR_PFD) != 0)
			    || (((slcap & PCIE_SLCAP_MSP) != 0)
				&& ((slcsr & PCIE_SLCSR_MS) != 0)))
				val |= __SHIFTIN(PCIE_SLCSR_IND_OFF,
				    PCIE_SLCSR_PIC);
			else
				val |= __SHIFTIN(PCIE_SLCSR_IND_ON,
				    PCIE_SLCSR_PIC);
		}

		val |= PCIE_SLCSR_DLLSCE | PCIE_SLCSR_HPE | PCIE_SLCSR_PDE;
		slcsr = val;
		pci_conf_write(pc, pa->pa_tag,
		    sc->sc_pciecapoff + PCIE_SLCSR, slcsr);

		/* Attach event counters */
		evcnt_attach_dynamic(&sc->sc_ev_intr, EVCNT_TYPE_INTR, NULL,
		    device_xname(sc->sc_dev), "Interrupt");
		evcnt_attach_dynamic(&sc->sc_ev_abp, EVCNT_TYPE_MISC, NULL,
		    device_xname(sc->sc_dev), "Attention Button Pressed");
		evcnt_attach_dynamic(&sc->sc_ev_pfd, EVCNT_TYPE_MISC, NULL,
		    device_xname(sc->sc_dev), "Power Fault Detected");
		evcnt_attach_dynamic(&sc->sc_ev_msc, EVCNT_TYPE_MISC, NULL,
		    device_xname(sc->sc_dev), "MRL Sensor Changed");
		evcnt_attach_dynamic(&sc->sc_ev_pdc, EVCNT_TYPE_MISC, NULL,
		    device_xname(sc->sc_dev), "Presence Detect Changed");
		evcnt_attach_dynamic(&sc->sc_ev_cc, EVCNT_TYPE_MISC, NULL,
		    device_xname(sc->sc_dev), "Command Completed");
		evcnt_attach_dynamic(&sc->sc_ev_lacs, EVCNT_TYPE_MISC, NULL,
		    device_xname(sc->sc_dev), "Data Link Layer State Changed");
	}
#endif /* PPB_USEINTR */

	/* Configuration test */
	if (PPB_BUSINFO_SECONDARY(busdata) != 0) {
		uint32_t base, limit;

		/* I/O region test */
		reg = pci_conf_read(pc, pa->pa_tag, PCI_BRIDGE_STATIO_REG);
		base = (reg & PCI_BRIDGE_STATIO_IOBASE_MASK) << 8;
		limit = ((reg >> PCI_BRIDGE_STATIO_IOLIMIT_SHIFT)
		    & PCI_BRIDGE_STATIO_IOLIMIT_MASK) << 8;
		limit |= 0x00000fff;
		if (PCI_BRIDGE_IO_32BITS(reg)) {
			reg = pci_conf_read(pc, pa->pa_tag,
			    PCI_BRIDGE_IOHIGH_REG);
			base |= ((reg >> PCI_BRIDGE_IOHIGH_BASE_SHIFT)
			    & 0xffff) << 16;
			limit |= ((reg >> PCI_BRIDGE_IOHIGH_LIMIT_SHIFT)
			    & 0xffff) << 16;
		}
		if (base < limit) {
			second_configured = true;
			goto configure;
		}

		/* Non-prefetchable memory region test */
		reg = pci_conf_read(pc, pa->pa_tag, PCI_BRIDGE_MEMORY_REG);
		base = ((reg >> PCI_BRIDGE_MEMORY_BASE_SHIFT)
		    & PCI_BRIDGE_MEMORY_BASE_MASK) << 20;
		limit = (((reg >> PCI_BRIDGE_MEMORY_LIMIT_SHIFT)
		    & PCI_BRIDGE_MEMORY_LIMIT_MASK) << 20) | 0x000fffff;
		if (base < limit) {
			second_configured = true;
			goto configure;
		}

		/* Prefetchable memory region test */
		reg = pci_conf_read(pc, pa->pa_tag,
		    PCI_BRIDGE_PREFETCHMEM_REG);
		base = ((reg >> PCI_BRIDGE_PREFETCHMEM_BASE_SHIFT)
		    & PCI_BRIDGE_PREFETCHMEM_BASE_MASK) << 20;
		limit = (((reg >> PCI_BRIDGE_PREFETCHMEM_LIMIT_SHIFT)
			& PCI_BRIDGE_PREFETCHMEM_LIMIT_MASK) << 20) | 0x000fffff;
		if (PCI_BRIDGE_PREFETCHMEM_64BITS(reg)) {
			reg = pci_conf_read(pc, pa->pa_tag,
			    PCI_BRIDGE_IOHIGH_REG);
			base |= (uint64_t)pci_conf_read(pc, pa->pa_tag,
			    PCI_BRIDGE_PREFETCHBASE32_REG) << 32;
			limit |= (uint64_t)pci_conf_read(pc, pa->pa_tag,
			    PCI_BRIDGE_PREFETCHLIMIT32_REG) << 32;
		}
		if (base < limit) {
			second_configured = true;
			goto configure;
		}
	}

configure:
	/*
	 * If the secondary bus is configured and the bus mastering is not
	 * enabled, enable it.
	 */
	if (second_configured) {
		reg = pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
		if ((reg & PCI_COMMAND_MASTER_ENABLE) == 0)
			pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
			    reg | PCI_COMMAND_MASTER_ENABLE);
	}

	if (!pmf_device_register(self, ppb_suspend, ppb_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/*
	 * Attach the PCI bus that hangs off of it.
	 *
	 * XXX Don't pass-through Memory Read Multiple.  Should we?
	 * XXX Consult the spec...
	 */
	pba.pba_iot = pa->pa_iot;
	pba.pba_memt = pa->pa_memt;
	pba.pba_dmat = pa->pa_dmat;
	pba.pba_dmat64 = pa->pa_dmat64;
	pba.pba_pc = pc;
	pba.pba_flags = pa->pa_flags & ~PCI_FLAGS_MRM_OKAY;
	pba.pba_bus = PPB_BUSINFO_SECONDARY(busdata);
	pba.pba_sub = PPB_BUSINFO_SUBORDINATE(busdata);
	pba.pba_bridgetag = &sc->sc_tag;
	pba.pba_intrswiz = pa->pa_intrswiz;
	pba.pba_intrtag = pa->pa_intrtag;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

static int
ppbdetach(device_t self, int flags)
{
#ifdef PPB_USEINTR
	struct ppb_softc *sc = device_private(self);
	pcireg_t slcsr;
#endif
	int rc;

	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;

#ifdef PPB_USEINTR
	if (sc->sc_intrhand != NULL) {
		/* Detach event counters */
		evcnt_detach(&sc->sc_ev_intr);
		evcnt_detach(&sc->sc_ev_abp);
		evcnt_detach(&sc->sc_ev_pfd);
		evcnt_detach(&sc->sc_ev_msc);
		evcnt_detach(&sc->sc_ev_pdc);
		evcnt_detach(&sc->sc_ev_cc);
		evcnt_detach(&sc->sc_ev_lacs);

		/* Clear any pending events and disable interrupt */
		slcsr = pci_conf_read(sc->sc_pc, sc->sc_tag,
		    sc->sc_pciecapoff + PCIE_SLCSR);
		slcsr &= ~PCIE_SLCSR_ENABLE_MASK;
		pci_conf_write(sc->sc_pc, sc->sc_tag,
		    sc->sc_pciecapoff + PCIE_SLCSR, slcsr);

		/* Disestablish the interrupt handler */
		pci_intr_disestablish(sc->sc_pc, sc->sc_intrhand);
		pci_intr_release(sc->sc_pc, sc->sc_pihp, 1);
	}
#endif

	pmf_device_deregister(self);
	return 0;
}

static bool
ppb_resume(device_t dv, const pmf_qual_t *qual)
{
	struct ppb_softc *sc = device_private(dv);
	int off;
	pcireg_t val;

        for (off = 0x40; off <= 0xff; off += 4) {
		val = pci_conf_read(sc->sc_pc, sc->sc_tag, off);
		if (val != sc->sc_pciconfext[(off - 0x40) / 4])
			pci_conf_write(sc->sc_pc, sc->sc_tag, off,
			    sc->sc_pciconfext[(off - 0x40)/4]);
	}

	return true;
}

static bool
ppb_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct ppb_softc *sc = device_private(dv);
	int off;

	for (off = 0x40; off <= 0xff; off += 4)
		sc->sc_pciconfext[(off - 0x40) / 4] =
		    pci_conf_read(sc->sc_pc, sc->sc_tag, off);

	return true;
}

static void
ppbchilddet(device_t self, device_t child)
{
	/* we keep no references to child devices, so do nothing */
}

#ifdef PPB_USEINTR
static int
ppb_intr(void *arg)
{
	struct ppb_softc *sc = arg;
	device_t dev = sc->sc_dev;
	pcireg_t reg;

	reg = pci_conf_read(sc->sc_pc, sc->sc_tag,
	    sc->sc_pciecapoff + PCIE_SLCSR);

	/*
	 * Not me. This check is only required for INTx.
	 * ppb_intr() would be spilted int ppb_intr_legacy() and ppb_intr_msi()
	 */
	if ((reg & PCIE_SLCSR_STATCHG_MASK) == 0)
		return 0;
		
	/* Clear interrupts. */
	pci_conf_write(sc->sc_pc, sc->sc_tag,
	    sc->sc_pciecapoff + PCIE_SLCSR, reg);

	sc->sc_ev_intr.ev_count++;

	/* Attention Button Pressed */
	if (reg & PCIE_SLCSR_ABP) {
		sc->sc_ev_abp.ev_count++;
		if (ppb_printevent)
			device_printf(dev, "Attention Button Pressed\n");
	}

	/* Power Fault Detected */
	if (reg & PCIE_SLCSR_PFD) {
		sc->sc_ev_pfd.ev_count++;
		if (ppb_printevent)
			device_printf(dev, "Power Fault Detected\n");
	}

	/* MRL Sensor Changed */
	if (reg & PCIE_SLCSR_MSC) {
		sc->sc_ev_msc.ev_count++;
		if (ppb_printevent)
			device_printf(dev, "MRL Sensor Changed\n");
	}

	/* Presence Detect Changed */
	if (reg & PCIE_SLCSR_PDC) {
		sc->sc_ev_pdc.ev_count++;
		if (ppb_printevent)
			device_printf(dev, "Presence Detect Changed\n");
		if (reg & PCIE_SLCSR_PDS) {
			/* XXX Insert */
		} else {
			/* XXX Remove */
		}
	}

	/* Command Completed */
	if (reg & PCIE_SLCSR_CC) {
		sc->sc_ev_cc.ev_count++;
		if (ppb_printevent)
			device_printf(dev, "Command Completed\n");
	}

	/* Data Link Layer State Changed */
	if (reg & PCIE_SLCSR_LACS) {
		sc->sc_ev_lacs.ev_count++;
		if (ppb_printevent)
			device_printf(dev, "Data Link Layer State Changed\n");
	}

	return 1;
}
#endif /* PPB_USEINTR */
