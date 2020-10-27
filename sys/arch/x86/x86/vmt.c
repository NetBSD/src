/* $NetBSD: vmt.c,v 1.21 2020/10/27 08:57:11 ryo Exp $ */
/* $OpenBSD: vmt.c,v 1.11 2011/01/27 21:29:25 dtucker Exp $ */

/*
 * Copyright (c) 2007 David Crawshaw <david@zentus.com>
 * Copyright (c) 2008 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Protocol reverse engineered by Ken Kato:
 * https://sites.google.com/site/chitchatvmback/backdoor
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/module.h>
#include <machine/cpuvar.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/vmt/vmtreg.h>
#include <dev/vmt/vmtvar.h>

static int	vmt_match(device_t, cfdata_t, void *);
static void	vmt_attach(device_t, device_t, void *);
static int	vmt_detach(device_t, int);

CFATTACH_DECL_NEW(vmt, sizeof(struct vmt_softc),
	vmt_match, vmt_attach, vmt_detach, NULL);

static int
vmt_match(device_t parent, cfdata_t match, void *aux)
{
	struct cpufeature_attach_args *cfaa = aux;
	struct cpu_info *ci = cfaa->ci;

	if (strcmp(cfaa->name, "vm") != 0)
		return 0;
	if ((ci->ci_flags & (CPUF_BSP|CPUF_SP|CPUF_PRIMARY)) == 0)
		return 0;

	return vmt_probe();
}

static void
vmt_attach(device_t parent, device_t self, void *aux)
{
	struct vmt_softc *sc = device_private(self);

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	vmt_common_attach(sc);
}

static int
vmt_detach(device_t self, int flags)
{
	struct vmt_softc *sc = device_private(self);

	return vmt_common_detach(sc);
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
