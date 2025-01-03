/* $NetBSD: jh71x0_temp.c,v 1.1 2025/01/03 11:49:04 skrll Exp $ */

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: jh71x0_temp.c,v 1.1 2025/01/03 11:49:04 skrll Exp $");

#include <sys/param.h>

#include <dev/fdt/fdtvar.h>

#include <dev/sysmon/sysmonvar.h>

struct jh71x0_temp_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	struct sysmon_envsys *	sc_sme;
	envsys_data_t		sc_sensor;
};

#define RD4(sc, reg)							       \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)						       \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

/* Register definitions */
#define JH71X0_TEMP		0x0000
#define JH71X0_TEMP_RSTN	__BIT(0)
#define JH71X0_TEMP_PD		__BIT(1)
#define JH71X0_TEMP_RUN		__BIT(2)
#define JH71X0_TEMP_DOUT_MASK	__BITS(27, 16)

/* DOUT to Celcius conversion constants */
#define JH71X0TEMP_Y1000	237500L
#define JH71X0TEMP_Z		4094L
#define JH71X0TEMP_K1000	81100L

/* Calculate the temperature in milli Celcius */
static int32_t
jh71x0_temp_get(struct jh71x0_temp_softc *sc)
{
	uint32_t temp = RD4(sc, JH71X0_TEMP);
	uint32_t dout = __SHIFTOUT(temp, JH71X0_TEMP_DOUT_MASK);

	return (dout * JH71X0TEMP_Y1000) / JH71X0TEMP_Z - JH71X0TEMP_K1000;
}

static void
jh71x0_temp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct jh71x0_temp_softc * const sc = sme->sme_cookie;

	// Convert milli Celcius to micro Kelvin
	sc->sc_sensor.value_cur = 273150000 + 1000 * jh71x0_temp_get(sc);
	sc->sc_sensor.state = ENVSYS_SVALID;
}

static void
jh71x0_temp_init(struct jh71x0_temp_softc *sc)
{
	/* Power down */
	WR4(sc, JH71X0_TEMP, JH71X0_TEMP_PD);
	delay(1);

	/* Power up with reset asserted */
	WR4(sc, JH71X0_TEMP, 0);
	delay(60);

	/* Deassert reset */
	WR4(sc, JH71X0_TEMP, JH71X0_TEMP_RSTN);
	delay(1);

	/* Start measuring */
	WR4(sc, JH71X0_TEMP, JH71X0_TEMP_RSTN | JH71X0_TEMP_RUN);
}

/* Compat string(s) */
static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7100-temp" },
	{ .compat = "starfive,jh7110-temp" },
	DEVICE_COMPAT_EOL
};

static int
jh71x0_temp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
jh71x0_temp_attach(device_t parent, device_t self, void *aux)
{
	struct jh71x0_temp_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d", addr,
		    error);
		return;
	}

	const char *crs[] = { "bus", "sense" };
	for (size_t i = 0; i < __arraycount(crs); i++) {
		const char *cr = crs[i];

		error = fdtbus_clock_enable(phandle, cr, true);
		if (error) {
			aprint_error(": couldn't enable clock '%s'\n", cr);
			return;
		}
		struct fdtbus_reset * rst = fdtbus_reset_get(phandle, cr);
		if (rst == NULL) {
			aprint_error(": couldn't get reset '%s'\n", cr);
			return;
		}
		error = fdtbus_reset_deassert(rst);
		if (error) {
			aprint_error(": couldn't de-assert reset '%s'\n", cr);
			return;
		}
	}

	aprint_naive("\n");
	aprint_normal(": JH71x0 temperature sensor\n");

	jh71x0_temp_init(sc);

	sc->sc_sme = sysmon_envsys_create();
	/* Initialize sensor data. */
	sc->sc_sensor.units = ENVSYS_STEMP;
	sc->sc_sensor.state = ENVSYS_SINVALID;
	(void)strlcpy(sc->sc_sensor.desc, device_xname(self),
	    sizeof(sc->sc_sensor.desc));
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	/* Hook into system monitor. */
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = jh71x0_temp_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

CFATTACH_DECL_NEW(jh71x0_temp, sizeof(struct jh71x0_temp_softc),
	jh71x0_temp_match, jh71x0_temp_attach, NULL, NULL);
