/*	$NetBSD: sa11x0.c,v 1.6.2.3 2001/03/27 15:30:52 bouyer Exp $	*/

/*-
 * Copyright (c) 2001, The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro and Ichiro FUKUHARA.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 */
/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <arm/mainbus/mainbus.h>
#include <hpcarm/sa11x0/sa11x0_reg.h>
#include <hpcarm/sa11x0/sa11x0_var.h>
#include <hpcarm/sa11x0/sa11x0_dmacreg.h>
#include <hpcarm/sa11x0/sa11x0_gpioreg.h>

#include "locators.h"

/* prototypes */
static int	sa11x0_match(struct device *, struct cfdata *, void *);
static void	sa11x0_attach(struct device *, struct device *, void *);
static int 	sa11x0_search(struct device *, struct cfdata *, void *);
static int	sa11x0_print(void *, const char *);

/* attach structures */
struct cfattach saip_ca = {
	sizeof(struct sa11x0_softc), sa11x0_match, sa11x0_attach
};

extern struct bus_space sa11x0_bs_tag;
extern vaddr_t saipic_base;

extern int SetCPSR(int, int);

#ifdef DEBUG  /* XXX */
extern int sacomcncharpoll();

int hoge(void *p)
{
	static int i = 0;
	int c = sacomcncharpoll();
	*(u_int32_t *)0xd0001010 = 8 | 2;       /* clear intr status bit */

	i++;
	if (! (i & 7))
		printf("h %ld.%03ld\n", time.tv_sec, time.tv_usec / 1000);
	if (c == 1)
		cpu_Debugger();
	return 1;
}
#endif

/*
 * int sa11x0_print(void *aux, const char *name)
 * print configuration info for children
 */

static int
sa11x0_print(aux, name)
	void *aux;
	const char *name;
{
	struct sa11x0_attach_args *sa = (struct sa11x0_attach_args*)aux;

	if (sa->sa_size)
                printf(" addr 0x%lx", sa->sa_addr);
        if (sa->sa_size > 1)
                printf("-0x%lx", sa->sa_addr + sa->sa_size - 1);
	if (sa->sa_memsize)
		printf(" membase 0x%lx", sa->sa_membase);
	if (sa->sa_memsize > 1)
		printf("-0x%lx", sa->sa_membase + sa->sa_memsize - 1);
        if (sa->sa_intr > 1)
                printf(" intr %d", sa->sa_intr);
	printf("\n");
        return (UNCONF);

}

int
sa11x0_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return 1;
}

void
sa11x0_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct sa11x0_softc *sc = (struct sa11x0_softc*)self;

	sc->sc_iot = &sa11x0_bs_tag;

	/* Map the SAIP */
	if (bus_space_map(sc->sc_iot, SAIPIC_BASE, SAIPIC_NPORTS,
			0, &sc->sc_ioh))
		panic("%s: Cannot map registers\n", self->dv_xname);
	saipic_base = sc->sc_ioh;

	/* Map the GPIO registers */
	if (bus_space_map(sc->sc_iot, SAGPIO_BASE, SAGPIO_NPORTS,
			  0, &sc->sc_gpioh))
		panic("%s: unable to map GPIO registers\n", self->dv_xname);

	/* Map the DMA controller registers */
	if (bus_space_map(sc->sc_iot, SADMAC_BASE, SADMAC_NPORTS,
			  0, &sc->sc_dmach))
		panic("%s: unable to map DMAC registers\n", self->dv_xname);

	printf("\n");

	/*
	 *  Mask all interrupts.
	 *  They are later unmasked at each device's attach routine.
	 */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAIPIC_MR, 0);

	/* Route all bits to IRQ */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAIPIC_LR, 0);

	/* Clear idle mask */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SAIPIC_CR, 0);

	/* diable all DMAC channels */
	bus_space_write_4(sc->sc_iot, sc->sc_dmach, SADMAC_DCR0_CLR, 1);
	bus_space_write_4(sc->sc_iot, sc->sc_dmach, SADMAC_DCR1_CLR, 1);
	bus_space_write_4(sc->sc_iot, sc->sc_dmach, SADMAC_DCR2_CLR, 1);
	bus_space_write_4(sc->sc_iot, sc->sc_dmach, SADMAC_DCR3_CLR, 1);
	bus_space_write_4(sc->sc_iot, sc->sc_dmach, SADMAC_DCR4_CLR, 1);
	bus_space_write_4(sc->sc_iot, sc->sc_dmach, SADMAC_DCR5_CLR, 1);

#ifdef DEBUG  /* XXX */
	sa11x0_intr_establish(0, 30, 1, IPL_HIGH, hoge, 0);
	*((u_int32_t *)0xd0001010) = 0x8;
#endif
	/*
	 * XXX this is probably a bad place, but intr bit shouldn't be
	 * XXX enabled before intr mask is set.
	 * XXX Having sane imask[] suffice??
	 */
	SetCPSR(I32_bit, 0);

	/*
	 *  Attach each devices
	 */
	config_search(sa11x0_search, self, sa11x0_print);
}

int
sa11x0_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sa11x0_softc *sc = (struct sa11x0_softc *)parent;
	struct sa11x0_attach_args sa;

	sa.sa_sc = sc;
        sa.sa_iot = sc->sc_iot;
        sa.sa_addr = cf->cf_loc[SAIPCF_ADDR];
        sa.sa_size = cf->cf_loc[SAIPCF_SIZE];
	sa.sa_membase = cf->cf_loc[SAIPCF_MEMBASE];
        sa.sa_memsize = cf->cf_loc[SAIPCF_MEMSIZE];
        sa.sa_intr = cf->cf_loc[SAIPCF_INTR];

        if (((*cf->cf_attach->ca_match)(parent, cf, &sa) == sc->sc_pri))
                config_attach(parent, cf, &sa, sa11x0_print);

        return 0;
}

/* end of sa11x0.c */

