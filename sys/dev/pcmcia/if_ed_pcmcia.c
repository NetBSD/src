#include <sys/param.h>
#include <sys/systm.h>
#include <sys/select.h>
#include <sys/device.h>
#include <sys/socket.h>

#include <net/if_types.h>
#include <net/if.h>
#include <net/if_ether.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>

#include <dev/ic/dp8390reg.h>
/* XXX these will move to dev/ic */
#include <dev/isa/if_edreg.h>
#include <dev/isa/if_edvar.h>

#ifdef __BROKEN_INDIRECT_CONFIG
int ed_pcmcia_match __P((struct device *, void *, void *));
#else
int ed_pcmcia_match __P((struct device *, struct cfdata *, void *));
#endif
void ed_pcmcia_attach __P((struct device *, struct device *, void *));

struct cfattach ed_pcmcia_ca = {
    sizeof(struct ed_softc), ed_pcmcia_match, ed_pcmcia_attach
};

struct ne2000dev {
    char *name;
    int manufacturer;
    int product;
    char *cis1_info0;
    char *cis1_info1;
    int function;
    int enet_maddr;
    unsigned char enet_vendor[3];
} ne2000devs[] = {
    { "PreMax PE-200",
      0xffff, 0xffff, "PMX   ", "PE-200", 0,
      0x07f0, { 0x00, 0x20, 0xe0 } },
    { "National Semiconductor InfoMover",
      0x00a4, 0x0002, NULL, NULL, 0,
      0x0ff0, { 0x08, 0x00, 0x5a } },
#if 0
    /* the rest of these are stolen from the linux pcnet pcmcia device
       driver.  Since I don't know the manfid or cis info strings for
       any of them, they're not compiled in until I do. */

    { "Accton EN2212", 
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0x00, 0xe8 } },
    { "Allied Telesis LA-PCM",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0x00, 0xf4 } },
    { "APEX MultiCard",
      0x0000, 0x0000, NULL, NULL, 0,
      0x03f4, { 0x00, 0x20, 0xe5 } },
    { "ASANTE FriendlyNet",
      0x0000, 0x0000, NULL, NULL, 0,
      0x4910, { 0x00, 0x00, 0x94 } },
    { "Danpex EN-6200P2",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0110, { 0x00, 0x40, 0xc7 } },
    { "DataTrek NetCard",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0x20, 0xe8 } },
    { "Dayna CommuniCard E",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0110, { 0x00, 0x80, 0x19 } },
    { "D-Link DE-650",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0040, { 0x00, 0x80, 0xc8 } },
    { "EP-210 Ethernet",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0110, { 0x00, 0x40, 0x33 } },
    { "Epson EEN10B",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0x00, 0x48 } },
    { "ELECOM Laneed LD-CDWA",
      0x0000, 0x0000, NULL, NULL, 0,
      0x00b8, { 0x08, 0x00, 0x42 } },
    { "Grey Cell GCS2220",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0000, { 0x00, 0x47, 0x43 } },
    { "Hypertec Ethernet",
      0x0000, 0x0000, NULL, NULL, 0,
      0x01c0, { 0x00, 0x40, 0x4c } },
    { "IBM CCAE",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x08, 0x00, 0x5a } },
    { "IBM CCAE",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0x04, 0xac } },
    { "IBM CCAE",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0x06, 0x29 } },
    { "IBM FME",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0374, { 0x00, 0x04, 0xac } },
    { "IBM FME",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0374, { 0x08, 0x00, 0x5a } },
    { "I-O DATA PCLA/T",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0xa0, 0xb0 } },
    { "Katron PE-520",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0110, { 0x00, 0x40, 0xf6 } },
    { "Kingston KNE-PCM/x",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0xc0, 0xf0 } },
    { "Kingston KNE-PCM/x",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0xe2, 0x0c, 0x0f } },
    { "Kingston KNE-PC2",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0180, { 0x00, 0xc0, 0xf0 } },
    { "Longshine LCS-8534",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0000, { 0x08, 0x00, 0x00 } },
    { "Maxtech PCN2000",
      0x0000, 0x0000, NULL, NULL, 0,
      0x5000, { 0x00, 0x00, 0xe8 } },
    { "NDC Instant-Link",
      0x0000, 0x0000, NULL, NULL, 0,
      0x003a, { 0x00, 0x80, 0xc6 } },
    { "NE2000 Compatible",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0xa0, 0x0c } },
    { "Network General Sniffer",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0x00, 0x65 } },
    { "Panasonic VEL211",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0x80, 0x45 } },
    { "RPTI EP400",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0110, { 0x00, 0x40, 0x95 } },
    { "SCM Ethernet",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0ff0, { 0x00, 0x20, 0xcb } },
    { "Socket EA",
      0x0000, 0x0000, NULL, NULL, 0,
      0x4000, { 0x00, 0xc0, 0x1b } },
    { "Volktek NPL-402CT",
      0x0000, 0x0000, NULL, NULL, 0,
      0x0060, { 0x00, 0x40, 0x05 } },
#endif
};

#define ne2000_match(card, fct, n) \
((((((card)->manufacturer == ne2000devs[(n)].manufacturer) && \
    ((card)->product == ne2000devs[(n)].product)) || \
   ((ne2000devs[(n)].cis1_info0) && (ne2000devs[(n)].cis1_info1) && \
    (strcmp((card)->cis1_info[0], ne2000devs[(n)].cis1_info0) == 0) && \
    (strcmp((card)->cis1_info[1], ne2000devs[(n)].cis1_info1) == 0))) && \
  (pa->pf->number == ne2000devs[(n)].function))? \
 &ne2000devs[(n)]:NULL)

int
ed_pcmcia_match(parent, match, aux)
     struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
     void *match;
#else
     struct cfdata *cf;
#endif
     void *aux;
{
    struct pcmcia_attach_args *pa = (struct pcmcia_attach_args *) aux;
    int i;

    for (i=0; i<(sizeof(ne2000devs)/sizeof(ne2000devs[0])); i++) {
	if (ne2000_match(pa->card, pa->pf->number, i))
	    return(1);
    }

    return(0);
}

void
ed_pcmcia_attach(parent, self, aux)
     struct device *parent, *self;
     void *aux;
{
    struct ed_softc *sc = (void *) self;
    struct pcmcia_attach_args *pa = aux;
    struct pcmcia_config_entry *cfe;
    int i;
    struct ne2000dev *ne_dev;

    cfe = pa->pf->cfe_head.sqh_first;

#if 0
    /* Some ne2000 driver's claim to have memory; others don't.
       Since I don't care, I don't check. */

    if (cfe->num_memspace != 1) {
	printf(": unexpected number of memory spaces %d should be 1\n",
	       cfe->num_memspace);
	return;
    }
#endif
     
    if (cfe->num_iospace == 1) {
	if (cfe->iospace[0].length != ED_NOVELL_IO_PORTS) {
	    printf(": unexpected I/O space configuration\n");
	    return;
	}
    } else if (cfe->num_iospace == 2) {
	/* The EFA Corp card claims io space 0x300-0x30f and
	   0x310-0x31f.  This is consistent with the separate NIC and
	   ASIC port spaces, but the MI driver only knows about one
	   space, so these need to be adjacent and in the right order,
	   although their location is unimportant.  Allocate
	   ED_NOVELL_IO_PORTS bytes of space, and map both spaces
	   there. */

	if ((cfe->iospace[0].length != ED_NOVELL_NIC_PORTS) ||
	    (cfe->iospace[1].length != ED_NOVELL_ASIC_PORTS)) {
	    printf(": unexpected I/O space configuration\n");
	    return;
	}
    } else {
	printf(": unexpected number of i/o spaces %d should be 1 or 2\n",
	       cfe->num_iospace);
    }

    /* XXX start or 0? */

    if (pcmcia_io_alloc(pa->pf, cfe->iospace[0].start, ED_NOVELL_IO_PORTS, 
			&sc->sc_iot, &sc->sc_ioh)) {
	printf(": can't alloc i/o space\n");
	return;
    }

    /* Enable the card. */
    if (pcmcia_enable_function(pa->pf, parent, cfe)) {
	printf(": function enable failed\n");
	return;
    }

    /* XXX arithmetic on ioh is not correct.  I should use
       a subregion here. */

    /* some cards claim to be io16, but they're lying. */
    if (pcmcia_io_map(pa->pf, PCMCIA_WIDTH_IO8,
		      ED_NOVELL_NIC_PORTS, sc->sc_iot,
		      sc->sc_ioh + ED_NOVELL_NIC_OFFSET,
		      &sc->nic_pcmcia_window)) {
	printf(": can't map NIC i/o space\n");
	return;
    }

    if (pcmcia_io_map(pa->pf, PCMCIA_WIDTH_IO16,
		      ED_NOVELL_ASIC_PORTS, sc->sc_iot,
		      sc->sc_ioh + ED_NOVELL_ASIC_OFFSET,
		      &sc->asic_pcmcia_window)) {
	printf(": can't map ASIC i/o space\n");
	return;
    }

    if (!ed_find_Novell(sc, sc->sc_dev.dv_cfdata, sc->sc_iot,
			sc->sc_ioh)) {
	printf(": chip attach failed\n");
	return;
    }

    /* find_Novell tried to read the mac addr off the board, but
       pcmcia does things differently. */

    for (i=0; i<(sizeof(ne2000devs)/sizeof(ne2000devs[0])); i++) {
	if ((ne_dev = ne2000_match(pa->card, pa->pf->number, i))) {
	    bus_space_tag_t enaddrt;
	    bus_space_handle_t enaddrh;
	    u_long offset;
	    int mhandle, window;
	    int i;

	    if (pcmcia_mem_alloc(pa->pf, ETHER_ADDR_LEN*2, &enaddrt,
				 &enaddrh, &mhandle, NULL)) {
		printf(": allocating mem for enet addr failed\n");
		return;
	    }
	    if (pcmcia_mem_map(pa->pf, PCMCIA_MEM_ATTR, ETHER_ADDR_LEN*2,
			       enaddrt, enaddrh, ne_dev->enet_maddr,
			       &offset, &window)) {
		printf(": mapping mem for enet addr failed\n");
		return;
	    }

	    for (i=0; i<ETHER_ADDR_LEN; i++)
		sc->sc_enaddr[i] = bus_space_read_1(enaddrt, enaddrh,
						    offset+i*2);

	    pcmcia_mem_unmap(pa->pf, window);
	    pcmcia_mem_free(pa->pf, ETHER_ADDR_LEN*2, enaddrt, enaddrh,
			    mhandle);

	    if ((sc->sc_enaddr[0] != ne_dev->enet_vendor[0]) ||
		(sc->sc_enaddr[1] != ne_dev->enet_vendor[1]) ||
		(sc->sc_enaddr[2] != ne_dev->enet_vendor[2])) {
		printf(": enet addr has incorrect vendor code (%02x:%02x:%02x should be %02x:%02x:%02x\n",
		       sc->sc_enaddr[0], sc->sc_enaddr[1], sc->sc_enaddr[2],
		       ne_dev->enet_vendor[0], ne_dev->enet_vendor[1],
		       ne_dev->enet_vendor[2]);
		return;
	    }

	    printf(": %s NE2000-compatible ethernet", ne_dev->name);

	    break;
	}
    }

    /* this prints a leading newline, don't know why */
    edattach(self);

    if (!(sc->sc_ih = pcmcia_intr_establish(pa->pf, IPL_NET, edintr, sc))) {
	printf("%s: couldn't establish interrupt\n", sc->sc_dev.dv_xname);
	return;
    }

    /* set up the interrupt */

    return;
}
