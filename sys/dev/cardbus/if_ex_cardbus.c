/*	$NetBSD: if_ex_cardbus.c,v 1.4 1999/10/25 19:18:10 drochner Exp $	*/

/*
 * CardBus specific routines for 3Com 3C575-family CardBus ethernet adapter
 *
 * Copyright (c) 1998 and 1999
 *       HAYAKAWA Koichi.  All rights reserved.
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
 *      This product includes software developed by the author.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY HAYAKAWA KOICHI ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL TAKESHI OHASHI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 */


/* #define EX_DEBUG 4 */	/* define to report infomation for debugging */

#define EX_POWER_STATIC		/* do not use enable/disable functions */
				/* I'm waiting elinkxl.c uses
                                   sc->enable and sc->disable
                                   functions. */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/syslog.h>
/* #include <sys/signalvar.h> */
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/if_ether.h>

#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/bus.h>
#if defined pciinc
#include <dev/pci/pcidevs.h>
#endif
#if pccard
#include <dev/pccard/cardbusvar.h>
#include <dev/pccard/pccardcis.h>
#else
#include <dev/cardbus/cardbusvar.h>
#include <dev/cardbus/cardbusdevs.h>
#endif

#include <dev/mii/miivar.h>

#include <dev/ic/elink3var.h>
#include <dev/ic/elink3reg.h>
#include <dev/ic/elinkxlreg.h>
#include <dev/ic/elinkxlvar.h>

#if defined DEBUG && !defined EX_DEBUG
#define EX_DEBUG
#endif

#if defined EX_DEBUG
#define STATIC
#define DPRINTF(a) printf a
#else
#define STATIC static
#define DPRINTF(a)
#endif

#define CARDBUS_3C575BTX_FUNCSTAT_PCIREG  CARDBUS_BASE2_REG  /* means 0x18 */
#define EX_CB_INTR 4		/* intr acknowledge reg. CardBus only */
#define EX_CB_INTR_ACK 0x8000 /* intr acknowledge bit */


/* Definitions, most of them has turned out to be unneccesary, but here they
 * are anyway.
 */


STATIC int ex_cardbus_match __P((struct device *, struct cfdata *, void *));
STATIC void ex_cardbus_attach __P((struct device *, struct device *,void *));
STATIC int ex_cardbus_detach __P((struct device *, int));
STATIC void ex_cardbus_intr_ack __P((struct ex_softc *));

#if 0
static void expoll __P((void *arg));
#endif

#if !defined EX_POWER_STATIC
STATIC int ex_cardbus_enable __P((struct ex_softc *sc));
STATIC void ex_cardbus_disable __P((struct ex_softc *sc));
#endif /* !defined EX_POWER_STATIC */


struct ex_cardbus_softc {
  struct ex_softc sc_softc;

  cardbus_devfunc_t sc_ct;
  int sc_intrline;
  u_int8_t sc_cardbus_flags;
#define EX_REATTACH		0x01
#define EX_ABSENT		0x02
  u_int8_t sc_cardtype;
#define EX_3C575		1
#define EX_3C575B		2

  /* CardBus function status space.  575B requests it. */
  bus_space_tag_t sc_funct;
  bus_space_handle_t sc_funch;
};

struct cfattach ex_cardbus_ca = {
  sizeof(struct ex_cardbus_softc), ex_cardbus_match,
  ex_cardbus_attach, ex_cardbus_detach
};



STATIC int
ex_cardbus_match(parent, cf, aux)
     struct device *parent;
     struct cfdata *cf;
     void *aux;
{
  struct cardbus_attach_args *ca = aux;

  if ((CARDBUS_VENDOR(ca->ca_id) == CARDBUS_VENDOR_3COM)) {
    if (CARDBUS_PRODUCT(ca->ca_id) == CARDBUS_PRODUCT_3COM_3C575TX
	|| CARDBUS_PRODUCT(ca->ca_id) == CARDBUS_PRODUCT_3COM_3C575BTX) {
      return 1;
    }
  }
  return 0;
}






STATIC void
ex_cardbus_attach(parent, self, aux)
     struct device *parent;
     struct device *self;
     void *aux;
{
  struct ex_cardbus_softc *psc = (void *)self;
  struct ex_softc *sc = &psc->sc_softc;
  struct cardbus_attach_args *ca = aux;
  cardbus_devfunc_t ct = ca->ca_ct;
  cardbus_chipset_tag_t cc = ct->ct_cc;
  cardbus_function_tag_t cf = ct->ct_cf;
  cardbusreg_t iob, command, bhlc;
  bus_space_handle_t ioh;
  bus_addr_t adr;


  if (cardbus_mapreg_map(ct, CARDBUS_BASE0_REG, CARDBUS_MAPREG_TYPE_IO, 0,
			 &(sc->sc_iot), &ioh, &adr, NULL)) {
    panic("io alloc in ex_attach_cardbus\n");
  }
  iob = adr;
  sc->sc_ioh = ioh;

#if rbus
#else
  (ct->ct_cf->cardbus_io_open)(cc, 0, iob, iob + 0x40);
#endif
  (ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_IO_ENABLE);

  /* enable the card */
  command = cardbus_conf_read(cc, cf, ca->ca_tag, CARDBUS_COMMAND_STATUS_REG);

  /* Card specific configuration */
  switch (CARDBUS_PRODUCT(ca->ca_id)) {
  case CARDBUS_PRODUCT_3COM_3C575TX:
    psc->sc_cardtype = EX_3C575;
    command |= (CARDBUS_COMMAND_IO_ENABLE | CARDBUS_COMMAND_MASTER_ENABLE);
    printf(": 3Com 3C575TX (boomerang)\n");
    break;
  case CARDBUS_PRODUCT_3COM_3C575BTX:
    psc->sc_cardtype = EX_3C575B;
    command |= (CARDBUS_COMMAND_IO_ENABLE | 
		CARDBUS_COMMAND_MEM_ENABLE | CARDBUS_COMMAND_MASTER_ENABLE);

    /* Cardbus function status window */
    if (cardbus_mapreg_map(ct, CARDBUS_3C575BTX_FUNCSTAT_PCIREG,
			   CARDBUS_MAPREG_TYPE_MEM, 0,
			   &psc->sc_funct, &psc->sc_funch, 0, NULL)) {
      panic("mem alloc in ex_attach_cardbus\n");
    }

    /*
     * Make sure CardBus brigde can access memory space.  Usually
     * memory access is enabled by BIOS, but some BIOSes do not enable
     * it.
     */
    (ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_MEM_ENABLE);

    /* Setup interrupt acknowledge hook */
    sc->intr_ack = ex_cardbus_intr_ack;
    
    printf(": 3Com 3C575BTX (cyclone)\n");
    break;
  }

  cardbus_conf_write(cc, cf, ca->ca_tag, CARDBUS_COMMAND_STATUS_REG, command);
  
  /*
   * set latency timmer
   */
  bhlc = cardbus_conf_read(cc, cf, ca->ca_tag, CARDBUS_BHLC_REG);
  if (CARDBUS_LATTIMER(bhlc) < 0x20) {
    /* at least the value of latency timer should 0x20. */
    DPRINTF(("if_ex_cardbus: lattimer 0x%x -> 0x20\n",
	     CARDBUS_LATTIMER(bhlc)));
    bhlc &= ~(CARDBUS_LATTIMER_MASK << CARDBUS_LATTIMER_SHIFT);
    bhlc |= (0x20 << CARDBUS_LATTIMER_SHIFT);
    cardbus_conf_write(cc, cf, ca->ca_tag, CARDBUS_BHLC_REG, bhlc);
  }

  sc->sc_dmat = ca->ca_dmat;
  sc->ex_bustype = EX_BUS_CARDBUS;
  psc->sc_ct = ca->ca_ct;
  psc->sc_intrline = ca->ca_intrline;

  switch (psc->sc_cardtype) {
  case EX_3C575:
    sc->ex_conf = EX_CONF_MII|EX_CONF_INTPHY;
    break;
  case EX_3C575B:
    sc->ex_conf = EX_CONF_90XB|EX_CONF_MII|EX_CONF_INTPHY;
    break;
  }

#if !defined EX_POWER_STATIC
  sc->enable = ex_cardbus_enable;
  sc->disable = ex_cardbus_disable;
#else
  sc->enable = NULL;
  sc->disable = NULL;
#endif  
  sc->enabled = 1;


#if defined EX_POWER_STATIC
  /* Map and establish the interrupt. */

  sc->sc_ih = cardbus_intr_establish(cc, cf, ca->ca_intrline, IPL_NET,
				     ex_intr, psc);
  if (sc->sc_ih == NULL) {
    printf("%s: couldn't establish interrupt",
	   sc->sc_dev.dv_xname);
    printf(" at %d", ca->ca_intrline);
    printf("\n");
    return;
  }
  printf("%s: interrupting at %d\n", sc->sc_dev.dv_xname, ca->ca_intrline);
#endif

#if 0
  timeout(expoll, sc, hz/20);	/* XXX */
#endif

  bus_space_write_2(sc->sc_iot, sc->sc_ioh, ELINK_COMMAND, GLOBAL_RESET);
  delay(400);
  {
    int i = 0;
    while (bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, ELINK_STATUS) \
	   & S_COMMAND_IN_PROGRESS) {
      if (++i > 10000) {
	printf("ex: timeout %x\n", bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, ELINK_STATUS));
	printf("ex: addr %x\n", cardbus_conf_read(cc, cf, ca->ca_tag, CARDBUS_BASE0_REG));
	return;			/* emargency exit */
      }
    }
  }

  ex_config(sc); /* I'm BOOMERANG or CYCLONE */

  if (psc->sc_cardtype == EX_3C575B) {
    bus_space_write_4(psc->sc_funct, psc->sc_funch, EX_CB_INTR, EX_CB_INTR_ACK);
  }

#if !defined EX_POWER_STATIC
  cardbus_function_disable(psc->sc_ct);  
  sc->enabled = 0;
#endif
  return;
}



STATIC void
ex_cardbus_intr_ack(sc)
     struct ex_softc *sc;
{
  struct ex_cardbus_softc *psc = (struct ex_cardbus_softc *)sc;
  bus_space_write_4 (psc->sc_funct, psc->sc_funch, EX_CB_INTR, EX_CB_INTR_ACK);
}


STATIC int
ex_cardbus_detach(self, arg)
     struct device *self;
     int arg;
{
  struct ex_cardbus_softc *psc = (void *)self;
  struct ex_softc *sc = &psc->sc_softc;
  cardbus_function_tag_t cf = psc->sc_ct->ct_cf;
  cardbus_chipset_tag_t cc = psc->sc_ct->ct_cc;

  /*
   * XXX Currently, no detach.
   */
  printf("- ex_cardbus_detach\n");

  cardbus_intr_disestablish(cc, cf, sc->sc_ih);

  sc->enabled = 0;

  return EBUSY;
}



#if !defined EX_POWER_STATIC
STATIC int
ex_cardbus_enable(sc)
     struct ex_softc *sc;
{
  struct ex_cardbus_softc *csc = (struct ex_cardbus_softc *)sc;
  cardbus_function_tag_t cf = csc->sc_ct->ct_cf;
  cardbus_chipset_tag_t cc = csc->sc_ct->ct_cc;

  cardbus_function_enable(csc->sc_ct);
  cardbus_restore_bar(csc->sc_ct);

  sc->sc_ih = cardbus_intr_establish(cc, cf, csc->sc_intrline, IPL_NET, ex_intr, sc);
  if (NULL == sc->sc_ih) {
    printf("%s: couldn't establish interrupt\n", sc->sc_dev.dv_xname);
    return 1;
  }

/*  printf("ex_pccard_enable: %s turned on\n", sc->sc_dev.dv_xname); */
  return 0;
}




STATIC void
ex_cardbus_disable(sc)
     struct ex_softc *sc;
{
  struct ex_cardbus_softc *csc = (struct ex_cardbus_softc *)sc;
  cardbus_function_tag_t cf = csc->sc_ct->ct_cf;
  cardbus_chipset_tag_t cc = csc->sc_ct->ct_cc;

  cardbus_save_bar(csc->sc_ct);
  
  cardbus_function_disable(csc->sc_ct);

  cardbus_intr_disestablish(cc, cf, sc->sc_ih);
}

#endif /* EX_POWER_STATIC */
