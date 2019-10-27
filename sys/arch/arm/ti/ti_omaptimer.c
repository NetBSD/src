/*	$NetBSD: ti_omaptimer.c,v 1.2 2019/10/27 17:59:21 jmcneill Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ti_omaptimer.c,v 1.2 2019/10/27 17:59:21 jmcneill Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/timetc.h>
#include <sys/kernel.h>

#include <arm/locore.h>
#include <arm/fdt/arm_fdtvar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/ti/ti_prcm.h>

#define	TIMER_IRQENABLE_SET	0x2c
#define	TIMER_IRQENABLE_CLR	0x30
#define	 MAT_EN_FLAG		__BIT(0)
#define	 OVF_EN_FLAG		__BIT(1)
#define	 TCAR_EN_FLAG		__BIT(2)
#define	TIMER_TCLR		0x38
#define	 TCLR_ST		__BIT(0)
#define	 TCLR_AR		__BIT(1)
#define	TIMER_TCRR		0x3c
#define	TIMER_TLDR		0x40

/* XXX */
#define	IS_TIMER2(addr)	((addr) == 0x48040000)
#define	IS_TIMER3(addr)	((addr) == 0x48042000)

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

	/* Enable interrupts */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, TIMER_IRQENABLE_SET, OVF_EN_FLAG);
}

static u_int
omaptimer_get_timecount(struct timecounter *tc)
{
	struct omaptimer_softc * const sc = tc->tc_priv;

	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, TIMER_TCRR);
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
	struct timecounter *tc = &sc->sc_tc;
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

	if (ti_prcm_enable_hwmod(OF_parent(phandle), 0) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Timer\n");

	if (IS_TIMER2(addr)) {
		/* Install timecounter */
		tc->tc_get_timecount = omaptimer_get_timecount;
		tc->tc_counter_mask = ~0u;
		tc->tc_frequency = 24000000;
		tc->tc_name = "Timer2";
		tc->tc_quality = 200;
		tc->tc_priv = sc;
		tc_init(tc);
	} else if (IS_TIMER3(addr)) {
		/* Configure the timer */
		const uint32_t value = (0xffffffff - ((24000000UL / hz) - 1));
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, TIMER_TLDR, value);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, TIMER_TCRR, value);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, TIMER_IRQENABLE_CLR, 
		    MAT_EN_FLAG | OVF_EN_FLAG | TCAR_EN_FLAG);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, TIMER_TCLR, TCLR_ST | TCLR_AR);

		/* Use this as the OS timer in UP configurations */
		if (!arm_has_mpext_p) {
			timer_softc = sc;
			arm_fdt_timer_register(omaptimer_cpu_initclocks);
		}
	}
}

CFATTACH_DECL_NEW(omaptimer, sizeof(struct omaptimer_softc),
    omaptimer_match, omaptimer_attach, NULL, NULL);

