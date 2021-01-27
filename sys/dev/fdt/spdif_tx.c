/* $NetBSD: spdif_tx.c,v 1.2 2021/01/27 03:10:21 thorpej Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: spdif_tx.c,v 1.2 2021/01/27 03:10:21 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/kmem.h>

#include <dev/audio/audio_dai.h>

#include <dev/fdt/fdtvar.h>

struct spdif_tx_softc {
	device_t		sc_dev;
	struct audio_dai_device	sc_dai;
};

static int	spdif_tx_match(device_t, cfdata_t, void *);
static void	spdif_tx_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "linux,spdif-dit" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(spdiftx, sizeof(struct spdif_tx_softc),
	spdif_tx_match, spdif_tx_attach, NULL, NULL);

static int
spdif_tx_set_format(audio_dai_tag_t dai, u_int format)
{
	return 0;
}

static int
spdif_tx_add_device(audio_dai_tag_t dai, audio_dai_tag_t aux)
{
	return 0;
}

static const struct audio_hw_if spdif_tx_hw_if = { };

static audio_dai_tag_t
spdif_tx_dai_get_tag(device_t dev, const void *data, size_t len)
{
	struct spdif_tx_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	return &sc->sc_dai;
}

static struct fdtbus_dai_controller_func spdif_tx_dai_funcs = {
	.get_tag = spdif_tx_dai_get_tag
};

static int
spdif_tx_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
spdif_tx_attach(device_t parent, device_t self, void *aux)
{
	struct spdif_tx_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal(": SPDIF transmitter\n");

	sc->sc_dai.dai_set_format = spdif_tx_set_format;
	sc->sc_dai.dai_add_device = spdif_tx_add_device;
	sc->sc_dai.dai_hw_if = &spdif_tx_hw_if;
	sc->sc_dai.dai_dev = self;
	sc->sc_dai.dai_priv = sc;
	fdtbus_register_dai_controller(self, phandle, &spdif_tx_dai_funcs);
}
