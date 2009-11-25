#include "opt_gemini.h"
#if !defined(GEMINI_MASTER)  && !defined(GEMINI_SLAVE)
# error IPI needs GEMINI_MASTER or GEMINI_SLAVE
#endif
#include "locators.h"
#include "geminiipm.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: gemini_ipi.c,v 1.5 2009/11/25 14:28:50 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/malloc.h>
#include <arch/arm/gemini/gemini_obiovar.h>
#include <arch/arm/gemini/gemini_ipivar.h>
#include <arch/arm/gemini/gemini_reg.h>

static int  gemini_ipi_match(struct device *, struct cfdata *, void *);
static void gemini_ipi_attach(struct device *, struct device *, void *);
static int  gemini_ipiintr(void *);

CFATTACH_DECL_NEW(geminiipi, sizeof(struct gemini_ipi_softc),
	gemini_ipi_match, gemini_ipi_attach, NULL, NULL);

static gemini_ipi_softc_t *gemini_ipi_sc;


static int
gemini_ipi_match(struct device *parent, struct cfdata *cf, void *aux)
{
        struct obio_attach_args *obio = aux;

        if (obio->obio_intr == LPCCF_INTR_DEFAULT)
                panic("ipi must specify intr in config.");

        return 1;
}

static void
gemini_ipi_attach(struct device *parent, struct device *self, void *aux)
{
        gemini_ipi_softc_t *sc = device_private(self);
        struct obio_attach_args *obio = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_size_t size;
	bus_addr_t addr;
	void *ih;

	iot = obio->obio_iot;
	addr = GEMINI_GLOBAL_BASE;
	size = 4096;		/* XXX */

	if (bus_space_map(iot, addr, size, 0, &ioh))
                panic("%s: Cannot map registers", device_xname(self));

	/*
	 * NOTE: we are using IPL_NET, not IPL_HIGH use of IPI on this system
	 * is (mainly) networking keep simple (for now) and force all IPIs
	 * to same level so splnet() can block them as any other NIC.
	 */
#if 0
	ih = intr_establish(obio->obio_intr, IPL_NET, IST_LEVEL_HIGH,
		gemini_ipiintr, sc);
#else
	ih = intr_establish(obio->obio_intr, IPL_NET, IST_EDGE_RISING,
		gemini_ipiintr, sc);
#endif
	if (ih == NULL)
		panic("%s: Cannot establish interrupt %d\n",
			device_xname(self), obio->obio_intr);

	SIMPLEQ_INIT(&sc->sc_intrq);

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_addr = addr;
	sc->sc_size = size;
	sc->sc_intr = obio->obio_intr;
	sc->sc_ih = ih;

	gemini_ipi_sc = sc;

	aprint_normal("\n");
	aprint_naive("\n");

#if NGEMINIIPM > 0
	config_found(self, __UNCONST("geminiipm"), NULL);
#endif
}

static inline int
gemini_ipi_intrq_empty(gemini_ipi_softc_t *sc)
{
	return SIMPLEQ_EMPTY(&sc->sc_intrq);
}

static inline void *
gemini_ipi_intrq_insert(gemini_ipi_softc_t *sc, int (*func)(void *), void *arg)
{
	gemini_ipi_intrq_t *iqp;

        iqp = malloc(sizeof(*iqp), M_DEVBUF, M_NOWAIT|M_ZERO);
        if (iqp == NULL) {
		printf("gemini_ipi_intrq_insert: malloc failed\n");
		return NULL;
	}

        iqp->iq_func = func;
        iqp->iq_arg = arg;
        SIMPLEQ_INSERT_TAIL(&sc->sc_intrq, iqp, iq_q);

	return (void *)iqp;
}

static inline void
gemini_ipi_intrq_remove(gemini_ipi_softc_t *sc, void *cookie)
{
	gemini_ipi_intrq_t *iqp;

	SIMPLEQ_FOREACH(iqp, &sc->sc_intrq, iq_q) {
		if ((void *)iqp == cookie) {
			SIMPLEQ_REMOVE(&sc->sc_intrq,
				iqp, gemini_ipi_intrq, iq_q);
			free(iqp, M_DEVBUF);
			return;
		}
	}
}

static inline int
gemini_ipi_intrq_dispatch(gemini_ipi_softc_t *sc)
{
	gemini_ipi_intrq_t *iqp;
	int rv = 0;

	SIMPLEQ_FOREACH(iqp, &sc->sc_intrq, iq_q)
		rv |= (*iqp->iq_func)(iqp->iq_arg);

	return (rv != 0);
}


void *
ipi_intr_establish(int (*func)(void *), void *arg)
{
        gemini_ipi_softc_t *sc = gemini_ipi_sc;
	void *ih;

	if (sc == NULL)
		return NULL;

	ih = gemini_ipi_intrq_insert(sc, func, arg);
#ifdef DEBUG
        if (ih == NULL)
		panic("%s: gemini_ipi_intrq_insert failed",
			device_xname(&sc->sc_dev));
#endif

	return ih;
}

void
ipi_intr_disestablish(void *ih)
{
        gemini_ipi_softc_t *sc = gemini_ipi_sc;

	if (sc == NULL)
		panic("%s: NULL gemini_ipi_sc", device_xname(&sc->sc_dev));

        gemini_ipi_intrq_remove(sc, ih);
}

int
ipi_send(void)
{
        gemini_ipi_softc_t *sc = gemini_ipi_sc;
	uint32_t r;
	uint32_t bit;
	bus_addr_t off;

	if (sc == NULL)
		return -1;

#if defined(GEMINI_MASTER)
	off = GEMINI_GLOBAL_CPU0;
	bit = GLOBAL_CPU0_IPICPU1;
#elif defined(GEMINI_SLAVE)
	off = GEMINI_GLOBAL_CPU1;
	bit = GLOBAL_CPU1_IPICPU0;
#endif

	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, off);
	r |= bit;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, off, r);

	return 0;
}

static inline void
ipi_ack(gemini_ipi_softc_t *sc)
{
	uint32_t r;
	uint32_t bit;
	bus_addr_t off;

#if defined(GEMINI_MASTER)
	off = GEMINI_GLOBAL_CPU1;
	bit = GLOBAL_CPU1_IPICPU0;
#elif defined(GEMINI_SLAVE) 
	off = GEMINI_GLOBAL_CPU0;
	bit = GLOBAL_CPU0_IPICPU1;
#endif

	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, off);
	r &= ~bit;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, off, r);
}

static int
gemini_ipiintr(void *arg)
{
	gemini_ipi_softc_t *sc = arg;
	int rv;

	if (sc == NULL)
		return -1;

	ipi_ack(sc);

	rv = gemini_ipi_intrq_dispatch(sc);

	return rv;
}
