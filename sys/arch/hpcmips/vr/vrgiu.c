/*	$NetBSD: vrgiu.c,v 1.2 1999/11/07 14:07:50 uch Exp $	*/

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
#include <sys/malloc.h>
#include <sys/queue.h>
#define TAILQ_FOREACH(var, head, field)					\
	for (var = TAILQ_FIRST(head); var; var = TAILQ_NEXT(var, field))
#define	TAILQ_EMPTY(head) ((head)->tqh_first == NULL)

#include <mips/cpuregs.h>
#include <machine/bus.h>

#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrgiureg.h>

#include "locators.h"

#ifdef VRGIUDEBUG
int	vrgiu_debug = 1;
#define	DPRINTF(arg) if (vrgiu_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

#define	LEGAL_INTR_PORT(x)	((x) >= 0 && (x) < MAX_GPIO_INOUT)
#define	LEGAL_OUT_PORT(x)	((x) >= 0 && (x) < MAX_GPIO_OUT)

int vrgiu_match __P((struct device*, struct cfdata*, void*));
void vrgiu_attach __P((struct device*, struct device*, void*));
int vrgiu_intr __P((void*));
int vrgiu_print __P((void*, const char*));
void vrgiu_callback __P((struct device*));

void	vrgiu_dump_regs(struct vrgiu_softc *sc);
u_int32_t vrgiu_regread_4 __P((vrgiu_chipset_tag_t, bus_addr_t));
void	vrgiu_regwrite_4 __P((vrgiu_chipset_tag_t, bus_addr_t, u_int32_t));

int vrgiu_port_register __P((vrgiu_chipset_tag_t, enum gpio_name, int));
int vrgiu_port_read __P((vrgiu_chipset_tag_t, vrgiu_gpioreg_t*));
int vrgiu_port_write __P((vrgiu_chipset_tag_t, enum gpio_name, int));

void *vrgiu_intr_establish __P((vrgiu_chipset_tag_t, int, int, int, int (*)(void *), void*));
void vrgiu_intr_disestablish __P((vrgiu_chipset_tag_t, void*));

struct vrgiu_function_tag vrgiu_functions = {
	vrgiu_port_register,
	vrgiu_port_read,
	vrgiu_port_write,
	vrgiu_regread_4,
	vrgiu_regwrite_4,
	vrgiu_intr_establish,
	vrgiu_intr_disestablish
};

struct cfattach vrgiu_ca = {
	sizeof(struct vrgiu_softc), vrgiu_match, vrgiu_attach
};

int
vrgiu_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 2; /* 1st attach group of vrip */
}

void
vrgiu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct vrip_attach_args *va = aux;
	struct vrgiu_softc *sc = (void*)self;
	struct gpbus_attach_args gpa;
	int i;

	sc->sc_vc = va->va_vc;
	sc->sc_iot = va->va_iot;
	bus_space_map(sc->sc_iot, va->va_addr, va->va_size,
		      0 /* no cache */, &sc->sc_ioh);
	/*
	 *  Disable all interrupts.
	 */
	sc->sc_intr_mask = 0;
#ifdef WINCE_DEFAULT_SETTING
#warning WINCE_DEFAULT_SETTING
#else
	vrgiu_regwrite_4(sc, GIUINTEN_REG, sc->sc_intr_mask);
#endif
    
	for (i = 0; i < MAX_GPIO_INOUT; i++)
		TAILQ_INIT(&sc->sc_intr_head[i]);
	if (!(sc->sc_ih = vrip_intr_establish(va->va_vc, va->va_intr, IPL_BIO,
					      vrgiu_intr, sc))) {
		printf("%s: can't establish interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	vrgiu_functions.gf_intr_establish = vrgiu_intr_establish;
	vrgiu_functions.gf_intr_disestablish = vrgiu_intr_disestablish;
	/*
	 * Register functions to upper interface. 
	 */
	vrip_giu_function_register(va->va_vc, &vrgiu_functions, self);
	/* Display port status (Input/Output) for debugging */
	{
		vrgiu_gpioreg_t preg;
		vrgiu_port_read(sc, &preg);
		printf("Output-port:");
		bitdisp64(preg);
	}
	/* 
	 *  General purpose bus 
	 */
	for (i = 0; i< MAX_GPIO_INOUT; i++)
		sc->sc_gpio_map[i] = GIUPORT_NOTDEF;
	gpa.gpa_busname = "gpbus";
	gpa.gpa_gc = sc;
	gpa.gpa_gf = &vrgiu_functions;
	config_found(self, &gpa, vrgiu_print);	
	/*
	 * GIU-ISA bridge
	 */
#if 1 /* XXX Sometimes mounting root device failed. Why? XXX*/
	config_defer(self, vrgiu_callback);
#else
	vrgiu_callback(self);
#endif
}

void
vrgiu_callback(self)
	struct device *self;
{
	struct vrgiu_softc *sc = (void*)self;
	struct gpbus_attach_args gpa;

	gpa.gpa_busname = "vrisab";
	gpa.gpa_gc = sc;
	gpa.gpa_gf = &vrgiu_functions;
	config_found(self, &gpa, vrgiu_print);
}

int
vrgiu_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	if (pnp)
		return (QUIET);
	return (UNCONF);
}

void
vrgiu_dump_regs(sc)
	struct vrgiu_softc *sc;
{
	if (sc == NULL) {
		panic("%s(%d): VRGIU device not initialized\n",
		      __FILE__, __LINE__);
	}
	printf("    IOSEL: %08x\n", vrgiu_regread_4(sc, GIUIOSEL_REG));
	printf("     PIOD: %08x\n", vrgiu_regread_4(sc, GIUPIOD_REG));
	printf("    PODAT: %08x\n", vrgiu_regread_4(sc, GIUPODAT_REG));
	printf("  INTSTAT: %08x\n", vrgiu_regread_4(sc, GIUINTSTAT_REG));
	printf("    INTEN: %08x\n", vrgiu_regread_4(sc, GIUINTEN_REG));
	printf("   INTTYP: %08x\n", vrgiu_regread_4(sc, GIUINTTYP_REG));
	printf(" INTALSEL: %08x\n", vrgiu_regread_4(sc, GIUINTALSEL_REG));
	printf(" INTHTSEL: %08x\n", vrgiu_regread_4(sc, GIUINTHTSEL_REG));
}
/*
 * GIU regster access method.
 */
u_int32_t
vrgiu_regread_4(vc, offs)
	vrgiu_chipset_tag_t vc;
	bus_addr_t offs;
{
	struct vrgiu_softc *sc = (void*)vc;
	u_int16_t reg[2];
	bus_space_read_region_2 (sc->sc_iot, sc->sc_ioh, offs, reg, 2);
	return reg[0]|(reg[1]<<16);
}

void
vrgiu_regwrite_4(vc, offs, data)
	vrgiu_chipset_tag_t vc;
	bus_addr_t offs;
	u_int32_t data;
{
	struct vrgiu_softc *sc = (void*)vc;

	u_int16_t reg[2];
	reg[0] = data & 0xffff;
	reg[1] = (data>>16)&0xffff;
	bus_space_write_region_2 (sc->sc_iot, sc->sc_ioh, offs, reg, 2);
}
/*
 * Assign Platform independent port name to GPIO # map.
 */
int
vrgiu_port_register(ic, gpio, port)
	vrgiu_chipset_tag_t ic;
	enum gpio_name gpio;
	int port;
{
	struct vrgiu_softc *sc = (void*)ic;
	if (sc->sc_gpio_map[gpio] != GIUPORT_NOTDEF)
		panic("vrgiu_port_register: already defined port.");
	sc->sc_gpio_map[gpio] = port;
	return 0;
}
/*
 * PORT 
 */
int
vrgiu_port_read(vc, reg)
	vrgiu_chipset_tag_t vc;
	vrgiu_gpioreg_t *reg;
{
	(*reg)[0] = vrgiu_regread_4(vc, GIUPIOD_REG);
	(*reg)[1] = vrgiu_regread_4(vc, GIUPODAT_REG);
	return 0;
}
    
int
vrgiu_port_write(vc, gpio, onoff)
	vrgiu_chipset_tag_t vc;
	enum gpio_name gpio;
	int onoff;
{
	struct vrgiu_softc *sc = (void*)vc;
	vrgiu_gpioreg_t reg;
	int port, bank;

	if (!LEGAL_OUT_PORT(gpio))
		panic("vrgiu_port_write: illegal gpio name");
	if ((port = sc->sc_gpio_map[gpio]) == GIUPORT_NOTDEF) {
		printf ("vrgiu_port_write: not defined port name%d\n", gpio);
		return 0;
	}
	if (!LEGAL_OUT_PORT(port))
		panic("vrgiu_port_write: illegal gpio port");

	vrgiu_port_read(vc, &reg);
	bank = port < 32 ? 0 : 1;
	if (bank == 1)
		port -= 32;

	if (onoff)
		reg[bank] |= (1<<port);
	else
		reg[bank] &= ~(1<<port);
	vrgiu_regwrite_4(vc, GIUPIOD_REG, reg[0]);
	vrgiu_regwrite_4(vc, GIUPODAT_REG, reg[1]);

	return 0;
}
/* 
 *  For before autoconfiguration.  
 */
void
__vrgiu_out(port, data)
	int port;
	int data;
{
	u_int16_t reg;
	u_int32_t addr;
	int offs;

	if (!LEGAL_OUT_PORT(port))
		panic("__vrgiu_out: illegal gpio port");
	if (port < 16) {
		addr = MIPS_PHYS_TO_KSEG1((VRIP_GIU_ADDR + GIUPIOD_L_REG_W));
		offs = port;
	} else if (port < 32) {
		addr = MIPS_PHYS_TO_KSEG1((VRIP_GIU_ADDR + GIUPIOD_H_REG_W));
		offs = port - 16;
	} else if (port < 48) {
		addr = MIPS_PHYS_TO_KSEG1((VRIP_GIU_ADDR + GIUPODAT_L_REG_W));	
		offs = port - 32;
	} else {
		addr = MIPS_PHYS_TO_KSEG1((VRIP_GIU_ADDR + GIUPODAT_H_REG_W));	
		offs = port - 48;
		panic ("__vrgiu_out: not coded yet.");
	}
	printf ("__vrgiu_out: addr %08x bit %d\n", addr, offs);
    
	wbflush();
	reg = *((volatile u_int16_t*)addr);
	if (data) {
		reg |= (1 << offs);
	} else {
		reg &= ~(1 << offs);
	}
	*((volatile u_int16_t*)addr) = reg;
	wbflush();
}
/*
 * Interrupt staff 
 */
void *
vrgiu_intr_establish(ic, port, mode, level, ih_fun, ih_arg)
	vrgiu_chipset_tag_t ic;
	int port; /* GPIO pin # */
	int mode; /* GIU trigger setting */
	int level;  /* XXX not yet */
	int (*ih_fun) __P((void*));
	void *ih_arg;
{
	struct vrgiu_softc *sc = (void*)ic;
	int s;
	u_int32_t reg, mask;
	struct vrgiu_intr_entry *ih;

	if (!LEGAL_INTR_PORT(port))
		panic ("vrgiu_intr_establish: bogus interrupt line.");
	if (sc->sc_intr_mode[port] && mode != sc->sc_intr_mode[port])
		panic ("vrgiu_intr_establish: bogus interrupt type.");
	else
		sc->sc_intr_mode[port] = mode;
	mask = (1 << port);

	s = splhigh();

	if (!(ih = malloc(sizeof(struct vrgiu_intr_entry), M_DEVBUF, M_NOWAIT)))
		panic ("vrgiu_intr_establish: no memory.");

	ih->ih_port = port;
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	TAILQ_INSERT_TAIL(&sc->sc_intr_head[port], ih, ih_link);
#ifdef WINCE_DEFAULT_SETTING
#warning WINCE_DEFAULT_SETTING
#else
	/*
	 *  Setup registers
	 */
	/* Input mode */
	reg = vrgiu_regread_4(sc, GIUIOSEL_REG);
	reg &= ~mask;
	vrgiu_regwrite_4(sc, GIUIOSEL_REG, reg);

	/* interrupt type */
	reg = vrgiu_regread_4(sc, GIUINTTYP_REG);
	DPRINTF(("[%s->",reg & mask ? "edge" : "level"));
	if (mode & VRGIU_INTR_EDGE) {
		DPRINTF(("edge]"));
		reg |= mask;	/* edge */
	} else {
		DPRINTF(("level]"));
		reg &= ~mask;	/* level */
	}
	vrgiu_regwrite_4(sc, GIUINTTYP_REG, reg);

	/* interrupt level */
	if (!(mode & VRGIU_INTR_EDGE)) {
		reg = vrgiu_regread_4(sc, GIUINTALSEL_REG);
		DPRINTF(("[%s->",reg & mask ? "high" : "low"));
		if (mode & VRGIU_INTR_HIGH) {
			DPRINTF(("high]"));
			reg |= mask;	/* high */
		} else {
			DPRINTF(("low]"));
			reg &= ~mask;	/* low */
		}
		vrgiu_regwrite_4(sc, GIUINTALSEL_REG, reg);
	}
	/* hold or through */
	reg = vrgiu_regread_4(sc, GIUINTHTSEL_REG);
	DPRINTF(("[%s->",reg & mask ? "hold" : "through"));
	if (mode & VRGIU_INTR_HOLD) {
		DPRINTF(("hold]"));
		reg |= mask;	/* hold */
	} else {
		DPRINTF(("through]"));
		reg &= ~mask;	/* through */
	}
	vrgiu_regwrite_4(sc, GIUINTHTSEL_REG, reg);
#endif
	/*
	 *  clear interrupt status
	 */
	reg = vrgiu_regread_4(sc, GIUINTSTAT_REG);
	reg &= ~mask;
	vrgiu_regwrite_4(sc, GIUINTSTAT_REG, reg);
	/*
	 *  enable interrupt
	 */
#ifdef WINCE_DEFAULT_SETTING
#warning WINCE_DEFAULT_SETTING
#else
	sc->sc_intr_mask |= mask;
	vrgiu_regwrite_4(sc, GIUINTEN_REG, sc->sc_intr_mask);
	/* Unmask GIU level 2 mask register */
	vrip_intr_setmask2(sc->sc_vc, sc->sc_ih, (1<<port), 1);
#endif
	splx(s);

	DPRINTF(("\n"));
#if 0 && defined VRGIUDEBUG
	vrgiu_dump_regs(sc);
#endif

	return ih;
}

void
vrgiu_intr_disestablish(ic, arg)
	vrgiu_chipset_tag_t ic;
	void *arg;
{
	struct vrgiu_intr_entry *ihe = arg;
	struct vrgiu_softc *sc = (void*)ic;
	int port = ihe->ih_port;
	struct vrgiu_intr_entry *ih;
	int s;

	s = splhigh();
	TAILQ_FOREACH(ih, &sc->sc_intr_head[port], ih_link) {
		if (ih == ihe) {
			TAILQ_REMOVE(&sc->sc_intr_head[port], ih, ih_link);
			free(ih, M_DEVBUF);
			if (TAILQ_EMPTY(&sc->sc_intr_head[port])) {
				/* Disable interrupt */
#ifdef WINCE_DEFAULT_SETTING
#warning WINCE_DEFAULT_SETTING
#else
				sc->sc_intr_mask &= ~(1<<port);
				vrgiu_regwrite_4(sc, GIUINTEN_REG, sc->sc_intr_mask);
#endif
			}
			splx(s);
			return;
		}
	}
	panic("vrgiu_intr_disetablish: no such a handle.");
	/* NOTREACHED */
}

int
vrgiu_intr(arg)
	void *arg;
{
#ifdef DUMP_GIU_LEVEL2_INTR
#warning DUMP_GIU_LEVEL2_INTR
	static u_int32_t oreg;
#endif
	struct vrgiu_softc *sc = arg;
	int i;
	u_int32_t reg;
	/* Get Level 2 interrupt status */
	vrip_intr_get_status2 (sc->sc_vc, sc->sc_ih, &reg);
#ifdef DUMP_GIU_LEVEL2_INTR
#warning DUMP_GIU_LEVEL2_INTR
	{
		u_int32_t uedge, dedge, j;
		for (j = 0x80000000; j > 0; j >>=1)
			printf ("%c" , reg&j ? '|' : '.');
		uedge = (reg ^ oreg) & reg;
		dedge = (reg ^ oreg) & ~reg;
		if (uedge || dedge) {
			for (j = 0; j < 32; j++) {
				if (uedge & (1 << j))
					printf ("+%d", j);
				else if (dedge & (1 << j))
					printf ("-%d", j);
			}
		}
		oreg = reg;
		printf ("\n");
	}
#endif
	/* Clear interrupt */
	vrgiu_regwrite_4(sc, GIUINTSTAT_REG, vrgiu_regread_4(sc, GIUINTSTAT_REG));

	/* Dispatch handler */
	for (i = 0; i < MAX_GPIO_INOUT; i++) {
		if (reg & (1 << i)) {
			register struct vrgiu_intr_entry *ih;
			TAILQ_FOREACH(ih, &sc->sc_intr_head[i], ih_link) {
				ih->ih_fun(ih->ih_arg);
			}
		}
	}

	return 0;
}
