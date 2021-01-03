/*	$NetBSD: sni_i2c.c,v 1.8.2.1 2021/01/03 16:34:53 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sni_i2c.c,v 1.8.2.1 2021/01/03 16:34:53 thorpej Exp $");

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
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_iob;
	bus_size_t		sc_ios;
	void			*sc_ih;
	kmutex_t		sc_lock;
	kmutex_t		sc_mtx;
	kcondvar_t		sc_cv;
	volatile bool		sc_busy;
	int			sc_phandle;
};

CFATTACH_DECL_NEW(sniiic_fdt, sizeof(struct sniiic_softc),
    sniiic_fdt_match, sniiic_fdt_attach, NULL, NULL);

CFATTACH_DECL_NEW(sniiic_acpi, sizeof(struct sniiic_softc),
    sniiic_acpi_match, sniiic_acpi_attach, NULL, NULL);

void sni_i2c_common_i(struct sniiic_softc *);

static int sni_i2c_acquire_bus(void *, int);
static void sni_i2c_release_bus(void *, int);
static int sni_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			size_t, void *, size_t, int);

static int sni_i2c_intr(void *);
static void sni_i2c_reset(struct sniiic_softc *);
static void sni_i2c_flush(struct sniiic_softc *);

#define I2C_READ(sc, reg) \
    bus_space_read_4((sc)->sc_ioh,(sc)->sc_ioh,(reg))
#define I2C_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_ioh,(sc)->sc_ioh,(reg),(val))

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
	const int phandle = faa->faa_phandle;
	bus_space_handle_t ioh;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0
	    || bus_space_map(faa->faa_bst, addr, size, 0, &ioh) != 0) {
		aprint_error(": unable to map device\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		goto fail;
	}
	sc->sc_ih = fdtbus_intr_establish(phandle,
			0, IPL_BIO, 0, sni_i2c_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto fail;
	}

	aprint_naive("\n");
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	sc->sc_ioh = ioh;
	sc->sc_iob = addr;
	sc->sc_ios = size;

	sni_i2c_common_i(sc);

	fdtbus_register_i2c_controller(&sc->sc_ic, phandle);
#if 0
	fdtbus_attach_i2cbus(self, phandle, &sc->sc_ic, iicbus_print);
#endif
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
	struct i2cbus_attach_args iba;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;
	mem = acpi_res_mem(&res, 0);
	irq = acpi_res_irq(&res, 0);
	if (mem == NULL || irq == NULL || mem->ar_length == 0) {
		aprint_error(": incomplete resources\n");
		return;
	}
	if (bus_space_map(aa->aa_memt, mem->ar_base, mem->ar_length, 0,
	    &ioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_ih = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)aa->aa_node->ad_handle,
	    IPL_BIO, false, sni_i2c_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto fail;
	}

	aprint_naive("\n");

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_memt;
	sc->sc_ioh = ioh;
	sc->sc_iob = mem->ar_base;
	sc->sc_ios = mem->ar_length;

	sni_i2c_common_i(sc);

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_ic;
#if 0
	(void) config_found_ia(sc->sc_dev, "i2cbus", &iba, iicbus_print);
#endif

	acpi_resource_cleanup(&res);

	return;
 fail:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	acpi_resource_cleanup(&res);
	return;
}

void
sni_i2c_common_i(struct sniiic_softc *sc)
{

	aprint_normal_dev(sc->sc_dev, "Socionext I2C controller\n");

	iic_tag_init(&sc->sc_ic);
	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = sni_i2c_acquire_bus;
	sc->sc_ic.ic_release_bus = sni_i2c_release_bus;
	sc->sc_ic.ic_exec = sni_i2c_exec;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_cv, device_xname(sc->sc_dev));

	/* no attach here */
}

static int
sni_i2c_acquire_bus(void *opaque, int flags)
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
sni_i2c_release_bus(void *opaque, int flags)
{
	struct sniiic_softc *sc = opaque;

	mutex_enter(&sc->sc_lock);
	sc->sc_busy = false;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);
}

static int
sni_i2c_exec(void *opaque, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct sniiic_softc *sc = opaque;
	int err;
#if 0
	printf("%s: exec op: %d addr: 0x%x cmdlen: %d len: %d flags 0x%x\n",
	    device_xname(sc->sc_dev), op, addr, (int)cmdlen, (int)len, flags);
#endif

	err = 0;
	/* AAA */
	goto done;
 done:
	if (err)
		sni_i2c_reset(sc);
	sni_i2c_flush(sc);
	return err;
}

static int
sni_i2c_intr(void *arg)
{
	struct sniiic_softc * const sc = arg;
	uint32_t stat = 0;

	(void)stat;
	mutex_enter(&sc->sc_mtx);
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_mtx);
	return 1;
}

static void
sni_i2c_reset(struct sniiic_softc *sc)
{
	/* AAA */
}

static void
sni_i2c_flush(struct sniiic_softc *sc)
{
	/* AAA */
}
