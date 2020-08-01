/* $NetBSD: mainbus.c,v 1.5 2020/08/01 12:36:35 jdolecek Exp $ */

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.5 2020/08/01 12:36:35 jdolecek Exp $");

#include "opt_acpi.h"
#include "opt_mpbios.h"
#include "opt_pcifixup.h"
#include "opt_pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/cpuvar.h>
#include <machine/mpbiosvar.h>
#include <machine/mpacpi.h>
#include <xen/hypervisor.h>

#include "pci.h"
#include "isa.h"
#include "isadma.h"
#include "acpica.h"
#include "ipmi.h"

#if NACPICA > 0
#include <dev/acpi/acpivar.h>
#endif

#include <x86/autoconf.h>

#if NIPMI > 0
#include <x86/ipmivar.h>
#endif

#if NPCI > 0
#include <dev/pci/pcivar.h>
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

bool acpi_present = false;
bool mpacpi_active = false;

int	mainbus_rescan(device_t, const char *, const int *);
void	mainbus_childdetached(device_t, device_t);
void	mainbus_attach(device_t, device_t, void *);
int	mainbus_match(device_t, cfdata_t, void *);

CFATTACH_DECL2_NEW(mainbus, sizeof(struct mainbus_softc),
    mainbus_match, mainbus_attach,
    NULL, NULL,
    mainbus_rescan, mainbus_childdetached);

#if defined(__i386__) && !defined(XENPV)
void i386_mainbus_childdetached(device_t, device_t);
int  i386_mainbus_rescan(device_t, const char *, const int *);
void i386_mainbus_attach(device_t, device_t, void *);
#endif

#if defined(__x86_64__) && !defined(XENPV)
void amd64_mainbus_attach(device_t, device_t, void *);
#endif

static int
mainbus_cpu_print(void *aux, const char *busname)
{
	char *cpuname = aux;

	if (busname)
		aprint_normal("%s at %s", cpuname, busname);
	return UNCONF;
}

/*
 * On x86, CPUs can be enumerated and attached to mainbus in mainly two ways
 * depending on the platform (configuration): via MP BIOS tables, and via
 * ACPI tables.
 *
 * Since CPUs are not an optional part of computers, this attachment is made
 * common across all x86 architectures and modes, and thus hard-coded into
 * the boot path, with the exception of XEN PV domU.
 *
 * Along with CPUs, APICs come in various shapes and forms, and to accommodate
 * for the configurable ioapic topology, the "ioapicbus" is also enumerated
 * here as part of the mpbios/mpacpi probe path.
 *
 * All other busses are attached variously depending on the platform
 * architecture and config(5).
 *
 * These configurations and attach orderings for various platforms are
 * currently respectively driven in the functions:
 *
 *     i386_mainbus_attach();
 *     amd64_mainbus_attach();
 *     xen_mainbus_attach();
 *
 * This arrangement gives us the flexibility to do things such as dynamic
 * attach path traversal at boot time, depending on the "mode" of operation,
 * ie: virtualition aware or native.
 *
 * For (a contrived) eg: XEN PVHVM would allow us to attach pci(9) either via
 * hypervisorbus or mainbus depending on if the kernel is running under the
 * hypervisor or not.
 */

static void
x86_cpubus_attach(device_t self)
{
	int numcpus = 0;

#if NPCI > 0

#ifdef __HAVE_PCI_MSI_MSIX
	msipic_init();
#endif

	/*
	 * ACPI needs to be able to access PCI configuration space.
	 */
	pci_mode_detect();
#if defined(PCI_BUS_FIXUP)
	int pci_maxbus = 0;

	if (pci_mode_detect() != 0) {
		pci_maxbus = pci_bus_fixup(NULL, 0);
		aprint_debug("PCI bus max, after pci_bus_fixup: %i\n",
		    pci_maxbus);
#if defined(PCI_ADDR_FIXUP)
		pciaddr.extent_port = NULL;
		pciaddr.extent_mem = NULL;
		pci_addr_fixup(NULL, pci_maxbus);
#endif
	}
#endif
#endif /* NPCI */

#if NACPICA > 0
	if ((boothowto & RB_MD2) == 0 && acpi_check(self, "acpibus"))
		acpi_present = acpi_probe() != 0;
	/*
	 * First, see if the MADT contains CPUs, and possibly I/O APICs.
	 * Building the interrupt routing structures can only
	 * be done later (via a callback).
	 */
	if (acpi_present)
		mpacpi_active = mpacpi_scan_apics(self, &numcpus) != 0;

	if (!mpacpi_active) {
#endif
#ifdef MPBIOS
		if (mpbios_probe(self))
			mpbios_scan(self, &numcpus);
		else
#endif
		if (numcpus == 0) {
			struct cpu_attach_args caa;

			memset(&caa, 0, sizeof(caa));
			caa.cpu_number = 0;
			caa.cpu_role = CPU_ROLE_SP;
			caa.cpu_func = 0;

			config_found_ia(self, "cpubus", &caa, mainbus_cpu_print);
		}
#if NACPICA > 0
	}
#endif
}

int
mainbus_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

void
mainbus_attach(device_t parent, device_t self, void *aux)
{

	aprint_naive("\n");
	aprint_normal("\n");

#if defined(XENPVHVM)
	xen_hvm_init(); /* before attaching CPUs */
#endif

#if defined(XENPV)
	if (xendomain_is_dom0()) {
#endif /* XENPV */
		x86_cpubus_attach(self);

#if defined(XENPV)
	}
#endif /* XENPV */
#if defined(XEN)
	/*
	 * before isa/pci probe, so that PV devices are not probed again
	 * as emulated
	 */
	xen_mainbus_attach(parent, self, aux);
#endif
#if defined(__i386__) && !defined(XENPV)
	i386_mainbus_attach(parent, self, aux);
#elif defined(__x86_64__) && !defined(XENPV)
	amd64_mainbus_attach(parent, self, aux);
#endif
}

int
mainbus_rescan(device_t self, const char *ifattr, const int *locators)
{
#if defined(__i386__) && !defined(XEN)
	return i386_mainbus_rescan(self, ifattr, locators);
#endif
	return ENOTTY; /* Inappropriate ioctl for this device */
}

void
mainbus_childdetached(device_t self, device_t child)
{
#if defined(__i386__) && !defined(XEN)
	i386_mainbus_childdetached(self, child);
#endif
}

