/*	$NetBSD: aic_pcmcia.c,v 1.1.2.11 1997/10/16 09:34:38 enami Exp $	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/select.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/scsi_all.h>

#include <dev/ic/aic6360var.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>

#define	PCMCIA_MANUFACTURER_ADAPTEC	0x012F
#define	PCMCIA_PRODUCT_ADAPTEC_APA1460	0x0001

#ifdef __BROKEN_INDIRECT_CONFIG
int	aic_pcmcia_match __P((struct device *, void *, void *));
#else
int	aic_pcmcia_match __P((struct device *, struct cfdata *, void *));
#endif
void	aic_pcmcia_attach __P((struct device *, struct device *, void *));

struct aic_pcmcia_softc {
	struct aic_softc sc_aic;		/* real "aic" softc */

	/* PCMCIA-specific goo. */
	struct pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	int sc_io_window;			/* our i/o window */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
	void *sc_ih;				/* interrupt handler */
};

struct cfattach aic_pcmcia_ca = {
	sizeof(struct aic_pcmcia_softc), aic_pcmcia_match, aic_pcmcia_attach
};

int
aic_pcmcia_match(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *cf;
#endif
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if ((pa->manufacturer == PCMCIA_MANUFACTURER_ADAPTEC) &&
	    (pa->product == PCMCIA_PRODUCT_ADAPTEC_APA1460) &&
	    (pa->pf->number == 0))
		return (1);

	return (0);
}

void
aic_pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct aic_pcmcia_softc *psc = (void *)self;
	struct aic_softc *sc = &psc->sc_aic;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct pcmcia_function *pf = pa->pf;

	psc->sc_pf = pf;

	for (cfe = pf->cfe_head.sqh_first; cfe; cfe = cfe->cfe_list.sqe_next) {
		if (cfe->num_memspace != 0 ||
		    cfe->num_iospace != 1)
			continue;

		if (pcmcia_io_alloc(pa->pf, cfe->iospace[0].start,
		    cfe->iospace[0].length, 0, &psc->sc_pcioh) == 0)
			break;
	}

	if (cfe == 0) {
		printf(": can't alloc i/o space\n");
		return;
	}

	sc->sc_iot = psc->sc_pcioh.iot;
	sc->sc_ioh = psc->sc_pcioh.ioh;

	/* Enable the card. */
	pcmcia_function_init(pf, cfe);
	if (pcmcia_function_enable(pf)) {
		printf(": function enable failed\n");
		return;
	}

	/* Map in the io space */
	if (pcmcia_io_map(pa->pf, PCMCIA_WIDTH_AUTO, 0, psc->sc_pcioh.size,
	    &psc->sc_pcioh, &psc->sc_io_window)) {
		printf(": can't map i/o space\n");
		return;
	}

	if (!aic_find(sc->sc_iot, sc->sc_ioh))
		printf(": coundn't find aic\n%s", sc->sc_dev.dv_xname);

	printf(": APA-1460 SCSI Host Adapter\n");

	aicattach(sc);

	/* Establish the interrupt handler. */
	psc->sc_ih = pcmcia_intr_establish(pa->pf, IPL_BIO, aicintr, sc);
	if (psc->sc_ih == NULL)
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
}
