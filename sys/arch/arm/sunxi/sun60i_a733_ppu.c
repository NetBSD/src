/* $NetBSD: sun60i_a733_ppu.c,v 1.1 2026/11/22 14:29:22 jmcneill Exp $ */

/*-
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
__KERNEL_RCSID(0, "$NetBSD: sun60i_a733_ppu.c,v 1.1 2026/11/22 14:29:22 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <arm/cpufunc.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#define	PD_BASE(id)			((id) * 0x1000)
#define	PD_REG_PWR(id)			(PD_BASE(id) + 0x000)
#define	PD_REG_STATUS(id)		(PD_BASE(id) + 0x008)
#define	PD_STATUS_MASK			__BITS(3, 0)
#define	PD_STATUS_OFF			__SHIFTIN(0x0, PD_STATUS_MASK)
#define	PD_STATUS_ON			__SHIFTIN(0x8, PD_STATUS_MASK)
#define	PD_REG_DEVCTRL0_DELAY(id)	(PD_BASE(id) + 0x170)
#define	PD_REG_DEVCTRL1_DELAY(id)	(PD_BASE(id) + 0x174)
#define	PD_REG_LOGIC_PWRSW0_DELAY(id)	(PD_BASE(id) + 0xc00)
#define	PD_REG_LOGIC_PWRSW1_DELAY(id)	(PD_BASE(id) + 0xc04)
#define	PD_REG_OFF2ON_DELAY(id)		(PD_BASE(id) + 0xc10)

#define	DEVCTRL0_DELAY_DEFAULT		0x001f1f1f
#define	DEVCTRL1_DELAY_DEFAULT		0x00001f1f
#define	LOGIC_PWRSW0_DELAY_DEFAULT	0x08080808
#define	LOGIC_PWRSW1_DELAY_DEFAULT	0x00000808
#define	OFF2ON_DELAY_DEFAULT		0x00000008

#define PMU_ENABLE_TIMEOUT		10000

#define	REG_LOCK(sc)		syscon_lock((sc)->sc_syscon)
#define	REG_UNLOCK(sc)		syscon_unlock((sc)->sc_syscon)
#define	RD4(sc, reg) 		syscon_read_4((sc)->sc_syscon, (reg))
#define	WR4(sc, reg, val) 	syscon_write_4((sc)->sc_syscon, (reg), (val))

struct sunxi_ppu_softc {
	device_t sc_dev;
	struct syscon *sc_syscon;
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun60iw2-pck-600" },
	DEVICE_COMPAT_EOL
};

static bool
sunxi_ppu_is_enabled(struct sunxi_ppu_softc *sc, u_int id)
{
	uint32_t val;

	/* REG_LOCK must be held */
	val = RD4(sc, PD_REG_PWR(id));

	return (val & PD_STATUS_MASK) == PD_STATUS_ON;
}

static void
sunxi_ppu_enable(device_t dev, const uint32_t *data, bool enable)
{
	struct sunxi_ppu_softc * const sc = device_private(dev);
	const u_int id = be32dec(data);

	REG_LOCK(sc);
	if (sunxi_ppu_is_enabled(sc, id) != enable) {
		uint32_t val;
		int timo;

		val = RD4(sc, PD_REG_PWR(id));
		val &= ~PD_STATUS_MASK;
		val |= (enable ? PD_STATUS_ON : PD_STATUS_OFF);
		WR4(sc, PD_REG_PWR(id), val);

		dsb(sy);

		for (timo = 0; timo < PMU_ENABLE_TIMEOUT; timo++) {
			if (sunxi_ppu_is_enabled(sc, id) == enable) {
				break;
			}
			delay(1);
		}
		if (timo == PMU_ENABLE_TIMEOUT) {
			aprint_error_dev(dev, "failed to %sable power domain\n",
			    enable ? "en" : "dis");
		}
	}
	REG_UNLOCK(sc);
}

static int
sunxi_ppu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_ppu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ppu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_syscon = fdtbus_syscon_lookup(OF_parent(phandle));
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't get syscon registers\n");
		return;
	}

	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal(": PPU600\n");

	for (int node = OF_child(phandle); node; node = OF_peer(node)) {
		static struct fdtbus_powerdomain_controller_func funcs = {
			.pdc_enable = sunxi_ppu_enable,
		};
		bus_size_t id;
		
		if (fdtbus_get_reg(phandle, 0, &id, NULL) != 0) {
			aprint_error_dev(self, "invalid 'reg' property\n");
			continue;
		}

		REG_LOCK(sc);
		WR4(sc, PD_REG_DEVCTRL0_DELAY(id), DEVCTRL0_DELAY_DEFAULT);
		WR4(sc, PD_REG_DEVCTRL1_DELAY(id), DEVCTRL1_DELAY_DEFAULT);
		WR4(sc, PD_REG_LOGIC_PWRSW0_DELAY(id),
		    LOGIC_PWRSW0_DELAY_DEFAULT);
		WR4(sc, PD_REG_LOGIC_PWRSW1_DELAY(id),
		    LOGIC_PWRSW1_DELAY_DEFAULT);
		WR4(sc, PD_REG_OFF2ON_DELAY(id), OFF2ON_DELAY_DEFAULT);
		REG_UNLOCK(sc);

		fdtbus_register_powerdomain_controller(self, node, &funcs);
	}
}

CFATTACH_DECL_NEW(sun60i_a733_ppu, sizeof(struct sunxi_ppu_softc),
    sunxi_ppu_match, sunxi_ppu_attach, NULL, NULL);
