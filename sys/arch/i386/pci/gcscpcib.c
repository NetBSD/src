/* $NetBSD: gcscpcib.c,v 1.6.8.1 2009/05/13 17:17:50 jym Exp $ */
/* $OpenBSD: gcscpcib.c,v 1.6 2007/11/17 17:02:47 mbalmer Exp $	*/

/*
 * Copyright (c) 2008 Yojiro UO <yuo@nui.org>
 * Copyright (c) 2007 Marc Balmer <mbalmer@openbsd.org>
 * Copyright (c) 2007 Michael Shalayeff
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * AMD CS5535/CS5536 series LPC bridge also containing timer, watchdog and GPIO.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gcscpcib.c,v 1.6.8.1 2009/05/13 17:17:50 jym Exp $");

#include "gpio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/sysctl.h>
#include <sys/timetc.h>
#include <sys/wdog.h>

#include <sys/bus.h>
#include <machine/cpufunc.h>

#if NGPIO > 0
#include <dev/gpio/gpiovar.h>
#endif
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/sysmon/sysmonvar.h>

#include <arch/i386/pci/gcscpcibreg.h>
#include <arch/x86/pci/pcibvar.h>

/* define if you need to select MFGPT for watchdog manually (0-5). */
/* #define AMD553X_WDT_FORCEUSEMFGPT 	0 */
/* select precision of watchdog timer (default value = 0.25sec (4Hz tick) */
#define	AMD553X_MFGPT_PRESCALE	AMD553X_MFGPT_DIV_8K /* 32K/8K = 4Hz */

/* #define GCSCPCIB_DEBUG */
#ifdef GCSCPCIB_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

/* 1 bit replace (not support multiple bit)*/
#define AMD553X_MFGPTx_NR_DISABLE(x, bit) \
	( wrmsr(AMD553X_MFGPT_NR, rdmsr(AMD553X_MFGPT_NR) & ~((bit) << (x))) )
#define AMD553X_MFGPTx_NR_ENABLE(x, bit) \
	( wrmsr(AMD553X_MFGPT_NR, rdmsr(AMD553X_MFGPT_NR) | ((bit) << (x))) )

/* caliculate watchdog timer setting */
#define	AMD553X_WDT_TICK (1<<(AMD553X_MFGPT_DIV_32K - AMD553X_MFGPT_PRESCALE))
#define AMD553X_WDT_COUNTMAX	(0xffff / AMD553X_WDT_TICK)

struct gcscpcib_softc {
	/* we call pcibattach() which assumes softc starts like this: */
	struct pcib_softc	sc_pcib;

	pcireg_t		sc_pirqrc;

	struct timecounter	sc_timecounter;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	/* Watchdog Timer */
	struct sysmon_wdog	sc_smw;
	int			sc_wdt_mfgpt;

	/* GPIO interface */
	bus_space_tag_t		sc_gpio_iot;
	bus_space_handle_t	sc_gpio_ioh;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[AMD553X_GPIO_NPINS];

	/* SMbus/i2c interface */ 
#if 0
	bus_space_tag_t		sc_smbus_iot;
        bus_space_handle_t	sc_smbus_ioh;
	i2c_addr_t		sc_smbus_slaveaddr; /* address of smbus slave */
	struct i2c_controller	sc_i2c;		/* i2c controller info */
	krwlock_t		sc_smbus_rwlock;
#endif
};

static int	gcscpcib_match(device_t, cfdata_t, void *);
static void	gcscpcib_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gcscpcib, sizeof(struct gcscpcib_softc),
	gcscpcib_match, gcscpcib_attach, NULL, NULL);

static u_int	gcscpcib_get_timecount(struct timecounter *tc);
static int	gscspcib_scan_mfgpt(struct gcscpcib_softc *sc);
static void 	gscspcib_wdog_update(struct gcscpcib_softc *, uint16_t);
static int	gcscpcib_wdog_setmode(struct sysmon_wdog *smw);
static int	gcscpcib_wdog_tickle(struct sysmon_wdog *smw);
static void	gcscpcib_wdog_enable(struct gcscpcib_softc *sc);
static void	gcscpcib_wdog_disable(struct gcscpcib_softc *sc);
static void	gcscpcib_wdog_reset(struct gcscpcib_softc *sc);

#if NGPIO > 0
static int	gcscpcib_gpio_pin_read(void *, int);
static void	gcscpcib_gpio_pin_write(void *, int, int);
static void	gcscpcib_gpio_pin_ctl(void *, int, int);
#endif

static int
gcscpcib_match(device_t parent, cfdata_t match, void *aux)
{ 
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_BRIDGE ||
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_ISA)
		return 0;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_NS_CS5535_ISA:
	case PCI_PRODUCT_AMD_CS5536_PCIB:
		return 2;	/* supersede pcib(4) */
	}

	return 0;
}

static void
gcscpcib_attach(device_t parent, device_t self, void *aux)
{
	struct gcscpcib_softc *sc = device_private(self);
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	struct timecounter *tc = &sc->sc_timecounter;
	bus_addr_t wdtbase;
	int wdt = 0;
#if NGPIO > 0
	struct gpiobus_attach_args gba;
	bus_addr_t gpiobase;
	int i, gpio;
#endif

	sc->sc_pcib.sc_pc = pa->pa_pc;
	sc->sc_pcib.sc_tag = pa->pa_tag;
	sc->sc_iot = pa->pa_iot;
	sc->sc_gpio_iot = pa->pa_iot;

	/* Attach the PCI-ISA bridge at first */
	pcibattach(parent, self, aux);

	/* Attach the CS553[56] timer */
	tc->tc_get_timecount = gcscpcib_get_timecount;
	tc->tc_counter_mask = 0xffffffff;
	tc->tc_frequency = 3579545;
	tc->tc_name = device_xname(self);
	tc->tc_quality = 1000;
	tc->tc_priv = sc;
	tc_init(tc);

	/* Attach the watchdog timer */
	wdtbase = rdmsr(MSR_LBAR_MFGPT) & 0xffff;
	if (bus_space_map(sc->sc_iot, wdtbase, 64, 0, &sc->sc_ioh)) {
		aprint_error_dev(self, "can't map memory space for WDT\n");
	} else {
		/* select a MFGPT timer for watchdog counter */
		if (!gscspcib_scan_mfgpt(sc)) {
			aprint_error_dev(self, "can't alloc an MFGPT for WDT\n");
			goto gpio;
		}
		/* 
		 * Note: MFGPGx_SETUP register is write once register
 		 * except CNT_EN and CMP[12]EV bit. 
		 */
		bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			AMD553X_MFGPTX_SETUP(sc->sc_wdt_mfgpt),
		    	AMD553X_MFGPT_CMP2EV | AMD553X_MFGPT_CMP2 |
		    	AMD553X_MFGPT_PRESCALE);

		/* disable watchdog action */
		AMD553X_MFGPTx_NR_DISABLE(sc->sc_wdt_mfgpt, 
			AMD553X_MFGPT0_C2_NMIM);
		AMD553X_MFGPTx_NR_DISABLE(sc->sc_wdt_mfgpt,
			AMD553X_MFGPT0_C2_RSTEN);

		sc->sc_smw.smw_name = device_xname(self);
		sc->sc_smw.smw_cookie = sc;
		sc->sc_smw.smw_setmode = gcscpcib_wdog_setmode;
		sc->sc_smw.smw_tickle = gcscpcib_wdog_tickle;
		sc->sc_smw.smw_period = 32;
		aprint_normal_dev(self, "Watchdog Timer via MFGPT%d",
			 sc->sc_wdt_mfgpt);
		wdt = 1;
	}

gpio:
#if NGPIO > 0
	/* map GPIO I/O space */
	gpiobase = rdmsr(MSR_LBAR_GPIO) & 0xffff;
	if (!bus_space_map(sc->sc_gpio_iot, gpiobase, 0xff, 0,
	    &sc->sc_gpio_ioh)) {
		aprint_normal(", GPIO");

		/* initialize pin array */
		for (i = 0; i < AMD553X_GPIO_NPINS; i++) {
			sc->sc_gpio_pins[i].pin_num = i;
			sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INPUT |
			    GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN |
			    GPIO_PIN_PUSHPULL | GPIO_PIN_TRISTATE |
			    GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN |
			    GPIO_PIN_INVIN | GPIO_PIN_INVOUT;

			/* read initial state */
			sc->sc_gpio_pins[i].pin_state =
			    gcscpcib_gpio_pin_read(sc, i);
		}

		/* create controller tag */
		sc->sc_gpio_gc.gp_cookie = sc;
		sc->sc_gpio_gc.gp_pin_read = gcscpcib_gpio_pin_read;
		sc->sc_gpio_gc.gp_pin_write = gcscpcib_gpio_pin_write;
		sc->sc_gpio_gc.gp_pin_ctl = gcscpcib_gpio_pin_ctl;

		gba.gba_gc = &sc->sc_gpio_gc;
		gba.gba_pins = sc->sc_gpio_pins;
		gba.gba_npins = AMD553X_GPIO_NPINS;
		gpio = 1;
	}
#endif
	aprint_normal("\n");

#if NGPIO > 0
	/* Attach GPIO framework */
	if (gpio)
                config_found_ia(self, "gpiobus", &gba, gpiobus_print);
#endif

	/* Register Watchdog timer to SMW */
	if (wdt) {
		if (sysmon_wdog_register(&sc->sc_smw) != 0)
			aprint_error_dev(self,
			    "cannot register wdog with sysmon\n");
	}
}

static u_int
gcscpcib_get_timecount(struct timecounter *tc)
{
        return rdmsr(AMD553X_TMC);
}

/* Watchdog timer support functions */
static int	
gscspcib_scan_mfgpt(struct gcscpcib_softc *sc)
{
	int i;

#ifdef AMD553X_WDT_FORCEUSEMFGPT
	if (AMD553X_WDT_FORCEUSEMFGPT >= AMD553X_MFGPT_MAX)
		return 0;
	sc->sc_wdt_mfgpt = AMD553X_WDT_FORCEUSEMFGPT;
	return 1;
#endif /* AMD553X_WDT_FORCEUSEMFGPT */

	for (i = 0; i < AMD553X_MFGPT_MAX; i++){
		if (bus_space_read_2(sc->sc_iot, sc->sc_ioh,
		    AMD553X_MFGPTX_SETUP(i)) == 0) {
			/* found unused MFGPT, use it. */
			sc->sc_wdt_mfgpt = i;
			return 1;
		}
	}
	/* no MFGPT for WDT found */
	return 0;  
}


static void
gscspcib_wdog_update(struct gcscpcib_softc *sc, uint16_t count)
{
#ifdef GCSCPCIB_DEBUG
	uint16_t cnt;
	cnt = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
		AMD553X_MFGPTX_CNT(sc->sc_wdt_mfgpt));
#endif
	if (count > AMD553X_WDT_COUNTMAX)
		count = AMD553X_WDT_COUNTMAX;
	/* 
	 * CS553X databook recommend following sequence to re-initialize
	 * the counter and compare value. (See p165 on CS5536 databook) 
	 * 1: suspend counter: clear counter enable bit to 0 
	 * 2: reset (and NMI, if need) enable bit in MSRs
	 * 3: update counter & clear event flags
	 * 4: resume (2) operation
	 * 5: re-enable counter
	 */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
		AMD553X_MFGPTX_SETUP(sc->sc_wdt_mfgpt), 0);
	AMD553X_MFGPTx_NR_DISABLE(sc->sc_wdt_mfgpt, AMD553X_MFGPT0_C2_RSTEN);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 
		AMD553X_MFGPTX_CNT(sc->sc_wdt_mfgpt), count);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
		AMD553X_MFGPTX_SETUP(sc->sc_wdt_mfgpt), 
			AMD553X_MFGPT_CMP1 | AMD553X_MFGPT_CMP2);
	AMD553X_MFGPTx_NR_ENABLE(sc->sc_wdt_mfgpt, AMD553X_MFGPT0_C2_RSTEN);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
		AMD553X_MFGPTX_SETUP(sc->sc_wdt_mfgpt),
	    	AMD553X_MFGPT_CNT_EN | AMD553X_MFGPT_CMP2);

	DPRINTF(("%s: MFGPT%d_CNT= %d -> %d (expect: %d), MFGPT_NR=%#.8x\n",
		__func__, sc->sc_wdt_mfgpt, cnt,
		bus_space_read_2(sc->sc_iot, sc->sc_ioh,
			 AMD553X_MFGPTX_CNT(sc->sc_wdt_mfgpt)), count,
		(uint32_t)(rdmsr(AMD553X_MFGPT_NR))));
}

static void
gcscpcib_wdog_disable(struct gcscpcib_softc *sc)
{
	/*
	 * stop counter and reset counter value 
	 * Note: as the MFGPTx_SETUP is write once register, the prescaler
	 * setting, clock select and compare mode are kept till reset.
	 */
	gscspcib_wdog_update(sc, 0);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
		AMD553X_MFGPTX_SETUP(sc->sc_wdt_mfgpt), 0);

	/* disable watchdog action */
	DPRINTF(("%s: disable watchdog action\n", __func__));
	AMD553X_MFGPTx_NR_DISABLE(sc->sc_wdt_mfgpt, AMD553X_MFGPT0_C2_RSTEN);
}

static void
gcscpcib_wdog_enable(struct gcscpcib_softc *sc)
{
	int period = sc->sc_smw.smw_period;

	/* clear recent event flag and counter value, and start counter */
	gcscpcib_wdog_reset(sc);
	/* set watchdog timer limit, counter tick is 0.5sec */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, 
		AMD553X_MFGPTX_CMP2(sc->sc_wdt_mfgpt), 
			period * AMD553X_WDT_TICK);

	/* enable watchdog action */
	DPRINTF(("%s: enable watchdog action. (MFGPT0_CMP2= %d)\n", __func__,
	        bus_space_read_2(sc->sc_iot, sc->sc_ioh,
			 AMD553X_MFGPTX_CMP2(sc->sc_wdt_mfgpt))));
	AMD553X_MFGPTx_NR_ENABLE(sc->sc_wdt_mfgpt, AMD553X_MFGPT0_C2_RSTEN);
}

static int
gcscpcib_wdog_setmode(struct sysmon_wdog *smw)
{
	struct gcscpcib_softc *sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		gcscpcib_wdog_disable(sc);
		return 0;
	}

	if (smw->smw_period == WDOG_PERIOD_DEFAULT)
		smw->smw_period = 32;
	else if (smw->smw_period > AMD553X_WDT_COUNTMAX) /* too big */
		return EINVAL;	

	gcscpcib_wdog_enable(sc);

	return 0;
}

static void
gcscpcib_wdog_reset(struct gcscpcib_softc *sc)
{
	/* reset counter value */
	gscspcib_wdog_update(sc, 0);
	/* start counter & clear recent event of CMP2 */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
		AMD553X_MFGPTX_SETUP(sc->sc_wdt_mfgpt),
	   	AMD553X_MFGPT_CNT_EN | AMD553X_MFGPT_CMP2);
}

static int
gcscpcib_wdog_tickle(struct sysmon_wdog *smw)
{
	struct gcscpcib_softc *sc = smw->smw_cookie;

	DPRINTF(("%s: update watchdog timer\n", __func__));
	gcscpcib_wdog_reset(sc);
	return 0;
}

#if NGPIO > 0
/* GPIO support functions */
static int
gcscpcib_gpio_pin_read(void *arg, int pin)
{
	struct gcscpcib_softc *sc = arg;
	uint32_t data;
	int reg;

	reg = AMD553X_GPIO_OUT_VAL;
	if (pin > 15) {
		pin &= 0x0f;
		reg += AMD553X_GPIOH_OFFSET;
	}
	data = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, reg);

	return data & 1 << pin ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
}

static void
gcscpcib_gpio_pin_write(void *arg, int pin, int value)
{
	struct gcscpcib_softc *sc = arg;
	uint32_t data;
	int reg;

	reg = AMD553X_GPIO_OUT_VAL;
	if (pin > 15) {
		pin &= 0x0f;
		reg += AMD553X_GPIOH_OFFSET;
	}
	if (value == 1)
		data = 1 << pin;
	else
		data = 1 << (pin + 16);

	bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, reg, data);
}

static void
gcscpcib_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct gcscpcib_softc *sc = arg;
	int n, reg[7], val[7], nreg = 0, off = 0;

	if (pin > 15) {
		pin &= 0x0f;
		off = AMD553X_GPIOH_OFFSET;
	}

	reg[nreg] = AMD553X_GPIO_IN_EN + off;
	if (flags & GPIO_PIN_INPUT)
		val[nreg++] = 1 << pin;
	else
		val[nreg++] = 1 << (pin + 16);

	reg[nreg] = AMD553X_GPIO_OUT_EN + off;
	if (flags & GPIO_PIN_OUTPUT)
		val[nreg++] = 1 << pin;
	else
		val[nreg++] = 1 << (pin + 16);

	reg[nreg] = AMD553X_GPIO_OD_EN + off;
	if (flags & GPIO_PIN_OPENDRAIN)
		val[nreg++] = 1 << pin;
	else
		val[nreg++] = 1 << (pin + 16);

	reg[nreg] = AMD553X_GPIO_PU_EN + off;
	if (flags & GPIO_PIN_PULLUP)
		val[nreg++] = 1 << pin;
	else
		val[nreg++] = 1 << (pin + 16);

	reg[nreg] = AMD553X_GPIO_PD_EN + off;
	if (flags & GPIO_PIN_PULLDOWN)
		val[nreg++] = 1 << pin;
	else
		val[nreg++] = 1 << (pin + 16);

	reg[nreg] = AMD553X_GPIO_IN_INVRT_EN + off;
	if (flags & GPIO_PIN_INVIN)
		val[nreg++] = 1 << pin;
	else
		val[nreg++] = 1 << (pin + 16);

	reg[nreg] = AMD553X_GPIO_OUT_INVRT_EN + off;
	if (flags & GPIO_PIN_INVOUT)
		val[nreg++] = 1 << pin;
	else
		val[nreg++] = 1 << (pin + 16);

	/* set flags */
	for (n = 0; n < nreg; n++)
		bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, reg[n],
		    val[n]);
} 
#endif /* NGPIO > 0 */

