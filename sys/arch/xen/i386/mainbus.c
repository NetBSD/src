/*	$NetBSD: mainbus.c,v 1.10 2006/10/01 21:36:11 bouyer Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.10 2006/10/01 21:36:11 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include "hypervisor.h"
#include "pci.h"

#include "opt_xen.h"
#include "opt_mpbios.h"

#include "acpi.h"
#include "ioapic.h"

#include "ipmi.h"

#include <machine/cpuvar.h>
#include <machine/i82093var.h>

#include <machine/xen.h>
#include <machine/hypervisor.h>

#if NIPMI > 0
#include <x86/ipmivar.h>
#endif

#if defined(XEN3) && NPCI > 0
#include <dev/pci/pcivar.h>
#if NACPI > 0
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_madt.h>       
#include <machine/mpacpi.h>       
#endif /* NACPI > 0 */
#ifdef MPBIOS
#include <machine/mpbiosvar.h>       
#endif /* MPBIOS */

#if defined(MPBIOS) || NACPI > 0
struct mp_bus *mp_busses;
int mp_nbus;
struct mp_intr_map *mp_intrs;
int mp_nintr;
 
int mp_isa_bus = -1;	    /* XXX */
int mp_eisa_bus = -1;	   /* XXX */

int acpi_present = 0;
int mpacpi_active = 0;
#ifdef MPVERBOSE
int mp_verbose = 1;
#else /* MPVERBOSE */
int mp_verbose = 0;
#endif /* MPVERBOSE */
#endif /* defined(MPBIOS) || NACPI > 0 */
#endif /* defined(XEN3) && NPCI > 0 */


int	mainbus_match(struct device *, struct cfdata *, void *);
void	mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

int	mainbus_print(void *, const char *);

union mainbus_attach_args {
	const char *mba_busname;		/* first elem of all */
	struct cpu_attach_args mba_caa;
#if NHYPERVISOR > 0
	struct hypervisor_attach_args mba_haa;
#endif
#if NIPMI > 0
	struct ipmi_attach_args mba_ipmi;
#endif
};

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
#if defined(DOM0OPS) && defined(XEN3)
	int numcpus = 0;
#ifdef MPBIOS
	int mpbios_present = 0;
#endif
#if NACPI > 0 || defined(MPBIOS)
	int numioapics = 0;     
#endif
#endif /* defined(DOM0OPS) && defined(XEN3) */

	printf("\n");

#ifndef XEN3
	memset(&mba.mba_caa, 0, sizeof(mba.mba_caa));
	mba.mba_caa.caa_name = "cpu";
	mba.mba_caa.cpu_number = 0;
	mba.mba_caa.cpu_role = CPU_ROLE_SP;
	mba.mba_caa.cpu_func = 0;
	config_found_ia(self, "cpubus", &mba.mba_caa, mainbus_print);
#else /* XEN3 */
#ifdef DOM0OPS
	if (xen_start_info.flags & SIF_INITDOMAIN) {
#ifdef MPBIOS
		mpbios_present = mpbios_probe(self);
#endif
#if NPCI > 0
		/* ACPI needs to be able to access PCI configuration space. */
		pci_mode = pci_mode_detect();
#ifdef PCI_BUS_FIXUP
		pci_maxbus = pci_bus_fixup(NULL, 0);
		aprint_debug("PCI bus max, after pci_bus_fixup: %i\n",
		    pci_maxbus);
#ifdef PCI_ADDR_FIXUP
		pciaddr.extent_port = NULL;
		pciaddr.extent_mem = NULL;
		pci_addr_fixup(NULL, pci_maxbus);
#endif /* PCI_ADDR_FIXUP */
#endif /* PCI_BUS_FIXUP */
#if NACPI > 0
		acpi_present = acpi_probe();
		if (acpi_present)
			mpacpi_active = mpacpi_scan_apics(self,
			    &numcpus, &numioapics);
		if (!mpacpi_active)
#endif
		{
#ifdef MPBIOS
			if (mpbios_present)
				mpbios_scan(self, &numcpus, &numioapics);       
			else
#endif
			if (numcpus == 0) {
				memset(&mba.mba_caa, 0, sizeof(mba.mba_caa));
				mba.mba_caa.caa_name = "cpu";
				mba.mba_caa.cpu_number = 0;
				mba.mba_caa.cpu_role = CPU_ROLE_SP;
				mba.mba_caa.cpu_func = 0;
				config_found_ia(self, "cpubus",
				    &mba.mba_caa, mainbus_print);
			}
		}
#if NIOAPIC > 0
	ioapic_enable();
#endif
#endif /* NPCI */
	}
#endif /* DOM0OPS */
#endif /* XEN3 */

#if NIPMI > 0
	memset(&mba.mba_ipmi, 0, sizeof(mba.mba_ipmi));
	mba.mba_ipmi.iaa_iot = X86_BUS_SPACE_IO;
	mba.mba_ipmi.iaa_memt = X86_BUS_SPACE_MEM;
	if (ipmi_probe(&mba.mba_ipmi))
		config_found_ia(self, "ipmibus", &mba.mba_ipmi, 0);
#endif

#if NHYPERVISOR > 0
	mba.mba_haa.haa_busname = "hypervisor";
	config_found_ia(self, "hypervisorbus", &mba.mba_haa, mainbus_print);
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
	return (UNCONF);
}
