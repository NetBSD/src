/*	$NetBSD: i386_mainbus.c,v 1.6 2021/08/07 16:18:55 thorpej Exp $	*/
/*	NetBSD: mainbus.c,v 1.104 2018/12/02 08:19:44 cherry Exp 	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: i386_mainbus.c,v 1.6 2021/08/07 16:18:55 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/bus.h>

#include <dev/isa/isavar.h>
#include <dev/eisa/eisavar.h>
#include <dev/pci/pcivar.h>

#include <dev/isa/isareg.h>		/* for ISA_HOLE_VADDR */

#include "pci.h"
#include "eisa.h"
#include "isa.h"
#include "isadma.h"
#include "mca.h"
#include "pnpbios.h"
#include "acpica.h"
#include "ipmi.h"

#include "opt_acpi.h"
#include "opt_mpbios.h"
#include "opt_pcifixup.h"

#include <machine/cpuvar.h>
#include <machine/i82093var.h>
#include <machine/mpbiosvar.h>
#include <machine/mpacpi.h>

#if NPNPBIOS > 0
#include <arch/i386/pnpbios/pnpbiosvar.h>
#endif

#if NACPICA > 0
#include <dev/acpi/acpivar.h>
#endif

#if NMCA > 0
#include <dev/mca/mcavar.h>
#endif

#include <x86/autoconf.h>

#if NIPMI > 0
#include <x86/ipmivar.h>
#endif

#if NPCI > 0
#if defined(PCI_BUS_FIXUP)
#include <arch/x86/pci/pci_bus_fixup.h>
#if defined(PCI_ADDR_FIXUP)
#include <arch/x86/pci/pci_addr_fixup.h>
#endif
#endif
#ifdef __HAVE_PCI_MSI_MSIX
#include <arch/x86/pci/msipic.h>
#endif /* __HAVE_PCI_MSI_MSIX */
#endif

void	i386_mainbus_childdetached(device_t, device_t);
int	i386_mainbus_match(device_t, cfdata_t, void *);
void	i386_mainbus_attach(device_t, device_t, void *);
int	i386_mainbus_rescan(device_t, const char *, const int *);

union i386_mainbus_attach_args {
	const char *mba_busname;		/* first elem of all */
	struct pcibus_attach_args mba_pba;
	struct eisabus_attach_args mba_eba;
	struct isabus_attach_args mba_iba;
#if NMCA > 0
	struct mcabus_attach_args mba_mba;
#endif
#if NPNPBIOS > 0
	struct pnpbios_attach_args mba_paa;
#endif
	struct cpu_attach_args mba_caa;
	struct apic_attach_args aaa_caa;
#if NACPICA > 0
	struct acpibus_attach_args mba_acpi;
#endif
#if NIPMI > 0
	struct ipmi_attach_args mba_ipmi;
#endif
};

/*
 * This is set when the ISA bus is attached.  If it's not set by the
 * time it's checked below, then mainbus attempts to attach an ISA.
 */
int isa_has_been_seen;
struct x86_isa_chipset x86_isa_chipset;
#if NISA > 0
static const struct isabus_attach_args mba_iba = {
	._iba_busname = "isa",
	.iba_dmat = &isa_bus_dma_tag,
	.iba_ic = &x86_isa_chipset
};
#endif

/*
 * Same as above, but for EISA.
 */
int eisa_has_been_seen;

#if defined(MPBIOS) || NACPICA > 0
struct mp_bus *mp_busses;
int mp_nbus;
struct mp_intr_map *mp_intrs;
int mp_nintr;

int mp_isa_bus = -1;	/* XXX */
int mp_eisa_bus = -1;	/* XXX */

extern bool acpi_present;
extern bool mpacpi_active;

# ifdef MPVERBOSE
#  if MPVERBOSE > 0
int mp_verbose = MPVERBOSE;
#  else
int mp_verbose = 1;
#  endif
# else
int mp_verbose = 0;
# endif
#endif

void
i386_mainbus_childdetached(device_t self, device_t child)
{
	struct mainbus_softc *sc = device_private(self);

	if (sc->sc_acpi == child)
		sc->sc_acpi = NULL;
	if (sc->sc_ipmi == child)
		sc->sc_ipmi = NULL;
	if (sc->sc_mca == child)
		sc->sc_mca = NULL;
	if (sc->sc_pnpbios == child)
		sc->sc_pnpbios = NULL;
	if (sc->sc_pci == child)
		sc->sc_pci = NULL;

#if NPCI > 0
	mp_pci_childdetached(self, child);
#endif
}

/*
 * Probe for the mainbus; always succeeds.
 */
int
i386_mainbus_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

/*
 * Attach the mainbus.
 */
void
i386_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_softc *sc = device_private(self);
	union i386_mainbus_attach_args mba;

	sc->sc_dev = self;

#if NISADMA > 0 && (NACPICA > 0 || NPNPBIOS > 0)
	/*
	 * ACPI and PNPBIOS need ISA DMA initialized before they start probing.
	 */
	isa_dmainit(&x86_isa_chipset, x86_bus_space_io, &isa_bus_dma_tag,
	    self);
#endif

	i386_mainbus_rescan(self, "acpibus", NULL);

	i386_mainbus_rescan(self, "pnpbiosbus", NULL);

	i386_mainbus_rescan(self, "ipmibus", NULL);

	i386_mainbus_rescan(self, "pcibus", NULL);

	i386_mainbus_rescan(self, "mcabus", NULL);

	if (memcmp(ISA_HOLE_VADDR(EISA_ID_PADDR), EISA_ID, EISA_ID_LEN) == 0 &&
	    eisa_has_been_seen == 0) {
		mba.mba_eba.eba_iot = x86_bus_space_io;
		mba.mba_eba.eba_memt = x86_bus_space_mem;
#if NEISA > 0
		mba.mba_eba.eba_dmat = &eisa_bus_dma_tag;
#endif
		config_found(self, &mba.mba_eba, eisabusprint,
		    CFARGS(.iattr = "eisabus"));
	}

#if NISA > 0
	if (isa_has_been_seen == 0) {
		mba.mba_iba = mba_iba;
		mba.mba_iba.iba_iot = x86_bus_space_io;
		mba.mba_iba.iba_memt = x86_bus_space_mem;
		config_found(self, &mba.mba_iba, isabusprint,
		    CFARGS(.iattr = "isabus"));
	}
#endif

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

/* scan for new children */
int
i386_mainbus_rescan(device_t self, const char *ifattr, const int *locators)
{
	struct mainbus_softc *sc = device_private(self);
#if NACPICA > 0 || NIPMI > 0 || NMCA > 0 || NPCI > 0 || NPNPBIOS > 0
	union i386_mainbus_attach_args mba;
#endif

	if (ifattr_match(ifattr, "acpibus") && sc->sc_acpi == NULL &&
	    acpi_present) {
#if NACPICA > 0
		mba.mba_acpi.aa_iot = x86_bus_space_io;
		mba.mba_acpi.aa_memt = x86_bus_space_mem;
		mba.mba_acpi.aa_pc = NULL;
		mba.mba_acpi.aa_pciflags =
		    PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
		    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY |
		    PCI_FLAGS_MWI_OKAY;
		mba.mba_acpi.aa_ic = &x86_isa_chipset;
		mba.mba_acpi.aa_dmat = &pci_bus_dma_tag;
		mba.mba_acpi.aa_dmat64 = NULL;
		sc->sc_acpi = config_found(self, &mba.mba_acpi, NULL,
		    CFARGS(.iattr = "acpibus"));
#if 0 /* XXXJRT not yet */
		if (acpi_active) {
			/*
			 * ACPI already did all the work for us, there
			 * is nothing more for us to do.
			 */
			return;
		}
#endif
#endif
	}

	if (ifattr_match(ifattr, "pnpbiosbus") && sc->sc_pnpbios == NULL) {
#if NPNPBIOS > 0
#if NACPICA > 0
		if (acpi_active == 0)
#endif
		if (pnpbios_probe()) {
			mba.mba_paa.paa_ic = &x86_isa_chipset;
			sc->sc_pnpbios = config_found(self, &mba.mba_paa, NULL,
			    CFARGS(.iattr = "pnpbiosbus"));
		}
#endif
	}

	if (ifattr_match(ifattr, "ipmibus") && sc->sc_ipmi == NULL) {
#if NIPMI > 0
		memset(&mba.mba_ipmi, 0, sizeof(mba.mba_ipmi));
		mba.mba_ipmi.iaa_iot = x86_bus_space_io;
		mba.mba_ipmi.iaa_memt = x86_bus_space_mem;
		if (ipmi_probe(&mba.mba_ipmi)) {
			sc->sc_ipmi = config_found(self, &mba.mba_ipmi, NULL,
			    CFARGS(.iattr = "ipmibus"));
		}
#endif
	}

	/*
	 * XXX Note also that the presence of a PCI bus should
	 * XXX _always_ be checked, and if present the bus should be
	 * XXX 'found'.  However, because of the structure of the code,
	 * XXX that's not currently possible.
	 */
#if NPCI > 0
	if (pci_mode_detect() != 0 && ifattr_match(ifattr, "pcibus")) {
		int npcibus = 0;

		mba.mba_pba.pba_iot = x86_bus_space_io;
		mba.mba_pba.pba_memt = x86_bus_space_mem;
		mba.mba_pba.pba_dmat = &pci_bus_dma_tag;
		mba.mba_pba.pba_dmat64 = NULL;
		mba.mba_pba.pba_pc = NULL;
		mba.mba_pba.pba_flags =
		    PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
		    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY |
		    PCI_FLAGS_MWI_OKAY;
		mba.mba_pba.pba_bus = 0;
		/* XXX On those machines with >1 Host-PCI bridge,
		 * XXX not every bus > pba_bus is subordinate to pba_bus,
		 * XXX but this works on many machines, and pba_sub is
		 * XXX not used today by any critical code, so it is safe
		 * XXX to be so inclusive at this time.
		 */
		mba.mba_pba.pba_sub = 255;
		mba.mba_pba.pba_bridgetag = NULL;
#if NACPICA > 0 && defined(ACPI_SCANPCI)
		if (npcibus == 0 && mpacpi_active)
			npcibus = mp_pci_scan(self, &mba.mba_pba, pcibusprint);
#endif
#if defined(MPBIOS) && defined(MPBIOS_SCANPCI)
		if (npcibus == 0 && mpbios_scanned != 0)
			npcibus = mp_pci_scan(self, &mba.mba_pba, pcibusprint);
#endif
		if (npcibus == 0 && sc->sc_pci == NULL) {
			sc->sc_pci =
			    config_found(self, &mba.mba_pba, pcibusprint,
					 CFARGS(.iattr = "pcibus"));
		}
#if NACPICA > 0
		if (mp_verbose)
			acpi_pci_link_state();
#endif
	}
#endif


	if (ifattr_match(ifattr, "mcabus") && sc->sc_mca == NULL) {
#if NMCA > 0
	/* Note: MCA bus probe is done in i386/machdep.c */
		if (MCA_system) {
			mba.mba_mba.mba_iot = x86_bus_space_io;
			mba.mba_mba.mba_memt = x86_bus_space_mem;
			mba.mba_mba.mba_dmat = &mca_bus_dma_tag;
			mba.mba_mba.mba_mc = NULL;
			mba.mba_mba.mba_bus = 0;
			sc->sc_mca = config_found(self,
			    &mba.mba_mba, mcabusprint,
			    CFARGS(.iattr = "mcabus"));
		}
#endif
	}
	return 0;
}

