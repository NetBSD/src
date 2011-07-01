/*	$NetBSD: gemini_lpchc.c,v 1.2 2011/07/01 19:32:28 dyoung Exp $	*/

/*
 * GEMINI LPC Host Controller
 */
#include "opt_gemini.h"
#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gemini_lpchc.c,v 1.2 2011/07/01 19:32:28 dyoung Exp $");

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/param.h>
#include <sys/bus.h>

#include <arm/gemini/gemini_lpchcvar.h>
#include <arm/gemini/gemini_lpcvar.h>
#include <arm/gemini/gemini_reg.h>

static inline void
gemini_lpchc_sirq_cfg(bus_space_tag_t iot, bus_space_handle_t ioh,
	bus_size_t offset, uint32_t bit, boolean_t doset)
{
	uint32_t r;

	r = bus_space_read_4(iot, ioh, offset);
	if (doset)
		r |= bit;
	else
		r &= ~bit;
	bus_space_write_4(iot, ioh, offset, r);
}

static inline void
gemini_lpchc_sirq_ack(bus_space_tag_t iot, bus_space_handle_t ioh,
	uint32_t bit)
{
	uint32_t r;

	r = bus_space_read_4(iot, ioh, GEMINI_LPCHC_SERIRQSTS);
	r &= bit;
	if (r != 0)
		bus_space_write_4(iot, ioh, GEMINI_LPCHC_SERIRQSTS, r);
}

static inline void
gemini_lpchc_sirq_disable(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	uint32_t r;

	r = bus_space_read_4(iot, ioh, GEMINI_LPCHC_IRQCTL);
	r &= ~LPCHC_IRQCTL_SIRQEN;
	bus_space_write_4(iot, ioh, GEMINI_LPCHC_IRQCTL, r);
}

static inline void
gemini_lpchc_sirq_enable(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	uint32_t r;

	r = bus_space_read_4(iot, ioh, GEMINI_LPCHC_IRQCTL);
	r |= LPCHC_IRQCTL_SIRQEN;
	r |= LPCHC_IRQCTL_SIRQMS;	/* XXX "continuous mode" */
	r |= IRQCTL_SIRQFW_8;		/* XXX SIRW Frame Width 8 clocks */
	bus_space_write_4(iot, ioh, GEMINI_LPCHC_IRQCTL, r);
#if 0
	delay(10);			/* wait 1 serial IRQ cycle */
	r &= ~LPCHC_IRQCTL_SIRQMS;	/* XXX "quiet mode" */
	bus_space_write_4(iot, ioh, GEMINI_LPCHC_IRQCTL, r);
#endif
}

static inline void
gemini_lpchc_intrq_init(gemini_lpchc_softc_t *sc)
{
	SIMPLEQ_INIT(&sc->sc_intrq);
}

static inline int
gemini_lpchc_intrq_empty(gemini_lpchc_softc_t *sc)
{
	return SIMPLEQ_EMPTY(&sc->sc_intrq);
}

static inline void *
gemini_lpchc_intrq_insert(gemini_lpchc_softc_t *sc, int (*func)(void *),
	void *arg, uint32_t bit, boolean_t isedge)
{
	gemini_lpchc_intrq_t *iqp;

        iqp = malloc(sizeof(*iqp), M_DEVBUF, M_NOWAIT|M_ZERO);
        if (iqp == NULL) {
		printf("gemini_lpchc_intrq_insert: malloc failed\n");
		return NULL;
	}

        iqp->iq_func = func;
        iqp->iq_arg = arg;
        iqp->iq_bit = bit;
        iqp->iq_isedge = isedge;
        SIMPLEQ_INSERT_TAIL(&sc->sc_intrq, iqp, iq_q);

	return (void *)iqp;
}

static inline void
gemini_lpchc_intrq_remove(gemini_lpchc_softc_t *sc, void *cookie)
{
	gemini_lpchc_intrq_t *iqp;

	SIMPLEQ_FOREACH(iqp, &sc->sc_intrq, iq_q) {
		if ((void *)iqp == cookie) {
			SIMPLEQ_REMOVE(&sc->sc_intrq,
				iqp, gemini_lpchc_intrq, iq_q);
			free(iqp, M_DEVBUF);
			return;
		}
	}
}

static inline int
gemini_lpchc_intrq_dispatch(gemini_lpchc_softc_t *sc)
{
	gemini_lpchc_intrq_t *iqp;
	uint32_t r;
	int rv = 0;

	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GEMINI_LPCHC_SERIRQSTS);
	r &= ~LPCHC_SERIRQSTS_RESV;
	SIMPLEQ_FOREACH(iqp, &sc->sc_intrq, iq_q) {
		if ((r & iqp->iq_bit) != 0) {
			if (iqp->iq_isedge) {
				gemini_lpchc_sirq_ack(sc->sc_iot, sc->sc_ioh,
					iqp->iq_bit);
			}
			rv |= (*iqp->iq_func)(iqp->iq_arg);
		}
	}
	return (rv != 0);
}

void
gemini_lpchc_init(lpcintrtag_t tag)
{
	gemini_lpchc_softc_t *sc = tag;
	uint32_t r;

	gemini_lpchc_intrq_init(sc);

	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GEMINI_LPCHC_CSR);
	r |= LPCHC_CSR_BEN;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GEMINI_LPCHC_CSR, r);
}

void *
gemini_lpchc_intr_establish(lpcintrtag_t tag, uint irq,
	int ipl, int type, int (*func)(void *), void *arg)
{
	gemini_lpchc_softc_t *sc = tag;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	uint32_t bit;
	boolean_t isedge;
	boolean_t ishigh;
	void *ih;

	isedge = ((type == IST_EDGE_RISING) || (type == IST_EDGE_FALLING));
	ishigh = ((type == IST_EDGE_RISING) || (type == IST_LEVEL_HIGH));

	if (irq >= GEMINI_LPCHC_NSERIRQ) {
		printf("%s: bad irq %d\n", __FUNCTION__, irq);
		return NULL;
	}
#if 0
	bit = 1 << irq;
#else
	bit = (1 << GEMINI_LPCHC_NSERIRQ) -1;	/* XXX */
#endif

	/* set IRQ type */
	gemini_lpchc_sirq_cfg(iot, ioh, GEMINI_LPCHC_SERIRQTYP,
		bit, isedge);

	/* set IRQ polarity */
	gemini_lpchc_sirq_cfg(iot, ioh, GEMINI_LPCHC_SERIRQPOLARITY, 
		bit, ishigh);

	/* ack a-priori edge status */
	if (isedge)
		gemini_lpchc_sirq_ack(iot, ioh, bit);

	if (gemini_lpchc_intrq_empty(sc))
		gemini_lpchc_sirq_enable(iot, ioh);

	ih = gemini_lpchc_intrq_insert(sc, func, arg, bit, isedge); 
	if (ih == NULL)
		if (gemini_lpchc_intrq_empty(sc))
			gemini_lpchc_sirq_disable(iot, ioh);

	return ih;
}

void
gemini_lpchc_intr_disestablish(lpcintrtag_t tag, void *ih)
{
	gemini_lpchc_softc_t *sc = tag;

	gemini_lpchc_intrq_remove(sc, ih);
	if (gemini_lpchc_intrq_empty(sc))
		gemini_lpchc_sirq_disable(sc->sc_iot, sc->sc_ioh);
}

int
gemini_lpchc_intr(void *arg)
{
	gemini_lpchc_softc_t *sc = arg;
	int rv;

printf("%s: enter\n", __FUNCTION__);
	rv = gemini_lpchc_intrq_dispatch(sc);
printf("%s: exit\n", __FUNCTION__);

	return rv;
}

