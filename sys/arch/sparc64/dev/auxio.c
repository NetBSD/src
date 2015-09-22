/*	$NetBSD: auxio.c,v 1.22.30.1 2015/09/22 12:05:52 skrll Exp $	*/

/*
 * Copyright (c) 2000, 2001, 2015 Matthew R. Green
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

/*
 * AUXIO registers support on the sbus & ebus2, used for the floppy driver
 * and to control the system LED, for the BLINK option.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auxio.c,v 1.22.30.1 2015/09/22 12:05:52 skrll Exp $");

#include "opt_auxio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>
#include <dev/sbus/sbusvar.h>
#include <sparc64/dev/auxioreg.h>
#include <sparc64/dev/auxiovar.h>

/*
 * on sun4u, auxio exists with one register (LED) on the sbus, and 5
 * registers on the ebus2 (pci) (LED, PCIMODE, FREQUENCY, SCSI
 * OSCILLATOR, and TEMP SENSE.
 */

struct auxio_softc {
	device_t		sc_dev;

	kmutex_t		sc_lock;

	/* parent's tag */
	bus_space_tag_t		sc_tag;

	/* handles to the various auxio register sets */
	bus_space_handle_t	sc_led;
	bus_space_handle_t	sc_pci;
	bus_space_handle_t	sc_freq;
	bus_space_handle_t	sc_scsi;
	bus_space_handle_t	sc_temp;

	int			sc_flags;
#define	AUXIO_LEDONLY		0x1	// only sc_led is valid
#define	AUXIO_EBUS		0x2
};

#define	AUXIO_ROM_NAME		"auxio"


static uint32_t	auxio_read_led(struct auxio_softc *);
static void	auxio_write_led(struct auxio_softc *, uint32_t);
static void	auxio_attach_common(struct auxio_softc *);
static int	auxio_ebus_match(device_t, cfdata_t, void *);
static void	auxio_ebus_attach(device_t, device_t, void *);
static int	auxio_sbus_match(device_t, cfdata_t, void *);
static void	auxio_sbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(auxio_ebus, sizeof(struct auxio_softc),
    auxio_ebus_match, auxio_ebus_attach, NULL, NULL);

CFATTACH_DECL_NEW(auxio_sbus, sizeof(struct auxio_softc),
    auxio_sbus_match, auxio_sbus_attach, NULL, NULL);

extern struct cfdriver auxio_cd;

static __inline__ uint32_t
auxio_read_led(struct auxio_softc *sc)
{
	uint32_t led;

	if (sc->sc_flags & AUXIO_EBUS)
		led = le32toh(bus_space_read_4(sc->sc_tag, sc->sc_led, 0));
	else
		led = bus_space_read_1(sc->sc_tag, sc->sc_led, 0);

	return led;
}

static __inline__ void
auxio_write_led(struct auxio_softc *sc, uint32_t led)
{

	if (sc->sc_flags & AUXIO_EBUS)
		bus_space_write_4(sc->sc_tag, sc->sc_led, 0, htole32(led));
	else
		bus_space_write_1(sc->sc_tag, sc->sc_led, 0, led);
}

#ifdef BLINK
static callout_t blink_ch;
static void auxio_blink(void *);

/* let someone disable it if it's already turned on; XXX sysctl? */
int do_blink = 1;

static void
auxio_blink(void *x)
{
	struct auxio_softc *sc = x;
	int s;
	uint32_t led;

	if (do_blink == 0)
		return;

	mutex_enter(&sc->sc_lock);
	led = auxio_read_led(sc);
	led = led ^ AUXIO_LED_LED;
	auxio_write_led(sc, led);
	mutex_exit(&sc->sc_lock);

	/*
	 * Blink rate is:
	 *	full cycle every second if completely idle (loadav = 0)
	 *	full cycle every 2 seconds if loadav = 1
	 *	full cycle every 3 seconds if loadav = 2
	 * etc.
	 */
	s = (((averunnable.ldavg[0] + FSCALE) * hz) >> (FSHIFT + 1));
	callout_reset(&blink_ch, s, auxio_blink, sc);
}
#endif

static void
auxio_attach_common(struct auxio_softc *sc)
{
#ifdef BLINK
	static int do_once = 1;

	/* only start one blinker */
	if (do_once) {
		mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);
		callout_init(&blink_ch, CALLOUT_MPSAFE);
		auxio_blink(sc);
		do_once = 0;
	}
#endif
	printf("\n");
}

static int
auxio_ebus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ebus_attach_args *ea = aux;

	return (strcmp(AUXIO_ROM_NAME, ea->ea_name) == 0);
}

static void
auxio_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct auxio_softc *sc = device_private(self);
	struct ebus_attach_args *ea = aux;

	sc->sc_dev = self;
	sc->sc_tag = ea->ea_bustag;

	if (ea->ea_nreg < 1) {
		printf(": no registers??\n");
		return;
	}

	sc->sc_flags = AUXIO_EBUS;
	if (ea->ea_nreg != 5) {
		printf(": not 5 (%d) registers, only setting led",
		    ea->ea_nreg);
		sc->sc_flags |= AUXIO_LEDONLY;
	} else if (ea->ea_nvaddr == 5) {
		sparc_promaddr_to_handle(sc->sc_tag, 
			ea->ea_vaddr[1], &sc->sc_pci);
		sparc_promaddr_to_handle(sc->sc_tag, 
			ea->ea_vaddr[2], &sc->sc_freq);
		sparc_promaddr_to_handle(sc->sc_tag, 
			ea->ea_vaddr[3], &sc->sc_scsi);
		sparc_promaddr_to_handle(sc->sc_tag, 
			ea->ea_vaddr[4], &sc->sc_temp);
	} else {
		bus_space_map(sc->sc_tag, EBUS_ADDR_FROM_REG(&ea->ea_reg[1]),
			ea->ea_reg[1].size, 0, &sc->sc_pci);
		bus_space_map(sc->sc_tag, EBUS_ADDR_FROM_REG(&ea->ea_reg[2]),
			ea->ea_reg[2].size, 0, &sc->sc_freq);
		bus_space_map(sc->sc_tag, EBUS_ADDR_FROM_REG(&ea->ea_reg[3]),
			ea->ea_reg[3].size, 0, &sc->sc_scsi);
		bus_space_map(sc->sc_tag, EBUS_ADDR_FROM_REG(&ea->ea_reg[4]),
			ea->ea_reg[4].size, 0, &sc->sc_temp);
	}

	if (ea->ea_nvaddr > 0) {
		sparc_promaddr_to_handle(sc->sc_tag, 
			ea->ea_vaddr[0], &sc->sc_led);
	} else {
		bus_space_map(sc->sc_tag, EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
			ea->ea_reg[0].size, 0, &sc->sc_led);
	}
	
	auxio_attach_common(sc);
}

static int
auxio_sbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sbus_attach_args *sa = aux;

	return strcmp(AUXIO_ROM_NAME, sa->sa_name) == 0;
}

static void
auxio_sbus_attach(device_t parent, device_t self, void *aux)
{
	struct auxio_softc *sc = device_private(self);
	struct sbus_attach_args *sa = aux;

	sc->sc_dev = self;
	sc->sc_tag = sa->sa_bustag;

	if (sa->sa_nreg < 1) {
		printf(": no registers??\n");
		return;
	}

	if (sa->sa_nreg != 1) {
		printf(": not 1 (%d/%d) registers??", sa->sa_nreg,
		    sa->sa_npromvaddrs);
		return;
	}

	/* sbus auxio only has one set of registers */
	sc->sc_flags = AUXIO_LEDONLY;
	if (sa->sa_npromvaddrs > 0) {
		sbus_promaddr_to_handle(sc->sc_tag,
			sa->sa_promvaddr, &sc->sc_led);
	} else {
		sbus_bus_map(sc->sc_tag, sa->sa_slot, sa->sa_offset,
			sa->sa_size, 0, &sc->sc_led);
	}

	auxio_attach_common(sc);
}

int
auxio_fd_control(u_int32_t bits)
{
	struct auxio_softc *sc;
	u_int32_t led;

	if (auxio_cd.cd_ndevs == 0) {
		return ENXIO;
	}

	/*
	 * XXX This does not handle > 1 auxio correctly.
	 * We'll assume the floppy drive is tied to first auxio found.
	 */
	sc = device_lookup_private(&auxio_cd, 0);
	if (!sc) {
		return ENXIO;
	}

	mutex_enter(&sc->sc_lock);
	led = auxio_read_led(sc);
	led = (led & ~AUXIO_LED_FLOPPY_MASK) | bits;
	auxio_write_led(sc, led);
	mutex_exit(&sc->sc_lock);

	return 0;
}
