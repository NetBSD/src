#include <sys/param.h>
#include <sys/systm.h>
#include <sys/select.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>

#define PCMCIA_MANUFACTURER_ADAPTEC	0x12F
#define PCMCIA_PRODUCT_ADAPTEC_APA1460	0x1

struct aic_softc {
    struct device dev;

    bus_space_tag_t iot;
    bus_space_handle_t ioh;
    int pcmcia_window;
};

#ifdef __BROKEN_INDIRECT_CONFIG
int aic_pcmcia_match __P((struct device *, void *, void *));
#else
int aic_pcmcia_match __P((struct device *, struct cfdata *, void *));
#endif
void aic_pcmcia_attach __P((struct device *, struct device *, void *));

struct cfattach aic_pcmcia_ca = {
    sizeof(struct aic_softc), aic_pcmcia_match, aic_pcmcia_attach
};

struct cfdriver aicx_cd = {
	NULL, "aicx", DV_DULL
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
    struct pcmcia_attach_args *pa = (struct pcmcia_attach_args *) aux;

    printf("aic_pcmcia_match: %x %x %d\n", pa->manufacturer,
	    pa->product, pa->pf->number);

    if ((pa->manufacturer == PCMCIA_MANUFACTURER_ADAPTEC) &&
	(pa->product == PCMCIA_PRODUCT_ADAPTEC_APA1460) &&
	(pa->pf->number == 0))
	return(1);

    return(0);
}

void
aic_pcmcia_attach(parent, self, aux)
     struct device *parent, *self;
     void *aux;
{
/*     struct aic_softc *sc = (void *) self; */
    struct pcmcia_attach_args *pa = aux;
    struct pcmcia_config_entry *cfe;

    cfe = pa->pf->cfe_head.sqh_first;

    /* Enable the card. */
    if (pcmcia_enable_function(pa->pf, parent, cfe)) {
	printf(": function enable failed\n");
	return;
    }

    if (cfe->num_memspace != 0) {
	printf(": unexpected number of memory spaces %d should be 0\n",
	       cfe->num_memspace);
	return;
    }
     
    if (cfe->num_iospace != 1) {
	printf(": unexpected number of I/O spaces %d should be 1\n",
	       cfe->num_iospace);
	return;
    }

    printf(": APA-1460 SCSI Host Adapter (not really attached)\n");

    return;
}
