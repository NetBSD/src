/* $NetBSD: gpiokeys.c,v 1.2.2.3 2017/08/28 17:52:02 skrll Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gpiokeys.c,v 1.2.2.3 2017/08/28 17:52:02 skrll Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <dev/fdt/fdtvar.h>

#define GPIOKEYS_POLL_INTERVAL	mstohz(200)

#define KEY_POWER	116
#define KEY_SLEEP	142

static int	gpiokeys_match(device_t, cfdata_t, void *);
static void	gpiokeys_attach(device_t, device_t, void *);

static void	gpiokeys_tick(void *);
static void	gpiokeys_task(void *);

struct gpiokeys_softc;

struct gpiokeys_key {
	int			key_phandle;
	char	 		*key_label;
	struct fdtbus_gpio_pin	*key_pin;
	u_int			key_debounce;
	u_int			key_code;
	struct sysmon_pswitch	key_pswitch;
	u_int			key_state;

	struct gpiokeys_key	*key_next;
};

struct gpiokeys_softc {
	device_t	sc_dev;
	int		sc_phandle;

	struct fdtbus_gpio_pin *sc_pin;
	bool		sc_always_on;
	bool		sc_enable_val;

	struct gpiokeys_key *sc_keys;
	callout_t	sc_tick;
};

CFATTACH_DECL_NEW(gpiokeys, sizeof(struct gpiokeys_softc),
    gpiokeys_match, gpiokeys_attach, NULL, NULL);

static int
gpiokeys_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "gpio-keys", NULL };
	const struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
gpiokeys_attach(device_t parent, device_t self, void *aux)
{
	struct gpiokeys_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	struct gpiokeys_key *key;
	int child, len;
	u_int debounce, code;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	aprint_naive("\n");
	aprint_normal(":");

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (of_getprop_uint32(child, "linux,code", &code))
			continue;
		if (of_getprop_uint32(child, "debounce-interval", &debounce))
			debounce = 5;	/* default */
		len = OF_getproplen(child, "label");
		if (len <= 0) {
			continue;
		}
		key = kmem_zalloc(sizeof(*key), KM_SLEEP);
		key->key_phandle = child;
		key->key_code = code;
		key->key_label = kmem_zalloc(len, KM_SLEEP);
		if (OF_getprop(child, "label", key->key_label, len) != len) {
			kmem_free(key->key_label, len);
			kmem_free(key, sizeof(*key));
			continue;
		}
		key->key_debounce = debounce;
		key->key_pin = fdtbus_gpio_acquire(child, "gpios",
		    GPIO_PIN_INPUT);
		if (key->key_pin)
			key->key_state = fdtbus_gpio_read(key->key_pin);

		key->key_pswitch.smpsw_name = key->key_label;
		switch (code) {
		case KEY_POWER:
			key->key_pswitch.smpsw_type = PSWITCH_TYPE_POWER;
			break;
		case KEY_SLEEP:
			key->key_pswitch.smpsw_type = PSWITCH_TYPE_SLEEP;
			break;
		default:
			key->key_pswitch.smpsw_type = PSWITCH_TYPE_HOTKEY;
			break;
		}

		if (sysmon_pswitch_register(&key->key_pswitch) != 0) {
			aprint_error(" %s:ERROR", key->key_label);
			kmem_free(key->key_label, len);
			kmem_free(key, sizeof(*key));
			continue;
		}

		if (sc->sc_keys) {
			aprint_normal(", %s", key->key_label);
		} else {
			aprint_normal(" %s", key->key_label);
		}

		key->key_next = sc->sc_keys;
		sc->sc_keys = key;
	}

	if (sc->sc_keys == NULL) {
		aprint_normal(" no keys configured\n");
		return;
	}

	aprint_normal("\n");

	callout_init(&sc->sc_tick, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_tick, gpiokeys_tick, sc);

	gpiokeys_tick(sc);
}

static void
gpiokeys_tick(void *priv)
{
	struct gpiokeys_softc * const sc = priv;
	struct gpiokeys_key *key;

	for (key = sc->sc_keys; key; key = key->key_next) {
		if (key->key_pin == NULL) {
			continue;
		}
		const int new_state = fdtbus_gpio_read(key->key_pin);
		if (new_state != key->key_state) {
			key->key_state = new_state;
			sysmon_task_queue_sched(0, gpiokeys_task, key);
		}
	}
	callout_schedule(&sc->sc_tick, GPIOKEYS_POLL_INTERVAL);
}

static void
gpiokeys_task(void *priv)
{
	struct gpiokeys_key *key = priv;

	sysmon_pswitch_event(&key->key_pswitch,
	    key->key_state ? PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED);
}
