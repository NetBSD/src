/* $NetBSD: acpi_cpu.c,v 1.23 2010/10/28 04:28:29 jruoho Exp $ */

/*-
 * Copyright (c) 2010 Jukka Ruohonen <jruohonen@iki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_cpu.c,v 1.23 2010/10/28 04:28:29 jruoho Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/once.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_cpu.h>

#include <machine/acpi_machdep.h>

#define _COMPONENT	  ACPI_BUS_COMPONENT
ACPI_MODULE_NAME	  ("acpi_cpu")

static int		  acpicpu_match(device_t, cfdata_t, void *);
static void		  acpicpu_attach(device_t, device_t, void *);
static int		  acpicpu_detach(device_t, int);
static int		  acpicpu_once_attach(void);
static int		  acpicpu_once_detach(void);
static void		  acpicpu_prestart(device_t);
static void		  acpicpu_start(device_t);

static int		  acpicpu_object(ACPI_HANDLE, struct acpicpu_object *);
static cpuid_t		  acpicpu_id(uint32_t);
static uint32_t		  acpicpu_cap(struct acpicpu_softc *);
static ACPI_STATUS	  acpicpu_cap_pdc(struct acpicpu_softc *, uint32_t);
static ACPI_STATUS	  acpicpu_cap_osc(struct acpicpu_softc *,
					  uint32_t, uint32_t *);
static void		  acpicpu_notify(ACPI_HANDLE, uint32_t, void *);
static bool		  acpicpu_suspend(device_t, const pmf_qual_t *);
static bool		  acpicpu_resume(device_t, const pmf_qual_t *);

struct acpicpu_softc	**acpicpu_sc = NULL;

static const char * const acpicpu_hid[] = {
	"ACPI0007",
	NULL
};

CFATTACH_DECL_NEW(acpicpu, sizeof(struct acpicpu_softc),
    acpicpu_match, acpicpu_attach, acpicpu_detach, NULL);

static int
acpicpu_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;
	struct acpicpu_object ao;
	int rv;

	if (aa->aa_node->ad_type != ACPI_TYPE_PROCESSOR)
		return 0;

	if (acpi_match_hid(aa->aa_node->ad_devinfo, acpicpu_hid) != 0)
		return 1;

	rv = acpicpu_object(aa->aa_node->ad_handle, &ao);

	if (rv != 0 || acpicpu_id(ao.ao_procid) == 0xFFFFFF)
		return 0;

	return 1;
}

static void
acpicpu_attach(device_t parent, device_t self, void *aux)
{
	struct acpicpu_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	static ONCE_DECL(once_attach);
	int rv;

	rv = acpicpu_object(aa->aa_node->ad_handle, &sc->sc_object);

	if (rv != 0)
		return;

	rv = RUN_ONCE(&once_attach, acpicpu_once_attach);

	if (rv != 0)
		return;

	sc->sc_dev = self;
	sc->sc_cold = true;
	sc->sc_passive = false;
	sc->sc_node = aa->aa_node;
	sc->sc_cpuid = acpicpu_id(sc->sc_object.ao_procid);

	if (sc->sc_cpuid == 0xFFFFFF) {
		aprint_error(": invalid CPU ID\n");
		return;
	}

	if (acpicpu_sc[sc->sc_cpuid] != NULL) {
		aprint_error(": already attached\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": ACPI CPU\n");

	acpicpu_sc[sc->sc_cpuid] = sc;

	sc->sc_cap = acpicpu_cap(sc);
	sc->sc_flags |= acpicpu_md_quirks();

	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_NONE);

	acpicpu_cstate_attach(self);
	acpicpu_pstate_attach(self);
	acpicpu_tstate_attach(self);

	(void)config_defer(self, acpicpu_prestart);
	(void)acpi_register_notify(sc->sc_node, acpicpu_notify);
	(void)pmf_device_register(self, acpicpu_suspend, acpicpu_resume);
}

static int
acpicpu_detach(device_t self, int flags)
{
	struct acpicpu_softc *sc = device_private(self);
	static ONCE_DECL(once_detach);
	int rv = 0;

	sc->sc_cold = true;
	acpi_deregister_notify(sc->sc_node);

	if ((sc->sc_flags & ACPICPU_FLAG_C) != 0)
		rv = acpicpu_cstate_detach(self);

	if (rv != 0)
		return rv;

	if ((sc->sc_flags & ACPICPU_FLAG_P) != 0)
		rv = acpicpu_pstate_detach(self);

	if (rv != 0)
		return rv;

	if ((sc->sc_flags & ACPICPU_FLAG_T) != 0)
		rv = acpicpu_tstate_detach(self);

	if (rv != 0)
		return rv;

	rv = RUN_ONCE(&once_detach, acpicpu_once_detach);

	if (rv != 0)
		return rv;

	mutex_destroy(&sc->sc_mtx);

	return 0;
}

static int
acpicpu_once_attach(void)
{
	struct acpicpu_softc *sc;
	unsigned int i;

	acpicpu_sc = kmem_zalloc(maxcpus * sizeof(*sc), KM_SLEEP);

	if (acpicpu_sc == NULL)
		return ENOMEM;

	for (i = 0; i < maxcpus; i++)
		acpicpu_sc[i] = NULL;

	return 0;
}

static int
acpicpu_once_detach(void)
{
	struct acpicpu_softc *sc;

	if (acpicpu_sc != NULL)
		kmem_free(acpicpu_sc, maxcpus * sizeof(*sc));

	return 0;
}

static void
acpicpu_prestart(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);
	static bool once = false;

	if (once != false) {
		sc->sc_cold = false;
		return;
	}

	once = true;

	(void)config_interrupts(self, acpicpu_start);
}

static void
acpicpu_start(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);

	/*
	 * Run the state-specific initialization
	 * routines. These should be called only
	 * once, after interrupts are enabled and
	 * all ACPI CPUs have attached.
	 */
	if ((sc->sc_flags & ACPICPU_FLAG_C) != 0)
		acpicpu_cstate_start(self);

	if ((sc->sc_flags & ACPICPU_FLAG_P) != 0)
		acpicpu_pstate_start(self);

	if ((sc->sc_flags & ACPICPU_FLAG_T) != 0)
		acpicpu_tstate_start(self);

	aprint_debug_dev(sc->sc_dev, "ACPI CPUs started (cap "
	    "0x%02x, flags 0x%06x)\n", sc->sc_cap, sc->sc_flags);

	sc->sc_cold = false;
}

static int
acpicpu_object(ACPI_HANDLE hdl, struct acpicpu_object *ao)
{
	ACPI_OBJECT *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	rv = acpi_eval_struct(hdl, NULL, &buf);

	if (ACPI_FAILURE(rv))
		return 1;

	obj = buf.Pointer;

	if (obj->Type != ACPI_TYPE_PROCESSOR) {
		rv = AE_TYPE;
		goto out;
	}

	if (obj->Processor.ProcId > (uint32_t)maxcpus) {
		rv = AE_LIMIT;
		goto out;
	}

	KDASSERT((uint64_t)obj->Processor.PblkAddress < UINT32_MAX);

	if (ao != NULL) {
		ao->ao_procid = obj->Processor.ProcId;
		ao->ao_pblklen = obj->Processor.PblkLength;
		ao->ao_pblkaddr = obj->Processor.PblkAddress;
	}

out:
	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	return ACPI_FAILURE(rv) ? 1 : 0;
}

static cpuid_t
acpicpu_id(uint32_t id)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	for (CPU_INFO_FOREACH(cii, ci)) {

		if (id == ci->ci_acpiid)
			return id;
	}

	return 0xFFFFFF;
}

static uint32_t
acpicpu_cap(struct acpicpu_softc *sc)
{
	uint32_t flags, cap = 0;
	const char *str;
	ACPI_STATUS rv;

	/*
	 * Query and set machine-dependent capabilities.
	 * Note that the Intel-specific _PDC method was
	 * deprecated in the ACPI 3.0 in favor of _OSC.
	 */
	flags = acpicpu_md_cap();
	rv = acpicpu_cap_osc(sc, flags, &cap);

	if (ACPI_FAILURE(rv) && rv != AE_NOT_FOUND) {
		str = "_OSC";
		goto fail;
	}

	rv = acpicpu_cap_pdc(sc, flags);

	if (ACPI_FAILURE(rv) && rv != AE_NOT_FOUND) {
		str = "_PDC";
		goto fail;
	}

	if (cap == 0)
		cap = flags;

	return cap;

fail:
	aprint_error_dev(sc->sc_dev, "failed to evaluate "
	    "%s: %s\n", str, AcpiFormatException(rv));

	return 0;
}

static ACPI_STATUS
acpicpu_cap_pdc(struct acpicpu_softc *sc, uint32_t flags)
{
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT obj;
	uint32_t cap[3];

	arg.Count = 1;
	arg.Pointer = &obj;

	cap[0] = ACPICPU_PDC_REVID;
	cap[1] = 1;
	cap[2] = flags;

	obj.Type = ACPI_TYPE_BUFFER;
	obj.Buffer.Length = sizeof(cap);
	obj.Buffer.Pointer = (void *)cap;

	return AcpiEvaluateObject(sc->sc_node->ad_handle, "_PDC", &arg, NULL);
}

static ACPI_STATUS
acpicpu_cap_osc(struct acpicpu_softc *sc, uint32_t flags, uint32_t *val)
{
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT obj[4];
	ACPI_OBJECT *osc;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	uint32_t cap[2];
	uint32_t *ptr;
	int i = 5;

	static uint8_t intel_uuid[16] = {
		0x16, 0xA6, 0x77, 0x40, 0x0C, 0x29, 0xBE, 0x47,
		0x9E, 0xBD, 0xD8, 0x70, 0x58, 0x71, 0x39, 0x53
	};

	cap[0] = ACPI_OSC_QUERY;
	cap[1] = flags;

again:
	arg.Count = 4;
	arg.Pointer = obj;

	obj[0].Type = ACPI_TYPE_BUFFER;
	obj[0].Buffer.Length = sizeof(intel_uuid);
	obj[0].Buffer.Pointer = intel_uuid;

	obj[1].Type = ACPI_TYPE_INTEGER;
	obj[1].Integer.Value = ACPICPU_PDC_REVID;

	obj[2].Type = ACPI_TYPE_INTEGER;
	obj[2].Integer.Value = __arraycount(cap);

	obj[3].Type = ACPI_TYPE_BUFFER;
	obj[3].Buffer.Length = sizeof(cap);
	obj[3].Buffer.Pointer = (void *)cap;

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, "_OSC", &arg, &buf);

	if (ACPI_FAILURE(rv))
		goto out;

	osc = buf.Pointer;

	if (osc->Type != ACPI_TYPE_BUFFER) {
		rv = AE_TYPE;
		goto out;
	}

	if (osc->Buffer.Length != sizeof(cap)) {
		rv = AE_BUFFER_OVERFLOW;
		goto out;
	}

	ptr = (uint32_t *)osc->Buffer.Pointer;

	if ((ptr[0] & ACPI_OSC_ERROR) != 0) {
		rv = AE_ERROR;
		goto out;
	}

	if ((ptr[0] & (ACPI_OSC_ERROR_REV | ACPI_OSC_ERROR_UUID)) != 0) {
		rv = AE_BAD_PARAMETER;
		goto out;
	}

	/*
	 * "It is strongly recommended that the OS evaluate
	 *  _OSC with the Query Support Flag set until _OSC
	 *  returns the Capabilities Masked bit clear, to
	 *  negotiate the set of features to be granted to
	 *  the OS for native support (ACPI 4.0, 6.2.10)."
	 */
	if ((ptr[0] & ACPI_OSC_ERROR_MASKED) != 0 && i >= 0) {

		ACPI_FREE(buf.Pointer);
		i--;

		goto again;
	}

	if ((cap[0] & ACPI_OSC_QUERY) != 0) {

		ACPI_FREE(buf.Pointer);
		cap[0] &= ~ACPI_OSC_QUERY;

		goto again;
	}

	/*
	 * It is permitted for _OSC to return all
	 * bits cleared, but this is specified to
	 * vary on per-device basis. Assume that
	 * everything rather than nothing will be
	 * supported in this case; we do not need
	 * the firmware to know the CPU features.
	 */
	*val = (ptr[1] != 0) ? ptr[1] : cap[1];

out:
	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	return rv;
}

static void
acpicpu_notify(ACPI_HANDLE hdl, uint32_t evt, void *aux)
{
	ACPI_OSD_EXEC_CALLBACK func;
	struct acpicpu_softc *sc;
	device_t self = aux;

	sc = device_private(self);

	if (sc->sc_cold != false)
		return;

	switch (evt) {

	case ACPICPU_C_NOTIFY:

		if ((sc->sc_flags & ACPICPU_FLAG_C) == 0)
			return;

		func = acpicpu_cstate_callback;
		break;

	case ACPICPU_P_NOTIFY:

		if ((sc->sc_flags & ACPICPU_FLAG_P) == 0)
			return;

		func = acpicpu_pstate_callback;
		break;

	case ACPICPU_T_NOTIFY:

		if ((sc->sc_flags & ACPICPU_FLAG_T) == 0)
			return;

		func = acpicpu_tstate_callback;
		break;

	default:
		aprint_error_dev(sc->sc_dev,  "unknown notify: 0x%02X\n", evt);
		return;
	}

	(void)AcpiOsExecute(OSL_NOTIFY_HANDLER, func, sc->sc_dev);
}

static bool
acpicpu_suspend(device_t self, const pmf_qual_t *qual)
{
	struct acpicpu_softc *sc = device_private(self);

	if ((sc->sc_flags & ACPICPU_FLAG_C) != 0)
		(void)acpicpu_cstate_suspend(self);

	if ((sc->sc_flags & ACPICPU_FLAG_P) != 0)
		(void)acpicpu_pstate_suspend(self);

	if ((sc->sc_flags & ACPICPU_FLAG_T) != 0)
		(void)acpicpu_tstate_suspend(self);

	sc->sc_cold = true;

	return true;
}

static bool
acpicpu_resume(device_t self, const pmf_qual_t *qual)
{
	struct acpicpu_softc *sc = device_private(self);

	sc->sc_cold = false;

	if ((sc->sc_flags & ACPICPU_FLAG_C) != 0)
		(void)acpicpu_cstate_resume(self);

	if ((sc->sc_flags & ACPICPU_FLAG_P) != 0)
		(void)acpicpu_pstate_resume(self);

	if ((sc->sc_flags & ACPICPU_FLAG_T) != 0)
		(void)acpicpu_tstate_resume(self);

	return true;
}

#ifdef _MODULE

MODULE(MODULE_CLASS_DRIVER, acpicpu, NULL);
CFDRIVER_DECL(acpicpu, DV_DULL, NULL);

static int acpicpuloc[] = { -1 };
extern struct cfattach acpicpu_ca;

static struct cfparent acpiparent = {
	"acpinodebus", NULL, DVUNIT_ANY
};

static struct cfdata acpicpu_cfdata[] = {
	{
		.cf_name = "acpicpu",
		.cf_atname = "acpicpu",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
		.cf_loc = acpicpuloc,
		.cf_flags = 0,
		.cf_pspec = &acpiparent,
	},

	{ NULL, NULL, 0, 0, NULL, 0, NULL }
};

static int
acpicpu_modcmd(modcmd_t cmd, void *context)
{
	int err;

	switch (cmd) {

	case MODULE_CMD_INIT:

		err = config_cfdriver_attach(&acpicpu_cd);

		if (err != 0)
			return err;

		err = config_cfattach_attach("acpicpu", &acpicpu_ca);

		if (err != 0) {
			config_cfdriver_detach(&acpicpu_cd);
			return err;
		}

		err = config_cfdata_attach(acpicpu_cfdata, 1);

		if (err != 0) {
			config_cfattach_detach("acpicpu", &acpicpu_ca);
			config_cfdriver_detach(&acpicpu_cd);
			return err;
		}

		return 0;

	case MODULE_CMD_FINI:

		err = config_cfdata_detach(acpicpu_cfdata);

		if (err != 0)
			return err;

		config_cfattach_detach("acpicpu", &acpicpu_ca);
		config_cfdriver_detach(&acpicpu_cd);

		return 0;

	default:
		return ENOTTY;
	}
}

#endif	/* _MODULE */
