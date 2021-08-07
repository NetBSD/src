/*	$NetBSD: xen_mainbus.c,v 1.10 2021/08/07 16:19:08 thorpej Exp $	*/
/*	NetBSD: mainbus.c,v 1.19 2017/05/23 08:54:39 nonaka Exp 	*/
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
__KERNEL_RCSID(0, "$NetBSD: xen_mainbus.c,v 1.10 2021/08/07 16:19:08 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include "hypervisor.h"
#include "pci.h"

#include "opt_xen.h"
#include "opt_mpbios.h"
#include "opt_pcifixup.h"

#include "acpica.h"
#include "ioapic.h"

#include "ipmi.h"

#include <machine/cpuvar.h>
#include <machine/i82093var.h>

#include <xen/xen.h>
#include <xen/hypervisor.h>

#if NIPMI > 0
#include <x86/ipmivar.h>
#endif

#if NPCI > 0
#include <dev/pci/pcivar.h>
#if NACPICA > 0
#include <dev/acpi/acpivar.h>
#include <xen/mpacpi.h>
#endif /* NACPICA > 0 */
#ifdef MPBIOS
#include <machine/mpbiosvar.h>
#endif /* MPBIOS */
#ifdef PCI_BUS_FIXUP
#include <arch/x86/pci/pci_bus_fixup.h>
#ifdef PCI_ADDR_FIXUP
#include <arch/x86/pci/pci_addr_fixup.h>
#endif
#endif

#if defined(XENPV) && (defined(MPBIOS) || NACPICA > 0)
struct mp_bus *mp_busses;
int mp_nbus;
struct mp_intr_map *mp_intrs;
int mp_nintr;

int mp_isa_bus = -1;	/* XXX */
int mp_eisa_bus = -1;	/* XXX */

#ifdef MPVERBOSE
int mp_verbose = 1;
#else /* MPVERBOSE */
int mp_verbose = 0;
#endif /* MPVERBOSE */
#endif /* defined(MPBIOS) || NACPICA > 0 */
#endif /* NPCI > 0 */

extern bool acpi_present;
extern bool mpacpi_active;

void	xen_mainbus_attach(device_t, device_t, void *);
static int	xen_mainbus_print(void *, const char *);

union xen_mainbus_attach_args {
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
 * Attach the mainbus.
 */
void
xen_mainbus_attach(device_t parent, device_t self, void *aux)
{
	union xen_mainbus_attach_args mba;

	switch(vm_guest) {
	case VM_GUEST_XENPV:
#if NIPMI > 0 && defined(XENPV)
		memset(&mba.mba_ipmi, 0, sizeof(mba.mba_ipmi));
		mba.mba_ipmi.iaa_iot = x86_bus_space_io;
		mba.mba_ipmi.iaa_memt = x86_bus_space_mem;
		if (ipmi_probe(&mba.mba_ipmi))
			config_found(self, &mba.mba_ipmi, NULL,
			    CFARGS(.iattr = "ipmibus"));
#endif
	/* FALLTHROUGH */
	case VM_GUEST_XENPVH:
	case VM_GUEST_XENPVHVM:
		mba.mba_haa.haa_busname = "hypervisor";
		config_found(self, &mba.mba_haa, xen_mainbus_print,
		    CFARGS(.iattr = "hypervisorbus"));
		break;
	default:
		return;
	}

	if (vm_guest == VM_GUEST_XENPV) {
		/* save/restore for Xen */
		if (!pmf_device_register(self, NULL, NULL))
			aprint_error_dev(self,
			    "couldn't establish power handler\n");
	}
}

static int
xen_mainbus_print(void *aux, const char *pnp)
{
	union xen_mainbus_attach_args *mba = aux;

	if (pnp)
		aprint_normal("%s at %s", mba->mba_busname, pnp);
	return UNCONF;
}
