/*	$NetBSD: it8368.c,v 1.5 2000/01/03 18:29:03 uch Exp $ */

/*
 * Copyright (c) 1999, 2000, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 */
#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txcsbusvar.h>
#include <hpcmips/dev/it8368reg.h>

#ifdef IT8368DEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

int	it8368e_match __P((struct device*, struct cfdata*, void*));
void	it8368e_attach __P((struct device*, struct device*, void*));
int	it8368_print __P((void*, const char*));
int	it8368_submatch __P((struct device*, struct cfdata*, void*));

#define IT8368_LASTSTATE_PRESENT	0x0002
#define IT8368_LASTSTATE_HALF		0x0001
#define IT8368_LASTSTATE_EMPTY	0x0000

struct it8368e_softc {
	struct device	sc_dev;
	struct device	*sc_pcmcia;
	tx_chipset_tag_t sc_tc;

	/* Register space */
	bus_space_tag_t		sc_csregt;
	bus_space_handle_t	sc_csregh;
	/* I/O, attribute space */
	bus_space_tag_t		sc_csiot;
	bus_space_handle_t	sc_csioh;
	bus_addr_t		sc_csiobase;
	bus_size_t		sc_csiosize;
	/*
	 *  XXX theses means attribute memory. not memory space. 
	 *	memory space is 0x64000000. 
	 */
	bus_space_tag_t		sc_csmemt;
	bus_space_handle_t	sc_csmemh;
	bus_addr_t		sc_csmembase;
	bus_size_t		sc_csmemsize;

	/* Separate I/O and attribute space mode */
	int sc_fixattr;

	/* Card interrupt handler */
	int	(*sc_card_fun) __P((void*));
	void	*sc_card_arg;
	void	*sc_card_ih;
	int	sc_card_irq;

	/* Card status change */
	int	sc_irq;
	void	*sc_ih;
	int	sc_laststate;
};

void	it8368_init_socket __P((struct it8368e_softc*));
void	it8368_attach_socket __P((struct it8368e_softc*));
void	it8368_access __P((struct it8368e_softc*, int, int));
int	it8368_intr __P((void*));

void	it8368_dump __P((struct it8368e_softc*));

int	it8368_chip_mem_alloc __P((pcmcia_chipset_handle_t, bus_size_t, 
				   struct pcmcia_mem_handle*));
void	it8368_chip_mem_free __P((pcmcia_chipset_handle_t, 
				  struct pcmcia_mem_handle*));
int	it8368_chip_mem_map __P((pcmcia_chipset_handle_t, int, bus_addr_t, 
				 bus_size_t, struct pcmcia_mem_handle*, 
				 bus_addr_t*, int*));
void	it8368_chip_mem_unmap __P((pcmcia_chipset_handle_t, int));
int	it8368_chip_io_alloc __P((pcmcia_chipset_handle_t, bus_addr_t, 
				  bus_size_t, bus_size_t, 
				  struct pcmcia_io_handle*));
void	it8368_chip_io_free __P((pcmcia_chipset_handle_t, 
				 struct pcmcia_io_handle*));
int	it8368_chip_io_map __P((pcmcia_chipset_handle_t, int, bus_addr_t, 
				bus_size_t, struct pcmcia_io_handle*, 
				int*));
void	it8368_chip_io_unmap __P((pcmcia_chipset_handle_t, int));
void	it8368_chip_socket_enable __P((pcmcia_chipset_handle_t));
void	it8368_chip_socket_disable __P((pcmcia_chipset_handle_t));
void	*it8368_chip_intr_establish __P((pcmcia_chipset_handle_t, 
					 struct pcmcia_function*, int, 
					 int (*) (void*), void*));
void	it8368_chip_intr_disestablish __P((pcmcia_chipset_handle_t, void*));

static struct pcmcia_chip_functions it8368_functions = {
	it8368_chip_mem_alloc,
	it8368_chip_mem_free,
	it8368_chip_mem_map,
	it8368_chip_mem_unmap,
	it8368_chip_io_alloc,
	it8368_chip_io_free,
	it8368_chip_io_map,
	it8368_chip_io_unmap,
	it8368_chip_intr_establish,
	it8368_chip_intr_disestablish,
	it8368_chip_socket_enable,
	it8368_chip_socket_disable
};

struct cfattach it8368e_ca = {
	sizeof(struct it8368e_softc), it8368e_match, it8368e_attach
};

/*
 *	IT8368 configuration register is big-endian.
 */
__inline u_int16_t	it8368_reg_read __P((bus_space_tag_t, 
					     bus_space_handle_t, int));
__inline void		it8368_reg_write __P((bus_space_tag_t, 
					      bus_space_handle_t, int, 
					      u_int16_t));

#define PRINTGPIO(m) __bitdisp(it8368_reg_read(csregt, csregh, \
	IT8368_GPIO##m##_REG), 0, IT8368_GPIO_MAX, #m, 1)
#define PRINTMFIO(m) __bitdisp(it8368_reg_read(csregt, csregh, \
	IT8368_MFIO##m##_REG), 0, IT8368_MFIO_MAX, #m, 1)

int
it8368e_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
it8368e_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct cs_attach_args *ca = aux;
	struct it8368e_softc *sc = (void*)self;
	tx_chipset_tag_t tc;
	bus_space_tag_t csregt;
	bus_space_handle_t csregh;
	u_int16_t reg;

	sc->sc_tc = tc = ca->ca_tc;
	sc->sc_csregt = csregt = ca->ca_csreg.cstag;
	
	bus_space_map(csregt, ca->ca_csreg.csbase, ca->ca_csreg.cssize,
		      0, &sc->sc_csregh);
	csregh = sc->sc_csregh;
	sc->sc_csiot = ca->ca_csio.cstag;
	sc->sc_csiobase = ca->ca_csio.csbase;
	sc->sc_csiosize = ca->ca_csio.cssize;

#ifdef IT8368DEBUG
	printf("\n\t[Windows CE setting]\n");
	it8368_dump(sc); /* print WindowsCE setting */
#endif
	/* LHA[14:13] <= HA[14:13]	*/
	reg = it8368_reg_read(csregt, csregh, IT8368_CTRL_REG);
	reg &= ~IT8368_CTRL_ADDRSEL;
	it8368_reg_write(csregt, csregh, IT8368_CTRL_REG, reg);

	/* Set all MFIO direction as LHA[23:13] output pins */
	reg = it8368_reg_read(csregt, csregh, IT8368_MFIODIR_REG);
	reg |= IT8368_MFIODIR_MASK;
	it8368_reg_write(csregt, csregh, IT8368_MFIODIR_REG, reg);

	/* Set all MFIO functions as LHA */
	reg = it8368_reg_read(csregt, csregh, IT8368_MFIOSEL_REG);
	reg &= ~IT8368_MFIOSEL_MASK;
	it8368_reg_write(csregt, csregh, IT8368_MFIOSEL_REG, reg);

	/* Disable MFIO interrupt */
	reg = it8368_reg_read(csregt, csregh, IT8368_MFIOPOSINTEN_REG);
	reg &= ~IT8368_MFIOPOSINTEN_MASK;
	it8368_reg_write(csregt, csregh, IT8368_MFIOPOSINTEN_REG, reg);
	reg = it8368_reg_read(csregt, csregh, IT8368_MFIONEGINTEN_REG);
	reg &= ~IT8368_MFIONEGINTEN_MASK;
	it8368_reg_write(csregt, csregh, IT8368_MFIONEGINTEN_REG, reg);

	/* Port direction */
	reg = IT8368_PIN_CRDVCCON1 | IT8368_PIN_CRDVCCON0 |
		IT8368_PIN_CRDVPPON1 | IT8368_PIN_CRDVPPON0 |
		IT8368_PIN_BCRDRST;
	it8368_reg_write(csregt, csregh, IT8368_GPIODIR_REG, reg);

	printf("\n");

	/* 
	 *	Separate I/O and attribute memory region 
	 */
	reg = it8368_reg_read(csregt, csregh, IT8368_CTRL_REG);
	reg |= IT8368_CTRL_FIXATTRIO;
	it8368_reg_write(csregt, csregh, IT8368_CTRL_REG, reg);
	
	if (IT8368_CTRL_FIXATTRIO & it8368_reg_read(csregt, csregh, 
						    IT8368_CTRL_REG)) {
		sc->sc_fixattr = 1;
		printf("%s: fix attr mode\n", sc->sc_dev.dv_xname);
		sc->sc_csmemt = sc->sc_csiot;
		sc->sc_csiosize /= 2;
		sc->sc_csmemsize = sc->sc_csiosize;
		sc->sc_csmembase = sc->sc_csiosize;
	} else {
		printf("%s: legacy attr mode", sc->sc_dev.dv_xname);
		sc->sc_fixattr = 0;
		sc->sc_csmemt = sc->sc_csiot;
		sc->sc_csmemh = sc->sc_csmemh;
		sc->sc_csmembase = sc->sc_csiobase;
		sc->sc_csmemsize = sc->sc_csiosize;
	}
	it8368_dump(sc);

	/* Enable card and interrupt driving. */
	reg = it8368_reg_read(csregt, csregh, IT8368_CTRL_REG);
	reg |= (IT8368_CTRL_GLOBALEN | IT8368_CTRL_CARDEN);
	if (sc->sc_fixattr)
		reg |= IT8368_CTRL_FIXATTRIO;
	it8368_reg_write(csregt, csregh, IT8368_CTRL_REG, reg);

	sc->sc_irq = ca->ca_irq1;
	sc->sc_card_irq = ca->ca_irq3;



	it8368_attach_socket(sc);
}

__inline u_int16_t
it8368_reg_read(t, h, ofs)
	bus_space_tag_t t;
	bus_space_handle_t h;
	int ofs;
{
	u_int16_t val;

	val = bus_space_read_2(t, h, ofs);
	return 0xffff & (((val >> 8) & 0xff)|((val << 8) & 0xff00));
}

__inline void
it8368_reg_write(t, h, ofs, v)
	bus_space_tag_t t;
	bus_space_handle_t h;
	int ofs;
	u_int16_t v;
{
	u_int16_t val;

	val = 0xffff & (((v >> 8) & 0xff)|((v << 8) & 0xff00));
	bus_space_write_2(t, h, ofs, val);
}

int
it8368_intr(arg)
 	void *arg;
{
	struct it8368e_softc *sc = arg;
	bus_space_tag_t csregt = sc->sc_csregt;
	bus_space_handle_t csregh = sc->sc_csregh;
	u_int16_t reg;

	reg = it8368_reg_read(csregt, csregh, IT8368_GPIONEGINTSTAT_REG);

	if (reg & IT8368_PIN_BCRDRDY) {
		if (sc->sc_card_fun) {
			/* clear interrupt */
			it8368_reg_write(csregt, csregh, 
					 IT8368_GPIONEGINTSTAT_REG,
					 IT8368_PIN_BCRDRDY);
			
			/* Dispatch card interrupt handler */
			(*sc->sc_card_fun)(sc->sc_card_arg);
		}
	} else if (reg & IT8368_PIN_CRDDET2) {
		it8368_reg_write(csregt, csregh, IT8368_GPIONEGINTSTAT_REG,
				 IT8368_PIN_CRDDET2);
		printf("[CSC]\n");
		it8368_dump(sc);
		it8368_chip_socket_disable(sc);
	} else {
		printf("unknown it8368 interrupt\n");
		it8368_dump(sc);
	}

	return 0;
}

int
it8368_print(arg, pnp)
	void *arg;
	const char *pnp;
{
	if (pnp)
		printf("pcmcia at %s", pnp);

	return UNCONF;
}

int
it8368_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

void
it8368_attach_socket(sc)
	struct it8368e_softc *sc;
{
	struct pcmciabus_attach_args paa;

	paa.paa_busname = "pcmcia";
	paa.pct = (pcmcia_chipset_tag_t)&it8368_functions;
	paa.pch = (pcmcia_chipset_handle_t)sc;
	paa.iobase = 0;		/* I don't use them */
	paa.iosize = 0;
	
	if ((sc->sc_pcmcia = config_found_sm((void*)sc, &paa, it8368_print,
 					     it8368_submatch))) {

		it8368_init_socket(sc);
	}
}

void
it8368_init_socket(sc)
	struct it8368e_softc *sc;
{
	bus_space_tag_t csregt = sc->sc_csregt;
	bus_space_handle_t csregh = sc->sc_csregh;
	u_int16_t reg;
	
	/* 
	 *  set up the card to interrupt on card detect 
	 */
	reg = IT8368_PIN_CRDDET2; /* CSC */
	/* enable negative edge */
	it8368_reg_write(csregt, csregh, IT8368_GPIONEGINTEN_REG, reg);
	/* disable positive edge */
	it8368_reg_write(csregt, csregh, IT8368_GPIOPOSINTEN_REG, 0);

	sc->sc_ih = tx_intr_establish(sc->sc_tc, sc->sc_irq, 
				      IST_EDGE, IPL_BIO, it8368_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: can't establish interrupt\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	/* 
	 *  if there's a card there, then attach it. 
	 */
	reg = it8368_reg_read(csregt, csregh, IT8368_GPIODATAIN_REG);

	if (reg & (IT8368_PIN_CRDDET2|IT8368_PIN_CRDDET1)) {
		sc->sc_laststate = IT8368_LASTSTATE_EMPTY;
	} else {
		pcmcia_card_attach(sc->sc_pcmcia);
		sc->sc_laststate = IT8368_LASTSTATE_PRESENT;
	}
}

void *
it8368_chip_intr_establish(pch, pf, ipl, ih_fun, ih_arg)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_function *pf;
	int ipl;
	int (*ih_fun) __P((void *));
	void *ih_arg;
{
	struct it8368e_softc *sc = (struct it8368e_softc*) pch;
	bus_space_tag_t csregt = sc->sc_csregt;
	bus_space_handle_t csregh = sc->sc_csregh;
	u_int16_t reg;

	if (sc->sc_card_fun)
		panic("it8368_chip_intr_establish: "
		      "duplicate card interrupt handler.");
	
	sc->sc_card_fun = ih_fun;
	sc->sc_card_arg = ih_arg;

	sc->sc_card_ih = tx_intr_establish(sc->sc_tc, sc->sc_card_irq, 
					   IST_EDGE, IPL_BIO, it8368_intr,
					   sc);

	/* enable card interrupt */
	reg = it8368_reg_read(csregt, csregh, IT8368_GPIONEGINTEN_REG);
	reg |= IT8368_PIN_BCRDRDY;
	it8368_reg_write(csregt, csregh, IT8368_GPIONEGINTEN_REG, reg);
	
	return sc->sc_card_ih;
}

void 
it8368_chip_intr_disestablish(pch, ih)
	pcmcia_chipset_handle_t pch;
	void *ih;
{
	struct it8368e_softc *sc = (struct it8368e_softc*) pch;
	bus_space_tag_t csregt = sc->sc_csregt;
	bus_space_handle_t csregh = sc->sc_csregh;
	u_int16_t reg;

	if (!sc->sc_card_fun)
		panic("it8368_chip_intr_disestablish:"
		      "no handler established.");
	assert(ih == sc->sc_card_ih);
	
	sc->sc_card_fun = 0;
	sc->sc_card_arg = 0;

	/* disable card interrupt */
	reg = it8368_reg_read(csregt, csregh, IT8368_GPIONEGINTEN_REG);
	reg &= ~IT8368_PIN_BCRDRDY;
	it8368_reg_write(csregt, csregh, IT8368_GPIONEGINTEN_REG, reg);

	tx_intr_disestablish(sc->sc_tc, ih);
}

int 
it8368_chip_mem_alloc(pch, size, pcmhp)
	pcmcia_chipset_handle_t pch;
	bus_size_t size;
	struct pcmcia_mem_handle *pcmhp;
{
	struct it8368e_softc *sc = (struct it8368e_softc*) pch;
	it8368_access(sc, 0, 0);
	
	pcmhp->memt = sc->sc_csmemt;

	if (bus_space_map(sc->sc_csmemt, sc->sc_csmembase, size, 0, 
			  &pcmhp->memh)) {
		return 1;
	}

	pcmhp->addr = pcmhp->memh;
	pcmhp->size = size;
	pcmhp->realsize = size;

	DPRINTF(("it8368_chip_mem_alloc %#x+%#x\n", pcmhp->memh, size));

	return 0;
}

void 
it8368_chip_mem_free(pch, pcmhp)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_mem_handle *pcmhp;
{
	bus_space_unmap(pcmhp->memt, pcmhp->memh, pcmhp->size);
}

int 
it8368_chip_mem_map(pch, kind, card_addr, size, pcmhp, offsetp, windowp)
	pcmcia_chipset_handle_t pch;
	int kind;
	bus_addr_t card_addr;
	bus_size_t size;
	struct pcmcia_mem_handle *pcmhp;
	bus_addr_t *offsetp;
	int *windowp;
{
	struct it8368e_softc *sc = (struct it8368e_softc*) pch;

	it8368_access(sc, 0, 0);

	*offsetp = card_addr;
	DPRINTF(("it8368_chip_mem_map %#x+%#x\n", pcmhp->memh, size));

	return 0;
}

void 
it8368_chip_mem_unmap(pch, window)
	pcmcia_chipset_handle_t pch;
	int window;
{
}

void
it8368_access(sc, io, width)
	struct it8368e_softc *sc;
	int io;
	int width;
{
#if not_required_yet
	txreg_t reg32;	

	reg32 = tx_conf_read(sc->sc_tc, TX39_MEMCONFIG3_REG);
	if (io && width == 1) {
		reg32 |= TX39_MEMCONFIG3_PORT8SEL;
	} else {
		reg32 &= ~TX39_MEMCONFIG3_PORT8SEL;
	}
	if (!sc->sc_fixattr) {
		if (io) {
			reg32 |= TX39_MEMCONFIG3_CARD1IOEN;
		} else {
			reg32 &= ~TX39_MEMCONFIG3_CARD1IOEN;
		}
	}
	tx_conf_write(sc->sc_tc, TX39_MEMCONFIG3_REG, reg32);

	reg32 = tx_conf_read(sc->sc_tc, TX39_MEMCONFIG3_REG);
	if (!(reg32 & TX39_MEMCONFIG3_CARD1IOEN))
		printf("CARDIOEN failed\n");
	if (!(reg32 & TX39_MEMCONFIG3_PORT8SEL))
		printf("PORT8SEL failed\n");
	
	delay(20);
#endif
}

int 
it8368_chip_io_alloc(pch, start, size, align, pcihp)
	pcmcia_chipset_handle_t pch;
	bus_addr_t start;
	bus_size_t size;
	bus_size_t align;
	struct pcmcia_io_handle *pcihp;
{
	struct it8368e_softc *sc = (struct it8368e_softc*) pch;

	it8368_access(sc, 1, 0);

	if (start) {
		if (bus_space_map(sc->sc_csiot, start, size, 0, 
				  &pcihp->ioh)) {
			return 1;
		}
		DPRINTF(("it8368_chip_io_alloc map port %#x+%#x\n",
			 start, size));
	} else {
		if (bus_space_alloc(sc->sc_csiot, sc->sc_csiobase,
				    sc->sc_csiobase + sc->sc_csiosize,
				    size, align, 0, 0, &pcihp->addr, 
				    &pcihp->ioh)) {
				    
			return 1;
		}
		pcihp->flags = PCMCIA_IO_ALLOCATED;
		DPRINTF(("it8368_chip_io_alloc alloc %#x from %#x\n",
			 size, pcihp->addr));
	}

	pcihp->iot = sc->sc_csiot;
	pcihp->size = size;
	
	return 0;
}

int 
it8368_chip_io_map(pch, width, offset, size, pcihp, windowp)
	pcmcia_chipset_handle_t pch;
	int width;
	bus_addr_t offset;
	bus_size_t size;
	struct pcmcia_io_handle *pcihp;
	int *windowp;
{
	struct it8368e_softc *sc = (struct it8368e_softc*) pch;

	it8368_access(sc, 1, 0);

	DPRINTF(("it8368_chip_io_map %#x:%#x+%#x\n", pcihp->ioh, offset, 
		 size));

	return 0;
}

void 
it8368_chip_io_free(pch, pcihp)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_io_handle *pcihp;
{
	if (pcihp->flags & PCMCIA_IO_ALLOCATED) {
		bus_space_free(pcihp->iot, pcihp->ioh, pcihp->size);
	} else {
		bus_space_unmap(pcihp->iot, pcihp->ioh, pcihp->size);
	}
	DPRINTF(("it8368_chip_io_free %#x+%#x\n", pcihp->ioh, pcihp->size));
}

void 
it8368_chip_io_unmap(pch, window)
	pcmcia_chipset_handle_t pch;
	int window;
{
}

void
it8368_chip_socket_enable(pch)
	pcmcia_chipset_handle_t pch;
{
	struct it8368e_softc *sc = (struct it8368e_softc*)pch;
	bus_space_tag_t csregt = sc->sc_csregt;
	bus_space_handle_t csregh = sc->sc_csregh;
	volatile u_int16_t reg;

	/* Power off */
	reg = it8368_reg_read(csregt, csregh, IT8368_GPIODATAOUT_REG);
	reg &= ~(IT8368_PIN_CRDVCCMASK | IT8368_PIN_CRDVPPMASK);
	reg |= (IT8368_PIN_CRDVCC_0V | IT8368_PIN_CRDVPP_0V);
	it8368_reg_write(csregt, csregh, IT8368_GPIODATAOUT_REG, reg);
	delay(20000);

	/* 
	 * wait 300ms until power fails (Tpf).  Then, wait 100ms since
	 * we are changing Vcc (Toff).
	 */
	delay((300 + 100) * 1000);

	/* Supply Vcc */
	reg = it8368_reg_read(csregt, csregh, IT8368_GPIODATAOUT_REG);
	reg &= ~(IT8368_PIN_CRDVCCMASK | IT8368_PIN_CRDVPPMASK);
	reg |= IT8368_PIN_CRDVCC_5V; /* XXX */
	it8368_reg_write(csregt, csregh, IT8368_GPIODATAOUT_REG, reg);

	/*
	 * wait 100ms until power raise (Tpr) and 20ms to become
	 * stable (Tsu(Vcc)).
	 *
	 * some machines require some more time to be settled
	 * (300ms is added here).
	 */
	delay((100 + 20 + 300) * 1000);

	/* Assert reset signal */
	reg = it8368_reg_read(csregt, csregh, IT8368_GPIODATAOUT_REG);
	reg |= IT8368_PIN_BCRDRST;
	it8368_reg_write(csregt, csregh, IT8368_GPIODATAOUT_REG, reg);

	/*
	 * hold RESET at least 10us.
	 */
	delay(10);

	/* Dessert reset signal */
	reg = it8368_reg_read(csregt, csregh, IT8368_GPIODATAOUT_REG);
	reg &= ~IT8368_PIN_BCRDRST;	
	it8368_reg_write(csregt, csregh, IT8368_GPIODATAOUT_REG, reg);
	delay(20000);

	DPRINTF(("socket enabled\n"));
}

void
it8368_chip_socket_disable(pch)
	pcmcia_chipset_handle_t pch;
{
	struct it8368e_softc *sc = (struct it8368e_softc*) pch;
	bus_space_tag_t csregt = sc->sc_csregt;
	bus_space_handle_t csregh = sc->sc_csregh;
	u_int16_t reg;

	/* Power down */
	reg = it8368_reg_read(csregt, csregh, IT8368_GPIODATAOUT_REG);
	reg &= ~(IT8368_PIN_CRDVCCMASK | IT8368_PIN_CRDVPPMASK);
	reg |= (IT8368_PIN_CRDVCC_0V | IT8368_PIN_CRDVPP_0V);
	it8368_reg_write(csregt, csregh, IT8368_GPIODATAOUT_REG, reg);
	delay(20000);

	/*
	 * wait 300ms until power fails (Tpf).
	 */
	delay(300 * 1000);

	DPRINTF(("socket disabled\n"));
}

void
it8368_dump(sc)
	struct it8368e_softc *sc;
{
#ifdef IT8368DEBUG
	bus_space_tag_t csregt = sc->sc_csregt;
	bus_space_handle_t csregh = sc->sc_csregh;

	printf("[GPIO]\n");
	PRINTGPIO(DIR);
	PRINTGPIO(DATAIN);
	PRINTGPIO(DATAOUT);
	PRINTGPIO(POSINTEN);	
	PRINTGPIO(NEGINTEN);
	PRINTGPIO(POSINTSTAT);
	PRINTGPIO(NEGINTSTAT);
	printf("[MFIO]\n");
	PRINTMFIO(SEL);
	PRINTMFIO(DIR);
	PRINTMFIO(DATAIN);
	PRINTMFIO(DATAOUT);
	PRINTMFIO(POSINTEN);	
	PRINTMFIO(NEGINTEN);
	PRINTMFIO(POSINTSTAT);
	PRINTMFIO(NEGINTSTAT);
	__bitdisp(it8368_reg_read(csregt, csregh, IT8368_CTRL_REG), 0, 15,
		  "CTRL", 1);
	__bitdisp(it8368_reg_read(csregt, csregh, IT8368_GPIODATAIN_REG),
		  8, 11, "]CRDDET/SENSE[", 1);
#endif
}
