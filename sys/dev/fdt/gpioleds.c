/* $NetBSD: gpioleds.c,v 1.1.8.2 2017/12/03 11:37:01 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: gpioleds.c,v 1.1.8.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <dev/led.h>

#include <dev/fdt/fdtvar.h>

static int	gpioleds_match(device_t, cfdata_t, void *);
static void	gpioleds_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gpioleds, 0, gpioleds_match, gpioleds_attach, NULL, NULL);

static int
gpioleds_get(void *priv)
{
	struct fdtbus_gpio_pin *pin = priv;

	return fdtbus_gpio_read(pin);
}

static void
gpioleds_set(void *priv, int state)
{
	struct fdtbus_gpio_pin *pin = priv;

	fdtbus_gpio_write(pin, state);
}

static int
gpioleds_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "gpio-leds", NULL };
	const struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
gpioleds_attach(device_t parent, device_t self, void *aux)
{
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_gpio_pin *pin;
	const char *default_state;
	char label[64];
	int child;

	aprint_naive("\n");
	aprint_normal(":");

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		/* Get the label, fallback to node name */
		if (OF_getprop(child, "label", label, sizeof(label)) <= 0 &&
		    OF_getprop(child, "name", label, sizeof(label)) <= 0)
			continue;

		/* Get the output pin */
		pin = fdtbus_gpio_acquire(child, "gpios", GPIO_PIN_OUTPUT);
		if (pin == NULL)
			continue;

		/* Attach the LED */
		if (led_attach(label, pin, gpioleds_get, gpioleds_set) == NULL)
			continue;

		aprint_normal(" %s", label);

		/* Set default state */
		default_state = fdtbus_get_string(child, "default-state");
		if (default_state && strcmp(default_state, "on") == 0)
			gpioleds_set(pin, LED_STATE_ON);
		else if (default_state && strcmp(default_state, "off") == 0)
			gpioleds_set(pin, LED_STATE_OFF);
	}

	aprint_normal("\n");
}
