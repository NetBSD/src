/* $NetBSD: acpi_cpu_pstate.c,v 1.26.2.3 2010/10/22 07:21:52 uebayasi Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: acpi_cpu_pstate.c,v 1.26.2.3 2010/10/22 07:21:52 uebayasi Exp $");

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/kmem.h>
#include <sys/once.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_cpu.h>

#define _COMPONENT	 ACPI_BUS_COMPONENT
ACPI_MODULE_NAME	 ("acpi_cpu_pstate")

static void		 acpicpu_pstate_attach_print(struct acpicpu_softc *);
static void		 acpicpu_pstate_attach_evcnt(struct acpicpu_softc *);
static void		 acpicpu_pstate_detach_evcnt(struct acpicpu_softc *);
static ACPI_STATUS	 acpicpu_pstate_pss(struct acpicpu_softc *);
static ACPI_STATUS	 acpicpu_pstate_pss_add(struct acpicpu_pstate *,
						ACPI_OBJECT *);
static ACPI_STATUS	 acpicpu_pstate_xpss(struct acpicpu_softc *);
static ACPI_STATUS	 acpicpu_pstate_xpss_add(struct acpicpu_pstate *,
						 ACPI_OBJECT *);
static ACPI_STATUS	 acpicpu_pstate_pct(struct acpicpu_softc *);
static int		 acpicpu_pstate_max(struct acpicpu_softc *);
static int		 acpicpu_pstate_min(struct acpicpu_softc *);
static void		 acpicpu_pstate_change(struct acpicpu_softc *);
static void		 acpicpu_pstate_reset(struct acpicpu_softc *);
static void		 acpicpu_pstate_bios(void);

static uint32_t		 acpicpu_pstate_saved = 0;

void
acpicpu_pstate_attach(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);
	const char *str;
	ACPI_HANDLE tmp;
	ACPI_STATUS rv;

	rv = acpicpu_pstate_pss(sc);

	if (ACPI_FAILURE(rv)) {
		str = "_PSS";
		goto fail;
	}

	/*
	 * Check the availability of extended _PSS.
	 * If present, this will override the data.
	 * Note that XPSS can not be used on Intel
	 * systems where _PDC or _OSC may be used.
	 */
	if (sc->sc_cap == 0) {
		rv = acpicpu_pstate_xpss(sc);

		if (ACPI_SUCCESS(rv))
			sc->sc_flags |= ACPICPU_FLAG_P_XPSS;
	}

	rv = acpicpu_pstate_pct(sc);

	if (ACPI_FAILURE(rv)) {
		str = "_PCT";
		goto fail;
	}

	/*
	 * The ACPI 3.0 and 4.0 specifications mandate three
	 * objects for P-states: _PSS, _PCT, and _PPC. A less
	 * strict wording is however used in the earlier 2.0
	 * standard, and some systems conforming to ACPI 2.0
	 * do not have _PPC, the method for dynamic maximum.
	 */
	rv = AcpiGetHandle(sc->sc_node->ad_handle, "_PPC", &tmp);

	if (ACPI_FAILURE(rv))
		aprint_debug_dev(self, "_PPC missing\n");

	/*
	 * Employ the XPSS structure by filling
	 * it with MD information required for FFH.
	 */
	rv = acpicpu_md_pstate_pss(sc);

	if (rv != 0) {
		rv = AE_SUPPORT;
		goto fail;
	}

	sc->sc_flags |= ACPICPU_FLAG_P;

	acpicpu_pstate_bios();
	acpicpu_pstate_reset(sc);
	acpicpu_pstate_attach_evcnt(sc);
	acpicpu_pstate_attach_print(sc);

	return;

fail:
	switch (rv) {

	case AE_NOT_FOUND:
		return;

	case AE_SUPPORT:
		aprint_verbose_dev(sc->sc_dev, "P-states not supported\n");
		return;

	default:
		aprint_error_dev(sc->sc_dev, "failed to evaluate "
		    "%s: %s\n", str, AcpiFormatException(rv));
	}
}

static void
acpicpu_pstate_attach_print(struct acpicpu_softc *sc)
{
	const uint8_t method = sc->sc_pstate_control.reg_spaceid;
	struct acpicpu_pstate *ps;
	static bool once = false;
	const char *str;
	uint32_t i;

	if (once != false)
		return;

	str = (method != ACPI_ADR_SPACE_SYSTEM_IO) ? "FFH" : "I/O";

	for (i = 0; i < sc->sc_pstate_count; i++) {

		ps = &sc->sc_pstate[i];

		if (ps->ps_freq == 0)
			continue;

		aprint_debug_dev(sc->sc_dev, "P%d: %3s, "
		    "lat %3u us, pow %5u mW, %4u MHz\n", i, str,
		    ps->ps_latency, ps->ps_power, ps->ps_freq);
	}

	once = true;
}

static void
acpicpu_pstate_attach_evcnt(struct acpicpu_softc *sc)
{
	struct acpicpu_pstate *ps;
	uint32_t i;

	for (i = 0; i < sc->sc_pstate_count; i++) {

		ps = &sc->sc_pstate[i];

		if (ps->ps_freq == 0)
			continue;

		(void)snprintf(ps->ps_name, sizeof(ps->ps_name),
		    "P%u (%u MHz)", i, ps->ps_freq);

		evcnt_attach_dynamic(&ps->ps_evcnt, EVCNT_TYPE_MISC,
		    NULL, device_xname(sc->sc_dev), ps->ps_name);
	}
}

int
acpicpu_pstate_detach(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);
	static ONCE_DECL(once_detach);
	size_t size;
	int rv;

	if ((sc->sc_flags & ACPICPU_FLAG_P) == 0)
		return 0;

	rv = RUN_ONCE(&once_detach, acpicpu_md_pstate_stop);

	if (rv != 0)
		return rv;

	size = sc->sc_pstate_count * sizeof(*sc->sc_pstate);

	if (sc->sc_pstate != NULL)
		kmem_free(sc->sc_pstate, size);

	sc->sc_flags &= ~ACPICPU_FLAG_P;
	acpicpu_pstate_detach_evcnt(sc);

	return 0;
}

static void
acpicpu_pstate_detach_evcnt(struct acpicpu_softc *sc)
{
	struct acpicpu_pstate *ps;
	uint32_t i;

	for (i = 0; i < sc->sc_pstate_count; i++) {

		ps = &sc->sc_pstate[i];

		if (ps->ps_freq != 0)
			evcnt_detach(&ps->ps_evcnt);
	}
}

void
acpicpu_pstate_start(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);
	struct acpicpu_pstate *ps;
	uint32_t i;
	int rv;

	rv = acpicpu_md_pstate_start();

	if (rv != 0)
		goto fail;

	/*
	 * Initialize the state to P0.
	 */
	for (i = 0, rv = ENXIO; i < sc->sc_pstate_count; i++) {

		ps = &sc->sc_pstate[i];

		if (ps->ps_freq != 0) {
			sc->sc_cold = false;
			rv = acpicpu_pstate_set(sc, ps->ps_freq);
			break;
		}
	}

	if (rv != 0)
		goto fail;

	return;

fail:
	sc->sc_flags &= ~ACPICPU_FLAG_P;

	if (rv == EEXIST) {
		aprint_error_dev(self, "driver conflicts with existing one\n");
		return;
	}

	aprint_error_dev(self, "failed to start P-states (err %d)\n", rv);
}

bool
acpicpu_pstate_suspend(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);
	struct acpicpu_pstate *ps = NULL;
	int32_t i;

	mutex_enter(&sc->sc_mtx);
	acpicpu_pstate_reset(sc);
	mutex_exit(&sc->sc_mtx);

	if (acpicpu_pstate_saved != 0)
		return true;

	/*
	 * Following design notes for Windows, we set the highest
	 * P-state when entering any of the system sleep states.
	 * When resuming, the saved P-state will be restored.
	 *
	 *	Microsoft Corporation: Windows Native Processor
	 *	Performance Control. Version 1.1a, November, 2002.
	 */
	for (i = sc->sc_pstate_count - 1; i >= 0; i--) {

		if (sc->sc_pstate[i].ps_freq != 0) {
			ps = &sc->sc_pstate[i];
			break;
		}
	}

	if (__predict_false(ps == NULL))
		return true;

	mutex_enter(&sc->sc_mtx);
	acpicpu_pstate_saved = sc->sc_pstate_current;
	mutex_exit(&sc->sc_mtx);

	if (acpicpu_pstate_saved == ps->ps_freq)
		return true;

	(void)acpicpu_pstate_set(sc, ps->ps_freq);

	return true;
}

bool
acpicpu_pstate_resume(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);

	if (acpicpu_pstate_saved != 0) {
		(void)acpicpu_pstate_set(sc, acpicpu_pstate_saved);
		acpicpu_pstate_saved = 0;
	}

	return true;
}

void
acpicpu_pstate_callback(void *aux)
{
	struct acpicpu_softc *sc;
	device_t self = aux;
	uint32_t old, new;

	sc = device_private(self);

	mutex_enter(&sc->sc_mtx);
	old = sc->sc_pstate_max;
	acpicpu_pstate_change(sc);
	new = sc->sc_pstate_max;
	mutex_exit(&sc->sc_mtx);

	if (old != new) {

		aprint_debug_dev(sc->sc_dev, "maximum frequency "
		    "changed from P%u (%u MHz) to P%u (%u MHz)\n",
		    old, sc->sc_pstate[old].ps_freq, new,
		    sc->sc_pstate[sc->sc_pstate_max].ps_freq);
#if 0
		/*
		 * If the maximum changed, proactively
		 * raise or lower the target frequency.
		 */
		(void)acpicpu_pstate_set(sc, sc->sc_pstate[new].ps_freq);

#endif
	}
}

ACPI_STATUS
acpicpu_pstate_pss(struct acpicpu_softc *sc)
{
	struct acpicpu_pstate *ps;
	ACPI_OBJECT *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	uint32_t count;
	uint32_t i, j;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_PSS", &buf);

	if (ACPI_FAILURE(rv))
		return rv;

	obj = buf.Pointer;

	if (obj->Type != ACPI_TYPE_PACKAGE) {
		rv = AE_TYPE;
		goto out;
	}

	sc->sc_pstate_count = obj->Package.Count;

	if (sc->sc_pstate_count == 0) {
		rv = AE_NOT_EXIST;
		goto out;
	}

	if (sc->sc_pstate_count > ACPICPU_P_STATE_MAX) {
		rv = AE_LIMIT;
		goto out;
	}

	sc->sc_pstate = kmem_zalloc(sc->sc_pstate_count *
	    sizeof(struct acpicpu_pstate), KM_SLEEP);

	if (sc->sc_pstate == NULL) {
		rv = AE_NO_MEMORY;
		goto out;
	}

	for (count = i = 0; i < sc->sc_pstate_count; i++) {

		ps = &sc->sc_pstate[i];
		rv = acpicpu_pstate_pss_add(ps, &obj->Package.Elements[i]);

		if (ACPI_FAILURE(rv)) {
			ps->ps_freq = 0;
			continue;
		}

		for (j = 0; j < i; j++) {

			if (ps->ps_freq >= sc->sc_pstate[j].ps_freq) {
				ps->ps_freq = 0;
				break;
			}
		}

		if (ps->ps_freq != 0)
			count++;
	}

	rv = (count != 0) ? AE_OK : AE_NOT_EXIST;

out:
	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	return rv;
}

static ACPI_STATUS
acpicpu_pstate_pss_add(struct acpicpu_pstate *ps, ACPI_OBJECT *obj)
{
	ACPI_OBJECT *elm;
	int i;

	if (obj->Type != ACPI_TYPE_PACKAGE)
		return AE_TYPE;

	if (obj->Package.Count != 6)
		return AE_BAD_DATA;

	elm = obj->Package.Elements;

	for (i = 0; i < 6; i++) {

		if (elm[i].Type != ACPI_TYPE_INTEGER)
			return AE_TYPE;

		if (elm[i].Integer.Value > UINT32_MAX)
			return AE_AML_NUMERIC_OVERFLOW;
	}

	ps->ps_freq       = elm[0].Integer.Value;
	ps->ps_power      = elm[1].Integer.Value;
	ps->ps_latency    = elm[2].Integer.Value;
	ps->ps_latency_bm = elm[3].Integer.Value;
	ps->ps_control    = elm[4].Integer.Value;
	ps->ps_status     = elm[5].Integer.Value;

	if (ps->ps_freq == 0 || ps->ps_freq > 9999)
		return AE_BAD_DECIMAL_CONSTANT;

	/*
	 * The latency is typically around 10 usec
	 * on Intel CPUs. Use that as the minimum.
	 */
	if (ps->ps_latency < 10)
		ps->ps_latency = 10;

	return AE_OK;
}

static ACPI_STATUS
acpicpu_pstate_xpss(struct acpicpu_softc *sc)
{
	static const size_t size = sizeof(struct acpicpu_pstate);
	struct acpicpu_pstate *ps, *pstate = NULL;
	ACPI_OBJECT *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	uint32_t count, pstate_count;
	uint32_t i, j;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "XPSS", &buf);

	if (ACPI_FAILURE(rv))
		return rv;

	obj = buf.Pointer;

	if (obj->Type != ACPI_TYPE_PACKAGE) {
		rv = AE_TYPE;
		goto out;
	}

	pstate_count = count = obj->Package.Count;

	if (count == 0) {
		rv = AE_NOT_EXIST;
		goto out;
	}

	if (count > ACPICPU_P_STATE_MAX) {
		rv = AE_LIMIT;
		goto out;
	}

	pstate = kmem_zalloc(count * size, KM_SLEEP);

	if (pstate == NULL) {
		rv = AE_NO_MEMORY;
		goto out;
	}

	for (count = i = 0; i < pstate_count; i++) {

		ps = &pstate[i];
		rv = acpicpu_pstate_xpss_add(ps, &obj->Package.Elements[i]);

		if (ACPI_FAILURE(rv)) {
			ps->ps_freq = 0;
			continue;
		}

		for (j = 0; j < i; j++) {

			if (ps->ps_freq >= pstate[j].ps_freq) {
				ps->ps_freq = 0;
				break;
			}
		}

		if (ps->ps_freq != 0)
			count++;
	}

	if (count > 0) {
		if (sc->sc_pstate != NULL)
			kmem_free(sc->sc_pstate, sc->sc_pstate_count * size);
		sc->sc_pstate = pstate;
		sc->sc_pstate_count = pstate_count;
		rv = AE_OK;
	} else {
		kmem_free(pstate, pstate_count * size);
		rv = AE_NOT_EXIST;
	}

out:
	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	return rv;
}

static ACPI_STATUS
acpicpu_pstate_xpss_add(struct acpicpu_pstate *ps, ACPI_OBJECT *obj)
{
	ACPI_OBJECT *elm;
	int i;

	if (obj->Type != ACPI_TYPE_PACKAGE)
		return AE_TYPE;

	if (obj->Package.Count != 8)
		return AE_BAD_DATA;

	elm = obj->Package.Elements;

	for (i = 0; i < 4; i++) {

		if (elm[i].Type != ACPI_TYPE_INTEGER)
			return AE_TYPE;

		if (elm[i].Integer.Value > UINT32_MAX)
			return AE_AML_NUMERIC_OVERFLOW;
	}

	for (; i < 8; i++) {

		if (elm[i].Type != ACPI_TYPE_BUFFER)
			return AE_TYPE;

		if (elm[i].Buffer.Length != 8)
			return AE_LIMIT;
	}

	ps->ps_freq       = elm[0].Integer.Value;
	ps->ps_power      = elm[1].Integer.Value;
	ps->ps_latency    = elm[2].Integer.Value;
	ps->ps_latency_bm = elm[3].Integer.Value;

	if (ps->ps_freq == 0 || ps->ps_freq > 9999)
		return AE_BAD_DECIMAL_CONSTANT;

	ps->ps_control = ACPI_GET64(elm[4].Buffer.Pointer);
	ps->ps_status = ACPI_GET64(elm[5].Buffer.Pointer);
	ps->ps_control_mask = ACPI_GET64(elm[6].Buffer.Pointer);
	ps->ps_status_mask = ACPI_GET64(elm[7].Buffer.Pointer);

	/*
	 * The latency is often defined to be
	 * zero on AMD systems. Raise that to 1.
	 */
	if (ps->ps_latency == 0)
		ps->ps_latency = 1;

	ps->ps_flags |= ACPICPU_FLAG_P_XPSS;

	return AE_OK;
}

ACPI_STATUS
acpicpu_pstate_pct(struct acpicpu_softc *sc)
{
	static const size_t size = sizeof(struct acpicpu_reg);
	struct acpicpu_reg *reg[2];
	struct acpicpu_pstate *ps;
	ACPI_OBJECT *elm, *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	uint8_t width;
	uint32_t i;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_PCT", &buf);

	if (ACPI_FAILURE(rv))
		return rv;

	obj = buf.Pointer;

	if (obj->Type != ACPI_TYPE_PACKAGE) {
		rv = AE_TYPE;
		goto out;
	}

	if (obj->Package.Count != 2) {
		rv = AE_LIMIT;
		goto out;
	}

	for (i = 0; i < 2; i++) {

		elm = &obj->Package.Elements[i];

		if (elm->Type != ACPI_TYPE_BUFFER) {
			rv = AE_TYPE;
			goto out;
		}

		if (size > elm->Buffer.Length) {
			rv = AE_AML_BAD_RESOURCE_LENGTH;
			goto out;
		}

		reg[i] = (struct acpicpu_reg *)elm->Buffer.Pointer;

		switch (reg[i]->reg_spaceid) {

		case ACPI_ADR_SPACE_SYSTEM_IO:

			if (reg[i]->reg_addr == 0) {
				rv = AE_AML_ILLEGAL_ADDRESS;
				goto out;
			}

			width = reg[i]->reg_bitwidth;

			if (width + reg[i]->reg_bitoffset > 32) {
				rv = AE_AML_BAD_RESOURCE_VALUE;
				goto out;
			}

			if (width != 8 && width != 16 && width != 32) {
				rv = AE_AML_BAD_RESOURCE_VALUE;
				goto out;
			}

			break;

		case ACPI_ADR_SPACE_FIXED_HARDWARE:

			if ((sc->sc_flags & ACPICPU_FLAG_P_XPSS) != 0) {

				if (reg[i]->reg_bitwidth != 64) {
					rv = AE_AML_BAD_RESOURCE_VALUE;
					goto out;
				}

				if (reg[i]->reg_bitoffset != 0) {
					rv = AE_AML_BAD_RESOURCE_VALUE;
					goto out;
				}

				break;
			}

			if ((sc->sc_flags & ACPICPU_FLAG_P_FFH) == 0) {
				rv = AE_SUPPORT;
				goto out;
			}

			break;

		default:
			rv = AE_AML_INVALID_SPACE_ID;
			goto out;
		}
	}

	if (reg[0]->reg_spaceid != reg[1]->reg_spaceid) {
		rv = AE_AML_INVALID_SPACE_ID;
		goto out;
	}

	(void)memcpy(&sc->sc_pstate_control, reg[0], size);
	(void)memcpy(&sc->sc_pstate_status,  reg[1], size);

	if ((sc->sc_flags & ACPICPU_FLAG_P_XPSS) == 0)
		goto out;

	/*
	 * In XPSS the control address can not be zero,
	 * but the status address may be. In this case,
	 * comparable to T-states, we can ignore the status
	 * check during the P-state (FFH) transition.
	 */
	if (sc->sc_pstate_control.reg_addr == 0) {
		rv = AE_AML_BAD_RESOURCE_LENGTH;
		goto out;
	}

	/*
	 * If XPSS is present, copy the MSR addresses
	 * to the P-state structures for convenience.
	 */
	for (i = 0; i < sc->sc_pstate_count; i++) {

		ps = &sc->sc_pstate[i];

		if (ps->ps_freq == 0)
			continue;

		ps->ps_status_addr  = sc->sc_pstate_status.reg_addr;
		ps->ps_control_addr = sc->sc_pstate_control.reg_addr;
	}

out:
	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	return rv;
}

static int
acpicpu_pstate_max(struct acpicpu_softc *sc)
{
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	/*
	 * Evaluate the currently highest P-state that can be used.
	 * If available, we can use either this state or any lower
	 * power (i.e. higher numbered) state from the _PSS object.
	 * Note that the return value must match the _OST parameter.
	 */
	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_PPC", &val);

	if (ACPI_SUCCESS(rv) && val < sc->sc_pstate_count) {

		if (sc->sc_pstate[val].ps_freq != 0) {
			sc->sc_pstate_max = val;
			return 0;
		}
	}

	return 1;
}

static int
acpicpu_pstate_min(struct acpicpu_softc *sc)
{
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	/*
	 * The _PDL object defines the minimum when passive cooling
	 * is being performed. If available, we can use the returned
	 * state or any higher power (i.e. lower numbered) state.
	 */
	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_PDL", &val);

	if (ACPI_SUCCESS(rv) && val < sc->sc_pstate_count) {

		if (sc->sc_pstate[val].ps_freq == 0)
			return 1;

		if (val >= sc->sc_pstate_max) {
			sc->sc_pstate_min = val;
			return 0;
		}
	}

	return 1;
}

static void
acpicpu_pstate_change(struct acpicpu_softc *sc)
{
	static ACPI_STATUS rv = AE_OK;
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT obj[2];

	acpicpu_pstate_reset(sc);

	arg.Count = 2;
	arg.Pointer = obj;

	obj[0].Type = ACPI_TYPE_INTEGER;
	obj[1].Type = ACPI_TYPE_INTEGER;

	obj[0].Integer.Value = ACPICPU_P_NOTIFY;
	obj[1].Integer.Value = acpicpu_pstate_max(sc);

	if (sc->sc_passive != false)
		(void)acpicpu_pstate_min(sc);

	if (ACPI_FAILURE(rv))
		return;

	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, "_OST", &arg, NULL);
}

static void
acpicpu_pstate_reset(struct acpicpu_softc *sc)
{

	sc->sc_pstate_max = 0;
	sc->sc_pstate_min = sc->sc_pstate_count - 1;

}

static void
acpicpu_pstate_bios(void)
{
	const uint8_t val = AcpiGbl_FADT.PstateControl;
	const uint32_t addr = AcpiGbl_FADT.SmiCommand;

	if (addr == 0 || val == 0)
		return;

	(void)AcpiOsWritePort(addr, val, 8);
}

int
acpicpu_pstate_get(struct acpicpu_softc *sc, uint32_t *freq)
{
	const uint8_t method = sc->sc_pstate_control.reg_spaceid;
	struct acpicpu_pstate *ps = NULL;
	uint32_t i, val = 0;
	uint64_t addr;
	uint8_t width;
	int rv;

	if (sc->sc_cold != false) {
		rv = EBUSY;
		goto fail;
	}

	if ((sc->sc_flags & ACPICPU_FLAG_P) == 0) {
		rv = ENODEV;
		goto fail;
	}

	mutex_enter(&sc->sc_mtx);

	if (sc->sc_pstate_current != ACPICPU_P_STATE_UNKNOWN) {
		*freq = sc->sc_pstate_current;
		mutex_exit(&sc->sc_mtx);
		return 0;
	}

	mutex_exit(&sc->sc_mtx);

	switch (method) {

	case ACPI_ADR_SPACE_FIXED_HARDWARE:

		rv = acpicpu_md_pstate_get(sc, freq);

		if (rv != 0)
			goto fail;

		break;

	case ACPI_ADR_SPACE_SYSTEM_IO:

		addr  = sc->sc_pstate_status.reg_addr;
		width = sc->sc_pstate_status.reg_bitwidth;

		(void)AcpiOsReadPort(addr, &val, width);

		if (val == 0) {
			rv = EIO;
			goto fail;
		}

		for (i = 0; i < sc->sc_pstate_count; i++) {

			if (sc->sc_pstate[i].ps_freq == 0)
				continue;

			if (val == sc->sc_pstate[i].ps_status) {
				ps = &sc->sc_pstate[i];
				break;
			}
		}

		if (__predict_false(ps == NULL)) {
			rv = EIO;
			goto fail;
		}

		*freq = ps->ps_freq;
		break;

	default:
		rv = ENOTTY;
		goto fail;
	}

	mutex_enter(&sc->sc_mtx);
	sc->sc_pstate_current = *freq;
	mutex_exit(&sc->sc_mtx);

	return 0;

fail:
	aprint_error_dev(sc->sc_dev, "failed "
	    "to get frequency (err %d)\n", rv);

	mutex_enter(&sc->sc_mtx);
	*freq = sc->sc_pstate_current = ACPICPU_P_STATE_UNKNOWN;
	mutex_exit(&sc->sc_mtx);

	return rv;
}

int
acpicpu_pstate_set(struct acpicpu_softc *sc, uint32_t freq)
{
	const uint8_t method = sc->sc_pstate_control.reg_spaceid;
	struct acpicpu_pstate *ps = NULL;
	uint32_t i, val;
	uint64_t addr;
	uint8_t width;
	int rv;

	if (sc->sc_cold != false) {
		rv = EBUSY;
		goto fail;
	}

	if ((sc->sc_flags & ACPICPU_FLAG_P) == 0) {
		rv = ENODEV;
		goto fail;
	}

	mutex_enter(&sc->sc_mtx);

	if (sc->sc_pstate_current == freq) {
		mutex_exit(&sc->sc_mtx);
		return 0;
	}

	for (i = sc->sc_pstate_max; i <= sc->sc_pstate_min; i++) {

		if (sc->sc_pstate[i].ps_freq == 0)
			continue;

		if (sc->sc_pstate[i].ps_freq == freq) {
			ps = &sc->sc_pstate[i];
			break;
		}
	}

	mutex_exit(&sc->sc_mtx);

	if (__predict_false(ps == NULL)) {
		rv = EINVAL;
		goto fail;
	}

	switch (method) {

	case ACPI_ADR_SPACE_FIXED_HARDWARE:

		rv = acpicpu_md_pstate_set(ps);

		if (rv != 0)
			goto fail;

		break;

	case ACPI_ADR_SPACE_SYSTEM_IO:

		addr  = sc->sc_pstate_control.reg_addr;
		width = sc->sc_pstate_control.reg_bitwidth;

		(void)AcpiOsWritePort(addr, ps->ps_control, width);

		addr  = sc->sc_pstate_status.reg_addr;
		width = sc->sc_pstate_status.reg_bitwidth;

		/*
		 * Some systems take longer to respond
		 * than the reported worst-case latency.
		 */
		for (i = val = 0; i < ACPICPU_P_STATE_RETRY; i++) {

			(void)AcpiOsReadPort(addr, &val, width);

			if (val == ps->ps_status)
				break;

			DELAY(ps->ps_latency);
		}

		if (i == ACPICPU_P_STATE_RETRY) {
			rv = EAGAIN;
			goto fail;
		}

		break;

	default:
		rv = ENOTTY;
		goto fail;
	}

	mutex_enter(&sc->sc_mtx);
	ps->ps_evcnt.ev_count++;
	sc->sc_pstate_current = freq;
	mutex_exit(&sc->sc_mtx);

	return 0;

fail:
	aprint_error_dev(sc->sc_dev, "failed to set "
	    "frequency to %u (err %d)\n", freq, rv);

	mutex_enter(&sc->sc_mtx);
	sc->sc_pstate_current = ACPICPU_P_STATE_UNKNOWN;
	mutex_exit(&sc->sc_mtx);

	return rv;
}
