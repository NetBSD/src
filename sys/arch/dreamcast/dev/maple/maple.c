/*	$NetBSD: maple.c,v 1.8.2.3 2002/06/23 17:35:34 jdolecek Exp $	*/

/*-
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
 *	This product includes software developed by Marcus Comstedt.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
#include <sh3/pmap.h>

#include <dreamcast/dev/maple/maple.h>
#include <dreamcast/dev/maple/mapleconf.h>
#include <dreamcast/dev/maple/maplevar.h>
#include <dreamcast/dev/maple/maplereg.h>
#include <dreamcast/dev/maple/mapleio.h>

#include "locators.h"

/* Internal macros, functions, and variables. */

#define MAPLE_CALLOUT_TICKS 2

#define MAPLEBUSUNIT(dev)  (minor(dev)>>5)
#define MAPLEPORT(dev)     ((minor(dev) & 0x18) >> 3)
#define MAPLESUBUNIT(dev)  (minor(dev) & 0x7)

/*
 * Function declarations.
 */
static int	maplematch(struct device *, struct cfdata *, void *);
static void	mapleattach(struct device *, struct device *, void *);
static int	mapleprint(void *, const char *);
static int	maplesubmatch(struct device *, struct cfdata *, void *);
static void	maple_attach_dev(struct maple_softc *, int, int);
static void	maple_begin_txbuf(struct maple_softc *);
static int	maple_end_txbuf(struct maple_softc *);
static void	maple_write_command(struct maple_softc *, int, int,
		    int, int, void *);
static void	maple_scanbus(struct maple_softc *);
static void	maple_callout(void *);
static void	maple_send_commands(struct maple_softc *);
static void	maple_check_responses(struct maple_softc *);

int	maple_alloc_dma(size_t, vaddr_t *, paddr_t *);
void	maple_free_dma(paddr_t, size_t);

int	mapleopen(dev_t, int, int, struct proc *);
int	mapleclose(dev_t, int, int, struct proc *);
int	maple_internal_ioctl(struct maple_softc *,  int, int, u_long, caddr_t,
	    int, struct proc *);
int	mapleioctl(dev_t, u_long, caddr_t, int, struct proc *);

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

extern struct cfdriver maple_cd;

static int
maplematch(struct device *parent, struct cfdata *cf, void *aux)
{

	if (strcmp("maple", cf->cf_driver->cd_name))
		return (0);

	return (1);
}

static void
maple_attach_dev(struct maple_softc *sc, int port, int subunit)
{
	struct maple_attach_args ma;
	u_int32_t func;
	int f;
	char oldxname[16];
	
	ma.ma_port = port;
	ma.ma_subunit = subunit;
	ma.ma_devinfo = &sc->sc_unit[port][subunit].devinfo;
	ma.ma_function = 1;
	func = ma.ma_devinfo->di_func;

	mapleprint(&ma, sc->sc_dev.dv_xname);
	printf("\n");
	strcpy(oldxname, sc->sc_dev.dv_xname);
	sprintf(sc->sc_dev.dv_xname, "maple%c", port+'A');
	if (subunit)
		sprintf(sc->sc_dev.dv_xname+6, "%d", subunit);

	for (f = 0; f < 32; f++, ma.ma_function <<= 1)
		if (func & ma.ma_function)
			(void)config_found_sm(&sc->sc_dev, &ma,
			    NULL, maplesubmatch);
	strcpy(sc->sc_dev.dv_xname, oldxname);
}

static void
maple_begin_txbuf(struct maple_softc *sc)
{

	sc->sc_txlink = sc->sc_txpos = sc->sc_txbuf;
}

static int
maple_end_txbuf(struct maple_softc *sc)
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
maple_write_command(struct maple_softc *sc, int port, int subunit, int command,
    int datalen, void *dataaddr)
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
	if (datalen > 255)
		datalen = 255;
	else if (datalen < 0)
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
maple_scanbus(struct maple_softc *sc)
{
	int p, s;

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
			if ((sc->sc_rxbuf[p][0][0] & 0xff) ==
			    MAPLE_RESPONSE_DEVINFO)
				sc->sc_port_units[p] =
				    ((sc->sc_rxbuf[p][0][0] >> 15) & 0x3e) | 1;
			else
				sc->sc_port_units[p] = 0;


		maple_begin_txbuf(sc);

		for (p = 0; p < MAPLE_PORTS; p++) {
			for (s = 0; s < MAPLE_SUBUNITS; s++) {
				if (sc->sc_port_units[p] & (1 << s))
					maple_write_command(sc, p, s,
					    MAPLE_COMMAND_DEVINFO, 0, NULL);
			}
		}

		if (maple_end_txbuf(sc)) {
			MAPLE_DMAADDR = sc->sc_txbuf_phys;
			MAPLE_STATE = 1;
			while (MAPLE_STATE != 0)
				;

			for (p = 0; p < MAPLE_PORTS; p++)
				for (s = 0; s < MAPLE_SUBUNITS; s++)
					if (sc->sc_port_units[p] & (1 << s)) {

						if ((sc->sc_rxbuf[p][s][0] &
						    0xff) ==
						    MAPLE_RESPONSE_DEVINFO) {

							u_int32_t *to_swap;
							int i;
		    
							memcpy((to_swap =
							    &sc->sc_unit[p][s].
							    devinfo.di_func),
							    sc->sc_rxbuf[p][s] +
							    1,
							    sizeof(struct
								maple_devinfo));
		    
							for (i = 0; i < 4; i++,
							    to_swap++)
								*to_swap =
								    ntohl
								    (*to_swap);
							maple_attach_dev(sc, p,
							    s);
						} else {

							printf("%s: no response"
							    " from port %d "
							    " subunit %d\n",
							    sc->sc_dev.dv_xname,
							    p, s);
							sc->sc_port_units[p] &=
							    ~(1 << s);
						}
					}
	    
		}

	}

	maple_polling = 0;

}

static void
maple_send_commands(struct maple_softc *sc)
{
	int p, s;

	if (sc->maple_commands_pending || MAPLE_STATE != 0)
		return;

	maple_begin_txbuf(sc);

	for (p = 0; p < MAPLE_PORTS; p++) {
		for (s = 0; s < MAPLE_SUBUNITS; s++) {
			if (sc->sc_unit[p][s].getcond_callback != NULL) {
				u_int32_t func =
				    ntohl(sc->sc_unit[p][s].getcond_func);
	      
				maple_write_command(sc, p, s,
				    MAPLE_COMMAND_GETCOND, 1, &func);
			}
		}
	}

	if (!maple_end_txbuf(sc))
		return;

	MAPLE_DMAADDR = sc->sc_txbuf_phys;
	MAPLE_STATE = 1;

	sc->maple_commands_pending = 1;
}

static void
maple_check_responses(struct maple_softc *sc)
{
	int p, s;

	if (!sc->maple_commands_pending || MAPLE_STATE != 0)
		return;

	for (p = 0; p < MAPLE_PORTS; p++) {
		for (s = 0; s < MAPLE_SUBUNITS; s++) {
			struct maple_unit *u = &sc->sc_unit[p][s];
			if (u->getcond_callback != NULL &&
			    (sc->sc_rxbuf[p][s][0] & 0xff) ==
			    MAPLE_RESPONSE_DATATRF &&
			    (sc->sc_rxbuf[p][s][0] >> 24) >= 1 &&
			    htonl(sc->sc_rxbuf[p][s][1]) == u->getcond_func) {
				(*u->getcond_callback)(u->getcond_data,
				    (void *)(sc->sc_rxbuf[p][s] + 2),
				    ((sc->sc_rxbuf[p][s][0] >> 22) & 1020) - 4);
			}
		}
	}

	sc->maple_commands_pending = 0;
}

void
maple_run_polling(struct device *dev)
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
maple_set_condition_callback(struct device *dev, int port, int subunit,
    u_int32_t func, void (*callback)(void *, void *, int), void *data)
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
maple_callout(void *ctx)
{
	struct maple_softc *sc = ctx;

	if (!maple_polling) {
		maple_check_responses(sc);
		maple_send_commands(sc);
	}

	callout_reset(&sc->maple_callout_ch, MAPLE_CALLOUT_TICKS,
	    (void *)maple_callout, sc);
}

int
maple_alloc_dma(size_t size, vaddr_t *vap, paddr_t *pap)
{
	extern paddr_t avail_start, avail_end;	/* from pmap.c */
	struct pglist mlist;
	struct vm_page *m;
	int error;

	size = round_page(size);

	error = uvm_pglistalloc(size, avail_start, avail_end - PAGE_SIZE,
	    0, 0, &mlist, 1, 0);
	if (error)
		return (error);

	m = TAILQ_FIRST(&mlist);
	*pap = VM_PAGE_TO_PHYS(m);
	*vap = SH3_PHYS_TO_P2SEG(VM_PAGE_TO_PHYS(m));

	return (0);
}

void
maple_free_dma(paddr_t paddr, size_t size)
{
	struct pglist mlist;
	struct vm_page *m;
	bus_addr_t addr;

	TAILQ_INIT(&mlist);
	for (addr = paddr; addr < paddr + size; addr += PAGE_SIZE) {
		m = PHYS_TO_VM_PAGE(addr);
		TAILQ_INSERT_TAIL(&mlist, m, pageq);
	}
	uvm_pglistfree(&mlist);
}

static void
mapleattach(struct device *parent, struct device *self, void *aux)
{
	struct maple_softc *sc;
	vaddr_t dmabuffer;
	paddr_t dmabuffer_phys;
	u_int32_t *p;
	int i, j;

	sc = (struct maple_softc *)self;

	printf("\n");

	if (maple_alloc_dma(MAPLE_DMABUF_SIZE, &dmabuffer, &dmabuffer_phys)) {
		printf("%s: unable to allocate DMA buffers.\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	p = (u_int32_t *)dmabuffer;

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

	memset(&sc->maple_callout_ch, 0, sizeof(sc->maple_callout_ch));

	callout_reset(&sc->maple_callout_ch, MAPLE_CALLOUT_TICKS,
	    (void *)maple_callout, sc);
}

int
mapleprint(void *aux, const char *pnp)
{
	struct maple_attach_args *ma = aux;

	if (pnp != NULL) {
		printf("maple%c", 'A'+ma->ma_port);
		if (ma->ma_subunit != 0)
			printf("%d", ma->ma_subunit);
		printf(" at %s", pnp);
	}

	printf(" port %d", ma->ma_port);

	if (ma->ma_subunit != 0)
		printf(" subunit %d", ma->ma_subunit);

	printf(": %.*s",
	    (int)sizeof(ma->ma_devinfo->di_product_name),
	    ma->ma_devinfo->di_product_name);

	return (0);
}

static int
maplesubmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct maple_attach_args *ma = aux;

	if (match->cf_loc[MAPLECF_PORT] != MAPLECF_PORT_DEFAULT &&
	    match->cf_loc[MAPLECF_PORT] != ma->ma_port)
		return (0);

	if (match->cf_loc[MAPLECF_SUBUNIT] != MAPLECF_SUBUNIT_DEFAULT &&
	    match->cf_loc[MAPLECF_SUBUNIT] != ma->ma_subunit)
		return (0);

	return ((*match->cf_attach->ca_match)(parent, match, aux));
}

u_int32_t
maple_get_function_data(struct maple_devinfo *devinfo, u_int32_t function_code)
{
	int i, p = 0;

	for (i = 31; i >= 0; --i)
		if (devinfo->di_func & (1U << i)) {
			if (function_code & (1U << i))
				return devinfo->di_function_data[p];
			else
				if (++p >= 3)
					break;
		}

	return (0);
}

/* Generic maple device interface */

int
mapleopen(dev_t dev, int flag, int mode, struct proc *p)
{
	struct maple_softc *sc;

	sc = device_lookup(&maple_cd, MAPLEBUSUNIT(dev));
	if (sc == NULL)			/* make sure it was attached */
		return (ENXIO);

	if (MAPLEPORT(dev) >= MAPLE_PORTS)
		return (ENXIO);
	
	if (MAPLESUBUNIT(dev) >= MAPLE_SUBUNITS)
		return (ENXIO);

	if (!(sc->sc_port_units[MAPLEPORT(dev)] & (1 << MAPLESUBUNIT(dev))))
		return (ENXIO);

	sc->sc_port_units_open[MAPLEPORT(dev)] |= 1 << MAPLESUBUNIT(dev);

	return (0);
}

int
mapleclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct maple_softc *sc;

	sc = device_lookup(&maple_cd, MAPLEBUSUNIT(dev));

	sc->sc_port_units_open[MAPLEPORT(dev)] &= ~(1 << MAPLESUBUNIT(dev));

	return (0);
}

int
maple_internal_ioctl(struct maple_softc *sc, int port, int subunit, u_long cmd,
    caddr_t data, int flag, struct proc *p)
{
	struct maple_unit *u = &sc->sc_unit[port][subunit];

	if (!(sc->sc_port_units[port] & (1 << subunit)))
		return (ENXIO);

	switch(cmd) {
	case MAPLEIO_GDEVINFO:
		memcpy(data, &u->devinfo, sizeof(struct maple_devinfo));
		return (0);
	default:
		return (EINVAL);
	}
}

int
mapleioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct maple_softc *sc;

	sc = device_lookup(&maple_cd, MAPLEBUSUNIT(dev));

	return (maple_internal_ioctl(sc, MAPLEPORT(dev), MAPLESUBUNIT(dev),
	    cmd, data, flag, p));
}
