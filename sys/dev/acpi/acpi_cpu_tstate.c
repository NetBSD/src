/* $NetBSD: acpi_cpu_tstate.c,v 1.16 2010/08/21 18:25:45 jruoho Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: acpi_cpu_tstate.c,v 1.16 2010/08/21 18:25:45 jruoho Exp $");

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/kmem.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_cpu.h>

#define _COMPONENT	 ACPI_BUS_COMPONENT
ACPI_MODULE_NAME	 ("acpi_cpu_tstate")

static void		 acpicpu_tstate_attach_print(struct acpicpu_softc *);
static void		 acpicpu_tstate_attach_evcnt(struct acpicpu_softc *);
static void		 acpicpu_tstate_detach_evcnt(struct acpicpu_softc *);
static ACPI_STATUS	 acpicpu_tstate_tss(struct acpicpu_softc *);
static ACPI_STATUS	 acpicpu_tstate_tss_add(struct acpicpu_tstate *,
						ACPI_OBJECT *);
static ACPI_STATUS	 acpicpu_tstate_ptc(struct acpicpu_softc *);
static ACPI_STATUS	 acpicpu_tstate_fadt(struct acpicpu_softc *);
static ACPI_STATUS	 acpicpu_tstate_change(struct acpicpu_softc *);
static void		 acpicpu_tstate_reset(struct acpicpu_softc *);

void
acpicpu_tstate_attach(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);
	const char *str;
	ACPI_HANDLE tmp;
	ACPI_STATUS rv;

	/*
	 * Disable T-states for PIIX4.
	 */
	if ((sc->sc_flags & ACPICPU_FLAG_PIIX4) != 0)
		return;

	rv  = acpicpu_tstate_tss(sc);

	if (ACPI_FAILURE(rv)) {
		str = "_TSS";
		goto out;
	}

	rv = acpicpu_tstate_ptc(sc);

	if (ACPI_FAILURE(rv)) {
		str = "_PTC";
		goto out;
	}

	/*
	 * Comparable to P-states, the _TPC object may
	 * be absent in some systems, even though it is
	 * required by ACPI 3.0 along with _TSS and _PTC.
	 */
	rv = AcpiGetHandle(sc->sc_node->ad_handle, "_TPC", &tmp);

	if (ACPI_FAILURE(rv)) {
		aprint_debug_dev(self, "_TPC missing\n");
		rv = AE_OK;
	}

out:
	if (ACPI_FAILURE(rv)) {

		if (rv != AE_NOT_FOUND)
			aprint_error_dev(sc->sc_dev, "failed to evaluate "
			    "%s: %s\n", str, AcpiFormatException(rv));

		rv = acpicpu_tstate_fadt(sc);

		if (ACPI_FAILURE(rv))
			return;

		sc->sc_flags |= ACPICPU_FLAG_T_FADT;
	}

	sc->sc_flags |= ACPICPU_FLAG_T;

	acpicpu_tstate_reset(sc);
	acpicpu_tstate_attach_evcnt(sc);
	acpicpu_tstate_attach_print(sc);
}

static void
acpicpu_tstate_attach_print(struct acpicpu_softc *sc)
{
	const uint8_t method = sc->sc_tstate_control.reg_spaceid;
	struct acpicpu_tstate *ts;
	static bool once = false;
	const char *str;
	uint32_t i;

	if (once != false)
		return;

	str = (method != ACPI_ADR_SPACE_FIXED_HARDWARE) ? "I/O" : "FFH";

	for (i = 0; i < sc->sc_tstate_count; i++) {

		ts = &sc->sc_tstate[i];

		if (ts->ts_percent == 0)
			continue;

		aprint_debug_dev(sc->sc_dev, "T%u: %3s, "
		    "lat %3u us, pow %5u mW, %3u %%\n", i, str,
		    ts->ts_latency, ts->ts_power, ts->ts_percent);
	}

	once = true;
}

static void
acpicpu_tstate_attach_evcnt(struct acpicpu_softc *sc)
{
	struct acpicpu_tstate *ts;
	uint32_t i;

	for (i = 0; i < sc->sc_tstate_count; i++) {

		ts = &sc->sc_tstate[i];

		if (ts->ts_percent == 0)
			continue;

		(void)snprintf(ts->ts_name, sizeof(ts->ts_name),
		    "T%u (%u %%)", i, ts->ts_percent);

		evcnt_attach_dynamic(&ts->ts_evcnt, EVCNT_TYPE_MISC,
		    NULL, device_xname(sc->sc_dev), ts->ts_name);
	}
}

int
acpicpu_tstate_detach(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);
	size_t size;

	if ((sc->sc_flags & ACPICPU_FLAG_T) == 0)
		return 0;

	size = sc->sc_tstate_count * sizeof(*sc->sc_tstate);

	if (sc->sc_tstate != NULL)
		kmem_free(sc->sc_tstate, size);

	sc->sc_flags &= ~ACPICPU_FLAG_T;
	acpicpu_tstate_detach_evcnt(sc);

	return 0;
}

static void
acpicpu_tstate_detach_evcnt(struct acpicpu_softc *sc)
{
	struct acpicpu_tstate *ts;
	uint32_t i;

	for (i = 0; i < sc->sc_tstate_count; i++) {

		ts = &sc->sc_tstate[i];

		if (ts->ts_percent != 0)
			evcnt_detach(&ts->ts_evcnt);
	}
}

void
acpicpu_tstate_start(device_t self)
{
	/* Nothing. */
}

bool
acpicpu_tstate_suspend(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);

	mutex_enter(&sc->sc_mtx);
	acpicpu_tstate_reset(sc);
	mutex_exit(&sc->sc_mtx);

	return true;
}

bool
acpicpu_tstate_resume(device_t self)
{

	return true;
}

void
acpicpu_tstate_callback(void *aux)
{
	struct acpicpu_softc *sc;
	device_t self = aux;
	uint32_t omax, omin;
	int i;

	sc = device_private(self);

	if ((sc->sc_flags & ACPICPU_FLAG_T_FADT) != 0)
		return;

	mutex_enter(&sc->sc_mtx);

	/*
	 * If P-states are in use, we should ignore
	 * the interrupt unless we are in the highest
	 * P-state (see ACPI 4.0, section 8.4.3.3).
	 */
	if ((sc->sc_flags & ACPICPU_FLAG_P) != 0) {

		for (i = sc->sc_pstate_count - 1; i >= 0; i--) {

			if (sc->sc_pstate[i].ps_freq != 0)
				break;
		}

		if (sc->sc_pstate_current != sc->sc_pstate[i].ps_freq) {
			mutex_exit(&sc->sc_mtx);
			return;
		}
	}

	omax = sc->sc_tstate_max;
	omin = sc->sc_tstate_min;

	(void)acpicpu_tstate_change(sc);

	if (omax != sc->sc_tstate_max || omin != sc->sc_tstate_min) {

		aprint_debug_dev(sc->sc_dev, "throttling window "
		    "changed from %u-%u %% to %u-%u %%\n",
		    sc->sc_tstate[omax].ts_percent,
		    sc->sc_tstate[omin].ts_percent,
		    sc->sc_tstate[sc->sc_tstate_max].ts_percent,
		    sc->sc_tstate[sc->sc_tstate_min].ts_percent);
	}

	mutex_exit(&sc->sc_mtx);
}

static ACPI_STATUS
acpicpu_tstate_tss(struct acpicpu_softc *sc)
{
	struct acpicpu_tstate *ts;
	ACPI_OBJECT *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	uint32_t count;
	uint32_t i, j;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_TSS", &buf);

	if (ACPI_FAILURE(rv))
		return rv;

	obj = buf.Pointer;

	if (obj->Type != ACPI_TYPE_PACKAGE) {
		rv = AE_TYPE;
		goto out;
	}

	sc->sc_tstate_count = obj->Package.Count;

	if (sc->sc_tstate_count == 0) {
		rv = AE_NOT_EXIST;
		goto out;
	}

	if (sc->sc_tstate_count > ACPICPU_T_STATE_MAX) {
		rv = AE_LIMIT;
		goto out;
	}

	sc->sc_tstate = kmem_zalloc(sc->sc_tstate_count *
	    sizeof(struct acpicpu_tstate), KM_SLEEP);

	if (sc->sc_tstate == NULL) {
		rv = AE_NO_MEMORY;
		goto out;
	}

	for (count = i = 0; i < sc->sc_tstate_count; i++) {

		ts = &sc->sc_tstate[i];
		rv = acpicpu_tstate_tss_add(ts, &obj->Package.Elements[i]);

		if (ACPI_FAILURE(rv)) {
			ts->ts_percent = 0;
			continue;
		}

		for (j = 0; j < i; j++) {

			if (ts->ts_percent >= sc->sc_tstate[j].ts_percent) {
				ts->ts_percent = 0;
				break;
			}
		}

		if (ts->ts_percent != 0)
			count++;
	}

	if (count == 0) {
		rv = AE_NOT_EXIST;
		goto out;
	}

	/*
	 * There must be an entry with the percent
	 * field of 100. If this is not true, and if
	 * this entry is not in the expected index,
	 * invalidate the use of T-states via _TSS.
	 */
	if (sc->sc_tstate[0].ts_percent != 100) {
		rv = AE_BAD_DECIMAL_CONSTANT;
		goto out;
	}

	/*
	 * The first entry with 100 % duty cycle
	 * should have zero in the control field.
	 */
	if (sc->sc_tstate[0].ts_control != 0) {
		rv = AE_AML_BAD_RESOURCE_VALUE;
		goto out;
	}

out:
	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	return rv;
}

static ACPI_STATUS
acpicpu_tstate_tss_add(struct acpicpu_tstate *ts, ACPI_OBJECT *obj)
{
	ACPI_OBJECT *elm;
	uint32_t val[5];
	uint32_t *p;
	int i;

	if (obj->Type != ACPI_TYPE_PACKAGE)
		return AE_TYPE;

	if (obj->Package.Count != 5)
		return AE_BAD_DATA;

	elm = obj->Package.Elements;

	for (i = 0; i < 5; i++) {

		if (elm[i].Type != ACPI_TYPE_INTEGER)
			return AE_TYPE;

		if (elm[i].Integer.Value > UINT32_MAX)
			return AE_AML_NUMERIC_OVERFLOW;

		val[i] = elm[i].Integer.Value;
	}

	p = &ts->ts_percent;

	for (i = 0; i < 5; i++, p++)
		*p = val[i];

	/*
	 * The minimum should be around 100 / 8 = 12.5 %.
	 */
        if (ts->ts_percent < 10 || ts->ts_percent > 100)
		return AE_BAD_DECIMAL_CONSTANT;

	if (ts->ts_latency < 1)
		ts->ts_latency = 1;

	return AE_OK;
}

ACPI_STATUS
acpicpu_tstate_ptc(struct acpicpu_softc *sc)
{
	static const size_t size = sizeof(struct acpicpu_reg);
	struct acpicpu_reg *reg[2];
	ACPI_OBJECT *elm, *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	int i;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_PTC", &buf);

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

			/*
			 * Check that the values match the IA32 clock
			 * modulation MSR, where the bit 0 is reserved,
			 * bits 1 through 3 define the duty cycle, and
			 * the fourth bit enables the modulation.
			 */
			if (reg[i]->reg_bitwidth != 4) {
				rv = AE_AML_BAD_RESOURCE_VALUE;
				goto out;
			}

			if (reg[i]->reg_bitoffset != 1) {
				rv = AE_AML_BAD_RESOURCE_VALUE;
				goto out;
			}

			break;

		case ACPI_ADR_SPACE_FIXED_HARDWARE:

			if ((sc->sc_flags & ACPICPU_FLAG_T_FFH) == 0) {
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

	(void)memcpy(&sc->sc_tstate_control, reg[0], size);
	(void)memcpy(&sc->sc_tstate_status,  reg[1], size);

out:
	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	return rv;
}

static ACPI_STATUS
acpicpu_tstate_fadt(struct acpicpu_softc *sc)
{
	static const size_t size = sizeof(struct acpicpu_tstate);
	const uint8_t offset = AcpiGbl_FADT.DutyOffset;
	const uint8_t width = AcpiGbl_FADT.DutyWidth;
	uint8_t beta, count, i;

	if (sc->sc_object.ao_pblkaddr == 0)
		return AE_AML_ILLEGAL_ADDRESS;

	/*
	 * A zero DUTY_WIDTH is used announce that
	 * T-states are not available via FADT.
	 */
	if (width == 0 || width + offset > 4)
		return AE_AML_BAD_RESOURCE_VALUE;

	count = 1 << width;

	if (count > ACPICPU_T_STATE_MAX)
		return AE_LIMIT;

	if (sc->sc_tstate != NULL)
		kmem_free(sc->sc_tstate, sc->sc_tstate_count * size);

	sc->sc_tstate = kmem_zalloc(count * size, KM_SLEEP);

	if (sc->sc_tstate == NULL)
		return ENOMEM;

	sc->sc_tstate_count = count;

	/*
	 * Approximate duty cycles and set the MSR values.
	 */
	for (beta = 100 / count, i = 0; i < count; i++) {
		sc->sc_tstate[i].ts_percent = 100 - beta * i;
		sc->sc_tstate[i].ts_latency = 1;
	}

	for (i = 1; i < count; i++)
		sc->sc_tstate[i].ts_control = (count - i) | __BIT(3);

	/*
	 * Fake values for throttling registers.
	 */
	(void)memset(&sc->sc_tstate_status, 0, sizeof(struct acpicpu_reg));
	(void)memset(&sc->sc_tstate_control, 0, sizeof(struct acpicpu_reg));

	sc->sc_tstate_status.reg_bitwidth = width;
	sc->sc_tstate_status.reg_bitoffset = offset;
	sc->sc_tstate_status.reg_addr = sc->sc_object.ao_pblkaddr;
	sc->sc_tstate_status.reg_spaceid = ACPI_ADR_SPACE_SYSTEM_IO;

	sc->sc_tstate_control.reg_bitwidth = width;
	sc->sc_tstate_control.reg_bitoffset = offset;
	sc->sc_tstate_control.reg_addr = sc->sc_object.ao_pblkaddr;
	sc->sc_tstate_control.reg_spaceid = ACPI_ADR_SPACE_SYSTEM_IO;

	return AE_OK;
}

static ACPI_STATUS
acpicpu_tstate_change(struct acpicpu_softc *sc)
{
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	acpicpu_tstate_reset(sc);

	/*
	 * Evaluate the available T-state window:
	 *
	 *   _TPC : either this maximum or any lower power
	 *          (i.e. higher numbered) state may be used.
	 *
	 *   _TDL : either this minimum or any higher power
	 *	    (i.e. lower numbered) state may be used.
	 *
	 *   _TDL >= _TPC || _TDL >= _TSS[last entry].
	 */
	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_TPC", &val);

	if (ACPI_SUCCESS(rv) && val < sc->sc_tstate_count) {

		if (sc->sc_tstate[val].ts_percent != 0)
			sc->sc_tstate_max = val;
	}

	if (sc->sc_passive != true)
		return AE_OK;

	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_TDL", &val);

	if (ACPI_SUCCESS(rv) && val < sc->sc_tstate_count) {

		if (val >= sc->sc_tstate_max &&
		    sc->sc_tstate[val].ts_percent != 0)
			sc->sc_tstate_min = val;
	}

	return AE_OK;
}

static void
acpicpu_tstate_reset(struct acpicpu_softc *sc)
{

	sc->sc_tstate_max = 0;
	sc->sc_tstate_min = sc->sc_tstate_count - 1;
}

int
acpicpu_tstate_get(struct acpicpu_softc *sc, uint32_t *percent)
{
	const uint8_t method = sc->sc_tstate_control.reg_spaceid;
	struct acpicpu_tstate *ts = NULL;
	uint32_t i, val = 0;
	uint8_t offset;
	uint64_t addr;
	int rv;

	if (sc->sc_cold != false) {
		rv = EBUSY;
		goto fail;
	}

	if ((sc->sc_flags & ACPICPU_FLAG_T) == 0) {
		rv = ENODEV;
		goto fail;
	}

	mutex_enter(&sc->sc_mtx);

	if (sc->sc_tstate_current != ACPICPU_T_STATE_UNKNOWN) {
		*percent = sc->sc_tstate_current;
		mutex_exit(&sc->sc_mtx);
		return 0;
	}

	mutex_exit(&sc->sc_mtx);

	switch (method) {

	case ACPI_ADR_SPACE_FIXED_HARDWARE:

		rv = acpicpu_md_tstate_get(sc, percent);

		if (rv != 0)
			goto fail;

		break;

	case ACPI_ADR_SPACE_SYSTEM_IO:

		addr   = sc->sc_tstate_status.reg_addr;
		offset = sc->sc_tstate_status.reg_bitoffset;

		(void)AcpiOsReadPort(addr, &val, 8);

		val = (val >> offset) & 0x0F;

		for (i = 0; i < sc->sc_tstate_count; i++) {

			if (sc->sc_tstate[i].ts_percent == 0)
				continue;

			if (val == sc->sc_tstate[i].ts_status) {
				ts = &sc->sc_tstate[i];
				break;
			}
		}

		if (__predict_false(ts == NULL)) {
			rv = EIO;
			goto fail;
		}

		*percent = ts->ts_percent;
		break;

	default:
		rv = ENOTTY;
		goto fail;
	}

	mutex_enter(&sc->sc_mtx);
	sc->sc_tstate_current = *percent;
	mutex_exit(&sc->sc_mtx);

	return 0;

fail:
	aprint_error_dev(sc->sc_dev, "failed "
	    "to get T-state (err %d)\n", rv);

	mutex_enter(&sc->sc_mtx);
	*percent = sc->sc_tstate_current = ACPICPU_T_STATE_UNKNOWN;
	mutex_exit(&sc->sc_mtx);

	return rv;
}

int
acpicpu_tstate_set(struct acpicpu_softc *sc, uint32_t percent)
{
	const uint8_t method = sc->sc_tstate_control.reg_spaceid;
	struct acpicpu_tstate *ts = NULL;
	uint32_t i, val;
	uint8_t offset;
	uint64_t addr;
	int rv;

	if (sc->sc_cold != false) {
		rv = EBUSY;
		goto fail;
	}

	if ((sc->sc_flags & ACPICPU_FLAG_T) == 0) {
		rv = ENODEV;
		goto fail;
	}

	mutex_enter(&sc->sc_mtx);

	if (sc->sc_tstate_current == percent) {
		mutex_exit(&sc->sc_mtx);
		return 0;
	}

	for (i = sc->sc_tstate_max; i <= sc->sc_tstate_min; i++) {

		if (sc->sc_tstate[i].ts_percent == 0)
			continue;

		if (sc->sc_tstate[i].ts_percent == percent) {
			ts = &sc->sc_tstate[i];
			break;
		}
	}

	mutex_exit(&sc->sc_mtx);

	if (__predict_false(ts == NULL)) {
		rv = EINVAL;
		goto fail;
	}

	switch (method) {

	case ACPI_ADR_SPACE_FIXED_HARDWARE:

		rv = acpicpu_md_tstate_set(ts);

		if (rv != 0)
			goto fail;

		break;

	case ACPI_ADR_SPACE_SYSTEM_IO:

		addr   = sc->sc_tstate_control.reg_addr;
		offset = sc->sc_tstate_control.reg_bitoffset;

		val = (ts->ts_control & 0x0F) << offset;

		if (ts->ts_percent != 100 && (val & __BIT(4)) == 0) {
			rv = EINVAL;
			goto fail;
		}

		(void)AcpiOsWritePort(addr, val, 8);

		/*
		 * If the status field is zero, the transition is
		 * specified to be "asynchronous" and there is no
		 * need to check the status (ACPI 4.0, 8.4.3.2).
		 */
		if (ts->ts_status == 0)
			break;

		addr   = sc->sc_tstate_status.reg_addr;
		offset = sc->sc_tstate_status.reg_bitoffset;

		for (i = val = 0; i < ACPICPU_T_STATE_RETRY; i++) {

			(void)AcpiOsReadPort(addr, &val, 8);

			val = (val >> offset) & 0x0F;

			if (val == ts->ts_status)
				break;

			DELAY(ts->ts_latency);
		}

		if (i == ACPICPU_T_STATE_RETRY) {
			rv = EAGAIN;
			goto fail;
		}

		break;

	default:
		rv = ENOTTY;
		goto fail;
	}

	mutex_enter(&sc->sc_mtx);
	ts->ts_evcnt.ev_count++;
	sc->sc_tstate_current = percent;
	mutex_exit(&sc->sc_mtx);

	return 0;

fail:
	aprint_error_dev(sc->sc_dev, "failed to "
	    "throttle to %u %% (err %d)\n", percent, rv);

	mutex_enter(&sc->sc_mtx);
	sc->sc_tstate_current = ACPICPU_T_STATE_UNKNOWN;
	mutex_exit(&sc->sc_mtx);

	return rv;
}
