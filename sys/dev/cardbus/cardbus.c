/*	$NetBSD: cardbus.c,v 1.4 1999/10/27 09:29:18 haya Exp $	*/

/*
 * Copyright (c) 1997, 1998 and 1999
 *     HAYAKAWA Koichi.  All rights reserved.
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
 *	This product includes software developed by HAYAKAWA Koichi.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_cardbus.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/syslog.h>

#include <machine/bus.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/cardbus/pccardcis.h>

#include <dev/pci/pcivar.h>	/* XXX */
#include <dev/pci/pcireg.h>	/* XXX */

#if defined CARDBUS_DEBUG
#define STATIC
#define DPRINTF(a) printf a
#define DDELAY(x) delay((x)*1000*1000)
#else
#define STATIC static
#define DPRINTF(a)
#endif



STATIC void cardbusattach __P((struct device *, struct device *, void *));
/* STATIC int cardbusprint __P((void *, const char *)); */
int cardbus_attach_card __P((struct cardbus_softc *));

#if !defined __BROKEN_INDIRECT_CONFIG
STATIC int cardbusmatch __P((struct device *, struct cfdata *, void *));
static int cardbussubmatch __P((struct device *, struct cfdata *, void *));
#else
STATIC int cardbusmatch __P((struct device *, void *, void *));
static int cardbussubmatch __P((struct device *, void *, void *));
#endif
static int cardbusprint __P((void *, const char *));

static int decode_tuples __P((u_int8_t *, int));


struct cfattach cardbus_ca = {
	sizeof(struct cardbus_softc), cardbusmatch, cardbusattach
};

#ifndef __NetBSD_Version__
struct cfdriver cardbus_cd = {
	NULL, "cardbus", DV_DULL
};
#endif


STATIC int
#if defined __BROKEN_INDIRECT_CONFIG
cardbusmatch(parent, match, aux)
     struct device *parent;
     void *match;
     void *aux;
#else
cardbusmatch(parent, cf, aux)
     struct device *parent;
     struct cfdata *cf;
     void *aux;
#endif
{
#if defined __BROKEN_INDIRECT_CONFIG
  struct cfdata *cf = match;
#endif
  struct cbslot_attach_args *cba = aux;

  if (strcmp(cba->cba_busname, cf->cf_driver->cd_name)) {
    DPRINTF(("cardbusmatch: busname differs %s <=> %s\n",
	     cba->cba_busname, cf->cf_driver->cd_name));
    return 0;
  }

  /* which function? */
  if (cf->cbslotcf_func != CBSLOT_UNK_FUNC &&
      cf->cbslotcf_func != cba->cba_function) {
    DPRINTF(("pccardmatch: function differs %d <=> %d\n",
	     cf->cbslotcf_func, cba->cba_function));
    return 0;
  }

  if (cba->cba_function < 0 || cba->cba_function > 255) {
    return 0;
  }

  return 1;
}



STATIC void
cardbusattach(parent, self, aux)
     struct device *parent;
     struct device *self;
     void *aux;
{
  struct cardbus_softc *sc = (void *)self;
  struct cbslot_attach_args *cba = aux;
  int cdstatus;

  sc->sc_bus = cba->cba_bus;
  sc->sc_device = cba->cba_function;
  sc->sc_intrline = cba->cba_intrline;
  sc->sc_cacheline = cba->cba_cacheline;
  sc->sc_lattimer = cba->cba_lattimer;

  printf(": bus %d device %d", sc->sc_bus, sc->sc_device);
  printf(" cacheline 0x%x, lattimer 0x%x\n", sc->sc_cacheline,sc->sc_lattimer);

  sc->sc_iot = cba->cba_iot;	/* CardBus I/O space tag */
  sc->sc_memt = cba->cba_memt;	/* CardBus MEM space tag */
  sc->sc_dmat = cba->cba_dmat;	/* DMA tag */
  sc->sc_cc = cba->cba_cc;
  sc->sc_cf = cba->cba_cf;

#if rbus
  sc->sc_rbus_iot = cba->cba_rbus_iot;
  sc->sc_rbus_memt = cba->cba_rbus_memt;
#endif

  sc->sc_funcs = NULL;

  cdstatus = 0;
}




/*
 * int cardbus_attach_card(struct cardbus_softc *sc)
 *
 *    This function attaches the card on the slot: turns on power,
 *    reads and analyses tuple, sets consifuration index.
 *
 *    This function returns the number of recognised device functions.
 *    If no functions are recognised, return 0.
 */
int
cardbus_attach_card(sc)
     struct cardbus_softc *sc;
{
  cardbus_chipset_tag_t cc;
  cardbus_function_tag_t cf;
  int cdstatus;
  cardbustag_t tag;
  cardbusreg_t id, class, cis_ptr;
  cardbusreg_t bhlc;
  u_int8_t tuple[2048];
  int function, nfunction;
  struct cardbus_devfunc **previous_next = &(sc->sc_funcs);
  struct device *csc;
  int no_work_funcs = 0;
  cardbus_devfunc_t ct;

  cc = sc->sc_cc;
  cf = sc->sc_cf;

  DPRINTF(("cardbus_attach_card: cb%d start\n", sc->sc_dev.dv_unit));

  /* inspect initial voltage */
  if (0 == (cdstatus = (cf->cardbus_ctrl)(cc, CARDBUS_CD))) {
    DPRINTF(("cardbusattach: no CardBus card on cb%d\n", sc->sc_dev.dv_unit));
    return 0;
  }

  if (cdstatus & CARDBUS_3V_CARD) {
    cf->cardbus_power(cc, CARDBUS_VCC_3V);
    sc->sc_poweron_func = 1;	/* function 0 on */
  }

  (cf->cardbus_ctrl)(cc, CARDBUS_RESET);

  function = 0;

  tag = cardbus_make_tag(cc, cf, sc->sc_bus, sc->sc_device, function);

  /*
   * Wait until power comes up.  Maxmum 500 ms.
   */
  {
    int i;
    for (i = 0; i < 5; ++i) {
      id = cardbus_conf_read(cc, cf, tag, CARDBUS_ID_REG);
      if (id != 0xffffffff && id != 0) {
	break;
      }
      delay(100*1000);		/* or tsleep */
    }
    if (i == 5) {
      return 0;
    }
  }
  
  bhlc = cardbus_conf_read(cc, cf, tag, CARDBUS_BHLC_REG);
  if (CARDBUS_LATTIMER(bhlc) < 0x10) {
    bhlc &= (CARDBUS_LATTIMER_MASK << CARDBUS_LATTIMER_SHIFT);
    bhlc |= (0x10 << CARDBUS_LATTIMER_SHIFT);
    cardbus_conf_write(cc, cf, tag, CARDBUS_BHLC_REG, bhlc);
  }

  nfunction = CARDBUS_HDRTYPE_MULTIFN(bhlc) ? 8 : 1;

  /*
   *           XXX multi-function card
   *
   * I don't know how to process CIS information for
   * multi-function cards.
   */

  id = cardbus_conf_read(cc, cf, tag, CARDBUS_ID_REG);
  class = cardbus_conf_read(cc, cf, tag, CARDBUS_CLASS_REG);
  cis_ptr = cardbus_conf_read(cc, cf, tag, CARDBUS_CIS_REG);

  DPRINTF(("cardbus_attach_card: Vendor 0x%x, Product 0x%x, CIS 0x%x\n",
	   CARDBUS_VENDOR(id), CARDBUS_PRODUCT(id), cis_ptr));

  bzero(tuple, 2048);

  if (CARDBUS_CIS_ASI_TUPLE == (CARDBUS_CIS_ASI(cis_ptr))) {
				/* Tuple is in Cardbus config space */
    int i = cis_ptr & CARDBUS_CIS_ADDRMASK;
    int j = 0;

    for (; i < 0xff; i += 4) {
      u_int32_t e = (cf->cardbus_conf_read)(cc, tag, i);
      tuple[j] = 0xff & e;
      e >>= 8;
      tuple[j + 1] = 0xff & e;
      e >>= 8;
      tuple[j + 2] = 0xff & e;
      e >>= 8;
      tuple[j + 3] = 0xff & e;
      j += 4;
    }
  } else if (CARDBUS_CIS_ASI(cis_ptr) <= CARDBUS_CIS_ASI_BAR5) {
    /*    int bar = CARDBUS_CIS_ASI_BAR(cis_ptr);*/
  }


  decode_tuples(tuple, 2048);

  {
    struct cardbus_attach_args ca;

    if (NULL == (ct = (cardbus_devfunc_t)malloc(sizeof(struct cardbus_devfunc),
						M_DEVBUF, M_NOWAIT))) {
      panic("no room for cardbus_tag");
    }

    ct->ct_cc = sc->sc_cc;
    ct->ct_cf = sc->sc_cf;
    ct->ct_bus = sc->sc_bus;
    ct->ct_dev = sc->sc_device;
    ct->ct_func = function;
    ct->ct_sc = sc;
    ct->ct_next = NULL;
    *previous_next = ct;

    ca.ca_unit = sc->sc_dev.dv_unit;
    ca.ca_ct = ct;

    ca.ca_iot = sc->sc_iot;
    ca.ca_memt = sc->sc_memt;
    ca.ca_dmat = sc->sc_dmat;

    ca.ca_tag = tag;
    ca.ca_device = sc->sc_device;
    ca.ca_function = function;
    ca.ca_id = id;
    ca.ca_class = class;

    ca.ca_intrline = sc->sc_intrline;

    if (NULL == (csc = config_found_sm((void *)sc, &ca, cardbusprint, cardbussubmatch))) {
      /* do not match */
      cf->cardbus_power(cc, CARDBUS_VCC_0V);
      sc->sc_poweron_func = 0;	/* no functions on */
      free(cc, M_DEVBUF);
    } else {
      /* found */
      previous_next = &(ct->ct_next);
      ct->ct_device = csc;
      ++no_work_funcs;
    }
  }

  return no_work_funcs;
}


static int
#ifdef __BROKEN_INDIRECT_CONFIG
cardbussubmatch(parent, match, aux)
#else
cardbussubmatch(parent, cf, aux)
#endif
     struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
     void *match;
#else
     struct cfdata *cf;
#endif
     void *aux;
{
#ifdef __BROKEN_INDIRECT_CONFIG
  struct cfdata *cf = match;
#endif
  struct cardbus_attach_args *ca = aux;

  if (cf->cardbuscf_dev != CARDBUS_UNK_DEV &&
      cf->cardbuscf_dev != ca->ca_unit) {
    return 0;
  }
  if (cf->cardbuscf_function != CARDBUS_UNK_FUNCTION &&
      cf->cardbuscf_function != ca->ca_function) {
    return 0;
  }

  return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}



static int
cardbusprint(aux, pnp)
     void *aux;
     const char *pnp;
{
  register struct cardbus_attach_args *ca = aux;
  char devinfo[256];

  if (pnp) {
    pci_devinfo(ca->ca_id, ca->ca_class, 1, devinfo);
    printf("%s at %s", devinfo, pnp);
  }
  printf(" dev %d function %d", ca->ca_device, ca->ca_function);

  return UNCONF;
}






/*
 * void *cardbus_intr_establish(cc, cf, irq, level, func, arg)
 *   Interrupt handler of pccard.
 *  args:
 *   cardbus_chipset_tag_t *cc
 *   int irq: 
 */
void *
cardbus_intr_establish(cc, cf, irq, level, func, arg)
     cardbus_chipset_tag_t cc;
     cardbus_function_tag_t cf;
     cardbus_intr_handle_t irq;
     int level;
     int (*func) __P((void *));
     void *arg;
{
  DPRINTF(("- cardbus_intr_establish: irq %d\n", irq));

  return (*cf->cardbus_intr_establish)(cc, irq, level, func, arg);
}



/*
 * void cardbus_intr_disestablish(cc, cf, handler)
 *   Interrupt handler of pccard.
 *  args:
 *   cardbus_chipset_tag_t *cc
 */
void
cardbus_intr_disestablish(cc, cf, handler)
     cardbus_chipset_tag_t cc;
     cardbus_function_tag_t cf;
     void *handler;
{
  DPRINTF(("- pccard_intr_disestablish\n"));

 (*cf->cardbus_intr_disestablish)(cc, handler);
  return;
}



/*
 * int cardbus_function_enable(cardbus_devfunc_t ct)
 *
 *   This function enables a function on a card.  When no power is
 *  applied on the card, power will be applied on it.
 */
int
cardbus_function_enable(ct)
     cardbus_devfunc_t ct;
{
  struct cardbus_softc *sc = ct->ct_sc;
  int func = ct->ct_func;
  cardbus_chipset_tag_t cc = sc->sc_cc;
  cardbus_function_tag_t cf = sc->sc_cf;
  cardbusreg_t command;
  cardbustag_t tag;

  DPRINTF(("entering cardbus_function_enable...  "));

  /* entering critical area */

  if (sc->sc_poweron_func == 0) {
    /* no functions are enabled */
    (*cf->cardbus_power)(cc, CARDBUS_VCC_3V); /* XXX: sc_vold should be used */
    (*cf->cardbus_ctrl)(cc, CARDBUS_RESET);
  }

  sc->sc_poweron_func |= (1 << func);

  /* exiting critical area */

  tag = Cardbus_make_tag(ct);
  command = cardbus_conf_read(cc, cf, tag, CARDBUS_COMMAND_STATUS_REG);
  command |= (CARDBUS_COMMAND_MEM_ENABLE | CARDBUS_COMMAND_IO_ENABLE | CARDBUS_COMMAND_MASTER_ENABLE); /* XXX: good guess needed */
  cardbus_conf_write(cc, cf, tag, CARDBUS_COMMAND_STATUS_REG, command);
  Cardbus_free_tag(ct, tag);


  DPRINTF(("%x\n", sc->sc_poweron_func));

  return 0;
}


/*
 * int cardbus_function_disable(cardbus_devfunc_t ct)
 *
 *   This function disable a function on a card.  When no functions are
 *  enabled, it turns off the power.
 */
int
cardbus_function_disable(ct)
     cardbus_devfunc_t ct;
{
  struct cardbus_softc *sc = ct->ct_sc;
  int func = ct->ct_func;
  cardbus_chipset_tag_t cc = sc->sc_cc;
  cardbus_function_tag_t cf = sc->sc_cf;

  DPRINTF(("entering cardbus_enable_disable...  "));

  sc->sc_poweron_func &= ~(1 << func);

  DPRINTF(("%x\n", sc->sc_poweron_func));

  if (sc->sc_poweron_func == 0) {
    /* power-off because no functions are enabled */
    (*cf->cardbus_power)(cc, CARDBUS_VCC_0V);
  }

  return 0;
}







/*
 * below this line, there are some functions for decoding tuples.
 * They should go out from this file.
 */

static u_int8_t *decode_tuple __P((u_int8_t *));
#ifdef CARDBUS_DEBUG
static char *tuple_name __P((int));
#endif

static int
decode_tuples(tuple, buflen)
     u_int8_t *tuple;
     int buflen;
{
  u_int8_t *tp = tuple;

  if (CISTPL_LINKTARGET != *tuple) {
    DPRINTF(("WRONG TUPLE: 0x%x\n", *tuple));
    return 0;
  }

  while (NULL != (tp = decode_tuple(tp))) {
    if (tuple + buflen < tp) {
      break;
    }
  }
  
  return 1;
}


static u_int8_t *
decode_tuple(tuple)
     u_int8_t *tuple;
{
  u_int8_t type;
  u_int8_t len;
#ifdef CARDBUS_DEBUG
  int i;
#endif

  type = tuple[0];
  len = tuple[1] + 2;

#ifdef CARDBUS_DEBUG
  printf("tuple: %s len %d\n", tuple_name(type), len);
#endif
  if (CISTPL_END == type) {
    return NULL;
  }

#ifdef CARDBUS_DEBUG
  for (i = 0; i < len; ++i) {
    if (i % 16 == 0) {
      printf("  0x%2x:", i);
    }
    printf(" %x",tuple[i]);
    if (i % 16 == 15) {
      printf("\n");
    }
  }
  if (i % 16 != 0) {
    printf("\n");
  }
#endif

  return tuple + len;
}


#ifdef CARDBUS_DEBUG
static char *
tuple_name(type)
     int type;
{
  static char *tuple_name_s [] = {
    "TPL_NULL", "TPL_DEVICE", "Reserved", "Reserved", /* 0-3 */
    "CONFIG_CB", "CFTABLE_ENTRY_CB", "Reserved", "BAR", /* 4-7 */
    "Reserved", "Reserved", "Reserved", "Reserved", /* 8-B */
    "Reserved", "Reserved", "Reserved", "Reserved", /* C-F */
    "CHECKSUM", "LONGLINK_A", "LONGLINK_C", "LINKTARGET",	/* 10-13 */
    "NO_LINK", "VERS_1", "ALTSTR", "DEVICE_A",
    "JEDEC_C", "JEDEC_A", "CONFIG", "CFTABLE_ENTRY",
    "DEVICE_OC", "DEVICE_OA", "DEVICE_GEO", "DEVICE_GEO_A",
    "MANFID", "FUNCID", "FUNCE", "SWIL", /* 20-23 */
    "Reserved", "Reserved", "Reserved", "Reserved", /* 24-27 */
    "Reserved", "Reserved", "Reserved", "Reserved", /* 28-2B */
    "Reserved", "Reserved", "Reserved", "Reserved", /* 2C-2F */
    "Reserved", "Reserved", "Reserved", "Reserved", /* 30-33 */
    "Reserved", "Reserved", "Reserved", "Reserved", /* 34-37 */
    "Reserved", "Reserved", "Reserved", "Reserved", /* 38-3B */
    "Reserved", "Reserved", "Reserved", "Reserved", /* 3C-3F */
    "VERS_2", "FORMAT", "GEOMETRY", "BYTEORDER",
    "DATE", "BATTERY", "ORG"
  };
#define NAME_LEN(x) (sizeof x / sizeof(x[0]))

  if (type > 0 && type < NAME_LEN(tuple_name_s)) {
    return tuple_name_s[type];
  } else if (0xff == type) {
    return "END";
  } else {
    return "Reserved";
  }
}
#endif
