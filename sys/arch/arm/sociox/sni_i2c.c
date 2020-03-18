/*	$NetBSD: sni_i2c.c,v 1.3 2020/03/18 07:49:01 nisimura Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

/*
 * Socionext SC2A11 SynQuacer I2C driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sni_i2c.c,v 1.3 2020/03/18 07:49:01 nisimura Exp $");

#ifdef I2CDEBUG
#define DPRINTF(args)	printf args
#else
#define DPRINTF(args)
#endif

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/i2c/i2cvar.h>

#include <dev/fdt/fdtvar.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

static int sniiic_fdt_match(device_t, struct cfdata *, void *);
static void sniiic_fdt_attach(device_t, device_t, void *);
static int sniiic_acpi_match(device_t, struct cfdata *, void *);
static void sniiic_acpi_attach(device_t, device_t, void *);

struct sniiic_softc {
	device_t		sc_dev;
	struct i2c_controller	sc_ic;
	device_t		sc_i2cdev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_iob;
	bus_size_t		sc_ios;
	void			*sc_ih;
	kmutex_t		sc_lock;
	kmutex_t		sc_mtx;
	kcondvar_t		sc_cv;
	int			sc_opflags;
	bool			sc_busy;
};

CFATTACH_DECL_NEW(sniiic_fdt, sizeof(struct sniiic_softc),
    sniiic_fdt_match, sniiic_fdt_attach, NULL, NULL);

CFATTACH_DECL_NEW(sniiic_acpi, sizeof(struct sniiic_softc),
    sniiic_acpi_match, sniiic_acpi_attach, NULL, NULL);

static int sniiic_acquire_bus(void *, int);
static void sniiic_release_bus(void *, int);
static int sniiic_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			size_t, void *, size_t, int);

static int sniiic_intr(void *);
static void sniiic_reset(struct sniiic_softc *);
static void sniiic_flush(struct sniiic_softc *);

static i2c_tag_t sniiic_get_tag(device_t);
static const struct fdtbus_i2c_controller_func sniiic_funcs = {
	.get_tag = sniiic_get_tag,
};

static int
sniiic_fdt_match(device_t parent, struct cfdata *match, void *aux)
{
	static const char * compatible[] = {
		"socionext,synquacer-i2c",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sniiic_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct sniiic_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	prop_dictionary_t dict = device_properties(self);
	const int phandle = faa->faa_phandle;
	bus_space_handle_t ioh;
	bus_addr_t addr;
	bus_size_t size;
	void *ih;
	char intrstr[128];
	_Bool disable;
	int error;

	prop_dictionary_get_bool(dict, "disable", &disable);
	if (disable) {
		aprint_naive(": disabled\n");
		aprint_normal(": disabled\n");
		return;
	}
	error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	error = bus_space_map(faa->faa_bst, addr, size, 0, &ioh);
	if (error) {
		aprint_error(": unable to map device\n");
		return;
	}
	error = fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr));
	if (error) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	aprint_naive(": I2C controller\n");
	aprint_normal(": I2C controller\n");

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	sc->sc_ioh = ioh;
	sc->sc_iob = addr;
	sc->sc_ios = size;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_NET);
	cv_init(&sc->sc_cv, "sniiic");
	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = sniiic_acquire_bus;
	sc->sc_ic.ic_release_bus = sniiic_release_bus;
	sc->sc_ic.ic_exec = sniiic_exec;

	ih = fdtbus_intr_establish(phandle, 0, IPL_NET, 0, sniiic_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto fail;
	}
	sc->sc_ih = ih;

	fdtbus_register_i2c_controller(self, phandle, &sniiic_funcs);
	fdtbus_attach_i2cbus(self, phandle, &sc->sc_ic, iicbus_print);

	return;
 fail:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	return;
}

static int
sniiic_acpi_match(device_t parent, struct cfdata *match, void *aux)
{
	static const char * compatible[] = {
		"SCX0003",
		NULL
	};
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;
	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
}

static void
sniiic_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct sniiic_softc * const sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	bus_space_handle_t ioh;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;
	void *ih;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	mem = acpi_res_mem(&res, 0);
	irq = acpi_res_irq(&res, 0);
	if (mem == NULL || irq == NULL) {
		aprint_error(": incomplete resources\n");
		return;
	}
	if (mem->ar_length == 0) {
		aprint_error(": zero length memory resource\n");
		return;
	}
	if (bus_space_map(aa->aa_memt, mem->ar_base, mem->ar_length, 0,
	    &ioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive(": I2C controller\n");
	aprint_normal(": I2C controller\n");

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_memt;
	sc->sc_ioh = ioh;
	sc->sc_iob = mem->ar_base;
	sc->sc_ios = mem->ar_length;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_NET);
	cv_init(&sc->sc_cv, "sniiic");
	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = sniiic_acquire_bus;
	sc->sc_ic.ic_release_bus = sniiic_release_bus;
	sc->sc_ic.ic_exec = sniiic_exec;

	ih = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)aa->aa_node->ad_handle,
	    IPL_BIO, false, sniiic_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto fail;
	}
	sc->sc_ih = ih;

	acpi_resource_cleanup(&res);

	return;
 fail:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	acpi_resource_cleanup(&res);
	return;
}

static int
sniiic_acquire_bus(void *opaque, int flags)
{
	struct sniiic_softc *sc = opaque;

	mutex_enter(&sc->sc_lock);
	while (sc->sc_busy)
		cv_wait(&sc->sc_cv, &sc->sc_lock);
	sc->sc_busy = true;
	mutex_exit(&sc->sc_lock);

	return 0;
}

static void
sniiic_release_bus(void *opaque, int flags)
{
	struct sniiic_softc *sc = opaque;

	mutex_enter(&sc->sc_lock);
	sc->sc_busy = false;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);
}

static int
sniiic_exec(void *opaque, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct sniiic_softc *sc = opaque;
	int err;

	DPRINTF(("sniic_exec: op 0x%x cmdlen %zd len %zd flags 0x%x\n",
	    op, cmdlen, len, flags));
	err = 0;
	/* AAA */
	goto done;
 done:
	if (err)
		sniiic_reset(sc);
	sniiic_flush(sc);
	DPRINTF(("sniiic_exec: done %d\n", err));
	return err;
}

static int
sniiic_intr(void *arg)
{
	struct sniiic_softc * const sc = arg;
	uint32_t stat = 0;

	mutex_enter(&sc->sc_mtx);
	DPRINTF(("sniiic_intr opflags=%#x\n", sc->sc_opflags));
	if ((sc->sc_opflags & I2C_F_POLL) == 0) {
		/* AAA */
		(void)stat;
		cv_broadcast(&sc->sc_cv);
	}
	mutex_exit(&sc->sc_mtx);
	DPRINTF(("sniiic_intr status 0x%x\n", stat));
	return 1;
}

static void
sniiic_reset(struct sniiic_softc *sc)
{
	/* AAA */
}

static void
sniiic_flush(struct sniiic_softc *sc)
{
	/* AAA */
}

static i2c_tag_t
sniiic_get_tag(device_t dev)
{
	struct sniiic_softc * const sc = device_private(dev);

	return &sc->sc_ic;
}
