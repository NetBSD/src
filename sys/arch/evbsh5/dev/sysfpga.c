/*	$NetBSD: sysfpga.c,v 1.1 2002/07/05 13:31:39 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Cayman's System FPGA Chip */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/dev/femivar.h>
#include <sh5/dev/intcreg.h>
#include <evbsh5/dev/sysfpgareg.h>
#include <evbsh5/dev/sysfpgavar.h>

struct sysfpga_ihandler {
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_level;
	int ih_group;
	int ih_inum;
};

struct sysfpga_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
	void *sc_ih[SYSFPGA_NGROUPS];
	struct sysfpga_ihandler sc_ih_superio[SYSFPGA_SUPERIO_NINTR];
	u_int8_t sc_intmr[SYSFPGA_NGROUPS];
};

static int sysfpgamatch(struct device *, struct cfdata *, void *);
static void sysfpgaattach(struct device *, struct device *, void *);
static int sysfpgaprint(void *, const char *);

struct cfattach sysfpga_ca = {
	sizeof(struct sysfpga_softc), sysfpgamatch, sysfpgaattach
};
extern struct cfdriver sysfpga_cd;

/*
 * Devices which hang off the System FPGA
 */
struct sysfpga_device {
	const char *sd_name;
	bus_addr_t sd_offset;
};

static struct sysfpga_device sysfpga_devices[] = {
	{"superio", SYSFPGA_OFFSET_SUPERIO},
	{NULL, 0}
};


#define	sysfpga_reg_read(s,r)	\
	    bus_space_read_4((s)->sc_bust, (s)->sc_bush, (r))
#define	sysfpga_reg_write(s,r,v)	\
	    bus_space_write_4((s)->sc_bust, (s)->sc_bush, (r), (v))


static int sysfpga_intr_handler_irl1(void *);
static int sysfpga_intr_dispatch(struct sysfpga_softc *,
	    struct sysfpga_ihandler *, int);


static const char *sysfpga_cpuclksel[] = {
	"400/200/100MHz", "400/200/66MHz", "400/200/50MHz", "<invalid>"
};

static struct sysfpga_softc *sysfpga_sc;


/*ARGSUSED*/
static int
sysfpgamatch(struct device *parent, struct cfdata *cf, void *args)
{

	if (sysfpga_sc)
		return (0);

	return (cf->cf_driver == &sysfpga_cd);
}

/*ARGSUSED*/
static void
sysfpgaattach(struct device *parent, struct device *self, void *args)
{
	struct sysfpga_softc *sc = (struct sysfpga_softc *)self;
	struct femi_attach_args *fa = args;
	struct sysfpga_attach_args sa;
	u_int32_t reg;
	int i;

	sysfpga_sc = sc;

	sc->sc_bust = fa->fa_bust;

	bus_space_map(sc->sc_bust, fa->fa_offset + SYSFPGA_OFFSET_REGS,
	    SYSFPGA_REG_SZ, 0, &sc->sc_bush);

	reg = sysfpga_reg_read(sc, SYSFPGA_REG_DATE);
	printf(
	    ": Cayman System FPGA, Revision: %d - %02x/%02x/%02x (yy/mm/dd)\n",
	    SYSFPGA_DATE_REV(reg), SYSFPGA_DATE_YEAR(reg),
	    SYSFPGA_DATE_MONTH(reg), SYSFPGA_DATE_DATE(reg));

	reg = sysfpga_reg_read(sc, SYSFPGA_REG_BDMR);
	printf("%s: CPUCLKSEL: %s, CPU Clock Mode: %d\n", sc->sc_dev.dv_xname,
	    sysfpga_cpuclksel[SYSFPGA_BDMR_CPUCLKSEL(reg)],
	    SYSFPGA_CPUMR_CLKMODE(sysfpga_reg_read(sc, SYSFPGA_REG_CPUMR)));

	memset(sc->sc_ih_superio, 0, sizeof(sc->sc_ih_superio));
	for (i = 0; i < SYSFPGA_NGROUPS; i++) {
		sysfpga_reg_write(sc, SYSFPGA_REG_INTMR(i), 0);
		sc->sc_intmr[i] = 0;
	}

	/*
	 * Hook interrupts from the Super IO device
	 */
        sc->sc_ih[SYSFPGA_IGROUP_SUPERIO] =
            sh5_intr_establish(INTC_INTEVT_IRL1, IST_LEVEL, IPL_SUPERIO,
            sysfpga_intr_handler_irl1, sc);

	if (sc->sc_ih[SYSFPGA_IGROUP_SUPERIO] == NULL)
		panic("sysfpga: failed to register superio isr");

	/*
	 * Attach configured children
	 */
	sa._sa_base = fa->fa_offset;
	for (i = 0; sysfpga_devices[i].sd_name != NULL; i++) {
		sa.sa_name = sysfpga_devices[i].sd_name;
		sa.sa_bust = fa->fa_bust;
		sa.sa_dmat = fa->fa_dmat;
		sa.sa_offset = sysfpga_devices[i].sd_offset + sa._sa_base;

		(void) config_found(self, &sa, sysfpgaprint);
	}
}

static int
sysfpgaprint(void *arg, const char *cp)
{
	struct sysfpga_attach_args *sa = arg;

	if (cp)
		printf("%s at %s", sa->sa_name, cp);

	printf(" offset 0x%lx", sa->sa_offset - sa->_sa_base);

	return (UNCONF);
}

static int
sysfpga_intr_handler_irl1(void *arg)
{
	struct sysfpga_softc *sc = arg;
	struct sysfpga_ihandler *ih;
	u_int8_t intsr0;
	int h = 0;

	ih = sc->sc_ih_superio;

	for (intsr0 = sysfpga_reg_read(sc, 0);
	    (intsr0 & sc->sc_intmr[0]) != 0;
	    intsr0 = sysfpga_reg_read(sc, 0)) {
		intsr0 &= sc->sc_intmr[0];

		if (intsr0 & (1 << SYSFPGA_SUPERIO_INUM_UART1))
			h |= sysfpga_intr_dispatch(sc, ih,
			    SYSFPGA_SUPERIO_INUM_UART1);

		if (intsr0 & (1 << SYSFPGA_SUPERIO_INUM_UART2))
			h |= sysfpga_intr_dispatch(sc, ih,
			    SYSFPGA_SUPERIO_INUM_UART2);

		if (intsr0 & (1 << SYSFPGA_SUPERIO_INUM_MOUSE))
			h |= sysfpga_intr_dispatch(sc, ih,
			    SYSFPGA_SUPERIO_INUM_MOUSE);

		if (intsr0 & (1 << SYSFPGA_SUPERIO_INUM_KBD))
			h |= sysfpga_intr_dispatch(sc, ih,
			    SYSFPGA_SUPERIO_INUM_KBD);

		if (intsr0 & (1 << SYSFPGA_SUPERIO_INUM_LAN))
			h |= sysfpga_intr_dispatch(sc, ih,
			    SYSFPGA_SUPERIO_INUM_LAN);

		if (intsr0 & (1 << SYSFPGA_SUPERIO_INUM_IDE))
			h |= sysfpga_intr_dispatch(sc, ih,
			    SYSFPGA_SUPERIO_INUM_IDE);

		if (intsr0 & (1 << SYSFPGA_SUPERIO_INUM_LPT))
			h |= sysfpga_intr_dispatch(sc, ih,
			    SYSFPGA_SUPERIO_INUM_LPT);

		if (h == 0)
			panic("sysfpga: unclaimed IRL1 interrupt: 0x%02x",
			    intsr0);
	}

	return (h);
}

static int
sysfpga_intr_dispatch(struct sysfpga_softc *sc, struct sysfpga_ihandler *ih,
    int hnum)
{
	int h, s;

	ih += hnum;

#ifdef DEBUG
	if (ih->ih_func == NULL)
		panic("sysfpga_intr_dispatch: NULL handler for isr %d", hnum);
#endif

	/*
	 * This splraise() is fine since sysfpga's interrupt handler
	 * runs at a lower ipl than anything the child drivers could request.
	 */
	s = splraise(ih->ih_level);
	h = (*ih->ih_func)(ih->ih_arg);
	splx(s);

	return (h);
}

struct evcnt *
sysfpga_intr_evcnt(int group)
{
	struct sysfpga_softc *sc = sysfpga_sc;

	KDASSERT(group < SYSFPGA_NGROUPS);
	KDASSERT(sc->sc_ih[group] != NULL);

	return (sh5_intr_evcnt(sc->sc_ih[group]));
}

void *
sysfpga_intr_establish(int group, int level, int inum,
    int (*func)(void *), void *arg)
{
	struct sysfpga_softc *sc = sysfpga_sc;
	struct sysfpga_ihandler *ih;
	int s;

	switch (group) {
	case SYSFPGA_IGROUP_SUPERIO:
		KDASSERT(inum < SYSFPGA_SUPERIO_NINTR);
		KDASSERT(level >= IPL_SUPERIO);
		ih = sc->sc_ih_superio;
		break;

	default:
		return (NULL);
	}

	KDASSERT(ih->ih_func == NULL);

	ih += inum;
	ih->ih_level = level;
	ih->ih_group = group;
	ih->ih_inum = inum;
	ih->ih_arg = arg;
	ih->ih_func = func;

	s = splhigh();
	sc->sc_intmr[group] |= 1 << inum;
	sysfpga_reg_write(sc, SYSFPGA_REG_INTMR(group), sc->sc_intmr[group]);
	splx(s);

	return ((void *)ih);
}

void
sysfpga_intr_disestablish(void *cookie)
{
	struct sysfpga_softc *sc = sysfpga_sc;
	struct sysfpga_ihandler *ih = cookie;
	int s;

	s = splhigh();
	sc->sc_intmr[ih->ih_group] &= ~(1 << ih->ih_inum);
	sysfpga_reg_write(sc, SYSFPGA_REG_INTMR(ih->ih_group),
	    sc->sc_intmr[ih->ih_group]);
	splx(s);

	ih->ih_func = NULL;
}
