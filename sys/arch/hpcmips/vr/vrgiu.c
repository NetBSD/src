/*	$NetBSD: vrgiu.c,v 1.25 2001/09/07 16:04:03 sato Exp $	*/
/*-
 * Copyright (c) 1999-2001
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
#include <sys/reboot.h>

#include <mips/cpuregs.h>
#include <machine/bus.h>
#include <machine/config_hook.h>

#include <dev/hpc/hpciovar.h>

#include "opt_vr41xx.h"
#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrgiureg.h>
#include <hpcmips/vr/vrgiuvar.h>

#include "locators.h"

#define VRGIUDEBUG
#ifdef VRGIUDEBUG
#define DEBUG_IO	1
#define DEBUG_INTR	2
#ifndef VRGIUDEBUG_CONF
#define VRGIUDEBUG_CONF 0
#endif /* VRGIUDEBUG_CONF */
int	vrgiu_debug = VRGIUDEBUG_CONF;
#define	DPRINTF(flag, arg) if (vrgiu_debug & flag) printf arg;
#define DDUMP_IO(flag, sc) if (vrgiu_debug & flag) vrgiu_dump_io(sc);
#define DDUMP_IOSETTING(flag, sc) \
		if (vrgiu_debug & flag) vrgiu_dump_iosetting(sc);
#define	VPRINTF(flag, arg) \
		if (bootverbose || vrgiu_debug & flag) printf arg;
#define VDUMP_IO(flag, sc) \
		if (bootverbose || vrgiu_debug & flag) vrgiu_dump_io(sc);
#define VDUMP_IOSETTING(flag, sc) \
		if (bootverbose || vrgiu_debug & flag) vrgiu_dump_iosetting(sc);
#else
#define	DPRINTF(flag, arg)
#define DDUMP_IO(flag, sc)
#define DDUMP_IOSETTING(flag, sc)
#define	VPRINTF(flag, arg) if (bootverbose) printf arg;
#define VDUMP_IO(flag, sc) if (bootverbose) vrgiu_dump_io(sc);
#define VDUMP_IOSETTING(flag, sc) \
			if (bootverbose) vrgiu_dump_iosetting(sc);
#endif

#ifdef VRGIU_INTR_NOLED
int vrgiu_intr_led = 0;
#else /* VRGIU_INTR_NOLED */
int vrgiu_intr_led = 1;
#endif /* VRGIU_INTR_NOLED */

#define	LEGAL_INTR_PORT(x)	((x) >= 0 && (x) < MAX_GPIO_INOUT)
#define	LEGAL_OUT_PORT(x)	((x) >= 0 && (x) < MAX_GPIO_OUT)

int vrgiu_match(struct device*, struct cfdata*, void*);
void vrgiu_attach(struct device*, struct device*, void*);
int vrgiu_intr(void*);
int vrgiu_print(void*, const char*);
void vrgiu_callback(struct device*);

void	vrgiu_dump_regs(struct vrgiu_softc *);
void	vrgiu_dump_io(struct vrgiu_softc *);
void	vrgiu_diff_io(void);
void	vrgiu_dump_iosetting(struct vrgiu_softc *);
void	vrgiu_diff_iosetting(void);
u_int32_t vrgiu_regread_4(struct vrgiu_softc *, bus_addr_t);
u_int16_t vrgiu_regread(struct vrgiu_softc *, bus_addr_t);
void	vrgiu_regwrite_4(struct vrgiu_softc *, bus_addr_t, u_int32_t);
void	vrgiu_regwrite(struct vrgiu_softc *, bus_addr_t, u_int16_t);

static int vrgiu_port_read(hpcio_chip_t, int);
static void vrgiu_port_write(hpcio_chip_t, int, int);
static void *vrgiu_intr_establish(hpcio_chip_t, int, int, int (*)(void *), void*);
static void vrgiu_intr_disestablish(hpcio_chip_t, void*);
static void vrgiu_intr_clear(hpcio_chip_t, void*);
static void vrgiu_register_iochip(hpcio_chip_t, hpcio_chip_t);
static void vrgiu_update(hpcio_chip_t);
static void vrgiu_dump(hpcio_chip_t);
static hpcio_chip_t vrgiu_getchip(void*, int);

static struct hpcio_chip vrgiu_iochip = {
	.hc_portread =		vrgiu_port_read,
	.hc_portwrite =		vrgiu_port_write,
	.hc_intr_establish =	vrgiu_intr_establish,
	.hc_intr_disestablish =	vrgiu_intr_disestablish,
	.hc_intr_clear =	vrgiu_intr_clear,
	.hc_register_iochip =	vrgiu_register_iochip,
	.hc_update =		vrgiu_update,
	.hc_dump =		vrgiu_dump,
};

struct cfattach vrgiu_ca = {
	sizeof(struct vrgiu_softc), vrgiu_match, vrgiu_attach
};

struct vrgiu_softc *this_giu;

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
	struct hpcio_attach_args haa;
	int i;

	this_giu = sc;
	sc->sc_vc = va->va_vc;
	sc->sc_iot = va->va_iot;
	bus_space_map(sc->sc_iot, va->va_addr, va->va_size,
		      0 /* no cache */, &sc->sc_ioh);
	/*
	 *  Disable all interrupts.
	 */
	sc->sc_intr_mask = 0;
	printf("\n");
#ifdef WINCE_DEFAULT_SETTING
#warning WINCE_DEFAULT_SETTING
#else
	VPRINTF(DEBUG_IO, ("WIN setting:                                "));
	VDUMP_IOSETTING(DEBUG_IO, sc);
	VPRINTF(DEBUG_IO, ("\n"));
	vrgiu_regwrite_4(sc, GIUINTEN_REG, sc->sc_intr_mask);
#endif
    
	for (i = 0; i < MAX_GPIO_INOUT; i++)
		TAILQ_INIT(&sc->sc_intr_head[i]);
	if (!(sc->sc_ih = vrip_intr_establish(va->va_vc, va->va_intr, IPL_BIO,
					      vrgiu_intr, sc))) {
		printf("%s: can't establish interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	/*
	 * fill hpcio_chip structure
	 */
	sc->sc_iochip = vrgiu_iochip; /* structure copy */
	sc->sc_iochip.hc_chipid = VRIP_IOCHIP_VRGIU;
	sc->sc_iochip.hc_name = sc->sc_dev.dv_xname;
	sc->sc_iochip.hc_sc = sc;
	/* Register functions to upper interface */
	vrip_gpio_register(va->va_vc, &sc->sc_iochip);

	/* Display port status (Input/Output) for debugging */
	VPRINTF(DEBUG_IO, ("I/O setting:                                "));
	VDUMP_IOSETTING(DEBUG_IO, sc);
	VPRINTF(DEBUG_IO, ("\n"));
	VPRINTF(DEBUG_IO, ("       data:"));
	VDUMP_IO(DEBUG_IO, sc);

	/* 
	 *  hpcio I/F
	 */
	haa.haa_busname = HPCIO_BUSNAME;
	haa.haa_sc = sc;
	haa.haa_getchip = vrgiu_getchip;
	haa.haa_iot = sc->sc_iot;
	while (config_found(self, &haa, vrgiu_print)) ;
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
	struct hpcio_attach_args haa;

	haa.haa_busname = "vrisab";
	haa.haa_sc = sc;
	haa.haa_getchip = vrgiu_getchip;
	haa.haa_iot = sc->sc_iot;
	config_found(self, &haa, vrgiu_print);
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
vrgiu_dump_iosetting(sc)
	struct vrgiu_softc *sc;
{
	long iosel, inten, useupdn, termupdn;
	u_int32_t m;
	iosel= vrgiu_regread_4(sc, GIUIOSEL_REG);
	inten= vrgiu_regread_4(sc, GIUINTEN_REG);
#ifdef ONLY_VR4122_4131
	useupdn = termupdn = 0;
#else
	useupdn = vrgiu_regread(sc, GIUUSEUPDN_REG_W);
	termupdn = vrgiu_regread(sc, GIUTERMUPDN_REG_W);
#endif
	for (m = 0x80000000; m; m >>=1)
		printf ("%c" , (useupdn&m) ?
			((termupdn&m) ? 'U' : 'D') :
			((iosel&m) ? 'o' : ((inten&m)?'I':'i')));
}

void
vrgiu_diff_iosetting()
{
	struct vrgiu_softc *sc = this_giu;
	static long oiosel = 0, ointen = 0, ouseupdn = 0, otermupdn = 0;
	long iosel, inten, useupdn, termupdn;
	u_int32_t m;

	iosel= vrgiu_regread_4(sc, GIUIOSEL_REG);
	inten= vrgiu_regread_4(sc, GIUINTEN_REG);
#ifdef ONLY_VR4122_4131
	useupdn = termupdn = 0;
#else
	useupdn = vrgiu_regread(sc, GIUUSEUPDN_REG_W);
	termupdn = vrgiu_regread(sc, GIUTERMUPDN_REG_W);
#endif
	if (oiosel != iosel || ointen != inten ||
	    ouseupdn != useupdn || otermupdn != termupdn) {
		for (m = 0x80000000; m; m >>=1)
			printf ("%c" , (useupdn&m) ?
				((termupdn&m) ? 'U' : 'D') :
				((iosel&m) ? 'o' : ((inten&m)?'I':'i')));
	}
	oiosel = iosel;
	ointen = inten;
	ouseupdn = useupdn;
	otermupdn = termupdn;
}

void
vrgiu_dump_io(sc)
	struct vrgiu_softc *sc;
{
	u_int32_t preg[2];

	preg[0] = vrgiu_regread_4(sc, GIUPIOD_REG);
	preg[1] = vrgiu_regread_4(sc, GIUPODAT_REG);

	bitdisp64(preg);
}

void
vrgiu_diff_io()
{
	struct vrgiu_softc *sc  = this_giu;
	static u_int32_t opreg[2] = {0, 0};
	u_int32_t preg[2];

	preg[0] = vrgiu_regread_4(sc, GIUPIOD_REG);
	preg[1] = vrgiu_regread_4(sc, GIUPODAT_REG);

	if (opreg[0] != preg[0] || opreg[1] != preg[1]) {
		printf("giu data: ");
		bitdisp64(preg);
	}
	opreg[0] = preg[0];
	opreg[1] = preg[1];
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
vrgiu_regread_4(sc, offs)
	struct vrgiu_softc *sc;
	bus_addr_t offs;
{
	u_int16_t reg[2];
	bus_space_read_region_2 (sc->sc_iot, sc->sc_ioh, offs, reg, 2);
	return reg[0]|(reg[1]<<16);
}

u_int16_t
vrgiu_regread(sc, off)
	struct vrgiu_softc *sc;
	bus_addr_t off;
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, off);
}

void
vrgiu_regwrite_4(sc, offs, data)
	struct vrgiu_softc *sc;
	bus_addr_t offs;
	u_int32_t data;
{
	u_int16_t reg[2];
	reg[0] = data & 0xffff;
	reg[1] = (data>>16)&0xffff;
	bus_space_write_region_2 (sc->sc_iot, sc->sc_ioh, offs, reg, 2);
}

void
vrgiu_regwrite(sc, off, data)
	struct vrgiu_softc *sc;
	bus_addr_t off;
	u_int16_t data;
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, off, data);
}

/*
 * PORT 
 */
int
vrgiu_port_read(hc, port)
	hpcio_chip_t hc;
	int port;
{
	struct vrgiu_softc *sc = hc->hc_sc;
	int on;

	if (!LEGAL_OUT_PORT(port))
		panic("vrgiu_port_read: illegal gpio port");

	if (port < 32)
		on = (vrgiu_regread_4(sc, GIUPIOD_REG) & (1 << port));
	else
		on = (vrgiu_regread_4(sc, GIUPODAT_REG) & (1 << (port - 32)));

	return (on ? 1 : 0);
}
    
void
vrgiu_port_write(hc, port, onoff)
	hpcio_chip_t hc;
	int port;
	int onoff;
{
	struct vrgiu_softc *sc = hc->hc_sc;
	u_int32_t reg[2];
	int bank;

	if (!LEGAL_OUT_PORT(port))
		panic("vrgiu_port_write: illegal gpio port");

	reg[0] = vrgiu_regread_4(sc, GIUPIOD_REG);
	reg[1] = vrgiu_regread_4(sc, GIUPODAT_REG);
	bank = port < 32 ? 0 : 1;
	if (bank == 1)
		port -= 32;

	if (onoff)
		reg[bank] |= (1<<port);
	else
		reg[bank] &= ~(1<<port);
	vrgiu_regwrite_4(sc, GIUPIOD_REG, reg[0]);
	vrgiu_regwrite_4(sc, GIUPODAT_REG, reg[1]);
}

static void
vrgiu_update(hc)
	hpcio_chip_t hc;
{
}

static void
vrgiu_dump(hc)
	hpcio_chip_t hc;
{
}

static hpcio_chip_t
vrgiu_getchip(scx, chipid)
	void* scx;
	int chipid;
{
	struct vrgiu_softc *sc = scx;

	return (&sc->sc_iochip);
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
	DPRINTF(DEBUG_IO, ("__vrgiu_out: addr %08x bit %d\n", addr, offs));
    
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
vrgiu_intr_establish(hc, port, mode, ih_fun, ih_arg)
	hpcio_chip_t hc;
	int port; /* GPIO pin # */
	int mode; /* GIU trigger setting */
	int (*ih_fun) __P((void*));
	void *ih_arg;
{
	struct vrgiu_softc *sc = hc->hc_sc;
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
	DPRINTF(DEBUG_INTR, ("[%s->",reg & mask ? "edge" : "level"));
	if (mode & HPCIO_INTR_EDGE) {
		DPRINTF(DEBUG_INTR, ("edge]"));
		reg |= mask;	/* edge */
	} else {
		DPRINTF(DEBUG_INTR, ("level]"));
		reg &= ~mask;	/* level */
	}
	vrgiu_regwrite_4(sc, GIUINTTYP_REG, reg);

	/* interrupt level */
	if (!(mode & HPCIO_INTR_EDGE)) {
		reg = vrgiu_regread_4(sc, GIUINTALSEL_REG);
		DPRINTF(DEBUG_INTR, ("[%s->",reg & mask ? "high" : "low"));
		if (mode & HPCIO_INTR_HIGH) {
			DPRINTF(DEBUG_INTR, ("high]"));
			reg |= mask;	/* high */
		} else {
			DPRINTF(DEBUG_INTR, ("low]"));
			reg &= ~mask;	/* low */
		}
		vrgiu_regwrite_4(sc, GIUINTALSEL_REG, reg);
	}
	/* hold or through */
	reg = vrgiu_regread_4(sc, GIUINTHTSEL_REG);
	DPRINTF(DEBUG_INTR, ("[%s->",reg & mask ? "hold" : "through"));
	if (mode & HPCIO_INTR_HOLD) {
		DPRINTF(DEBUG_INTR, ("hold]"));
		reg |= mask;	/* hold */
	} else {
		DPRINTF(DEBUG_INTR, ("through]"));
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

	DPRINTF(DEBUG_INTR, ("\n"));

	return ih;
}

void
vrgiu_intr_disestablish(hc, arg)
	hpcio_chip_t hc;
	void *arg;
{
	struct vrgiu_intr_entry *ihe = arg;
	struct vrgiu_softc *sc = hc->hc_sc;
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

/* Clear interrupt */
void
vrgiu_intr_clear(hc, arg)
	hpcio_chip_t hc;
	void *arg;
{
	struct vrgiu_softc *sc = hc->hc_sc;
	struct vrgiu_intr_entry *ihe = arg;
	u_int32_t reg;

	reg = vrgiu_regread_4(sc, GIUINTSTAT_REG);
	vrgiu_regwrite_4(sc, GIUINTSTAT_REG, reg & ~(1 << ihe->ih_port));
}

static void
vrgiu_register_iochip(hc, iochip)
	hpcio_chip_t hc;
	hpcio_chip_t iochip;
{
	struct vrgiu_softc *sc = hc->hc_sc;

	vrip_gpio_register(sc->sc_vc, iochip);
}

/* interrupt handler */
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
	int ledvalue = CONFIG_HOOK_LED_FLASH;

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

	if (vrgiu_intr_led) 
		config_hook_call(CONFIG_HOOK_SET,
				 CONFIG_HOOK_LED,
				(void *)&ledvalue);
	return 0;
}
