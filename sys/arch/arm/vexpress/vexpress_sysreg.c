/* $NetBSD: vexpress_sysreg.c,v 1.3.6.2 2017/12/03 11:35:56 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: vexpress_sysreg.c,v 1.3.6.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/fdt/fdtvar.h>

#define	SYS_CFGDATA	0x00a0
#define	SYS_CFGCTRL	0x00a4
#define	 SYS_CFGCTRL_START	__BIT(31)
#define	 SYS_CFGCTRL_WRITE	__BIT(30)
#define	 SYS_CFGCTRL_FUNCTION	__BITS(25,20)
#define	  SYS_CFGCTRL_FUNCTION_SHUTDOWN	8
#define	  SYS_CFGCTRL_FUNCTION_REBOOT	9
#define	 SYS_CFGCTRL_SITE	__BITS(17,16)
#define	  SYS_CFGCTRL_SITE_MB		0
#define	SYS_CFGSTAT	0x00a8

static int	vexpress_sysreg_match(device_t, cfdata_t, void *);
static void	vexpress_sysreg_attach(device_t, device_t, void *);

static const char * const compatible[] = { "arm,vexpress-sysreg", NULL };

struct vexpress_sysreg_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

CFATTACH_DECL_NEW(vexpress_sysreg, sizeof(struct vexpress_sysreg_softc),
	vexpress_sysreg_match, vexpress_sysreg_attach, NULL, NULL);

static void
vexpress_sysreg_write(device_t dev, u_int func, u_int site)
{
	struct vexpress_sysreg_softc * const sc = device_private(dev);

	const uint32_t cfgctrl = SYS_CFGCTRL_START | SYS_CFGCTRL_WRITE |
	    __SHIFTIN(func, SYS_CFGCTRL_FUNCTION) |
	    __SHIFTIN(site, SYS_CFGCTRL_SITE);

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, SYS_CFGSTAT, 0);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, SYS_CFGDATA, 0);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, SYS_CFGCTRL, cfgctrl);
}

static void
vexpress_sysreg_reset(device_t dev)
{
	delay(1000000);
	vexpress_sysreg_write(dev, SYS_CFGCTRL_FUNCTION_REBOOT, SYS_CFGCTRL_SITE_MB);
}

static void
vexpress_sysreg_poweroff(device_t dev)
{
	delay(1000000);
	vexpress_sysreg_write(dev, SYS_CFGCTRL_FUNCTION_SHUTDOWN, SYS_CFGCTRL_SITE_MB);
}

static struct fdtbus_power_controller_func vexpress_sysreg_power_funcs = {
	.reset = vexpress_sysreg_reset,
	.poweroff = vexpress_sysreg_poweroff,
};

static int
vexpress_sysreg_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
vexpress_sysreg_attach(device_t parent, device_t self, void *aux)
{
	struct vexpress_sysreg_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": missing 'reg' property\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_bsh)) {
		aprint_error(": couldn't map device\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal("\n");

	fdtbus_register_power_controller(self, phandle,
	    &vexpress_sysreg_power_funcs);
}
