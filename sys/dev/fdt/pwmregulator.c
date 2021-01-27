/* $NetBSD: pwmregulator.c,v 1.3 2021/01/27 03:10:21 thorpej Exp $ */

/*
 * Copyright (c) 2020 Ryo Shimizu <ryo@nerv.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pwmregulator.c,v 1.3 2021/01/27 03:10:21 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>
#include <dev/pwm/pwmvar.h>

static int pwmregulator_match(device_t, cfdata_t, void *);
static void pwmregulator_attach(device_t, device_t, void *);

/* fdtbus_regulator_controller_func callback */
static int pwmregulator_acquire(device_t);
static void pwmregulator_release(device_t);
static int pwmregulator_enable(device_t, bool);
static int pwmregulator_set_voltage(device_t, u_int, u_int);
static int pwmregulator_get_voltage(device_t, u_int *);

static const struct fdtbus_regulator_controller_func pwmregulator_funcs = {
	.acquire = pwmregulator_acquire,
	.release = pwmregulator_release,
	.enable = pwmregulator_enable,
	.set_voltage = pwmregulator_set_voltage,
	.get_voltage = pwmregulator_get_voltage
};

struct voltage_duty {
	uint32_t microvolt;
	uint32_t duty;		/* percentage; 0-100 */
};

struct pwmregulator_softc {
	device_t sc_dev;
	pwm_tag_t sc_pwm;
	struct fdtbus_gpio_pin *sc_pin;
	struct voltage_duty *sc_voltage_table;
	int sc_voltage_table_num;
	int sc_phandle;
	uint32_t sc_microvolt_min;
	uint32_t sc_microvolt_max;
	uint32_t sc_dutycycle_unit;
	uint32_t sc_dutycycle_range[2];
	bool sc_always_on;
	bool sc_boot_on;
};

CFATTACH_DECL_NEW(pregulator, sizeof(struct pwmregulator_softc),
    pwmregulator_match, pwmregulator_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "pwm-regulator" },
	DEVICE_COMPAT_EOL
};

static int
pwmregulator_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
pwmregulator_attach(device_t parent, device_t self, void *aux)
{
	struct pwmregulator_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	int len;
	char *name;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	aprint_naive("\n");
	len = OF_getproplen(phandle, "regulator-name");
	if (len > 0) {
		name = kmem_zalloc(len, KM_SLEEP);
		if (OF_getprop(phandle, "regulator-name", name, len) == len)
			aprint_normal(": %s\n", name);
		else
			aprint_normal("\n");
		kmem_free(name, len);
	} else {
		aprint_normal("\n");
	}

	if (of_getprop_uint32(phandle, "regulator-min-microvolt",
	    &sc->sc_microvolt_min) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "missing regulator-min-microvolt properties\n");
		return;
	}
	if (of_getprop_uint32(phandle, "regulator-max-microvolt",
	    &sc->sc_microvolt_max) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "missing regulator-max-microvolt properties\n");
		return;
	}

	if (of_getprop_uint32(phandle, "pwm-dutycycle-unit",
	    &sc->sc_dutycycle_unit) != 0)
		sc->sc_dutycycle_unit = 100;

	if (of_getprop_uint32_array(phandle, "pwm-dutycycle-range",
	    sc->sc_dutycycle_range, 2) != 0) {
		sc->sc_dutycycle_range[0] = 0;
		sc->sc_dutycycle_range[1] = 100;
	}

	len = OF_getproplen(phandle, "voltage-table");
	if (len > 0) {
		struct voltage_duty *voltage_table = kmem_zalloc(len, KM_SLEEP);
		if (of_getprop_uint32_array(phandle, "voltage-table",
		    (uint32_t *)voltage_table, len / sizeof(uint32_t)) == 0) {
			sc->sc_voltage_table = voltage_table;
			sc->sc_voltage_table_num =
			    len / sizeof(struct voltage_duty);
#ifdef PWMREGULATOR_DEBUG
			for (int i = 0; i < sc->sc_voltage_table_num; i++) {
				aprint_debug_dev(sc->sc_dev,
				    "VoltageTable[%d]: %uuV = Duty:%u%%\n", i,
				    voltage_table[i].voltage,
				    voltage_table[i].duty);
			}
#endif
			/*
			 * if voltage-table is provided, the duty in the table
			 * represents a percentage, i.e. 0-100%, so
			 * dutycycle_unit is 100.
			 */
			sc->sc_dutycycle_unit = 100;
		} else {
			kmem_free(sc->sc_voltage_table, len);
		}
	}
#ifdef PWMREGULATOR_DEBUG
	if (sc->sc_voltage_table_num == 0) {
		aprint_debug_dev(sc->sc_dev, "Duty:%u%%=%uuV, Duty:%u%%=%uuV\n",
		    sc->sc_dutycycle_range[0], sc->sc_microvolt_min,
		    sc->sc_dutycycle_range[1], sc->sc_microvolt_max);
	}
#endif

	sc->sc_always_on = of_getprop_bool(phandle, "regulator-always-on");
	sc->sc_boot_on = of_getprop_bool(phandle, "regulator-boot-on");

	fdtbus_register_regulator_controller(self, phandle,
	    &pwmregulator_funcs);

	/*
	 * If the regulator is flagged as always on or enabled at boot,
	 * ensure that it is enabled
	 */
	if (sc->sc_always_on || sc->sc_boot_on)
		pwmregulator_enable(self, true);
}

static int
pwmregulator_acquire(device_t dev)
{
	struct pwmregulator_softc * const sc = device_private(dev);

	/* "enable-gpios" is optional */
	sc->sc_pin = fdtbus_gpio_acquire(sc->sc_phandle, "enable-gpios",
	    GPIO_PIN_OUTPUT);

	sc->sc_pwm = fdtbus_pwm_acquire(sc->sc_phandle, "pwms");
	if (sc->sc_pwm == NULL)
		return ENXIO;

	return 0;
}

static void
pwmregulator_release(device_t dev)
{
	struct pwmregulator_softc * const sc = device_private(dev);

	if (sc->sc_pin != NULL) {
		fdtbus_gpio_write(sc->sc_pin, 0);
		fdtbus_gpio_release(sc->sc_pin);
	}

	sc->sc_pwm = NULL;
}

static int
pwmregulator_enable(device_t dev, bool enable)
{
	struct pwmregulator_softc * const sc = device_private(dev);
	int error;

	if (sc->sc_pwm == NULL)
		return ENXIO;

	if (enable) {
		if (sc->sc_pin != NULL)
			fdtbus_gpio_write(sc->sc_pin, 1);
		error = pwm_enable(sc->sc_pwm);
	} else {
		error = pwm_disable(sc->sc_pwm);
		if (sc->sc_pin != NULL)
			fdtbus_gpio_write(sc->sc_pin, 0);
	}

	return error;
}

static int
pwmregulator_set_voltage(device_t dev, u_int min_uvolt, u_int max_uvolt)
{
	struct pwmregulator_softc * const sc = device_private(dev);
	struct pwm_config conf;
	int duty, d0, d1, v0, v1, uv, rc;

	if (sc->sc_pwm == NULL)
		return ENXIO;

	rc = pwm_get_config(sc->sc_pwm, &conf);
	if (rc != 0) {
		device_printf(dev, "%s: couldn't get pwm config, error=%d\n",
		    __func__, rc);
		return rc;
	}

	uv = (min_uvolt + max_uvolt) / 2;

	if (sc->sc_voltage_table_num > 0) {
		/* find the nearest duty from voltage-table */
		int i, bestidx = 0;
		for (i = 1; i < sc->sc_voltage_table_num; i++) {
			if (abs(sc->sc_voltage_table[i].microvolt - uv) <
			    abs(sc->sc_voltage_table[bestidx].microvolt - uv))
				bestidx = i;
		}
		duty = sc->sc_voltage_table[bestidx].duty;
	} else {
		/* calculate duty from voltage */
		v0 = sc->sc_microvolt_min;
		v1 = sc->sc_microvolt_max;
		d0 = sc->sc_dutycycle_range[0];
		d1 = sc->sc_dutycycle_range[1];
		duty = (uv - v0) * (d1 - d0) / (v1 - v0) + d0;
	}

	conf.duty_cycle = duty * conf.period / sc->sc_dutycycle_unit;

	rc = pwm_set_config(sc->sc_pwm, &conf);
	if (rc != 0)
		device_printf(dev, "couldn't set pwm config, error=%d\n", rc);
	return rc;
}

static int
pwmregulator_get_voltage(device_t dev, u_int *puvolt)
{
	struct pwmregulator_softc * const sc = device_private(dev);
	struct pwm_config conf;
	int duty, d0, d1, v0, v1, uv, rc;

	if (sc->sc_pwm == NULL)
		return ENXIO;

	rc = pwm_get_config(sc->sc_pwm, &conf);
	if (rc != 0) {
		device_printf(dev, "%s: couldn't get pwm config, error=%d\n",
		    __func__, rc);
		return rc;
	}

	duty = conf.duty_cycle * sc->sc_dutycycle_unit / conf.period;

	if (sc->sc_voltage_table_num > 0) {
		/* find the nearest voltage from voltage-table */
		int i, bestidx = 0;
		for (i = 1; i < sc->sc_voltage_table_num; i++) {
			if (abs(sc->sc_voltage_table[i].duty - duty) <
			    abs(sc->sc_voltage_table[bestidx].duty - duty))
				bestidx = i;
		}
		uv = sc->sc_voltage_table[bestidx].microvolt;
	} else {
		/* calculate voltage from duty */
		d0 = sc->sc_dutycycle_range[0];
		d1 = sc->sc_dutycycle_range[1];
		v0 = sc->sc_microvolt_min;
		v1 = sc->sc_microvolt_max;
		uv = (duty - d0) * (v1 - v0) / (d1 - d0)  + v0;
	}

	*puvolt = uv;
	return 0;
}
