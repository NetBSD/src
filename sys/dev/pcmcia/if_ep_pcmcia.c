#include "bpfilter.h" 
 
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h> 
#include <sys/socket.h> 
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h> 
#include <netinet/if_inarp.h>
#endif
 
#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif
  
#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/elink3var.h>
#include <dev/ic/elink3reg.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>

#define PCMCIA_MANUFACTURER_3COM	0x101
#define PCMCIA_PRODUCT_3COM_3C562	0x562
#define PCMCIA_PRODUCT_3COM_3C589	0x589

#ifdef __BROKEN_INDIRECT_CONFIG
int ep_pcmcia_match __P((struct device *, void *, void *));
#else
int ep_pcmcia_match __P((struct device *, struct cfdata *, void *));
#endif
void ep_pcmcia_attach __P((struct device *, struct device *, void *));

int ep_pcmcia_get_enaddr(struct pcmcia_tuple *, void *);

struct cfattach ep_pcmcia_ca = {
    sizeof(struct ep_softc), ep_pcmcia_match, ep_pcmcia_attach
};

int
ep_pcmcia_match(parent, match, aux)
     struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
     void *match;
#else
     struct cfdata *cf;
#endif
     void *aux;
{
    struct pcmcia_attach_args *pa = (struct pcmcia_attach_args *) aux;

    printf("ep_pcmcia_match: %x %x %d\n", pa->manufacturer,
	    pa->product, pa->pf->number);

    if ((pa->manufacturer == PCMCIA_MANUFACTURER_3COM) &&
	(pa->product == PCMCIA_PRODUCT_3COM_3C562) &&
	(pa->pf->number == 0))
	return(1);

    if ((pa->manufacturer == PCMCIA_MANUFACTURER_3COM) &&
	(pa->product == PCMCIA_PRODUCT_3COM_3C589) &&
	(pa->pf->number == 0))
	return(1);

    return(0);
}

void
ep_pcmcia_attach(parent, self, aux)
     struct device *parent, *self;
     void *aux;
{
    struct ep_softc *sc = (void *) self;
    struct pcmcia_attach_args *pa = aux;
    struct pcmcia_config_entry *cfe;
    int i;
    u_int8_t myla[ETHER_ADDR_LEN];
    u_int8_t *enaddr;
    char *model;

    cfe = pa->pf->cfe_head.sqh_first;

    /* Enable the card. */
    if (pcmcia_enable_function(pa->pf, parent, cfe))
	printf(": function enable failed\n");

    /* turn off the bit which disables the modem */
    if (pa->product == PCMCIA_PRODUCT_3COM_3C562) {
	int reg;

	reg = pcmcia_ccr_read(pa->pf, PCMCIA_CCR_OPTION);
	reg &= ~0x08;
	pcmcia_ccr_write(pa->pf, PCMCIA_CCR_OPTION, reg);
    }

    if (cfe->num_memspace != 0)
	printf(": unexpected number of memory spaces %d should be 0\n",
	       cfe->num_memspace);
     
    if (cfe->num_iospace != 1)
	printf(": unexpected number of I/O spaces %d should be 1\n",
	       cfe->num_iospace);

    /* XXX there's a comment in the linux driver about the io address
       having to be between 0x00 and 0x70 mod 0x100.  weird. */

    if (pa->product == PCMCIA_PRODUCT_3COM_3C562) {
	for (i = 0x300; i < 0x1000; i += ((i%0x100) == 0x70)?0x90:0x10) {
	    if (pcmcia_io_alloc(pa->pf, i, cfe->iospace[0].length,
				&sc->sc_iot, &sc->sc_ioh) == 0)
		break;
	}
	if (i == 0x1000) {
	    printf(": can't allocate i/o space\n");
	    return;
	}
    } else {
	if (pcmcia_io_alloc(pa->pf, 0, cfe->iospace[0].length,
			    &sc->sc_iot, &sc->sc_ioh))
	    printf(": can't allocate i/o space\n");
    }

    if (pcmcia_io_map(pa->pf, ((cfe->flags & PCMCIA_CFE_IO16)?
			       PCMCIA_WIDTH_IO16:PCMCIA_WIDTH_IO8),
		      cfe->iospace[0].length,
		      sc->sc_iot, sc->sc_ioh, &sc->pcmcia_io_window)) {
	printf(": can't map i/o space\n");
	return;
    }

    if (pa->product == PCMCIA_PRODUCT_3COM_3C562) {
	if (pcmcia_scan_cis(parent, ep_pcmcia_get_enaddr, myla)) {
	    printf(": can't read ethernet address from CIS\n");
	    return;
	}
	enaddr = myla;
    } else {
	enaddr = NULL;
    }

    sc->bustype = EP_BUS_PCMCIA;

    switch (pa->product) {
    case PCMCIA_PRODUCT_3COM_3C589:
	model = "3Com 3C589 Ethernet";
	break;
    case PCMCIA_PRODUCT_3COM_3C562:
	model = "3Com 3C562 Ethernet";
	break;
    default:
	model = "3Com Ethernet, model unknown";
	break;
    }

    printf(": %s\n", model);

    epconfig(sc, EP_CHIPSET_3C509, enaddr);

    /* establish the interrupt. */
    sc->sc_ih = pcmcia_intr_establish(pa->pf, IPL_NET, epintr, sc);
    if (sc->sc_ih == NULL) {
	printf("%s: couldn't establish interrupt\n",
	       sc->sc_dev.dv_xname);
	return;
    }
}

int
ep_pcmcia_get_enaddr(tuple, arg)
     struct pcmcia_tuple *tuple;
     void *arg;
{
    u_int8_t *myla = (u_int8_t *) arg;
    int i;

    /* this is 3c562 magic */

    if (tuple->code == 0x88) {
	if (tuple->length < ETHER_ADDR_LEN)
	    return(0);

	for (i=0; i<ETHER_ADDR_LEN; i+=2) {
	    myla[i] = pcmcia_tuple_read_1(tuple, i+1);
	    myla[i+1] = pcmcia_tuple_read_1(tuple, i);
	}

	return(1);
    }

    return(0);
}
