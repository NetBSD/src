/* $NetBSD: hypervisor.c,v 1.73.2.6 2020/04/18 15:06:18 bouyer Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: hypervisor.c,v 1.73.2.6 2020/04/18 15:06:18 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/sysctl.h>

#include "xenbus.h"
#include "xencons.h"
#include "isa.h"
#include "pci.h"
#include "acpica.h"

#include "opt_xen.h"
#include "opt_mpbios.h"

#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/include/public/version.h>
#include <x86/pio.h>
#include <x86/machdep.h>

#include <sys/cpu.h>
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

#if defined(XENPVHVM)
#include <xen/include/public/arch-x86/cpuid.h>
#include <xen/include/public/hvm/hvm_op.h>
#include <xen/include/public/hvm/params.h>
#include <xen/include/public/vcpu.h>

#include <x86/bootinfo.h>

#define	IDTVEC(name)	__CONCAT(X, name)
typedef void (vector)(void);
extern vector IDTVEC(syscall);
extern vector IDTVEC(syscall32);
extern vector IDTVEC(osyscall);
extern vector *x86_exceptions[];

extern vector IDTVEC(hypervisor_pvhvm_callback);
extern volatile struct xencons_interface *xencons_interface; /* XXX */
extern struct xenstore_domain_interface *xenstore_interface; /* XXX */

volatile shared_info_t *HYPERVISOR_shared_info __read_mostly;
paddr_t HYPERVISOR_shared_info_pa;
union start_info_union start_info_union __aligned(PAGE_SIZE);

static int xen_hvm_vec = 0;
#endif

int xen_version;

/* power management, for save/restore */
static bool hypervisor_suspend(device_t, const pmf_qual_t *);
static bool hypervisor_resume(device_t, const pmf_qual_t *);

/* from FreeBSD */
#define XEN_MAGIC_IOPORT 0x10
enum {
	XMI_MAGIC                        = 0x49d2,
	XMI_UNPLUG_IDE_DISKS             = 0x01,
	XMI_UNPLUG_NICS                  = 0x02,
	XMI_UNPLUG_IDE_EXCEPT_PRI_MASTER = 0x04
}; 


#ifdef XENPVHVM

static bool
xen_check_hypervisordev(void)
{
	extern struct cfdata cfdata[];
	for (int i = 0; cfdata[i].cf_name != NULL; i++) {
		if (strcasecmp("hypervisor", cfdata[i].cf_name) == 0) {
			switch(cfdata[i].cf_fstate) {
			case FSTATE_NOTFOUND:
			case FSTATE_FOUND:
			case FSTATE_STAR:
				printf("xen_check_hypervisordev: enabled\n");
				return true;
			default:
				printf("xen_check_hypervisordev: disabled\n");
				return false;
			}
		}
	}
	printf("xen_check_hypervisordev: notfound\n");
	return 0;
}
int
xen_hvm_init(void)
{
	extern vaddr_t hypercall_page;
	u_int descs[4];

	/*
	 * We need to setup the HVM interfaces early, so that we can
	 * properly setup the CPUs later (especially, all CPUs needs to
	 * run x86_cpuid() locally to get their vcpuid.
	 *
	 * if everything goes fine, we switch vm_guest to VM_GUEST_XENPVHVM
	 */

	if (vm_guest != VM_GUEST_XENHVM)
		return 0;

	/* check if hypervisor was disabled with userconf */
	if (!xen_check_hypervisordev())
		return 0;

	aprint_normal("Identified Guest XEN in HVM mode.\n");

	x86_cpuid(XEN_CPUID_LEAF(2), descs);

	/* 
	 * Given 32 bytes per hypercall stub, and an optimistic number
	 * of 100 hypercalls ( the current max is 55), there shouldn't
	 * be any reason to spill over the arbitrary number of 1
	 * hypercall page. This is what we allocate in locore.S
	 * anyway. Make sure the allocation matches the registration.
	 */

	KASSERT(descs[0] == 1);

	/* XXX: vtophys(&hypercall_page) */
	wrmsr(descs[1], (uintptr_t)&hypercall_page - KERNBASE);

	if (-1 != HYPERVISOR_xen_version(XENVER_version, NULL)) {
		printf("Xen HVM: detected functional hypercall page.\n");
		xen_init_features();
	}

	/* Init various preset boot time data structures  */

	/* XEN xenstore shared page address, event channel */
	struct xen_hvm_param xen_hvm_param;

	xen_hvm_param.domid = DOMID_SELF;
	xen_hvm_param.index = HVM_PARAM_STORE_PFN;
	
	if ( HYPERVISOR_hvm_op(HVMOP_get_param, &xen_hvm_param) < 0) {
		aprint_error(
		    "Xen HVM: Unable to obtain xenstore page address\n");
		return 0;
	}

	/* Re-use PV field */
	xen_start_info.store_mfn = xen_hvm_param.value;

	pmap_kenter_pa((vaddr_t) xenstore_interface, ptoa(xen_start_info.store_mfn),
	    VM_PROT_READ|VM_PROT_WRITE, 0);

	xen_hvm_param.domid = DOMID_SELF;
	xen_hvm_param.index = HVM_PARAM_STORE_EVTCHN;

	if ( HYPERVISOR_hvm_op(HVMOP_get_param, &xen_hvm_param) < 0) {
		aprint_error(
		    "Xen HVM: Unable to obtain xenstore event channel\n");
		return 0;
	}

	xen_start_info.store_evtchn = xen_hvm_param.value;

	xen_hvm_param.domid = DOMID_SELF;
	xen_hvm_param.index = HVM_PARAM_CONSOLE_PFN;
	
	if ( HYPERVISOR_hvm_op(HVMOP_get_param, &xen_hvm_param) < 0) {
		aprint_error(
		    "Xen HVM: Unable to obtain xencons page address\n");
		return 0;
	}

	/* Re-use PV field */
	xen_start_info.console.domU.mfn = xen_hvm_param.value;

	pmap_kenter_pa((vaddr_t) xencons_interface, ptoa(xen_start_info.console.domU.mfn),
	    VM_PROT_READ|VM_PROT_WRITE, 0);

	xen_hvm_param.domid = DOMID_SELF;
	xen_hvm_param.index = HVM_PARAM_CONSOLE_EVTCHN;

	if ( HYPERVISOR_hvm_op(HVMOP_get_param, &xen_hvm_param) < 0) {
		aprint_error(
		    "Xen HVM: Unable to obtain xencons event channel\n");
		return 0;
	}

	xen_start_info.console.domU.evtchn = xen_hvm_param.value;

	/* HYPERVISOR_shared_info */
	struct xen_add_to_physmap xmap = {
		.domid = DOMID_SELF,
		.space = XENMAPSPACE_shared_info,
		.idx = 0, /* Important - XEN checks for this */
		.gpfn = atop(HYPERVISOR_shared_info_pa)
	};

	if (HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xmap) < 0) {
		aprint_error(
		    "Xen HVM: Unable to register HYPERVISOR_shared_info\n");
		return 0;
	}

	/* HYPERVISOR_shared_info va,pa has been allocated in pmap_bootstrap() */
	pmap_kenter_pa((vaddr_t) HYPERVISOR_shared_info,
	    HYPERVISOR_shared_info_pa, VM_PROT_READ|VM_PROT_WRITE, 0);

	/*
	 * First register callback: here's why
	 * http://xenbits.xen.org/gitweb/?p=xen.git;a=commit;h=7b5b8ca7dffde866d851f0b87b994e0b13e5b867
	 */

	/*
	 * Check for XENFEAT_hvm_callback_vector. Can't proceed
	 * without it.
	 */
	if (!xen_feature(XENFEAT_hvm_callback_vector)) {
		aprint_error("Xen HVM: XENFEAT_hvm_callback_vector"
		    "not available, cannot proceed");
		return 0;
	}

	/*
	 * prepare vector.
	 * We don't really care where it is, as long as it's free
	 */
	xen_hvm_vec = idt_vec_alloc(129, 255);
	idt_vec_set(xen_hvm_vec, &IDTVEC(hypervisor_pvhvm_callback));

	events_default_setup();

	delay_func = xen_delay;
	x86_initclock_func = xen_initclocks;
	x86_cpu_initclock_func = xen_cpu_initclocks;
	x86_cpu_idle_set(x86_cpu_idle_xen, "xen", true);
	vm_guest = VM_GUEST_XENPVHVM; /* Be more specific */
	return 1;
}

int
xen_hvm_init_cpu(struct cpu_info *ci)
{
	u_int32_t descs[4];
	struct xen_hvm_param xen_hvm_param;

	if (vm_guest != VM_GUEST_XENPVHVM)
		return 0;

	KASSERT(ci == curcpu());

	descs[0] = 0;
	x86_cpuid(XEN_CPUID_LEAF(4), descs);
	if (!(descs[0] & XEN_HVM_CPUID_VCPU_ID_PRESENT)) {
		aprint_error_dev(ci->ci_dev, "Xen HVM: can't get VCPU id\n");
		vm_guest = VM_GUEST_XENHVM;
		return 0;
	}
	printf("cpu %s ci_acpiid %d vcpuid %d domid %d\n",
	    device_xname(ci->ci_dev), ci->ci_acpiid, descs[1], descs[2]);

	ci->ci_vcpuid = descs[1];
	ci->ci_vcpu = &HYPERVISOR_shared_info->vcpu_info[ci->ci_vcpuid];

	/* Register event callback handler. */

	xen_hvm_param.domid = DOMID_SELF;
	xen_hvm_param.index = HVM_PARAM_CALLBACK_IRQ;

	/* val[63:56] = 2, val[7:0] = vec */
	xen_hvm_param.value = ((int64_t)0x2 << 56) | xen_hvm_vec;

	if (HYPERVISOR_hvm_op(HVMOP_set_param, &xen_hvm_param) < 0) {
		aprint_error_dev(ci->ci_dev,
		    "Xen HVM: Unable to register event callback vector\n");
		vm_guest = VM_GUEST_XENHVM;
		return 0;
	}

	return 1;
}

#endif /* XENPVHVM */

/*
 * Probe for the hypervisor; always succeeds.
 */
int
hypervisor_match(device_t parent, cfdata_t match, void *aux)
{
	struct hypervisor_attach_args *haa = aux;

	/* Attach path sanity check */
	if (strncmp(haa->haa_busname, "hypervisor", sizeof("hypervisor")) != 0)
		return 0;


#ifdef XENPVHVM
	if (vm_guest != VM_GUEST_XENPVHVM)
		return 0;
#endif
	/* If we got here, it must mean we matched */
	return 1;
}

#ifdef MULTIPROCESSOR
static int
hypervisor_vcpu_print(void *aux, const char *parent)
{
	/* Unconfigured cpus are ignored quietly. */
	return (QUIET);
}
#endif /* MULTIPROCESSOR */

/*
 * Attach the hypervisor.
 */
void
hypervisor_attach(device_t parent, device_t self, void *aux)
{

#if NPCI >0
#ifdef PCI_BUS_FIXUP
	int pci_maxbus = 0;
#endif
#endif /* NPCI */
	union hypervisor_attach_cookie hac;
	char xen_extra_version[XEN_EXTRAVERSION_LEN];
	static char xen_version_string[20];
	int rc;
	const struct sysctlnode *node = NULL;

#ifdef XENPVHVM
	/*
	 * Set the boot device to xbd0a.
	 * We claim this is a reasonable default which is picked up
	 * later as the rootfs device.
	 *
	 * We need to do this because the HVM domain loader uses the
	 * regular BIOS based native boot(8) procedure, which sets the
	 * boot device to the native driver/partition of whatever was
	 * detected by the native bootloader.
	 */

	struct btinfo_rootdevice bi;
	snprintf(bi.devname, 6, "xbd0a");
	bi.common.type = BTINFO_ROOTDEVICE;
	bi.common.len = sizeof(struct btinfo_rootdevice);

	/* From i386/multiboot.c */
	/*	$NetBSD: hypervisor.c,v 1.73.2.6 2020/04/18 15:06:18 bouyer Exp $	*/
	int i, len;
	vaddr_t data;
	extern struct bootinfo	bootinfo;
	struct bootinfo *bip = (struct bootinfo *)&bootinfo;
	len = bi.common.len;

	data = (vaddr_t)&bip->bi_data;
	for (i = 0; i < bip->bi_nentries; i++) {
		struct btinfo_common *tmp;

		tmp = (struct btinfo_common *)data;
		data += tmp->len;
	}
	if (data + len < (vaddr_t)&bip->bi_data + sizeof(bip->bi_data)) {
		memcpy((void *)data, &bi, len);
		bip->bi_nentries++;
	}

	/* disable emulated devices */
	if (inw(XEN_MAGIC_IOPORT) == XMI_MAGIC) {
		outw(XEN_MAGIC_IOPORT, XMI_UNPLUG_IDE_DISKS | XMI_UNPLUG_NICS);
	} else {
		aprint_error_dev(self, "Unable to disable emulated devices\n");
	}
#endif /* XENPVHVM */
	xenkernfs_init();

	xen_version = HYPERVISOR_xen_version(XENVER_version, NULL);
	memset(xen_extra_version, 0, sizeof(xen_extra_version));
	HYPERVISOR_xen_version(XENVER_extraversion, xen_extra_version);
	rc = snprintf(xen_version_string, 20, "%d.%d%s", XEN_MAJOR(xen_version),
		XEN_MINOR(xen_version), xen_extra_version);
	aprint_normal(": Xen version %s\n", xen_version_string);
	if (rc >= 20)
		aprint_debug(": xen_version_string truncated\n");

	sysctl_createv(NULL, 0, NULL, &node, 0,
	    CTLTYPE_NODE, "xen",
	    SYSCTL_DESCR("Xen top level node"),
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	if (node != NULL) {
		sysctl_createv(NULL, 0, &node, NULL, CTLFLAG_READONLY,
		    CTLTYPE_STRING, "version",
		    SYSCTL_DESCR("Xen hypervisor version"),
		    NULL, 0, xen_version_string, 0, CTL_CREATE, CTL_EOL);
	}

	aprint_verbose_dev(self, "features: ");
#define XEN_TST_F(n) \
	if (xen_feature(XENFEAT_##n)) \
		aprint_verbose(" %s", #n);

	XEN_TST_F(writable_page_tables);
	XEN_TST_F(writable_descriptor_tables);
	XEN_TST_F(auto_translated_physmap);
	XEN_TST_F(supervisor_mode_kernel);
	XEN_TST_F(pae_pgdir_above_4gb);
	XEN_TST_F(mmu_pt_update_preserve_ad);
	XEN_TST_F(highmem_assist);
	XEN_TST_F(gnttab_map_avail_bits);
	XEN_TST_F(hvm_callback_vector);
	XEN_TST_F(hvm_safe_pvclock);
	XEN_TST_F(hvm_pirqs);
#undef XEN_TST_F
	aprint_verbose("\n");

	xengnt_init();
	events_init();

	memset(&hac, 0, sizeof(hac));
	hac.hac_vcaa.vcaa_name = "vcpu";
	hac.hac_vcaa.vcaa_caa.cpu_number = 0;
	hac.hac_vcaa.vcaa_caa.cpu_role = CPU_ROLE_BP;
	hac.hac_vcaa.vcaa_caa.cpu_func = NULL; /* See xen/x86/cpu.c:vcpu_attach() */
	config_found_ia(self, "xendevbus", &hac.hac_vcaa, hypervisor_print);

#ifdef MULTIPROCESSOR

	/*
	 * The xenstore contains the configured number of vcpus.
	 * The xenstore however, is not accessible until much later in
	 * the boot sequence. We therefore bruteforce check for
	 * allocated vcpus (See: cpu.c:vcpu_match()) by iterating
	 * through the maximum supported by NetBSD MP.
	 */
	cpuid_t vcpuid;

	for (vcpuid = 1; vcpuid < maxcpus; vcpuid++) {
		memset(&hac, 0, sizeof(hac));
		hac.hac_vcaa.vcaa_name = "vcpu";
		hac.hac_vcaa.vcaa_caa.cpu_number = vcpuid;
		hac.hac_vcaa.vcaa_caa.cpu_role = CPU_ROLE_AP;
		hac.hac_vcaa.vcaa_caa.cpu_func = NULL; /* See xen/x86/cpu.c:vcpu_attach() */
		if (NULL == config_found_ia(self, "xendevbus", &hac.hac_vcaa,
			hypervisor_vcpu_print)) {
			break;
		}
	}

#endif /* MULTIPROCESSOR */

#if NXENBUS > 0
	memset(&hac, 0, sizeof(hac));
	hac.hac_xenbus.xa_device = "xenbus";
	config_found_ia(self, "xendevbus", &hac.hac_xenbus, hypervisor_print);
#endif
#if NXENCONS > 0
	memset(&hac, 0, sizeof(hac));
	hac.hac_xencons.xa_device = "xencons";
	config_found_ia(self, "xendevbus", &hac.hac_xencons, hypervisor_print);
#endif
#ifdef DOM0OPS
#if NPCI > 0
#if NACPICA > 0
	if (acpi_present) {
		memset(&hac, 0, sizeof(hac));
		hac.hac_acpi.aa_iot = x86_bus_space_io;
		hac.hac_acpi.aa_memt = x86_bus_space_mem;
		hac.hac_acpi.aa_pc = NULL;
		hac.hac_acpi.aa_pciflags =
			PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
			PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY |
			PCI_FLAGS_MWI_OKAY;
		hac.hac_acpi.aa_ic = &x86_isa_chipset;
		hac.hac_acpi.aa_dmat = &pci_bus_dma_tag;
#ifdef _LP64
		hac.hac_acpi.aa_dmat64 = &pci_bus_dma64_tag;
#else
		hac.hac_acpi.aa_dmat64 = NULL;
#endif /* _LP64 */
		config_found_ia(self, "acpibus", &hac.hac_acpi, 0);
	}
#endif /* NACPICA */
	memset(&hac, 0, sizeof(hac));
	hac.hac_pba.pba_iot = x86_bus_space_io;
	hac.hac_pba.pba_memt = x86_bus_space_mem;
	hac.hac_pba.pba_dmat = &pci_bus_dma_tag;
#ifdef _LP64
	hac.hac_pba.pba_dmat64 = &pci_bus_dma64_tag;
#else
	hac.hac_pba.pba_dmat64 = NULL;
#endif /* _LP64 */
	hac.hac_pba.pba_flags = PCI_FLAGS_MEM_OKAY | PCI_FLAGS_IO_OKAY;
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
		memset(&hac, 0, sizeof(hac));
		hac.hac_iba._iba_busname = "isa";
		hac.hac_iba.iba_iot = x86_bus_space_io;
		hac.hac_iba.iba_memt = x86_bus_space_mem;
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

	if (!pmf_device_register(self, hypervisor_suspend, hypervisor_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

}

static bool
hypervisor_suspend(device_t dev, const pmf_qual_t *qual)
{
#ifdef XENPV
	events_suspend();
	xengnt_suspend();
#endif
	return true;
}

static bool
hypervisor_resume(device_t dev, const pmf_qual_t *qual)
{
#ifdef XENPV
	hypervisor_machdep_resume();

	xengnt_resume();
	events_resume();
#endif
	return true;
}

static int
hypervisor_print(void *aux, const char *parent)
{
	union hypervisor_attach_cookie *hac = aux;

	if (parent)
		aprint_normal("%s at %s", hac->hac_device, parent);
	return (UNCONF);
}

#define DIR_MODE	(S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)

kernfs_parentdir_t *kernxen_pkt;

void
xenkernfs_init(void)
{
	kernfs_entry_t *dkt;

	KERNFS_ALLOCENTRY(dkt, KM_SLEEP);
	KERNFS_INITENTRY(dkt, DT_DIR, "xen", NULL, KFSsubdir, VDIR, DIR_MODE);
	kernfs_addentry(NULL, dkt);
	kernxen_pkt = KERNFS_ENTOPARENTDIR(dkt);
}
