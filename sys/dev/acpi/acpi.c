/*	$NetBSD: acpi.c,v 1.32 2003/02/14 11:05:39 tshiozak Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: acpi.c,v 1.32 2003/02/14 11:05:39 tshiozak Exp $");

#include "opt_acpi.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_osd.h>
#ifdef ACPIVERBOSE
#include <dev/acpi/acpidevs_data.h>
#endif

#ifndef ACPI_PCI_FIXUP
#define ACPI_PCI_FIXUP 1
#endif

#ifndef ACPI_ACTIVATE_DEV
#define ACPI_ACTIVATE_DEV 0
#endif

#if ACPI_PCI_FIXUP
#include <dev/acpi/acpica/Subsystem/acnamesp.h> /* AcpiNsGetNodeByPath() */
#include <dev/pci/pcidevs.h>
#endif

#include <machine/acpi_machdep.h>

#ifdef ENABLE_DEBUGGER
#define	ACPI_DBGR_INIT		0x01
#define	ACPI_DBGR_TABLES	0x02
#define	ACPI_DBGR_ENABLE	0x04
#define	ACPI_DBGR_PROBE		0x08
#define	ACPI_DBGR_RUNNING	0x10

int	acpi_dbgr = 0x00;
#endif

int	acpi_match(struct device *, struct cfdata *, void *);
void	acpi_attach(struct device *, struct device *, void *);

int	acpi_print(void *aux, const char *);

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
 * Prototypes.
 */
void		acpi_shutdown(void *);
ACPI_STATUS	acpi_disable(struct acpi_softc *sc);
void		acpi_build_tree(struct acpi_softc *);
ACPI_STATUS	acpi_make_devnode(ACPI_HANDLE, UINT32, void *, void **);

void		acpi_enable_fixed_events(struct acpi_softc *);
#if ACPI_PCI_FIXUP
void		acpi_pci_fixup(struct acpi_softc *);
#endif
#if ACPI_PCI_FIXUP || ACPI_ACTIVATE_DEV
ACPI_STATUS	acpi_allocate_resources(ACPI_HANDLE handle);
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
#ifdef ENABLE_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_INIT)
		acpi_osd_debugger();
#endif

	rv = AcpiInitializeSubsystem();
	if (rv != AE_OK) {
		printf("ACPI: unable to initialize ACPICA: %d\n", rv);
		return (0);
	}

#ifdef ENABLE_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_TABLES)
		acpi_osd_debugger();
#endif

	rv = AcpiLoadTables();
	if (rv != AE_OK) {
		printf("ACPI: unable to load tables: %d\n", rv);
		return (0);
	}

	/*
	 * Looks like we have ACPI!
	 */

	return (1);
}

/*
 * acpi_match:
 *
 *	Autoconfiguration `match' routine.
 */
int
acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpibus_attach_args *aa = aux;

	if (strcmp(aa->aa_busname, acpi_cd.cd_name) != 0)
		return (0);

	/*
	 * XXX Check other locators?  Hard to know -- machine
	 * dependent code has already checked for the presence
	 * of ACPI by calling acpi_probe(), so I suppose we
	 * don't really have to do anything else.
	 */
	return (1);
}

/*
 * acpi_attach:
 *
 *	Autoconfiguration `attach' routine.  Finish initializing
 *	ACPICA (some initialization was done in acpi_probe(),
 *	which was required to check for the presence of ACPI),
 *	and enable the ACPI subsystem.
 */
void
acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpi_softc *sc = (void *) self;
	struct acpibus_attach_args *aa = aux;
	ACPI_TABLE_HEADER *ap = &AcpiGbl_XSDT->Header;
	ACPI_STATUS rv;

	printf("\n");

	if (acpi_softc != NULL)
		panic("acpi_attach: ACPI has already been attached");

	printf("%s: X/RSDT: OemId <%6.6s,%8.8s,%08x>, AslId <%4.4s,%08x>\n",
	    sc->sc_dev.dv_xname,
	    ap->OemId, ap->OemTableId, ap->OemRevision,
	    ap->AslCompilerId, ap->AslCompilerRevision);

	sc->sc_iot = aa->aa_iot;
	sc->sc_memt = aa->aa_memt;
	sc->sc_pc = aa->aa_pc;
	sc->sc_pciflags = aa->aa_pciflags;
	sc->sc_ic = aa->aa_ic;

	acpi_softc = sc;

	/*
	 * Install the default address space handlers.
	 */

	rv = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
	    ACPI_ADR_SPACE_SYSTEM_MEMORY, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (rv != AE_OK) {
		printf("%s: unable to install SYSTEM MEMORY handler: %d\n",
		    sc->sc_dev.dv_xname, rv);
		return;
	}

	rv = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
	    ACPI_ADR_SPACE_SYSTEM_IO, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (rv != AE_OK) {
		printf("%s: unable to install SYSTEM IO handler: %d\n",
		    sc->sc_dev.dv_xname, rv);
		return;
	}

	rv = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
	    ACPI_ADR_SPACE_PCI_CONFIG, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (rv != AE_OK) {
		printf("%s: unable to install PCI CONFIG handler: %d\n",
		    sc->sc_dev.dv_xname, rv);
		return;
	}

	/*
	 * Bring ACPI on-line.
	 *
	 * Note that we request that _STA (device init) and _INI (object init)
	 * methods not be run.
	 *
	 * XXX We need to arrange for the object init pass after we have
	 * XXX attached all of our children.
	 */
#ifdef ENABLE_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_ENABLE)
		acpi_osd_debugger();
#endif
	rv = AcpiEnableSubsystem(ACPI_NO_DEVICE_INIT | ACPI_NO_OBJECT_INIT);
	if (rv != AE_OK) {
		printf("%s: unable to enable ACPI: %d\n",
		    sc->sc_dev.dv_xname, rv);
		return;
	}
	acpi_active = 1;

	/*
	 * Set up the default sleep state to enter when various
	 * switches are activated.
	 */
	sc->sc_switch_sleep[ACPI_SWITCH_POWERBUTTON] = ACPI_STATE_S5;
	sc->sc_switch_sleep[ACPI_SWITCH_SLEEPBUTTON] = ACPI_STATE_S1;
	sc->sc_switch_sleep[ACPI_SWITCH_LID]	     = ACPI_STATE_S1;

	/* Our current state is "awake". */
	sc->sc_sleepstate = ACPI_STATE_S0;

	/* Show SCI interrupt. */
	if (AcpiGbl_FADT != NULL)
		printf("%s: SCI interrupting at int %d\n",
			sc->sc_dev.dv_xname, AcpiGbl_FADT->SciInt);
	/*
	 * Check for fixed-hardware features.
	 */
	acpi_enable_fixed_events(sc);

	/*
	 * Fix up PCI devices.
	 */
#if ACPI_PCI_FIXUP
	acpi_pci_fixup(sc);
#endif
 
	/*
	 * Scan the namespace and build our device tree.
	 */
#ifdef ENABLE_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_PROBE)
		acpi_osd_debugger();
#endif
	acpi_md_callback((struct device *)sc);
	acpi_build_tree(sc);

	/*
	 * Register a shutdown hook that disables certain ACPI
	 * events that might happen and confuse us while we're
	 * trying to shut down.
	 */
	sc->sc_sdhook = shutdownhook_establish(acpi_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		printf("%s: WARNING: unable to register shutdown hook\n",
		    sc->sc_dev.dv_xname);

#ifdef ENABLE_DEBUGGER
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
void
acpi_shutdown(void *arg)
{
	struct acpi_softc *sc = arg;

	if (acpi_disable(sc) != AE_OK)
		printf("%s: WARNING: unable to disable ACPI\n",
		    sc->sc_dev.dv_xname);
}

/*
 * acpi_disable:
 *
 *	Disable ACPI.
 */
ACPI_STATUS
acpi_disable(struct acpi_softc *sc)
{
	ACPI_STATUS rv = AE_OK;

	if (acpi_active) {
		rv = AcpiDisable();
		if (rv == AE_OK)
			acpi_active = 0;
	}
	return (rv);
}

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
void
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
	int i;

	TAILQ_INIT(&sc->sc_scopes);

	state.softc = sc;

	/*
	 * Scan the namespace and build our tree.
	 */
	for (i = 0; scopes[i] != NULL; i++) {
		as = malloc(sizeof(*as), M_DEVBUF, M_WAITOK);
		as->as_name = scopes[i];
		TAILQ_INIT(&as->as_devnodes);

		TAILQ_INSERT_TAIL(&sc->sc_scopes, as, as_list);

		state.scope = as;

		if (AcpiGetHandle(ACPI_ROOT_OBJECT, (char *) scopes[i],
		    &parent) == AE_OK) {
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

			if (ad->ad_devinfo.Type == ACPI_TYPE_DEVICE) {
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
				if ((ad->ad_devinfo.CurrentStatus &
				     (ACPI_STA_DEV_PRESENT|ACPI_STA_DEV_ENABLED|
				      ACPI_STA_DEV_OK)) !=
				    (ACPI_STA_DEV_PRESENT|ACPI_STA_DEV_ENABLED|
				     ACPI_STA_DEV_OK))
					continue;

				/*
				 * XXX Same problem as above...
				 */
				if ((ad->ad_devinfo.Valid & ACPI_VALID_HID)
				    == 0)
					continue;
			}

			ad->ad_device = config_found(&sc->sc_dev,
			    &aa, acpi_print);
		}
	}
}

#if ACPI_ACTIVATE_DEV
static void
acpi_activate_device(ACPI_HANDLE handle, ACPI_DEVICE_INFO *di)
{
	ACPI_STATUS rv;

#ifdef ACPI_DEBUG
	printf("acpi_activate_device: %s, old status=%x\n", 
	       di->HardwareId, di->CurrentStatus);
#endif

	rv = acpi_allocate_resources(handle);
	if (ACPI_FAILURE(rv)) {
		printf("acpi: activate failed for %s\n", di->HardwareId);
	} else {
		printf("acpi: activated %s\n", di->HardwareId);
	}

	(void)AcpiGetObjectInfo(handle, di);
#ifdef ACPI_DEBUG
	printf("acpi_activate_device: %s, new status=%x\n", 
	       di->HardwareId, di->CurrentStatus);
#endif
}
#endif /* ACPI_ACTIVATE_DEV */

/*
 * acpi_make_devnode:
 *
 *	Make an ACPI devnode.
 */
ACPI_STATUS
acpi_make_devnode(ACPI_HANDLE handle, UINT32 level, void *context,
    void **status)
{
	struct acpi_make_devnode_state *state = context;
#if defined(ACPI_DEBUG) || defined(ACPI_EXTRA_DEBUG)
	struct acpi_softc *sc = state->softc;
#endif
	struct acpi_scope *as = state->scope;
	struct acpi_devnode *ad;
	ACPI_DEVICE_INFO devinfo;
	ACPI_OBJECT_TYPE type;
	ACPI_STATUS rv;

	if (AcpiGetType(handle, &type) == AE_OK) {
		rv = AcpiGetObjectInfo(handle, &devinfo);
		if (rv != AE_OK) {
#ifdef ACPI_DEBUG
			printf("%s: AcpiGetObjectInfo failed\n",
			    sc->sc_dev.dv_xname);
#endif
			goto out; /* XXX why return OK */
		}

		switch (type) {
		case ACPI_TYPE_DEVICE:
#if ACPI_ACTIVATE_DEV
			if ((devinfo.Valid & ACPI_VALID_STA) &&
			    (devinfo.CurrentStatus &
			     (ACPI_STA_DEV_PRESENT|ACPI_STA_DEV_ENABLED)) ==
			    ACPI_STA_DEV_PRESENT)
				acpi_activate_device(handle, &devinfo);
			/* FALLTHROUGH */
#endif

		case ACPI_TYPE_PROCESSOR:
		case ACPI_TYPE_THERMAL:
		case ACPI_TYPE_POWER:
			ad = malloc(sizeof(*ad), M_DEVBUF, M_NOWAIT|M_ZERO);
			if (ad == NULL)
				return (AE_NO_MEMORY);

			ad->ad_handle = handle;
			ad->ad_level = level;
			ad->ad_scope = as;
			ad->ad_type = type;

			TAILQ_INSERT_TAIL(&as->as_devnodes, ad, ad_list);

			rv = AcpiGetObjectInfo(handle, &ad->ad_devinfo);
			if (rv != AE_OK)
				goto out;

			if ((ad->ad_devinfo.Valid & ACPI_VALID_HID) == 0)
				goto out;

#ifdef ACPI_EXTRA_DEBUG
			printf("%s: HID %s found in scope %s level %d\n",
			    sc->sc_dev.dv_xname, ad->ad_devinfo.HardwareId,
			    as->as_name, ad->ad_level);
			if (ad->ad_devinfo.Valid & ACPI_VALID_UID)
				printf("       UID %s\n",
				    ad->ad_devinfo.UniqueId);
			if (ad->ad_devinfo.Valid & ACPI_VALID_ADR)
				printf("       ADR 0x%016qx\n",
				    ad->ad_devinfo.Address);
			if (ad->ad_devinfo.Valid & ACPI_VALID_STA)
				printf("       STA 0x%08x\n",
				    ad->ad_devinfo.CurrentStatus);
#endif
		}
	}
 out:
	return (AE_OK);
}

/*
 * acpi_print:
 *
 *	Autoconfiguration print routine.
 */
int
acpi_print(void *aux, const char *pnp)
{
	struct acpi_attach_args *aa = aux;

	if (pnp) {
		if (aa->aa_node->ad_devinfo.Valid & ACPI_VALID_HID) {
			char *pnpstr = aa->aa_node->ad_devinfo.HardwareId;
			char *str;

			aprint_normal("%s ", pnpstr);
			if (acpi_eval_string(aa->aa_node->ad_handle,
			    "_STR", &str) == AE_OK) {
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
						printf("[%s] ",
						    acpi_knowndevs[i].str);
					}
				}
			}
			    
#endif
		} else {
			aprint_normal("ACPI Object Type '%s' (0x%02x) ",
			   AcpiUtGetTypeName(aa->aa_node->ad_devinfo.Type),
			   aa->aa_node->ad_devinfo.Type);
		}
		aprint_normal("at %s", pnp);
	} else {
		aprint_normal(" (%s", aa->aa_node->ad_devinfo.HardwareId);
		if (aa->aa_node->ad_devinfo.Valid & ACPI_VALID_UID) {
			char *uid;

			if (aa->aa_node->ad_devinfo.UniqueId[0] == '\0')
				uid = "<null>";
			else
				uid = aa->aa_node->ad_devinfo.UniqueId;
			aprint_normal("-%s", uid);
		}
		aprint_normal(")");
	}

	return (UNCONF);
}

/*****************************************************************************
 * ACPI fixed-hardware feature handlers
 *****************************************************************************/

UINT32		acpi_fixed_power_button_handler(void *);
UINT32		acpi_fixed_sleep_button_handler(void *);

/*
 * acpi_enable_fixed_events:
 *
 *	Enable any fixed-hardware feature handlers.
 */
void
acpi_enable_fixed_events(struct acpi_softc *sc)
{
	static int beenhere;
	ACPI_STATUS rv;

	/*
	 * Check for fixed-hardware buttons.
	 */

	if (AcpiGbl_FADT != NULL && AcpiGbl_FADT->PwrButton == 0) {
		if (beenhere == 0)
			printf("%s: fixed-feature power button present\n",
			    sc->sc_dev.dv_xname);
		rv = AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON,
		    acpi_fixed_power_button_handler, sc);
		if (rv != AE_OK)
			printf("%s: unable to install handler for fixed "
			    "power button: %d\n", sc->sc_dev.dv_xname, rv);
	}

	if (AcpiGbl_FADT != NULL && AcpiGbl_FADT->SleepButton == 0) {
		if (beenhere == 0)
			printf("%s: fixed-feature sleep button present\n",
			    sc->sc_dev.dv_xname);
		rv = AcpiInstallFixedEventHandler(ACPI_EVENT_SLEEP_BUTTON,
		    acpi_fixed_sleep_button_handler, sc);
		if (rv != AE_OK)
			printf("%s: unable to install handler for fixed "
			    "power button: %d\n", sc->sc_dev.dv_xname, rv);
	}

	beenhere = 1;
}

/*
 * acpi_fixed_power_button_handler:
 *
 *	Fixed event handler for the power button.
 */
UINT32
acpi_fixed_power_button_handler(void *context)
{
	struct acpi_softc *sc = context;

	/* XXX XXX XXX */

	printf("%s: fixed power button pressed\n", sc->sc_dev.dv_xname);

	return (ACPI_INTERRUPT_HANDLED);
}

/*
 * acpi_fixed_sleep_button_handler:
 *
 *	Fixed event handler for the sleep button.
 */
UINT32
acpi_fixed_sleep_button_handler(void *context)
{
	struct acpi_softc *sc = context;

	/* XXX XXX XXX */

	printf("%s: fixed sleep button pressed\n", sc->sc_dev.dv_xname);

	return (ACPI_INTERRUPT_HANDLED);
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
acpi_eval_integer(ACPI_HANDLE handle, char *path, int *valp)
{
	ACPI_STATUS rv;
	ACPI_BUFFER buf;
	ACPI_OBJECT param;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	buf.Pointer = &param;
	buf.Length = sizeof(param);

	rv = AcpiEvaluateObject(handle, path, NULL, &buf);
	if (rv == AE_OK) {
		if (param.Type == ACPI_TYPE_INTEGER)
			*valp = param.Integer.Value;
		else
			rv = AE_TYPE;
	}

	return (rv);
}

/*
 * acpi_eval_string:
 *
 *	Evaluate a (Unicode) string object.
 */
ACPI_STATUS
acpi_eval_string(ACPI_HANDLE handle, char *path, char **stringp)
{
	ACPI_STATUS rv;
	ACPI_BUFFER buf;
	ACPI_OBJECT *param;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_BUFFER;

	rv = AcpiEvaluateObject(handle, path, NULL, &buf);
	param = (ACPI_OBJECT *)buf.Pointer;
	if (rv == AE_OK && param) {
		if (param->Type == ACPI_TYPE_STRING) {
			char *ptr = param->String.Pointer;
			size_t len;
			while (*ptr++)
				continue;
			len = ptr - param->String.Pointer;
			if ((*stringp = AcpiOsAllocate(len)) == NULL) {
				rv = AE_NO_MEMORY;
				goto done;
			}
			(void)memcpy(*stringp, param->String.Pointer, len);
			goto done;
		}
		rv = AE_TYPE;
	}
done:
	if (buf.Pointer)
		AcpiOsFree(buf.Pointer);
	return (rv);
}


/*
 * acpi_eval_struct:
 *
 *	Evaluate a more complex structure.  Caller must free buf.Pointer.
 */
ACPI_STATUS
acpi_eval_struct(ACPI_HANDLE handle, char *path, ACPI_BUFFER *bufp)
{
	ACPI_STATUS rv;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	bufp->Pointer = NULL;
	bufp->Length = ACPI_ALLOCATE_BUFFER;

	rv = AcpiEvaluateObject(handle, path, NULL, bufp);

	return (rv);
}

/*
 * acpi_get:
 *
 *	Fetch data info the specified (empty) ACPI buffer.
 */
ACPI_STATUS
acpi_get(ACPI_HANDLE handle, ACPI_BUFFER *buf,
    ACPI_STATUS (*getit)(ACPI_HANDLE, ACPI_BUFFER *))
{
	ACPI_STATUS rv;

	buf->Pointer = NULL;
	buf->Length = 0;

	rv = (*getit)(handle, buf);
	if (rv != AE_BUFFER_OVERFLOW)
		return (rv);

	buf->Pointer = AcpiOsAllocate(buf->Length);
	if (buf->Pointer == NULL)
		return (AE_NO_MEMORY);
	memset(buf->Pointer, 0, buf->Length);

	return ((*getit)(handle, buf));
}


/*
 * acpi_acquire_global_lock:
 *
 *	Acquire global lock, with sleeping appropriately.
 */
#define ACQUIRE_WAIT 1000

ACPI_STATUS
acpi_acquire_global_lock(UINT32 *handle)
{
	ACPI_STATUS status;
	UINT32 h;
	int s;

	s = splhigh();
	simple_lock(&acpi_slock);
	while (acpi_locked)
		ltsleep(&acpi_slock, PVM, "acpi_lock", 0, &acpi_slock);
	acpi_locked = 1;
	simple_unlock(&acpi_slock);
	splx(s);
	status = AcpiAcquireGlobalLock(ACQUIRE_WAIT, &h);
	if (ACPI_SUCCESS(status))
		*handle = h;
	else {
		printf("acpi: cannot acquire lock. status=0x%X\n", status);
		s = splhigh();
		simple_lock(&acpi_slock);
		acpi_locked = 0;
		simple_unlock(&acpi_slock);
		splx(s);
	}

	return (status);
}

void
acpi_release_global_lock(UINT32 handle)
{
	ACPI_STATUS status;
	int s;

	if (!acpi_locked) {
		printf("acpi: not locked.\n");
		return;
	}

	status = AcpiReleaseGlobalLock(handle);
	if (ACPI_FAILURE(status)) {
		printf("acpi: AcpiReleaseGlobalLock failed. status=0x%X\n",
		       status);
		return;
	}
 
	s = splhigh();
	simple_lock(&acpi_slock);
	acpi_locked = 0;
	simple_unlock(&acpi_slock);
	splx(s);
	wakeup(&acpi_slock);
}

int
acpi_is_global_locked(void)
{
	return (acpi_locked);
}



/*****************************************************************************
 * ACPI sleep support.
 *****************************************************************************/

static int
is_available_state(struct acpi_softc *sc, int state)
{
	UINT8 type_a, type_b;

	return (ACPI_SUCCESS(AcpiGetSleepTypeData((UINT8)state,
						  &type_a, &type_b)));
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
			splx(s);
			dopowerhooks(PWR_SOFTRESUME);
			if (state==ACPI_STATE_S4)
				AcpiEnable();
		}
		AcpiLeaveSleepState((UINT8)state);
		break;
	case ACPI_STATE_S5:
		AcpiEnterSleepStatePrep(ACPI_STATE_S5);
		acpi_md_OsDisableInterrupt();
		AcpiEnterSleepState(ACPI_STATE_S5);
		printf("WARNING: powerdown failed!\n");
		break;
	}

	return (ret);
}

#if ACPI_PCI_FIXUP
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

#ifdef ACPI_DEBUG
	printf("acpi_pci_fixup starts:\n");
#endif
	if (AcpiGetHandle(ACPI_ROOT_OBJECT, "\\_SB_", &parent) != AE_OK)
		return;
	sc->sc_pci_bus = 0;
	AcpiWalkNamespace(ACPI_TYPE_DEVICE, parent, 100,
	    acpi_pci_fixup_bus, sc, NULL);
}

static ACPI_HANDLE
acpi_get_node(char *name)
{
	ACPI_NAMESPACE_NODE *ObjDesc;
	ACPI_STATUS Status;

	Status = AcpiNsGetNodeByPath(name, NULL, 0, &ObjDesc);
	if (ACPI_FAILURE (Status)) {
		printf("acpi_get_node: could not find: %s\n",
		       AcpiFormatException (Status));
		return NULL;
	}
	return ObjDesc;
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
		return (intr);
	for (res = ret.Pointer; res->Id != ACPI_RSTYPE_END_TAG;
	     res = ACPI_NEXT_RESOURCE(res)) {
		if (res->Id == ACPI_RSTYPE_IRQ) {
			irq = (ACPI_RESOURCE_IRQ *)&res->Data;
			if (irq->NumberOfInterrupts == 1)
				intr = irq->Interrupts[0];
			break;
		}
	}
	free(ret.Pointer, M_DEVBUF);
	return (intr);
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
	ACPI_NAMESPACE_NODE *node;
	ACPI_INTEGER val;

	rv = acpi_get(handle, &buf, AcpiGetIrqRoutingTable);
	if (ACPI_FAILURE(rv))
		return (AE_OK);

	/*
	 * If at level 1, this is a PCI root bus. Try the _BBN method
	 * to get the right PCI bus numbering for the following
	 * busses (this is a depth-first walk). It may fail,
	 * for example if there's only one root bus, but that
	 * case should be ok, so we'll ignore that.
	 */
	if (level == 1) {
		node = AcpiNsMapHandleToNode(handle);
		rv = AcpiUtEvaluateNumericObject(METHOD_NAME__BBN, node, &val);
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

		link = acpi_get_node(PrtElement->Source);
		if (link == NULL)
			continue;
		line = acpi_get_intr(link);
		if (line == -1) {
#ifdef ACPI_DEBUG
			printf("%s: fixing up intr link %s\n",
			    sc->sc_dev.dv_xname, PrtElement->Source);
#endif
			rv = acpi_allocate_resources(link);
			if (ACPI_FAILURE(rv)) {
				printf("%s: interrupt allocation failed %s\n",
				    sc->sc_dev.dv_xname, PrtElement->Source);
				continue;
			}
			line = acpi_get_intr(link);
			if (line == -1) {
				printf("%s: get intr failed %s\n",
				    sc->sc_dev.dv_xname, PrtElement->Source);
				continue;
			}
		}

		acpi_pci_set_line(sc->sc_pci_bus, PrtElement->Address >> 16,
		    PrtElement->Pin + 1, line);
	}

	sc->sc_pci_bus++;

	free(buf.Pointer, M_DEVBUF);
	return (AE_OK);
}
#endif /* ACPI_PCI_FIXUP */

#if ACPI_PCI_FIXUP || ACPI_ACTIVATE_DEV
/* XXX This very incomplete */
ACPI_STATUS
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
	bufn.Pointer = resn = malloc(bufn.Length, M_DEVBUF, M_WAITOK);
	resp = bufp.Pointer;
	resc = bufc.Pointer;
	while (resc->Id != ACPI_RSTYPE_END_TAG &&
	       resp->Id != ACPI_RSTYPE_END_TAG) {
		while (resc->Id != resp->Id && resp->Id != ACPI_RSTYPE_END_TAG)
			resp = ACPI_NEXT_RESOURCE(resp);
		if (resp->Id == ACPI_RSTYPE_END_TAG)
			break;
		/* Found identical Id */
		resn->Id = resc->Id;
		switch (resc->Id) {
		case ACPI_RSTYPE_IRQ:
			memcpy(&resn->Data, &resp->Data,
			       sizeof(ACPI_RESOURCE_IRQ));
			irq = (ACPI_RESOURCE_IRQ *)&resn->Data;
			irq->Interrupts[0] =
			    ((ACPI_RESOURCE_IRQ *)&resp->Data)->
			        Interrupts[irq->NumberOfInterrupts-1];
			irq->NumberOfInterrupts = 1;
			resn->Length = ACPI_SIZEOF_RESOURCE(ACPI_RESOURCE_IRQ);
			break;
		case ACPI_RSTYPE_IO:
			memcpy(&resn->Data, &resp->Data,
			       sizeof(ACPI_RESOURCE_IO));
			resn->Length = resp->Length;
			break;
		default:
			printf("acpi_allocate_resources: res=%d\n", resc->Id);
			rv = AE_BAD_DATA;
			goto out2;
		}
		resc = ACPI_NEXT_RESOURCE(resc);
		resn = ACPI_NEXT_RESOURCE(resn);
		delta = (UINT8 *)resn - (UINT8 *)bufn.Pointer;
		if (delta >= 
		    bufn.Length-ACPI_SIZEOF_RESOURCE(ACPI_RESOURCE_DATA)) {
			bufn.Length *= 2;
			bufn.Pointer = realloc(bufn.Pointer, bufn.Length,
					       M_DEVBUF, M_WAITOK);
			resn = (ACPI_RESOURCE *)((UINT8 *)bufn.Pointer + delta);
		}
	}
	if (resc->Id != ACPI_RSTYPE_END_TAG) {
		printf("acpi_allocate_resources: resc not exhausted\n");
		rv = AE_BAD_DATA;
		goto out3;
	}

	resn->Id = ACPI_RSTYPE_END_TAG;
	rv = AcpiSetCurrentResources(handle, &bufn);
	if (ACPI_FAILURE(rv)) {
		printf("acpi_allocate_resources: AcpiSetCurrentResources %s\n",
		       AcpiFormatException(rv));
	}

out3:
	free(bufn.Pointer, M_DEVBUF);
out2:
	free(bufc.Pointer, M_DEVBUF);
out1:
	free(bufp.Pointer, M_DEVBUF);
out:
	return rv;
}
#endif /* ACPI_PCI_FIXUP || ACPI_ACTIVATE_DEV */
