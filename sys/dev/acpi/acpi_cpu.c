/* $NetBSD: acpi_cpu.c,v 1.5 2010/07/23 05:32:02 jruoho Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: acpi_cpu.c,v 1.5 2010/07/23 05:32:02 jruoho Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/module.h>
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

static int		  acpicpu_object(ACPI_HANDLE, struct acpicpu_object *);
static cpuid_t		  acpicpu_id(uint32_t);
static uint32_t		  acpicpu_cap(struct acpicpu_softc *);
static ACPI_OBJECT	 *acpicpu_cap_init(void);
static ACPI_STATUS	  acpicpu_cap_pdc(ACPI_HANDLE);
static ACPI_STATUS	  acpicpu_cap_osc(ACPI_HANDLE, uint32_t *);
static const char	 *acpicpu_cap_oscerr(uint32_t);
static void		  acpicpu_notify(ACPI_HANDLE, uint32_t, void *);
static bool		  acpicpu_suspend(device_t, const pmf_qual_t *);
static bool		  acpicpu_resume(device_t, const pmf_qual_t *);

kmutex_t		  acpicpu_mtx;
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

	KASSERT(acpicpu_sc != NULL);

	mutex_enter(&acpicpu_mtx);

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_node = aa->aa_node;
	sc->sc_cpuid = acpicpu_id(sc->sc_object.ao_procid);

	if (sc->sc_cpuid == 0xFFFFFF) {
		mutex_exit(&acpicpu_mtx);
		aprint_error(": invalid CPU ID\n");
		return;
	}

	if (acpicpu_sc[sc->sc_cpuid] != NULL) {
		mutex_exit(&acpicpu_mtx);
		aprint_error(": already probed\n");
		return;
	}

	acpicpu_sc[sc->sc_cpuid] = sc;
	mutex_exit(&acpicpu_mtx);

	sc->sc_cap = acpicpu_cap(sc);
	sc->sc_flags |= acpicpu_md_quirks();

	aprint_naive("\n");
	aprint_normal(": ACPI CPU");
	aprint_verbose(", cap 0x%02x, addr 0x%06x, len 0x%02x",
	    sc->sc_cap, sc->sc_object.ao_pblkaddr, sc->sc_object.ao_pblklen);
	aprint_normal("\n");

	/*
	 * We should claim the bus space. However, we do this only
	 * to announce that the space is in use. This is unnecessary
	 * if system I/O type C-states are not used. Many systems also
	 * report invalid values in the processor object. Finally, this
	 * is known to conflict with other devices. But as is noted in
	 * ichlpcib(4), we can continue our I/O without bus_space(9).
	 */
	if (sc->sc_object.ao_pblklen == 6 && sc->sc_object.ao_pblkaddr != 0) {

		rv = bus_space_map(sc->sc_iot, sc->sc_object.ao_pblkaddr,
		    sc->sc_object.ao_pblklen, 0, &sc->sc_ioh);

		if (rv != 0)
			sc->sc_ioh = 0;
	}

	acpicpu_cstate_attach(self);

	(void)acpi_register_notify(sc->sc_node, acpicpu_notify);
	(void)config_finalize_register(self, acpicpu_cstate_start);
	(void)pmf_device_register(self, acpicpu_suspend, acpicpu_resume);
}

static int
acpicpu_detach(device_t self, int flags)
{
	struct acpicpu_softc *sc = device_private(self);
	const bus_addr_t addr = sc->sc_object.ao_pblkaddr;
	static ONCE_DECL(once_detach);
	int rv = 0;

	acpi_deregister_notify(sc->sc_node);

	if ((sc->sc_flags & ACPICPU_FLAG_C) != 0)
		rv = acpicpu_cstate_detach(self);

	if (rv != 0)
		return rv;

	rv = RUN_ONCE(&once_detach, acpicpu_once_detach);

	if (rv != 0)
		return rv;

	if (sc->sc_ioh != 0)
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, addr);

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

	mutex_init(&acpicpu_mtx, MUTEX_DEFAULT, IPL_VM);

	return 0;
}

static int
acpicpu_once_detach(void)
{
	struct acpicpu_softc *sc;

	KASSERT(acpicpu_sc != NULL);

	mutex_destroy(&acpicpu_mtx);
	kmem_free(acpicpu_sc, maxcpus * sizeof(*sc));
	acpicpu_sc = NULL;

	return 0;
}

static int
acpicpu_object(ACPI_HANDLE hdl, struct acpicpu_object *ao)
{
	ACPI_OBJECT *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	rv = acpi_eval_struct(hdl, NULL, &buf);

	if (ACPI_FAILURE(rv))
		return 0;

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

		if (id == ci->ci_cpuid)
			return id;
	}

	return 0xFFFFFF;
}

static uint32_t
acpicpu_cap(struct acpicpu_softc *sc)
{
	uint32_t cap[3] = { 0 };
	ACPI_STATUS rv;
	int err;

	/*
	 * Set machine-dependent processor capabilities.
	 *
	 * The _PDC was deprecated in ACPI 3.0 in favor of the _OSC,
	 * but firmware may expect that we evaluate it nevertheless.
	 */
	rv = acpicpu_cap_pdc(sc->sc_node->ad_handle);

	if (ACPI_FAILURE(rv) && rv != AE_NOT_FOUND)
		aprint_error_dev(sc->sc_dev, "failed to evaluate _PDC: "
		    "%s\n", AcpiFormatException(rv));

	rv = acpicpu_cap_osc(sc->sc_node->ad_handle, cap);

	if (ACPI_FAILURE(rv) && rv != AE_NOT_FOUND)
		aprint_error_dev(sc->sc_dev, "failed to evaluate _OSC: "
		    "%s\n", AcpiFormatException(rv));

	if (ACPI_SUCCESS(rv)) {

		err = cap[0] & ~__BIT(0);

		if (err != 0) {
			aprint_error_dev(sc->sc_dev, "errors in "
			    "_OSC: %s\n", acpicpu_cap_oscerr(err));
			cap[2] = 0;
		}
	}

	return cap[2];
}

static ACPI_OBJECT *
acpicpu_cap_init(void)
{
	static uint32_t cap[3];
	static ACPI_OBJECT obj;

	cap[0] = ACPICPU_PDC_REVID;
	cap[1] = 1;
	cap[2] = acpicpu_md_cap();

	obj.Type = ACPI_TYPE_BUFFER;
	obj.Buffer.Length = sizeof(cap);
	obj.Buffer.Pointer = (uint8_t *)cap;

	return &obj;
}

static ACPI_STATUS
acpicpu_cap_pdc(ACPI_HANDLE hdl)
{
	ACPI_OBJECT_LIST arg_list;

	arg_list.Count = 1;
	arg_list.Pointer = acpicpu_cap_init();

	return AcpiEvaluateObject(hdl, "_PDC", &arg_list, NULL);
}

static ACPI_STATUS
acpicpu_cap_osc(ACPI_HANDLE hdl, uint32_t *val)
{
	ACPI_OBJECT_LIST arg_list;
	ACPI_OBJECT *cap, *obj;
	ACPI_OBJECT arg[4];
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	/* Intel. */
	static uint8_t cpu_oscuuid[16] = {
		0x16, 0xA6, 0x77, 0x40, 0x0C, 0x29, 0xBE, 0x47,
		0x9E, 0xBD, 0xD8, 0x70, 0x58, 0x71, 0x39, 0x53
	};

	cap = acpicpu_cap_init();

	arg_list.Count = 4;
	arg_list.Pointer = arg;

	arg[0].Type = ACPI_TYPE_BUFFER;
	arg[0].Buffer.Length = sizeof(cpu_oscuuid);
	arg[0].Buffer.Pointer = cpu_oscuuid;

	arg[1].Type = ACPI_TYPE_INTEGER;
	arg[1].Integer.Value = ACPICPU_PDC_REVID;

	arg[2].Type = ACPI_TYPE_INTEGER;
	arg[2].Integer.Value = cap->Buffer.Length / sizeof(uint32_t);

	arg[3] = *cap;

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	rv = AcpiEvaluateObject(hdl, "_OSC", &arg_list, &buf);

	if (ACPI_FAILURE(rv))
		return rv;

	obj = buf.Pointer;

	if (obj->Type != ACPI_TYPE_BUFFER) {
		rv = AE_TYPE;
		goto out;
	}

	if (obj->Buffer.Length != cap->Buffer.Length) {
		rv = AE_BUFFER_OVERFLOW;
		goto out;
	}

	(void)memcpy(val, obj->Buffer.Pointer, obj->Buffer.Length);

out:
	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	return rv;
}

static const char *
acpicpu_cap_oscerr(uint32_t err)
{

	KASSERT((err & __BIT(0)) == 0);

	if ((err & __BIT(1)) != 0)
		return "_OSC failure";

	if ((err & __BIT(2)) != 0)
		return "unrecognized UUID";

	if ((err & __BIT(3)) != 0)
		return "unrecognized revision";

	if ((err & __BIT(4)) != 0)
		return "capabilities masked";

	return "unknown error";
}

static void
acpicpu_notify(ACPI_HANDLE hdl, uint32_t evt, void *aux)
{
	ACPI_OSD_EXEC_CALLBACK func;
	struct acpicpu_softc *sc;
	device_t self = aux;

	sc = device_private(self);

	switch (evt) {

	case ACPICPU_C_NOTIFY:

		if ((sc->sc_flags & ACPICPU_FLAG_C) == 0)
			return;

		func = acpicpu_cstate_callback;
		break;

	case ACPICPU_P_NOTIFY:

		if ((sc->sc_flags & ACPICPU_FLAG_P) == 0)
			return;

		func = NULL;
		break;

	case ACPICPU_T_NOTIFY:

		if ((sc->sc_flags & ACPICPU_FLAG_T) == 0)
			return;

		func = NULL;
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

	return true;
}

static bool
acpicpu_resume(device_t self, const pmf_qual_t *qual)
{
	struct acpicpu_softc *sc = device_private(self);

	if ((sc->sc_flags & ACPICPU_FLAG_C) != 0)
		(void)acpicpu_cstate_resume(self);

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
