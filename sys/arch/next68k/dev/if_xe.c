/*	$NetBSD: if_xe.c,v 1.5 2001/03/31 06:56:54 dbj Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Darrin B. Jewell
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <next68k/next68k/isr.h>

#include <next68k/dev/mb8795reg.h>
#include <next68k/dev/mb8795var.h>

#include <next68k/dev/nextdmareg.h>
#include <next68k/dev/nextdmavar.h>

#include <next68k/dev/if_xevar.h>


int	xe_match __P((struct device *, struct cfdata *, void *));
void	xe_attach __P((struct device *, struct device *, void *));
int	xe_tint __P((void *));
int	xe_rint __P((void *));

struct cfattach xe_ca = {
	sizeof(struct xe_softc), xe_match, xe_attach
};

int
xe_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
  /* should probably probe here */
  /* Should also probably set up data from config */
  return (1);
}

void
xe_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
  struct xe_softc *xe_sc = (struct xe_softc *)self;
  struct mb8795_softc *sc = &xe_sc->sc_mb8795;


  {
    extern u_char rom_enetaddr[6];     /* kludge from machdep.c:next68k_bootargs() */
    int i;
    for(i=0;i<6;i++) {
      sc->sc_enaddr[i] = rom_enetaddr[i];
    }
  }

  printf("\n%s at MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
         sc->sc_dev.dv_xname,
         sc->sc_enaddr[0],sc->sc_enaddr[1],sc->sc_enaddr[2],
         sc->sc_enaddr[3],sc->sc_enaddr[4],sc->sc_enaddr[5]);

  /* @@@ This setup is really ugly.  Instead, we should be attaching
	 * the DMA controller all by itself and glue things together
	 * with config.
	 * Darrin B. Jewell <dbj@netbsd.org>  Sat May 16 16:04:34 1998
	 */

  sc->sc_bst = NEXT68K_INTIO_BUS_SPACE;

  if (bus_space_map(sc->sc_bst, NEXT_P_ENET,
                    XE_SIZE, 0, &sc->sc_bsh)) {
    printf("\n%s: can't map mb8795 registers\n",
           sc->sc_dev.dv_xname);
    return;
  }

  /* initialize rx and tx dma channels */
	xe_sc->sc_rxdma.nd_bst = NEXT68K_INTIO_BUS_SPACE;

	if (bus_space_map(xe_sc->sc_rxdma.nd_bst, NEXT_P_ENETR_CSR,
			DD_SIZE, 0, &xe_sc->sc_rxdma.nd_bsh)) {
		printf("\n%s: can't map ethernet receive DMA registers\n",
				sc->sc_dev.dv_xname);
		return;
	} 
  xe_sc->sc_rxdma.nd_intr = NEXT_I_ENETR_DMA;
	xe_sc->sc_rxdma.nd_continue_cb = NULL;
	xe_sc->sc_rxdma.nd_completed_cb = NULL;
	xe_sc->sc_rxdma.nd_shutdown_cb = NULL;
	xe_sc->sc_rxdma.nd_cb_arg = NULL;
  sc->sc_rx_dmat = &(xe_sc->sc_rxdma._nd_dmat);
	sc->sc_rx_nd = &(xe_sc->sc_rxdma);
	nextdma_config(sc->sc_rx_nd);

	xe_sc->sc_txdma.nd_bst = NEXT68K_INTIO_BUS_SPACE;
	if (bus_space_map(xe_sc->sc_txdma.nd_bst, NEXT_P_ENETX_CSR,
			DD_SIZE, 0, &xe_sc->sc_txdma.nd_bsh)) {
		printf("\n%s: can't map ethernet transmit DMA registers\n",
				sc->sc_dev.dv_xname);
		return;
	}
  xe_sc->sc_txdma.nd_intr = NEXT_I_ENETX_DMA;
	xe_sc->sc_txdma.nd_continue_cb = NULL;
	xe_sc->sc_txdma.nd_completed_cb = NULL;
	xe_sc->sc_txdma.nd_shutdown_cb = NULL;
	xe_sc->sc_txdma.nd_cb_arg = NULL;
  sc->sc_tx_dmat = &(xe_sc->sc_txdma._nd_dmat);
	sc->sc_tx_nd = &(xe_sc->sc_txdma);
	nextdma_config(sc->sc_tx_nd);

  mb8795_config(sc);

  isrlink_autovec(xe_tint, sc, NEXT_I_IPL(NEXT_I_ENETX), 1);
  INTR_ENABLE(NEXT_I_ENETX);
  isrlink_autovec(xe_rint, sc, NEXT_I_IPL(NEXT_I_ENETR), 1);
  INTR_ENABLE(NEXT_I_ENETR);

  booted_device = &(sc->sc_dev);
}


int
xe_tint(arg)
     void *arg;
{
  if (!INTR_OCCURRED(NEXT_I_ENETX)) return 0;
  mb8795_tint((struct mb8795_softc *)arg);
  return(1);
}

int
xe_rint(arg)
     void *arg;
{
  if (!INTR_OCCURRED(NEXT_I_ENETR)) return(0);
  mb8795_rint((struct mb8795_softc *)arg);
  return(1);
}
