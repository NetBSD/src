/*	$NetBSD: acpi.c,v 1.87 2006/06/19 02:32:12 jmcneill Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: acpi.c,v 1.87 2006/06/19 02:32:12 jmcneill Exp $");

#include "opt_acpi.h"
#include "opt_pcifixup.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/sysctl.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_osd.h>
#ifdef ACPIVERBOSE
#include <dev/acpi/acpidevs_data.h>
#endif

#if defined(ACPI_PCI_FIXUP)
#error The option ACPI_PCI_FIXUP has been obsoleted by PCI_INTR_FIXUP.  Please adjust your kernel configuration file.
#endif

#ifdef PCI_INTR_FIXUP
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

static int	acpi_match(struct device *, struct cfdata *, void *);
static void	acpi_attach(struct device *, struct device *, void *);

static int	acpi_print(void *aux, const char *);

static int	sysctl_hw_acpi_sleepstate(SYSCTLFN_ARGS);

extern struct cfdriver acpi_cd;

CFATTACH_DECL(acpi, sizeof(struct acpi_softc),
    acpi_match, acpi_attach, NULL, NULL);

/*
 * This is a flag we set when the ACPI subsystem is active.  Machine
 * dependent code may wish to skip other steps (such as attaching
 * subsystems that ACPI supercedes) when ACPI is active.
 */
int	acpi_active;

/*
 * Pointer to the ACPI subsystem's state.  There can be only
 * one ACPI instance.
 */
struct acpi_softc *acpi_softc;

/*
 * Locking stuff.
 */
static struct simplelock acpi_slock;
static int acpi_locked;

/*
 * sysctl-related information
 */

static int acpi_node = CTL_EOL;
static uint64_t acpi_root_pointer;	/* found as hw.acpi.root */
static int acpi_sleepstate = ACPI_STATE_S0;

/*
 * Prototypes.
 */
static void		acpi_shutdown(void *);
static void		acpi_build_tree(struct acpi_softc *);
static ACPI_STATUS	acpi_make_devnode(ACPI_HANDLE, UINT32, void *, void **);

static void		acpi_enable_fixed_events(struct acpi_softc *);
#ifdef PCI_INTR_FIXUP
void			acpi_pci_fixup(struct acpi_softc *);
#endif
#if defined(PCI_INTR_FIXUP) || defined(ACPI_ACTIVATE_DEV)
static ACPI_STATUS	acpi_allocate_resources(ACPI_HANDLE handle);
#endif

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
	ACPI_STATUS rv;

	if (beenhere != 0)
		panic("acpi_probe: ACPI has already been probed");
	beenhere = 1;

	simple_lock_init(&acpi_slock);
	acpi_locked = 0;

	/*
	 * Start up ACPICA.
	 */
#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_INIT)
		acpi_osd_debugger();
#endif

	rv = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(rv)) {
		printf("ACPI: unable to initialize ACPICA: %s\n",
		    AcpiFormatException(rv));
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
		return 0;
	}

	/*
	 * Looks like we have ACPI!
	 */

	return 1;
}

ACPI_STATUS
acpi_OsGetRootPointer(UINT32 Flags, ACPI_POINTER *PhysicalAddress)
{
	ACPI_STATUS rv;

	/*
	 * IA-32: Use AcpiFindRootPointer() to locate the RSDP.
	 *
	 * IA-64: Use the EFI.
	 *
	 * We let MD code handle this since there are multiple
	 * ways to do it.
	 */

	rv = acpi_md_OsGetRootPointer(Flags, PhysicalAddress);

	if (acpi_root_pointer == 0 && ACPI_SUCCESS(rv))
		acpi_root_pointer =
		    (uint64_t)PhysicalAddress->Pointer.Physical;

	return rv;
}

/*
 * acpi_match:
 *
 *	Autoconfiguration `match' routine.
 */
static int
acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	/*
	 * XXX Check other locators?  Hard to know -- machine
	 * dependent code has already checked for the presence
	 * of ACPI by calling acpi_probe(), so I suppose we
	 * don't really have to do anything else.
	 */
	return 1;
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
acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpi_softc *sc = (void *) self;
	struct acpibus_attach_args *aa = aux;
	ACPI_STATUS rv;

	aprint_naive(": Advanced Configuration and Power Interface\n");
	aprint_normal(": Advanced Configuration and Power Interface\n");

	if (acpi_softc != NULL)
		panic("acpi_attach: ACPI has already been attached");

	sysmon_power_settype("acpi");

	aprint_verbose("%s: using Intel ACPI CA subsystem version %08x\n",
	    sc->sc_dev.dv_xname, ACPI_CA_VERSION);

	aprint_verbose("%s: X/RSDT: OemId <%6.6s,%8.8s,%08x>, AslId <%4.4s,%08x>\n",
	    sc->sc_dev.dv_xname,
	    AcpiGbl_XSDT->OemId, AcpiGbl_XSDT->OemTableId,
	    AcpiGbl_XSDT->OemRevision,
	    AcpiGbl_XSDT->AslCompilerId, AcpiGbl_XSDT->AslCompilerRevision);

	sc->sc_quirks = acpi_find_quirks();

	sc->sc_iot = aa->aa_iot;
	sc->sc_memt = aa->aa_memt;
	sc->sc_pc = aa->aa_pc;
	sc->sc_pciflags = aa->aa_pciflags;
	sc->sc_ic = aa->aa_ic;

	acpi_softc = sc;

	/*
	 * Bring ACPI on-line.
	 */
#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_ENABLE)
		acpi_osd_debugger();
#endif

	rv = AcpiEnableSubsystem(0);
	if (ACPI_FAILURE(rv)) {
		aprint_error("%s: unable to enable ACPI: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return;
	}

	/* early EC handler initialization if ECDT table is available */
#if NACPIEC > 0
	acpiec_early_attach(&sc->sc_dev);
#endif

	rv = AcpiInitializeObjects(0);
	if (ACPI_FAILURE(rv)) {
		aprint_error("%s: unable to initialize ACPI objects: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return;
	}
	acpi_active = 1;

	/* Our current state is "awake". */
	sc->sc_sleepstate = ACPI_STATE_S0;

	/* Show SCI interrupt. */
	if (AcpiGbl_FADT != NULL)
		aprint_verbose("%s: SCI interrupting at int %d\n",
		    sc->sc_dev.dv_xname, AcpiGbl_FADT->SciInt);
	/*
	 * Check for fixed-hardware features.
	 */
	acpi_enable_fixed_events(sc);

	/*
	 * Fix up PCI devices.
	 */
#ifdef PCI_INTR_FIXUP
	if ((sc->sc_quirks & (ACPI_QUIRK_BADPCI | ACPI_QUIRK_BADIRQ)) == 0)
		acpi_pci_fixup(sc);
#endif

	/*
	 * Scan the namespace and build our device tree.
	 */
#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_PROBE)
		acpi_osd_debugger();
#endif
	acpi_md_callback((struct device *)sc);
	acpi_build_tree(sc);

	if (acpi_root_pointer != 0 && acpi_node != CTL_EOL) {
		(void)sysctl_createv(NULL, 0, NULL, NULL,
		    CTLFLAG_IMMEDIATE,
		    CTLTYPE_QUAD, "root", NULL, NULL,
		    acpi_root_pointer, NULL, 0,
		    CTL_HW, acpi_node, CTL_CREATE, CTL_EOL);
	}


	/*
	 * Register a shutdown hook that disables certain ACPI
	 * events that might happen and confuse us while we're
	 * trying to shut down.
	 */
	sc->sc_sdhook = shutdownhook_establish(acpi_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		aprint_error("%s: WARNING: unable to register shutdown hook\n",
		    sc->sc_dev.dv_xname);

#ifdef ACPI_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_RUNNING)
		acpi_osd_debugger();
#endif
}

/*
 * acpi_shutdown:
 *
 *	Shutdown hook for ACPI -- disable some events that
 *	might confuse us.
 */
static void
acpi_shutdown(void *arg)
{
	/* nothing */
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
		"\\_SI_",	/* system idicator namespace */
		"\\_TZ_",	/* ACPI 1.0 thermal zone namespace */
		NULL,
	};
	struct acpi_attach_args aa;
	struct acpi_make_devnode_state state;
	struct acpi_scope *as;
	struct acpi_devnode *ad;
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

		/* Now, for this namespace, try and attach the devices. */
		TAILQ_FOREACH(ad, &as->as_devnodes, ad_list) {
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

				/*
				 * XXX Same problem as above...
				 */
				if ((ad->ad_devinfo->Valid & ACPI_VALID_HID)
				    == 0)
					continue;
			}

			ad->ad_device = config_found(&sc->sc_dev,
			    &aa, acpi_print);
		}
	}
}

#ifdef ACPI_ACTIVATE_DEV
static void
acpi_activate_device(ACPI_HANDLE handle, ACPI_DEVICE_INFO **di)
{
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_BUFFER;

#ifdef ACPI_DEBUG
	aprint_normal("acpi_activate_device: %s, old status=%x\n",
	       (*di)->HardwareId.Value, (*di)->CurrentStatus);
#endif

	rv = acpi_allocate_resources(handle);
	if (ACPI_FAILURE(rv)) {
		aprint_error("acpi: activate failed for %s\n",
		       (*di)->HardwareId.Value);
	} else {
		aprint_normal("acpi: activated %s\n", (*di)->HardwareId.Value);
	}

	(void)AcpiGetObjectInfo(handle, &buf);
	AcpiOsFree(*di);
	*di = buf.Pointer;

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
	ACPI_BUFFER buf;
	ACPI_DEVICE_INFO *devinfo;
	ACPI_STATUS rv;

	rv = AcpiGetType(handle, &type);
	if (ACPI_SUCCESS(rv)) {
		buf.Pointer = NULL;
		buf.Length = ACPI_ALLOCATE_BUFFER;
		rv = AcpiGetObjectInfo(handle, &buf);
		if (ACPI_FAILURE(rv)) {
#ifdef ACPI_DEBUG
			aprint_normal("%s: AcpiGetObjectInfo failed: %s\n",
			    sc->sc_dev.dv_xname, AcpiFormatException(rv));
#endif
			goto out; /* XXX why return OK */
		}

		devinfo = buf.Pointer;

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

			TAILQ_INSERT_TAIL(&as->as_devnodes, ad, ad_list);

			if ((ad->ad_devinfo->Valid & ACPI_VALID_HID) == 0)
				goto out;

#ifdef ACPI_EXTRA_DEBUG
			aprint_normal("%s: HID %s found in scope %s level %d\n",
			    sc->sc_dev.dv_xname,
			    ad->ad_devinfo->HardwareId.Value,
			    as->as_name, ad->ad_level);
			if (ad->ad_devinfo->Valid & ACPI_VALID_UID)
				aprint_normal("       UID %s\n",
				    ad->ad_devinfo->UniqueId.Value);
			if (ad->ad_devinfo->Valid & ACPI_VALID_ADR)
				aprint_normal("       ADR 0x%016qx\n",
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
 *	Autoconfiguration print routine.
 */
static int
acpi_print(void *aux, const char *pnp)
{
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;

	if (pnp) {
		if (aa->aa_node->ad_devinfo->Valid & ACPI_VALID_HID) {
			char *pnpstr =
			    aa->aa_node->ad_devinfo->HardwareId.Value;
			char *str;

			aprint_normal("%s ", pnpstr);
			rv = acpi_eval_string(aa->aa_node->ad_handle,
			    "_STR", &str);
			if (ACPI_SUCCESS(rv)) {
				aprint_normal("[%s] ", str);
				AcpiOsFree(str);
			}
#ifdef ACPIVERBOSE
			else {
				int i;

				for (i = 0; i < sizeof(acpi_knowndevs) /
				    sizeof(acpi_knowndevs[0]); i++) {
					if (strcmp(acpi_knowndevs[i].pnp,
					    pnpstr) == 0) {
						aprint_normal("[%s] ",
						    acpi_knowndevs[i].str);
					}
				}
			}

#endif
		} else {
			aprint_normal("ACPI Object Type '%s' (0x%02x) ",
			   AcpiUtGetTypeName(aa->aa_node->ad_devinfo->Type),
			   aa->aa_node->ad_devinfo->Type);
		}
		aprint_normal("at %s", pnp);
	} else {
		if (aa->aa_node->ad_devinfo->Valid & ACPI_VALID_HID) {
			aprint_normal(" (%s", aa->aa_node->ad_devinfo->HardwareId.Value);
			if (aa->aa_node->ad_devinfo->Valid & ACPI_VALID_UID) {
				const char *uid;

				uid = aa->aa_node->ad_devinfo->UniqueId.Value;
				if (uid[0] == '\0')
					uid = "<null>";
				aprint_normal("-%s", uid);
			}
			aprint_normal(")");
		}
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

	if (AcpiGbl_FADT != NULL && AcpiGbl_FADT->PwrButton == 0) {
		aprint_normal("%s: fixed-feature power button present\n",
		    sc->sc_dev.dv_xname);
		sc->sc_smpsw_power.smpsw_name = sc->sc_dev.dv_xname;
		sc->sc_smpsw_power.smpsw_type = PSWITCH_TYPE_POWER;
		if (sysmon_pswitch_register(&sc->sc_smpsw_power) != 0) {
			aprint_error("%s: unable to register fixed power "
			    "button with sysmon\n", sc->sc_dev.dv_xname);
		} else {
			rv = AcpiInstallFixedEventHandler(
			    ACPI_EVENT_POWER_BUTTON,
			    acpi_fixed_button_handler, &sc->sc_smpsw_power);
			if (ACPI_FAILURE(rv)) {
				aprint_error("%s: unable to install handler "
				    "for fixed power button: %s\n",
				    sc->sc_dev.dv_xname,
				    AcpiFormatException(rv));
			}
		}
	}

	if (AcpiGbl_FADT != NULL && AcpiGbl_FADT->SleepButton == 0) {
		aprint_normal("%s: fixed-feature sleep button present\n",
		    sc->sc_dev.dv_xname);
		sc->sc_smpsw_sleep.smpsw_name = sc->sc_dev.dv_xname;
		sc->sc_smpsw_sleep.smpsw_type = PSWITCH_TYPE_SLEEP;
		if (sysmon_pswitch_register(&sc->sc_smpsw_power) != 0) {
			aprint_error("%s: unable to register fixed sleep "
			    "button with sysmon\n", sc->sc_dev.dv_xname);
		} else {
			rv = AcpiInstallFixedEventHandler(
			    ACPI_EVENT_SLEEP_BUTTON,
			    acpi_fixed_button_handler, &sc->sc_smpsw_sleep);
			if (ACPI_FAILURE(rv)) {
				aprint_error("%s: unable to install handler "
				    "for fixed sleep button: %s\n",
				    sc->sc_dev.dv_xname,
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
	int rv;

#ifdef ACPI_BUT_DEBUG
	printf("%s: fixed button handler\n", smpsw->smpsw_name);
#endif

	rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
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

	rv = AcpiEvaluateObjectTyped(handle, path, NULL, &buf, ACPI_TYPE_INTEGER);
	if (ACPI_SUCCESS(rv))
		*valp = param.Integer.Value;

	return rv;
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
	buf.Length = ACPI_ALLOCATE_BUFFER;

	rv = AcpiEvaluateObjectTyped(handle, path, NULL, &buf, ACPI_TYPE_STRING);
	if (ACPI_SUCCESS(rv)) {
		ACPI_OBJECT *param = buf.Pointer;
		const char *ptr = param->String.Pointer;
		size_t len = param->String.Length;
		if ((*stringp = AcpiOsAllocate(len)) == NULL)
			rv = AE_NO_MEMORY;
		else
			(void)memcpy(*stringp, ptr, len);
		AcpiOsFree(param);
	}

	return rv;
}


/*
 * acpi_eval_struct:
 *
 *	Evaluate a more complex structure.
 *	Caller must free buf.Pointer by AcpiOsFree().
 */
ACPI_STATUS
acpi_eval_struct(ACPI_HANDLE handle, const char *path, ACPI_BUFFER *bufp)
{
	ACPI_STATUS rv;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	bufp->Pointer = NULL;
	bufp->Length = ACPI_ALLOCATE_BUFFER;

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
 *	Caller must free buf.Pointer by AcpiOsFree().
 */
ACPI_STATUS
acpi_get(ACPI_HANDLE handle, ACPI_BUFFER *buf,
    ACPI_STATUS (*getit)(ACPI_HANDLE, ACPI_BUFFER *))
{
	buf->Pointer = NULL;
	buf->Length = ACPI_ALLOCATE_BUFFER;

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
			if (pmatch(ad->HardwareId.Value, *ids, NULL) == 2)
				return 1;
		}

		if (ad->Valid & ACPI_VALID_CID) {
			for (i = 0; i < ad->CompatibilityId.Count; i++) {
				if (pmatch(ad->CompatibilityId.Id[i].Value, *ids, NULL) == 2)
					return 1;
			}
		}
		ids++;
	}

	return 0;
}

/*
 * acpi_set_wake_gpe
 *
 *	Set GPE as both Runtime and Wake
 */
void
acpi_set_wake_gpe(ACPI_HANDLE handle)
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
	AcpiSetGpeType(NULL, elt[0].Integer.Value, ACPI_GPE_TYPE_WAKE_RUN);
	AcpiEnableGpe(NULL, elt[0].Integer.Value, ACPI_NOT_ISR);

 out:
	AcpiOsFree(buf.Pointer);
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
	int s;
	ACPI_STATUS ret = AE_OK;

	switch (state) {
	case ACPI_STATE_S0:
		break;
	case ACPI_STATE_S1:
	case ACPI_STATE_S2:
	case ACPI_STATE_S3:
	case ACPI_STATE_S4:
		if (!is_available_state(sc, state)) {
			printf("acpi: cannot enter the sleep state (%d).\n",
			       state);
			break;
		}
		ret = AcpiEnterSleepStatePrep(state);
		if (ACPI_FAILURE(ret)) {
			printf("acpi: failed preparing to sleep (%s)\n",
			       AcpiFormatException(ret));
			break;
		}
		if (state==ACPI_STATE_S1) {
			/* just enter the state */
			acpi_md_OsDisableInterrupt();
			AcpiEnterSleepState((UINT8)state);
			AcpiUtReleaseMutex(ACPI_MTX_HARDWARE);
		} else {
			/* XXX: powerhooks(9) framework is too poor to
			 * support ACPI sleep state...
			 */
			dopowerhooks(PWR_SOFTSUSPEND);
			s = splhigh();
			dopowerhooks(PWR_SUSPEND);
			acpi_md_sleep(state);
			dopowerhooks(PWR_RESUME);
#ifdef PCI_INTR_FIXUP
			if (state==ACPI_STATE_S3)
				acpi_pci_fixup(sc);
#endif
			splx(s);
			dopowerhooks(PWR_SOFTRESUME);
			if (state==ACPI_STATE_S4)
				AcpiEnable();
		}
		AcpiLeaveSleepState((UINT8)state);
		break;
	case ACPI_STATE_S5:
		ret = AcpiEnterSleepStatePrep(ACPI_STATE_S5);
		if (ACPI_FAILURE(ret)) {
			printf("acpi: failed preparing to sleep (%s)\n",
			       AcpiFormatException(ret));
			break;
		}
		acpi_md_OsDisableInterrupt();
		AcpiEnterSleepState(ACPI_STATE_S5);
		printf("WARNING: powerdown failed!\n");
		break;
	}

	return ret;
}

#ifdef PCI_INTR_FIXUP
ACPI_STATUS acpi_pci_fixup_bus(ACPI_HANDLE, UINT32, void *, void **);
/*
 * acpi_pci_fixup:
 *
 *	Set up PCI devices that BIOS didn't handle right.
 *	Iterate through all devices and try to get the _PTR
 *	(PCI Routing Table).  If it exists then make sure all
 *	interrupt links that it uses are working.
 */
void
acpi_pci_fixup(struct acpi_softc *sc)
{
	ACPI_HANDLE parent;
	ACPI_STATUS rv;

#ifdef ACPI_DEBUG
	printf("acpi_pci_fixup starts:\n");
#endif
	rv = AcpiGetHandle(ACPI_ROOT_OBJECT, "\\_SB_", &parent);
	if (ACPI_FAILURE(rv))
		return;
	sc->sc_pci_bus = 0;
	AcpiWalkNamespace(ACPI_TYPE_DEVICE, parent, 100,
	    acpi_pci_fixup_bus, sc, NULL);
}

static uint
acpi_get_intr(ACPI_HANDLE handle)
{
	ACPI_BUFFER ret;
	ACPI_STATUS rv;
	ACPI_RESOURCE *res;
	ACPI_RESOURCE_IRQ *irq;
	uint intr;

	intr = -1;
	rv = acpi_get(handle, &ret, AcpiGetCurrentResources);
	if (ACPI_FAILURE(rv))
		return intr;
	for (res = ret.Pointer; res->Type != ACPI_RESOURCE_TYPE_END_TAG;
	     res = ACPI_NEXT_RESOURCE(res)) {
		if (res->Type == ACPI_RESOURCE_TYPE_IRQ) {
			irq = (ACPI_RESOURCE_IRQ *)&res->Data;
			if (irq->InterruptCount == 1)
				intr = irq->Interrupts[0];
			break;
		}
	}
	AcpiOsFree(ret.Pointer);
	return intr;
}

static void
acpi_pci_set_line(int bus, int dev, int pin, int line)
{
	ACPI_STATUS err;
	ACPI_PCI_ID pid;
	UINT32 intr, id, bhlc;
	int func, nfunc;

	pid.Bus = bus;
	pid.Device = dev;
	pid.Function = 0;

	err = AcpiOsReadPciConfiguration(&pid, PCI_BHLC_REG, &bhlc, 32);
	if (err)
		return;
	if (PCI_HDRTYPE_MULTIFN(bhlc))
		nfunc = 8;
	else
		nfunc = 1;

	for (func = 0; func < nfunc; func++) {
		pid.Function = func;

		err = AcpiOsReadPciConfiguration(&pid, PCI_ID_REG, &id, 32);
		if (err || PCI_VENDOR(id) == PCI_VENDOR_INVALID ||
		    PCI_VENDOR(id) == 0)
			continue;

		err = AcpiOsReadPciConfiguration(&pid, PCI_INTERRUPT_REG,
			  &intr, 32);
		if (err) {
			printf("AcpiOsReadPciConfiguration failed %d\n", err);
			return;
		}
		if (pin == PCI_INTERRUPT_PIN(intr) &&
		    line != PCI_INTERRUPT_LINE(intr)) {
#ifdef ACPI_DEBUG
			printf("acpi fixup pci intr: %d:%d:%d %c: %d -> %d\n",
			       bus, dev, func,
			       pin + '@', PCI_INTERRUPT_LINE(intr),
			       line);
#endif
			intr &= ~(PCI_INTERRUPT_LINE_MASK <<
				  PCI_INTERRUPT_LINE_SHIFT);
			intr |= line << PCI_INTERRUPT_LINE_SHIFT;
			err = AcpiOsWritePciConfiguration(&pid,
				  PCI_INTERRUPT_REG, intr, 32);
			if (err) {
				printf("AcpiOsWritePciConfiguration failed"
				       " %d\n", err);
				return;
			}
		}
	}
}

ACPI_STATUS
acpi_pci_fixup_bus(ACPI_HANDLE handle, UINT32 level, void *context,
		   void **status)
{
	struct acpi_softc *sc = context;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;
	UINT8 *Buffer;
	ACPI_PCI_ROUTING_TABLE *PrtElement;
	ACPI_HANDLE link;
	uint line;
	ACPI_INTEGER val;

	rv = acpi_get(handle, &buf, AcpiGetIrqRoutingTable);
	if (ACPI_FAILURE(rv))
		return AE_OK;

	/*
	 * If at level 1, this is a PCI root bus. Try the _BBN method
	 * to get the right PCI bus numbering for the following
	 * busses (this is a depth-first walk). It may fail,
	 * for example if there's only one root bus, but that
	 * case should be ok, so we'll ignore that.
	 */
	if (level == 1) {
		rv = acpi_eval_integer(handle, METHOD_NAME__BBN, &val);
		if (!ACPI_FAILURE(rv)) {
#ifdef ACPI_DEBUG
			printf("%s: fixup: _BBN success, bus # was %d now %d\n",
			    sc->sc_dev.dv_xname, sc->sc_pci_bus,
			    ACPI_LOWORD(val));
#endif
			sc->sc_pci_bus = ACPI_LOWORD(val);
		}
	}


#ifdef ACPI_DEBUG
	printf("%s: fixing up PCI bus %d at level %u\n", sc->sc_dev.dv_xname,
	    sc->sc_pci_bus, level);
#endif

        for (Buffer = buf.Pointer; ; Buffer += PrtElement->Length) {
		PrtElement = (ACPI_PCI_ROUTING_TABLE *)Buffer;
		if (PrtElement->Length == 0)
			break;
		if (PrtElement->Source[0] == 0)
			continue;

		rv = AcpiGetHandle(NULL, PrtElement->Source, &link);
		if (ACPI_FAILURE(rv))
			continue;
		line = acpi_get_intr(link);
		if (line == (uint)-1 || line == 0) {
			printf("%s: fixing up intr link %s\n",
			    sc->sc_dev.dv_xname, PrtElement->Source);
			rv = acpi_allocate_resources(link);
			if (ACPI_FAILURE(rv)) {
				printf("%s: interrupt allocation failed %s\n",
				    sc->sc_dev.dv_xname, PrtElement->Source);
				continue;
			}
			line = acpi_get_intr(link);
			if (line == (uint)-1) {
				printf("%s: get intr failed %s\n",
				    sc->sc_dev.dv_xname, PrtElement->Source);
				continue;
			}
		}

		acpi_pci_set_line(sc->sc_pci_bus, PrtElement->Address >> 16,
		    PrtElement->Pin + 1, line);
	}

	sc->sc_pci_bus++;

	AcpiOsFree(buf.Pointer);
	return AE_OK;
}
#endif /* PCI_INTR_FIXUP */

#if defined(PCI_INTR_FIXUP) || defined(ACPI_ACTIVATE_DEV)
/* XXX This very incomplete */
static ACPI_STATUS
acpi_allocate_resources(ACPI_HANDLE handle)
{
	ACPI_BUFFER bufp, bufc, bufn;
	ACPI_RESOURCE *resp, *resc, *resn;
	ACPI_RESOURCE_IRQ *irq;
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
	AcpiOsFree(bufc.Pointer);
out1:
	AcpiOsFree(bufp.Pointer);
out:
	return rv;
}
#endif /* PCI_INTR_FIXUP || ACPI_ACTIVATE_DEV */

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

	acpi_node = node->sysctl_num;

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

	if (t < ACPI_STATE_S0 || t > ACPI_STATE_S5)
		return EINVAL;

	if (acpi_softc != NULL && acpi_sleepstate != t) {
		acpi_sleepstate = t;
		aprint_normal("acpi0: entering state %d\n", t);
		acpi_enter_sleep_state(acpi_softc, t);
		aprint_normal("acpi0: resuming\n");
		t = acpi_sleepstate = ACPI_STATE_S0;
	}

	return 0;
}
