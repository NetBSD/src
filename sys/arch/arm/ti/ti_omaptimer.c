/*	$NetBSD: ti_omaptimer.c,v 1.1 2017/10/26 01:16:32 jakllsch Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ti_omaptimer.c,v 1.1 2017/10/26 01:16:32 jakllsch Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/timetc.h>
#include <sys/kernel.h>

#include <arm/locore.h>
#include <arm/fdt/arm_fdtvar.h>

#include <dev/fdt/fdtvar.h>

static const char * const compatible[] = {
	"ti,am335x-timer-1ms",
	"ti,am335x-timer",
	NULL
};

struct omaptimer_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_phandle;
	struct clk *sc_clk;
	struct timecounter sc_tc;
};

static struct omaptimer_softc *timer_softc;

static int
omaptimer_intr(void *arg)
{
	struct omaptimer_softc * const sc = timer_softc;
	struct clockframe * const frame = arg;

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, 0x28, 2);
	hardclock(frame);

	return 1;
}

static void
omaptimer_cpu_initclocks(void)
{
	struct omaptimer_softc * const sc = timer_softc;
	char intrstr[128];
	void *ih;

	KASSERT(sc != NULL);
	if (!fdtbus_intr_str(sc->sc_phandle, 0, intrstr, sizeof(intrstr)))
		panic("%s: failed to decode interrupt", __func__);
	ih = fdtbus_intr_establish(sc->sc_phandle, 0, IPL_CLOCK,
	    FDT_INTR_MPSAFE, omaptimer_intr, NULL);
	if (ih == NULL)
		panic("%s: failed to establish timer interrupt", __func__);
	
	aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);

	uint32_t value;
	value = (0xffffffff - ((24000000UL / hz) - 1));
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, 0x40, value);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, 0x3c, value);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, 0x2c, 2);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, 0x38, 3);
}

static int
omaptimer_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
omaptimer_attach(device_t parent, device_t self, void *aux)
{
	struct omaptimer_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

#if 0
	if ((sc->sc_clk = fdtbus_clock_get_index(phandle, 0)) == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}
#endif

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		device_printf(self, "unable to map bus space");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Timer\n");

	/* Use this as the OS timer in UP configurations */
	if (!arm_has_mpext_p && addr == 0x48042000) { /* TIMER3 */
		timer_softc = sc;
		arm_fdt_timer_register(omaptimer_cpu_initclocks);
	}
}

CFATTACH_DECL_NEW(omaptimer, sizeof(struct omaptimer_softc),
    omaptimer_match, omaptimer_attach, NULL, NULL);

