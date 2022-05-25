/* $NetBSD: hypervisor.c,v 1.94 2022/05/25 14:35:15 bouyer Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: hypervisor.c,v 1.94 2022/05/25 14:35:15 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/sysctl.h>

#include "xenbus.h"
#include "xencons.h"
#include "isa.h"
#include "pci.h"
#include "acpica.h"
#include "kernfs.h"

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
#if defined(XENPV) && defined(DOM0OPS)
int     isa_has_been_seen;
#if NISA > 0
struct  x86_isa_chipset x86_isa_chipset;
#endif
#endif

#if defined(XENPVHVM) || defined(XENPVH)
#include <xen/include/public/arch-x86/cpuid.h>
#include <xen/include/public/arch-x86/hvm/start_info.h>
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
extern struct xenstore_domain_interface *xenstore_interface; /* XXX */

volatile shared_info_t *HYPERVISOR_shared_info __read_mostly;
paddr_t HYPERVISOR_shared_info_pa;
union start_info_union start_info_union __aligned(PAGE_SIZE);
struct hvm_start_info *hvm_start_info;

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

bool xenhvm_use_percpu_callback = 0;

static void
xen_init_hypercall_page(void)
{
	extern vaddr_t hypercall_page;
	u_int descs[4];

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
}

uint32_t hvm_start_paddr;

void init_xen_early(void);
void
init_xen_early(void)
{
	const char *cmd_line;
	if (vm_guest != VM_GUEST_XENPVH)
		return;
	xen_init_hypercall_page();
	hvm_start_info = (void *)((uintptr_t)hvm_start_paddr + KERNBASE);

	HYPERVISOR_shared_info = (void *)((uintptr_t)HYPERVISOR_shared_info_pa + KERNBASE);
	struct xen_add_to_physmap xmap = {
		.domid = DOMID_SELF,
		.space = XENMAPSPACE_shared_info,
		.idx = 0, /* Important - XEN checks for this */
		.gpfn = atop(HYPERVISOR_shared_info_pa)
	};

	int err;

	if ((err = HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xmap)) < 0) {
		printk(
		    "Xen HVM: Unable to register HYPERVISOR_shared_info %d\n", err);
	}
	delay_func = x86_delay = xen_delay;
	x86_initclock_func = xen_initclocks;
	if (hvm_start_info->cmdline_paddr != 0) {
		cmd_line =
		    (void *)((uintptr_t)hvm_start_info->cmdline_paddr + KERNBASE);
		strlcpy(xen_start_info.cmd_line, cmd_line,
		    sizeof(xen_start_info.cmd_line));
	} else {
		xen_start_info.cmd_line[0] = '\0';
	}
	xen_start_info.flags = hvm_start_info->flags;
}


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
				return true;
			default:
				return false;
			}
		}
	}
	return 0;
}

static int
xen_hvm_init_late(void)
{
	struct idt_vec *iv = &(cpu_info_primary.ci_idtvec);

	if (HYPERVISOR_xen_version(XENVER_version, NULL) < 0) {
		aprint_error("Xen HVM: hypercall page not working\n");
		return 0;
	}
	xen_init_features();

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
	xen_hvm_vec = idt_vec_alloc(iv, 129, 255);
	idt_vec_set(iv, xen_hvm_vec, &IDTVEC(hypervisor_pvhvm_callback));

	events_default_setup();
	return 1;
}

int
xen_hvm_init(void)
{
	/*
	 * We need to setup the HVM interfaces early, so that we can
	 * properly setup the CPUs later (especially, all CPUs needs to
	 * run x86_cpuid() locally to get their vcpuid.
	 *
	 * For PVH, part of it has already been done.
	 */
	if (vm_guest == VM_GUEST_XENPVH) {
		if (xen_hvm_init_late() == 0) {
			panic("hvm_init failed");
		}
		return 1;
	}

	if (vm_guest != VM_GUEST_XENHVM)
		return 0;

	/* check if hypervisor was disabled with userconf */
	if (!xen_check_hypervisordev())
		return 0;

	aprint_normal("Identified Guest XEN in HVM mode.\n");

	xen_init_hypercall_page();

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

	if (xen_hvm_init_late() == 0)
		return 0;

	struct xen_hvm_param xen_hvm_param;
	xen_hvm_param.domid = DOMID_SELF;
	xen_hvm_param.index = HVM_PARAM_CONSOLE_PFN;
	
	if ( HYPERVISOR_hvm_op(HVMOP_get_param, &xen_hvm_param) < 0) {
		aprint_debug(
		    "Xen HVM: Unable to obtain xencons page address\n");
		xen_start_info.console.domU.mfn = 0;
		xen_start_info.console.domU.evtchn = -1;
		xencons_interface = 0;
	} else {
		/* Re-use PV field */
		xen_start_info.console.domU.mfn = xen_hvm_param.value;

		pmap_kenter_pa((vaddr_t) xencons_interface,
		    ptoa(xen_start_info.console.domU.mfn),
		    VM_PROT_READ|VM_PROT_WRITE, 0);

		xen_hvm_param.domid = DOMID_SELF;
		xen_hvm_param.index = HVM_PARAM_CONSOLE_EVTCHN;

		if ( HYPERVISOR_hvm_op(HVMOP_get_param, &xen_hvm_param) < 0) {
			aprint_error(
			   "Xen HVM: Unable to obtain xencons event channel\n");
			return 0;
		}

		xen_start_info.console.domU.evtchn = xen_hvm_param.value;
	}

	/*
	 * PR port-amd64/55543
	 * workround for amazon's Xen 4.2: it looks like the Xen clock is not
	 * fully functional here. This version also doesn't support
	 * HVM_PARAM_CONSOLE_PFN. 
	 */
	if (xencons_interface != 0) {
		delay_func = x86_delay = xen_delay;
		x86_initclock_func = xen_initclocks;
	}

	vm_guest = VM_GUEST_XENPVHVM; /* Be more specific */
	return 1;
}

int
xen_hvm_init_cpu(struct cpu_info *ci)
{
	u_int32_t descs[4];
	struct xen_hvm_param xen_hvm_param;
	int error;
	static bool again = 0;

	if (!vm_guest_is_xenpvh_or_pvhvm())
		return 0;

	KASSERT(ci == curcpu());

	descs[0] = 0;
	x86_cpuid(XEN_CPUID_LEAF(4), descs);
	if (descs[0] & XEN_HVM_CPUID_VCPU_ID_PRESENT) {
		ci->ci_vcpuid = descs[1];
	} else {
		aprint_debug_dev(ci->ci_dev,
		    "Xen HVM: can't get VCPU id, falling back to ci_acpiid\n");
		ci->ci_vcpuid = ci->ci_acpiid;
	}

	xen_map_vcpu(ci);

	/* Register event callback handler. */

	xen_hvm_param.domid = DOMID_SELF;
	xen_hvm_param.index = HVM_PARAM_CALLBACK_IRQ;

	/* val[63:56] = 2, val[7:0] = vec */
	xen_hvm_param.value = ((int64_t)0x2 << 56) | xen_hvm_vec;

	/* First try to set up a per-cpu vector. */
	if (!again || xenhvm_use_percpu_callback) {
		struct xen_hvm_evtchn_upcall_vector xen_hvm_uvec;
		xen_hvm_uvec.vcpu = ci->ci_vcpuid;
		xen_hvm_uvec.vector = xen_hvm_vec;

		xenhvm_use_percpu_callback = 1;
		error = HYPERVISOR_hvm_op(
		    HVMOP_set_evtchn_upcall_vector, &xen_hvm_uvec);
		if (error < 0) {
			aprint_error_dev(ci->ci_dev,
			    "failed to set event upcall vector: %d\n", error);
			if (again)
				panic("event upcall vector");
			aprint_error_dev(ci->ci_dev,
			    "falling back to global vector\n");
			xenhvm_use_percpu_callback = 0;
		} else {
			/*
			 * From FreeBSD:
			 * Trick toolstack to think we are enlightened
			 */
			xen_hvm_param.value = 1;
			aprint_verbose_dev(ci->ci_dev,
			    "using event upcall vector: %d\n", xen_hvm_vec );
		}
	}

	if (again)
		return 1;

	if (HYPERVISOR_hvm_op(HVMOP_set_param, &xen_hvm_param) < 0) {
		aprint_error_dev(ci->ci_dev,
		    "Xen HVM: Unable to register event callback vector\n");
		vm_guest = VM_GUEST_XENHVM;
		return 0;
	}
	again = 1;
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
	if (!vm_guest_is_xenpvh_or_pvhvm())
		return 0;
#endif
	/* If we got here, it must mean we matched */
	return 1;
}

#if defined(MULTIPROCESSOR) && defined(XENPV)
static int
hypervisor_vcpu_print(void *aux, const char *parent)
{
	/* Unconfigured cpus are ignored quietly. */
	return (QUIET);
}
#endif /* MULTIPROCESSOR && XENPV */

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
	if (vm_guest == VM_GUEST_XENPVHVM) {
		/* disable emulated devices */
		if (inw(XEN_MAGIC_IOPORT) == XMI_MAGIC) {
			outw(XEN_MAGIC_IOPORT,
			    XMI_UNPLUG_IDE_DISKS | XMI_UNPLUG_NICS);
		} else {
			aprint_error_dev(self,
			    "Unable to disable emulated devices\n");
		}
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

#ifdef XENPV
	memset(&hac, 0, sizeof(hac));
	hac.hac_vcaa.vcaa_name = "vcpu";
	hac.hac_vcaa.vcaa_caa.cpu_number = 0;
	hac.hac_vcaa.vcaa_caa.cpu_role = CPU_ROLE_BP;
	hac.hac_vcaa.vcaa_caa.cpu_func = NULL; /* See xen/x86/cpu.c:vcpu_attach() */
	config_found(self, &hac.hac_vcaa, hypervisor_print,
	    CFARGS(.iattr = "xendevbus"));

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
		if (NULL == config_found(self, &hac.hac_vcaa,
					 hypervisor_vcpu_print,
					 CFARGS(.iattr = "xendevbus"))) {
			break;
		}
	}

#endif /* MULTIPROCESSOR */
#endif /* XENPV */

#if NXENBUS > 0
	extern struct x86_bus_dma_tag xenbus_bus_dma_tag;
	memset(&hac, 0, sizeof(hac));
	hac.hac_xenbus.xa_device = "xenbus";
	hac.hac_xenbus.xa_dmat = &xenbus_bus_dma_tag;
	config_found(self, &hac.hac_xenbus, hypervisor_print,
	    CFARGS(.iattr = "xendevbus"));
#endif
#if NXENCONS > 0
	if (xencons_interface != 0 || vm_guest != VM_GUEST_XENPVHVM) {
		memset(&hac, 0, sizeof(hac));
		hac.hac_xencons.xa_device = "xencons";
		config_found(self, &hac.hac_xencons, hypervisor_print,
		    CFARGS(.iattr = "xendevbus"));
	}
#endif

#if defined(DOM0OPS)
#if defined(XENPV)
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
		config_found(self, &hac.hac_acpi, NULL,
		    CFARGS(.iattr = "acpibus"));
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
	config_found(self, &hac.hac_pba, pcibusprint,
	    CFARGS(.iattr = "pcibus"));
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
		config_found(self, &hac.hac_iba, isabusprint,
		    CFARGS(.iattr = "isabus"));
	}
#endif /* NISA */
#endif /* NPCI */
#endif /* XENPV */

	if (xendomain_is_privileged()) {
		xenprivcmd_init();
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
#if NKERNFS > 0
	kernfs_entry_t *dkt;

	KERNFS_ALLOCENTRY(dkt, KM_SLEEP);
	KERNFS_INITENTRY(dkt, DT_DIR, "xen", NULL, KFSsubdir, VDIR, DIR_MODE);
	kernfs_addentry(NULL, dkt);
	kernxen_pkt = KERNFS_ENTOPARENTDIR(dkt);
#endif
}

/*
 * setup Xen's vcpu_info. requires ci_vcpuid to be initialized.
 */
void
xen_map_vcpu(struct cpu_info *ci)
{
	int size;
	uintptr_t ptr;
	struct vcpu_register_vcpu_info vcpu_info_op;
	paddr_t ma;
	int ret;

	if (ci->ci_vcpuid < XEN_LEGACY_MAX_VCPUS) {
		ci->ci_vcpu = &HYPERVISOR_shared_info->vcpu_info[ci->ci_vcpuid];
		return;
	}

	/*
	 * need to map it via VCPUOP_register_vcpu_info
	 * aligning to the smallest power-of-2 size which can contain
	 * vcpu_info ensures this. Also make sure it's cache-line aligned,
	 * for performances.
	 */
	size = CACHE_LINE_SIZE;
	while (size < sizeof(struct vcpu_info)) {
		size = size << 1;
	}
	ptr = (uintptr_t)uvm_km_alloc(kernel_map,
		    sizeof(struct vcpu_info) + size - 1, 0,
		    UVM_KMF_WIRED|UVM_KMF_ZERO);
	ptr = roundup2(ptr, size);
	ci->ci_vcpu = (struct vcpu_info *)ptr;

	pmap_extract_ma(pmap_kernel(), (ptr & ~PAGE_MASK), &ma);
	vcpu_info_op.mfn = ma >> PAGE_SHIFT;
	vcpu_info_op.offset = (ptr & PAGE_MASK);
	vcpu_info_op.rsvd = 0;

	ret = HYPERVISOR_vcpu_op(VCPUOP_register_vcpu_info,
	    ci->ci_vcpuid, &vcpu_info_op);
	if (ret) {
		panic("VCPUOP_register_vcpu_info for %d failed: %d",
		    ci->ci_vcpuid, ret);
	}
}
