/*	$NetBSD: maple.c,v 1.1 2001/01/16 00:32:42 marcus Exp $	*/

/*
 * Copyright (c) 2001 Marcus Comstedt
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
 *	This product includes software developed by Bradley A. Grantham.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <sh3/shbvar.h>
#include <sh3/pmap.h>


#include <dreamcast/dev/maple/maple.h>
#include <dreamcast/dev/maple/mapleconf.h>
#include <dreamcast/dev/maple/maplevar.h>
#include <dreamcast/dev/maple/maplereg.h>


#define MAPLE_CALLOUT_TICKS 1

/*
 * Function declarations.
 */
static int	maplematch __P((struct device *, struct cfdata *, void *));
static void	mapleattach __P((struct device *, struct device *, void *));
static int	mapleprint __P((void *, const char *));
static void	maple_attach_dev __P((struct maple_softc *, int, int));
static void	maple_begin_txbuf __P((struct maple_softc *));
static int	maple_end_txbuf __P((struct maple_softc *));
static void	maple_write_command __P((struct maple_softc *, int, int,
					 int, int, void *));
static void	maple_scanbus __P((struct maple_softc *));
static void	maple_callout __P((void *));
static void	maple_send_commands __P((struct maple_softc *));
static void	maple_check_responses __P((struct maple_softc *));


/*
 * Global variables.
 */
int	maple_polling = 0;	/* Are we polling?  (Debugger mode) */

/*
 * Driver definition.
 */
struct cfattach maple_ca = {
	sizeof(struct maple_softc), maplematch, mapleattach
};

int	maplesearch __P((struct device *, struct cfdata *, void *));

static int
maplematch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct shb_attach_args *sa = aux;

	if (strcmp("maple", cf->cf_driver->cd_name))
	  return (0);

	sa->ia_iosize = 0 /* 0x100 */;
	return (1);
}


static void
maple_attach_dev(sc, port, subunit)
	struct maple_softc *sc;
	int port;
	int subunit;
{
	struct maple_attach_args ma;
	ma.ma_port = port;
	ma.ma_subunit = subunit;
	ma.ma_devinfo = &sc->sc_unit[port][subunit].devinfo;
	config_search(maplesearch, &sc->sc_dev, &ma);
}

static void
maple_begin_txbuf(sc)
	struct maple_softc *sc;
{
	sc->sc_txlink = sc->sc_txpos = sc->sc_txbuf;
}

static int
maple_end_txbuf(sc)
	struct maple_softc *sc;
{
	/* if no frame have been written, we can't mark the
	   list end, and so the DMA must not be activated   */
  	if (sc->sc_txpos == sc->sc_txbuf)
	  return (0);

	*sc->sc_txlink |= 0x80000000;

	return (1);
}

static int8_t subunit_code[] = { 0x20, 0x01, 0x02, 0x04, 0x08, 0x10 };

static void
maple_write_command(sc, port, subunit, command, datalen, dataaddr)
	struct maple_softc *sc;
	int port;
	int subunit;
	int command;
	int datalen;
	void *dataaddr;
{
	int to, from;
	u_int32_t *p = sc->sc_txpos;

	if ((port & ~(MAPLE_PORTS-1)) != 0 ||
	    subunit < 0 || subunit >= MAPLE_SUBUNITS)
	  return;

	/* Compute sender and recipient address */
	from = port << 6;
	to = from | subunit_code[subunit];

	/* Max data length = 255 longs = 1020 bytes */
	if(datalen > 255)
	  datalen = 255;
	else if(datalen < 0)
	  datalen = 0;

	sc->sc_txlink = p;

	/* Set length of packet and destination port (A-D) */
	*p++ = datalen | (port << 16);

	/* Write address to receive buffer where the response
	   frame should be put */
	*p++ = sc->sc_rxbuf_phys[port][subunit];

	/* Create the frame header.  The fields are assembled "backwards"
	   because of the Maple Bus big-endianness.                       */
	*p++ = (command & 0xff) | (to << 8) | (from << 16) | (datalen << 24);

	/* Copy parameter data, if any */
	if (datalen > 0) {
	  u_int32_t *param = dataaddr;
	  int i;
	  for (i = 0; i < datalen; i++)
	    *p++ = *param++;
	}

	sc->sc_txpos = p;
}

static void
maple_scanbus(sc)
	struct maple_softc *sc;
{
	int p;

	maple_polling = 1;

	maple_begin_txbuf(sc);

	for (p = 0; p < MAPLE_PORTS; p++) {
	  maple_write_command(sc, p, 0, MAPLE_COMMAND_DEVINFO, 0, NULL);
	}

	if (maple_end_txbuf(sc)) {

	  MAPLE_DMAADDR = sc->sc_txbuf_phys;
	  MAPLE_STATE = 1;
	  while (MAPLE_STATE != 0)
	    ;

	  for (p = 0; p < MAPLE_PORTS; p++)
	    if ((sc->sc_rxbuf[p][0][0] & 0xff) == MAPLE_RESPONSE_DEVINFO) {

	      u_int32_t *to_swap;
	      int i;

	      bcopy(sc->sc_rxbuf[p][0]+1,
		    (to_swap = &sc->sc_unit[p][0].devinfo.di_func),
		    sizeof(struct maple_devinfo));

	      for (i = 0; i < 4; i++, to_swap++)
		*to_swap = ntohl(*to_swap);

	      maple_attach_dev(sc, p, 0);
	    }

	}

	maple_polling = 0;

}

static void
maple_send_commands(sc)
	struct maple_softc *sc;
{
	int p;

	if (sc->maple_commands_pending || MAPLE_STATE != 0)
	  return;

	maple_begin_txbuf(sc);

	for (p = 0; p < MAPLE_PORTS; p++) {

	  if (sc->sc_unit[p][0].getcond_callback != NULL) {

	    u_int32_t func = ntohl(sc->sc_unit[p][0].getcond_func);

	    maple_write_command(sc, p, 0, MAPLE_COMMAND_GETCOND, 1, &func);

	  }

	}

	if (!maple_end_txbuf(sc))
	  return;

	MAPLE_DMAADDR = sc->sc_txbuf_phys;
	MAPLE_STATE = 1;

	sc->maple_commands_pending = 1;
}

static void
maple_check_responses(sc)
	struct maple_softc *sc;
{
	int p;

	if (!sc->maple_commands_pending || MAPLE_STATE != 0)
	  return;

	for (p = 0; p < MAPLE_PORTS; p++) {
	  struct maple_unit * u = &sc->sc_unit[p][0];
	  if (u->getcond_callback != NULL &&
	      (sc->sc_rxbuf[p][0][0] & 0xff) == MAPLE_RESPONSE_DATATRF &&
	      (sc->sc_rxbuf[p][0][0]>>24) >= 1 &&
	      htonl(sc->sc_rxbuf[p][0][1]) == u->getcond_func) {
	    (*u->getcond_callback)(u->getcond_data,
				   (void *)(sc->sc_rxbuf[p][0]+2),
				   ((sc->sc_rxbuf[p][0][0]>>22)&1020)-4);
	  }
	}

	sc->maple_commands_pending = 0;
}

void
maple_run_polling(dev)
	struct device *dev;
{
	struct maple_softc *sc;

	sc = (struct maple_softc *)dev;

	if (MAPLE_STATE != 0)
	  return;

	maple_send_commands(sc);

	while (MAPLE_STATE != 0)
	  ;

	maple_check_responses(sc);
}

void
maple_set_condition_callback(dev, port, subunit, func, callback, data)
	struct device *dev;
	int port;
	int subunit;
	u_int32_t func;
	void (*callback)(void *, void *, int);
	void *data;
{
	struct maple_softc *sc;

	sc = (struct maple_softc *)dev;

	if ((port & ~(MAPLE_PORTS-1)) != 0 ||
	    subunit < 0 || subunit >= MAPLE_SUBUNITS)
	  return;

	sc->sc_unit[port][subunit].getcond_func = func;
	sc->sc_unit[port][subunit].getcond_callback = callback;
	sc->sc_unit[port][subunit].getcond_data = data;
}

static void
maple_callout(ctx)
	void *ctx;
{
	struct maple_softc *sc = ctx;

	if(!maple_polling) {

	  maple_check_responses(sc);

	  maple_send_commands(sc);

	}

	callout_reset(&sc->maple_callout_ch, MAPLE_CALLOUT_TICKS,
		      (void *)maple_callout, sc);
}

static void
mapleattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct maple_softc *sc;
	vaddr_t dmabuffer;
	u_int32_t *p;
	int i, j;

	sc = (struct maple_softc *)self;

	printf("\n");

	dmabuffer = uvm_km_alloc(phys_map, MAPLE_DMABUF_SIZE);

	p = (u_int32_t *) vtophys(dmabuffer);

	for (i = 0; i < MAPLE_PORTS; i++)
	  for (j = 0; j < MAPLE_SUBUNITS; j++) {

	    sc->sc_rxbuf[i][j] = p;
	    sc->sc_rxbuf_phys[i][j] = SH3_P2SEG_TO_PHYS(p);
	    p += 256;

	  }

	sc->sc_txbuf = p;
	sc->sc_txbuf_phys = SH3_P2SEG_TO_PHYS(p);

	sc->maple_commands_pending = 0;

	MAPLE_RESET = RESET_MAGIC;
	MAPLE_RESET2 = 0;

	MAPLE_SPEED = SPEED_2MBPS | TIMEOUT(50000);

	MAPLE_ENABLE = 1;

	maple_scanbus(sc);

	bzero(&sc->maple_callout_ch, sizeof(sc->maple_callout_ch));

	callout_reset(&sc->maple_callout_ch, MAPLE_CALLOUT_TICKS,
		      (void *)maple_callout, sc);
}

int
mapleprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct maple_attach_args *ma = aux;

	if (pnp != NULL)
		printf("%s", pnp);

	printf(" port %c", ma->ma_port+'A');

	if (ma->ma_subunit != 0)
	  printf("%d", ma->ma_subunit);

	return (UNCONF);
}

int
maplesearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	if ((*cf->cf_attach->ca_match)(parent, cf, aux) > 0)
	  config_attach(parent, cf, aux, mapleprint);

	return (0);
}

u_int32_t
maple_get_function_data(devinfo, function_code)
	struct maple_devinfo *devinfo;
	u_int32_t function_code;
{
	int i, p = 0;

	for (i = 31; i >= 0; --i)
	  if (devinfo->di_func & (1U<<i))
	    if (function_code & (1U<<i))
	      return devinfo->di_function_data[p];
	    else
	      if (++p >= 3)
		break;
	return (0);
}

