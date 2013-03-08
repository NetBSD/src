/*
 * Copyright (c) 2012 Oleksandr Tymoshenko
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/malloc.h>
#include <sys/rman.h>
#include <sys/timeet.h>
#include <sys/timetc.h>
#include <sys/watchdog.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/intr.h>

#include <dev/fdt/fdt_common.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/ofw_bus_subr.h>

#include <machine/bus.h>
#include <machine/fdt.h>

#include "vchiq_arm.h"
#include "vchiq_2835.h"

#define	VCHIQ_LOCK	do {		\
	mtx_lock(&bcm_vchiq_sc->lock);	\
} while(0)

#define	VCHIQ_UNLOCK	do {		\
	mtx_unlock(&bcm_vchiq_sc->lock);	\
} while(0)

#ifdef  DEBUG
#define dprintf(fmt, args...) printf(fmt, ##args)
#else
#define dprintf(fmt, args...)
#endif

struct bcm_vchiq_softc {
	struct mtx		lock;
	struct resource *	mem_res;
	struct resource *	irq_res;
	void*			intr_hl;
	bus_space_tag_t		bst;
	bus_space_handle_t	bsh;
};

static struct bcm_vchiq_softc *bcm_vchiq_sc = NULL;

#define	vchiq_read_4(reg)		\
    bus_space_read_4(bcm_vchiq_sc->bst, bcm_vchiq_sc->bsh, reg)
#define	vchiq_write_4(reg, val)		\
    bus_space_write_4(bcm_vchiq_sc->bst, bcm_vchiq_sc->bsh, reg, val)

/* 
 * Extern functions */
void vchiq_exit(void);
int vchiq_init(void);

extern VCHIQ_STATE_T g_state;

static void
bcm_vchiq_intr(void *arg)
{
	VCHIQ_STATE_T *state = &g_state;
	unsigned int status;

	/* Read (and clear) the doorbell */
	status = vchiq_read_4(0x40);

	if (status & 0x4) {  /* Was the doorbell rung? */
		remote_event_pollall(state);
	}
}

void
remote_event_signal(REMOTE_EVENT_T *event)
{
	event->fired = 1;

	/* The test on the next line also ensures the write on the previous line
		has completed */

	if (event->armed) {
		/* trigger vc interrupt */
 		__asm __volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory");

		vchiq_write_4(0x48, 0);
	}
}

static int
bcm_vchiq_probe(device_t dev)
{

	if (ofw_bus_is_compatible(dev, "broadcom,bcm2835-vchiq")) {
		device_set_desc(dev, "BCM2835 VCHIQ");
		return(BUS_PROBE_DEFAULT);
	}

	return (ENXIO);
}

static int
bcm_vchiq_attach(device_t dev)
{
	struct bcm_vchiq_softc *sc = device_get_softc(dev);
	int rid = 0;

	if (bcm_vchiq_sc != NULL)
		return (EINVAL);

	sc->mem_res = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &rid, RF_ACTIVE);
	if (sc->mem_res == NULL) {
		device_printf(dev, "could not allocate memory resource\n");
		return (ENXIO);
	}

	sc->bst = rman_get_bustag(sc->mem_res);
	sc->bsh = rman_get_bushandle(sc->mem_res);

	rid = 0;
	sc->irq_res = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid, RF_ACTIVE);
	if (sc->irq_res == NULL) {
		device_printf(dev, "could not allocate interrupt resource\n");
		return (ENXIO);
	}

	/* Setup and enable the timer */
	if (bus_setup_intr(dev, sc->irq_res, INTR_TYPE_MISC,
			NULL, bcm_vchiq_intr, sc,
			&sc->intr_hl) != 0) {
		bus_release_resource(dev, SYS_RES_IRQ, rid,
			sc->irq_res);
		device_printf(dev, "Unable to setup the clock irq handler.\n");
		return (ENXIO);
	}

	mtx_init(&sc->lock, "vchiq", MTX_DEF, 0);
	bcm_vchiq_sc = sc;

	vchiq_init();

	return (0);
}

static int
bcm_vchiq_detach(device_t dev)
{
	struct bcm_vchiq_softc *sc = device_get_softc(dev);

	vchiq_exit();

	if (sc->intr_hl)
                bus_teardown_intr(dev, sc->irq_res, sc->intr_hl);
	bus_release_resource(dev, SYS_RES_IRQ, 0,
		sc->irq_res);
	bus_release_resource(dev, SYS_RES_MEMORY, 0,
		sc->mem_res);

	mtx_destroy(&sc->lock);

	return (0);
}


static device_method_t bcm_vchiq_methods[] = {
	DEVMETHOD(device_probe,		bcm_vchiq_probe),
	DEVMETHOD(device_attach,	bcm_vchiq_attach),
	DEVMETHOD(device_detach,	bcm_vchiq_detach),
	{ 0, 0 }
};

static driver_t bcm_vchiq_driver = {
	"vchiq",
	bcm_vchiq_methods,
	sizeof(struct bcm_vchiq_softc),
};

static devclass_t bcm_vchiq_devclass;

DRIVER_MODULE(vchiq, simplebus, bcm_vchiq_driver, bcm_vchiq_devclass, 0, 0);
