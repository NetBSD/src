/* $NetBSD: acpi_cpu_cstate.c,v 1.32 2010/08/22 17:45:48 jruoho Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: acpi_cpu_cstate.c,v 1.32 2010/08/22 17:45:48 jruoho Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/kernel.h>
#include <sys/once.h>
#include <sys/mutex.h>
#include <sys/timetc.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_cpu.h>
#include <dev/acpi/acpi_timer.h>

#include <machine/acpi_machdep.h>

#define _COMPONENT	 ACPI_BUS_COMPONENT
ACPI_MODULE_NAME	 ("acpi_cpu_cstate")

static void		 acpicpu_cstate_attach_print(struct acpicpu_softc *);
static void		 acpicpu_cstate_attach_evcnt(struct acpicpu_softc *);
static void		 acpicpu_cstate_detach_evcnt(struct acpicpu_softc *);
static ACPI_STATUS	 acpicpu_cstate_cst(struct acpicpu_softc *);
static ACPI_STATUS	 acpicpu_cstate_cst_add(struct acpicpu_softc *,
						ACPI_OBJECT *);
static void		 acpicpu_cstate_cst_bios(void);
static void		 acpicpu_cstate_memset(struct acpicpu_softc *);
static void		 acpicpu_cstate_fadt(struct acpicpu_softc *);
static void		 acpicpu_cstate_quirks(struct acpicpu_softc *);
static int		 acpicpu_cstate_latency(struct acpicpu_softc *);
static bool		 acpicpu_cstate_bm_check(void);
static void		 acpicpu_cstate_idle_enter(struct acpicpu_softc *,int);

extern struct acpicpu_softc **acpicpu_sc;

/*
 * XXX:	The local APIC timer (as well as TSC) is typically stopped in C3.
 *	For now, we cannot but disable C3. But there appears to be timer-
 *	related interrupt issues also in C2. The only entirely safe option
 *	at the moment is to use C1.
 */
#ifdef ACPICPU_ENABLE_C3
static int cs_state_max = ACPI_STATE_C3;
#else
static int cs_state_max = ACPI_STATE_C1;
#endif

void
acpicpu_cstate_attach(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);
	ACPI_STATUS rv;

	/*
	 * Either use the preferred _CST or resort to FADT.
	 */
	rv = acpicpu_cstate_cst(sc);

	switch (rv) {

	case AE_OK:
		acpicpu_cstate_cst_bios();
		break;

	default:
		sc->sc_flags |= ACPICPU_FLAG_C_FADT;
		acpicpu_cstate_fadt(sc);
		break;
	}

	sc->sc_flags |= ACPICPU_FLAG_C;

	acpicpu_cstate_quirks(sc);
	acpicpu_cstate_attach_evcnt(sc);
	acpicpu_cstate_attach_print(sc);
}

void
acpicpu_cstate_attach_print(struct acpicpu_softc *sc)
{
	struct acpicpu_cstate *cs;
	static bool once = false;
	const char *str;
	int i;

	if (once != false)
		return;

	for (i = 0; i < ACPI_C_STATE_COUNT; i++) {

		cs = &sc->sc_cstate[i];

		if (cs->cs_method == 0)
			continue;

		switch (cs->cs_method) {

		case ACPICPU_C_STATE_HALT:
			str = "HLT";
			break;

		case ACPICPU_C_STATE_FFH:
			str = "FFH";
			break;

		case ACPICPU_C_STATE_SYSIO:
			str = "I/O";
			break;

		default:
			panic("NOTREACHED");
		}

		aprint_debug_dev(sc->sc_dev, "C%d: %3s, "
		    "lat %3u us, pow %5u mW, flags 0x%02x\n", i, str,
		    cs->cs_latency, cs->cs_power, cs->cs_flags);
	}

	once = true;
}

static void
acpicpu_cstate_attach_evcnt(struct acpicpu_softc *sc)
{
	struct acpicpu_cstate *cs;
	const char *str;
	int i;

	for (i = 0; i < ACPI_C_STATE_COUNT; i++) {

		cs = &sc->sc_cstate[i];

		if (cs->cs_method == 0)
			continue;

		str = "HALT";

		if (cs->cs_method == ACPICPU_C_STATE_FFH)
			str = "MWAIT";

		if (cs->cs_method == ACPICPU_C_STATE_SYSIO)
			str = "I/O";

		(void)snprintf(cs->cs_name, sizeof(cs->cs_name),
		    "C%d (%s)", i, str);

		evcnt_attach_dynamic(&cs->cs_evcnt, EVCNT_TYPE_MISC,
		    NULL, device_xname(sc->sc_dev), cs->cs_name);
	}
}

int
acpicpu_cstate_detach(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);
	static ONCE_DECL(once_detach);
	int rv;

	rv = RUN_ONCE(&once_detach, acpicpu_md_idle_stop);

	if (rv != 0)
		return rv;

	sc->sc_flags &= ~ACPICPU_FLAG_C;
	acpicpu_cstate_detach_evcnt(sc);

	return 0;
}

static void
acpicpu_cstate_detach_evcnt(struct acpicpu_softc *sc)
{
	struct acpicpu_cstate *cs;
	int i;

	for (i = 0; i < ACPI_C_STATE_COUNT; i++) {

		cs = &sc->sc_cstate[i];

		if (cs->cs_method != 0)
			evcnt_detach(&cs->cs_evcnt);
	}
}

void
acpicpu_cstate_start(device_t self)
{

	(void)acpicpu_md_idle_start();
}

bool
acpicpu_cstate_suspend(device_t self)
{

	return true;
}

bool
acpicpu_cstate_resume(device_t self)
{
	struct acpicpu_softc *sc = device_private(self);

	if ((sc->sc_flags & ACPICPU_FLAG_C_FADT) == 0)
		acpicpu_cstate_callback(self);

	return true;
}

void
acpicpu_cstate_callback(void *aux)
{
	struct acpicpu_softc *sc;
	device_t self = aux;

	sc = device_private(self);

	if ((sc->sc_flags & ACPICPU_FLAG_C_FADT) != 0)
		return;

	mutex_enter(&sc->sc_mtx);
	(void)acpicpu_cstate_cst(sc);
	mutex_exit(&sc->sc_mtx);
}

static ACPI_STATUS
acpicpu_cstate_cst(struct acpicpu_softc *sc)
{
	ACPI_OBJECT *elm, *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	uint32_t i, n;
	uint8_t count;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_CST", &buf);

	if (ACPI_FAILURE(rv))
		return rv;

	obj = buf.Pointer;

	if (obj->Type != ACPI_TYPE_PACKAGE) {
		rv = AE_TYPE;
		goto out;
	}

	if (obj->Package.Count < 2) {
		rv = AE_LIMIT;
		goto out;
	}

	elm = obj->Package.Elements;

	if (elm[0].Type != ACPI_TYPE_INTEGER) {
		rv = AE_TYPE;
		goto out;
	}

	n = elm[0].Integer.Value;

	if (n != obj->Package.Count - 1) {
		rv = AE_BAD_VALUE;
		goto out;
	}

	if (n > ACPI_C_STATES_MAX) {
		rv = AE_LIMIT;
		goto out;
	}

	acpicpu_cstate_memset(sc);

	CTASSERT(ACPI_STATE_C0 == 0 && ACPI_STATE_C1 == 1);
	CTASSERT(ACPI_STATE_C2 == 2 && ACPI_STATE_C3 == 3);

	for (count = 0, i = 1; i <= n; i++) {

		elm = &obj->Package.Elements[i];
		rv = acpicpu_cstate_cst_add(sc, elm);

		if (ACPI_SUCCESS(rv))
			count++;
	}

	rv = (count != 0) ? AE_OK : AE_NOT_EXIST;

out:
	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	return rv;
}

static ACPI_STATUS
acpicpu_cstate_cst_add(struct acpicpu_softc *sc, ACPI_OBJECT *elm)
{
	const struct acpicpu_object *ao = &sc->sc_object;
	struct acpicpu_cstate *cs = sc->sc_cstate;
	struct acpicpu_cstate state;
	struct acpicpu_reg *reg;
	ACPI_STATUS rv = AE_OK;
	ACPI_OBJECT *obj;
	uint32_t type;

	(void)memset(&state, 0, sizeof(*cs));

	state.cs_flags = ACPICPU_FLAG_C_BM_STS;

	if (elm->Type != ACPI_TYPE_PACKAGE) {
		rv = AE_TYPE;
		goto out;
	}

	if (elm->Package.Count != 4) {
		rv = AE_LIMIT;
		goto out;
	}

	/*
	 * Type.
	 */
	obj = &elm->Package.Elements[1];

	if (obj->Type != ACPI_TYPE_INTEGER) {
		rv = AE_TYPE;
		goto out;
	}

	type = obj->Integer.Value;

	if (type < ACPI_STATE_C1 || type > ACPI_STATE_C3) {
		rv = AE_TYPE;
		goto out;
	}

	/*
	 * Latency.
	 */
	obj = &elm->Package.Elements[2];

	if (obj->Type != ACPI_TYPE_INTEGER) {
		rv = AE_TYPE;
		goto out;
	}

	state.cs_latency = obj->Integer.Value;

	/*
	 * Power.
	 */
	obj = &elm->Package.Elements[3];

	if (obj->Type != ACPI_TYPE_INTEGER) {
		rv = AE_TYPE;
		goto out;
	}

	state.cs_power = obj->Integer.Value;

	/*
	 * Register.
	 */
	obj = &elm->Package.Elements[0];

	if (obj->Type != ACPI_TYPE_BUFFER) {
		rv = AE_TYPE;
		goto out;
	}

	CTASSERT(sizeof(struct acpicpu_reg) == 15);

	if (obj->Buffer.Length < sizeof(struct acpicpu_reg)) {
		rv = AE_LIMIT;
		goto out;
	}

	reg = (struct acpicpu_reg *)obj->Buffer.Pointer;

	switch (reg->reg_spaceid) {

	case ACPI_ADR_SPACE_SYSTEM_IO:
		state.cs_method = ACPICPU_C_STATE_SYSIO;

		if (reg->reg_addr == 0) {
			rv = AE_AML_ILLEGAL_ADDRESS;
			goto out;
		}

		if (reg->reg_bitwidth != 8) {
			rv = AE_AML_BAD_RESOURCE_LENGTH;
			goto out;
		}

		/*
		 * Check only that the address is in the mapped space.
		 * Systems are allowed to change it when operating
		 * with _CST (see ACPI 4.0, pp. 94-95). For instance,
		 * the offset of P_LVL3 may change depending on whether
		 * acpiacad(4) is connected or disconnected.
		 */
		if (reg->reg_addr > ao->ao_pblkaddr + ao->ao_pblklen) {
			rv = AE_BAD_ADDRESS;
			goto out;
		}

		state.cs_addr = reg->reg_addr;
		break;

	case ACPI_ADR_SPACE_FIXED_HARDWARE:
		state.cs_method = ACPICPU_C_STATE_FFH;

		switch (type) {

		case ACPI_STATE_C1:

			if ((sc->sc_flags & ACPICPU_FLAG_C_FFH) == 0)
				state.cs_method = ACPICPU_C_STATE_HALT;

			break;

		default:

			if ((sc->sc_flags & ACPICPU_FLAG_C_FFH) == 0) {
				rv = AE_SUPPORT;
				goto out;
			}
		}

		if (sc->sc_cap != 0) {

			/*
			 * The _CST FFH GAS encoding may contain
			 * additional hints on Intel processors.
			 * Use these to determine whether we can
			 * avoid the bus master activity check.
			 */
			if ((reg->reg_accesssize & ACPICPU_PDC_GAS_BM) == 0)
				state.cs_flags &= ~ACPICPU_FLAG_C_BM_STS;
		}

		break;

	default:
		rv = AE_AML_INVALID_SPACE_ID;
		goto out;
	}

	if (cs[type].cs_method != 0) {
		rv = AE_ALREADY_EXISTS;
		goto out;
	}

	cs[type].cs_addr = state.cs_addr;
	cs[type].cs_power = state.cs_power;
	cs[type].cs_flags = state.cs_flags;
	cs[type].cs_method = state.cs_method;
	cs[type].cs_latency = state.cs_latency;

out:
	if (ACPI_FAILURE(rv))
		aprint_debug_dev(sc->sc_dev, "invalid "
		    "_CST: %s\n", AcpiFormatException(rv));

	return rv;
}

static void
acpicpu_cstate_cst_bios(void)
{
	const uint8_t val = AcpiGbl_FADT.CstControl;
	const uint32_t addr = AcpiGbl_FADT.SmiCommand;

	if (addr == 0 || val == 0)
		return;

	(void)AcpiOsWritePort(addr, val, 8);
}

static void
acpicpu_cstate_memset(struct acpicpu_softc *sc)
{
	int i = 0;

	while (i < ACPI_C_STATE_COUNT) {

		sc->sc_cstate[i].cs_addr = 0;
		sc->sc_cstate[i].cs_power = 0;
		sc->sc_cstate[i].cs_flags = 0;
		sc->sc_cstate[i].cs_method = 0;
		sc->sc_cstate[i].cs_latency = 0;

		i++;
	}
}

static void
acpicpu_cstate_fadt(struct acpicpu_softc *sc)
{
	struct acpicpu_cstate *cs = sc->sc_cstate;

	acpicpu_cstate_memset(sc);

	/*
	 * All x86 processors should support C1 (a.k.a. HALT).
	 */
	if ((AcpiGbl_FADT.Flags & ACPI_FADT_C1_SUPPORTED) != 0)
		cs[ACPI_STATE_C1].cs_method = ACPICPU_C_STATE_HALT;

	if (sc->sc_object.ao_pblkaddr == 0)
		return;

	if (acpicpu_md_cpus_running() > 1) {

		if ((AcpiGbl_FADT.Flags & ACPI_FADT_C2_MP_SUPPORTED) == 0)
			return;
	}

	cs[ACPI_STATE_C2].cs_method = ACPICPU_C_STATE_SYSIO;
	cs[ACPI_STATE_C3].cs_method = ACPICPU_C_STATE_SYSIO;

	cs[ACPI_STATE_C2].cs_latency = AcpiGbl_FADT.C2Latency;
	cs[ACPI_STATE_C3].cs_latency = AcpiGbl_FADT.C3Latency;

	cs[ACPI_STATE_C2].cs_addr = sc->sc_object.ao_pblkaddr + 4;
	cs[ACPI_STATE_C3].cs_addr = sc->sc_object.ao_pblkaddr + 5;

	/*
	 * The P_BLK length should always be 6. If it
	 * is not, reduce functionality accordingly.
	 */
	if (sc->sc_object.ao_pblklen < 5)
		cs[ACPI_STATE_C2].cs_method = 0;

	if (sc->sc_object.ao_pblklen < 6)
		cs[ACPI_STATE_C3].cs_method = 0;

	/*
	 * Sanity check the latency levels in FADT.
	 * Values above the thresholds are used to
	 * inform that C-states are not supported.
	 */
	CTASSERT(ACPICPU_C_C2_LATENCY_MAX == 100);
	CTASSERT(ACPICPU_C_C3_LATENCY_MAX == 1000);

	if (AcpiGbl_FADT.C2Latency > ACPICPU_C_C2_LATENCY_MAX)
		cs[ACPI_STATE_C2].cs_method = 0;

	if (AcpiGbl_FADT.C3Latency > ACPICPU_C_C3_LATENCY_MAX)
		cs[ACPI_STATE_C3].cs_method = 0;
}

static void
acpicpu_cstate_quirks(struct acpicpu_softc *sc)
{
	const uint32_t reg = AcpiGbl_FADT.Pm2ControlBlock;
	const uint32_t len = AcpiGbl_FADT.Pm2ControlLength;

	/*
	 * Disable C3 for PIIX4.
	 */
	if ((sc->sc_flags & ACPICPU_FLAG_PIIX4) != 0) {
		sc->sc_cstate[ACPI_STATE_C3].cs_method = 0;
		return;
	}

	/*
	 * Check bus master arbitration. If ARB_DIS
	 * is not available, processor caches must be
	 * flushed before C3 (ACPI 4.0, section 8.2).
	 */
	if (reg != 0 && len != 0) {
		sc->sc_flags |= ACPICPU_FLAG_C_ARB;
		return;
	}

	/*
	 * Disable C3 entirely if WBINVD is not present.
	 */
	if ((AcpiGbl_FADT.Flags & ACPI_FADT_WBINVD) == 0)
		sc->sc_cstate[ACPI_STATE_C3].cs_method = 0;
	else {
		/*
		 * If WBINVD is present and functioning properly,
		 * flush all processor caches before entering C3.
		 */
		if ((AcpiGbl_FADT.Flags & ACPI_FADT_WBINVD_FLUSH) == 0)
			sc->sc_flags &= ~ACPICPU_FLAG_C_BM;
		else
			sc->sc_cstate[ACPI_STATE_C3].cs_method = 0;
	}
}

static int
acpicpu_cstate_latency(struct acpicpu_softc *sc)
{
	static const uint32_t cs_factor = 3;
	struct acpicpu_cstate *cs;
	int i;

	for (i = cs_state_max; i > 0; i--) {

		cs = &sc->sc_cstate[i];

		if (__predict_false(cs->cs_method == 0))
			continue;

		/*
		 * Choose a state if we have previously slept
		 * longer than the worst case latency of the
		 * state times an arbitrary multiplier.
		 */
		if (sc->sc_cstate_sleep > cs->cs_latency * cs_factor)
			return i;
	}

	return ACPI_STATE_C1;
}

/*
 * The main idle loop.
 */
void
acpicpu_cstate_idle(void)
{
        struct cpu_info *ci = curcpu();
	struct acpicpu_softc *sc;
	int state;

	if (__predict_false(ci->ci_want_resched) != 0)
		return;

	acpi_md_OsDisableInterrupt();

	KASSERT(acpicpu_sc != NULL);
	KASSERT(ci->ci_acpiid < maxcpus);

	sc = acpicpu_sc[ci->ci_acpiid];

	if (__predict_false(sc == NULL))
		goto halt;

	KASSERT(ci->ci_ilevel == IPL_NONE);
	KASSERT((sc->sc_flags & ACPICPU_FLAG_C) != 0);

	if (__predict_false(sc->sc_cold != false))
		goto halt;

	if (__predict_false(mutex_tryenter(&sc->sc_mtx) == 0))
		goto halt;

	mutex_exit(&sc->sc_mtx);
	state = acpicpu_cstate_latency(sc);

	/*
	 * Check for bus master activity. Note that particularly usb(4)
	 * causes high activity, which may prevent the use of C3 states.
	 */
	if ((sc->sc_cstate[state].cs_flags & ACPICPU_FLAG_C_BM_STS) != 0) {

		if (acpicpu_cstate_bm_check() != false)
			state--;

		if (__predict_false(sc->sc_cstate[state].cs_method == 0))
			state = ACPI_STATE_C1;
	}

	KASSERT(state != ACPI_STATE_C0);

	if (state != ACPI_STATE_C3) {
		acpicpu_cstate_idle_enter(sc, state);
		return;
	}

	/*
	 * On all recent (Intel) CPUs caches are shared
	 * by CPUs and bus master control is required to
	 * keep these coherent while in C3. Flushing the
	 * CPU caches is only the last resort.
	 */
	if ((sc->sc_flags & ACPICPU_FLAG_C_BM) == 0)
		ACPI_FLUSH_CPU_CACHE();

	/*
	 * Allow the bus master to request that any given
	 * CPU should return immediately to C0 from C3.
	 */
	if ((sc->sc_flags & ACPICPU_FLAG_C_BM) != 0)
		(void)AcpiWriteBitRegister(ACPI_BITREG_BUS_MASTER_RLD, 1);

	/*
	 * It may be necessary to disable bus master arbitration
	 * to ensure that bus master cycles do not occur while
	 * sleeping in C3 (see ACPI 4.0, section 8.1.4).
	 */
	if ((sc->sc_flags & ACPICPU_FLAG_C_ARB) != 0)
		(void)AcpiWriteBitRegister(ACPI_BITREG_ARB_DISABLE, 1);

	acpicpu_cstate_idle_enter(sc, state);

	/*
	 * Disable bus master wake and re-enable the arbiter.
	 */
	if ((sc->sc_flags & ACPICPU_FLAG_C_BM) != 0)
		(void)AcpiWriteBitRegister(ACPI_BITREG_BUS_MASTER_RLD, 0);

	if ((sc->sc_flags & ACPICPU_FLAG_C_ARB) != 0)
		(void)AcpiWriteBitRegister(ACPI_BITREG_ARB_DISABLE, 0);

	return;

halt:
	acpicpu_md_idle_enter(ACPICPU_C_STATE_HALT, ACPI_STATE_C1);
}

static void
acpicpu_cstate_idle_enter(struct acpicpu_softc *sc, int state)
{
	struct acpicpu_cstate *cs = &sc->sc_cstate[state];
	uint32_t end, start, val;

	start = acpitimer_read_fast(NULL);

	switch (cs->cs_method) {

	case ACPICPU_C_STATE_FFH:
	case ACPICPU_C_STATE_HALT:
		acpicpu_md_idle_enter(cs->cs_method, state);
		break;

	case ACPICPU_C_STATE_SYSIO:
		(void)AcpiOsReadPort(cs->cs_addr, &val, 8);
		break;

	default:
		acpicpu_md_idle_enter(ACPICPU_C_STATE_HALT, ACPI_STATE_C1);
		break;
	}

	cs->cs_evcnt.ev_count++;

	end = acpitimer_read_fast(NULL);
	sc->sc_cstate_sleep = hztoms(acpitimer_delta(end, start)) * 1000;

	acpi_md_OsEnableInterrupt();
}

static bool
acpicpu_cstate_bm_check(void)
{
	uint32_t val = 0;
	ACPI_STATUS rv;

	rv = AcpiReadBitRegister(ACPI_BITREG_BUS_MASTER_STATUS, &val);

	if (ACPI_FAILURE(rv) || val == 0)
		return false;

	(void)AcpiWriteBitRegister(ACPI_BITREG_BUS_MASTER_STATUS, 1);

	return true;
}
