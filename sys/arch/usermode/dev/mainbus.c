/* $NetBSD: mainbus.c,v 1.9.46.1 2018/06/25 07:25:46 pgoyette Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.9.46.1 2018/06/25 07:25:46 pgoyette Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/cons.h>

#include <machine/mainbus.h>

static int	mainbus_match(device_t, cfdata_t, void *);
static void	mainbus_attach(device_t, device_t, void *);

static int	mainbus_print(void *, const char *);

typedef struct mainbus_softc {
	device_t	sc_dev;
} mainbus_softc_t;

CFATTACH_DECL_NEW(mainbus, sizeof(mainbus_softc_t),
    mainbus_match, mainbus_attach, NULL, NULL);

extern char *usermode_disk_image_path[];
extern int usermode_disk_image_path_count;
extern int   usermode_vdev_type[];
extern char *usermode_vdev_path[];
extern int usermode_vdev_count;
extern char *usermode_tap_device;
extern char *usermode_tap_eaddr;
extern char *usermode_audio_device;
extern int usermode_vnc_width, usermode_vnc_height, usermode_vnc_port;

static int
mainbus_match(device_t parent, cfdata_t match, void *opaque)
{

	return 1;
}

static void
mainbus_attach(device_t parent, device_t self, void *opaque)
{
	mainbus_softc_t *sc = device_private(self);
	struct thunkbus_attach_args taa;
	int i;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;

	taa.taa_type = THUNKBUS_TYPE_CPU;
	config_found_ia(self, "thunkbus", &taa, mainbus_print);

	taa.taa_type = THUNKBUS_TYPE_TTYCONS;
	config_found_ia(self, "thunkbus", &taa, mainbus_print);

	if (usermode_vnc_port > 0 && usermode_vnc_port < 65536) {
		taa.taa_type = THUNKBUS_TYPE_VNCFB;
		taa.u.vnc.width = usermode_vnc_width;
		taa.u.vnc.height = usermode_vnc_height;
		taa.u.vnc.port = usermode_vnc_port;
		config_found_ia(self, "thunkbus", &taa, mainbus_print);
	}

	taa.taa_type = THUNKBUS_TYPE_CLOCK;
	config_found_ia(self, "thunkbus", &taa, mainbus_print);

	if (usermode_tap_device) {
		taa.taa_type = THUNKBUS_TYPE_VETH;
		taa.u.veth.device = usermode_tap_device;
		taa.u.veth.eaddr = usermode_tap_eaddr;
		config_found_ia(self, "thunkbus", &taa, mainbus_print);
	}

	if (usermode_audio_device) {
		taa.taa_type = THUNKBUS_TYPE_VAUDIO;
		taa.u.vaudio.device = usermode_audio_device;
		config_found_ia(self, "thunkbus", &taa, mainbus_print);
	}

	for (i = 0; i < usermode_disk_image_path_count; i++) {
		taa.taa_type = THUNKBUS_TYPE_DISKIMAGE;
		taa.u.diskimage.path = usermode_disk_image_path[i];
		config_found_ia(self, "thunkbus", &taa, mainbus_print);
	}

	for (i = 0; i < usermode_vdev_count; i++) {
		taa.taa_type = usermode_vdev_type[i];
		taa.u.vdev.path = usermode_vdev_path[i];
		config_found_ia(self, "thunkbus", &taa, mainbus_print);
	}
}

static int
mainbus_print(void *opaque, const char *pnp)
{
	if (pnp)
		aprint_normal("%s", pnp);
	return UNCONF;
}
