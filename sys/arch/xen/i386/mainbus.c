/*	$NetBSD: mainbus.c,v 1.2 2004/04/17 12:56:27 cl Exp $	*/
/*	NetBSD: mainbus.c,v 1.53 2003/10/27 14:11:47 junyoung Exp 	*/

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.2 2004/04/17 12:56:27 cl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/eisa/eisavar.h>
#include <dev/pci/pcivar.h>

#include <dev/isa/isareg.h>		/* for ISA_HOLE_VADDR */

#include "pci.h"
#include "eisa.h"
#include "isa.h"
#include "isadma.h"
#include "mca.h"
#include "apm.h"
#include "pnpbios.h"
#include "acpi.h"
#include "vesabios.h"
#include "xenc.h"
#include "xennet.h"
#include "xbd.h"
#include "npx.h"

#include "opt_mpacpi.h"
#include "opt_mpbios.h"
#include "opt_xen.h"

#include <machine/cpuvar.h>
#include <machine/i82093var.h>
#include <machine/mpbiosvar.h>
#include <machine/mpacpi.h>

#if NAPM > 0
#include <machine/bioscall.h>
#include <machine/apmvar.h>
#endif

#if NPNPBIOS > 0
#include <arch/i386/pnpbios/pnpbiosvar.h>
#endif

#if NACPI > 0
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_madt.h>
#endif

#if NMCA > 0
#include <dev/mca/mcavar.h>
#endif

#if NVESABIOS > 0
#include <arch/i386/bios/vesabios.h>
#endif

#ifdef XEN
#include <machine/xen.h>
#include <machine/hypervisor.h>
#endif

#if NXENNET > 0
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <machine/if_xennetvar.h>
#endif

#if NXBD > 0
#include <sys/buf.h>
#include <sys/disk.h>
#include <dev/dkvar.h>
#include <machine/xbdvar.h>
#endif

int	mainbus_match(struct device *, struct cfdata *, void *);
void	mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

int	mainbus_print(void *, const char *);

union mainbus_attach_args {
	const char *mba_busname;		/* first elem of all */
	struct pcibus_attach_args mba_pba;
	struct eisabus_attach_args mba_eba;
	struct isabus_attach_args mba_iba;
#if NMCA > 0
	struct mcabus_attach_args mba_mba;
#endif
#if NAPM > 0
	struct apm_attach_args mba_aaa;
#endif
#if NPNPBIOS > 0
	struct pnpbios_attach_args mba_paa;
#endif
	struct cpu_attach_args mba_caa;
	struct apic_attach_args aaa_caa;
#if NACPI > 0
	struct acpibus_attach_args mba_acpi;
#endif
#if NVESABIOS > 0
	struct vesabios_attach_args mba_vba;
#endif
#if NXENC > 0
	struct xenc_attach_args mba_xenc;
#endif
#if NXENNET > 0
	struct xennet_attach_args mba_xennet;
#endif
#if NXBD > 0
	struct xbd_attach_args mba_xbd;
#endif
#if NNPX > 0
	struct xen_npx_attach_args mba_xennpx;
#endif
};

/*
 * This is set when the ISA bus is attached.  If it's not set by the
 * time it's checked below, then mainbus attempts to attach an ISA.
 */
int	isa_has_been_seen;
struct x86_isa_chipset x86_isa_chipset;
#if NISA > 0
struct isabus_attach_args mba_iba = {
	"isa",
	X86_BUS_SPACE_IO, X86_BUS_SPACE_MEM,
	&isa_bus_dma_tag,
	&x86_isa_chipset
};
#endif

/*
 * Same as above, but for EISA.
 */
int	eisa_has_been_seen;

#if defined(MPBIOS) || defined(MPACPI)
struct mp_bus *mp_busses;
int mp_nbus;
struct mp_intr_map *mp_intrs;
int mp_nintr;
 
int mp_isa_bus = -1;            /* XXX */
int mp_eisa_bus = -1;           /* XXX */

#ifdef MPVERBOSE
int mp_verbose = 1;
#else
int mp_verbose = 0;
#endif
#endif


/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union mainbus_attach_args mba;
#if NACPI > 0
	int acpi_present = 0;
#endif
#ifdef MPBIOS
	int mpbios_present = 0;
#endif
	int mpacpi_active = 0;

	printf("\n");

#ifdef MPBIOS
	mpbios_present = mpbios_probe(self);
#endif

#if NPCI > 0
	/*
	 * ACPI needs to be able to access PCI configuration space.
	 */
	pci_mode = pci_mode_detect();
#endif

#if NACPI > 0
	acpi_present = acpi_probe();
#ifdef MPACPI
	/*
	 * First, see if the MADT contains CPUs, and possibly I/O APICs.
	 * Building the interrupt routing structures can only
	 * be done later (via a callback).
	 */
	if (acpi_present)
		mpacpi_active = mpacpi_scan_apics(self);
#endif
#endif

	if (!mpacpi_active) {
#ifdef MPBIOS
		if (mpbios_present)
			mpbios_scan(self);
		else
#endif
		{
			struct cpu_attach_args caa;
			
			memset(&caa, 0, sizeof(caa));
			caa.caa_name = "cpu";
			caa.cpu_number = 0;
			caa.cpu_role = CPU_ROLE_SP;
			caa.cpu_func = 0;
			
			config_found(self, &caa, mainbus_print);
		}
	}

#if NVESABIOS > 0
	if (vbeprobe()) {
		mba.mba_vba.vaa_busname = "vesabios";
		config_found(self, &mba.mba_vba, mainbus_print);
	}
#endif

#if NISADMA > 0 && (NACPI > 0 || NPNPBIOS > 0)
	/*
	 * ACPI and PNPBIOS need ISA DMA initialized before they start probing.
	 */
	isa_dmainit(&x86_isa_chipset, X86_BUS_SPACE_IO, &isa_bus_dma_tag,
	    self);
#endif

#if NACPI > 0
	if (acpi_present) {
		mba.mba_acpi.aa_busname = "acpi";
		mba.mba_acpi.aa_iot = X86_BUS_SPACE_IO;
		mba.mba_acpi.aa_memt = X86_BUS_SPACE_MEM;
		mba.mba_acpi.aa_pc = NULL;
		mba.mba_acpi.aa_pciflags =
		    PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
		    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY |
		    PCI_FLAGS_MWI_OKAY;
		mba.mba_acpi.aa_ic = &x86_isa_chipset;
		config_found(self, &mba.mba_acpi, mainbus_print);
#if 0 /* XXXJRT not yet */
		if (acpi_active) {
			/*
			 * ACPI already did all the work for us, there
			 * is nothing more for us to do.
			 */
			return;
		}
#endif
	}
#endif

#if NPNPBIOS > 0
#if NACPI > 0
	if (acpi_active == 0)
#endif
	if (pnpbios_probe()) {
		mba.mba_paa.paa_busname = "pnpbios";
		mba.mba_paa.paa_ic = &x86_isa_chipset;
		config_found(self, &mba.mba_paa, mainbus_print);
	}
#endif

	/*
	 * XXX Note also that the presence of a PCI bus should
	 * XXX _always_ be checked, and if present the bus should be
	 * XXX 'found'.  However, because of the structure of the code,
	 * XXX that's not currently possible.
	 */
#if NPCI > 0
	if (pci_mode != 0) {
		mba.mba_pba.pba_busname = "pci";
		mba.mba_pba.pba_iot = X86_BUS_SPACE_IO;
		mba.mba_pba.pba_memt = X86_BUS_SPACE_MEM;
		mba.mba_pba.pba_dmat = &pci_bus_dma_tag;
		mba.mba_pba.pba_dmat64 = NULL;
		mba.mba_pba.pba_pc = NULL;
		mba.mba_pba.pba_flags = pci_bus_flags();
		mba.mba_pba.pba_bus = 0;
		mba.mba_pba.pba_bridgetag = NULL;
#if defined(MPACPI) && defined(MPACPI_SCANPCI)
		if (mpacpi_active)
			mpacpi_scan_pci(self, &mba.mba_pba, mainbus_print);
		else
#endif
#if defined(MPBIOS) && defined(MPBIOS_SCANPCI)
		if (mpbios_scanned != 0)
			mpbios_scan_pci(self, &mba.mba_pba, mainbus_print);
		else
#endif
		config_found(self, &mba.mba_pba, mainbus_print);
	}
#endif

#if NMCA > 0
	/* Note: MCA bus probe is done in i386/machdep.c */
	if (MCA_system) {
		mba.mba_mba.mba_busname = "mca";
		mba.mba_mba.mba_iot = X86_BUS_SPACE_IO;
		mba.mba_mba.mba_memt = X86_BUS_SPACE_MEM;
		mba.mba_mba.mba_dmat = &mca_bus_dma_tag;
		mba.mba_mba.mba_mc = NULL;
		mba.mba_mba.mba_bus = 0;
		config_found(self, &mba.mba_mba, mainbus_print);
	}
#endif

#ifndef XEN
	if (memcmp(ISA_HOLE_VADDR(EISA_ID_PADDR), EISA_ID, EISA_ID_LEN) == 0 &&
	    eisa_has_been_seen == 0) {
		mba.mba_eba.eba_busname = "eisa";
		mba.mba_eba.eba_iot = X86_BUS_SPACE_IO;
		mba.mba_eba.eba_memt = X86_BUS_SPACE_MEM;
#if NEISA > 0
		mba.mba_eba.eba_dmat = &eisa_bus_dma_tag;
#endif
		config_found(self, &mba.mba_eba, mainbus_print);
	}
#endif

#if NISA > 0
	if (isa_has_been_seen == 0)
		config_found(self, &mba_iba, mainbus_print);
#endif

#if NAPM > 0
#if NACPI > 0
	if (acpi_active == 0)
#endif
	if (apm_busprobe()) {
		mba.mba_aaa.aaa_busname = "apm";
		config_found(self, &mba.mba_aaa, mainbus_print);
	}
#endif

#if NXENC > 0
	mba.mba_xenc.xa_busname = "xenc";
	config_found(self, &mba.mba_xenc, mainbus_print);
#endif
#if NXENNET > 0
	mba.mba_xennet.xa_busname = "xennet";
	xennet_scan(self, &mba.mba_xennet, mainbus_print);
#endif
#if NXBD > 0
	mba.mba_xbd.xa_busname = "xbd";
	xbd_scan(self, &mba.mba_xbd, mainbus_print);
#endif
#if NNPX > 0
	mba.mba_xennpx.xa_busname = "npx";
	config_found(self, &mba.mba_xennpx, mainbus_print);
#endif
}

int
mainbus_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	union mainbus_attach_args *mba = aux;

	if (pnp)
		aprint_normal("%s at %s", mba->mba_busname, pnp);
	if (strcmp(mba->mba_busname, "pci") == 0)
		aprint_normal(" bus %d", mba->mba_pba.pba_bus);
	return (UNCONF);
}
