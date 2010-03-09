/* $NetBSD: acpi_wakedev.c,v 1.6 2010/03/09 18:15:22 jruoho Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: acpi_wakedev.c,v 1.6 2010/03/09 18:15:22 jruoho Exp $");

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

struct acpi_wakedev {
	struct acpi_devnode	  *aw_node;
	struct sysctllog	  *aw_sysctllog;
	int			   aw_enabled;

	TAILQ_ENTRY(acpi_wakedev)  aw_list;
};

struct acpi_wakedev;
static int acpi_wakedev_node = -1;

static TAILQ_HEAD(, acpi_wakedev) acpi_wakedevlist =
    TAILQ_HEAD_INITIALIZER(acpi_wakedevlist);

static const char * const acpi_wakedev_default[] = {
	"PNP0C0C",	/* power button */
	"PNP0C0E",	/* sleep button */
	"PNP0C0D",	/* lid switch */
	"PNP03??",	/* PC KBD port */
	NULL,
};

static void	acpi_wakedev_sysctl_add(struct acpi_wakedev *);
static bool	acpi_wakedev_add(struct acpi_softc *, struct acpi_devnode *);
static void	acpi_wakedev_print(struct acpi_wakedev *);
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

static void
acpi_wakedev_sysctl_add(struct acpi_wakedev *aw)
{
	int err;

	if (acpi_wakedev_node == -1)
		return;

	err = sysctl_createv(&aw->aw_sysctllog, 0, NULL, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, aw->aw_node->ad_name,
	    NULL, NULL, 0, &aw->aw_enabled, 0,
	    CTL_HW, acpi_wakedev_node, CTL_CREATE, CTL_EOL);
	if (err)
		aprint_error("%s: sysctl_createv(hw.wake.%s) failed (%d)\n",
		    __func__, aw->aw_node->ad_name, err);
}

static bool
acpi_wakedev_add(struct acpi_softc *sc, struct acpi_devnode *ad)
{
	struct acpi_wakedev *aw;
	ACPI_HANDLE hdl;

	if (ACPI_FAILURE(AcpiGetHandle(ad->ad_handle, "_PRW", &hdl)))
		return false;

	aw = kmem_alloc(sizeof(*aw), KM_SLEEP);
	if (aw == NULL) {
		aprint_error("%s: kmem_alloc failed\n", __func__);
		return false;
	}
	aw->aw_node = ad;
	aw->aw_sysctllog = NULL;
	if (acpi_match_hid(ad->ad_devinfo, acpi_wakedev_default))
		aw->aw_enabled = 1;
	else
		aw->aw_enabled = 0;

	TAILQ_INSERT_TAIL(&acpi_wakedevlist, aw, aw_list);

	acpi_wakedev_sysctl_add(aw);

	return true;
}

static void
acpi_wakedev_print(struct acpi_wakedev *aw)
{
	aprint_debug(" %s", aw->aw_node->ad_name);
}

int
acpi_wakedev_scan(struct acpi_softc *sc)
{
	struct acpi_devnode *ad;
	struct acpi_wakedev *aw;
	ACPI_DEVICE_INFO *di;
	int count = 0;

#define ACPI_STA_DEV_VALID	\
	(ACPI_STA_DEV_PRESENT|ACPI_STA_DEV_ENABLED|ACPI_STA_DEV_OK)

	SIMPLEQ_FOREACH(ad, &sc->sc_devnodes, ad_list) {

		di = ad->ad_devinfo;

		if (di->Type != ACPI_TYPE_DEVICE)
			continue;

		if ((di->Valid & ACPI_VALID_STA) != 0 &&
		    (di->CurrentStatus & ACPI_STA_DEV_VALID) !=
		     ACPI_STA_DEV_VALID)
			continue;

		if (acpi_wakedev_add(sc, ad) == true)
			++count;
	}

#undef ACPI_STA_DEV_VALID

	if (count == 0)
		return 0;

	aprint_debug_dev(sc->sc_dev, "wakeup devices:");
	TAILQ_FOREACH(aw, &acpi_wakedevlist, aw_list)
		acpi_wakedev_print(aw);
	aprint_debug("\n");

	return count;
}

void
acpi_wakedev_commit(struct acpi_softc *sc, int state)
{
	struct acpi_wakedev *aw;

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
	TAILQ_FOREACH(aw, &acpi_wakedevlist, aw_list) {

		if (aw->aw_enabled) {
			aprint_debug_dev(sc->sc_dev, "set wake GPE (%s)\n",
			    aw->aw_node->ad_name);
			acpi_set_wake_gpe(aw->aw_node->ad_handle);
		} else
			acpi_clear_wake_gpe(aw->aw_node->ad_handle);

		acpi_wakedev_prepare(aw->aw_node, aw->aw_enabled, state);
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
	aprint_error_dev(ad->ad_device, "failed to evaluate wake "
	    "control method: %s\n", AcpiFormatException(rv));
}
