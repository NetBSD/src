/*	$NetBSD: if_fxp_pci.c,v 1.12.2.4 2002/04/17 00:06:01 nathanw Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * PCI bus front-end for the Intel i82557 fast Ethernet controller
 * driver.  Works with Intel Etherexpress Pro 10+, 100B, 100+ cards.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_fxp_pci.c,v 1.12.2.4 2002/04/17 00:06:01 nathanw Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/endian.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>

#include <dev/ic/i82557reg.h>
#include <dev/ic/i82557var.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

struct fxp_pci_softc {
	struct fxp_softc psc_fxp;

	pci_chipset_tag_t psc_pc;	/* pci chipset tag */
	pcireg_t psc_regs[0x20>>2];	/* saved PCI config regs (sparse) */
	pcitag_t psc_tag;		/* pci register tag */
	void *psc_powerhook;		/* power hook */

	int psc_pwrmgmt_csr_reg;	/* ACPI power management register */
	pcireg_t psc_pwrmgmt_csr;	/* ...and the contents at D0 */
};

int	fxp_pci_match __P((struct device *, struct cfdata *, void *));
void	fxp_pci_attach __P((struct device *, struct device *, void *));

int	fxp_pci_enable __P((struct fxp_softc *));
void	fxp_pci_disable __P((struct fxp_softc *));

static void	fxp_pci_confreg_restore __P((struct fxp_pci_softc *psc));
static void	fxp_pci_power __P((int why, void *arg));

struct cfattach fxp_pci_ca = {
	sizeof(struct fxp_pci_softc), fxp_pci_match, fxp_pci_attach
};

const struct fxp_pci_product {
	u_int32_t	fpp_prodid;	/* PCI product ID */
	const char	*fpp_name;	/* device name */
} fxp_pci_products[] = {
	{ PCI_PRODUCT_INTEL_82557,
	  "Intel i82557 Ethernet" },
	{ PCI_PRODUCT_INTEL_82559ER,
	  "Intel i82559ER Ethernet" },
	{ PCI_PRODUCT_INTEL_IN_BUSINESS,
	  "Intel InBusiness Ethernet" },
	{ PCI_PRODUCT_INTEL_82801BA_LAN,
	  "Intel i82562 Ethernet" },
	{ PCI_PRODUCT_INTEL_PRO_100_VE_0,
	  "Intel PRO/100 VE Network Controller" },
	{ PCI_PRODUCT_INTEL_PRO_100_VE_1,
	  "Intel PRO/100 VE Network Controller" },
	{ 0,
	  NULL },
};

static const struct fxp_pci_product *
fxp_pci_lookup(const struct pci_attach_args *pa)
{
	const struct fxp_pci_product *fpp;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return (NULL);

	for (fpp = fxp_pci_products; fpp->fpp_name != NULL; fpp++)
		if (PCI_PRODUCT(pa->pa_id) == fpp->fpp_prodid)
			return (fpp);

	return (NULL);
}

int
fxp_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (fxp_pci_lookup(pa) != NULL)
		return (1);

	return (0);
}

/*
 * Restore PCI configuration registers that may have been clobbered.
 * This is necessary due to bugs on the Sony VAIO Z505-series on-board
 * ethernet, after an APM suspend/resume, as well as after an ACPI
 * D3->D0 transition.  We call this function from a power hook after
 * APM resume events, as well as after the ACPI D3->D0 transition.
 */
static void
fxp_pci_confreg_restore(psc)
        struct fxp_pci_softc *psc;
{
	pcireg_t reg;

#if 0
	/*
	 * Check to see if the command register is blank -- if so, then
	 * we'll assume that all the clobberable-registers have been
	 * clobbered.
	 */

	/*
	 * In general, the above metric is accurate. Unfortunately,
	 * it is inaccurate across a hibernation. Ideally APM/ACPI
	 * code should take note of hibernation events and execute
	 * a hibernation wakeup hook, but at present a hibernation wake
	 * is indistinguishable from a suspend wake.
	 */

	if (((reg = pci_conf_read(psc->psc_pc, psc->psc_tag,
	    PCI_COMMAND_STATUS_REG)) & 0xffff) != 0)
		return;
#else
	reg = pci_conf_read(psc->psc_pc, psc->psc_tag, PCI_COMMAND_STATUS_REG);
#endif

	pci_conf_write(psc->psc_pc, psc->psc_tag,
	    PCI_COMMAND_STATUS_REG,
	    (reg & 0xffff0000) |
	    (psc->psc_regs[PCI_COMMAND_STATUS_REG>>2] & 0xffff));
	pci_conf_write(psc->psc_pc, psc->psc_tag, PCI_BHLC_REG,
	    psc->psc_regs[PCI_BHLC_REG>>2]);
	pci_conf_write(psc->psc_pc, psc->psc_tag, PCI_MAPREG_START+0x0,
	    psc->psc_regs[(PCI_MAPREG_START+0x0)>>2]);
	pci_conf_write(psc->psc_pc, psc->psc_tag, PCI_MAPREG_START+0x4,
	    psc->psc_regs[(PCI_MAPREG_START+0x4)>>2]);
	pci_conf_write(psc->psc_pc, psc->psc_tag, PCI_MAPREG_START+0x8,
	    psc->psc_regs[(PCI_MAPREG_START+0x8)>>2]);
}


/*
 * Power handler routine. Called when the system is transitioning into/out
 * of power save modes. We restore the (bashed) PCI configuration registers
 * on a resume.
 */
static void
fxp_pci_power(why, arg)
	int why;
	void *arg;
{
	struct fxp_pci_softc *psc = arg;

	if (why == PWR_RESUME)
		fxp_pci_confreg_restore(psc);
}

void
fxp_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fxp_pci_softc *psc = (struct fxp_pci_softc *)self;
	struct fxp_softc *sc = (struct fxp_softc *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const struct fxp_pci_product *fpp;
	const char *intrstr = NULL;
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	int ioh_valid, memh_valid;
	bus_addr_t addr;
	bus_size_t size;
	int flags;
 	int pci_pwrmgmt_cap_reg;

	/*
	 * Map control/status registers.
	 */
	ioh_valid = (pci_mapreg_map(pa, FXP_PCI_IOBA,
	    PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, NULL) == 0);

	/*
	 * Version 2.1 of the PCI spec, page 196, "Address Maps":
	 *
	 *	Prefetchable
	 *
	 *	Set to one if there are no side effects on reads, the
	 *	device returns all bytes regardless of the byte enables,
	 *	and host bridges can merge processor writes into this
	 *	range without causing errors.  Bit must be set to zero
	 *	otherwise.
	 *
	 * The 82557 incorrectly sets the "prefetchable" bit, resulting
	 * in errors on systems which will do merged reads and writes.
	 * These errors manifest themselves as all-bits-set when reading
	 * from the EEPROM or other < 4 byte registers.
	 *
	 * We must work around this problem by always forcing the mapping
	 * for memory space to be uncacheable.  On systems which cannot
	 * create an uncacheable mapping (because the firmware mapped it
	 * into only cacheable/prefetchable space due to the "prefetchable"
	 * bit), we can fall back onto i/o mapped access.
	 */
	memh_valid = 0;
	memt = pa->pa_memt;
	if (((pa->pa_flags & PCI_FLAGS_MEM_ENABLED) != 0) &&
	    pci_mapreg_info(pa->pa_pc, pa->pa_tag, FXP_PCI_MMBA,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT,
	    &addr, &size, &flags) == 0) {
		flags &= ~BUS_SPACE_MAP_PREFETCHABLE;
		if (bus_space_map(memt, addr, size, flags, &memh) == 0)
			memh_valid = 1;
	}

	if (memh_valid) {
		sc->sc_st = memt;
		sc->sc_sh = memh;
	} else if (ioh_valid) {
		sc->sc_st = iot;
		sc->sc_sh = ioh;
	} else {
		printf(": unable to map device registers\n");
		return;
	}

	sc->sc_dmat = pa->pa_dmat;

	fpp = fxp_pci_lookup(pa);
	if (fpp == NULL) {
		printf("\n");
		panic("fxp_pci_attach: impossible");
	}

	sc->sc_rev = PCI_REVISION(pa->pa_class);

	switch (fpp->fpp_prodid) {
	case PCI_PRODUCT_INTEL_82557:
	case PCI_PRODUCT_INTEL_82559ER:
	case PCI_PRODUCT_INTEL_IN_BUSINESS:
	    {
		const char *chipname = NULL;

		if (sc->sc_rev >= FXP_REV_82558_A4) {
			chipname = "i82558 Ethernet";
			/*
			 * Enable the MWI command for memory writes.
			 */
			if (pa->pa_flags & PCI_FLAGS_MWI_OKAY)
				sc->sc_flags |= FXPF_MWI;
		}
		if (sc->sc_rev >= FXP_REV_82559_A0)
			chipname = "i82559 Ethernet";
		if (sc->sc_rev >= FXP_REV_82559S_A)
			chipname = "i82559S Ethernet";
		if (sc->sc_rev >= FXP_REV_82550)
			chipname = "i82550 Ethernet";

		/*
		 * Mark all i82559 and i82550 revisions as having
		 * the "resume bug".  See i82557.c for details.
		 */
		if (sc->sc_rev >= FXP_REV_82559_A0)
			sc->sc_flags |= FXPF_HAS_RESUME_BUG;

		printf(": %s, rev %d\n", chipname != NULL ? chipname :
		    fpp->fpp_name, sc->sc_rev);
		break;
	    }

	case PCI_PRODUCT_INTEL_82801BA_LAN:
		printf(": %s, rev %d\n", fpp->fpp_name, sc->sc_rev);

		/*
		 * The 82801BA Ethernet has a bug which requires us to send a
		 * NOP before a CU_RESUME if we're in 10baseT mode.
		 */
		if (fpp->fpp_prodid == PCI_PRODUCT_INTEL_82801BA_LAN)
			sc->sc_flags |= FXPF_HAS_RESUME_BUG;
		break;

	case PCI_PRODUCT_INTEL_PRO_100_VE_0:
	case PCI_PRODUCT_INTEL_PRO_100_VE_1:
	case PCI_PRODUCT_INTEL_PRO_100_VM_0:
	case PCI_PRODUCT_INTEL_PRO_100_VM_1:
	case PCI_PRODUCT_INTEL_82562EH_HPNA_0:
	case PCI_PRODUCT_INTEL_82562EH_HPNA_1:
	case PCI_PRODUCT_INTEL_82562EH_HPNA_2:
	case PCI_PRODUCT_INTEL_PRO_100_VM_2:
		printf(": %s, rev %d\n", fpp->fpp_name, sc->sc_rev);

		/*
		 * ICH3 chips apparently have problems with the enhanced
		 * features, so just treat them as an i82557.  It also
		 * has the resume bug that the ICH2 has.
		 */
		sc->sc_rev = 1;
		sc->sc_flags |= FXPF_HAS_RESUME_BUG;
		break;
	}

	/* Make sure bus-mastering is enabled. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

  	/*
	 * Under some circumstances (such as APM suspend/resume
	 * cycles, and across ACPI power state changes), the
	 * i82257-family can lose the contents of critical PCI
	 * configuration registers, causing the card to be
	 * non-responsive and useless.  This occurs on the Sony VAIO
	 * Z505-series, among others.  Preserve them here so they can
	 * be later restored (by fxp_pci_confreg_restore()).
	 */
	psc->psc_pc = pc;
	psc->psc_tag = pa->pa_tag;
	psc->psc_regs[PCI_COMMAND_STATUS_REG>>2] =
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	psc->psc_regs[PCI_BHLC_REG>>2] =
	    pci_conf_read(pc, pa->pa_tag, PCI_BHLC_REG);
	psc->psc_regs[(PCI_MAPREG_START+0x0)>>2] =
	    pci_conf_read(pc, pa->pa_tag, PCI_MAPREG_START+0x0);
	psc->psc_regs[(PCI_MAPREG_START+0x4)>>2] =
	    pci_conf_read(pc, pa->pa_tag, PCI_MAPREG_START+0x4);
	psc->psc_regs[(PCI_MAPREG_START+0x8)>>2] =
	    pci_conf_read(pc, pa->pa_tag, PCI_MAPREG_START+0x8);

	/*
	 * Work around BIOS ACPI bugs where the chip is inadvertantly
	 * left in ACPI D3 (lowest power state).  First confirm the device
	 * supports ACPI power management, then move it to the D0 (fully
	 * functional) state if it is not already there.
	 */
	if (pci_get_capability(pc, pa->pa_tag, PCI_CAP_PWRMGMT,
	    &pci_pwrmgmt_cap_reg, 0)) {
		pcireg_t reg;

		sc->sc_enable = fxp_pci_enable;
		sc->sc_disable = fxp_pci_disable;

		psc->psc_pwrmgmt_csr_reg = pci_pwrmgmt_cap_reg + 4;
		reg = pci_conf_read(pc, pa->pa_tag, psc->psc_pwrmgmt_csr_reg);
		psc->psc_pwrmgmt_csr = (reg & ~PCI_PMCSR_STATE_MASK) |
		    PCI_PMCSR_STATE_D0;
		if ((reg & PCI_PMCSR_STATE_MASK) != PCI_PMCSR_STATE_D0)
			pci_conf_write(pc, pa->pa_tag, psc->psc_pwrmgmt_csr_reg,
			    psc->psc_pwrmgmt_csr);
	}
	/* Restore PCI configuration registers. */
	fxp_pci_confreg_restore(psc);

	sc->sc_enabled = 1;

	/*
	 * Map and establish our interrupt.
	 */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, fxp_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/* Finish off the attach. */
	fxp_attach(sc);
	if (sc->sc_disable != NULL)
		fxp_disable(sc);

	/* Add a suspend hook to restore PCI config state */
	psc->psc_powerhook = powerhook_establish(fxp_pci_power, psc);
	if (psc->psc_powerhook == NULL)
		printf ("%s: WARNING: unable to establish pci power hook\n",
		    sc->sc_dev.dv_xname);
}

int
fxp_pci_enable(struct fxp_softc *sc)
{
	struct fxp_pci_softc *psc = (void *) sc;

#if 0
	printf("%s: going to power state D0\n", sc->sc_dev.dv_xname);
#endif

	/* Bring the device into D0 power state. */
	pci_conf_write(psc->psc_pc, psc->psc_tag,
	    psc->psc_pwrmgmt_csr_reg, psc->psc_pwrmgmt_csr);

	/* Now restore the configuration registers. */
	fxp_pci_confreg_restore(psc);

	return (0);
}

void
fxp_pci_disable(struct fxp_softc *sc)
{
	struct fxp_pci_softc *psc = (void *) sc;

#if 0
	printf("%s: going to power state D3\n", sc->sc_dev.dv_xname);
#endif

	/* Put the device into D3 state. */
	pci_conf_write(psc->psc_pc, psc->psc_tag,
	    psc->psc_pwrmgmt_csr_reg, (psc->psc_pwrmgmt_csr &
	    ~PCI_PMCSR_STATE_MASK) | PCI_PMCSR_STATE_D3);
}
