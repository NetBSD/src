/*	$NetBSD: if_le_isapnp.c,v 1.2 1997/03/31 20:36:38 jonathan Exp $	*/

/*
 * Copyright (c) 1997 Jonathan Stone <jonathan@NetBSD.org> and 
 * Mattias Drochner. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Jonathan Stone.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>

#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>
#include <dev/isapnp/if_levar.h>

#ifdef __alpha__			/* XXX */
/* XXX XXX NEED REAL DMA MAPPING SUPPORT XXX XXX */ 
#undef vtophys
#define	vtophys(va)	alpha_XXX_dmamap((vm_offset_t)(va))
#endif



#ifdef __BROKEN_INDIRECT_CONFIG
int le_isapnp_match __P((struct device *, void *, void *));
#else
int le_isapnp_match __P((struct device *, struct cfdata *, void *));
#endif
void le_isapnp_attach __P((struct device *, struct device *, void *));

struct cfattach ep_isapnp_ca = {
	sizeof(struct le_softc), le_isapnp_match, le_isapnp_attach
};

int	le_isapnp_intredge __P((void *));
static void le_isapnp_wrcsr __P((struct am7990_softc *, u_int16_t, u_int16_t));
static u_int16_t le_isapnp_rdcsr __P((struct am7990_softc *, u_int16_t));


/*
 * Names accepted by the match routine.
 */
static char *if_le_isapnp_devnames[] = {
    "TKN0010",
    0
};


static void
le_isapnp_wrcsr(sc, port, val)
	struct am7990_softc *sc;
	u_int16_t port, val;
{
	struct le_softc *lesc = (struct le_softc *)sc;
	bus_space_tag_t iot = lesc->sc_iot;
	bus_space_handle_t ioh = lesc->sc_ioh;

	bus_space_write_2(iot, ioh, lesc->sc_rap, port);
	bus_space_write_2(iot, ioh, lesc->sc_rdp, val);
}

static u_int16_t
le_isapnp_rdcsr(sc, port)
	struct am7990_softc *sc;
	u_int16_t port;
{
	struct le_softc *lesc = (struct le_softc *)sc;
	bus_space_tag_t iot = lesc->sc_iot;
	bus_space_handle_t ioh = lesc->sc_ioh;
	u_int16_t val;

	bus_space_write_2(iot, ioh, lesc->sc_rap, port);
	val = bus_space_read_2(iot, ioh, lesc->sc_rdp);
	return (val);
}

int
le_isapnp_match(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct isapnp_attach_args *ipa = aux;
	char **card_name = &if_le_isapnp_devnames[0];

	while (*card_name)
	    if(!strcmp(ipa->ipa_devlogic, *card_name++))
		return(1);

	return (1);
}

void
le_isapnp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct le_softc *lesc = (void *)self;
	struct am7990_softc *sc = &lesc->sc_am7990;
	struct isapnp_attach_args *ipa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int i;


	lesc->sc_iot = iot = ipa->ipa_iot;
	lesc->sc_ioh = ioh = ipa->ipa_io[0].h;

	lesc->sc_rap = PCNET_RAP;
	lesc->sc_rdp = PCNET_RDP;

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf("%s: error in region allocation\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Extract the physical MAC address from the ROM.
	 */
	for (i = 0; i < sizeof(sc->sc_enaddr); i++)
		sc->sc_enaddr[i] = bus_space_read_1(iot, ioh, PCNET_SAPROM+i);

	/* XXX SHOULD GET DMA-CAPABLE BUFFER SPACE */
	sc->sc_mem = malloc(16384, M_DEVBUF, M_NOWAIT);
	if (sc->sc_mem == 0) {
		printf("%s: couldn't allocate memory for card\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_conf3 = 0;
	sc->sc_addr = kvtop(sc->sc_mem);	/* XXX XXX XXX !! */
	sc->sc_memsize = 16384;

	sc->sc_copytodesc = am7990_copytobuf_contig;
	sc->sc_copyfromdesc = am7990_copyfrombuf_contig;
	sc->sc_copytobuf = am7990_copytobuf_contig;
	sc->sc_copyfrombuf = am7990_copyfrombuf_contig;
	sc->sc_zerobuf = am7990_zerobuf_contig;

	sc->sc_rdcsr = le_isapnp_rdcsr;
	sc->sc_wrcsr = le_isapnp_wrcsr;
	sc->sc_hwinit = NULL;

	printf("%s: %s %s\n", sc->sc_dev.dv_xname, ipa->ipa_devident,
	    ipa->ipa_devclass);
	am7990_config(sc);

	if (ipa->ipa_ndrq > 0)
		isa_dmacascade(ipa->ipa_drq[0].num);

	lesc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
	    IST_EDGE, IPL_NET, le_isapnp_intredge, sc);
}


/*
 * Controller interrupt.
 */
int
le_isapnp_intredge(arg)
	void *arg;
{

	if (am7990_intr(arg) == 0)
		return (0);
	for (;;)
		if (am7990_intr(arg) == 0)
			return (1);
}
