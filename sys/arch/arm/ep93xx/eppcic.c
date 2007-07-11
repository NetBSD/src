/*	$NetBSD: eppcic.c,v 1.1.38.1 2007/07/11 19:58:08 mjf Exp $	*/

/*
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
__KERNEL_RCSID(0, "$NetBSD: eppcic.c,v 1.1.38.1 2007/07/11 19:58:08 mjf Exp $");

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
#include <arm/ep93xx/epsocvar.h> 
#include <arm/ep93xx/epgpiovar.h> 
#include <arm/ep93xx/eppcicvar.h> 
#include <arm/ep93xx/ep93xxreg.h>
#include <arm/ep93xx/epsmcreg.h>
#include "epled.h"
#if NEPLED > 0
#include <arm/ep93xx/epledvar.h> 
#endif

#include "epgpio.h"
#if NEPGPIO == 0
#error "epgpio requires in eppcic"
#endif

#ifdef EPPCIC_DEBUG
int eppcic_debug = EPPCIC_DEBUG;
#define DPRINTFN(n,x)	if (eppcic_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)
#endif

/* Mem & I/O */
#define	SOCKET0_MCCD1	1	/* pin36/pin26 (negative) Card Detect 1 */
#define	SOCKET0_MCCD2	2	/* pin67/pin25 (negative) Card Detect 2 */
#define	SOCKET0_VS1	5	/* pin33/pin43 (negative) Voltage Sense 1 */
#define	SOCKET0_VS2	7	/* pin57/pin40 (negative) Voltage Sense 2 */
/* Memory */
#define	SOCKET0_WP	0	/* pin33/pin24 Write Protect */
#define	SOCKET0_MCBVD1	3	/* pin63/pin46 Battery Voltage Detect 1 */
#define	SOCKET0_MCBVD2	4	/* pin62/pin45 Battery Voltage Detect 2 */
#define	SOCKET0_READY	6	/* pin16/pin37 Ready */
/* I/O */
#define	SOCKET0_STSCHG	3	/* pin63/pin46 (negative) Status Change */
#define	SOCKET0_SPKR	4	/* pin62/pin45 (negative) Speaker */
#define	SOCKET0_IREQ	6	/* pin16/pin37 Interrupt Request */

struct eppcic_handle {
	int			ph_socket;	/* socket number */
	struct eppcic_softc	*ph_sc;
	struct device		*ph_card;
	int			(*ph_ih_func)(void *);
	void			*ph_ih_arg;
	lwp_t			*ph_event_thread;
	int			ph_run;		/* ktread running */
	int			ph_width;	/* 8 or 16 */
	int			ph_vcc;		/* 3 or 5 */
	int			ph_status[2];	/* cd1 and cd2 */
	int			ph_port;	/* GPIO port */
	int			ph_cd[2];	/* card detect */
	int			ph_vs[2];	/* voltage sense */
	int			ph_ireq;	/* interrupt request */
	struct {
		bus_size_t	reg;
		bus_addr_t	base;
		bus_size_t	size;
	} ph_space[3];
#define	IO		0
#define	COMMON		1
#define	ATTRIBUTE	2
};

static int eppcic_intr_carddetect(void *);
static int eppcic_intr_socket(void *);
static int eppcic_print(void *, const char *);
static void eppcic_event_thread(void *);
void eppcic_shutdown(void *);

static int eppcic_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
			    struct pcmcia_mem_handle *);
static void eppcic_mem_free(pcmcia_chipset_handle_t,
			    struct pcmcia_mem_handle *);
static int eppcic_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t,
			  struct pcmcia_mem_handle *, bus_size_t *, int *);
static void eppcic_mem_unmap(pcmcia_chipset_handle_t, int);
static int eppcic_io_alloc(pcmcia_chipset_handle_t, bus_addr_t, bus_size_t,
			   bus_size_t, struct pcmcia_io_handle *);
static void eppcic_io_free(pcmcia_chipset_handle_t, struct pcmcia_io_handle *);
static int eppcic_io_map(pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t,
			 struct pcmcia_io_handle *, int *);
static void eppcic_io_unmap(pcmcia_chipset_handle_t, int);
static void *eppcic_intr_establish(pcmcia_chipset_handle_t,
				   struct pcmcia_function *,
				   int, int (*)(void *), void *);
static void eppcic_intr_disestablish(pcmcia_chipset_handle_t, void *);
static void eppcic_socket_enable(pcmcia_chipset_handle_t);
static void eppcic_socket_disable(pcmcia_chipset_handle_t);
static void eppcic_socket_settype(pcmcia_chipset_handle_t, int);

static void eppcic_attach_socket(struct eppcic_handle *);
static void eppcic_config_socket(struct eppcic_handle *);
static int eppcic_get_voltage(struct eppcic_handle *);
static void eppcic_set_pcreg(struct eppcic_handle *, int);

static struct pcmcia_chip_functions eppcic_functions = {
	eppcic_mem_alloc,	eppcic_mem_free,
	eppcic_mem_map,		eppcic_mem_unmap,
	eppcic_io_alloc,	eppcic_io_free,
	eppcic_io_map,		eppcic_io_unmap,
	eppcic_intr_establish,	eppcic_intr_disestablish,
	eppcic_socket_enable,	eppcic_socket_disable,
	eppcic_socket_settype
};

void
eppcic_attach_common(struct device *parent, struct device *self, void *aux,
		     eppcic_chipset_tag_t pcic)
{
	struct eppcic_softc *sc = (struct eppcic_softc *)self;
	struct epsoc_attach_args *sa = aux;
	struct eppcic_handle *ph;
	int reg;
	int i;

	if (!sa->sa_gpio) {
		printf("%s: epgpio requires\n", self->dv_xname);
		return;
	}
	sc->sc_gpio = sa->sa_gpio;
	sc->sc_iot = sa->sa_iot;
	sc->sc_hclk = sa->sa_hclk;
	sc->sc_pcic = pcic;
	sc->sc_enable = 0;
	if (bus_space_map(sa->sa_iot, sa->sa_addr,
			  sa->sa_size, 0, &sc->sc_ioh)){
		printf("%s: Cannot map registers\n", self->dv_xname);
		return;
	}
	printf("\n");

#if NEPLED > 0
	epled_green_on();
	epled_red_off();
#endif
	/* socket 0 */
	if (!(ph = malloc(sizeof(struct eppcic_handle), M_DEVBUF, M_NOWAIT))) {
		printf("%s: Cannot allocate memory\n", self->dv_xname);
		return; /* ENOMEM */
	}
	sc->sc_ph[0] = ph;
	ph->ph_sc = sc;
	ph->ph_socket = 0;
	ph->ph_port = PORT_F;
	ph->ph_cd[0] = SOCKET0_MCCD1;
	ph->ph_cd[1] = SOCKET0_MCCD2;
	ph->ph_vs[0] = SOCKET0_VS1;
	ph->ph_vs[1] = SOCKET0_VS2;
	ph->ph_ireq = SOCKET0_IREQ;
	ph->ph_space[IO].reg = EP93XX_PCMCIA0_IO;
	ph->ph_space[IO].base = EP93XX_PCMCIA0_HWBASE + EP93XX_PCMCIA_IO;
	ph->ph_space[IO].size = EP93XX_PCMCIA_IO_SIZE;
	ph->ph_space[COMMON].reg = EP93XX_PCMCIA0_Common;
	ph->ph_space[COMMON].base = EP93XX_PCMCIA0_HWBASE
				    + EP93XX_PCMCIA_COMMON;
	ph->ph_space[COMMON].size = EP93XX_PCMCIA_COMMON_SIZE;
	ph->ph_space[ATTRIBUTE].reg = EP93XX_PCMCIA0_Attribute;
	ph->ph_space[ATTRIBUTE].base = EP93XX_PCMCIA0_HWBASE
				       + EP93XX_PCMCIA_ATTRIBUTE;
	ph->ph_space[ATTRIBUTE].size = EP93XX_PCMCIA_ATTRIBUTE_SIZE;
	eppcic_attach_socket(ph);

	reg = EP93XX_PCMCIA_WEN | (pcic->socket_type)(sc, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EP93XX_PCMCIA_Ctrl,
			  EP93XX_PCMCIA_RST | reg);
	delay(10);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EP93XX_PCMCIA_Ctrl, reg);
	delay(500);

	for (i = 0; i < EP93XX_PCMCIA_NSOCKET; i++)
		eppcic_config_socket(sc->sc_ph[i]);
#if NEPLED > 0
	epled_green_off();
#endif
}

static void
eppcic_attach_socket(struct eppcic_handle *ph)
{
	struct eppcic_softc *sc = ph->ph_sc;
	eppcic_chipset_tag_t pcic = sc->sc_pcic;
	int wait;

	ph->ph_width = 16;
	ph->ph_vcc = 3;
	ph->ph_event_thread = NULL;
	ph->ph_run = 0;
	ph->ph_ih_func = NULL;
	ph->ph_ih_arg = NULL;
	epgpio_in(sc->sc_gpio, ph->ph_port, ph->ph_cd[0]);
	epgpio_in(sc->sc_gpio, ph->ph_port, ph->ph_cd[1]);
	epgpio_in(sc->sc_gpio, ph->ph_port, ph->ph_vs[0]);
	epgpio_in(sc->sc_gpio, ph->ph_port, ph->ph_vs[1]);
	ph->ph_status[0] = epgpio_read(sc->sc_gpio, ph->ph_port, ph->ph_cd[0]);
	ph->ph_status[1] = epgpio_read(sc->sc_gpio, ph->ph_port, ph->ph_cd[1]);
	wait = (pcic->power_ctl)(sc, ph->ph_socket, POWER_OFF);
	delay(wait);
	eppcic_set_pcreg(ph, ph->ph_space[IO].reg);
	eppcic_set_pcreg(ph, ph->ph_space[COMMON].reg);
	eppcic_set_pcreg(ph, ph->ph_space[ATTRIBUTE].reg);
	wait = (pcic->power_ctl)(sc, ph->ph_socket, POWER_ON);
	delay(wait);
}

static void
eppcic_config_socket(struct eppcic_handle *ph)
{
	struct eppcic_softc *sc = ph->ph_sc;
	eppcic_chipset_tag_t pcic = sc->sc_pcic;
	struct pcmciabus_attach_args paa;
	int wait;

	paa.paa_busname = "pcmcia";
	paa.pct = (pcmcia_chipset_tag_t)&eppcic_functions;
	paa.pch = (pcmcia_chipset_handle_t)ph;
	paa.iobase = ph->ph_space[IO].base;
	paa.iosize = ph->ph_space[IO].size;
	ph->ph_card = config_found_ia((void*)sc, "pcmciabus", &paa,
				      eppcic_print);
	
	epgpio_intr_establish(sc->sc_gpio, ph->ph_port, ph->ph_cd[0],
			      EDGE_TRIGGER | FALLING_EDGE | DEBOUNCE,
			      IPL_TTY, eppcic_intr_carddetect, ph);
	epgpio_intr_establish(sc->sc_gpio, ph->ph_port, ph->ph_cd[1],
			      EDGE_TRIGGER | RISING_EDGE | DEBOUNCE,
			      IPL_TTY, eppcic_intr_carddetect, ph);
	wait = (pcic->power_ctl)(sc, ph->ph_socket, POWER_OFF);
	delay(wait);


	ph->ph_status[0] = epgpio_read(sc->sc_gpio, ph->ph_port, ph->ph_cd[0]);
	ph->ph_status[1] = epgpio_read(sc->sc_gpio, ph->ph_port, ph->ph_cd[1]);

	DPRINTFN(1, ("eppcic_config_socket: cd1=%d, cd2=%d\n",ph->ph_status[0],ph->ph_status[1]));

	if (!(ph->ph_status[0] | ph->ph_status[1]))
		pcmcia_card_attach(ph->ph_card);

	ph->ph_run = 1;
	kthread_create(PRI_NONE, 0, NULL, eppcic_event_thread, ph,
	    &ph->ph_event_thread, "%s,%d", sc->sc_dev.dv_xname,
	    ph->ph_socket);
}

static int     
eppcic_print(void *arg, const char *pnp)
{                       
	return (UNCONF);
}       

static void
eppcic_event_thread(void *arg)
{
	struct eppcic_handle *ph = arg;

	for (;;) {
		tsleep(ph, PWAIT, "CSC wait", 0);
		if (!ph->ph_run)
			break;

		DPRINTFN(1, ("eppcic_event_thread: cd1=%d, cd2=%d\n",ph->ph_status[0],ph->ph_status[1]));

		if (!ph->ph_status[0] && !ph->ph_status[1])
			pcmcia_card_attach(ph->ph_card);
		else if (ph->ph_status[0] && ph->ph_status[1])
			pcmcia_card_detach(ph->ph_card, DETACH_FORCE);
	}

	DPRINTFN(1, ("eppcic_event_thread: run=%d\n",ph->ph_run));
	ph->ph_event_thread = NULL;
	kthread_exit(0);
}

void
eppcic_shutdown(void *arg)
{
	struct eppcic_handle *ph = arg;

	DPRINTFN(1, ("eppcic_shutdown\n"));
	ph->ph_run = 0;
	wakeup(ph);
}

static int
eppcic_intr_carddetect(void *arg)
{
	struct eppcic_handle *ph = arg;
	struct eppcic_softc *sc = ph->ph_sc;
	int nstatus[2];

	nstatus[0] = epgpio_read(sc->sc_gpio, ph->ph_port, ph->ph_cd[0]);
	nstatus[1] = epgpio_read(sc->sc_gpio, ph->ph_port, ph->ph_cd[1]);

	DPRINTFN(1, ("eppcic_intr: cd1=%#x, cd2=%#x\n",nstatus[0],nstatus[1]));

	if (nstatus[0] != ph->ph_status[0] || nstatus[1] != ph->ph_status[1])
		wakeup(ph);
	ph->ph_status[0] = nstatus[0];
	ph->ph_status[1] = nstatus[1];
	return 0;
}

static int
eppcic_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
		 struct pcmcia_mem_handle *pmh)
{
	struct eppcic_handle *ph = (struct eppcic_handle *)pch;
	struct eppcic_softc *sc = ph->ph_sc;

	DPRINTFN(1, ("eppcic_mem_alloc: size=%#x\n",(unsigned)size));

	pmh->memt = sc->sc_iot;
	return 0;
}

static void
eppcic_mem_free(pcmcia_chipset_handle_t pch, struct pcmcia_mem_handle *pmh)
{
	DPRINTFN(1, ("eppcic_mem_free\n"));
}

static int
eppcic_mem_map(pcmcia_chipset_handle_t pch, int kind, bus_addr_t addr,
	       bus_size_t size, struct pcmcia_mem_handle *pmh,
	       bus_size_t *offsetp, int *windowp)
{
	struct eppcic_handle *ph = (struct eppcic_handle *)pch;
	struct eppcic_softc *sc = ph->ph_sc;
	bus_addr_t pa;
	int err;

	DPRINTFN(1, ("eppcic_mem_map: kind=%d, addr=%#x, size=%#x\n",kind,(unsigned)addr,(unsigned)size));

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
		eppcic_set_pcreg(ph, ph->ph_space[ATTRIBUTE].reg);
		pa += ph->ph_space[ATTRIBUTE].base;
		break;
	case PCMCIA_MEM_COMMON:
		eppcic_set_pcreg(ph, ph->ph_space[COMMON].reg);
		pa += ph->ph_space[COMMON].base;
		break;
	default:
		return -1;
	}

	DPRINTFN(1, ("eppcic_mem_map: pa=%#x, *offsetp=%#x, size=%#x\n",(unsigned)pa,(unsigned)addr,(unsigned)size));

	if (!(err = bus_space_map(sc->sc_iot, pa, size, 0, &pmh->memh)))
		*windowp = (int)pmh->memh;
	return err;
}

static void
eppcic_mem_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct eppcic_handle *ph = (struct eppcic_handle *)pch;
	struct eppcic_softc *sc = ph->ph_sc;

	DPRINTFN(1, ("eppcic_mem_unmap: window=%#x\n",window));

	bus_space_unmap(sc->sc_iot, (bus_addr_t)window, 0x400);
}

static int
eppcic_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start, bus_size_t size,
		bus_size_t align, struct pcmcia_io_handle *pih)
{
	struct eppcic_handle *ph = (struct eppcic_handle *)pch;
	struct eppcic_softc *sc = ph->ph_sc;
	bus_addr_t pa;

	DPRINTFN(1, ("eppcic_io_alloc: start=%#x, size=%#x, align=%#x\n",(unsigned)start,(unsigned)size,(unsigned)align));

	pih->iot = sc->sc_iot;
	pih->addr = start;
	pih->size = size;
	pa = pih->addr + ph->ph_space[IO].base;
	return bus_space_map(sc->sc_iot, pa, size, 0, &pih->ioh);
}

static void
eppcic_io_free(pcmcia_chipset_handle_t pch, struct pcmcia_io_handle *pih)
{
	struct eppcic_handle *ph = (struct eppcic_handle *)pch;
	struct eppcic_softc *sc = ph->ph_sc;

	DPRINTFN(1, ("eppcic_io_free\n"));

	bus_space_unmap(sc->sc_iot, pih->ioh, pih->size);
}

static int
eppcic_io_map(pcmcia_chipset_handle_t pch, int width, bus_addr_t offset,
	      bus_size_t size, struct pcmcia_io_handle *pih, int *windowp)
{
	struct eppcic_handle *ph = (struct eppcic_handle *)pch;

	DPRINTFN(1, ("eppcic_io_map: offset=%#x, size=%#x, width=%d",(unsigned)offset,(unsigned)size,width));

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
	eppcic_set_pcreg(ph, ph->ph_space[IO].reg);
	*windowp = 0; /* unused */
	return 0;
}

static void
eppcic_io_unmap(pcmcia_chipset_handle_t pch, int window)
{
	DPRINTFN(1, ("eppcic_io_unmap: window=%#x\n",window));
}

static void *
eppcic_intr_establish(pcmcia_chipset_handle_t pch, struct pcmcia_function *pf,
		      int ipl, int (*ih_func)(void *), void *ih_arg)
{
	struct eppcic_handle *ph = (struct eppcic_handle *)pch;
	struct eppcic_softc *sc = ph->ph_sc;

	DPRINTFN(1, ("eppcic_intr_establish\n"));

	if (ph->ph_ih_func)
		return 0;

	ph->ph_ih_func = ih_func;
	ph->ph_ih_arg = ih_arg;
	return epgpio_intr_establish(sc->sc_gpio, ph->ph_port, ph->ph_ireq,
				     LEVEL_SENSE | LOW_LEVEL,
				     ipl, eppcic_intr_socket, ph);
}

static void
eppcic_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	struct eppcic_handle *ph = (struct eppcic_handle *)pch;
	struct eppcic_softc *sc = ph->ph_sc;

	DPRINTFN(1, ("eppcic_intr_disestablish\n"));

	ph->ph_ih_func = NULL;
	ph->ph_ih_arg = NULL;
	epgpio_intr_disestablish(sc->sc_gpio, ph->ph_port, ph->ph_ireq);
}

static int
eppcic_intr_socket(void *arg)
{
	struct eppcic_handle *ph = arg;
	int err = 0;

	if (ph->ph_ih_func) {
#if NEPLED > 0
		epled_red_on();
#endif
		err = (*ph->ph_ih_func)(ph->ph_ih_arg);
#if NEPLED > 0
		epled_red_off();
#endif
	}
	return err;
}


static void
eppcic_socket_enable(pcmcia_chipset_handle_t pch)
{
	struct eppcic_handle *ph = (struct eppcic_handle *)pch;
	struct eppcic_softc *sc = ph->ph_sc;
	eppcic_chipset_tag_t pcic = sc->sc_pcic;
	int wait;

	DPRINTFN(1, ("eppcic_socket_enable\n"));

	wait = (pcic->power_ctl)(sc, ph->ph_socket, POWER_ON);
	delay(wait);
#if NEPLED > 0
	if (!sc->sc_enable++)
		epled_green_on();
#endif
	ph->ph_vcc = eppcic_get_voltage(ph);
}

static void
eppcic_socket_disable(pcmcia_chipset_handle_t pch)
{
	struct eppcic_handle *ph = (struct eppcic_handle *)pch;
	struct eppcic_softc *sc = ph->ph_sc;
	eppcic_chipset_tag_t pcic = sc->sc_pcic;
	int wait;

	DPRINTFN(1, ("eppcic_socket_disable\n"));

	wait = (pcic->power_ctl)(sc, ph->ph_socket, POWER_OFF);
	delay(wait);
#if NEPLED > 0
	if (!--sc->sc_enable)
		epled_green_off();
#endif
}

static void
eppcic_socket_settype(pcmcia_chipset_handle_t pch, int type)
{
	DPRINTFN(1, ("eppcic_socket_settype: type=%d",type));

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

static int
eppcic_get_voltage(struct eppcic_handle *ph)
{
	struct eppcic_softc *sc = ph->ph_sc;
	eppcic_chipset_tag_t pcic = sc->sc_pcic;
	int cap, vcc = 0;

	cap = (pcic->power_capability)(sc, ph->ph_socket);
	if (epgpio_read(sc->sc_gpio, ph->ph_port, ph->ph_vs[0])) {
		if (cap | VCC_5V)
			vcc = 5;
		else
			printf("%s: unsupported Vcc 5 Volts",
			       sc->sc_dev.dv_xname);
	} else {
		if (cap | VCC_3V)
			vcc = 3;
		else
			printf("%s: unsupported Vcc 3.3 Volts",
			       sc->sc_dev.dv_xname);
	}
	DPRINTFN(1, ("eppcic_get_voltage: vs1=%d, vs2=%d (%dV)\n",epgpio_read_bit(sc->sc_gpio, ph->ph_port, ph->ph_vs[0]),epgpio_read_bit(sc->sc_gpio, ph->ph_port, ph->ph_vs[1]),vcc));
	return vcc;
}

#define	EXTRA_DELAY	40

static void
eppcic_set_pcreg(struct eppcic_handle *ph, int kind)
{
	struct eppcic_softc *sc = ph->ph_sc;
	int atiming, htiming, ptiming;
	int period = 1000000000 / sc->sc_hclk;
	int width;

	switch (ph->ph_width) {
	case 8:
		width = 0;
		break;
	case 16:
		width = EP93XX_PCMCIA_WIDTH_16;
		break;
	default:
		return;
	}
	switch (kind) {
	case IO:
		atiming = 165; htiming = 20; ptiming = 70;
		break;
	case COMMON:
#if linux_timing!=hamajima20050816
		switch (ph->ph_vcc) {
		case 3:
			atiming = 465; htiming = 35; ptiming = 100;
			break;
		case 5:
			atiming = 200; htiming = 20; ptiming = 30;
			break;
		default:
			return;
		}
		break;
#endif
	case ATTRIBUTE:
		switch (ph->ph_vcc) {
		case 3:
#if linux_timing!=hamajima20050816
			atiming = 465; htiming = 35; ptiming = 100;
#else
			atiming = 600; htiming = 35; ptiming = 100;
#endif
			break;
		case 5:
#if linux_timing!=hamajima20050816
			atiming = 250; htiming = 20; ptiming = 30;
#else
			atiming = 300; htiming = 20; ptiming = 30;
#endif
			break;
		default:
			return;
		}
		break;
	default:
		return;
	}

#if linux_timing!=hamajima20050816
	period = 1000000000 / 50000000;
	width = EP93XX_PCMCIA_WIDTH_16;
#endif

	atiming = (atiming + EXTRA_DELAY) / period;
	if (atiming>0xff)
		atiming = 0xff;
	htiming = ((htiming + EXTRA_DELAY) / period) + 1;
	if (htiming>0xf)
		htiming = 0xf;
	ptiming = (ptiming + EXTRA_DELAY) / period;
	if (ptiming>0xff)
		ptiming = 0xff;

	DPRINTFN(1, ("eppcic_set_pcreg: width=%d, access=%d, hold=%d, pre-charge=%d\n",ph->ph_width,atiming,htiming,ptiming));

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ph->ph_space[kind].reg,
			  width
			  | (atiming<<EP93XX_PCMCIA_ACCESS_SHIFT)
			  | (htiming<<EP93XX_PCMCIA_HOLD_SHIFT)
			  | (ptiming<<EP93XX_PCMCIA_PRECHARGE_SHIFT));
}
