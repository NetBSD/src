/*	$NetBSD: acpi_ec.c,v 1.108 2023/07/18 10:17:12 riastradh Exp $	*/

/*-
 * Copyright (c) 2007 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * The ACPI Embedded Controller (EC) driver serves two different purposes:
 * - read and write access from ASL, e.g. to read battery state
 * - notification of ASL of System Control Interrupts.
 *
 * Lock order:
 *	sc_access_mtx (serializes EC transactions -- read, write, or SCI)
 *	-> ACPI global lock (excludes other ACPI access during EC transaction)
 *	-> sc_mtx (serializes state machine transitions and waits)
 *
 * SCIs are processed in a kernel thread.
 *
 * Read and write requests spin around for a short time as many requests
 * can be handled instantly by the EC.  During normal processing interrupt
 * mode is used exclusively.  At boot and resume time interrupts are not
 * working and the handlers just busy loop.
 *
 * A callout is scheduled to compensate for missing interrupts on some
 * hardware.  If the EC doesn't process a request for 5s, it is most likely
 * in a wedged state.  No method to reset the EC is currently known.
 *
 * Special care has to be taken to not poll the EC in a busy loop without
 * delay.  This can prevent processing of Power Button events. At least some
 * Lenovo Thinkpads seem to be implement the Power Button Override in the EC
 * and the only option to recover on those models is to cut off all power.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_ec.c,v 1.108 2023/07/18 10:17:12 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_acpi_ec.h"
#endif

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_ecvar.h>

#define _COMPONENT          ACPI_EC_COMPONENT
ACPI_MODULE_NAME            ("acpi_ec")

/* Maximum time to wait for global ACPI lock in ms */
#define	EC_LOCK_TIMEOUT		5

/* Maximum time to poll for completion of a command  in ms */
#define	EC_POLL_TIMEOUT		5

/* Maximum time to give a single EC command in s */
#define EC_CMD_TIMEOUT		10

/* From ACPI 3.0b, chapter 12.3 */
#define EC_COMMAND_READ		0x80
#define	EC_COMMAND_WRITE	0x81
#define	EC_COMMAND_BURST_EN	0x82
#define	EC_COMMAND_BURST_DIS	0x83
#define	EC_COMMAND_QUERY	0x84

/* From ACPI 3.0b, chapter 12.2.1 */
#define	EC_STATUS_OBF		0x01
#define	EC_STATUS_IBF		0x02
#define	EC_STATUS_CMD		0x08
#define	EC_STATUS_BURST		0x10
#define	EC_STATUS_SCI		0x20
#define	EC_STATUS_SMI		0x40

#define	EC_STATUS_FMT							      \
	"\x10\10IGN7\7SMI\6SCI\5BURST\4CMD\3IGN2\2IBF\1OBF"

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "PNP0C09" },
	DEVICE_COMPAT_EOL
};

#define	EC_STATE_ENUM(F)						      \
	F(EC_STATE_QUERY, "QUERY")					      \
	F(EC_STATE_QUERY_VAL, "QUERY_VAL")				      \
	F(EC_STATE_READ, "READ")					      \
	F(EC_STATE_READ_ADDR, "READ_ADDR")				      \
	F(EC_STATE_READ_VAL, "READ_VAL")				      \
	F(EC_STATE_WRITE, "WRITE")					      \
	F(EC_STATE_WRITE_ADDR, "WRITE_ADDR")				      \
	F(EC_STATE_WRITE_VAL, "WRITE_VAL")				      \
	F(EC_STATE_FREE, "FREE")					      \

enum ec_state_t {
#define	F(N, S)	N,
	EC_STATE_ENUM(F)
#undef F
};

#ifdef ACPIEC_DEBUG
static const char *const acpiec_state_names[] = {
#define F(N, S)	[N] = S,
	EC_STATE_ENUM(F)
#undef F
};
#endif

struct acpiec_softc {
	device_t sc_dev;

	ACPI_HANDLE sc_ech;

	ACPI_HANDLE sc_gpeh;
	uint8_t sc_gpebit;

	bus_space_tag_t sc_data_st;
	bus_space_handle_t sc_data_sh;

	bus_space_tag_t sc_csr_st;
	bus_space_handle_t sc_csr_sh;

	bool sc_need_global_lock;
	uint32_t sc_global_lock;

	kmutex_t sc_mtx, sc_access_mtx;
	kcondvar_t sc_cv, sc_cv_sci;
	enum ec_state_t sc_state;
	bool sc_got_sci;
	callout_t sc_pseudo_intr;

	uint8_t sc_cur_addr, sc_cur_val;
};

#ifdef ACPIEC_DEBUG

#define	ACPIEC_DEBUG_ENUM(F)						      \
	F(ACPIEC_DEBUG_REG, "REG")					      \
	F(ACPIEC_DEBUG_RW, "RW")					      \
	F(ACPIEC_DEBUG_QUERY, "QUERY")					      \
	F(ACPIEC_DEBUG_TRANSITION, "TRANSITION")			      \
	F(ACPIEC_DEBUG_INTR, "INTR")					      \

enum {
#define	F(N, S)	N,
	ACPIEC_DEBUG_ENUM(F)
#undef F
};

static const char *const acpiec_debug_names[] = {
#define	F(N, S)	[N] = S,
	ACPIEC_DEBUG_ENUM(F)
#undef F
};

int acpiec_debug = ACPIEC_DEBUG;

#define	DPRINTF(n, sc, fmt, ...) do					      \
{									      \
	if (acpiec_debug & __BIT(n)) {					      \
		char dprintbuf[16];					      \
		const char *state;					      \
									      \
		/* paranoia */						      \
		if ((sc)->sc_state < __arraycount(acpiec_state_names)) {      \
			state = acpiec_state_names[(sc)->sc_state];	      \
		} else {						      \
			snprintf(dprintbuf, sizeof(dprintbuf), "0x%x",	      \
			    (sc)->sc_state);				      \
			state = dprintbuf;				      \
		}							      \
									      \
		device_printf((sc)->sc_dev, "(%s) [%s] "fmt,		      \
		    acpiec_debug_names[n], state, ##__VA_ARGS__);	      \
	}								      \
} while (0)

#else

#define	DPRINTF(n, sc, fmt, ...)	__nothing

#endif

static int acpiecdt_match(device_t, cfdata_t, void *);
static void acpiecdt_attach(device_t, device_t, void *);

static int acpiec_match(device_t, cfdata_t, void *);
static void acpiec_attach(device_t, device_t, void *);

static void acpiec_common_attach(device_t, device_t, ACPI_HANDLE,
    bus_space_tag_t, bus_addr_t, bus_space_tag_t, bus_addr_t,
    ACPI_HANDLE, uint8_t);

static bool acpiec_suspend(device_t, const pmf_qual_t *);
static bool acpiec_resume(device_t, const pmf_qual_t *);
static bool acpiec_shutdown(device_t, int);

static bool acpiec_parse_gpe_package(device_t, ACPI_HANDLE,
    ACPI_HANDLE *, uint8_t *);

static void acpiec_callout(void *);
static void acpiec_gpe_query(void *);
static uint32_t acpiec_gpe_handler(ACPI_HANDLE, uint32_t, void *);
static ACPI_STATUS acpiec_space_setup(ACPI_HANDLE, uint32_t, void *, void **);
static ACPI_STATUS acpiec_space_handler(uint32_t, ACPI_PHYSICAL_ADDRESS,
    uint32_t, ACPI_INTEGER *, void *, void *);

static void acpiec_gpe_state_machine(struct acpiec_softc *);

CFATTACH_DECL_NEW(acpiec, sizeof(struct acpiec_softc),
    acpiec_match, acpiec_attach, NULL, NULL);

CFATTACH_DECL_NEW(acpiecdt, sizeof(struct acpiec_softc),
    acpiecdt_match, acpiecdt_attach, NULL, NULL);

static device_t ec_singleton = NULL;
static bool acpiec_cold = false;

static bool
acpiecdt_find(device_t parent, ACPI_HANDLE *ec_handle,
    bus_addr_t *cmd_reg, bus_addr_t *data_reg, uint8_t *gpebit)
{
	ACPI_TABLE_ECDT *ecdt;
	ACPI_STATUS rv;

	rv = AcpiGetTable(ACPI_SIG_ECDT, 1, (ACPI_TABLE_HEADER **)&ecdt);
	if (ACPI_FAILURE(rv))
		return false;

	if (ecdt->Control.BitWidth != 8 || ecdt->Data.BitWidth != 8) {
		aprint_error_dev(parent,
		    "ECDT register width invalid (%u/%u)\n",
		    ecdt->Control.BitWidth, ecdt->Data.BitWidth);
		return false;
	}

	rv = AcpiGetHandle(ACPI_ROOT_OBJECT, ecdt->Id, ec_handle);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(parent,
		    "failed to look up EC object %s: %s\n",
		    ecdt->Id, AcpiFormatException(rv));
		return false;
	}

	*cmd_reg = ecdt->Control.Address;
	*data_reg = ecdt->Data.Address;
	*gpebit = ecdt->Gpe;

	return true;
}

static int
acpiecdt_match(device_t parent, cfdata_t match, void *aux)
{
	ACPI_HANDLE ec_handle;
	bus_addr_t cmd_reg, data_reg;
	uint8_t gpebit;

	if (acpiecdt_find(parent, &ec_handle, &cmd_reg, &data_reg, &gpebit))
		return 1;
	else
		return 0;
}

static void
acpiecdt_attach(device_t parent, device_t self, void *aux)
{
	struct acpibus_attach_args *aa = aux;
	ACPI_HANDLE ec_handle;
	bus_addr_t cmd_reg, data_reg;
	uint8_t gpebit;

	if (!acpiecdt_find(parent, &ec_handle, &cmd_reg, &data_reg, &gpebit))
		panic("ECDT disappeared");

	aprint_naive("\n");
	aprint_normal(": ACPI Embedded Controller via ECDT\n");

	acpiec_common_attach(parent, self, ec_handle, aa->aa_iot, cmd_reg,
	    aa->aa_iot, data_reg, NULL, gpebit);
}

static int
acpiec_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
acpiec_attach(device_t parent, device_t self, void *aux)
{
	struct acpi_attach_args *aa = aux;
	struct acpi_resources ec_res;
	struct acpi_io *io0, *io1;
	ACPI_HANDLE gpe_handle;
	uint8_t gpebit;
	ACPI_STATUS rv;

	if (ec_singleton != NULL) {
		aprint_naive(": using %s\n", device_xname(ec_singleton));
		aprint_normal(": using %s\n", device_xname(ec_singleton));
		goto fail0;
	}

	if (!acpi_device_present(aa->aa_node->ad_handle)) {
		aprint_normal(": not present\n");
		goto fail0;
	}

	if (!acpiec_parse_gpe_package(self, aa->aa_node->ad_handle,
				      &gpe_handle, &gpebit))
		goto fail0;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS",
	    &ec_res, &acpi_resource_parse_ops_default);
	if (rv != AE_OK) {
		aprint_error_dev(self, "resource parsing failed: %s\n",
		    AcpiFormatException(rv));
		goto fail0;
	}

	if ((io0 = acpi_res_io(&ec_res, 0)) == NULL) {
		aprint_error_dev(self, "no data register resource\n");
		goto fail1;
	}
	if ((io1 = acpi_res_io(&ec_res, 1)) == NULL) {
		aprint_error_dev(self, "no CSR register resource\n");
		goto fail1;
	}

	acpiec_common_attach(parent, self, aa->aa_node->ad_handle,
	    aa->aa_iot, io1->ar_base, aa->aa_iot, io0->ar_base,
	    gpe_handle, gpebit);

	acpi_resource_cleanup(&ec_res);
	return;

fail1:	acpi_resource_cleanup(&ec_res);
fail0:	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static void
acpiec_common_attach(device_t parent, device_t self,
    ACPI_HANDLE ec_handle, bus_space_tag_t cmdt, bus_addr_t cmd_reg,
    bus_space_tag_t datat, bus_addr_t data_reg,
    ACPI_HANDLE gpe_handle, uint8_t gpebit)
{
	struct acpiec_softc *sc = device_private(self);
	ACPI_STATUS rv;
	ACPI_INTEGER val;

	sc->sc_dev = self;

	sc->sc_csr_st = cmdt;
	sc->sc_data_st = datat;

	sc->sc_ech = ec_handle;
	sc->sc_gpeh = gpe_handle;
	sc->sc_gpebit = gpebit;

	sc->sc_state = EC_STATE_FREE;
	mutex_init(&sc->sc_mtx, MUTEX_DRIVER, IPL_TTY);
	mutex_init(&sc->sc_access_mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_cv, "eccv");
	cv_init(&sc->sc_cv_sci, "ecsci");

	if (bus_space_map(sc->sc_data_st, data_reg, 1, 0,
	    &sc->sc_data_sh) != 0) {
		aprint_error_dev(self, "unable to map data register\n");
		return;
	}

	if (bus_space_map(sc->sc_csr_st, cmd_reg, 1, 0, &sc->sc_csr_sh) != 0) {
		aprint_error_dev(self, "unable to map CSR register\n");
		goto post_data_map;
	}

	rv = acpi_eval_integer(sc->sc_ech, "_GLK", &val);
	if (rv == AE_OK) {
		sc->sc_need_global_lock = val != 0;
	} else if (rv != AE_NOT_FOUND) {
		aprint_error_dev(self, "unable to evaluate _GLK: %s\n",
		    AcpiFormatException(rv));
		goto post_csr_map;
	} else {
		sc->sc_need_global_lock = false;
	}
	if (sc->sc_need_global_lock)
		aprint_normal_dev(self, "using global ACPI lock\n");

	callout_init(&sc->sc_pseudo_intr, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_pseudo_intr, acpiec_callout, sc);

	rv = AcpiInstallAddressSpaceHandler(sc->sc_ech, ACPI_ADR_SPACE_EC,
	    acpiec_space_handler, acpiec_space_setup, sc);
	if (rv != AE_OK) {
		aprint_error_dev(self,
		    "unable to install address space handler: %s\n",
		    AcpiFormatException(rv));
		goto post_csr_map;
	}

	rv = AcpiInstallGpeHandler(sc->sc_gpeh, sc->sc_gpebit,
	    ACPI_GPE_EDGE_TRIGGERED, acpiec_gpe_handler, sc);
	if (rv != AE_OK) {
		aprint_error_dev(self, "unable to install GPE handler: %s\n",
		    AcpiFormatException(rv));
		goto post_csr_map;
	}

	rv = AcpiEnableGpe(sc->sc_gpeh, sc->sc_gpebit);
	if (rv != AE_OK) {
		aprint_error_dev(self, "unable to enable GPE: %s\n",
		    AcpiFormatException(rv));
		goto post_csr_map;
	}

	if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL, acpiec_gpe_query,
		sc, NULL, "acpiec sci thread")) {
		aprint_error_dev(self, "unable to create query kthread\n");
		goto post_csr_map;
	}

	ec_singleton = self;

	if (!pmf_device_register1(self, acpiec_suspend, acpiec_resume,
	    acpiec_shutdown))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;

post_csr_map:
	(void)AcpiRemoveGpeHandler(sc->sc_gpeh, sc->sc_gpebit,
	    acpiec_gpe_handler);
	(void)AcpiRemoveAddressSpaceHandler(sc->sc_ech,
	    ACPI_ADR_SPACE_EC, acpiec_space_handler);
	bus_space_unmap(sc->sc_csr_st, sc->sc_csr_sh, 1);
post_data_map:
	bus_space_unmap(sc->sc_data_st, sc->sc_data_sh, 1);
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static bool
acpiec_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct acpiec_softc *sc = device_private(dv);

	/*
	 * XXX This looks bad because acpiec_cold is global and
	 * sc->sc_mtx doesn't look like it's global, but we can have
	 * only one acpiec(4) device anyway.  Maybe acpiec_cold should
	 * live in the softc to make this look less bad?
	 *
	 * XXX Should this block read/write/query transactions until
	 * resume?
	 *
	 * XXX Should this interrupt existing transactions to make them
	 * fail promptly or restart on resume?
	 */
	mutex_enter(&sc->sc_mtx);
	acpiec_cold = true;
	mutex_exit(&sc->sc_mtx);

	return true;
}

static bool
acpiec_resume(device_t dv, const pmf_qual_t *qual)
{
	struct acpiec_softc *sc = device_private(dv);

	mutex_enter(&sc->sc_mtx);
	acpiec_cold = false;
	mutex_exit(&sc->sc_mtx);

	return true;
}

static bool
acpiec_shutdown(device_t dv, int how)
{
	struct acpiec_softc *sc = device_private(dv);

	mutex_enter(&sc->sc_mtx);
	acpiec_cold = true;
	mutex_exit(&sc->sc_mtx);

	return true;
}

static bool
acpiec_parse_gpe_package(device_t self, ACPI_HANDLE ec_handle,
    ACPI_HANDLE *gpe_handle, uint8_t *gpebit)
{
	ACPI_BUFFER buf;
	ACPI_OBJECT *p, *c;
	ACPI_STATUS rv;

	rv = acpi_eval_struct(ec_handle, "_GPE", &buf);
	if (rv != AE_OK) {
		aprint_error_dev(self, "unable to evaluate _GPE: %s\n",
		    AcpiFormatException(rv));
		return false;
	}

	p = buf.Pointer;

	if (p->Type == ACPI_TYPE_INTEGER) {
		*gpe_handle = NULL;
		*gpebit = p->Integer.Value;
		ACPI_FREE(p);
		return true;
	}

	if (p->Type != ACPI_TYPE_PACKAGE) {
		aprint_error_dev(self, "_GPE is neither integer nor package\n");
		ACPI_FREE(p);
		return false;
	}

	if (p->Package.Count != 2) {
		aprint_error_dev(self,
		    "_GPE package does not contain 2 elements\n");
		ACPI_FREE(p);
		return false;
	}

	c = &p->Package.Elements[0];
	rv = acpi_eval_reference_handle(c, gpe_handle);

	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "failed to evaluate _GPE handle\n");
		ACPI_FREE(p);
		return false;
	}

	c = &p->Package.Elements[1];

	if (c->Type != ACPI_TYPE_INTEGER) {
		aprint_error_dev(self,
		    "_GPE package needs integer as 2nd field\n");
		ACPI_FREE(p);
		return false;
	}
	*gpebit = c->Integer.Value;
	ACPI_FREE(p);
	return true;
}

static uint8_t
acpiec_read_data(struct acpiec_softc *sc)
{
	uint8_t x;

	KASSERT(mutex_owned(&sc->sc_mtx));

	x = bus_space_read_1(sc->sc_data_st, sc->sc_data_sh, 0);
	DPRINTF(ACPIEC_DEBUG_REG, sc, "read data=0x%"PRIx8"\n", x);

	return x;
}

static void
acpiec_write_data(struct acpiec_softc *sc, uint8_t val)
{

	KASSERT(mutex_owned(&sc->sc_mtx));

	DPRINTF(ACPIEC_DEBUG_REG, sc, "write data=0x%"PRIx8"\n", val);
	bus_space_write_1(sc->sc_data_st, sc->sc_data_sh, 0, val);
}

static uint8_t
acpiec_read_status(struct acpiec_softc *sc)
{
	uint8_t x;

	KASSERT(mutex_owned(&sc->sc_mtx));

	x = bus_space_read_1(sc->sc_csr_st, sc->sc_csr_sh, 0);
	DPRINTF(ACPIEC_DEBUG_REG, sc, "read status=0x%"PRIx8"\n", x);

	return x;
}

static void
acpiec_write_command(struct acpiec_softc *sc, uint8_t cmd)
{

	KASSERT(mutex_owned(&sc->sc_mtx));

	DPRINTF(ACPIEC_DEBUG_REG, sc, "write command=0x%"PRIx8"\n", cmd);
	bus_space_write_1(sc->sc_csr_st, sc->sc_csr_sh, 0, cmd);
}

static ACPI_STATUS
acpiec_space_setup(ACPI_HANDLE region, uint32_t func, void *arg,
    void **region_arg)
{

	if (func == ACPI_REGION_DEACTIVATE)
		*region_arg = NULL;
	else
		*region_arg = arg;

	return AE_OK;
}

static void
acpiec_lock(struct acpiec_softc *sc)
{
	ACPI_STATUS rv;

	mutex_enter(&sc->sc_access_mtx);

	if (sc->sc_need_global_lock) {
		rv = AcpiAcquireGlobalLock(EC_LOCK_TIMEOUT,
		    &sc->sc_global_lock);
		if (rv != AE_OK) {
			aprint_error_dev(sc->sc_dev,
			    "failed to acquire global lock: %s\n",
			    AcpiFormatException(rv));
			return;
		}
	}
}

static void
acpiec_unlock(struct acpiec_softc *sc)
{
	ACPI_STATUS rv;

	if (sc->sc_need_global_lock) {
		rv = AcpiReleaseGlobalLock(sc->sc_global_lock);
		if (rv != AE_OK) {
			aprint_error_dev(sc->sc_dev,
			    "failed to release global lock: %s\n",
			    AcpiFormatException(rv));
		}
	}
	mutex_exit(&sc->sc_access_mtx);
}

static ACPI_STATUS
acpiec_wait_timeout(struct acpiec_softc *sc)
{
	device_t dv = sc->sc_dev;
	int i;

	for (i = 0; i < EC_POLL_TIMEOUT; ++i) {
		acpiec_gpe_state_machine(sc);
		if (sc->sc_state == EC_STATE_FREE)
			return AE_OK;
		delay(1);
	}

	DPRINTF(ACPIEC_DEBUG_RW, sc, "SCI polling timeout\n");
	if (cold || acpiec_cold) {
		int timeo = 1000 * EC_CMD_TIMEOUT;

		while (sc->sc_state != EC_STATE_FREE && timeo-- > 0) {
			delay(1000);
			acpiec_gpe_state_machine(sc);
		}
		if (sc->sc_state != EC_STATE_FREE) {
			aprint_error_dev(dv, "command timed out, state %d\n",
			    sc->sc_state);
			return AE_ERROR;
		}
	} else {
		const unsigned deadline = getticks() + EC_CMD_TIMEOUT*hz;
		unsigned delta;

		while (sc->sc_state != EC_STATE_FREE &&
		    (delta = deadline - getticks()) < INT_MAX)
			(void)cv_timedwait(&sc->sc_cv, &sc->sc_mtx, delta);
		if (sc->sc_state != EC_STATE_FREE) {
			aprint_error_dev(dv,
			    "command takes over %d sec...\n",
			    EC_CMD_TIMEOUT);
			return AE_ERROR;
		}
	}

	return AE_OK;
}

static ACPI_STATUS
acpiec_read(struct acpiec_softc *sc, uint8_t addr, uint8_t *val)
{
	ACPI_STATUS rv;

	acpiec_lock(sc);
	mutex_enter(&sc->sc_mtx);

	DPRINTF(ACPIEC_DEBUG_RW, sc,
	    "pid %ld %s, lid %ld%s%s: read addr 0x%"PRIx8"\n",
	    (long)curproc->p_pid, curproc->p_comm,
	    (long)curlwp->l_lid, curlwp->l_name ? " " : "",
	    curlwp->l_name ? curlwp->l_name : "",
	    addr);

	KASSERT(sc->sc_state == EC_STATE_FREE);

	sc->sc_cur_addr = addr;
	sc->sc_state = EC_STATE_READ;

	rv = acpiec_wait_timeout(sc);
	if (ACPI_FAILURE(rv))
		goto out;

	DPRINTF(ACPIEC_DEBUG_RW, sc,
	    "pid %ld %s, lid %ld%s%s: read addr 0x%"PRIx8": 0x%"PRIx8"\n",
	    (long)curproc->p_pid, curproc->p_comm,
	    (long)curlwp->l_lid, curlwp->l_name ? " " : "",
	    curlwp->l_name ? curlwp->l_name : "",
	    addr, sc->sc_cur_val);

	*val = sc->sc_cur_val;

out:	mutex_exit(&sc->sc_mtx);
	acpiec_unlock(sc);
	return rv;
}

static ACPI_STATUS
acpiec_write(struct acpiec_softc *sc, uint8_t addr, uint8_t val)
{
	ACPI_STATUS rv;

	acpiec_lock(sc);
	mutex_enter(&sc->sc_mtx);

	DPRINTF(ACPIEC_DEBUG_RW, sc,
	    "pid %ld %s, lid %ld%s%s write addr 0x%"PRIx8": 0x%"PRIx8"\n",
	    (long)curproc->p_pid, curproc->p_comm,
	    (long)curlwp->l_lid, curlwp->l_name ? " " : "",
	    curlwp->l_name ? curlwp->l_name : "",
	    addr, val);

	KASSERT(sc->sc_state == EC_STATE_FREE);

	sc->sc_cur_addr = addr;
	sc->sc_cur_val = val;
	sc->sc_state = EC_STATE_WRITE;

	rv = acpiec_wait_timeout(sc);
	if (ACPI_FAILURE(rv))
		goto out;

	DPRINTF(ACPIEC_DEBUG_RW, sc,
	    "pid %ld %s, lid %ld%s%s: write addr 0x%"PRIx8": 0x%"PRIx8
	    " done\n",
	    (long)curproc->p_pid, curproc->p_comm,
	    (long)curlwp->l_lid, curlwp->l_name ? " " : "",
	    curlwp->l_name ? curlwp->l_name : "",
	    addr, val);

out:	mutex_exit(&sc->sc_mtx);
	acpiec_unlock(sc);
	return rv;
}

/*
 * acpiec_space_handler(func, paddr, bitwidth, value, arg, region_arg)
 *
 *	Transfer bitwidth/8 bytes of data between paddr and *value:
 *	from paddr to *value when func is ACPI_READ, and the other way
 *	when func is ACPI_WRITE.  arg is the acpiec_softc pointer.
 *	region_arg is ignored (XXX why? determined by
 *	acpiec_space_setup but never used by anything that I can see).
 *
 *	The caller always provides storage at *value large enough for
 *	an ACPI_INTEGER object, i.e., a 64-bit integer.  However,
 *	bitwidth may be larger; in this case the caller provides larger
 *	storage at *value, e.g. 128 bits as documented in
 *	<https://gnats.netbsd.org/55206>.
 *
 *	On reads, this fully initializes one ACPI_INTEGER's worth of
 *	data at *value, even if bitwidth < 64.  The integer is
 *	interpreted in host byte order; in other words, bytes of data
 *	are transferred in order between paddr and (uint8_t *)value.
 *	The transfer is not atomic; it may go byte-by-byte.
 *
 *	XXX This only really makes sense on little-endian systems.
 *	E.g., thinkpad_acpi.c assumes that a single byte is transferred
 *	in the low-order bits of the result.  A big-endian system could
 *	read a 64-bit integer in big-endian (and it did for a while!),
 *	but what should it do for larger reads?  Unclear!
 *
 *	XXX It's not clear whether the object at *value is always
 *	_aligned_ adequately for an ACPI_INTEGER object.  Currently it
 *	always is as long as malloc, used by AcpiOsAllocate, returns
 *	64-bit-aligned data.
 */
static ACPI_STATUS
acpiec_space_handler(uint32_t func, ACPI_PHYSICAL_ADDRESS paddr,
    uint32_t width, ACPI_INTEGER *value, void *arg, void *region_arg)
{
	struct acpiec_softc *sc = arg;
	ACPI_STATUS rv;
	uint8_t addr, *buf;
	unsigned int i;

	if (paddr > 0xff || width % 8 != 0 ||
	    value == NULL || arg == NULL || paddr + width / 8 > 0x100)
		return AE_BAD_PARAMETER;

	addr = paddr;
	buf = (uint8_t *)value;

	rv = AE_OK;

	switch (func) {
	case ACPI_READ:
		for (i = 0; i < width; i += 8, ++addr, ++buf) {
			rv = acpiec_read(sc, addr, buf);
			if (rv != AE_OK)
				break;
		}
		/*
		 * Make sure to fully initialize at least an
		 * ACPI_INTEGER-sized object.
		 */
		for (; i < sizeof(*value)*8; i += 8, ++buf)
			*buf = 0;
		break;
	case ACPI_WRITE:
		for (i = 0; i < width; i += 8, ++addr, ++buf) {
			rv = acpiec_write(sc, addr, *buf);
			if (rv != AE_OK)
				break;
		}
		break;
	default:
		aprint_error_dev(sc->sc_dev,
		    "invalid Address Space function called: %x\n",
		    (unsigned int)func);
		return AE_BAD_PARAMETER;
	}

	return rv;
}

static void
acpiec_wait(struct acpiec_softc *sc)
{
	int i;

	/*
	 * First, attempt to get the query by polling.
	 */
	for (i = 0; i < EC_POLL_TIMEOUT; ++i) {
		acpiec_gpe_state_machine(sc);
		if (sc->sc_state == EC_STATE_FREE)
			return;
		delay(1);
	}

	/*
	 * Polling timed out.  Try waiting for interrupts -- either GPE
	 * interrupts, or periodic callouts in case GPE interrupts are
	 * broken.
	 */
	DPRINTF(ACPIEC_DEBUG_QUERY, sc, "SCI polling timeout\n");
	while (sc->sc_state != EC_STATE_FREE)
		cv_wait(&sc->sc_cv, &sc->sc_mtx);
}

static void
acpiec_gpe_query(void *arg)
{
	struct acpiec_softc *sc = arg;
	uint8_t reg;
	char qxx[5];
	ACPI_STATUS rv;

loop:
	/*
	 * Wait until the EC sends an SCI requesting a query.
	 */
	mutex_enter(&sc->sc_mtx);
	while (!sc->sc_got_sci)
		cv_wait(&sc->sc_cv_sci, &sc->sc_mtx);
	DPRINTF(ACPIEC_DEBUG_QUERY, sc, "SCI query requested\n");
	mutex_exit(&sc->sc_mtx);

	/*
	 * EC wants to submit a query to us.  Exclude concurrent reads
	 * and writes while we handle it.
	 */
	acpiec_lock(sc);
	mutex_enter(&sc->sc_mtx);

	DPRINTF(ACPIEC_DEBUG_QUERY, sc, "SCI query\n");

	KASSERT(sc->sc_state == EC_STATE_FREE);

	/* The Query command can always be issued, so be defensive here. */
	KASSERT(sc->sc_got_sci);
	sc->sc_got_sci = false;
	sc->sc_state = EC_STATE_QUERY;

	acpiec_wait(sc);

	reg = sc->sc_cur_val;
	DPRINTF(ACPIEC_DEBUG_QUERY, sc, "SCI query: 0x%"PRIx8"\n", reg);

	mutex_exit(&sc->sc_mtx);
	acpiec_unlock(sc);

	if (reg == 0)
		goto loop; /* Spurious query result */

	/*
	 * Evaluate _Qxx to respond to the controller.
	 */
	snprintf(qxx, sizeof(qxx), "_Q%02X", (unsigned int)reg);
	rv = AcpiEvaluateObject(sc->sc_ech, qxx, NULL, NULL);
	if (rv != AE_OK && rv != AE_NOT_FOUND) {
		aprint_error_dev(sc->sc_dev, "GPE query method %s failed: %s",
		    qxx, AcpiFormatException(rv));
	}

	goto loop;
}

static void
acpiec_gpe_state_machine(struct acpiec_softc *sc)
{
	uint8_t reg;

	KASSERT(mutex_owned(&sc->sc_mtx));

	reg = acpiec_read_status(sc);

#ifdef ACPIEC_DEBUG
	if (acpiec_debug & __BIT(ACPIEC_DEBUG_TRANSITION)) {
		char buf[128];

		snprintb(buf, sizeof(buf), EC_STATUS_FMT, reg);
		DPRINTF(ACPIEC_DEBUG_TRANSITION, sc, "%s\n", buf);
	}
#endif

	switch (sc->sc_state) {
	case EC_STATE_QUERY:
		if ((reg & EC_STATUS_IBF) != 0)
			break; /* Nothing of interest here. */
		acpiec_write_command(sc, EC_COMMAND_QUERY);
		sc->sc_state = EC_STATE_QUERY_VAL;
		break;

	case EC_STATE_QUERY_VAL:
		if ((reg & EC_STATUS_OBF) == 0)
			break; /* Nothing of interest here. */
		sc->sc_cur_val = acpiec_read_data(sc);
		sc->sc_state = EC_STATE_FREE;
		break;

	case EC_STATE_READ:
		if ((reg & EC_STATUS_IBF) != 0)
			break; /* Nothing of interest here. */
		acpiec_write_command(sc, EC_COMMAND_READ);
		sc->sc_state = EC_STATE_READ_ADDR;
		break;

	case EC_STATE_READ_ADDR:
		if ((reg & EC_STATUS_IBF) != 0)
			break; /* Nothing of interest here. */
		acpiec_write_data(sc, sc->sc_cur_addr);
		sc->sc_state = EC_STATE_READ_VAL;
		break;

	case EC_STATE_READ_VAL:
		if ((reg & EC_STATUS_OBF) == 0)
			break; /* Nothing of interest here. */
		sc->sc_cur_val = acpiec_read_data(sc);
		sc->sc_state = EC_STATE_FREE;
		break;

	case EC_STATE_WRITE:
		if ((reg & EC_STATUS_IBF) != 0)
			break; /* Nothing of interest here. */
		acpiec_write_command(sc, EC_COMMAND_WRITE);
		sc->sc_state = EC_STATE_WRITE_ADDR;
		break;

	case EC_STATE_WRITE_ADDR:
		if ((reg & EC_STATUS_IBF) != 0)
			break; /* Nothing of interest here. */
		acpiec_write_data(sc, sc->sc_cur_addr);
		sc->sc_state = EC_STATE_WRITE_VAL;
		break;

	case EC_STATE_WRITE_VAL:
		if ((reg & EC_STATUS_IBF) != 0)
			break; /* Nothing of interest here. */
		acpiec_write_data(sc, sc->sc_cur_val);
		sc->sc_state = EC_STATE_FREE;
		break;

	case EC_STATE_FREE:
		break;

	default:
		panic("invalid state");
	}

	/*
	 * If we are not in a transaction, wake anyone waiting to start
	 * one.  If an SCI was requested, notify the SCI thread that it
	 * needs to handle the SCI.
	 */
	if (sc->sc_state == EC_STATE_FREE) {
		cv_signal(&sc->sc_cv);
		if (reg & EC_STATUS_SCI) {
			DPRINTF(ACPIEC_DEBUG_TRANSITION, sc,
			    "wake SCI thread\n");
			sc->sc_got_sci = true;
			cv_signal(&sc->sc_cv_sci);
		}
	}

	/*
	 * In case GPE interrupts are broken, poll once per tick for EC
	 * status updates while a transaction is still pending.
	 */
	if (sc->sc_state != EC_STATE_FREE) {
		DPRINTF(ACPIEC_DEBUG_INTR, sc, "schedule callout\n");
		callout_schedule(&sc->sc_pseudo_intr, 1);
	}

	DPRINTF(ACPIEC_DEBUG_TRANSITION, sc, "return\n");
}

static void
acpiec_callout(void *arg)
{
	struct acpiec_softc *sc = arg;

	mutex_enter(&sc->sc_mtx);
	DPRINTF(ACPIEC_DEBUG_INTR, sc, "callout\n");
	acpiec_gpe_state_machine(sc);
	mutex_exit(&sc->sc_mtx);
}

static uint32_t
acpiec_gpe_handler(ACPI_HANDLE hdl, uint32_t gpebit, void *arg)
{
	struct acpiec_softc *sc = arg;

	mutex_enter(&sc->sc_mtx);
	DPRINTF(ACPIEC_DEBUG_INTR, sc, "GPE\n");
	acpiec_gpe_state_machine(sc);
	mutex_exit(&sc->sc_mtx);

	return ACPI_INTERRUPT_HANDLED | ACPI_REENABLE_GPE;
}

ACPI_STATUS
acpiec_bus_read(device_t dv, u_int addr, ACPI_INTEGER *val, int width)
{
	struct acpiec_softc *sc = device_private(dv);

	return acpiec_space_handler(ACPI_READ, addr, width * 8, val, sc, NULL);
}

ACPI_STATUS
acpiec_bus_write(device_t dv, u_int addr, ACPI_INTEGER val, int width)
{
	struct acpiec_softc *sc = device_private(dv);

	return acpiec_space_handler(ACPI_WRITE, addr, width * 8, &val, sc,
	    NULL);
}

ACPI_HANDLE
acpiec_get_handle(device_t dv)
{
	struct acpiec_softc *sc = device_private(dv);

	return sc->sc_ech;
}
