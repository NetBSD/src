/*	$NetBSD: acpi.c,v 1.140 2010/01/05 13:39:49 jruoho Exp $	*/

/*-
 * Copyright (c) 2003, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum of By Noon Software, Inc.
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

/*
 * Copyright 2001, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Autoconfiguration support for the Intel ACPI Component Architecture
 * ACPI reference implementation.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi.c,v 1.140 2010/01/05 13:39:49 jruoho Exp $");

#include "opt_acpi.h"
#include "opt_pcifixup.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/sysctl.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_osd.h>
#include <dev/acpi/acpi_timer.h>
#include <dev/acpi/acpi_wakedev.h>
#include <dev/acpi/acpi_pci.h>
#ifdef ACPIVERBOSE
#include <dev/acpi/acpidevs_data.h>
#endif

#define _COMPONENT          ACPI_TOOLS 
ACPI_MODULE_NAME            ("acpi")

#if defined(ACPI_PCI_FIXUP)
#error The option ACPI_PCI_FIXUP has been obsoleted by PCI_INTR_FIXUP_DISABLED.  Please adjust your kernel configuration file.
#endif

#ifdef PCI_INTR_FIXUP_DISABLED
#include <dev/pci/pcidevs.h>
#endif

MALLOC_DECLARE(M_ACPI);

#include <machine/acpi_machdep.h>

#ifdef ACPI_DEBUGGER
#define	ACPI_DBGR_INIT		0x01
#define	ACPI_DBGR_TABLES	0x02
#define	ACPI_DBGR_ENABLE	0x04
#define	ACPI_DBGR_PROBE		0x08
#define	ACPI_DBGR_RUNNING	0x10

static int acpi_dbgr = 0x00;
#endif

static ACPI_TABLE_DESC	acpi_initial_tables[128];

static int	acpi_match(device_t, cfdata_t, void *);
static void	acpi_attach(device_t, device_t, void *);
static void	acpi_childdet(device_t, device_t);
static int	acpi_detach(device_t, int);

static int	acpi_rescan(device_t, const char *, const int *);
static void	acpi_rescan1(struct acpi_softc *, const char *, const int *);
static void	acpi_rescan_nodes(struct acpi_softc *);

static int	acpi_print(void *aux, const char *);

static int	sysctl_hw_acpi_sleepstate(SYSCTLFN_ARGS);

extern struct cfdriver acpi_cd;

CFATTACH_DECL2_NEW(acpi, sizeof(struct acpi_softc),
    acpi_match, acpi_attach, acpi_detach, NULL, acpi_rescan, acpi_childdet);

/*
 * This is a flag we set when the ACPI subsystem is active.  Machine
 * dependent code may wish to skip other steps (such as attaching
 * subsystems that ACPI supercedes) when ACPI is active.
 */
int	acpi_active;
int	acpi_force_load;
int	acpi_suspended = 0;

/*
 * Pointer to the ACPI subsystem's state.  There can be only
 * one ACPI instance.
 */
struct acpi_softc *acpi_softc;

/*
 * Locking stuff.
 */
extern kmutex_t acpi_interrupt_list_mtx;

/*
 * Ignored HIDs
 */
static const char * const acpi_ignored_ids[] = {
#if defined(i386) || defined(x86_64)
	"PNP0000",	/* AT interrupt controller is handled internally */
	"PNP0200",	/* AT DMA controller is handled internally */
	"PNP0A??",	/* PCI Busses are handled internally */
	"PNP0B00",	/* AT RTC is handled internally */
	"PNP0C01",	/* No "System Board" driver */
	"PNP0C02",	/* No "PnP motherboard register resources" driver */
	"PNP0C0F",	/* ACPI PCI link devices are handled internally */
	"INT0800",	/* Intel HW RNG is handled internally */
#endif
#if defined(x86_64)
	"PNP0C04",	/* FPU is handled internally */
#endif
	NULL
};

/*
 * sysctl-related information
 */

static uint64_t acpi_root_pointer;	/* found as hw.acpi.root */
static int acpi_sleepstate = ACPI_STATE_S0;
static char acpi_supported_states[3 * 6 + 1] = "";

/*
 * Prototypes.
 */
static void		acpi_build_tree(struct acpi_softc *);
static ACPI_STATUS	acpi_make_devnode(ACPI_HANDLE, UINT32, void *, void **);

static void		acpi_enable_fixed_events(struct acpi_softc *);

static ACPI_TABLE_HEADER *acpi_map_rsdt(void);
static void		acpi_unmap_rsdt(ACPI_TABLE_HEADER *);
static int		is_available_state(struct acpi_softc *, int);

static bool		acpi_suspend(device_t PMF_FN_PROTO);
static bool		acpi_resume(device_t PMF_FN_PROTO);

/*
 * acpi_probe:
 *
 *	Probe for ACPI support.  This is called by the
 *	machine-dependent ACPI front-end.  All of the
 *	actual work is done by ACPICA.
 *
 *	NOTE: This is not an autoconfiguration interface function.
 */
int
acpi_probe(void)
{
	static int beenhere;
	ACPI_TABLE_HEADER *rsdt;
	ACPI_STATUS rv;

	if (beenhere != 0)
		panic("acpi_probe: ACPI has already been probed");
	beenhere = 1;

	mutex_init(&acpi_interrupt_list_mtx, MUTEX_DEFAULT, IPL_NONE);

	/*
	 * Start up ACPICA.
	 */
#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_INIT)
		acpi_osd_debugger();
#endif

	AcpiGbl_AllMethodsSerialized = FALSE;
	AcpiGbl_EnableInterpreterSlack = TRUE;

	rv = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(rv)) {
		printf("ACPI: unable to initialize ACPICA: %s\n",
		    AcpiFormatException(rv));
		return 0;
	}

	rv = AcpiInitializeTables(acpi_initial_tables, 128, 0);
	if (ACPI_FAILURE(rv)) {
#ifdef ACPI_DEBUG
		printf("ACPI: unable to initialize ACPI tables: %s\n",
		    AcpiFormatException(rv));
#endif
		AcpiTerminate();
		return 0;
	}

	rv = AcpiReallocateRootTable();
	if (ACPI_FAILURE(rv)) {
		printf("ACPI: unable to reallocate root table: %s\n",
		    AcpiFormatException(rv));
		AcpiTerminate();
		return 0;
	}

#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_TABLES)
		acpi_osd_debugger();
#endif

	rv = AcpiLoadTables();
	if (ACPI_FAILURE(rv)) {
		printf("ACPI: unable to load tables: %s\n",
		    AcpiFormatException(rv));
		AcpiTerminate();
		return 0;
	}

	rsdt = acpi_map_rsdt();
	if (rsdt == NULL) {
		printf("ACPI: unable to map RSDT\n");
		AcpiTerminate();
		return 0;
	}

	if (!acpi_force_load && (acpi_find_quirks() & ACPI_QUIRK_BROKEN)) {
		printf("ACPI: BIOS implementation in listed as broken:\n");
		printf("ACPI: X/RSDT: OemId <%6.6s,%8.8s,%08x>, "
		       "AslId <%4.4s,%08x>\n",
			rsdt->OemId, rsdt->OemTableId,
		        rsdt->OemRevision,
			rsdt->AslCompilerId,
		        rsdt->AslCompilerRevision);
		printf("ACPI: not used. set acpi_force_load to use anyway.\n");
		acpi_unmap_rsdt(rsdt);
		AcpiTerminate();
		return 0;
	}

	acpi_unmap_rsdt(rsdt);

#if notyet
	/* Install the default address space handlers. */
	rv = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
	    ACPI_ADR_SPACE_SYSTEM_MEMORY, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(rv)) {
		printf("ACPI: unable to initialize SystemMemory handler: %s\n",
		    AcpiFormatException(rv));
		AcpiTerminate();
		return 0;
	}
	rv = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
	    ACPI_ADR_SPACE_SYSTEM_IO, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(rv)) {
		printf("ACPI: unable to initialize SystemIO handler: %s\n",
		     AcpiFormatException(rv));
		AcpiTerminate();
		return 0;
	}
	rv = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
	    ACPI_ADR_SPACE_PCI_CONFIG, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(rv)) {
		printf("ACPI: unable to initialize PciConfig handler: %s\n",
		    AcpiFormatException(rv));
		AcpiTerminate();
		return 0;
	}
#endif

	rv = AcpiEnableSubsystem(~(ACPI_NO_HARDWARE_INIT|ACPI_NO_ACPI_ENABLE));
	if (ACPI_FAILURE(rv)) {
		printf("ACPI: unable to enable: %s\n", AcpiFormatException(rv));
		AcpiTerminate();
		return 0;
	}

	/*
	 * Looks like we have ACPI!
	 */

	return 1;
}

static int
acpi_submatch(device_t parent, cfdata_t cf, const int *locs, void *aux)
{
	struct cfattach *ca;

	ca = config_cfattach_lookup(cf->cf_name, cf->cf_atname);
	return (ca == &acpi_ca);
}

int
acpi_check(device_t parent, const char *ifattr)
{
	return (config_search_ia(acpi_submatch, parent, ifattr, NULL) != NULL);
}

ACPI_PHYSICAL_ADDRESS
acpi_OsGetRootPointer(void)
{
	ACPI_PHYSICAL_ADDRESS PhysicalAddress;

	/*
	 * IA-32: Use AcpiFindRootPointer() to locate the RSDP.
	 *
	 * IA-64: Use the EFI.
	 *
	 * We let MD code handle this since there are multiple
	 * ways to do it.
	 */

	PhysicalAddress = acpi_md_OsGetRootPointer();

	if (acpi_root_pointer == 0)
		acpi_root_pointer = PhysicalAddress;

	return PhysicalAddress;
}

/*
 * acpi_match:
 *
 *	Autoconfiguration `match' routine.
 */
static int
acpi_match(device_t parent, cfdata_t match, void *aux)
{
	/*
	 * XXX Check other locators?  Hard to know -- machine
	 * dependent code has already checked for the presence
	 * of ACPI by calling acpi_probe(), so I suppose we
	 * don't really have to do anything else.
	 */
	return 1;
}

/* Remove references to child devices.
 *
 * XXX Need to reclaim any resources?
 */
static void
acpi_childdet(device_t self, device_t child)
{
	struct acpi_softc *sc = device_private(self);
	struct acpi_scope *as;
	struct acpi_devnode *ad;

	if (sc->sc_apmbus == child)
		sc->sc_apmbus = NULL;

	TAILQ_FOREACH(as, &sc->sc_scopes, as_list) {
		TAILQ_FOREACH(ad, &as->as_devnodes, ad_list) {
			if (ad->ad_device == child)
				ad->ad_device = NULL;
		}
	}
}

/*
 * acpi_attach:
 *
 *	Autoconfiguration `attach' routine.  Finish initializing
 *	ACPICA (some initialization was done in acpi_probe(),
 *	which was required to check for the presence of ACPI),
 *	and enable the ACPI subsystem.
 */
static void
acpi_attach(device_t parent, device_t self, void *aux)
{
	struct acpi_softc *sc = device_private(self);
	struct acpibus_attach_args *aa = aux;
	ACPI_STATUS rv;
	ACPI_TABLE_HEADER *rsdt;

	aprint_naive("\n");
	aprint_normal(": Intel ACPICA %08x\n", ACPI_CA_VERSION);

	if (acpi_softc != NULL)
		panic("acpi_attach: ACPI has already been attached");

	sysmon_power_settype("acpi");

	rsdt = acpi_map_rsdt();
	if (rsdt) {
		aprint_verbose_dev(
		    self,
		    "X/RSDT: OemId <%6.6s,%8.8s,%08x>, AslId <%4.4s,%08x>\n",
		    rsdt->OemId, rsdt->OemTableId,
		    rsdt->OemRevision,
		    rsdt->AslCompilerId, rsdt->AslCompilerRevision);
	} else
		aprint_error_dev(self, "X/RSDT: Not found\n");
	acpi_unmap_rsdt(rsdt);

	sc->sc_dev = self;
	sc->sc_quirks = acpi_find_quirks();

	sc->sc_iot = aa->aa_iot;
	sc->sc_memt = aa->aa_memt;
	sc->sc_pc = aa->aa_pc;
	sc->sc_pciflags = aa->aa_pciflags;
	sc->sc_ic = aa->aa_ic;

	acpi_softc = sc;

	/*
	 * Register null power management handler
	 */
	if (!pmf_device_register(self, acpi_suspend, acpi_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/*
	 * Bring ACPI on-line.
	 */
#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_ENABLE)
		acpi_osd_debugger();
#endif

#define ACPI_ENABLE_PHASE1 \
    (ACPI_NO_HANDLER_INIT | ACPI_NO_EVENT_INIT)
#define ACPI_ENABLE_PHASE2 \
    (ACPI_NO_HARDWARE_INIT | ACPI_NO_ACPI_ENABLE | \
     ACPI_NO_ADDRESS_SPACE_INIT)

	rv = AcpiEnableSubsystem(ACPI_ENABLE_PHASE1);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "unable to enable ACPI: %s\n",
		    AcpiFormatException(rv));
		return;
	}

	acpi_md_callback();

	rv = AcpiEnableSubsystem(ACPI_ENABLE_PHASE2);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "unable to enable ACPI: %s\n",
		    AcpiFormatException(rv));
		return;
	}

	/* early EC handler initialization if ECDT table is available */
	config_found_ia(self, "acpiecdtbus", NULL, NULL);

	rv = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self,
		    "unable to initialize ACPI objects: %s\n",
		    AcpiFormatException(rv));
		return;
	}
	acpi_active = 1;

	/* Our current state is "awake". */
	sc->sc_sleepstate = ACPI_STATE_S0;

	/* Show SCI interrupt. */
	aprint_verbose_dev(self, "SCI interrupting at int %d\n",
	    AcpiGbl_FADT.SciInterrupt);

	/*
	 * Check for fixed-hardware features.
	 */
	acpi_enable_fixed_events(sc);
	acpitimer_init();

	/*
	 * Scan the namespace and build our device tree.
	 */
#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_PROBE)
		acpi_osd_debugger();
#endif
	acpi_build_tree(sc);

	snprintf(acpi_supported_states, sizeof(acpi_supported_states),
	    "%s%s%s%s%s%s",
	    is_available_state(sc, ACPI_STATE_S0) ? "S0 " : "",
	    is_available_state(sc, ACPI_STATE_S1) ? "S1 " : "",
	    is_available_state(sc, ACPI_STATE_S2) ? "S2 " : "",
	    is_available_state(sc, ACPI_STATE_S3) ? "S3 " : "",
	    is_available_state(sc, ACPI_STATE_S4) ? "S4 " : "",
	    is_available_state(sc, ACPI_STATE_S5) ? "S5 " : "");

#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_RUNNING)
		acpi_osd_debugger();
#endif
}

static int
acpi_detach(device_t self, int flags)
{
	int rc;

#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_RUNNING)
		acpi_osd_debugger();
#endif

	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;

#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_PROBE)
		acpi_osd_debugger();
#endif

	if ((rc = acpitimer_detach()) != 0)
		return rc;

#if 0
	/*
	 * Bring ACPI on-line.
	 */
#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_ENABLE)
		acpi_osd_debugger();
#endif

#define ACPI_ENABLE_PHASE1 \
    (ACPI_NO_HANDLER_INIT | ACPI_NO_EVENT_INIT)
#define ACPI_ENABLE_PHASE2 \
    (ACPI_NO_HARDWARE_INIT | ACPI_NO_ACPI_ENABLE | \
     ACPI_NO_ADDRESS_SPACE_INIT)

	rv = AcpiEnableSubsystem(ACPI_ENABLE_PHASE1);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "unable to enable ACPI: %s\n",
		    AcpiFormatException(rv));
		return;
	}

	rv = AcpiEnableSubsystem(ACPI_ENABLE_PHASE2);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "unable to enable ACPI: %s\n",
		    AcpiFormatException(rv));
		return;
	}

	/* early EC handler initialization if ECDT table is available */
	config_found_ia(self, "acpiecdtbus", NULL, NULL);

	rv = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self,
		    "unable to initialize ACPI objects: %s\n",
		    AcpiFormatException(rv));
		return;
	}
	acpi_active = 1;

	acpi_enable_fixed_events(sc);
#endif

	pmf_device_deregister(self);

#if 0
	sysmon_power_settype("acpi");
#endif
	acpi_softc = NULL;

	return 0;
}

static bool
acpi_suspend(device_t dv PMF_FN_ARGS)
{
	acpi_suspended = 1;
	return true;
}

static bool
acpi_resume(device_t dv PMF_FN_ARGS)
{
	acpi_suspended = 0;
	return true;
}

#if 0
/*
 * acpi_disable:
 *
 *	Disable ACPI.
 */
static ACPI_STATUS
acpi_disable(struct acpi_softc *sc)
{
	ACPI_STATUS rv = AE_OK;

	if (acpi_active) {
		rv = AcpiDisable();
		if (ACPI_SUCCESS(rv))
			acpi_active = 0;
	}
	return rv;
}
#endif

struct acpi_make_devnode_state {
	struct acpi_softc *softc;
	struct acpi_scope *scope;
};

/*
 * acpi_build_tree:
 *
 *	Scan relevant portions of the ACPI namespace and attach
 *	child devices.
 */
static void
acpi_build_tree(struct acpi_softc *sc)
{
	static const char *scopes[] = {
		"\\_PR_",	/* ACPI 1.0 processor namespace */
		"\\_SB_",	/* system bus namespace */
		"\\_SI_",	/* system indicator namespace */
		"\\_TZ_",	/* ACPI 1.0 thermal zone namespace */
		NULL,
	};
	struct acpi_make_devnode_state state;
	struct acpi_scope *as;
	ACPI_HANDLE parent;
	ACPI_STATUS rv;
	int i;

	TAILQ_INIT(&sc->sc_scopes);

	state.softc = sc;

	/*
	 * Scan the namespace and build our tree.
	 */
	for (i = 0; scopes[i] != NULL; i++) {
		as = malloc(sizeof(*as), M_ACPI, M_WAITOK);
		as->as_name = scopes[i];
		TAILQ_INIT(&as->as_devnodes);

		TAILQ_INSERT_TAIL(&sc->sc_scopes, as, as_list);

		state.scope = as;

		rv = AcpiGetHandle(ACPI_ROOT_OBJECT, scopes[i],
		    &parent);
		if (ACPI_SUCCESS(rv)) {
			AcpiWalkNamespace(ACPI_TYPE_ANY, parent, 100,
			    acpi_make_devnode, &state, NULL);
		}
	}

	acpi_rescan1(sc, NULL, NULL);

	acpi_wakedev_scan(sc);

	acpi_pcidev_scan(sc);
}

static int
acpi_rescan(device_t self, const char *ifattr, const int *locators)
{
	struct acpi_softc *sc = device_private(self);

	acpi_rescan1(sc, ifattr, locators);
	return 0;
}

/* XXX share this with sys/arch/i386/pci/elan520.c */
static bool
ifattr_match(const char *snull, const char *t)
{
	return (snull == NULL) || strcmp(snull, t) == 0;
}

static void
acpi_rescan1(struct acpi_softc *sc, const char *ifattr, const int *locators)
{
	if (ifattr_match(ifattr, "acpinodebus"))
		acpi_rescan_nodes(sc);

	if (ifattr_match(ifattr, "acpiapmbus") && sc->sc_apmbus == NULL) {
		sc->sc_apmbus = config_found_ia(sc->sc_dev, "acpiapmbus", NULL,
		    NULL);
	}
}

static void
acpi_rescan_nodes(struct acpi_softc *sc)
{
	struct acpi_scope *as;

	TAILQ_FOREACH(as, &sc->sc_scopes, as_list) {
		struct acpi_devnode *ad;

		/* Now, for this namespace, try to attach the devices. */
		TAILQ_FOREACH(ad, &as->as_devnodes, ad_list) {
			struct acpi_attach_args aa;

			if (ad->ad_device != NULL)
				continue;

			aa.aa_node = ad;
			aa.aa_iot = sc->sc_iot;
			aa.aa_memt = sc->sc_memt;
			aa.aa_pc = sc->sc_pc;
			aa.aa_pciflags = sc->sc_pciflags;
			aa.aa_ic = sc->sc_ic;

			if (ad->ad_devinfo->Type == ACPI_TYPE_DEVICE) {
				/*
				 * XXX We only attach devices which are:
				 *
				 *	- present
				 *	- enabled
				 *	- functioning properly
				 *
				 * However, if enabled, it's decoding resources,
				 * so we should claim them, if possible.
				 * Requires changes to bus_space(9).
				 */
				if ((ad->ad_devinfo->Valid & ACPI_VALID_STA) ==
				    ACPI_VALID_STA &&
				    (ad->ad_devinfo->CurrentStatus &
				     (ACPI_STA_DEV_PRESENT|ACPI_STA_DEV_ENABLED|
				      ACPI_STA_DEV_OK)) !=
				    (ACPI_STA_DEV_PRESENT|ACPI_STA_DEV_ENABLED|
				     ACPI_STA_DEV_OK))
					continue;
			}

			/*
			 * XXX Same problem as above...
			 *
			 * Do this check only for devices, as e.g.
			 * a Thermal Zone doesn't have a HID.
			 */
			if (ad->ad_devinfo->Type == ACPI_TYPE_DEVICE &&
			    (ad->ad_devinfo->Valid & ACPI_VALID_HID) == 0)
				continue;

			/*
			 * Handled internally
			 */
			if (ad->ad_devinfo->Type == ACPI_TYPE_PROCESSOR ||
			    ad->ad_devinfo->Type == ACPI_TYPE_POWER)
				continue;

			/*
			 * Skip ignored HIDs
			 */
			if (acpi_match_hid(ad->ad_devinfo, acpi_ignored_ids))
				continue;

			ad->ad_device = config_found_ia(sc->sc_dev,
			    "acpinodebus", &aa, acpi_print);
		}
	}
}

#ifdef ACPI_ACTIVATE_DEV
static void
acpi_activate_device(ACPI_HANDLE handle, ACPI_DEVICE_INFO **di)
{
	ACPI_STATUS rv;
	ACPI_DEVICE_INFO *newdi;

#ifdef ACPI_DEBUG
	aprint_normal("acpi_activate_device: %s, old status=%x\n",
	       (*di)->HardwareId.Value, (*di)->CurrentStatus);
#endif

	rv = acpi_allocate_resources(handle);
	if (ACPI_FAILURE(rv)) {
		aprint_error("acpi: activate failed for %s\n",
		       (*di)->HardwareId.String);
	} else {
		aprint_verbose("acpi: activated %s\n",
		    (*di)->HardwareId.String);
	}

	(void)AcpiGetObjectInfo(handle, &newdi);
	ACPI_FREE(*di);
	*di = newdi;

#ifdef ACPI_DEBUG
	aprint_normal("acpi_activate_device: %s, new status=%x\n",
	       (*di)->HardwareId.Value, (*di)->CurrentStatus);
#endif
}
#endif /* ACPI_ACTIVATE_DEV */

/*
 * acpi_make_devnode:
 *
 *	Make an ACPI devnode.
 */
static ACPI_STATUS
acpi_make_devnode(ACPI_HANDLE handle, UINT32 level, void *context,
    void **status)
{
	struct acpi_make_devnode_state *state = context;
#if defined(ACPI_DEBUG) || defined(ACPI_EXTRA_DEBUG)
	struct acpi_softc *sc = state->softc;
#endif
	struct acpi_scope *as = state->scope;
	struct acpi_devnode *ad;
	ACPI_OBJECT_TYPE type;
	ACPI_DEVICE_INFO *devinfo;
	ACPI_STATUS rv;
	ACPI_NAME_UNION *anu;
	int i, clear = 0;

	rv = AcpiGetType(handle, &type);
	if (ACPI_SUCCESS(rv)) {
		rv = AcpiGetObjectInfo(handle, &devinfo);
		if (ACPI_FAILURE(rv)) {
#ifdef ACPI_DEBUG
			aprint_normal_dev(sc->sc_dev,
			    "AcpiGetObjectInfo failed: %s\n",
			    AcpiFormatException(rv));
#endif
			goto out; /* XXX why return OK */
		}

		switch (type) {
		case ACPI_TYPE_DEVICE:
#ifdef ACPI_ACTIVATE_DEV
			if ((devinfo->Valid & (ACPI_VALID_STA|ACPI_VALID_HID)) ==
			    (ACPI_VALID_STA|ACPI_VALID_HID) &&
			    (devinfo->CurrentStatus &
			     (ACPI_STA_DEV_PRESENT|ACPI_STA_DEV_ENABLED)) ==
			    ACPI_STA_DEV_PRESENT)
				acpi_activate_device(handle, &devinfo);

			/* FALLTHROUGH */
#endif

		case ACPI_TYPE_PROCESSOR:
		case ACPI_TYPE_THERMAL:
		case ACPI_TYPE_POWER:
			ad = malloc(sizeof(*ad), M_ACPI, M_NOWAIT|M_ZERO);
			if (ad == NULL)
				return AE_NO_MEMORY;

			ad->ad_devinfo = devinfo;
			ad->ad_handle = handle;
			ad->ad_level = level;
			ad->ad_scope = as;
			ad->ad_type = type;

			anu = (ACPI_NAME_UNION *)&devinfo->Name;
			ad->ad_name[4] = '\0';
			for (i = 3, clear = 0; i >= 0; i--) {
				if (!clear && anu->Ascii[i] == '_')
					ad->ad_name[i] = '\0';
				else {
					ad->ad_name[i] = anu->Ascii[i];
					clear = 1;
				}
			}
			if (ad->ad_name[0] == '\0')
				ad->ad_name[0] = '_';

			TAILQ_INSERT_TAIL(&as->as_devnodes, ad, ad_list);

			if (type == ACPI_TYPE_DEVICE &&
			    (ad->ad_devinfo->Valid & ACPI_VALID_HID) == 0)
				goto out;

#ifdef ACPI_EXTRA_DEBUG
			aprint_normal_dev(sc->sc_dev,
			    "HID %s found in scope %s level %d\n",
			    ad->ad_devinfo->HardwareId.String,
			    as->as_name, ad->ad_level);
			if (ad->ad_devinfo->Valid & ACPI_VALID_UID)
				aprint_normal("       UID %s\n",
				    ad->ad_devinfo->UniqueId.String);
			if (ad->ad_devinfo->Valid & ACPI_VALID_ADR)
				aprint_normal("       ADR 0x%016" PRIx64 "\n",
				    ad->ad_devinfo->Address);
			if (ad->ad_devinfo->Valid & ACPI_VALID_STA)
				aprint_normal("       STA 0x%08x\n",
				    ad->ad_devinfo->CurrentStatus);
#endif
		}
	}
 out:
	return AE_OK;
}

/*
 * acpi_print:
 *
 *	Autoconfiguration print routine for ACPI node bus.
 */
static int
acpi_print(void *aux, const char *pnp)
{
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;

	if (pnp) {
		if (aa->aa_node->ad_devinfo->Valid & ACPI_VALID_HID) {
			char *pnpstr =
			    aa->aa_node->ad_devinfo->HardwareId.String;
			ACPI_BUFFER buf;

			aprint_normal("%s (%s) ", aa->aa_node->ad_name,
			    pnpstr);

			rv = acpi_eval_struct(aa->aa_node->ad_handle,
			    "_STR", &buf);
			if (ACPI_SUCCESS(rv)) {
				ACPI_OBJECT *obj = buf.Pointer;
				switch (obj->Type) {
				case ACPI_TYPE_STRING:
					aprint_normal("[%s] ", obj->String.Pointer);
					break;
				case ACPI_TYPE_BUFFER:
					aprint_normal("buffer %p ", obj->Buffer.Pointer);
					break;
				default:
					aprint_normal("type %d ",obj->Type);
					break;
				}
				ACPI_FREE(buf.Pointer);
			}
#ifdef ACPIVERBOSE
			else {
				int i;

				for (i = 0; i < __arraycount(acpi_knowndevs);
				    i++) {
					if (strcmp(acpi_knowndevs[i].pnp,
					    pnpstr) == 0) {
						aprint_normal("[%s] ",
						    acpi_knowndevs[i].str);
					}
				}
			}

#endif
			aprint_normal("at %s", pnp);
		} else if (aa->aa_node->ad_devinfo->Type != ACPI_TYPE_DEVICE) {
			aprint_normal("%s (ACPI Object Type '%s' "
			    "[0x%02x]) ", aa->aa_node->ad_name,
			     AcpiUtGetTypeName(aa->aa_node->ad_devinfo->Type),
			     aa->aa_node->ad_devinfo->Type);
			aprint_normal("at %s", pnp);
		} else
			return 0;
	} else {
		aprint_normal(" (%s", aa->aa_node->ad_name);
		if (aa->aa_node->ad_devinfo->Valid & ACPI_VALID_HID) {
			aprint_normal(", %s", aa->aa_node->ad_devinfo->HardwareId.String);
			if (aa->aa_node->ad_devinfo->Valid & ACPI_VALID_UID) {
				const char *uid;

				uid = aa->aa_node->ad_devinfo->UniqueId.String;
				if (uid[0] == '\0')
					uid = "<null>";
				aprint_normal("-%s", uid);
			}
		}
		aprint_normal(")");
	}

	return UNCONF;
}

/*****************************************************************************
 * ACPI fixed-hardware feature handlers
 *****************************************************************************/

static UINT32	acpi_fixed_button_handler(void *);
static void	acpi_fixed_button_pressed(void *);

/*
 * acpi_enable_fixed_events:
 *
 *	Enable any fixed-hardware feature handlers.
 */
static void
acpi_enable_fixed_events(struct acpi_softc *sc)
{
	static int beenhere;
	ACPI_STATUS rv;

	KASSERT(beenhere == 0);
	beenhere = 1;

	/*
	 * Check for fixed-hardware buttons.
	 */

	if ((AcpiGbl_FADT.Flags & ACPI_FADT_POWER_BUTTON) == 0) {
		aprint_verbose_dev(sc->sc_dev,
		    "fixed-feature power button present\n");
		sc->sc_smpsw_power.smpsw_name = device_xname(sc->sc_dev);
		sc->sc_smpsw_power.smpsw_type = PSWITCH_TYPE_POWER;
		if (sysmon_pswitch_register(&sc->sc_smpsw_power) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to register fixed power "
			    "button with sysmon\n");
		} else {
			rv = AcpiInstallFixedEventHandler(
			    ACPI_EVENT_POWER_BUTTON,
			    acpi_fixed_button_handler, &sc->sc_smpsw_power);
			if (ACPI_FAILURE(rv)) {
				aprint_error_dev(sc->sc_dev,
				    "unable to install handler "
				    "for fixed power button: %s\n",
				    AcpiFormatException(rv));
			}
		}
	}

	if ((AcpiGbl_FADT.Flags & ACPI_FADT_SLEEP_BUTTON) == 0) {
		aprint_verbose_dev(sc->sc_dev,
		    "fixed-feature sleep button present\n");
		sc->sc_smpsw_sleep.smpsw_name = device_xname(sc->sc_dev);
		sc->sc_smpsw_sleep.smpsw_type = PSWITCH_TYPE_SLEEP;
		if (sysmon_pswitch_register(&sc->sc_smpsw_power) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to register fixed sleep "
			    "button with sysmon\n");
		} else {
			rv = AcpiInstallFixedEventHandler(
			    ACPI_EVENT_SLEEP_BUTTON,
			    acpi_fixed_button_handler, &sc->sc_smpsw_sleep);
			if (ACPI_FAILURE(rv)) {
				aprint_error_dev(sc->sc_dev,
				    "unable to install handler "
				    "for fixed sleep button: %s\n",
				    AcpiFormatException(rv));
			}
		}
	}
}

/*
 * acpi_fixed_button_handler:
 *
 *	Event handler for the fixed buttons.
 */
static UINT32
acpi_fixed_button_handler(void *context)
{
	struct sysmon_pswitch *smpsw = context;
	ACPI_STATUS rv;

#ifdef ACPI_BUT_DEBUG
	printf("%s: fixed button handler\n", smpsw->smpsw_name);
#endif

	rv = AcpiOsExecute(OSL_NOTIFY_HANDLER,
	    acpi_fixed_button_pressed, smpsw);
	if (ACPI_FAILURE(rv))
		printf("%s: WARNING: unable to queue fixed button pressed "
		    "callback: %s\n", smpsw->smpsw_name,
		    AcpiFormatException(rv));

	return ACPI_INTERRUPT_HANDLED;
}

/*
 * acpi_fixed_button_pressed:
 *
 *	Deal with a fixed button being pressed.
 */
static void
acpi_fixed_button_pressed(void *context)
{
	struct sysmon_pswitch *smpsw = context;

#ifdef ACPI_BUT_DEBUG
	printf("%s: fixed button pressed, calling sysmon\n",
	    smpsw->smpsw_name);
#endif

	sysmon_pswitch_event(smpsw, PSWITCH_EVENT_PRESSED);
}

/*****************************************************************************
 * ACPI utility routines.
 *****************************************************************************/

/*
 * acpi_eval_integer:
 *
 *	Evaluate an integer object.
 */
ACPI_STATUS
acpi_eval_integer(ACPI_HANDLE handle, const char *path, ACPI_INTEGER *valp)
{
	ACPI_STATUS rv;
	ACPI_BUFFER buf;
	ACPI_OBJECT param;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	buf.Pointer = &param;
	buf.Length = sizeof(param);

	rv = AcpiEvaluateObjectTyped(handle, path, NULL, &buf,
	    ACPI_TYPE_INTEGER);
	if (ACPI_SUCCESS(rv))
		*valp = param.Integer.Value;

	return rv;
}

ACPI_STATUS
acpi_eval_set_integer(ACPI_HANDLE handle, const char *path, ACPI_INTEGER arg)
{
	ACPI_OBJECT param_arg;
	ACPI_OBJECT_LIST param_args;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	param_arg.Type = ACPI_TYPE_INTEGER;
	param_arg.Integer.Value = arg;

	param_args.Count = 1;
	param_args.Pointer = &param_arg;

	return AcpiEvaluateObject(handle, path, &param_args, NULL);
}

/*
 * acpi_eval_string:
 *
 *	Evaluate a (Unicode) string object.
 */
ACPI_STATUS
acpi_eval_string(ACPI_HANDLE handle, const char *path, char **stringp)
{
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	rv = AcpiEvaluateObjectTyped(handle, path, NULL, &buf, ACPI_TYPE_STRING);
	if (ACPI_SUCCESS(rv)) {
		ACPI_OBJECT *param = buf.Pointer;
		const char *ptr = param->String.Pointer;
		size_t len = param->String.Length;
		if ((*stringp = ACPI_ALLOCATE(len)) == NULL)
			rv = AE_NO_MEMORY;
		else
			(void)memcpy(*stringp, ptr, len);
		ACPI_FREE(param);
	}

	return rv;
}


/*
 * acpi_eval_struct:
 *
 *	Evaluate a more complex structure.
 *	Caller must free buf.Pointer by ACPI_FREE().
 */
ACPI_STATUS
acpi_eval_struct(ACPI_HANDLE handle, const char *path, ACPI_BUFFER *bufp)
{
	ACPI_STATUS rv;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	bufp->Pointer = NULL;
	bufp->Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	rv = AcpiEvaluateObject(handle, path, NULL, bufp);

	return rv;
}

/*
 * acpi_foreach_package_object:
 *
 *	Iterate over all objects in a in a packages and pass then all
 *	to a function. If the called function returns non AE_OK, the
 *	iteration is stopped and that value is returned.
 */

ACPI_STATUS
acpi_foreach_package_object(ACPI_OBJECT *pkg,
    ACPI_STATUS (*func)(ACPI_OBJECT *, void *),
    void *arg)
{
	ACPI_STATUS rv = AE_OK;
	int i;

	if (pkg == NULL || pkg->Type != ACPI_TYPE_PACKAGE)
		return AE_BAD_PARAMETER;

	for (i = 0; i < pkg->Package.Count; i++) {
		rv = (*func)(&pkg->Package.Elements[i], arg);
		if (ACPI_FAILURE(rv))
			break;
	}

	return rv;
}

const char *
acpi_name(ACPI_HANDLE handle)
{
	static char buffer[80];
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	buf.Length = sizeof(buffer);
	buf.Pointer = buffer;

	rv = AcpiGetName(handle, ACPI_FULL_PATHNAME, &buf);
	if (ACPI_FAILURE(rv))
		return "(unknown acpi path)";
	return buffer;
}

/*
 * acpi_get:
 *
 *	Fetch data info the specified (empty) ACPI buffer.
 *	Caller must free buf.Pointer by ACPI_FREE().
 */
ACPI_STATUS
acpi_get(ACPI_HANDLE handle, ACPI_BUFFER *buf,
    ACPI_STATUS (*getit)(ACPI_HANDLE, ACPI_BUFFER *))
{
	buf->Pointer = NULL;
	buf->Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	return (*getit)(handle, buf);
}


/*
 * acpi_match_hid
 *
 *	Match given ids against _HID and _CIDs
 */
int
acpi_match_hid(ACPI_DEVICE_INFO *ad, const char * const *ids)
{
	int i;

	while (*ids) {
		if (ad->Valid & ACPI_VALID_HID) {
			if (pmatch(ad->HardwareId.String, *ids, NULL) == 2)
				return 1;
		}

		if (ad->Valid & ACPI_VALID_CID) {
			for (i = 0; i < ad->CompatibleIdList.Count; i++) {
				if (pmatch(ad->CompatibleIdList.Ids[i].String, *ids, NULL) == 2)
					return 1;
			}
		}
		ids++;
	}

	return 0;
}

/*
 * acpi_wake_gpe_helper
 *
 *	Set/unset GPE as both Runtime and Wake
 */
static void
acpi_wake_gpe_helper(ACPI_HANDLE handle, bool enable)
{
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	ACPI_OBJECT *p, *elt;

	rv = acpi_eval_struct(handle, METHOD_NAME__PRW, &buf);
	if (ACPI_FAILURE(rv))
		return;			/* just ignore */

	p = buf.Pointer;
	if (p->Type != ACPI_TYPE_PACKAGE || p->Package.Count < 2)
		goto out;		/* just ignore */

	elt = p->Package.Elements;

	/* TBD: package support */
	if (enable) {
		AcpiSetGpeType(NULL, elt[0].Integer.Value,
		    ACPI_GPE_TYPE_WAKE_RUN);
		AcpiEnableGpe(NULL, elt[0].Integer.Value, ACPI_NOT_ISR);
	} else
		AcpiDisableGpe(NULL, elt[0].Integer.Value, ACPI_NOT_ISR);

 out:
	ACPI_FREE(buf.Pointer);
}

/*
 * acpi_clear_wake_gpe
 *
 *	Clear GPE as both Runtime and Wake
 */
void
acpi_clear_wake_gpe(ACPI_HANDLE handle)
{
	acpi_wake_gpe_helper(handle, false);
}

/*
 * acpi_set_wake_gpe
 *
 *	Set GPE as both Runtime and Wake
 */
void
acpi_set_wake_gpe(ACPI_HANDLE handle)
{
	acpi_wake_gpe_helper(handle, true);
}


/*****************************************************************************
 * ACPI sleep support.
 *****************************************************************************/

static int
is_available_state(struct acpi_softc *sc, int state)
{
	UINT8 type_a, type_b;

	return ACPI_SUCCESS(AcpiGetSleepTypeData((UINT8)state,
				&type_a, &type_b));
}

/*
 * acpi_enter_sleep_state:
 *
 *	enter to the specified sleep state.
 */

ACPI_STATUS
acpi_enter_sleep_state(struct acpi_softc *sc, int state)
{
	int err;
	ACPI_STATUS ret = AE_OK;

	if (state == acpi_sleepstate)
		return AE_OK;

	aprint_normal_dev(sc->sc_dev, "entering state %d\n", state);

	switch (state) {
	case ACPI_STATE_S0:
		break;
	case ACPI_STATE_S1:
	case ACPI_STATE_S2:
	case ACPI_STATE_S3:
	case ACPI_STATE_S4:
		if (!is_available_state(sc, state)) {
			aprint_error_dev(sc->sc_dev,
			    "ACPI S%d not available on this platform\n", state);
			break;
		}

		acpi_wakedev_commit(sc);

		if (state != ACPI_STATE_S1 && !pmf_system_suspend(PMF_Q_NONE)) {
			aprint_error_dev(sc->sc_dev, "aborting suspend\n");
			break;
		}

		ret = AcpiEnterSleepStatePrep(state);
		if (ACPI_FAILURE(ret)) {
			aprint_error_dev(sc->sc_dev,
			    "failed preparing to sleep (%s)\n",
			    AcpiFormatException(ret));
			break;
		}

		acpi_sleepstate = state;
		if (state == ACPI_STATE_S1) {
			/* just enter the state */
			acpi_md_OsDisableInterrupt();
			ret = AcpiEnterSleepState((UINT8)state);
			if (ACPI_FAILURE(ret))
				aprint_error_dev(sc->sc_dev,
				    "failed to enter sleep state S1: %s\n",
				    AcpiFormatException(ret));
			AcpiLeaveSleepState((UINT8)state);
		} else {
			err = acpi_md_sleep(state);
			if (state == ACPI_STATE_S4)
				AcpiEnable();
			pmf_system_bus_resume(PMF_Q_NONE);
			AcpiLeaveSleepState((UINT8)state);
			pmf_system_resume(PMF_Q_NONE);
		}

		break;
	case ACPI_STATE_S5:
		ret = AcpiEnterSleepStatePrep(ACPI_STATE_S5);
		if (ACPI_FAILURE(ret)) {
			aprint_error_dev(sc->sc_dev,
			    "failed preparing to sleep (%s)\n",
			    AcpiFormatException(ret));
			break;
		}
		DELAY(1000000);
		acpi_sleepstate = state;
		acpi_md_OsDisableInterrupt();
		AcpiEnterSleepState(ACPI_STATE_S5);
		aprint_error_dev(sc->sc_dev, "WARNING powerdown failed!\n");
		break;
	}

	acpi_sleepstate = ACPI_STATE_S0;
	return ret;
}

#if defined(ACPI_ACTIVATE_DEV)
/* XXX This very incomplete */
ACPI_STATUS
acpi_allocate_resources(ACPI_HANDLE handle)
{
	ACPI_BUFFER bufp, bufc, bufn;
	ACPI_RESOURCE *resp, *resc, *resn;
	ACPI_RESOURCE_IRQ *irq;
	ACPI_RESOURCE_EXTENDED_IRQ *xirq;
	ACPI_STATUS rv;
	uint delta;

	rv = acpi_get(handle, &bufp, AcpiGetPossibleResources);
	if (ACPI_FAILURE(rv))
		goto out;
	rv = acpi_get(handle, &bufc, AcpiGetCurrentResources);
	if (ACPI_FAILURE(rv)) {
		goto out1;
	}

	bufn.Length = 1000;
	bufn.Pointer = resn = malloc(bufn.Length, M_ACPI, M_WAITOK);
	resp = bufp.Pointer;
	resc = bufc.Pointer;
	while (resc->Type != ACPI_RESOURCE_TYPE_END_TAG &&
	       resp->Type != ACPI_RESOURCE_TYPE_END_TAG) {
		while (resc->Type != resp->Type && resp->Type != ACPI_RESOURCE_TYPE_END_TAG)
			resp = ACPI_NEXT_RESOURCE(resp);
		if (resp->Type == ACPI_RESOURCE_TYPE_END_TAG)
			break;
		/* Found identical Id */
		resn->Type = resc->Type;
		switch (resc->Type) {
		case ACPI_RESOURCE_TYPE_IRQ:
			memcpy(&resn->Data, &resp->Data,
			       sizeof(ACPI_RESOURCE_IRQ));
			irq = (ACPI_RESOURCE_IRQ *)&resn->Data;
			irq->Interrupts[0] =
			    ((ACPI_RESOURCE_IRQ *)&resp->Data)->
			        Interrupts[irq->InterruptCount-1];
			irq->InterruptCount = 1;
			resn->Length = ACPI_RS_SIZE(ACPI_RESOURCE_IRQ);
			break;
		case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
			memcpy(&resn->Data, &resp->Data,
			       sizeof(ACPI_RESOURCE_EXTENDED_IRQ));
			xirq = (ACPI_RESOURCE_EXTENDED_IRQ *)&resn->Data;
#if 0
			/*
			 * XXX not duplicating the interrupt logic above
			 * because its not clear what it accomplishes.
			 */
			xirq->Interrupts[0] =
			    ((ACPI_RESOURCE_EXT_IRQ *)&resp->Data)->
			    Interrupts[irq->NumberOfInterrupts-1];
			xirq->NumberOfInterrupts = 1;
#endif
			resn->Length = ACPI_RS_SIZE(ACPI_RESOURCE_EXTENDED_IRQ);
			break;
		case ACPI_RESOURCE_TYPE_IO:
			memcpy(&resn->Data, &resp->Data,
			       sizeof(ACPI_RESOURCE_IO));
			resn->Length = resp->Length;
			break;
		default:
			printf("acpi_allocate_resources: res=%d\n", resc->Type);
			rv = AE_BAD_DATA;
			goto out2;
		}
		resc = ACPI_NEXT_RESOURCE(resc);
		resn = ACPI_NEXT_RESOURCE(resn);
		resp = ACPI_NEXT_RESOURCE(resp);
		delta = (UINT8 *)resn - (UINT8 *)bufn.Pointer;
		if (delta >=
		    bufn.Length-ACPI_RS_SIZE(ACPI_RESOURCE_DATA)) {
			bufn.Length *= 2;
			bufn.Pointer = realloc(bufn.Pointer, bufn.Length,
					       M_ACPI, M_WAITOK);
			resn = (ACPI_RESOURCE *)((UINT8 *)bufn.Pointer + delta);
		}
	}
	if (resc->Type != ACPI_RESOURCE_TYPE_END_TAG) {
		printf("acpi_allocate_resources: resc not exhausted\n");
		rv = AE_BAD_DATA;
		goto out3;
	}

	resn->Type = ACPI_RESOURCE_TYPE_END_TAG;
	rv = AcpiSetCurrentResources(handle, &bufn);
	if (ACPI_FAILURE(rv)) {
		printf("acpi_allocate_resources: AcpiSetCurrentResources %s\n",
		       AcpiFormatException(rv));
	}

out3:
	free(bufn.Pointer, M_ACPI);
out2:
	ACPI_FREE(bufc.Pointer);
out1:
	ACPI_FREE(bufp.Pointer);
out:
	return rv;
}
#endif /* ACPI_ACTIVATE_DEV */

SYSCTL_SETUP(sysctl_acpi_setup, "sysctl hw.acpi subtree setup")
{
	const struct sysctlnode *node;
	const struct sysctlnode *ssnode;

	if (sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_EOL) != 0)
		return;

	if (sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "acpi", NULL,
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL) != 0)
		return;

	sysctl_createv(NULL, 0, NULL, NULL, CTLFLAG_READONLY,
	    CTLTYPE_QUAD, "root",
	    SYSCTL_DESCR("ACPI root pointer"),
	    NULL, 0, &acpi_root_pointer, sizeof(acpi_root_pointer),
	    CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL);
	sysctl_createv(NULL, 0, NULL, NULL, CTLFLAG_READONLY,
	    CTLTYPE_STRING, "supported_states",
	    SYSCTL_DESCR("Supported ACPI system states"),
	    NULL, 0, acpi_supported_states, 0,
	    CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL);

	/* ACPI sleepstate sysctl */
	if (sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL) != 0)
		return;
	if (sysctl_createv(NULL, 0, &node, &ssnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "sleep_state",
	    NULL, sysctl_hw_acpi_sleepstate, 0, NULL, 0, CTL_CREATE,
	    CTL_EOL) != 0)
		return;
}

static int
sysctl_hw_acpi_sleepstate(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = acpi_sleepstate;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (acpi_softc == NULL)
		return ENOSYS;

	acpi_enter_sleep_state(acpi_softc, t);

	return 0;
}

static ACPI_TABLE_HEADER *
acpi_map_rsdt(void)
{
	ACPI_PHYSICAL_ADDRESS paddr;
	ACPI_TABLE_RSDP *rsdp;

	paddr = AcpiOsGetRootPointer();
	if (paddr == 0) {
		printf("ACPI: couldn't get root pointer\n");
		return NULL;
	}
	rsdp = AcpiOsMapMemory(paddr, sizeof(ACPI_TABLE_RSDP));
	if (rsdp == NULL) {
		printf("ACPI: couldn't map RSDP\n");
		return NULL;
	}
	if (rsdp->Revision > 1 && rsdp->XsdtPhysicalAddress)
		paddr = (ACPI_PHYSICAL_ADDRESS)rsdp->XsdtPhysicalAddress;
	else
		paddr = (ACPI_PHYSICAL_ADDRESS)rsdp->RsdtPhysicalAddress;
	AcpiOsUnmapMemory(rsdp, sizeof(ACPI_TABLE_RSDP));

	return AcpiOsMapMemory(paddr, sizeof(ACPI_TABLE_HEADER));
}

static void
acpi_unmap_rsdt(ACPI_TABLE_HEADER *rsdt)
{
	if (rsdt == NULL)
		return;

	AcpiOsUnmapMemory(rsdt, sizeof(ACPI_TABLE_HEADER));
}
