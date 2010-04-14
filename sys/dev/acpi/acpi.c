/*	$NetBSD: acpi.c,v 1.169 2010/04/14 17:12:14 jruoho Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi.c,v 1.169 2010/04/14 17:12:14 jruoho Exp $");

#include "opt_acpi.h"
#include "opt_pcifixup.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_osd.h>
#include <dev/acpi/acpi_pci.h>
#include <dev/acpi/acpi_timer.h>
#include <dev/acpi/acpi_wakedev.h>

#ifdef ACPIVERBOSE
#include <dev/acpi/acpidevs_data.h>
#endif

#define _COMPONENT	ACPI_BUS_COMPONENT
ACPI_MODULE_NAME	("acpi")

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

/*
 * This is a flag we set when the ACPI subsystem is active.  Machine
 * dependent code may wish to skip other steps (such as attaching
 * subsystems that ACPI supercedes) when ACPI is active.
 */
int	acpi_active;
int	acpi_force_load;
int	acpi_suspended = 0;

struct acpi_softc *acpi_softc;
static uint64_t acpi_root_pointer;
extern kmutex_t acpi_interrupt_list_mtx;

/*
 * Ignored HIDs.
 */
static const char * const acpi_ignored_ids[] = {
#if defined(i386) || defined(x86_64)
	"PNP0000",	/* AT interrupt controller is handled internally */
	"PNP0200",	/* AT DMA controller is handled internally */
	"PNP0A??",	/* PCI Busses are handled internally */
	"PNP0B00",	/* AT RTC is handled internally */
	"PNP0C01",	/* No "System Board" driver */
	"PNP0C02",	/* No "PnP motherboard register resources" driver */
	"PNP0C0B",	/* No need for "ACPI fan" driver */
	"PNP0C0F",	/* ACPI PCI link devices are handled internally */
	"IFX0102",	/* No driver for Infineon TPM */
	"INT0800",	/* No driver for Intel Firmware Hub device */
#endif
#if defined(x86_64)
	"PNP0C04",	/* FPU is handled internally */
#endif
	NULL
};

static int		acpi_match(device_t, cfdata_t, void *);
static int		acpi_submatch(device_t, cfdata_t, const int *, void *);
static void		acpi_attach(device_t, device_t, void *);
static int		acpi_detach(device_t, int);
static void		acpi_childdet(device_t, device_t);
static bool		acpi_suspend(device_t, const pmf_qual_t *);
static bool		acpi_resume(device_t, const pmf_qual_t *);

static void		acpi_build_tree(struct acpi_softc *);
static ACPI_STATUS	acpi_make_devnode(ACPI_HANDLE, uint32_t,
					  void *, void **);

#ifdef ACPI_ACTIVATE_DEV
static void		acpi_activate_device(ACPI_HANDLE, ACPI_DEVICE_INFO **);
static ACPI_STATUS	acpi_allocate_resources(ACPI_HANDLE);
#endif

static int		acpi_rescan(device_t, const char *, const int *);
static void		acpi_rescan1(struct acpi_softc *,
				     const char *, const int *);
static void		acpi_rescan_nodes(struct acpi_softc *);
static void		acpi_rescan_capabilities(struct acpi_softc *);
static int		acpi_print(void *aux, const char *);

static void		acpi_register_fixed_button(struct acpi_softc *, int);
static void		acpi_deregister_fixed_button(struct acpi_softc *, int);
static uint32_t		acpi_fixed_button_handler(void *);
static void		acpi_fixed_button_pressed(void *);

static void		acpi_sleep_init(struct acpi_softc *);

static int		sysctl_hw_acpi_fixedstats(SYSCTLFN_ARGS);
static int		sysctl_hw_acpi_sleepstate(SYSCTLFN_ARGS);
static int		sysctl_hw_acpi_sleepstates(SYSCTLFN_ARGS);

static ACPI_TABLE_HEADER *acpi_map_rsdt(void);
static void		acpi_unmap_rsdt(ACPI_TABLE_HEADER *);

extern struct cfdriver acpi_cd;

CFATTACH_DECL2_NEW(acpi, sizeof(struct acpi_softc),
    acpi_match, acpi_attach, acpi_detach, NULL, acpi_rescan, acpi_childdet);

/*
 * Probe for ACPI support.
 *
 * This is called by the machine-dependent ACPI front-end.
 * Note: this is not an autoconfiguration interface function.
 */
int
acpi_probe(void)
{
	ACPI_TABLE_HEADER *rsdt;
	ACPI_STATUS rv;
	static int once;

	if (once != 0)
		panic("%s: already probed", __func__);

	once = 1;

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
	/*
	 * Install the default address space handlers.
	 */
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

int
acpi_check(device_t parent, const char *ifattr)
{
	return (config_search_ia(acpi_submatch, parent, ifattr, NULL) != NULL);
}

static int
acpi_match(device_t parent, cfdata_t match, void *aux)
{
	/*
	 * XXX: Nada; MD code has called acpi_probe().
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

static void
acpi_attach(device_t parent, device_t self, void *aux)
{
	struct acpi_softc *sc = device_private(self);
	struct acpibus_attach_args *aa = aux;
	ACPI_TABLE_HEADER *rsdt;
	ACPI_STATUS rv;

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
	sc->sc_sleepstate = ACPI_STATE_S0;

	sc->sc_iot = aa->aa_iot;
	sc->sc_memt = aa->aa_memt;
	sc->sc_pc = aa->aa_pc;
	sc->sc_pciflags = aa->aa_pciflags;
	sc->sc_ic = aa->aa_ic;

	SIMPLEQ_INIT(&sc->sc_devnodes);

	acpi_softc = sc;

	/*
	 * Register null power management handler.
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

	/* Early EC handler initialization if ECDT table is available. */
	config_found_ia(self, "acpiecdtbus", aa, NULL);

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
	aprint_verbose_dev(self, "SCI interrupting at int %u\n",
	    AcpiGbl_FADT.SciInterrupt);

	/*
	 * Install fixed-event handlers.
	 */
	acpi_register_fixed_button(sc, ACPI_EVENT_POWER_BUTTON);
	acpi_register_fixed_button(sc, ACPI_EVENT_SLEEP_BUTTON);

	acpitimer_init();

#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_PROBE)
		acpi_osd_debugger();
#endif

	/*
	 * Scan the namespace and build our device tree.
	 */
	acpi_build_tree(sc);
	acpi_sleep_init(sc);

#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_RUNNING)
		acpi_osd_debugger();
#endif

#ifdef ACPI_DEBUG
	acpi_debug_init();
#endif
}

static int
acpi_detach(device_t self, int flags)
{
	struct acpi_softc *sc = device_private(self);
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

	/* Early EC handler initialization if ECDT table is available. */
	config_found_ia(self, "acpiecdtbus", aa, NULL);

	rv = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self,
		    "unable to initialize ACPI objects: %s\n",
		    AcpiFormatException(rv));
		return;
	}

	acpi_active = 1;
#endif

	acpi_deregister_fixed_button(sc, ACPI_EVENT_POWER_BUTTON);
	acpi_deregister_fixed_button(sc, ACPI_EVENT_SLEEP_BUTTON);

	pmf_device_deregister(self);

#if 0
	sysmon_power_settype("acpi");
#endif
	acpi_softc = NULL;

	return 0;
}

/*
 * XXX: Need to reclaim any resources? Yes.
 */
static void
acpi_childdet(device_t self, device_t child)
{
	struct acpi_softc *sc = device_private(self);
	struct acpi_devnode *ad;

	if (sc->sc_apmbus == child)
		sc->sc_apmbus = NULL;

	SIMPLEQ_FOREACH(ad, &sc->sc_devnodes, ad_list) {

		if (ad->ad_device == child)
			ad->ad_device = NULL;
	}
}

static bool
acpi_suspend(device_t dv, const pmf_qual_t *qual)
{

	acpi_suspended = 1;

	return true;
}

static bool
acpi_resume(device_t dv, const pmf_qual_t *qual)
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

/*
 * Namespace scan.
 */
static void
acpi_build_tree(struct acpi_softc *sc)
{

	(void)AcpiWalkNamespace(ACPI_TYPE_ANY, ACPI_ROOT_OBJECT,
	    UINT32_MAX, acpi_make_devnode, NULL, sc, NULL);

	acpi_rescan1(sc, NULL, NULL);
	acpi_rescan_capabilities(sc);

	acpi_pcidev_scan(sc);
}

static ACPI_STATUS
acpi_make_devnode(ACPI_HANDLE handle, uint32_t level,
    void *context, void **status)
{
	struct acpi_softc *sc = context;
	struct acpi_devnode *ad;
	ACPI_DEVICE_INFO *devinfo;
	ACPI_OBJECT_TYPE type;
	ACPI_NAME_UNION *anu;
	ACPI_STATUS rv;
	int clear, i;

	rv = AcpiGetObjectInfo(handle, &devinfo);

	if (ACPI_FAILURE(rv))
		return AE_OK;	/* Do not terminate the walk. */

	type = devinfo->Type;

	switch (type) {

	case ACPI_TYPE_DEVICE:

#ifdef ACPI_ACTIVATE_DEV
		acpi_activate_device(handle, &devinfo);
#endif

	case ACPI_TYPE_PROCESSOR:
	case ACPI_TYPE_THERMAL:
	case ACPI_TYPE_POWER:

		ad = malloc(sizeof(*ad), M_ACPI, M_NOWAIT | M_ZERO);

		if (ad == NULL)
			return AE_NO_MEMORY;

		ad->ad_parent = sc->sc_dev;
		ad->ad_devinfo = devinfo;
		ad->ad_handle = handle;
		ad->ad_type = type;

		anu = (ACPI_NAME_UNION *)&devinfo->Name;
		ad->ad_name[4] = '\0';

		for (i = 3, clear = 0; i >= 0; i--) {

			if (clear == 0 && anu->Ascii[i] == '_')
				ad->ad_name[i] = '\0';
			else {
				ad->ad_name[i] = anu->Ascii[i];
				clear = 1;
			}
		}

		if (ad->ad_name[0] == '\0')
			ad->ad_name[0] = '_';

		SIMPLEQ_INSERT_TAIL(&sc->sc_devnodes, ad, ad_list);

#ifdef ACPIVERBOSE

		if (type != ACPI_TYPE_DEVICE)
			return AE_OK;

		aprint_normal_dev(sc->sc_dev, "%-5s ", ad->ad_name);

		aprint_normal("HID %-10s ",
		    ((devinfo->Valid & ACPI_VALID_HID) != 0) ?
		    devinfo->HardwareId.String: "-");

		aprint_normal("UID %-4s ",
		    ((devinfo->Valid & ACPI_VALID_UID) != 0) ?
		    devinfo->UniqueId.String : "-");

		if ((devinfo->Valid & ACPI_VALID_STA) != 0)
			aprint_normal("STA 0x%08X ", devinfo->CurrentStatus);
		else
			aprint_normal("STA %10s ", "-");

		if ((devinfo->Valid & ACPI_VALID_ADR) != 0)
			aprint_normal("ADR 0x%016" PRIX64"",
			    devinfo->Address);

		aprint_normal("\n");
#endif
	}

	return AE_OK;
}

#ifdef ACPI_ACTIVATE_DEV

#define ACPI_DEV_VALID	(ACPI_VALID_STA | ACPI_VALID_HID)
#define ACPI_DEV_STATUS	(ACPI_STA_DEV_PRESENT | ACPI_STA_DEV_ENABLED)

static void
acpi_activate_device(ACPI_HANDLE handle, ACPI_DEVICE_INFO **di)
{
	ACPI_DEVICE_INFO *newdi;
	ACPI_STATUS rv;
	uint32_t old;

	/*
	 * If the device is valid and present,
	 * but not enabled, try to activate it.
	 */
	if (((*di)->Valid & ACPI_DEV_VALID) != ACPI_DEV_VALID)
		return;

	old = (*di)->CurrentStatus;

	if ((old & ACPI_DEV_STATUS) != ACPI_STA_DEV_PRESENT)
		return;

	rv = acpi_allocate_resources(handle);

	if (ACPI_FAILURE(rv))
		goto fail;

	rv = AcpiGetObjectInfo(handle, &newdi);

	if (ACPI_FAILURE(rv))
		goto fail;

	ACPI_FREE(*di);
	*di = newdi;

	aprint_verbose_dev(acpi_softc->sc_dev,
	    "%s activated, STA 0x%08X -> STA 0x%08X\n",
	    (*di)->HardwareId.String, old, (*di)->CurrentStatus);

	return;

fail:
	aprint_error_dev(acpi_softc->sc_dev, "failed to "
	    "activate %s\n", (*di)->HardwareId.String);
}

/*
 * XXX: This very incomplete.
 */
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
			aprint_error_dev(acpi_softc->sc_dev,
			    "%s: invalid type %u\n", __func__, resc->Type);
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
		aprint_error_dev(acpi_softc->sc_dev,
		    "%s: resc not exhausted\n", __func__);
		rv = AE_BAD_DATA;
		goto out3;
	}

	resn->Type = ACPI_RESOURCE_TYPE_END_TAG;
	rv = AcpiSetCurrentResources(handle, &bufn);

	if (ACPI_FAILURE(rv))
		aprint_error_dev(acpi_softc->sc_dev, "%s: failed to set "
		    "resources: %s\n", __func__, AcpiFormatException(rv));

out3:
	free(bufn.Pointer, M_ACPI);
out2:
	ACPI_FREE(bufc.Pointer);
out1:
	ACPI_FREE(bufp.Pointer);
out:
	return rv;
}

#undef ACPI_DEV_VALID
#undef ACPI_DEV_STATUS

#endif /* ACPI_ACTIVATE_DEV */

/*
 * Device attachment.
 */
static int
acpi_rescan(device_t self, const char *ifattr, const int *locators)
{
	struct acpi_softc *sc = device_private(self);

	acpi_rescan1(sc, ifattr, locators);

	return 0;
}

static void
acpi_rescan1(struct acpi_softc *sc, const char *ifattr, const int *locators)
{

	if (ifattr_match(ifattr, "acpinodebus"))
		acpi_rescan_nodes(sc);

	if (ifattr_match(ifattr, "acpiapmbus") && sc->sc_apmbus == NULL)
		sc->sc_apmbus = config_found_ia(sc->sc_dev,
		    "acpiapmbus", NULL, NULL);
}

static void
acpi_rescan_nodes(struct acpi_softc *sc)
{
	struct acpi_attach_args aa;
	struct acpi_devnode *ad;

	SIMPLEQ_FOREACH(ad, &sc->sc_devnodes, ad_list) {

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
		 * Handled internally.
		 */
		if (ad->ad_devinfo->Type == ACPI_TYPE_PROCESSOR ||
		    ad->ad_devinfo->Type == ACPI_TYPE_POWER)
			continue;

		/*
		 * Skip ignored HIDs.
		 */
		if (acpi_match_hid(ad->ad_devinfo, acpi_ignored_ids))
			continue;

		ad->ad_device = config_found_ia(sc->sc_dev,
		    "acpinodebus", &aa, acpi_print);
	}
}

#define ACPI_STA_DEV_VALID      \
	(ACPI_STA_DEV_PRESENT | ACPI_STA_DEV_ENABLED | ACPI_STA_DEV_OK)

static void
acpi_rescan_capabilities(struct acpi_softc *sc)
{
	struct acpi_devnode *ad;
	ACPI_DEVICE_INFO *di;
	ACPI_HANDLE tmp;
	ACPI_STATUS rv;

	SIMPLEQ_FOREACH(ad, &sc->sc_devnodes, ad_list) {

		di = ad->ad_devinfo;

		if (di->Type != ACPI_TYPE_DEVICE)
			continue;

		if ((di->Valid & ACPI_VALID_STA) != 0 &&
		    (di->CurrentStatus & ACPI_STA_DEV_VALID) !=
		     ACPI_STA_DEV_VALID)
			continue;

		/*
		 * Scan power resource capabilities.
		 */
		rv = AcpiGetHandle(ad->ad_handle, "_PR0", &tmp);

		if (ACPI_FAILURE(rv))
			rv = AcpiGetHandle(ad->ad_handle, "_PSC", &tmp);

		if (ACPI_SUCCESS(rv))
			ad->ad_flags |= ACPI_DEVICE_POWER;

		/*
		 * Scan wake-up capabilities.
		 */
		rv = AcpiGetHandle(ad->ad_handle, "_PRW", &tmp);

		if (ACPI_SUCCESS(rv)) {
			ad->ad_flags |= ACPI_DEVICE_WAKEUP;
			acpi_wakedev_add(ad);
		}

		if (ad->ad_flags != 0) {
			aprint_debug_dev(sc->sc_dev, "%-5s ", ad->ad_name);

			if ((ad->ad_flags & ACPI_DEVICE_POWER) != 0)
				aprint_debug("power ");

			if ((ad->ad_flags & ACPI_DEVICE_WAKEUP) != 0)
				aprint_debug("wake-up ");

			aprint_debug("\n");
		}
	}
}

#undef ACPI_STA_DEV_VALID

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
					aprint_normal("type %u ",obj->Type);
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

/*
 * Fixed buttons.
 */
static void
acpi_register_fixed_button(struct acpi_softc *sc, int event)
{
	struct sysmon_pswitch *smpsw;
	ACPI_STATUS rv;
	int type;

	switch (event) {

	case ACPI_EVENT_POWER_BUTTON:

		if ((AcpiGbl_FADT.Flags & ACPI_FADT_POWER_BUTTON) != 0)
			return;

		type = PSWITCH_TYPE_POWER;
		smpsw = &sc->sc_smpsw_power;
		break;

	case ACPI_EVENT_SLEEP_BUTTON:

		if ((AcpiGbl_FADT.Flags & ACPI_FADT_SLEEP_BUTTON) != 0)
			return;

		type = PSWITCH_TYPE_SLEEP;
		smpsw = &sc->sc_smpsw_sleep;
		break;

	default:
		rv = AE_TYPE;
		goto fail;
	}

	smpsw->smpsw_type = type;
	smpsw->smpsw_name = device_xname(sc->sc_dev);

	if (sysmon_pswitch_register(smpsw) != 0) {
		rv = AE_ERROR;
		goto fail;
	}

	rv = AcpiInstallFixedEventHandler(event,
	    acpi_fixed_button_handler, smpsw);

	if (ACPI_FAILURE(rv))
		goto fail;

	aprint_debug_dev(sc->sc_dev, "fixed %s button present\n",
	    (type != ACPI_EVENT_SLEEP_BUTTON) ? "power" : "sleep");

	return;

fail:
	aprint_error_dev(sc->sc_dev, "failed to register "
	    "fixed event: %s\n", AcpiFormatException(rv));
}

static void
acpi_deregister_fixed_button(struct acpi_softc *sc, int event)
{
	struct sysmon_pswitch *smpsw;
	ACPI_STATUS rv;

	switch (event) {

	case ACPI_EVENT_POWER_BUTTON:
		smpsw = &sc->sc_smpsw_power;

		if ((AcpiGbl_FADT.Flags & ACPI_FADT_POWER_BUTTON) != 0) {
			KASSERT(smpsw->smpsw_type != PSWITCH_TYPE_POWER);
			return;
		}

		break;

	case ACPI_EVENT_SLEEP_BUTTON:
		smpsw = &sc->sc_smpsw_sleep;

		if ((AcpiGbl_FADT.Flags & ACPI_FADT_SLEEP_BUTTON) != 0) {
			KASSERT(smpsw->smpsw_type != PSWITCH_TYPE_SLEEP);
			return;
		}

		break;

	default:
		rv = AE_TYPE;
		goto fail;
	}

	rv = AcpiRemoveFixedEventHandler(event, acpi_fixed_button_handler);

	if (ACPI_SUCCESS(rv)) {
		sysmon_pswitch_unregister(smpsw);
		return;
	}

fail:
	aprint_error_dev(sc->sc_dev, "failed to deregister "
	    "fixed event: %s\n", AcpiFormatException(rv));
}

static uint32_t
acpi_fixed_button_handler(void *context)
{
	static const int handler = OSL_NOTIFY_HANDLER;
	struct sysmon_pswitch *smpsw = context;

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "%s: fixed event\n", __func__));

	(void)AcpiOsExecute(handler, acpi_fixed_button_pressed, smpsw);

	return ACPI_INTERRUPT_HANDLED;
}

static void
acpi_fixed_button_pressed(void *context)
{
	struct sysmon_pswitch *smpsw = context;

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "%s: %s fixed button pressed\n",
		__func__, smpsw->smpsw_name));

	sysmon_pswitch_event(smpsw, PSWITCH_EVENT_PRESSED);
}

/*
 * Sleep.
 */
static void
acpi_sleep_init(struct acpi_softc *sc)
{
	uint8_t a, b, i;
	ACPI_STATUS rv;

	CTASSERT(ACPI_STATE_S0 == 0 && ACPI_STATE_S1 == 1);
	CTASSERT(ACPI_STATE_S2 == 2 && ACPI_STATE_S3 == 3);
	CTASSERT(ACPI_STATE_S4 == 4 && ACPI_STATE_S5 == 5);

	/*
	 * Evaluate supported sleep states.
	 */
	for (i = ACPI_STATE_S0; i <= ACPI_STATE_S5; i++) {

		rv = AcpiGetSleepTypeData(i, &a, &b);

		if (ACPI_SUCCESS(rv))
			sc->sc_sleepstates |= __BIT(i);
	}
}

ACPI_STATUS
acpi_enter_sleep_state(struct acpi_softc *sc, int state)
{
	ACPI_STATUS rv = AE_OK;
	int err;

	if (state == sc->sc_sleepstate)
		return AE_OK;

	aprint_normal_dev(sc->sc_dev, "entering state S%d\n", state);

	switch (state) {

	case ACPI_STATE_S0:
		break;

	case ACPI_STATE_S1:
	case ACPI_STATE_S2:
	case ACPI_STATE_S3:
	case ACPI_STATE_S4:

		if ((sc->sc_sleepstates & __BIT(state)) == 0) {
			aprint_error_dev(sc->sc_dev, "sleep state "
			    "S%d is not available\n", state);
			break;
		}

		acpi_wakedev_commit(sc, state);

		if (state != ACPI_STATE_S1 &&
		    pmf_system_suspend(PMF_Q_NONE) != true) {
			aprint_error_dev(sc->sc_dev, "aborting suspend\n");
			break;
		}

		rv = AcpiEnterSleepStatePrep(state);

		if (ACPI_FAILURE(rv)) {
			aprint_error_dev(sc->sc_dev, "failed to prepare "
			    "S%d: %s\n", state, AcpiFormatException(rv));
			break;
		}

		sc->sc_sleepstate = state;

		if (state == ACPI_STATE_S1) {

			/* Just enter the state. */
			acpi_md_OsDisableInterrupt();
			rv = AcpiEnterSleepState(state);

			if (ACPI_FAILURE(rv))
				aprint_error_dev(sc->sc_dev, "failed to "
				    "enter S1: %s\n", AcpiFormatException(rv));

			(void)AcpiLeaveSleepState(state);

		} else {

			err = acpi_md_sleep(state);

			if (state == ACPI_STATE_S4)
				AcpiEnable();

			pmf_system_bus_resume(PMF_Q_NONE);
			(void)AcpiLeaveSleepState(state);
			pmf_system_resume(PMF_Q_NONE);
		}

		break;
	case ACPI_STATE_S5:

		rv = AcpiEnterSleepStatePrep(ACPI_STATE_S5);

		if (ACPI_FAILURE(rv)) {
			aprint_error_dev(sc->sc_dev, "failed to prepare "
			    "S%d: %s\n", state, AcpiFormatException(rv));
			break;
		}

		DELAY(1000000);

		sc->sc_sleepstate = state;
		acpi_md_OsDisableInterrupt();

		(void)AcpiEnterSleepState(ACPI_STATE_S5);

		aprint_error_dev(sc->sc_dev, "WARNING: powerdown failed!\n");
		break;
	}

	sc->sc_sleepstate = ACPI_STATE_S0;

	return rv;
}

/*
 * Sysctl.
 */
SYSCTL_SETUP(sysctl_acpi_setup, "sysctl hw.acpi subtree setup")
{
	const struct sysctlnode *mnode, *rnode;
	int err;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw",
	    NULL, NULL, 0, NULL, 0,
	    CTL_HW, CTL_EOL);

	if (err != 0)
		return;

	err = sysctl_createv(clog, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE,
	    "acpi", SYSCTL_DESCR("ACPI subsystem parameters"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	if (err != 0)
		return;

	(void)sysctl_createv(NULL, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READONLY, CTLTYPE_QUAD,
	    "root", SYSCTL_DESCR("ACPI root pointer"),
	    NULL, 0, &acpi_root_pointer, sizeof(acpi_root_pointer),
	    CTL_CREATE, CTL_EOL);

	(void)sysctl_createv(NULL, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READONLY, CTLTYPE_STRING,
	    "supported_states", SYSCTL_DESCR("Supported system states"),
	    sysctl_hw_acpi_sleepstates, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	err = sysctl_createv(NULL, 0, NULL, &mnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep",
	    NULL, NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL);

	if (err == 0) {

		(void)sysctl_createv(NULL, 0, &mnode, NULL,
		    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT,
		    "sleep_state", SYSCTL_DESCR("System sleep state"),
		    sysctl_hw_acpi_sleepstate, 0, NULL, 0,
		    CTL_CREATE, CTL_EOL);
	}

	err = sysctl_createv(clog, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE,
	    "stat", SYSCTL_DESCR("ACPI statistics"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	if (err != 0)
		return;

	(void)sysctl_createv(clog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READONLY, CTLTYPE_QUAD,
	    "gpe", SYSCTL_DESCR("Number of dispatched GPEs"),
	    NULL, 0, &AcpiGpeCount, sizeof(AcpiGpeCount),
	    CTL_CREATE, CTL_EOL);

	(void)sysctl_createv(clog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READONLY, CTLTYPE_QUAD,
	    "sci", SYSCTL_DESCR("Number of SCI interrupts"),
	    NULL, 0, &AcpiSciCount, sizeof(AcpiSciCount),
	    CTL_CREATE, CTL_EOL);

	(void)sysctl_createv(clog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READONLY, CTLTYPE_QUAD,
	    "fixed", SYSCTL_DESCR("Number of fixed events"),
	    sysctl_hw_acpi_fixedstats, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	(void)sysctl_createv(clog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READONLY, CTLTYPE_QUAD,
	    "method", SYSCTL_DESCR("Number of methods executed"),
	    NULL, 0, &AcpiMethodCount, sizeof(AcpiMethodCount),
	    CTL_CREATE, CTL_EOL);

	CTASSERT(sizeof(AcpiGpeCount) == sizeof(uint64_t));
	CTASSERT(sizeof(AcpiSciCount) == sizeof(uint64_t));
}

static int
sysctl_hw_acpi_fixedstats(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint64_t t;
	int err, i;

	for (i = t = 0; i < __arraycount(AcpiFixedEventCount); i++)
		t += AcpiFixedEventCount[i];

	node = *rnode;
	node.sysctl_data = &t;

	err = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (err || newp == NULL)
		return err;

	return 0;
}

static int
sysctl_hw_acpi_sleepstate(SYSCTLFN_ARGS)
{
	struct acpi_softc *sc = acpi_softc;
	struct sysctlnode node;
	int err, t;

	if (acpi_softc == NULL)
		return ENOSYS;

	node = *rnode;
	t = sc->sc_sleepstate;
	node.sysctl_data = &t;

	err = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (err || newp == NULL)
		return err;

	if (t < ACPI_STATE_S0 || t > ACPI_STATE_S5)
		return EINVAL;

	acpi_enter_sleep_state(sc, t);

	return 0;
}

static int
sysctl_hw_acpi_sleepstates(SYSCTLFN_ARGS)
{
	struct acpi_softc *sc = acpi_softc;
	struct sysctlnode node;
	char t[3 * 6 + 1];
	int err;

	if (acpi_softc == NULL)
		return ENOSYS;

	(void)memset(t, '\0', sizeof(t));

	(void)snprintf(t, sizeof(t), "%s%s%s%s%s%s",
	    ((sc->sc_sleepstates & __BIT(0)) != 0) ? "S0 " : "",
	    ((sc->sc_sleepstates & __BIT(1)) != 0) ? "S1 " : "",
	    ((sc->sc_sleepstates & __BIT(2)) != 0) ? "S2 " : "",
	    ((sc->sc_sleepstates & __BIT(3)) != 0) ? "S3 " : "",
	    ((sc->sc_sleepstates & __BIT(4)) != 0) ? "S4 " : "",
	    ((sc->sc_sleepstates & __BIT(5)) != 0) ? "S5 " : "");

	node = *rnode;
	node.sysctl_data = &t;

	err = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (err || newp == NULL)
		return err;

	return 0;
}

/*
 * Miscellaneous.
 */
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
