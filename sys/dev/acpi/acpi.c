/*	$NetBSD: acpi.c,v 1.5.2.4 2002/06/23 17:45:02 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: acpi.c,v 1.5.2.4 2002/06/23 17:45:02 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_osd.h>

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

struct cfattach acpi_ca = {
	sizeof(struct acpi_softc), acpi_match, acpi_attach,
};

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

void		acpi_shutdown(void *);
ACPI_STATUS	acpi_disable(struct acpi_softc *sc);
void		acpi_build_tree(struct acpi_softc *);
ACPI_STATUS	acpi_make_devnode(ACPI_HANDLE, UINT32, void *, void **);

void		acpi_enable_fixed_events(struct acpi_softc *);

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
	ACPI_STATUS rv;

	printf("\n");

	if (acpi_softc != NULL)
		panic("acpi_attach: ACPI has already been attached");

	sc->sc_iot = aa->aa_iot;
	sc->sc_memt = aa->aa_memt;
	sc->sc_pc = aa->aa_pc;
	sc->sc_pciflags = aa->aa_pciflags;

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

	/*
	 * Check for fixed-hardware features.
	 */
	acpi_enable_fixed_events(sc);

	/*
	 * Scan the namespace and build our device tree.
	 */
#ifdef ENABLE_DEBUGGER
	if (acpi_dbgr & ACPI_DBGR_PROBE)
		acpi_osd_debugger();
#endif
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

			/*
			 * XXX We only attach devices which are:
			 *
			 *	- present
			 *	- enabled
			 *	- to be shown
			 *	- functioning properly
			 *
			 * However, if enabled, it's decoding resources,
			 * so we should claim them, if possible.  Requires
			 * changes to bus_space(9).
			 */
			if ((ad->ad_devinfo.CurrentStatus &
			     (ACPI_STA_DEV_PRESENT|ACPI_STA_DEV_ENABLED|
			      ACPI_STA_DEV_SHOW|ACPI_STA_DEV_OK)) !=
			    (ACPI_STA_DEV_PRESENT|ACPI_STA_DEV_ENABLED|
			     ACPI_STA_DEV_SHOW|ACPI_STA_DEV_OK))
				continue;

			/*
			 * XXX Same problem as above...
			 */
			if ((ad->ad_devinfo.Valid & ACPI_VALID_HID) == 0)
				continue;

			ad->ad_device = config_found(&sc->sc_dev,
			    &aa, acpi_print);
		}
	}
}

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
#ifdef ACPI_DEBUG
	struct acpi_softc *sc = state->softc;
#endif
	struct acpi_scope *as = state->scope;
	struct acpi_devnode *ad;
	ACPI_OBJECT_TYPE type;
	ACPI_STATUS rv;

	if (AcpiGetType(handle, &type) == AE_OK) {
		switch (type) {
		case ACPI_TYPE_DEVICE:
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

#ifdef ACPI_DEBUG
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
#if 0
	char *str;
#endif

	if (pnp) {
		printf("%s ", aa->aa_node->ad_devinfo.HardwareId);
#if 0 /* Not until we fix acpi_eval_string */
		if (acpi_eval_string(aa->aa_node->ad_handle,
		    "_STR", &str) == AE_OK) {
			printf("[%s] ", str);
			AcpiOsFree(str);
		}
#endif
		printf("at %s", pnp);
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

#if 0
/*
 * acpi_eval_string:
 *
 *	Evaluate a (Unicode) string object.
 * XXX current API may leak memory, so don't use this.
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
	buf.Length = 0;

	rv = AcpiEvaluateObject(handle, path, NULL, &buf);
	if (rv != AE_BUFFER_OVERFLOW)
		return (rv);

	buf.Pointer = AcpiOsAllocate(buf.Length);
	if (buf.Pointer == NULL)
		return (AE_NO_MEMORY);

	rv = AcpiEvaluateObject(handle, path, NULL, &buf);
	param = (ACPI_OBJECT *)buf.Pointer;
	if (rv == AE_OK) {
		if (param->Type == ACPI_TYPE_STRING) {
			/* XXX may leak buf.Pointer!! */
			*stringp = param->String.Pointer;
			return (AE_OK);
		}
		rv = AE_TYPE;
	}

	AcpiOsFree(buf.Pointer);
	return (rv);
}
#endif


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
	bufp->Length = 0;

	rv = AcpiEvaluateObject(handle, path, NULL, bufp);
	if (rv != AE_BUFFER_OVERFLOW)
		return (rv);

	bufp->Pointer = AcpiOsAllocate(bufp->Length);
	if (bufp->Pointer == NULL)
		return (AE_NO_MEMORY);

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
