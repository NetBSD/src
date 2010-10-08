/* $NetBSD: acpi_wakedev.c,v 1.18 2010/10/08 07:04:31 gsutre Exp $ */

/*-
 * Copyright (c) 2009, 2010 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_wakedev.c,v 1.18 2010/10/08 07:04:31 gsutre Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_power.h>
#include <dev/acpi/acpi_wakedev.h>

#define _COMPONENT		ACPI_BUS_COMPONENT
ACPI_MODULE_NAME		("acpi_wakedev")

static const char * const acpi_wakedev_default[] = {
	"PNP0C0C",	/* power button */
	"PNP0C0E",	/* sleep button */
	"PNP0C0D",	/* lid switch */
	"PNP03??",	/* PC KBD port */
	NULL,
};

static int32_t acpi_wakedev_acpinode = CTL_EOL;
static int32_t acpi_wakedev_wakenode = CTL_EOL;

static void	acpi_wakedev_method(struct acpi_devnode *, int, int);
static void	acpi_wakedev_gpe(struct acpi_devnode *, int, int);
static void	acpi_wakedev_power(struct acpi_devnode *, ACPI_OBJECT *);

SYSCTL_SETUP(sysctl_acpi_wakedev_setup, "sysctl hw.acpi.wake subtree setup")
{
	const struct sysctlnode *rnode;
	int err;

	err = sysctl_createv(NULL, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw",
	    NULL, NULL, 0, NULL, 0,
	    CTL_HW, CTL_EOL);

	if (err != 0)
		return;

	err = sysctl_createv(NULL, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "acpi",
	    NULL, NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	if (err != 0)
		return;

	acpi_wakedev_acpinode = rnode->sysctl_num;

	err = sysctl_createv(NULL, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE,
	    "wake", SYSCTL_DESCR("ACPI device wake-up"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	if (err != 0)
		return;

	acpi_wakedev_wakenode = rnode->sysctl_num;
}

void
acpi_wakedev_add(struct acpi_devnode *ad)
{
	int err;

	KASSERT(ad != NULL && ad->ad_root != NULL);
	KASSERT((ad->ad_flags & ACPI_DEVICE_WAKEUP) != 0);

	ad->ad_wake = 0;

	if (acpi_match_hid(ad->ad_devinfo, acpi_wakedev_default))
		ad->ad_wake = 1;

	if (acpi_wakedev_acpinode == CTL_EOL ||
	    acpi_wakedev_wakenode == CTL_EOL)
		return;

	err = sysctl_createv(NULL, 0, NULL, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, ad->ad_name,
	    NULL, NULL, 0, &ad->ad_wake, 0,
	    CTL_HW, acpi_wakedev_acpinode, acpi_wakedev_wakenode,
	    CTL_CREATE, CTL_EOL);

	if (err != 0)
		aprint_error_dev(ad->ad_root, "sysctl_createv"
		    "(hw.acpi.wake.%s) failed (err %d)\n", ad->ad_name, err);
}

void
acpi_wakedev_commit(struct acpi_softc *sc, int state)
{
	struct acpi_devnode *ad;

	/*
	 * To prepare a device for wakeup:
	 *
	 *  1.	Set appropriate GPEs.
	 *
	 *  2.	Enable all power resources in _PRW.
	 *
	 *  3.	If present, execute _DSW/_PSW method.
	 */
	SIMPLEQ_FOREACH(ad, &sc->ad_head, ad_list) {

		if ((ad->ad_flags & ACPI_DEVICE_WAKEUP) == 0)
			continue;

		acpi_wakedev_gpe(ad, ad->ad_wake, state);
		acpi_wakedev_method(ad, ad->ad_wake, state);
	}
}

static void
acpi_wakedev_method(struct acpi_devnode *ad, int enable, int state)
{
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT obj[3];
	ACPI_STATUS rv;

	/*
	 * First try to call the Device Sleep Wake control method, _DSW.
	 * Only if this is not available, resort to to the Power State
	 * Wake control method, _PSW, which was deprecated in ACPI 3.0.
	 *
	 * The arguments to these methods are as follows:
	 *
	 *		arg0		arg1		arg2
	 *		----		----		----
	 *	 _PSW	0: disable
	 *		1: enable
	 *
	 *	 _DSW	0: disable	0: S0		0: D0
	 *		1: enable	1: S1		1: D0 or D1
	 *						2: D0, D1, or D2
	 *				x: Sx		3: D0, D1, D2 or D3
	 */
	arg.Count = 3;
	arg.Pointer = obj;

	obj[0].Integer.Value = enable;
	obj[1].Integer.Value = state;
	obj[2].Integer.Value = ACPI_STATE_D0;

	obj[0].Type = obj[1].Type = obj[2].Type = ACPI_TYPE_INTEGER;

	rv = AcpiEvaluateObject(ad->ad_handle, "_DSW", &arg, NULL);

	if (ACPI_SUCCESS(rv))
		return;

	if (rv != AE_NOT_FOUND)
		goto fail;

	rv = acpi_eval_set_integer(ad->ad_handle, "_PSW", enable);

	if (ACPI_FAILURE(rv) && rv != AE_NOT_FOUND)
		goto fail;

	return;

fail:
	aprint_error_dev(ad->ad_root, "failed to evaluate wake "
	    "control method: %s\n", AcpiFormatException(rv));
}

static void
acpi_wakedev_gpe(struct acpi_devnode *ad, int enable, int state)
{
	ACPI_OBJECT *elm, *obj;
	ACPI_HANDLE hdl = NULL;
	ACPI_INTEGER val;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	rv = acpi_eval_struct(ad->ad_handle, "_PRW", &buf);

	if (ACPI_FAILURE(rv))
		return;

	obj = buf.Pointer;

	if (obj->Type != ACPI_TYPE_PACKAGE || obj->Package.Count < 2)
		goto out;

	/*
	 * As noted in ACPI 3.0 (section 7.2.10), the _PRW object is
	 * a package in which the first element is either an integer
	 * or again a package. In the latter case the package inside
	 * the package element has two elements, a reference handle
	 * and the GPE number.
	 */
	elm = &obj->Package.Elements[0];

	switch (elm->Type) {

	case ACPI_TYPE_INTEGER:
		val = elm->Integer.Value;
		break;

	case ACPI_TYPE_PACKAGE:

		if (elm->Package.Count < 2)
			goto out;

		if (elm->Package.Elements[0].Type != ACPI_TYPE_LOCAL_REFERENCE)
			goto out;

		if (elm->Package.Elements[1].Type != ACPI_TYPE_INTEGER)
			goto out;

		hdl = elm->Package.Elements[0].Reference.Handle;
		val = elm->Package.Elements[1].Integer.Value;
		break;

	default:
		goto out;
	}

 	/*
	 * The second element is an integer that contains the
	 * lowest sleep state that can be entered while still
	 * providing wake-up functionality. The rest of the
	 * elements are references to power resources.
	 */
	elm = &obj->Package.Elements[1];

	if (elm->Type != ACPI_TYPE_INTEGER)
		goto out;

	if (state > elm->Integer.Value)
		aprint_error_dev(ad->ad_root, "sleep state S%d "
		    "loses wake for %s\n", state, ad->ad_name);

	/*
	 * Turn on power resources.
	 */
	if (enable != 0)
		acpi_wakedev_power(ad, obj);

	/*
	 * Set both runtime and wake GPEs, but unset only wake GPEs.
	 */
	if (enable != 0)
		(void)AcpiEnableGpe(hdl, val, ACPI_GPE_TYPE_WAKE_RUN);
	else
		(void)AcpiDisableGpe(hdl, val, ACPI_GPE_TYPE_WAKE);

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "wake GPE %s for %s\n",
		(enable != 0) ? "enabled" : "disabled", ad->ad_name));

out:
	ACPI_FREE(buf.Pointer);
}

static void
acpi_wakedev_power(struct acpi_devnode *ad, ACPI_OBJECT *obj)
{
	ACPI_OBJECT *elm;
	ACPI_HANDLE hdl;
	ACPI_STATUS rv;
	uint32_t i, n;

	n = obj->Package.Count;

	if (n < 3)
		return;

	for (i = 2; i < n; i++) {

		elm = &obj->Package.Elements[i];
		rv = acpi_eval_reference_handle(elm, &hdl);

		if (ACPI_FAILURE(rv))
			continue;

		(void)acpi_power_res(hdl, ad->ad_handle, true);
	}
}
