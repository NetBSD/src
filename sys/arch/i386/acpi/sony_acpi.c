/*	$NetBSD: sony_acpi.c,v 1.2.6.1 2006/04/22 11:37:31 simonb Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sony_acpi.c,v 1.2.6.1 2006/04/22 11:37:31 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/sysctl.h>

#include <machine/bus.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>

struct sony_acpi_softc {
        struct device sc_dev;
	struct sysctllog *sc_log;
	struct acpi_devnode *sc_node;
};

static const char * const sony_acpi_ids[] = {
	"SNY5001",
	NULL
};

static int	sony_acpi_match(struct device *, struct cfdata *, void *);
static void	sony_acpi_attach(struct device *, struct device *, void *);
static ACPI_STATUS sony_acpi_eval_set_integer(ACPI_HANDLE, const char *,
    ACPI_INTEGER, ACPI_INTEGER *);


CFATTACH_DECL(sony_acpi, sizeof(struct sony_acpi_softc),
    sony_acpi_match, sony_acpi_attach, NULL, NULL);

static int
sony_acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, sony_acpi_ids);
}

static int
sony_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	ACPI_INTEGER acpi_val;
	ACPI_STATUS rv;
	int val, old_val, error;
	char buf[SYSCTL_NAMELEN + 1], *ptr;
	struct sony_acpi_softc *sc = rnode->sysctl_data;

	(void)snprintf(buf, sizeof(buf), "G%s", rnode->sysctl_name);
	for (ptr = buf; *ptr; ptr++)
		*ptr = toupper(*ptr);

	rv = acpi_eval_integer(sc->sc_node->ad_handle, buf, &acpi_val);
	if (ACPI_FAILURE(rv)) {
#ifdef DIAGNOSTIC
		printf("%s: couldn't get `%s'\n", sc->sc_dev.dv_xname, buf);
#endif
		return EIO;
	}
	val = old_val = acpi_val;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	(void)snprintf(buf, sizeof(buf), "S%s", rnode->sysctl_name);
	acpi_val = val;
	rv = sony_acpi_eval_set_integer(sc->sc_node->ad_handle, buf,
	    acpi_val, NULL);
	if (ACPI_FAILURE(rv)) {
#ifdef DIAGNOSTIC
		printf("%s: couldn't set `%s' to %d\n",
		    sc->sc_dev.dv_xname, buf, val);
#endif
		return EIO;
	}
	return 0;
}

static ACPI_STATUS
sony_walk_cb(ACPI_HANDLE hnd, UINT32 v, void *context, void **status)
{
	struct sony_acpi_softc *sc = (void*)context;
	const struct sysctlnode *node, *snode;
	const char *name = acpi_name(hnd);
	ACPI_INTEGER acpi_val;
	char buf[SYSCTL_NAMELEN + 1], *ptr;
	int rv;

	if ((name = strrchr(name, '.')) == NULL)
		return AE_OK;

	name++;
	if ((*name != 'G') && (*name != 'S'))
		return AE_OK;

	(void)strlcpy(buf, name, sizeof(buf));
	*buf = 'G';

	/*
	 * We assume that if the 'get' of the name as an integer is
	 * successful it is ok.
	 */
	if (acpi_eval_integer(sc->sc_node->ad_handle, buf, &acpi_val))
		return AE_OK;

	for (ptr = buf; *ptr; ptr++)
		*ptr = tolower(*ptr);

	if ((rv = sysctl_createv(&sc->sc_log, 0, NULL, &node, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "hw", NULL, NULL, 0, NULL, 0, CTL_HW, CTL_EOL)) != 0)
		goto out;

	if ((rv = sysctl_createv(&sc->sc_log, 0, &node, &snode, 0,
	    CTLTYPE_NODE, sc->sc_dev.dv_xname, SYSCTL_DESCR("sony controls"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto out;

	if ((rv = sysctl_createv(&sc->sc_log, 0, &snode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, buf + 1, NULL,
	    sony_sysctl_helper, 0, sc, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto out;

out:
#ifdef DIAGNOSTIC
	if (rv)
		printf("%s: sysctl_createv failed (rv = %d)\n",
		    sc->sc_dev.dv_xname, rv);
#endif
	return AE_OK;
}

ACPI_STATUS
sony_acpi_eval_set_integer(ACPI_HANDLE handle, const char *path,
    ACPI_INTEGER val, ACPI_INTEGER *valp)
{
	ACPI_STATUS rv;
	ACPI_BUFFER buf;
	ACPI_OBJECT param, ret_val;
	ACPI_OBJECT_LIST params;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	params.Count = 1;
	params.Pointer = &param;

	param.Type = ACPI_TYPE_INTEGER;
	param.Integer.Value = val;

	buf.Pointer = &ret_val;
	buf.Length = sizeof(ret_val);

	rv = AcpiEvaluateObjectTyped(handle, path, &params, &buf,
	    ACPI_TYPE_INTEGER);

	if (ACPI_SUCCESS(rv) && valp)
		*valp = ret_val.Integer.Value;

	return rv;
}

static void
sony_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct sony_acpi_softc *sc = (void *)self;
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;

	aprint_naive(": Sony Miscellaneous Controller\n");
	aprint_normal(": Sony Miscellaneous Controller\n");

	sc->sc_node = aa->aa_node;

	/* Install sysctl handler */
	rv = AcpiWalkNamespace(ACPI_TYPE_METHOD,
	    sc->sc_node->ad_handle, 1, sony_walk_cb, sc, NULL);
#ifdef DIAGNOSTIC
	if (ACPI_FAILURE(rv))
		aprint_error("%s: Cannot walk ACPI namespace (%d)\n",
		    sc->sc_dev.dv_xname, rv);
#endif
}
