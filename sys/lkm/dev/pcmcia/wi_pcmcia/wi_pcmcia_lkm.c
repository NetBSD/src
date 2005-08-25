/* $NetBSD: wi_pcmcia_lkm.c,v 1.2 2005/08/25 15:08:06 drochner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/lkm.h>

#ifndef WI_DRIVER_IS_PRESENT
CFDRIVER_DECL(wi, DV_DULL, NULL);
#endif

extern struct cfdriver wi_cd;
extern struct cfattach wi_pcmcia_ca;
extern struct cfiattrdata pcmciacf_iattrdata;

static int pcmcialoc[] = { -1, -1 }; /* function, irq */
static struct cfparent pcmciaparent = {
	"pcmcia", "pcmcia", DVUNIT_ANY
};
static struct cfdata wi_pcmcia_cfdata[] = {
	{"wi", "wi_pcmcia", 0, FSTATE_STAR, pcmcialoc, 0, &pcmciaparent},
	{ 0 }
};

static struct cfdriver *wi_pcmcia_cfdrivers[] = {
	&wi_cd,
	NULL
};
static struct cfattach *wi_pcmcia_cfattachs[] = {
	&wi_pcmcia_ca,
	NULL
};
static const struct cfattachlkminit wi_pcmcia_cfattachinit[] = {
	{ "wi", wi_pcmcia_cfattachs },
	{ NULL }
};

int wi_pcmcia_lkmentry(struct lkm_table *, int, int);
MOD_DRV("wi_pcmcia", wi_pcmcia_cfdrivers, wi_pcmcia_cfattachinit,
	wi_pcmcia_cfdata);

int
wi_pcmcia_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{

	DISPATCH(lkmtp, cmd, ver, lkm_nofunc, lkm_nofunc, lkm_nofunc);
}
