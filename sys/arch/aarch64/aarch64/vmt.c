/* $NetBSD */

/*
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#include <sys/device.h>
#include <sys/module.h>

#include <dev/vmt/vmtreg.h>
#include <dev/vmt/vmtvar.h>

static int	vmt_match(device_t, cfdata_t, void *);
static void	vmt_attach(device_t, device_t, void *);
static int	vmt_detach(device_t, int);

CFATTACH_DECL_NEW(vmt, sizeof(struct vmt_softc),
	vmt_match, vmt_attach, vmt_detach, NULL);

static bool vmt_attached = false;

static int
vmt_match(device_t parent, cfdata_t match, void *aux)
{
	/* vmt should not attach to more than a single CPU. */
	if (vmt_attached)
		return 0;

	return vmt_probe();
}

static void
vmt_attach(device_t parent, device_t self, void *aux)
{
	struct vmt_softc *sc = device_private(self);

	aprint_naive("\n");
	aprint_normal(": VMware Tools driver\n");

	sc->sc_dev = self;
	vmt_common_attach(sc);

	vmt_attached = true;
}

static int
vmt_detach(device_t self, int flags)
{
	struct vmt_softc *sc = device_private(self);
	int rv;

	rv = vmt_common_detach(sc);
	if (rv != 0)
		return rv;

	vmt_attached = false;
	return 0;
}

MODULE(MODULE_CLASS_DRIVER, vmt, "sysmon_power,sysmon_taskq");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
vmt_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_vmt,
		    cfattach_ioconf_vmt, cfdata_ioconf_vmt);
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_vmt,
		    cfattach_ioconf_vmt, cfdata_ioconf_vmt);
#endif
		break;
	case MODULE_CMD_AUTOUNLOAD:
		error = EBUSY;
		break;
	default:
		error = ENOTTY;
		break;
	}

	return error;
}
