/*	$NetBSD: maple.c,v 1.32.2.2 2007/07/15 13:15:46 ad Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by ITOH Yasufumi.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: maple.c,v 1.32.2.2 2007/07/15 13:15:46 ad Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/sysasicvar.h>
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

/* interrupt priority level */
#define	IPL_MAPLE	IPL_BIO
#define splmaple()	splbio()

/*
 * Function declarations.
 */
static int	maplematch(struct device *, struct cfdata *, void *);
static void	mapleattach(struct device *, struct device *, void *);
static void	maple_scanbus(struct maple_softc *);
static char *	maple_unit_name(char *, int port, int subunit);
static void	maple_begin_txbuf(struct maple_softc *);
static int	maple_end_txbuf(struct maple_softc *);
static void	maple_queue_command(struct maple_softc *, struct maple_unit *,
		    int command, int datalen, const void *dataaddr);
static void	maple_write_command(struct maple_softc *, struct maple_unit *,
		    int, int, const void *);
static void	maple_start(struct maple_softc *sc);
static void	maple_start_poll(struct maple_softc *);
static void	maple_check_subunit_change(struct maple_softc *,
		    struct maple_unit *);
static void	maple_check_unit_change(struct maple_softc *,
		    struct maple_unit *);
static void	maple_print_unit(void *, const char *);
static int	maplesubmatch(struct device *, struct cfdata *,
		    const int *, void *);
static int	mapleprint(void *, const char *);
static void	maple_attach_unit(struct maple_softc *, struct maple_unit *);
static void	maple_detach_unit_nofix(struct maple_softc *,
		    struct maple_unit *);
static void	maple_detach_unit(struct maple_softc *, struct maple_unit *);
static void	maple_queue_cmds(struct maple_softc *,
		    struct maple_cmdq_head *);
static void	maple_unit_probe(struct maple_softc *);
static void	maple_unit_ping(struct maple_softc *);
static int	maple_send_defered_periodic(struct maple_softc *);
static void	maple_send_periodic(struct maple_softc *);
static void	maple_remove_from_queues(struct maple_softc *,
		    struct maple_unit *);
static int	maple_retry(struct maple_softc *, struct maple_unit *,
		    enum maple_dma_stat);
static void	maple_queue_retry(struct maple_softc *);
static void	maple_check_responses(struct maple_softc *);
static void	maple_event_thread(void *);
static int	maple_intr(void *);
static void	maple_callout(void *);

int	maple_alloc_dma(size_t, vaddr_t *, paddr_t *);
#if 0
void	maple_free_dma(paddr_t, size_t);
#endif

/*
 * Global variables.
 */
int	maple_polling;		/* Are we polling?  (Debugger mode) */

CFATTACH_DECL(maple, sizeof(struct maple_softc),
    maplematch, mapleattach, NULL, NULL);

extern struct cfdriver maple_cd;

dev_type_open(mapleopen);
dev_type_close(mapleclose);
dev_type_ioctl(mapleioctl);

const struct cdevsw maple_cdevsw = {
	mapleopen, mapleclose, noread, nowrite, mapleioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

static int
maplematch(struct device *parent, struct cfdata *cf, void *aux)
{

	return 1;
}

static void
mapleattach(struct device *parent, struct device *self, void *aux)
{
	struct maple_softc *sc;
	struct maple_unit *u;
	vaddr_t dmabuffer;
	paddr_t dmabuffer_phys;
	uint32_t *p;
	int port, subunit, f;

	sc = (struct maple_softc *)self;

	printf(": %s\n", sysasic_intr_string(IPL_MAPLE));

	if (maple_alloc_dma(MAPLE_DMABUF_SIZE, &dmabuffer, &dmabuffer_phys)) {
		printf("%s: unable to allocate DMA buffers.\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	p = (uint32_t *)dmabuffer;

	for (port = 0; port < MAPLE_PORTS; port++) {
		for (subunit = 0; subunit < MAPLE_SUBUNITS; subunit++) {
			u = &sc->sc_unit[port][subunit];
			u->port = port;
			u->subunit = subunit;
			u->u_dma_stat = MAPLE_DMA_IDLE;
			u->u_rxbuf = p;
			u->u_rxbuf_phys = SH3_P2SEG_TO_PHYS(p);
			p += 256;

			for (f = 0; f < MAPLE_NFUNC; f++) {
				u->u_func[f].f_funcno = f;
				u->u_func[f].f_unit = u;
			}
		}
	}

	sc->sc_txbuf = p;
	sc->sc_txbuf_phys = SH3_P2SEG_TO_PHYS(p);

	SIMPLEQ_INIT(&sc->sc_retryq);
	TAILQ_INIT(&sc->sc_probeq);
	TAILQ_INIT(&sc->sc_pingq);
	TAILQ_INIT(&sc->sc_periodicq);
	TAILQ_INIT(&sc->sc_periodicdeferq);
	TAILQ_INIT(&sc->sc_acmdq);
	TAILQ_INIT(&sc->sc_pcmdq);

	MAPLE_RESET = RESET_MAGIC;
	MAPLE_RESET2 = 0;

	MAPLE_SPEED = SPEED_2MBPS | TIMEOUT(50000);

	MAPLE_ENABLE = 1;

	maple_polling = 1;
	maple_scanbus(sc);

	callout_init(&sc->maple_callout_ch, 0);

	sc->sc_intrhand = sysasic_intr_establish(SYSASIC_EVENT_MAPLE_DMADONE,
	    IPL_MAPLE, SYSASIC_IRL9, maple_intr, sc);

	config_pending_incr();	/* create thread before mounting root */

	if (kthread_create(PRI_NONE, 0, NULL, maple_event_thread, sc,
	    &sc->event_thread, "%s", sc->sc_dev.dv_xname) == 0)
		return;

	panic("%s: unable to create event thread", sc->sc_dev.dv_xname);
}

/*
 * initial device attach
 */
static void
maple_scanbus(struct maple_softc *sc)
{
	struct maple_unit *u;
	int port;
	int last_port, last_subunit;
	int i;

	KASSERT(cold && maple_polling);

	/* probe all ports */
	for (port = 0; port < MAPLE_PORTS; port++) {
		u = &sc->sc_unit[port][0];
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 2
		{
			char buf[16];
			printf("%s: queued to probe 1\n",
			    maple_unit_name(buf, u->port, u->subunit));
		}
#endif
		TAILQ_INSERT_TAIL(&sc->sc_probeq, u, u_q);
		u->u_queuestat = MAPLE_QUEUE_PROBE;
	}

	last_port = last_subunit = -1;
	maple_begin_txbuf(sc);
	while ((u = TAILQ_FIRST(&sc->sc_probeq)) != NULL) {
		/*
		 * Check wrap condition
		 */
		if (u->port < last_port || u->subunit <= last_subunit)
			break;
		last_port = u->port;
		if (u->port == MAPLE_PORTS - 1)
			last_subunit = u->subunit;

		maple_unit_probe(sc);
		for (i = 10 /* just not forever */; maple_end_txbuf(sc); i--) {
			maple_start_poll(sc);
			maple_check_responses(sc);
			if (i == 0)
				break;
			/* attach may issue cmds */
			maple_queue_cmds(sc, &sc->sc_acmdq);
		}
	}
}

void
maple_run_polling(struct device *dev)
{
	struct maple_softc *sc;
	int port, subunit;
	int i;

	sc = (struct maple_softc *)dev;

	/*
	 * first, make sure polling works
	 */
	while (MAPLE_STATE != 0)	/* XXX may lost a DMA cycle */
		;

	/* XXX this will break internal state */
	for (port = 0; port < MAPLE_PORTS; port++)
		for (subunit = 0; subunit < MAPLE_SUBUNITS; subunit++)
			sc->sc_unit[port][subunit].u_dma_stat = MAPLE_DMA_IDLE;
	SIMPLEQ_INIT(&sc->sc_retryq);	/* XXX discard current retrys */

	/*
	 * do polling (periodic status check only)
	 */
	maple_begin_txbuf(sc);
	maple_send_defered_periodic(sc);
	maple_send_periodic(sc);
	for (i = 10 /* just not forever */; maple_end_txbuf(sc); i--) {
		maple_start_poll(sc);
		maple_check_responses(sc);
		if (i == 0)
			break;

		/* maple_check_responses() has executed maple_begin_txbuf() */
		maple_queue_retry(sc);
		maple_send_defered_periodic(sc);
	}
}

static char *
maple_unit_name(char *buf, int port, int subunit)
{

	sprintf(buf, "maple%c", port + 'A');
	if (subunit)
		sprintf(buf+6, "%d", subunit);

	return buf;
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
		return error;

	m = TAILQ_FIRST(&mlist);
	*pap = VM_PAGE_TO_PHYS(m);
	*vap = SH3_PHYS_TO_P2SEG(VM_PAGE_TO_PHYS(m));

	return 0;
}

#if 0	/* currently unused */
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
#endif

static void
maple_begin_txbuf(struct maple_softc *sc)
{

	sc->sc_txlink = sc->sc_txpos = sc->sc_txbuf;
	SIMPLEQ_INIT(&sc->sc_dmaq);
}

static int
maple_end_txbuf(struct maple_softc *sc)
{

	/* if no frame have been written, we can't mark the
	   list end, and so the DMA must not be activated   */
	if (sc->sc_txpos == sc->sc_txbuf)
		return 0;

	*sc->sc_txlink |= 0x80000000;

	return 1;
}

static const int8_t subunit_code[] = { 0x20, 0x01, 0x02, 0x04, 0x08, 0x10 };

static void
maple_queue_command(struct maple_softc *sc, struct maple_unit *u,
	int command, int datalen, const void *dataaddr)
{
	int to, from;
	uint32_t *p = sc->sc_txpos;

	/* Max data length = 255 longs = 1020 bytes */
	KASSERT(datalen >= 0 && datalen <= 255);

	/* Compute sender and recipient address */
	from = u->port << 6;
	to = from | subunit_code[u->subunit];

	sc->sc_txlink = p;

	/* Set length of packet and destination port (A-D) */
	*p++ = datalen | (u->port << 16);

	/* Write address to receive buffer where the response
	   frame should be put */
	*p++ = u->u_rxbuf_phys;

	/* Create the frame header.  The fields are assembled "backwards"
	   because of the Maple Bus big-endianness.                       */
	*p++ = (command & 0xff) | (to << 8) | (from << 16) | (datalen << 24);

	/* Copy parameter data, if any */
	if (datalen > 0) {
		const uint32_t *param = dataaddr;
		int i;
		for (i = 0; i < datalen; i++)
			*p++ = *param++;
	}

	sc->sc_txpos = p;

	SIMPLEQ_INSERT_TAIL(&sc->sc_dmaq, u, u_dmaq);
}

static void
maple_write_command(struct maple_softc *sc, struct maple_unit *u, int command,
	int datalen, const void *dataaddr)
{
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 2
	char buf[16];

	if (u->u_retrycnt)
		printf("%s: retrycnt %d\n",
		    maple_unit_name(buf, u->port, u->subunit), u->u_retrycnt);
#endif
	u->u_retrycnt = 0;
	u->u_command = command;
	u->u_datalen = datalen;
	u->u_dataaddr = dataaddr;

	maple_queue_command(sc, u, command, datalen, dataaddr);
}

/* start DMA */
static void
maple_start(struct maple_softc *sc)
{

	MAPLE_DMAADDR = sc->sc_txbuf_phys;
	MAPLE_STATE = 1;
}

/* start DMA -- wait until DMA done */
static void
maple_start_poll(struct maple_softc *sc)
{

	MAPLE_DMAADDR = sc->sc_txbuf_phys;
	MAPLE_STATE = 1;
	while (MAPLE_STATE != 0)
		;
}

static void
maple_check_subunit_change(struct maple_softc *sc, struct maple_unit *u)
{
	struct maple_unit *u1;
	int port;
	int8_t unit_map;
	int units, un;
	int i;

	KASSERT(u->subunit == 0);

	port = u->port;
	unit_map = ((int8_t *) u->u_rxbuf)[2];
	if (sc->sc_port_unit_map[port] == unit_map)
		return;

	units = ((unit_map & 0x1f) << 1) | 1;
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 1
	{
		char buf[16];
		printf("%s: unit_map 0x%x -> 0x%x (units 0x%x)\n",
		    maple_unit_name(buf, u->port, u->subunit),
		    sc->sc_port_unit_map[port], unit_map, units);
	}
#endif
#if 0	/* this detects unit removal rapidly but is not reliable */
	/* check for unit change */
	un = sc->sc_port_units[port] & ~units;

	/* detach removed devices */
	for (i = MAPLE_SUBUNITS - 1; i > 0; i--)
		if (un & (1 << i))
			maple_detach_unit_nofix(sc, &sc->sc_unit[port][i]);
#endif

	sc->sc_port_unit_map[port] = unit_map;

	/* schedule scanning child devices */
	un = units & ~sc->sc_port_units[port];
	for (i = MAPLE_SUBUNITS - 1; i > 0; i--)
		if (un & (1 << i)) {
			u1 = &sc->sc_unit[port][i];
			maple_remove_from_queues(sc, u1);
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 2
			{
				char buf[16];
				printf("%s: queued to probe 2\n",
				    maple_unit_name(buf, u1->port, u1->subunit));
			}
#endif
			TAILQ_INSERT_HEAD(&sc->sc_probeq, u1, u_q);
			u1->u_queuestat = MAPLE_QUEUE_PROBE;
			u1->u_proberetry = 0;
		}
}

static void
maple_check_unit_change(struct maple_softc *sc, struct maple_unit *u)
{
	struct maple_devinfo *newinfo = (void *) (u->u_rxbuf + 1);
	int port, subunit;

	port = u->port;
	subunit = u->subunit;
	if (memcmp(&u->devinfo, newinfo, sizeof(struct maple_devinfo)) == 0)
		goto out;	/* no change */

	/* unit inserted */

	/* attach this device */
	u->devinfo = *newinfo;
	maple_attach_unit(sc, u);

out:
	maple_remove_from_queues(sc, u);
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 2
	{
		char buf[16];
		printf("%s: queued to ping\n",
		    maple_unit_name(buf, u->port, u->subunit));
	}
#endif
	TAILQ_INSERT_TAIL(&sc->sc_pingq, u, u_q);
	u->u_queuestat = MAPLE_QUEUE_PING;
}

static void
maple_print_unit(void *aux, const char *pnp)
{
	struct maple_attach_args *ma = aux;
	int port, subunit;
	char buf[16];
	char *prod, *p, oc;

	port = ma->ma_unit->port;
	subunit = ma->ma_unit->subunit;

	if (pnp != NULL)
		printf("%s at %s", maple_unit_name(buf, port, subunit), pnp);

	printf(" port %d", port);

	if (subunit != 0)
		printf(" subunit %d", subunit);

#ifdef MAPLE_DEBUG
	printf(": a %#x c %#x fn %#x d %#x,%#x,%#x",
	    ma->ma_devinfo->di_area_code,
	    ma->ma_devinfo->di_connector_direction,
	    be32toh(ma->ma_devinfo->di_func),
	    be32toh(ma->ma_devinfo->di_function_data[0]),
	    be32toh(ma->ma_devinfo->di_function_data[1]),
	    be32toh(ma->ma_devinfo->di_function_data[2]));
#endif

	/* nul termination */
	prod = ma->ma_devinfo->di_product_name;
	for (p = prod + sizeof ma->ma_devinfo->di_product_name; p >= prod; p--)
		if (p[-1] != '\0' && p[-1] != ' ')
			break;
	oc = *p;
	*p = '\0';

	printf(": %s", prod);

	*p = oc;	/* restore */
}

static int
maplesubmatch(struct device *parent, struct cfdata *match,
	      const int *ldesc, void *aux)
{
	struct maple_attach_args *ma = aux;

	if (match->cf_loc[MAPLECF_PORT] != MAPLECF_PORT_DEFAULT &&
	    match->cf_loc[MAPLECF_PORT] != ma->ma_unit->port)
		return 0;

	if (match->cf_loc[MAPLECF_SUBUNIT] != MAPLECF_SUBUNIT_DEFAULT &&
	    match->cf_loc[MAPLECF_SUBUNIT] != ma->ma_unit->subunit)
		return 0;

	return config_match(parent, match, aux);
}

static int
mapleprint(void *aux, const char *str)
{
	struct maple_attach_args *ma = aux;

#ifdef MAPLE_DEBUG
	if (str)
		aprint_normal("%s", str);
	aprint_normal(" function %d", ma->ma_function);

	return UNCONF;
#else	/* quiet */
	if (!str)
		aprint_normal(" function %d", ma->ma_function);

	return QUIET;
#endif
}

static void
maple_attach_unit(struct maple_softc *sc, struct maple_unit *u)
{
	struct maple_attach_args ma;
	uint32_t func;
	int f;
	char oldxname[16];

	ma.ma_unit = u;
	ma.ma_devinfo = &u->devinfo;
	ma.ma_basedevinfo = &sc->sc_unit[u->port][0].devinfo;
	func = be32toh(ma.ma_devinfo->di_func);

	maple_print_unit(&ma, sc->sc_dev.dv_xname);
	printf("\n");
	strcpy(oldxname, sc->sc_dev.dv_xname);
	maple_unit_name(sc->sc_dev.dv_xname, u->port, u->subunit);

	for (f = 0; f < MAPLE_NFUNC; f++) {
		u->u_func[f].f_callback = NULL;
		u->u_func[f].f_arg = NULL;
		u->u_func[f].f_cmdstat = MAPLE_CMDSTAT_NONE;
		u->u_func[f].f_dev = NULL;
		if (func & MAPLE_FUNC(f)) {
			ma.ma_function = f;
			u->u_func[f].f_dev = config_found_sm_loc(&sc->sc_dev,
			    "maple", NULL, &ma, mapleprint, maplesubmatch);
			u->u_ping_func = f;	/* XXX using largest func */
		}
	}
#ifdef MAPLE_MEMCARD_PING_HACK
	/*
	 * Some 3rd party memory card pretend to be Visual Memory,
	 * but need special handling for ping.
	 */
	if (func == (MAPLE_FUNC(MAPLE_FN_MEMCARD) | MAPLE_FUNC(MAPLE_FN_LCD) |
	    MAPLE_FUNC(MAPLE_FN_CLOCK))) {
		u->u_ping_func = MAPLE_FN_MEMCARD;
		u->u_ping_stat = MAPLE_PING_MEMCARD;
	} else {
		u->u_ping_stat = MAPLE_PING_NORMAL;
	}
#endif
	strcpy(sc->sc_dev.dv_xname, oldxname);

	sc->sc_port_units[u->port] |= 1 << u->subunit;
}

static void
maple_detach_unit_nofix(struct maple_softc *sc, struct maple_unit *u)
{
	struct maple_func *fn;
	struct device *dev;
	struct maple_unit *u1;
	int port;
	int error;
	int i;
	char buf[16];

#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 1
	printf("%s: remove\n", maple_unit_name(buf, u->port, u->subunit));
#endif
	maple_remove_from_queues(sc, u);
	port = u->port;
	sc->sc_port_units[port] &= ~(1 << u->subunit);

	if (u->subunit == 0) {
		for (i = MAPLE_SUBUNITS - 1; i > 0; i--)
			maple_detach_unit_nofix(sc, &sc->sc_unit[port][i]);
	}

	for (fn = u->u_func; fn < &u->u_func[MAPLE_NFUNC]; fn++) {
		if ((dev = fn->f_dev) != NULL) {
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 1
			printf("%s: detaching func %d\n",
			    maple_unit_name(buf, port, u->subunit),
			    fn->f_funcno);
#endif

			/*
			 * Remove functions from command queue.
			 */
			switch (fn->f_cmdstat) {
			case MAPLE_CMDSTAT_ASYNC:
			case MAPLE_CMDSTAT_PERIODIC_DEFERED:
				TAILQ_REMOVE(&sc->sc_acmdq, fn, f_cmdq);
				break;
			case MAPLE_CMDSTAT_ASYNC_PERIODICQ:
			case MAPLE_CMDSTAT_PERIODIC:
				TAILQ_REMOVE(&sc->sc_pcmdq, fn, f_cmdq);
				break;
			default:
				break;
			}

			/*
			 * Detach devices.
			 */
			if ((error = config_detach(fn->f_dev, DETACH_FORCE))) {
				printf("%s: failed to detach %s (func %d), errno %d\n",
				    maple_unit_name(buf, port, u->subunit),
				    fn->f_dev->dv_xname, fn->f_funcno, error);
			}
		}

		maple_enable_periodic(&sc->sc_dev, u, fn->f_funcno, 0);

		fn->f_dev = NULL;
		fn->f_callback = NULL;
		fn->f_arg = NULL;
		fn->f_cmdstat = MAPLE_CMDSTAT_NONE;
	}
	if (u->u_dma_stat == MAPLE_DMA_RETRY) {
		/* XXX expensive? */
		SIMPLEQ_FOREACH(u1, &sc->sc_retryq, u_dmaq) {
			if (u1 == u) {
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 1
				printf("%s: abort retry\n",
				    maple_unit_name(buf, port, u->subunit));
#endif
				SIMPLEQ_REMOVE(&sc->sc_retryq, u, maple_unit,
				    u_dmaq);
				break;
			}
		}
	}
	u->u_dma_stat = MAPLE_DMA_IDLE;
	u->u_noping = 0;
	/* u->u_dma_func = uninitialized; */
	KASSERT(u->getcond_func_set == 0);
	memset(&u->devinfo, 0, sizeof(struct maple_devinfo));

	if (u->subunit == 0) {
		sc->sc_port_unit_map[port] = 0;
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 2
		{
			char buf[16];
			printf("%s: queued to probe 3\n",
			    maple_unit_name(buf, port, u->subunit));
		}
#endif
		TAILQ_INSERT_TAIL(&sc->sc_probeq, u, u_q);
		u->u_queuestat = MAPLE_QUEUE_PROBE;
	}
}

static void
maple_detach_unit(struct maple_softc *sc, struct maple_unit *u)
{

	maple_detach_unit_nofix(sc, u);
	if (u->subunit != 0)
		sc->sc_port_unit_map[u->port] &= ~(1 << (u->subunit - 1));
}

/*
 * Send a command (called by drivers)
 *
 * The "cataaddr" must not point at temporary storage like stack.
 * Only one command (per function) is valid at a time.
 */
void
maple_command(struct device *dev, struct maple_unit *u, int func,
	int command, int datalen, const void *dataaddr, int flags)
{
	struct maple_softc *sc = (void *) dev;
	struct maple_func *fn;
	int s;

	KASSERT(func >= 0 && func < 32);
	KASSERT(command);
	KASSERT((flags & ~MAPLE_FLAG_CMD_PERIODIC_TIMING) == 0);

	s = splsoftclock();

	fn = &u->u_func[func];
#if 1 /*def DIAGNOSTIC*/
	{char buf[16];
	if (fn->f_cmdstat != MAPLE_CMDSTAT_NONE)
		panic("maple_command: %s func %d: requesting more than one commands",
		    maple_unit_name(buf, u->port, u->subunit), func);
	}
#endif
	fn->f_command = command;
	fn->f_datalen = datalen;
	fn->f_dataaddr = dataaddr;
	if (flags & MAPLE_FLAG_CMD_PERIODIC_TIMING) {
		fn->f_cmdstat = MAPLE_CMDSTAT_PERIODIC;
		TAILQ_INSERT_TAIL(&sc->sc_pcmdq, fn, f_cmdq);
	} else {
		fn->f_cmdstat = MAPLE_CMDSTAT_ASYNC;
		TAILQ_INSERT_TAIL(&sc->sc_acmdq, fn, f_cmdq);
		wakeup(&sc->sc_event);	/* wake for async event */
	}
	splx(s);
}

static void
maple_queue_cmds(struct maple_softc *sc,
	struct maple_cmdq_head *head)
{
	struct maple_func *fn, *nextfn;
	struct maple_unit *u;

	/*
	 * Note: since the queue element may be queued immediately,
	 *	 we can't use TAILQ_FOREACH.
	 */
	fn = TAILQ_FIRST(head);
	TAILQ_INIT(head);
	for ( ; fn; fn = nextfn) {
		nextfn = TAILQ_NEXT(fn, f_cmdq);

		KASSERT(fn->f_cmdstat != MAPLE_CMDSTAT_NONE);
		u = fn->f_unit;
		if (u->u_dma_stat == MAPLE_DMA_IDLE) {
			maple_write_command(sc, u,
			    fn->f_command, fn->f_datalen, fn->f_dataaddr);
			u->u_dma_stat = (fn->f_cmdstat == MAPLE_CMDSTAT_ASYNC ||
			    fn->f_cmdstat == MAPLE_CMDSTAT_ASYNC_PERIODICQ) ?
			    MAPLE_DMA_ACMD : MAPLE_DMA_PCMD;
			u->u_dma_func = fn->f_funcno;
			fn->f_cmdstat = MAPLE_CMDSTAT_NONE;
		} else if (u->u_dma_stat == MAPLE_DMA_RETRY) {
			/* unit is busy --- try again */
			/*
			 * always add to periodic command queue
			 * (wait until the next periodic timing),
			 * since the unit will never be freed until the
			 * next periodic timing.
			 */
			switch (fn->f_cmdstat) {
			case MAPLE_CMDSTAT_ASYNC:
				fn->f_cmdstat = MAPLE_CMDSTAT_ASYNC_PERIODICQ;
				break;
			case MAPLE_CMDSTAT_PERIODIC_DEFERED:
				fn->f_cmdstat = MAPLE_CMDSTAT_PERIODIC;
				break;
			default:
				break;
			}
			TAILQ_INSERT_TAIL(&sc->sc_pcmdq, fn, f_cmdq);
		} else {
			/* unit is busy --- try again */
			/*
			 * always add to async command queue
			 * (process immediately)
			 */
			switch (fn->f_cmdstat) {
			case MAPLE_CMDSTAT_ASYNC_PERIODICQ:
				fn->f_cmdstat = MAPLE_CMDSTAT_ASYNC;
				break;
			case MAPLE_CMDSTAT_PERIODIC:
				fn->f_cmdstat = MAPLE_CMDSTAT_PERIODIC_DEFERED;
				break;
			default:
				break;
			}
			TAILQ_INSERT_TAIL(&sc->sc_acmdq, fn, f_cmdq);
		}
	}
}

/* schedule probing a device */
static void
maple_unit_probe(struct maple_softc *sc)
{
	struct maple_unit *u;

	if ((u = TAILQ_FIRST(&sc->sc_probeq)) != NULL) {
		KASSERT(u->u_dma_stat == MAPLE_DMA_IDLE);
		KASSERT(u->u_queuestat == MAPLE_QUEUE_PROBE);
		maple_remove_from_queues(sc, u);
		maple_write_command(sc, u, MAPLE_COMMAND_DEVINFO, 0, NULL);
		u->u_dma_stat = MAPLE_DMA_PROBE;
		/* u->u_dma_func = ignored; */
	}
}

/*
 * Enable/disable unit pinging (called by drivers)
 */
/* ARGSUSED */
void
maple_enable_unit_ping(struct device *dev, struct maple_unit *u,
	int func, int enable)
{
#if 0	/* currently unused */
	struct maple_softc *sc = (void *) dev;
#endif

	if (enable)
		u->u_noping &= ~MAPLE_FUNC(func);
	else
		u->u_noping |= MAPLE_FUNC(func);
}

/* schedule pinging a device */
static void
maple_unit_ping(struct maple_softc *sc)
{
	struct maple_unit *u;
	struct maple_func *fn;
#ifdef MAPLE_MEMCARD_PING_HACK
	static const uint32_t memcard_ping_arg[2] = {
		0x02000000,	/* htobe32(MAPLE_FUNC(MAPLE_FN_MEMCARD)) */
		0		/* pt (1 byte) and unused 3 bytes */
	};
#endif

	if ((u = TAILQ_FIRST(&sc->sc_pingq)) != NULL) {
		KASSERT(u->u_queuestat == MAPLE_QUEUE_PING);
		maple_remove_from_queues(sc, u);
		if (u->u_dma_stat == MAPLE_DMA_IDLE && u->u_noping == 0) {
#ifdef MAPLE_MEMCARD_PING_HACK
			if (u->u_ping_stat == MAPLE_PING_MINFO) {
				/* use MINFO for some memory cards */
				maple_write_command(sc, u,
				    MAPLE_COMMAND_GETMINFO,
				    2, memcard_ping_arg);
			} else
#endif
			{
				fn = &u->u_func[u->u_ping_func];
				fn->f_work = htobe32(MAPLE_FUNC(u->u_ping_func));
				maple_write_command(sc, u,
				    MAPLE_COMMAND_GETCOND,
				    1, &fn->f_work);
			}
			u->u_dma_stat = MAPLE_DMA_PING;
			/* u->u_dma_func = XXX; */
		} else {
			/* no need if periodic */
			TAILQ_INSERT_TAIL(&sc->sc_pingq, u, u_q);
			u->u_queuestat = MAPLE_QUEUE_PING;
		}
	}
}

/*
 * Enable/disable periodic GETCOND (called by drivers)
 */
void
maple_enable_periodic(struct device *dev, struct maple_unit *u,
	int func, int on)
{
	struct maple_softc *sc = (void *) dev;
	struct maple_func *fn;

	KASSERT(func >= 0 && func < 32);

	fn = &u->u_func[func];

	if (on) {
		if (fn->f_periodic_stat == MAPLE_PERIODIC_NONE) {
			TAILQ_INSERT_TAIL(&sc->sc_periodicq, fn, f_periodicq);
			fn->f_periodic_stat = MAPLE_PERIODIC_INQ;
			u->getcond_func_set |= MAPLE_FUNC(func);
		}
	} else {
		if (fn->f_periodic_stat == MAPLE_PERIODIC_INQ)
			TAILQ_REMOVE(&sc->sc_periodicq, fn, f_periodicq);
		else if (fn->f_periodic_stat == MAPLE_PERIODIC_DEFERED)
			TAILQ_REMOVE(&sc->sc_periodicdeferq, fn, f_periodicq);
		fn->f_periodic_stat = MAPLE_PERIODIC_NONE;
		u->getcond_func_set &= ~MAPLE_FUNC(func);
	}
}

/*
 * queue periodic GETCOND
 */
static int
maple_send_defered_periodic(struct maple_softc *sc)
{
	struct maple_unit *u;
	struct maple_func *fn, *nextfn;
	int defer_remain = 0;

	for (fn = TAILQ_FIRST(&sc->sc_periodicdeferq); fn; fn = nextfn) {
		KASSERT(fn->f_periodic_stat == MAPLE_PERIODIC_DEFERED);

		nextfn = TAILQ_NEXT(fn, f_periodicq);

		u = fn->f_unit;
		if (u->u_dma_stat == MAPLE_DMA_IDLE ||
		    u->u_dma_stat == MAPLE_DMA_RETRY) {
			/*
			 * if IDLE  ->	queue this request
			 * if RETRY ->	the unit never be freed until the next
			 *		periodic timing, so just restore to
			 *		the normal periodic queue.
			 */
			TAILQ_REMOVE(&sc->sc_periodicdeferq, fn, f_periodicq);
			TAILQ_INSERT_TAIL(&sc->sc_periodicq, fn, f_periodicq);
			fn->f_periodic_stat = MAPLE_PERIODIC_INQ;

			if (u->u_dma_stat == MAPLE_DMA_IDLE) {
				/*
				 * queue periodic command
				 */
				fn->f_work = htobe32(MAPLE_FUNC(fn->f_funcno));
				maple_write_command(sc, u,
				    MAPLE_COMMAND_GETCOND, 1, &fn->f_work);
				u->u_dma_stat = MAPLE_DMA_PERIODIC;
				u->u_dma_func = fn->f_funcno;
			}
		} else {
			defer_remain = 1;
		}
	}

	return defer_remain;
}

static void
maple_send_periodic(struct maple_softc *sc)
{
	struct maple_unit *u;
	struct maple_func *fn, *nextfn;

	for (fn = TAILQ_FIRST(&sc->sc_periodicq); fn; fn = nextfn) {
		KASSERT(fn->f_periodic_stat == MAPLE_PERIODIC_INQ);

		nextfn = TAILQ_NEXT(fn, f_periodicq);

		u = fn->f_unit;
		if (u->u_dma_stat != MAPLE_DMA_IDLE) {
			if (u->u_dma_stat != MAPLE_DMA_RETRY) {
				/*
				 * can't be queued --- move to defered queue
				 */
				TAILQ_REMOVE(&sc->sc_periodicq, fn,
				    f_periodicq);
				TAILQ_INSERT_TAIL(&sc->sc_periodicdeferq, fn,
				    f_periodicq);
				fn->f_periodic_stat = MAPLE_PERIODIC_DEFERED;
			}
		} else {
			/*
			 * queue periodic command
			 */
			fn->f_work = htobe32(MAPLE_FUNC(fn->f_funcno));
			maple_write_command(sc, u, MAPLE_COMMAND_GETCOND,
			    1, &fn->f_work);
			u->u_dma_stat = MAPLE_DMA_PERIODIC;
			u->u_dma_func = fn->f_funcno;
		}
	}
}

static void
maple_remove_from_queues(struct maple_softc *sc, struct maple_unit *u)
{

	/* remove from queues */
	if (u->u_queuestat == MAPLE_QUEUE_PROBE)
		TAILQ_REMOVE(&sc->sc_probeq, u, u_q);
	else if (u->u_queuestat == MAPLE_QUEUE_PING)
		TAILQ_REMOVE(&sc->sc_pingq, u, u_q);
#ifdef DIAGNOSTIC
	else if (u->u_queuestat != MAPLE_QUEUE_NONE)
		panic("maple_remove_from_queues: queuestat %d", u->u_queuestat);
#endif
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 2
	if (u->u_queuestat != MAPLE_QUEUE_NONE) {
		char buf[16];
		printf("%s: dequeued\n",
		    maple_unit_name(buf, u->port, u->subunit));
	}
#endif

	u->u_queuestat = MAPLE_QUEUE_NONE;
}

/*
 * retry current command at next periodic timing
 */
static int
maple_retry(struct maple_softc *sc, struct maple_unit *u,
	enum maple_dma_stat st)
{

	KASSERT(st != MAPLE_DMA_IDLE && st != MAPLE_DMA_RETRY);

#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 2
	if (u->u_retrycnt == 0) {
		char buf[16];
		printf("%s: retrying: %#x, %#x, %p\n",
		    maple_unit_name(buf, u->port, u->subunit),
		    u->u_command, u->u_datalen, u->u_dataaddr);
	}
#endif
	if (u->u_retrycnt >= MAPLE_RETRY_MAX)
		return 1;

	u->u_retrycnt++;

	u->u_saved_dma_stat = st;
	u->u_dma_stat = MAPLE_DMA_RETRY; /* no new command before retry done */
	SIMPLEQ_INSERT_TAIL(&sc->sc_retryq, u, u_dmaq);

	return 0;
}

static void
maple_queue_retry(struct maple_softc *sc)
{
	struct maple_unit *u, *nextu;

	/*
	 * Note: since the queue element is queued immediately
	 *	 in maple_queue_command, we can't use SIMPLEQ_FOREACH.
	 */
	for (u = SIMPLEQ_FIRST(&sc->sc_retryq); u; u = nextu) {
		nextu = SIMPLEQ_NEXT(u, u_dmaq);

		/*
		 * Retrying is in the highest priority, and the unit shall
		 * always be free.
		 */
		KASSERT(u->u_dma_stat == MAPLE_DMA_RETRY);
		maple_queue_command(sc, u, u->u_command, u->u_datalen,
		    u->u_dataaddr);
		u->u_dma_stat = u->u_saved_dma_stat;

#ifdef DIAGNOSTIC
		KASSERT(u->u_saved_dma_stat != MAPLE_DMA_IDLE);
		u->u_saved_dma_stat = MAPLE_DMA_IDLE;
#endif
	}
	SIMPLEQ_INIT(&sc->sc_retryq);
}

/*
 * Process DMA results.
 * Requires kernel context.
 */
static void
maple_check_responses(struct maple_softc *sc)
{
	struct maple_unit *u, *nextu;
	struct maple_func *fn;
	maple_response_t response;
	int func_code, len;
	int flags;
	char buf[16];

	/*
	 * Note: since the queue element may be queued immediately,
	 *	 we can't use SIMPLEQ_FOREACH.
	 */
	for (u = SIMPLEQ_FIRST(&sc->sc_dmaq), maple_begin_txbuf(sc);
	    u; u = nextu) {
		nextu = SIMPLEQ_NEXT(u, u_dmaq);

		if (u->u_dma_stat == MAPLE_DMA_IDLE)
			continue;	/* just detached or DDB was active */

		/*
		 * check for retransmission
		 */
		if ((response = u->u_rxbuf[0]) == MAPLE_RESPONSE_AGAIN) {
			if (maple_retry(sc, u, u->u_dma_stat) == 0)
				continue;
			/* else pass error to upper layer */
		}

		len = (u->u_rxbuf[0] >> 24);	/* length in long */
		len <<= 2;			/* length in byte */

		/*
		 * call handler
		 */
		if (u->u_dma_stat == MAPLE_DMA_PERIODIC) {
			/*
			 * periodic GETCOND
			 */
			u->u_dma_stat = MAPLE_DMA_IDLE;
			func_code = u->u_dma_func;
			if (response == MAPLE_RESPONSE_DATATRF && len > 0 &&
			    be32toh(u->u_rxbuf[1]) == MAPLE_FUNC(func_code)) {
				fn = &u->u_func[func_code];
				if (fn->f_dev)
					(*fn->f_callback)(fn->f_arg,
					    (void *)u->u_rxbuf, len,
					    MAPLE_FLAG_PERIODIC);
			} else if (response == MAPLE_RESPONSE_NONE) {
				/* XXX OK? */
				/* detach */
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 2
				printf("%s: func: %d: periodic response %d\n",
				    maple_unit_name(buf, u->port, u->subunit),
				    u->u_dma_func,
				    response);
#endif
				/*
				 * Some 3rd party devices sometimes
				 * do not respond.
				 */
				if (maple_retry(sc, u, MAPLE_DMA_PERIODIC))
					maple_detach_unit(sc, u);
			}
			/* XXX check unexpected conditions? */

		} else if (u->u_dma_stat == MAPLE_DMA_PROBE) {
			KASSERT(u->u_queuestat == MAPLE_QUEUE_NONE);
			u->u_dma_stat = MAPLE_DMA_IDLE;
			switch (response) {
			default:
			case MAPLE_RESPONSE_NONE:
				/*
				 * Do not use maple_retry(), which conflicts
				 * with probe structure.
				 */
				if (u->subunit != 0 &&
				    ++u->u_proberetry > MAPLE_PROBERETRY_MAX) {
					printf("%s: no response\n",
					    maple_unit_name(buf,
						u->port, u->subunit));
				} else {
					/* probe again */
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 2
					printf("%s: queued to probe 4\n",
					    maple_unit_name(buf, u->port, u->subunit));
#endif
					TAILQ_INSERT_TAIL(&sc->sc_probeq, u,
					    u_q);
					u->u_queuestat = MAPLE_QUEUE_PROBE;
				}
				break;
			case MAPLE_RESPONSE_DEVINFO:
				/* check if the unit is changed */
				maple_check_unit_change(sc, u);
				break;
			}

		} else if (u->u_dma_stat == MAPLE_DMA_PING) {
			KASSERT(u->u_queuestat == MAPLE_QUEUE_NONE);
			u->u_dma_stat = MAPLE_DMA_IDLE;
			switch (response) {
			default:
			case MAPLE_RESPONSE_NONE:
				/*
				 * Some 3rd party devices sometimes
				 * do not respond.
				 */
				if (maple_retry(sc, u, MAPLE_DMA_PING)) {
					/* detach */
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 1
					printf("%s: ping response %d\n",
					    maple_unit_name(buf, u->port,
						u->subunit),
					    response);
#endif
#ifdef MAPLE_MEMCARD_PING_HACK
					if (u->u_ping_stat
					    == MAPLE_PING_MEMCARD) {
						/*
						 * The unit claims itself to be
						 * a Visual Memory, and has
						 * never responded to GETCOND.
						 * Try again using MINFO, in
						 * case it is a poorly
						 * implemented 3rd party card.
						 */
#ifdef MAPLE_DEBUG
						printf("%s: switching ping method\n",
						    maple_unit_name(buf,
							u->port, u->subunit));
#endif
						u->u_ping_stat
						    = MAPLE_PING_MINFO;
						TAILQ_INSERT_TAIL(&sc->sc_pingq,
						    u, u_q);
						u->u_queuestat
						    = MAPLE_QUEUE_PING;
					} else
#endif	/* MAPLE_MEMCARD_PING_HACK */
					maple_detach_unit(sc, u);
				}
				break;
			case MAPLE_RESPONSE_BADCMD:
			case MAPLE_RESPONSE_BADFUNC:
			case MAPLE_RESPONSE_DATATRF:
				TAILQ_INSERT_TAIL(&sc->sc_pingq, u, u_q);
				u->u_queuestat = MAPLE_QUEUE_PING;
#ifdef MAPLE_MEMCARD_PING_HACK
				/*
				 * If the unit responds to GETCOND, it is a
				 * normal implementation.
				 */
				if (u->u_ping_stat == MAPLE_PING_MEMCARD)
					u->u_ping_stat = MAPLE_PING_NORMAL;
#endif
				break;
			}

		} else {
			/*
			 * Note: Do not rely on the consistency of responses.
			 */

			if (response == MAPLE_RESPONSE_NONE) {
				if (maple_retry(sc, u, u->u_dma_stat)) {
					/* detach */
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 1
					printf("%s: command response %d\n",
					    maple_unit_name(buf, u->port,
						u->subunit),
					    response);
#endif
					maple_detach_unit(sc, u);
				}
				continue;
			}

			flags = (u->u_dma_stat == MAPLE_DMA_PCMD) ?
			    MAPLE_FLAG_CMD_PERIODIC_TIMING : 0;
			u->u_dma_stat = MAPLE_DMA_IDLE;

			func_code = u->u_dma_func;
			fn = &u->u_func[func_code];
			if (fn->f_dev == NULL) {
				/* detached right now */
#ifdef MAPLE_DEBUG
				printf("%s: unknown function: function %d, response %d\n",
				    maple_unit_name(buf, u->port, u->subunit),
				    func_code, response);
#endif
				continue;
			}
			if (fn->f_callback != NULL) {
				(*fn->f_callback)(fn->f_arg,
				    (void *)u->u_rxbuf, len, flags);
			}
		}

		/*
		 * check for subunit change and schedule probing subunits
		 */
		if (u->subunit == 0 && response != MAPLE_RESPONSE_NONE &&
		    response != MAPLE_RESPONSE_AGAIN &&
		    ((int8_t *) u->u_rxbuf)[2] != sc->sc_port_unit_map[u->port])
			maple_check_subunit_change(sc, u);
	}
}

/*
 * Main Maple Bus thread
 */
static void
maple_event_thread(void *arg)
{
	struct maple_softc *sc = arg;
	unsigned cnt = 1;	/* timing counter */
	int s;
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 1
	int noreq = 0;
#endif

#ifdef MAPLE_DEBUG
	printf("%s: forked event thread, pid %d\n",
	    sc->sc_dev.dv_xname, sc->event_thread->p_pid);
#endif

	/* begin first DMA cycle */
	maple_begin_txbuf(sc);

	sc->sc_event = 1;

	/* OK, continue booting system */
	maple_polling = 0;
	config_pending_decr();

	for (;;) {
		/*
		 * queue requests
		 */

		/* queue async commands */
		if (!TAILQ_EMPTY(&sc->sc_acmdq))
			maple_queue_cmds(sc, &sc->sc_acmdq);

		/* send defered periodic command */
		if (!TAILQ_EMPTY(&sc->sc_periodicdeferq))
			maple_send_defered_periodic(sc);

		/* queue periodic commands */
		if (sc->sc_event) {
			/* queue commands on periodic timing */
			if (!TAILQ_EMPTY(&sc->sc_pcmdq))
				maple_queue_cmds(sc, &sc->sc_pcmdq);

			/* retry */
			if (!SIMPLEQ_EMPTY(&sc->sc_retryq))
				maple_queue_retry(sc);

			if ((cnt & 31) == 0)	/* XXX */
				maple_unit_probe(sc);
			cnt++;

			maple_send_periodic(sc);
			if ((cnt & 7) == 0)	/* XXX */
				maple_unit_ping(sc);

			/*
			 * schedule periodic event
			 */
			sc->sc_event = 0;
			callout_reset(&sc->maple_callout_ch,
			    MAPLE_CALLOUT_TICKS, maple_callout, sc);
		}

		if (maple_end_txbuf(sc)) {

			/*
			 * start DMA
			 */
			s = splmaple();
			maple_start(sc);

			/*
			 * wait until DMA done
			 */
			if (tsleep(&sc->sc_dmadone, PWAIT, "mdma", hz)
			    == EWOULDBLOCK) {
				/* was DDB active? */
				printf("%s: timed out\n", sc->sc_dev.dv_xname);
			}
			splx(s);

			/*
			 * call handlers
			 */
			maple_check_responses(sc);
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 1
			noreq = 0;
#endif
		}
#if defined(MAPLE_DEBUG) && MAPLE_DEBUG > 1
		else {
			/* weird if occurs in succession */
#if MAPLE_DEBUG <= 2
			if (noreq)	/* ignore first time */
#endif
				printf("%s: no request %d\n",
				    sc->sc_dev.dv_xname, noreq);
			noreq++;
		}
#endif

		/*
		 * wait for an event
		 */
		s = splsoftclock();
		if (TAILQ_EMPTY(&sc->sc_acmdq) && sc->sc_event == 0 &&
		    TAILQ_EMPTY(&sc->sc_periodicdeferq)) {
			if (tsleep(&sc->sc_event, PWAIT, "mslp", hz)
			    == EWOULDBLOCK) {
				printf("%s: event timed out\n",
				    sc->sc_dev.dv_xname);
			}

		}
		splx(s);

	}

#if 0	/* maple root device can't be detached */
	kthread_exit(0);
	/* NOTREACHED */
#endif
}

static int
maple_intr(void *arg)
{
	struct maple_softc *sc = arg;

	wakeup(&sc->sc_dmadone);

	return 1;
}

static void
maple_callout(void *ctx)
{
	struct maple_softc *sc = ctx;

	sc->sc_event = 1;	/* mark as periodic event */
	wakeup(&sc->sc_event);
}

/*
 * Install callback handler (called by drivers)
 */
/* ARGSUSED */
void
maple_set_callback(struct device *dev, struct maple_unit *u, int func,
	void (*callback)(void *, struct maple_response *, int, int), void *arg)
{
#if 0	/* currently unused */
	struct maple_softc *sc = (void *) dev;
#endif
	struct maple_func *fn;

	KASSERT(func >= 0 && func < MAPLE_NFUNC);

	fn = &u->u_func[func];

	fn->f_callback = callback;
	fn->f_arg = arg;
}

/*
 * Return function definition data (called by drivers)
 */
uint32_t
maple_get_function_data(struct maple_devinfo *devinfo, int function_code)
{
	int i, p = 0;
	uint32_t func;

	func = be32toh(devinfo->di_func);
	for (i = 31; i >= 0; --i)
		if (func & MAPLE_FUNC(i)) {
			if (function_code == i)
				return be32toh(devinfo->di_function_data[p]);
			else
				if (++p >= 3)
					break;
		}

	return 0;
}

/* Generic maple device interface */

int
mapleopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct maple_softc *sc;

	sc = device_lookup(&maple_cd, MAPLEBUSUNIT(dev));
	if (sc == NULL)			/* make sure it was attached */
		return ENXIO;

	if (MAPLEPORT(dev) >= MAPLE_PORTS)
		return ENXIO;

	if (MAPLESUBUNIT(dev) >= MAPLE_SUBUNITS)
		return ENXIO;

	if (!(sc->sc_port_units[MAPLEPORT(dev)] & (1 << MAPLESUBUNIT(dev))))
		return ENXIO;

	sc->sc_port_units_open[MAPLEPORT(dev)] |= 1 << MAPLESUBUNIT(dev);

	return 0;
}

int
mapleclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct maple_softc *sc;

	sc = device_lookup(&maple_cd, MAPLEBUSUNIT(dev));

	sc->sc_port_units_open[MAPLEPORT(dev)] &= ~(1 << MAPLESUBUNIT(dev));

	return 0;
}

int
maple_unit_ioctl(struct device *dev, struct maple_unit *u, u_long cmd,
    void *data, int flag, struct lwp *l)
{
	struct maple_softc *sc = (struct maple_softc *)dev;

	if (!(sc->sc_port_units[u->port] & (1 << u->subunit)))
		return ENXIO;

	switch(cmd) {
	case MAPLEIO_GDEVINFO:
		memcpy(data, &u->devinfo, sizeof(struct maple_devinfo));
		break;
	default:
		return EPASSTHROUGH;
	}

	return 0;
}

int
mapleioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct maple_softc *sc;
	struct maple_unit *u;

	sc = device_lookup(&maple_cd, MAPLEBUSUNIT(dev));
	u = &sc->sc_unit[MAPLEPORT(dev)][MAPLESUBUNIT(dev)];

	return maple_unit_ioctl(&sc->sc_dev, u, cmd, data, flag, l);
}
