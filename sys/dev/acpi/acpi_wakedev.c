/* $NetBSD: acpi_wakedev.c,v 1.7 2010/03/16 05:48:43 jruoho Exp $ */

/*-
 * Copyright (c) 2009 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: acpi_wakedev.c,v 1.7 2010/03/16 05:48:43 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_wakedev.h>

#define _COMPONENT		   ACPI_BUS_COMPONENT
ACPI_MODULE_NAME		   ("acpi_wakedev")

static int acpi_wakedev_node = -1;

static const char * const acpi_wakedev_default[] = {
	"PNP0C0C",	/* power button */
	"PNP0C0E",	/* sleep button */
	"PNP0C0D",	/* lid switch */
	"PNP03??",	/* PC KBD port */
	NULL,
};

static void	acpi_wakedev_prepare(struct acpi_devnode *, int, int);

SYSCTL_SETUP(sysctl_acpi_wakedev_setup, "sysctl hw.wake subtree setup")
{
	const struct sysctlnode *rnode;
	int err;

	err = sysctl_createv(NULL, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_EOL);
	if (err)
		return;
	err = sysctl_createv(NULL, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "wake", NULL,
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);
	if (err)
		return;
	acpi_wakedev_node = rnode->sysctl_num;
}

void
acpi_wakedev_add(struct acpi_devnode *ad)
{
	int err;

	KASSERT(ad != NULL && ad->ad_parent != NULL);
	KASSERT((ad->ad_flags & ACPI_DEVICE_WAKEUP) != 0);

	ad->ad_wake.wake_enabled = 0;
	ad->ad_wake.wake_sysctllog = NULL;

	if (acpi_match_hid(ad->ad_devinfo, acpi_wakedev_default))
		ad->ad_wake.wake_enabled = 1;

	if (acpi_wakedev_node == -1)
		return;

	err = sysctl_createv(&ad->ad_wake.wake_sysctllog, 0, NULL, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, ad->ad_name,
	    NULL, NULL, 0, &ad->ad_wake.wake_enabled, 0,
	    CTL_HW, acpi_wakedev_node, CTL_CREATE, CTL_EOL);

	if (err != 0)
		aprint_error_dev(ad->ad_parent, "sysctl_createv(hw.wake.%s) "
		    "failed (err %d)\n", ad->ad_name, err);
}

void
acpi_wakedev_commit(struct acpi_softc *sc, int state)
{
	struct acpi_devnode *ad;

	/*
	 * As noted in ACPI 3.0 (p. 243), preparing
	 * a device for wakeup is a two-step process:
	 *
	 *  1.	Enable all power resources in _PRW.
	 *
	 *  2.	If present, execute _DSW/_PSW method.
	 *
	 * XXX: The first one is yet to be implemented.
	 */
	SIMPLEQ_FOREACH(ad, &sc->sc_devnodes, ad_list) {

		if ((ad->ad_flags & ACPI_DEVICE_WAKEUP) == 0)
			continue;

		if (ad->ad_wake.wake_enabled == 0)
			acpi_clear_wake_gpe(ad->ad_handle);
		else {
			aprint_debug_dev(ad->ad_parent,
			    "set wake GPE for %s\n", ad->ad_name);
			acpi_set_wake_gpe(ad->ad_handle);
		}

		acpi_wakedev_prepare(ad, ad->ad_wake.wake_enabled, state);
	}
}

static void
acpi_wakedev_prepare(struct acpi_devnode *ad, int enable, int state)
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
	obj[2].Integer.Value = 3;

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
	aprint_error_dev(ad->ad_parent, "failed to evaluate wake "
	    "control method: %s\n", AcpiFormatException(rv));
}
