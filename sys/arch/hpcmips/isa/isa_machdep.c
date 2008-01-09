/*	$NetBSD: isa_machdep.c,v 1.32.38.1 2008/01/09 01:46:14 matt Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: isa_machdep.c,v 1.32.38.1 2008/01/09 01:46:14 matt Exp $");

#include "opt_vr41xx.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>
#include <machine/bus.h>
#include <machine/bus_space_hpcmips.h>
#include <machine/debug.h>

#include <dev/hpc/hpciovar.h>

#include <hpcmips/vr/vripif.h>

#include "locators.h"

#define VRISADEBUG

#ifdef VRISADEBUG
#ifndef VRISADEBUG_CONF
#define VRISADEBUG_CONF 0
#endif /* VRISADEBUG_CONF */
int vrisa_debug = VRISADEBUG_CONF;
#define DPRINTF(arg) if (vrisa_debug) printf arg;
#define DBITDISP(mask) if (vrisa_debug) dbg_bit_print(mask);
#define VPRINTF(arg) if (bootverbose || vrisa_debug) printf arg;
#else /* VRISADEBUG */
#define DPRINTF(arg)
#define DBITDISP(mask)
#define VPRINTF(arg) if (bootverbose) printf arg;
#endif /* VRISADEBUG */

/*
 * intrrupt no. encoding:
 *
 * 0x0000000f ISA IRQ#
 * 0x00ff0000 GPIO port#
 * 0x01000000 interrupt signal hold/through	(1:hold/0:though)
 * 0x02000000 interrupt detection level		(1:low /0:high	)
 * 0x04000000 interrupt detection trigger	(1:edge/0:level	)
 */
#define INTR_IRQ(i)	(((i)>> 0) & 0x0f)
#define INTR_PORT(i)	(((i)>>16) & 0xff)
#define INTR_MODE(i)	(((i)>>24) & 0x07)
#define INTR_NIRQS	16

int	vrisabprint(void *, const char *);
int	vrisabmatch(struct device *, struct cfdata *, void *);
void	vrisabattach(struct device *, struct device *, void *);

struct vrisab_softc {
	struct device sc_dev;
	hpcio_chip_t sc_hc;
	int sc_intr_map[INTR_NIRQS]; /* ISA <-> GIU inerrupt line mapping */
	struct hpcmips_isa_chipset sc_isa_ic;
};

CFATTACH_DECL(vrisab, sizeof(struct vrisab_softc),
    vrisabmatch, vrisabattach, NULL, NULL);

#ifdef DEBUG_FIND_PCIC
#include <mips/cpuregs.h>
#warning DEBUG_FIND_PCIC
static void __find_pcic(void);
#endif

#ifdef DEBUG_FIND_COMPORT
#include <mips/cpuregs.h>
#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>
#warning DEBUG_FIND_COMPORT
static void __find_comport(void);
#endif

int
vrisabmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct hpcio_attach_args *haa = aux;
	platid_mask_t mask;
	int n;

	if (strcmp(haa->haa_busname, match->cf_name))
		return (0);

	if (match->cf_loc[HPCIOIFCF_PLATFORM] == HPCIOIFCF_PLATFORM_DEFAULT) 
		return (1);

	mask = PLATID_DEREF(match->cf_loc[HPCIOIFCF_PLATFORM]);
	if ((n = platid_match(&platid, &mask)) != 0)
		return (n + 2);

	return (0);
}

void
vrisabattach(struct device *parent, struct device *self, void *aux)
{
	struct hpcio_attach_args *haa = aux;
	struct vrisab_softc *sc = (void*)self;
	struct isabus_attach_args iba;
	struct bus_space_tag_hpcmips *iot, *memt;
	bus_addr_t offset;
	int i;
    
	sc->sc_hc = (*haa->haa_getchip)(haa->haa_sc, VRIP_IOCHIP_VRGIU);
	sc->sc_isa_ic.ic_sc = sc;

	iba.iba_ic	= &sc->sc_isa_ic;
	iba.iba_dmat    = 0; /* XXX not yet */

	/* Allocate ISA memory space */
	memt = hpcmips_alloc_bus_space_tag();
	offset = device_cfdata(&sc->sc_dev)->cf_loc[VRISABIFCF_ISAMEMOFFSET];
	hpcmips_init_bus_space(memt,
	    (struct bus_space_tag_hpcmips *)haa->haa_iot, "ISA mem",
	    VR_ISA_MEM_BASE + offset, VR_ISA_MEM_SIZE - offset);
	iba.iba_memt = &memt->bst;

	/* Allocate ISA port space */
	iot = hpcmips_alloc_bus_space_tag();
	offset = device_cfdata(&sc->sc_dev)->cf_loc[VRISABIFCF_ISAPORTOFFSET];
	hpcmips_init_bus_space(iot,
	    (struct bus_space_tag_hpcmips *)haa->haa_iot, "ISA port",
	    VR_ISA_PORT_BASE + offset, VR_ISA_PORT_SIZE - offset);
	iba.iba_iot = &iot->bst;

#ifdef DEBUG_FIND_PCIC
#warning DEBUG_FIND_PCIC
	__find_pcic();
#else
	/* Initialize ISA IRQ <-> GPIO mapping */
	for (i = 0; i < INTR_NIRQS; i++)
		sc->sc_intr_map[i] = -1;
	printf(": ISA port %#x-%#x mem %#x-%#x\n",
	    iot->base, iot->base + iot->size,
	    memt->base, memt->base + memt->size);
	config_found_ia(self, "isabus", &iba, vrisabprint);
#endif

#ifdef DEBUG_FIND_COMPORT
#warning DEBUG_FIND_COMPORT
	__find_comport();
#endif
}

int
vrisabprint(void *aux, const char *pnp)
{
	if (pnp)
		return (QUIET);

	return (UNCONF);
}

void
isa_attach_hook(struct device *parent, struct device *self,
    struct isabus_attach_args *iba)
{

}

const struct evcnt *
isa_intr_evcnt(isa_chipset_tag_t ic, int irq)
{

	/* XXX for now, no evcnt parent reported */
	return (NULL);
}

void *
isa_intr_establish(isa_chipset_tag_t ic, int intr, int type, int level,
    int (*ih_fun)(void*), void *ih_arg)
{
	struct vrisab_softc *sc = ic->ic_sc;
	int port, irq, mode;

	static int intr_modes[8] = {
		HPCIO_INTR_LEVEL_HIGH_THROUGH,
		HPCIO_INTR_LEVEL_HIGH_HOLD,
		HPCIO_INTR_LEVEL_LOW_THROUGH,
		HPCIO_INTR_LEVEL_LOW_HOLD,
		HPCIO_INTR_EDGE_THROUGH,
		HPCIO_INTR_EDGE_HOLD,
		HPCIO_INTR_EDGE_THROUGH,
		HPCIO_INTR_EDGE_HOLD,
	};
#ifdef VRISADEBUG
	static const char* intr_mode_names[8] = {
		"level high through",
		"level high hold",
		"level low through",
		"level low hold",
		"edge through",
		"edge hold",
		"edge through",
		"edge hold",
	};
#endif /* VRISADEBUG */
	/* 
	 * ISA IRQ <-> GPIO port mapping
	 */
	irq = INTR_IRQ(intr);
	if (sc->sc_intr_map[irq] != -1) {
		/* already mapped */
		intr = sc->sc_intr_map[irq];
	} else {
		/* not mapped yet */
		sc->sc_intr_map[irq] = intr; /* Register it */
	}
	mode = INTR_MODE(intr);
	port = INTR_PORT(intr);

	VPRINTF(("ISA IRQ %d -> %s port %d, %s\n",
	    irq, sc->sc_hc->hc_name, port, intr_mode_names[mode]));

	/* Call Vr routine */
	return (hpcio_intr_establish(sc->sc_hc, port, intr_modes[mode],
	    ih_fun, ih_arg));
}

void
isa_intr_disestablish(ic, arg)
	isa_chipset_tag_t ic;
	void *arg;
{
	struct vrisab_softc *sc = ic->ic_sc;
	/* Call Vr routine */
	hpcio_intr_disestablish(sc->sc_hc, arg);
}

int
isa_intr_alloc(isa_chipset_tag_t ic, int mask, int type, int *irq)
{
	/* XXX not coded yet. this is temporary XXX */
	DPRINTF(("isa_intr_alloc:"));
	DBITDISP(mask);
	*irq = (ffs(mask) -1); /* XXX */

	return (0);
}

#ifdef DEBUG_FIND_PCIC
#warning DEBUG_FIND_PCIC
static void
__find_pcic(void)
{
	int i, j, step, found;
	u_int32_t addr;
	u_int8_t reg;
	int __read_revid (u_int32_t port) 
	    {
		    addr = MIPS_PHYS_TO_KSEG1(i + port);
		    printf("%#x\r", i);
		    for (found = 0, j = 0; j < 0x100; j += 0x40) {
			    *((volatile u_int8_t *)addr) = j;
			    reg = *((volatile u_int8_t *)(addr + 1));
#ifdef DEBUG_FIND_PCIC_I82365SL_ONLY
			    if (reg == 0x82 || reg == 0x83) {
#else
			    if ((reg & 0xc0) == 0x80) {
#endif
					    found++;
			    }
			    if (found)
				    printf("\nfound %d socket at %#x"
					"(base from %#x)\n", found, addr,
					i + port - VR_ISA_PORT_BASE);
		    }
	    }
	step = 0x1000000;
	printf("\nFinding PCIC. Trying ISA port %#x-%#x step %#x\n", 
	    VR_ISA_PORT_BASE, VR_ISA_PORT_BASE + VR_ISA_PORT_SIZE, step);
	for (i = VR_ISA_PORT_BASE; i < VR_ISA_PORT_BASE+VR_ISA_PORT_SIZE;
	    i+= step) {
		__read_revid (0x3e0);
		__read_revid (0x3e2);
	}
}
#endif /* DEBUG_FIND_PCIC */


#ifdef DEBUG_FIND_COMPORT
#warning DEBUG_FIND_COMPORT

static int probe_com(u_int32_t);

static int
probe_com(u_int32_t port_addr)
{
	u_int32_t addr;
	u_int8_t ubtmp1, ubtmp2;

	addr = MIPS_PHYS_TO_KSEG1(port_addr);

	*((volatile u_int8_t *)(addr + com_cfcr)) = LCR_8BITS;
	*((volatile u_int8_t *)(addr + com_iir)) = 0;

	ubtmp1 = *((volatile u_int8_t *)(addr + com_cfcr));
	ubtmp2 = *((volatile u_int8_t *)(addr + com_iir));

	if ((ubtmp1 != LCR_8BITS) || ((ubtmp2 & 0x38) != 0)) {
		return (0);
	}

	return (1);
}

static void
__find_comport()
{
	int found;
	u_int32_t port, step;

	found = 0;
	step = 0x08;

	printf("Searching COM port. Trying ISA port %#x-%#x step %#x\n",
	    VR_ISA_PORT_BASE, VR_ISA_PORT_BASE + VR_ISA_PORT_SIZE - 1, step );

	for (port = VR_ISA_PORT_BASE;
	    port < (VR_ISA_PORT_BASE + VR_ISA_PORT_SIZE); port += step){
		if (probe_com(port)) {
			found++;
			printf("found %d at %#x\n", found, port);
		}
	}
}
#endif /* DEBUG_FIND_COMPORT */
