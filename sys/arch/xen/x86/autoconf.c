/*	$NetBSD: autoconf.c,v 1.2.16.2 2008/01/09 01:50:13 matt Exp $	*/
/*	NetBSD: autoconf.c,v 1.75 2003/12/30 12:33:22 pk Exp 	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)autoconf.c	7.1 (Berkeley) 5/9/91
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.2.16.2 2008/01/09 01:50:13 matt Exp $");

#include "opt_xen.h"
#include "opt_compat_oldboot.h"
#include "opt_multiprocessor.h"
#include "opt_nfs_boot.h"
#include "xennet_hypervisor.h"
#include "xennet_xenbus.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/conf.h>
#ifdef COMPAT_OLDBOOT
#include <sys/reboot.h>
#endif
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/dkio.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kauth.h>

#ifdef NFS_BOOT_BOOTSTATIC
#include <net/if.h>
#include <net/if_ether.h>
#include <netinet/in.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsdiskless.h>
#include <xen/if_xennetvar.h>
#endif

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/gdt.h>
#include <machine/pcb.h>
#include <machine/bootinfo.h>

static void findroot(void);
static int is_valid_disk(struct device *);
static void handle_wedges(struct device *, int);

struct disklist *x86_alldisks;
int x86_ndisks;

#include "bios32.h"
#if NBIOS32 > 0
#include <machine/bios32.h>
#endif

#include "opt_pcibios.h"
#ifdef PCIBIOS
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <i386/pci/pcibios.h>
#endif

#include "opt_kvm86.h"
#ifdef KVM86
#include <machine/kvm86.h>
#endif

/*
 * Determine i/o configuration for a machine.
 */
void
cpu_configure(void)
{

	startrtclock();

#if NBIOS32 > 0 && defined(DOM0OPS)
#ifdef XEN3
	if (xen_start_info.flags & SIF_INITDOMAIN)
#endif
		bios32_init();
#endif /* NBIOS32 > 0 && DOM0OPS */
#ifdef PCIBIOS
	pcibios_init();
#endif

#ifdef KVM86
	kvm86_init();
#endif

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

#ifdef INTRDEBUG
	intr_printconfig();
#endif

	/* resync cr0 after FPU configuration */
	lwp0.l_addr->u_pcb.pcb_cr0 = rcr0();
#ifdef MULTIPROCESSOR
	/* propagate this to the idle pcb's. */
	cpu_init_idle_lwps();
#endif

	spl0();
}

void
cpu_rootconf(void)
{
	findroot();

	if (booted_wedge) {
		KASSERT(booted_device != NULL);
		printf("boot device: %s (%s)\n",
		    booted_wedge->dv_xname, booted_device->dv_xname);
		setroot(booted_wedge, 0);
	} else {
		printf("boot device: %s\n",
		    booted_device ? booted_device->dv_xname : "<unknown>");
		setroot(booted_device, booted_partition);
	}
}


/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
void
findroot(void)
{
	struct device *dv;
	union xen_cmdline_parseinfo xcp;

	if (booted_device)
		return;

	xen_parse_cmdline(XEN_PARSE_BOOTDEV, &xcp);

	TAILQ_FOREACH(dv, &alldevs, dv_list) {
		if (is_valid_disk(dv) == 0)
			continue;

		if (xcp.xcp_bootdev[0] == 0) {
			handle_wedges(dv, 0);
			break;
		}

		if (strncmp(xcp.xcp_bootdev, dv->dv_xname,
		    strlen(dv->dv_xname)))
			continue;

		if (strlen(xcp.xcp_bootdev) > strlen(dv->dv_xname)) {
			booted_partition = toupper(
				xcp.xcp_bootdev[strlen(dv->dv_xname)]) - 'A';
		}

		booted_device = dv;
		break;
	}
}

#include "pci.h"

#include <dev/isa/isavar.h>
#if NPCI > 0
#include <dev/pci/pcivar.h>
#endif


#if defined(NFS_BOOT_BOOTSTATIC) && defined(DOM0OPS)
static int
dom0_bootstatic_callback(struct nfs_diskless *nd)
{
#if 0
	struct ifnet *ifp = nd->nd_ifp;
#endif
	union xen_cmdline_parseinfo xcp;
	struct sockaddr_in *sin;

	memset(&xcp, 0, sizeof(xcp.xcp_netinfo));
	xcp.xcp_netinfo.xi_ifno = 0; /* XXX first interface hardcoded */
	xcp.xcp_netinfo.xi_root = nd->nd_root.ndm_host;
	xen_parse_cmdline(XEN_PARSE_NETINFO, &xcp);

	nd->nd_myip.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[0]);
	nd->nd_gwip.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[2]);
	nd->nd_mask.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[3]);

	sin = (struct sockaddr_in *) &nd->nd_root.ndm_saddr;
	memset((void *)sin, 0, sizeof(*sin));
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = ntohl(xcp.xcp_netinfo.xi_ip[1]);

	if (nd->nd_myip.s_addr == 0)
		return NFS_BOOTSTATIC_NOSTATIC;
	else
		return (NFS_BOOTSTATIC_HAS_MYIP|NFS_BOOTSTATIC_HAS_GWIP|
		    NFS_BOOTSTATIC_HAS_MASK|NFS_BOOTSTATIC_HAS_SERVADDR|
		    NFS_BOOTSTATIC_HAS_SERVER);
}
#endif

void
device_register(struct device *dev, void *aux)
{
	/*
	 * Handle network interfaces here, the attachment information is
	 * not available driver independently later.
	 * For disks, there is nothing useful available at attach time.
	 */
#if NXENNET_HYPERVISOR > 0 || NXENNET_XENBUS > 0 || defined(DOM0OPS)
	if (device_class(dev) == DV_IFNET) {
		union xen_cmdline_parseinfo xcp;

		xen_parse_cmdline(XEN_PARSE_BOOTDEV, &xcp);
		if (strncmp(xcp.xcp_bootdev, dev->dv_xname, 16) == 0) {
#ifdef NFS_BOOT_BOOTSTATIC
#ifdef DOM0OPS
			if (xen_start_info.flags & SIF_PRIVILEGED) {
				nfs_bootstatic_callback = dom0_bootstatic_callback;
			} else
#endif
#if NXENNET_HYPERVISOR > 0 || NXENNET_XENBUS > 0
			nfs_bootstatic_callback = xennet_bootstatic_callback;
#endif
#endif
			goto found;
		}
	}
#endif
	if (device_class(dev) == DV_IFNET) {
		struct btinfo_netif *bin = lookup_bootinfo(BTINFO_NETIF);
		if (bin == NULL)
			return;

		/*
		 * We don't check the driver name against the device name
		 * passed by the boot ROM. The ROM should stay usable
		 * if the driver gets obsoleted.
		 * The physical attachment information (checked below)
		 * must be sufficient to identify the device.
		 */

		if (bin->bus == BI_BUS_ISA &&
		    device_is_a(device_parent(dev), "isa")) {
			struct isa_attach_args *iaa = aux;

			/* compare IO base address */
			/* XXXJRT what about multiple I/O addrs? */
			if (iaa->ia_nio > 0 &&
			    bin->addr.iobase == iaa->ia_io[0].ir_addr)
				goto found;
		}
#if NPCI > 0
		if (bin->bus == BI_BUS_PCI &&
		    device_is_a(device_parent(dev), "pci")) {
			struct pci_attach_args *paa = aux;
			int b, d, f;

			/*
			 * Calculate BIOS representation of:
			 *
			 *	<bus,device,function>
			 *
			 * and compare.
			 */
			pci_decompose_tag(paa->pa_pc, paa->pa_tag, &b, &d, &f);
			if (bin->addr.tag == ((b << 8) | (d << 3) | f))
				goto found;
		}
#endif
	}
	return;

found:
	if (booted_device) {
		/* XXX should be a "panic()" */
		printf("warning: double match for boot device (%s, %s)\n",
		    booted_device->dv_xname, dev->dv_xname);
		return;
	}
	booted_device = dev;
}

static void
handle_wedges(struct device *dv, int par)
{
	if (config_handle_wedges(dv, par) == 0)
		return;
	booted_device = dv;
	booted_partition = par;
}

static int
is_valid_disk(struct device *dv)
{

	if (device_class(dv) != DV_DISK)
		return (0);

	return (device_is_a(dv, "dk") ||
		device_is_a(dv, "sd") ||
		device_is_a(dv, "wd") ||
		device_is_a(dv, "ld") ||
		device_is_a(dv, "ed") ||
		device_is_a(dv, "xbd"));
}
