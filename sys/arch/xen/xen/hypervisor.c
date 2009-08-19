/* $NetBSD: hypervisor.c,v 1.36.4.2 2009/08/19 18:46:55 yamt Exp $ */

/*
 * Copyright (c) 2005 Manuel Bouyer.
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
 *      This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 * Copyright (c) 2004 Christian Limpach.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: hypervisor.c,v 1.36.4.2 2009/08/19 18:46:55 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include "xenbus.h"
#include "xencons.h"
#ifndef __x86_64__
#include "npx.h"
#else
#define NNPX 0
#endif /* __x86_64__ */
#include "isa.h"
#include "pci.h"
#include "acpica.h"

#include "opt_xen.h"
#include "opt_mpbios.h"

#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/xen3-public/version.h>

#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/tree.h>
#include <sys/vnode.h>
#include <miscfs/specfs/specdev.h>
#include <miscfs/kernfs/kernfs.h>
#include <xen/kernfs_machdep.h>
#include <dev/isa/isavar.h>
#include <xen/granttables.h>
#include <xen/vcpuvar.h>
#if NPCI > 0
#include <dev/pci/pcivar.h>
#if NACPICA > 0
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_madt.h>       
#include <machine/mpconfig.h>
#include <xen/mpacpi.h>       
#endif
#ifdef MPBIOS
#include <machine/mpbiosvar.h>       
#endif
#endif /* NPCI */

#if NXENBUS > 0
#include <xen/xenbus.h>
#endif

#if NXENNET_HYPERVISOR > 0
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <xen/if_xennetvar.h>
#endif

#if NXBD_HYPERVISOR > 0
#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/bufq.h>
#include <dev/dkvar.h>
#include <xen/xbdvar.h>
#endif

int	hypervisor_match(device_t, cfdata_t, void *);
void	hypervisor_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(hypervisor, 0,
    hypervisor_match, hypervisor_attach, NULL, NULL);

static int hypervisor_print(void *, const char *);

union hypervisor_attach_cookie {
	const char *hac_device;		/* first elem of all */
#if NXENCONS > 0
	struct xencons_attach_args hac_xencons;
#endif
#if NXENBUS > 0
	struct xenbus_attach_args hac_xenbus;
#endif
#if NXENNET_HYPERVISOR > 0
	struct xennet_attach_args hac_xennet;
#endif
#if NXBD_HYPERVISOR > 0
	struct xbd_attach_args hac_xbd;
#endif
#if NNPX > 0
	struct xen_npx_attach_args hac_xennpx;
#endif
#if NPCI > 0
	struct pcibus_attach_args hac_pba;
#if defined(DOM0OPS) && NISA > 0
	struct isabus_attach_args hac_iba;
#endif
#if NACPICA > 0
	struct acpibus_attach_args hac_acpi;
#endif
#endif /* NPCI */
	struct vcpu_attach_args hac_vcaa;
};

/* 
 * This is set when the ISA bus is attached.  If it's not set by the
 * time it's checked below, then mainbus attempts to attach an ISA. 
 */   
#ifdef DOM0OPS
int     isa_has_been_seen;
#if NISA > 0
struct  x86_isa_chipset x86_isa_chipset;
#endif
#endif

/*
 * Probe for the hypervisor; always succeeds.
 */
int
hypervisor_match(device_t parent, cfdata_t match, void *aux)
{
	struct hypervisor_attach_args *haa = aux;

	if (strcmp(haa->haa_busname, "hypervisor") == 0)
		return 1;
	return 0;
}

/*
 * Attach the hypervisor.
 */
void
hypervisor_attach(device_t parent, device_t self, void *aux)
{
	int xen_version;
#if NPCI >0
#ifdef PCI_BUS_FIXUP
	int pci_maxbus = 0;
#endif
#endif /* NPCI */
	union hypervisor_attach_cookie hac;

#ifdef DOM0OPS
	if (xendomain_is_privileged()) {
		xenkernfs_init();
	}
#endif
	xen_version = HYPERVISOR_xen_version(XENVER_version, NULL);
	aprint_normal(": Xen version %d.%d\n", (xen_version & 0xffff0000) >> 16,
	       xen_version & 0x0000ffff);

	xengnt_init();

	memset(&hac.hac_vcaa, 0, sizeof(hac.hac_vcaa));
	hac.hac_vcaa.vcaa_name = "vcpu";
	hac.hac_vcaa.vcaa_caa.cpu_number = 0;
	hac.hac_vcaa.vcaa_caa.cpu_role = CPU_ROLE_SP;
	hac.hac_vcaa.vcaa_caa.cpu_func = 0;
	config_found_ia(self, "xendevbus", &hac.hac_vcaa, hypervisor_print);

	events_init();

#if NXENBUS > 0
	hac.hac_xenbus.xa_device = "xenbus";
	config_found_ia(self, "xendevbus", &hac.hac_xenbus, hypervisor_print);
#endif
#if NXENCONS > 0
	hac.hac_xencons.xa_device = "xencons";
	config_found_ia(self, "xendevbus", &hac.hac_xencons, hypervisor_print);
#endif
#if NXENNET_HYPERVISOR > 0
	hac.hac_xennet.xa_device = "xennet";
	xennet_scan(self, &hac.hac_xennet, hypervisor_print);
#endif
#if NXBD_HYPERVISOR > 0
	hac.hac_xbd.xa_device = "xbd";
	xbd_scan(self, &hac.hac_xbd, hypervisor_print);
#endif
#if NNPX > 0
	hac.hac_xennpx.xa_device = "npx";
	config_found_ia(self, "xendevbus", &hac.hac_xennpx, hypervisor_print);
#endif
#ifdef DOM0OPS
#if NPCI > 0
#if NACPICA > 0
	if (acpi_present) {
		hac.hac_acpi.aa_iot = X86_BUS_SPACE_IO;
		hac.hac_acpi.aa_memt = X86_BUS_SPACE_MEM;
		hac.hac_acpi.aa_pc = NULL;
		hac.hac_acpi.aa_pciflags =
			PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
			PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY |
			PCI_FLAGS_MWI_OKAY;
		hac.hac_acpi.aa_ic = &x86_isa_chipset;
		config_found_ia(self, "acpibus", &hac.hac_acpi, 0);
	}
#endif /* NACPICA */
	hac.hac_pba.pba_iot = X86_BUS_SPACE_IO;
	hac.hac_pba.pba_memt = X86_BUS_SPACE_MEM;
	hac.hac_pba.pba_dmat = &pci_bus_dma_tag;
#ifdef _LP64
	hac.hac_pba.pba_dmat64 = &pci_bus_dma64_tag;
#else
	hac.hac_pba.pba_dmat64 = NULL;
#endif /* _LP64 */
	hac.hac_pba.pba_flags = PCI_FLAGS_MEM_ENABLED | PCI_FLAGS_IO_ENABLED;
	hac.hac_pba.pba_bridgetag = NULL;
	hac.hac_pba.pba_bus = 0;
#if NACPICA > 0 && defined(ACPI_SCANPCI)
	if (mpacpi_active)
		mp_pci_scan(self, &hac.hac_pba, pcibusprint);
	else
#endif
#if defined(MPBIOS) && defined(MPBIOS_SCANPCI)
	if (mpbios_scanned != 0)
		mp_pci_scan(self, &hac.hac_pba, pcibusprint);
	else
#endif
	config_found_ia(self, "pcibus", &hac.hac_pba, pcibusprint);
#if NACPICA > 0
	if (mp_verbose)
		acpi_pci_link_state();
#endif
#if NISA > 0
	if (isa_has_been_seen == 0) {
		hac.hac_iba._iba_busname = "isa";
		hac.hac_iba.iba_iot = X86_BUS_SPACE_IO;
		hac.hac_iba.iba_memt = X86_BUS_SPACE_MEM;
		hac.hac_iba.iba_dmat = &isa_bus_dma_tag;
		hac.hac_iba.iba_ic = NULL; /* No isa DMA yet */
		config_found_ia(self, "isabus", &hac.hac_iba, isabusprint);
	}
#endif /* NISA */
#endif /* NPCI */

	if (xendomain_is_privileged()) {
		xenprivcmd_init();
		xen_shm_init();
	}
#endif /* DOM0OPS */

	hypervisor_machdep_attach();
}

static int
hypervisor_print(void *aux, const char *parent)
{
	union hypervisor_attach_cookie *hac = aux;

	if (parent)
		aprint_normal("%s at %s", hac->hac_device, parent);
	return (UNCONF);
}

#if defined(DOM0OPS)

#define DIR_MODE	(S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)

kernfs_parentdir_t *kernxen_pkt;

void
xenkernfs_init(void)
{
	kernfs_entry_t *dkt;

	KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
	KERNFS_INITENTRY(dkt, DT_DIR, "xen", NULL, KFSsubdir, VDIR, DIR_MODE);
	kernfs_addentry(NULL, dkt);
	kernxen_pkt = KERNFS_ENTOPARENTDIR(dkt);
}
#endif /* DOM0OPS */
