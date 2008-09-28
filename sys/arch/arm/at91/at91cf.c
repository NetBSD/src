/*	$Id: at91cf.c,v 1.1.16.1 2008/09/28 10:39:48 mjf Exp $	*/
/*	$NetBSD: at91cf.c,v 1.1.16.1 2008/09/28 10:39:48 mjf Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
 *
 * Based on arch/evbarm/ep93xx/eppcic.c,
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: at91cf.c,v 1.1.16.1 2008/09/28 10:39:48 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/kthread.h>
#include <uvm/uvm_param.h>
#include <machine/bus.h>
#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>
#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>
#include <arm/at91/at91cfvar.h>

#include "at91pio.h"
#if NAT91PIO == 0
#error "at91cf requires at91pio"
#endif

#ifdef AT91CF_DEBUG
int at91cf_debug = AT91CF_DEBUG;
#define DPRINTFN(n,x)	if (at91cf_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)
#endif

struct at91cf_handle {
	struct at91cf_softc	*ph_sc;
	device_t		ph_card;
	int			(*ph_ih_func)(void *);
	void			*ph_ih_arg;
	lwp_t			*ph_event_thread;
	int			ph_type;	/* current access type */
	int			ph_run;		/* kthread running */
	int			ph_width;	/* 8 or 16 */
	int			ph_vcc;		/* 3 or 5 */
	int			ph_status;	/* combined cd1 and cd2 */
	struct {
		bus_size_t	reg;
		bus_addr_t	base;
		bus_size_t	size;
	} ph_space[3];
#define	IO		0
#define	COMMON		1
#define	ATTRIBUTE	2
};

static int at91cf_intr_carddetect(void *);
static int at91cf_intr_socket(void *);
static int at91cf_print(void *, const char *);
static void at91cf_create_event_thread(void *);
static void at91cf_event_thread(void *);
void at91cf_shutdown(void *);

static int at91cf_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
			    struct pcmcia_mem_handle *);
static void at91cf_mem_free(pcmcia_chipset_handle_t,
			    struct pcmcia_mem_handle *);
static int at91cf_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t,
			  struct pcmcia_mem_handle *, bus_size_t *, int *);
static void at91cf_mem_unmap(pcmcia_chipset_handle_t, int);
static int at91cf_io_alloc(pcmcia_chipset_handle_t, bus_addr_t, bus_size_t,
			   bus_size_t, struct pcmcia_io_handle *);
static void at91cf_io_free(pcmcia_chipset_handle_t, struct pcmcia_io_handle *);
static int at91cf_io_map(pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t,
			 struct pcmcia_io_handle *, int *);
static void at91cf_io_unmap(pcmcia_chipset_handle_t, int);
static void *at91cf_intr_establish(pcmcia_chipset_handle_t,
				   struct pcmcia_function *,
				   int, int (*)(void *), void *);
static void at91cf_intr_disestablish(pcmcia_chipset_handle_t, void *);
static void at91cf_socket_enable(pcmcia_chipset_handle_t);
static void at91cf_socket_disable(pcmcia_chipset_handle_t);
static void at91cf_socket_settype(pcmcia_chipset_handle_t, int);

static void at91cf_attach_socket(struct at91cf_handle *);
static void at91cf_config_socket(struct at91cf_handle *);
//static int at91cf_get_voltage(struct at91cf_handle *);

static struct pcmcia_chip_functions at91cf_functions = {
	at91cf_mem_alloc,	at91cf_mem_free,
	at91cf_mem_map,		at91cf_mem_unmap,
	at91cf_io_alloc,	at91cf_io_free,
	at91cf_io_map,		at91cf_io_unmap,
	at91cf_intr_establish,	at91cf_intr_disestablish,
	at91cf_socket_enable,	at91cf_socket_disable,
	at91cf_socket_settype
};

#define	MEMORY_BASE	0
#define	MEMORY_SIZE	0x1000
#define	COMMON_BASE	0x400000
#define	COMMON_SIZE	0x1000
#define	IO_BASE		0x800000
#define	IO_SIZE		0x1000
#define	MIN_SIZE	(IO_BASE + IO_SIZE)

void
at91cf_attach_common(device_t parent, device_t self, void *aux,
		      at91cf_chipset_tag_t cscf)
{
	struct at91cf_softc *sc = device_private(self);
	struct at91bus_attach_args *sa = aux;
	struct at91cf_handle *ph;

	if (sa->sa_size < MIN_SIZE) {
		printf("%s: it's not possible to map registers\n",
		    device_xname(self));
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = sa->sa_iot;
	sc->sc_cscf = cscf;
	sc->sc_enable = 0;

	if (bus_space_map(sa->sa_iot, sa->sa_addr + MEMORY_BASE,
			  MEMORY_SIZE, 0, &sc->sc_memory_ioh)){
		printf("%s: Cannot map memory space\n", device_xname(self));
		return;
	}

	if (bus_space_map(sa->sa_iot, sa->sa_addr + COMMON_BASE,
			  COMMON_SIZE, 0, &sc->sc_common_ioh)){
		printf("%s: Cannot map common memory space\n",
		    device_xname(self));
		bus_space_unmap(sa->sa_iot, sc->sc_memory_ioh, MEMORY_SIZE);
		return;
	}

	if (bus_space_map(sa->sa_iot, sa->sa_addr + IO_BASE,
			  IO_SIZE, 0, &sc->sc_io_ioh)){
		printf("%s: Cannot map I/O space\n", device_xname(self));
		bus_space_unmap(sa->sa_iot, sc->sc_memory_ioh, MEMORY_SIZE);
		bus_space_unmap(sa->sa_iot, sc->sc_common_ioh, COMMON_SIZE);
		return;
	}

	printf("\n");

	/* socket 0 */
	ph = malloc(sizeof(struct at91cf_handle), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (ph == NULL) {
		printf("%s: Cannot allocate memory\n", device_xname(self));
		// @@@@ unmap? @@@@
		return; /* ENOMEM */
	}
	sc->sc_ph = ph;
	ph->ph_sc = sc;
	ph->ph_space[IO].base = sa->sa_addr + IO_BASE;
	ph->ph_space[IO].size = IO_SIZE;
	ph->ph_space[COMMON].base = sa->sa_addr + COMMON_BASE;
	ph->ph_space[COMMON].size = COMMON_SIZE;
	ph->ph_space[ATTRIBUTE].base = sa->sa_addr + MEMORY_BASE;
	ph->ph_space[ATTRIBUTE].size = MEMORY_SIZE;
	at91cf_attach_socket(ph);

	// @@@ reset CF now? @@@@

	at91cf_config_socket(sc->sc_ph);
}

static void
at91cf_attach_socket(struct at91cf_handle *ph)
{
	struct at91cf_softc *sc = ph->ph_sc;
	at91cf_chipset_tag_t cscf = sc->sc_cscf;
	int wait;

	ph->ph_width = 16;
	ph->ph_vcc = 3;
	ph->ph_event_thread = NULL;
	ph->ph_run = 0;
	ph->ph_ih_func = NULL;
	ph->ph_ih_arg = NULL;

	ph->ph_status = (*cscf->card_detect)(sc);

	wait = (cscf->power_ctl)(sc, POWER_OFF);
	delay(wait);
	wait = (cscf->power_ctl)(sc, POWER_ON);
	delay(wait);
}

static void
at91cf_config_socket(struct at91cf_handle *ph)
{
	struct at91cf_softc *sc = ph->ph_sc;
	at91cf_chipset_tag_t cscf = sc->sc_cscf;
	struct pcmciabus_attach_args paa;
	int wait;

	paa.paa_busname = "pcmcia";
	paa.pct = (pcmcia_chipset_tag_t)&at91cf_functions;
	paa.pch = (pcmcia_chipset_handle_t)ph;
	paa.iobase = ph->ph_space[IO].base;
	paa.iosize = ph->ph_space[IO].size;
	ph->ph_card = config_found_ia(sc->sc_dev, "pcmciabus", &paa,
				      at91cf_print);

	(*cscf->intr_establish)(sc, CD_IRQ, IPL_BIO, at91cf_intr_carddetect, ph);
	wait = (*cscf->power_ctl)(sc, POWER_OFF);
	delay(wait);

	kthread_create(PRI_NONE, 0, NULL, at91cf_create_event_thread, ph,
	    &ph->ph_event_thread, "%s", device_xname(sc->sc_dev));
}

static int     
at91cf_print(void *arg, const char *pnp)
{                       
	return (UNCONF);
}       

static void
at91cf_create_event_thread(void *arg)
{
	struct at91cf_handle *ph = arg;
	struct at91cf_softc *sc = ph->ph_sc;
	at91cf_chipset_tag_t cscf = sc->sc_cscf;

	ph->ph_status = (*cscf->card_detect)(sc);

	DPRINTFN(1, ("at91cf_create_event_thread: status=%d\n", ph->ph_status));

	if (ph->ph_status)
		pcmcia_card_attach(ph->ph_card);

	ph->ph_run = 1;
	kthread_create(PRI_NONE, 0, NULL, at91cf_event_thread, ph,
	    &ph->ph_event_thread, "%s", device_xname(sc->sc_dev));
}

static void
at91cf_event_thread(void *arg)
{
	struct at91cf_handle *ph = arg;
	int status;

	for (;;) {
		status = ph->ph_status;
		tsleep(ph, PWAIT, "CSC wait", 0);
		if (!ph->ph_run)
			break;

		DPRINTFN(1, ("at91cf_event_thread: old status=%d, new status=%d\n", status, ph->ph_status));

		if (!status && ph->ph_status)
			pcmcia_card_attach(ph->ph_card);
		else if (status && !ph->ph_status)
			pcmcia_card_detach(ph->ph_card, DETACH_FORCE);
	}

	DPRINTFN(1, ("at91cf_event_thread: run=%d\n",ph->ph_run));
	ph->ph_event_thread = NULL;
	kthread_exit(0);
}

void
at91cf_shutdown(void *arg)
{
	struct at91cf_handle *ph = arg;

	DPRINTFN(1, ("at91cf_shutdown\n"));
	ph->ph_run = 0;
	wakeup(ph);
}

static int
at91cf_intr_carddetect(void *arg)
{
	struct at91cf_handle *ph = arg;
	struct at91cf_softc *sc = ph->ph_sc;
	at91cf_chipset_tag_t cscf = sc->sc_cscf;
	int nstatus;

	nstatus = (*cscf->card_detect)(sc);

	DPRINTFN(1, ("at91cf_intr: nstatus=%#x, ostatus=%#x\n", nstatus, ph->ph_status));

	if (nstatus != ph->ph_status) {
		ph->ph_status = nstatus;
		wakeup(ph);
	}

	return 0;
}

static int
at91cf_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
		 struct pcmcia_mem_handle *pmh)
{
	struct at91cf_handle *ph = (struct at91cf_handle *)pch;
	struct at91cf_softc *sc = ph->ph_sc;

	DPRINTFN(1, ("at91cf_mem_alloc: size=%#x\n",(unsigned)size));

	pmh->memt = sc->sc_iot;
	return 0;
}

static void
at91cf_mem_free(pcmcia_chipset_handle_t pch, struct pcmcia_mem_handle *pmh)
{
	DPRINTFN(1, ("at91cf_mem_free\n"));
}

static int
at91cf_mem_map(pcmcia_chipset_handle_t pch, int kind, bus_addr_t addr,
	       bus_size_t size, struct pcmcia_mem_handle *pmh,
	       bus_size_t *offsetp, int *windowp)
{
	struct at91cf_handle *ph = (struct at91cf_handle *)pch;
	struct at91cf_softc *sc = ph->ph_sc;
	bus_addr_t pa;
	int err;

	DPRINTFN(1, ("at91cf_mem_map: kind=%d, addr=%#x, size=%#x\n",kind,(unsigned)addr,(unsigned)size));

	pa = addr;
	*offsetp = 0;
	size = round_page(size);
	pmh->realsize = size;
	if (kind & PCMCIA_WIDTH_MEM8)
		ph->ph_width = 8;
	else
		ph->ph_width = 16;
	switch (kind & ~PCMCIA_WIDTH_MEM_MASK) {
	case PCMCIA_MEM_ATTR:
		pa += ph->ph_space[ATTRIBUTE].base;
		break;
	case PCMCIA_MEM_COMMON:
		pa += ph->ph_space[COMMON].base;
		break;
	default:
		return -1;
	}

	DPRINTFN(1, ("at91cf_mem_map: pa=%#x, *offsetp=%#x, size=%#x\n",(unsigned)pa,(unsigned)addr,(unsigned)size));

	if (!(err = bus_space_map(sc->sc_iot, pa, size, 0, &pmh->memh)))
		*windowp = (int)pmh->memh;
	return err;
}

static void
at91cf_mem_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct at91cf_handle *ph = (struct at91cf_handle *)pch;
	struct at91cf_softc *sc = ph->ph_sc;

	DPRINTFN(1, ("at91cf_mem_unmap: window=%#x\n",window));

	bus_space_unmap(sc->sc_iot, (bus_addr_t)window, 0x1000);
}

static int
at91cf_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start, bus_size_t size,
		bus_size_t align, struct pcmcia_io_handle *pih)
{
	struct at91cf_handle *ph = (struct at91cf_handle *)pch;
	struct at91cf_softc *sc = ph->ph_sc;
	bus_addr_t pa;

	DPRINTFN(1, ("at91cf_io_alloc: start=%#x, size=%#x, align=%#x\n",(unsigned)start,(unsigned)size,(unsigned)align));

	pih->iot = sc->sc_iot;
	pih->addr = start;
	pih->size = size;
	pa = pih->addr + ph->ph_space[IO].base;
	return bus_space_map(sc->sc_iot, pa, size, 0, &pih->ioh);
}

static void
at91cf_io_free(pcmcia_chipset_handle_t pch, struct pcmcia_io_handle *pih)
{
	struct at91cf_handle *ph = (struct at91cf_handle *)pch;
	struct at91cf_softc *sc = ph->ph_sc;

	DPRINTFN(1, ("at91cf_io_free\n"));

	bus_space_unmap(sc->sc_iot, pih->ioh, pih->size);
}

static int
at91cf_io_map(pcmcia_chipset_handle_t pch, int width, bus_addr_t offset,
	      bus_size_t size, struct pcmcia_io_handle *pih, int *windowp)
{
	struct at91cf_handle *ph = (struct at91cf_handle *)pch;

	DPRINTFN(1, ("at91cf_io_map: offset=%#x, size=%#x, width=%d",(unsigned)offset,(unsigned)size,width));

	switch (width) {
	case PCMCIA_WIDTH_IO8:
		DPRINTFN(1, ("(8bit)\n"));
		ph->ph_width = 8;
		break;
	case PCMCIA_WIDTH_IO16:
	case PCMCIA_WIDTH_AUTO:	/* I don't understand how I check it */
		DPRINTFN(1, ("(16bit)\n"));
		ph->ph_width = 16;
		break;
	default:
		DPRINTFN(1, ("(unknown)\n"));
		return -1;
	}
	*windowp = 0; /* unused */
	return 0;
}

static void
at91cf_io_unmap(pcmcia_chipset_handle_t pch, int window)
{
	DPRINTFN(1, ("at91cf_io_unmap: window=%#x\n",window));
}

static void *
at91cf_intr_establish(pcmcia_chipset_handle_t pch, struct pcmcia_function *pf,
		      int ipl, int (*ih_func)(void *), void *ih_arg)
{
	struct at91cf_handle *ph = (struct at91cf_handle *)pch;
	struct at91cf_softc *sc = ph->ph_sc;
	at91cf_chipset_tag_t cscf = sc->sc_cscf;

	DPRINTFN(1, ("at91cf_intr_establish\n"));

	if (ph->ph_ih_func)
		return 0;

	ph->ph_ih_func = ih_func;
	ph->ph_ih_arg = ih_arg;

	return (*cscf->intr_establish)(sc, CF_IRQ, ipl, at91cf_intr_socket, ph);
//	return (*cscf->intr_establish)(sc, CF_IRQ, ipl, ih_func, ih_arg);
}

static void
at91cf_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	struct at91cf_handle *ph = (struct at91cf_handle *)pch;
	struct at91cf_softc *sc = ph->ph_sc;
	at91cf_chipset_tag_t cscf = sc->sc_cscf;

	DPRINTFN(1, ("at91cf_intr_disestablish\n"));

	ph->ph_ih_func = NULL;
	ph->ph_ih_arg = NULL;

	(*cscf->intr_disestablish)(sc, CF_IRQ, ih);
}

static int
at91cf_intr_socket(void *arg)
{
	struct at91cf_handle *ph = arg;
	struct at91cf_softc *sc = ph->ph_sc;
	at91cf_chipset_tag_t cscf = sc->sc_cscf;
	int err = 0, irq;

	if (ph->ph_ih_func) {
		irq = (*cscf->irq_line)(sc);
		if (ph->ph_type == PCMCIA_IFTYPE_IO)
			irq = !irq;
		if (irq)
			err = (*ph->ph_ih_func)(ph->ph_ih_arg);
		else
			DPRINTFN(2,("%s: other edge ignored\n", __FUNCTION__));
	}

	return err;
}

static void
at91cf_socket_enable(pcmcia_chipset_handle_t pch)
{
	struct at91cf_handle *ph = (struct at91cf_handle *)pch;
	struct at91cf_softc *sc = ph->ph_sc;
	at91cf_chipset_tag_t cscf = sc->sc_cscf;
	int wait;

	DPRINTFN(1, ("at91cf_socket_enable\n"));

	wait = (cscf->power_ctl)(sc, POWER_ON);
	delay(wait);
}

static void
at91cf_socket_disable(pcmcia_chipset_handle_t pch)
{
	struct at91cf_handle *ph = (struct at91cf_handle *)pch;
	struct at91cf_softc *sc = ph->ph_sc;
	at91cf_chipset_tag_t cscf = sc->sc_cscf;
	int wait;

	DPRINTFN(1, ("at91cf_socket_disable\n"));

	wait = (cscf->power_ctl)(sc, POWER_OFF);
	delay(wait);
}

static void
at91cf_socket_settype(pcmcia_chipset_handle_t pch, int type)
{
	struct at91cf_handle *ph = (struct at91cf_handle *)pch;

	DPRINTFN(1, ("at91cf_socket_settype: type=%d",type));

	ph->ph_type = type;

	switch (type) {
	case PCMCIA_IFTYPE_MEMORY:
		DPRINTFN(1, ("(Memory)\n"));
		break;
	case PCMCIA_IFTYPE_IO:
		DPRINTFN(1, ("(I/O)\n"));
		break;
	default:
		DPRINTFN(1, ("(unknown)\n"));
		return;
	}
}

#if 0
static int
at91cf_get_voltage(struct at91cf_handle *ph)
{
	struct at91cf_softc *sc = ph->ph_sc;
	at91cf_chipset_tag_t cscf = sc->sc_cscf;
	int cap, vcc = 0;

	cap = (cscf->power_capability)(sc);
	if (eppio_read(sc->sc_pio, ph->ph_port, ph->ph_vs[0])) {
		if (cap | VCC_5V)
			vcc = 5;
		else
			printf("%s: unsupported Vcc 5 Volts",
			       device_xname(sc->sc_dev));
	} else {
		if (cap | VCC_3V)
			vcc = 3;
		else
			printf("%s: unsupported Vcc 3.3 Volts",
			       device_xname(sc->sc_dev));
	}
	DPRINTFN(1, ("at91cf_get_voltage: vs1=%d, vs2=%d (%dV)\n",eppio_read_bit(sc->sc_pio, ph->ph_port, ph->ph_vs[0]),eppio_read_bit(sc->sc_pio, ph->ph_port, ph->ph_vs[1]),vcc));
	return vcc;
}

#endif
