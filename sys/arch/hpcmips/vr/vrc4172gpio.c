/*	$NetBSD: vrc4172gpio.c,v 1.9.16.2 2008/01/21 09:36:40 yamt Exp $	*/
/*-
 * Copyright (c) 2001 TAKEMRUA Shin. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vrc4172gpio.c,v 1.9.16.2 2008/01/21 09:36:40 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/reboot.h>
#include <machine/bus.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/hpc/hpciovar.h>

#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrc4172gpioreg.h>

#include "locators.h"

#define VRC2GPIODEBUG
#ifdef VRC2GPIODEBUG
#define DBG_IO		(1<<0)
#define DBG_INTR	(1<<1)
#define DBG_INFO	(1<<2)
#ifndef VRC2GPIODEBUG_CONF
#define VRC2GPIODEBUG_CONF 0
#endif /* VRC2GPIODEBUG_CONF */
int	vrc4172gpio_debug = VRC2GPIODEBUG_CONF;
#define DBG(flag)		(vrc4172gpio_debug & (flag))
#define	DPRINTF(flag, arg...)	do { \
					if (DBG(flag)) \
						printf(arg); \
				} while (0)
#else
#define DBG(flag)		(0)
#define	DPRINTF(flag, arg...)	do {} while(0)
#endif
#define	VPRINTF(arg...)	do { \
				if (bootverbose) \
					printf(##arg); \
			} while (0)

#define	CHECK_PORT(x)	(0 <= (x) && (x) < VRC2_EXGP_NPORTS)

struct vrc4172gpio_intr_entry {
	int ih_port;
	int (*ih_fun)(void*);
	void *ih_arg;
	TAILQ_ENTRY(vrc4172gpio_intr_entry) ih_link;
};

struct vrc4172gpio_softc {
	struct	device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct hpcio_attach_args sc_args;
	struct hpcio_chip *sc_hc;

	void *sc_intr_handle;
	u_int32_t sc_intr_mask;
	u_int32_t sc_data;
	u_int32_t sc_intr_mode[VRC2_EXGP_NPORTS];
	TAILQ_HEAD(, vrc4172gpio_intr_entry) sc_intr_head[VRC2_EXGP_NPORTS]; 
	struct hpcio_chip sc_iochip;
	struct hpcio_attach_args sc_haa;
};

int vrc4172gpio_match(struct device*, struct cfdata*, void*);
void vrc4172gpio_attach(struct device*, struct device*, void*);
void vrc4172gpio_callback(struct device *self);
int vrc4172gpio_intr(void*);
int vrc4172gpio_print(void*, const char*);

int vrc4172gpio_port_read(hpcio_chip_t, int);
void vrc4172gpio_port_write(hpcio_chip_t, int, int);
void *vrc4172gpio_intr_establish(hpcio_chip_t, int, int, int (*)(void *), void*);
void vrc4172gpio_intr_disestablish(hpcio_chip_t, void*);
void vrc4172gpio_intr_clear(hpcio_chip_t, void*);
void vrc4172gpio_register_iochip(hpcio_chip_t, hpcio_chip_t);
void vrc4172gpio_update(hpcio_chip_t);
void vrc4172gpio_dump(hpcio_chip_t);
void vrc4172gpio_intr_dump(struct vrc4172gpio_softc *, int);
hpcio_chip_t vrc4172gpio_getchip(void*, int);
static void vrc4172gpio_diffport(struct vrc4172gpio_softc *sc);

static u_int16_t read_2(struct vrc4172gpio_softc *, bus_addr_t);
static void write_2(struct vrc4172gpio_softc *, bus_addr_t, u_int16_t);
static u_int32_t read_4(struct vrc4172gpio_softc *, bus_addr_t);
static void write_4(struct vrc4172gpio_softc *, bus_addr_t, u_int32_t);
static void dumpbits(u_int32_t*, int, int, int, const char[2]);

static struct hpcio_chip vrc4172gpio_iochip = {
	.hc_portread =		vrc4172gpio_port_read,
	.hc_portwrite =		vrc4172gpio_port_write,
	.hc_intr_establish =	vrc4172gpio_intr_establish,
	.hc_intr_disestablish =	vrc4172gpio_intr_disestablish,
	.hc_intr_clear =	vrc4172gpio_intr_clear,
	.hc_register_iochip =	vrc4172gpio_register_iochip,
	.hc_update =		vrc4172gpio_update,
	.hc_dump =		vrc4172gpio_dump,
};

static int intlv_regs[] = {
	VRC2_EXGPINTLV0L,
	VRC2_EXGPINTLV0H,
	VRC2_EXGPINTLV1L
};

CFATTACH_DECL(vrc4172gpio, sizeof(struct vrc4172gpio_softc),
    vrc4172gpio_match, vrc4172gpio_attach, NULL, NULL);

/*
 * regster access method
 */
static inline u_int16_t
read_2(struct vrc4172gpio_softc *sc, bus_addr_t off)
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, off);
}

static inline void
write_2(struct vrc4172gpio_softc *sc, bus_addr_t off, u_int16_t data)
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, off, data);
}

static u_int32_t
read_4(struct vrc4172gpio_softc *sc, bus_addr_t off)
{
	u_int16_t reg0, reg1;

	reg0 = read_2(sc, off);
	reg1 = read_2(sc, off + VRC2_EXGP_OFFSET);

	return (reg0|(reg1<<16));
}

static void
write_4(struct vrc4172gpio_softc *sc, bus_addr_t off, u_int32_t data)
{
	write_2(sc, off, data & 0xffff);
	write_2(sc, off + VRC2_EXGP_OFFSET, (data>>16)&0xffff);
}

int
vrc4172gpio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hpcio_attach_args *haa = aux;
	platid_mask_t mask;

	if (strcmp(haa->haa_busname, HPCIO_BUSNAME))
		return (0);
	if (cf->cf_loc[HPCIOIFCF_PLATFORM] == 0)
		return (0);
	mask = PLATID_DEREF(cf->cf_loc[HPCIOIFCF_PLATFORM]);

	return platid_match(&platid, &mask);
}

void
vrc4172gpio_attach(struct device *parent, struct device *self, void *aux)
{
	struct hpcio_attach_args *args = aux;
	struct vrc4172gpio_softc *sc = (void*)self;
	int i, *loc, port, mode;
	u_int32_t regs[6], t0, t1, t2;

	printf("\n");
	loc = device_cfdata(&sc->sc_dev)->cf_loc;

	/*
	 * map bus space
	 */
	sc->sc_iot = args->haa_iot;
	sc->sc_hc = (*args->haa_getchip)(args->haa_sc, loc[HPCIOIFCF_IOCHIP]);
	sc->sc_args = *args; /* structure copy */
	bus_space_map(sc->sc_iot, loc[HPCIOIFCF_ADDR], loc[HPCIOIFCF_SIZE],
		      0 /* no cache */, &sc->sc_ioh);
	if (sc->sc_ioh == 0) {
		printf("%s: can't map bus space\n", sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * dump Windows CE register setting
	 */
	regs[0] = read_4(sc, VRC2_EXGPDATA);
	regs[1] = read_4(sc, VRC2_EXGPDIR);
	regs[2] = read_4(sc, VRC2_EXGPINTEN);
	regs[3] = read_4(sc, VRC2_EXGPINTTYP);
	t0 = read_2(sc, VRC2_EXGPINTLV0L);
	t1 = read_2(sc, VRC2_EXGPINTLV0H);
	t2 = read_2(sc, VRC2_EXGPINTLV1L);
	regs[4] = ((t2&0xff00)<<8) | (t1&0xff00) | ((t0&0xff00)>>8);
	regs[5] = ((t2&0xff)<<16) | ((t1&0xff)<<8) | (t0&0xff);

	if (bootverbose || DBG(DBG_INFO)) {
		/*
		 *  o: output
		 *  i: input (no interrupt)
		 *  H: level sense interrupt (active high)
		 *  L: level sense interrupt (active low)
		 *  B: both edge trigger interrupt
		 *  P: positive edge trigger interrupt
		 *  N: negative edge trigger interrupt
		 */
		printf("      port#:321098765432109876543210\n");
		printf(" EXGPDATA  :");
		dumpbits(&regs[0], 1, 23, 0, "10\n");
		printf("WIN setting:");
		dumpbits(&regs[1], 5, 23, 0,
		    "oooo"	/* dir=1  en=1  typ=1	*/
		    "oooo"	/* dir=1  en=1  typ=0	*/
		    "oooo"	/* dir=1  en=0  typ=1	*/
		    "oooo"	/* dir=1  en=0  typ=0	*/
		    "BBPN"	/* dir=0  en=1  typ=1	*/
		    "HLHL"	/* dir=0  en=1  typ=0	*/
		    "iiii"	/* dir=0  en=0  typ=1	*/
		    "iiii"	/* dir=0  en=0  typ=0	*/
		    );
		printf("\n");
	}
#ifdef VRC2GPIODEBUG
	if (DBG(DBG_INFO)) {
		printf(" EXGPDIR   :");
		dumpbits(&regs[1], 1, 23, 0, "oi\n");

		printf(" EXGPINTEN :");
		dumpbits(&regs[2], 1, 23, 0, "I-\n");

		printf(" EXGPINTTYP:");
		dumpbits(&regs[3], 1, 23, 0, "EL\n");

		printf(" EXPIB     :");
		dumpbits(&regs[4], 1, 23, 0, "10\n");

		printf(" EXPIL     :");
		dumpbits(&regs[5], 1, 23, 0, "10\n");

		printf(" EXGPINTLV :%04x %04x %04x\n", t2, t1, t0);
	}
#endif /*  VRC2GPIODEBUG */

	/*
	 * initialize register and internal data
	 */
	sc->sc_intr_mask = 0;
	write_2(sc, VRC2_EXGPINTEN, sc->sc_intr_mask);
	for (i = 0; i < VRC2_EXGP_NPORTS; i++)
		TAILQ_INIT(&sc->sc_intr_head[i]);
	sc->sc_data = read_4(sc, VRC2_EXGPDATA);
	if (bootverbose || DBG(DBG_INFO)) {
		u_int32_t data;

		sc->sc_intr_mask = (~read_4(sc, VRC2_EXGPDIR) & 0xffffff);
		write_4(sc, VRC2_EXGPINTTYP, 0); /* level sence interrupt */
		data = ~read_4(sc, VRC2_EXGPDATA);
		write_2(sc, VRC2_EXGPINTLV0L, (data >>  0) & 0xff);
		write_2(sc, VRC2_EXGPINTLV0H, (data >>  8) & 0xff);
		write_2(sc, VRC2_EXGPINTLV1L, (data >> 16) & 0xff);
	}

	/*
	 * install interrupt handler
	 */
	port = loc[HPCIOIFCF_PORT];
	mode = HPCIO_INTR_LEVEL | HPCIO_INTR_HIGH;
	sc->sc_intr_handle = 
	    hpcio_intr_establish(sc->sc_hc, port, mode, vrc4172gpio_intr, sc);
	if (sc->sc_intr_handle == NULL) {
		printf("%s: can't establish interrupt\n", sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * fill hpcio_chip structure
	 */
	sc->sc_iochip = vrc4172gpio_iochip; /* structure copy */
	sc->sc_iochip.hc_chipid = VRIP_IOCHIP_VRC4172GPIO;
	sc->sc_iochip.hc_name = sc->sc_dev.dv_xname;
	sc->sc_iochip.hc_sc = sc;
	/* Register functions to upper interface */
	hpcio_register_iochip(sc->sc_hc, &sc->sc_iochip);

	/* 
	 *  hpcio I/F
	 */
	sc->sc_haa.haa_busname = HPCIO_BUSNAME;
	sc->sc_haa.haa_sc = sc;
	sc->sc_haa.haa_getchip = vrc4172gpio_getchip;
	sc->sc_haa.haa_iot = sc->sc_iot;
	while (config_found(self, &sc->sc_haa, vrc4172gpio_print)) ;
	/*
	 * GIU-ISA bridge
	 */
#if 1 /* XXX Sometimes mounting root device failed. Why? XXX*/
	config_defer(self, vrc4172gpio_callback);
#else
	vrc4172gpio_callback(self);
#endif
}

void
vrc4172gpio_callback(struct device *self)
{
	struct vrc4172gpio_softc *sc = (void*)self;

	sc->sc_haa.haa_busname = "vrisab";
	config_found(self, &sc->sc_haa, vrc4172gpio_print);
}

int
vrc4172gpio_print(void *aux, const char *pnp)
{
	if (pnp)
		return (QUIET);
	return (UNCONF);
}

/*
 * PORT 
 */
int
vrc4172gpio_port_read(hpcio_chip_t hc, int port)
{
	struct vrc4172gpio_softc *sc = hc->hc_sc;
	int on;

	if (!CHECK_PORT(port))
		panic("%s: illegal gpio port", __func__);

	on = (read_4(sc, VRC2_EXGPDATA) & (1 << port));

	return (on ? 1 : 0);
}
    
void
vrc4172gpio_port_write(hpcio_chip_t hc, int port, int onoff)
{
	struct vrc4172gpio_softc *sc = hc->hc_sc;
	u_int32_t data;

	if (!CHECK_PORT(port))
		panic("%s: illegal gpio port", __func__);
	data = read_4(sc, VRC2_EXGPDATA);
	if (onoff)
		data |= (1<<port);
	else
		data &= ~(1<<port);
	write_4(sc, VRC2_EXGPDATA, data);
}

void
vrc4172gpio_update(hpcio_chip_t hc)
{
}

void
vrc4172gpio_intr_dump(struct vrc4172gpio_softc *sc, int port)
{
	u_int32_t mask, mask2;
	int intlv_reg;

	mask = (1 << port);
	mask2 = (1 << (port % 8));
	intlv_reg = intlv_regs[port/8];

	if (read_4(sc, VRC2_EXGPDIR) & mask) {
		printf(" output");
		return;
	}
	printf(" input");

	if (read_4(sc, VRC2_EXGPINTTYP) & mask) {
		if (read_4(sc, intlv_reg) & (mask2 << 8)) {
			printf(", both edge");
		} else {
			if (read_4(sc, intlv_reg) & mask2)
				printf(", positive edge");
			else
				printf(", negative edge");
		}
	} else {
		if (read_4(sc, intlv_reg) & mask2)
			printf(", high level");
		else
			printf(", low level");
	}
}

static void
vrc4172gpio_diffport(struct vrc4172gpio_softc *sc)
{
	u_int32_t data;
	data = read_4(sc, VRC2_EXGPDATA);
	if (sc->sc_data != data) {
		printf("      port# 321098765432109876543210\n");
		printf("vrc4172data:");
		dumpbits(&data, 1, 23, 0, "10\n");
		/* bits which changed */
		data = (data & ~sc->sc_data)|(~data & sc->sc_data);
		printf("            ");
		dumpbits(&data, 1, 23, 0, "^ \n");
		sc->sc_data = data;
	}
}

static void
dumpbits(u_int32_t *data, int ndata, int start, int end, const char *sym)
{
	int i, j;

	if (start <= end)
		panic("%s(%d): %s", __FILE__, __LINE__, __func__);

	for (i = start; end <= i; i--) {
		int d = 0;
		for (j = 0; j < ndata; j++)
			d = (d << 1) | ((data[j] & (1 << i)) ? 1 : 0);
		printf("%c", sym[(1 << ndata) - d - 1]);
		
	}
	if (sym[1<<ndata])
		printf("%c", sym[1<<ndata]);
}

void
vrc4172gpio_dump(hpcio_chip_t hc)
{
}

hpcio_chip_t
vrc4172gpio_getchip(void* scx, int chipid)
{
	struct vrc4172gpio_softc *sc = scx;

	return (&sc->sc_iochip);
}

/*
 * Interrupt staff 
 */
void *
vrc4172gpio_intr_establish(
	hpcio_chip_t hc,
	int port, /* GPIO pin # */
	int mode, /* GIU trigger setting */
	int (*ih_fun)(void*),
	void *ih_arg)
{
	struct vrc4172gpio_softc *sc = hc->hc_sc;
	int s;
	u_int32_t reg, mask, mask2;
	struct vrc4172gpio_intr_entry *ih;
	int intlv_reg;

	s = splhigh();

	if (!CHECK_PORT(port))
		panic ("%s: bogus interrupt line", __func__);
	if (sc->sc_intr_mode[port] && mode != sc->sc_intr_mode[port])
		panic ("%s: bogus interrupt type", __func__);
	else
		sc->sc_intr_mode[port] = mode;

	mask = (1 << port);
	mask2 = (1 << (port % 8));
	intlv_reg = intlv_regs[port/8];

	ih = malloc(sizeof(struct vrc4172gpio_intr_entry), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		panic("%s: no memory", __func__);

	ih->ih_port = port;
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	TAILQ_INSERT_TAIL(&sc->sc_intr_head[port], ih, ih_link);

#ifdef VRC2GPIODEBUG
	if (DBG(DBG_INFO)) {
		printf("port %2d:", port);
		vrc4172gpio_intr_dump(sc, port);
		printf("->");
	}
#endif

	/*
	 *  Setup registers
	 */
	/* I/O direction */
	reg = read_4(sc, VRC2_EXGPDIR);
	reg &= ~mask;
	write_4(sc, VRC2_EXGPDIR, reg);

	/* interrupt triger (level/edge) */
	reg = read_4(sc, VRC2_EXGPINTTYP);
	if (mode & HPCIO_INTR_EDGE)
		reg |= mask;	/* edge */
	else
		reg &= ~mask;	/* level */
	write_4(sc, VRC2_EXGPINTTYP, reg);

	/* interrupt trigger option */
	reg = read_4(sc, intlv_reg);
	if (mode & HPCIO_INTR_EDGE) {
		switch (mode & (HPCIO_INTR_POSEDGE | HPCIO_INTR_NEGEDGE)) {
		case HPCIO_INTR_POSEDGE:
			reg &= ~(mask2 << 8);
			reg |= mask2;
			break;
		case HPCIO_INTR_NEGEDGE:
			reg &= ~(mask2 << 8);
			reg &= ~mask2;
			break;
		case HPCIO_INTR_POSEDGE | HPCIO_INTR_NEGEDGE:
		default:
			reg |= (mask2 << 8);
			break;
		}
	} else {
		if (mode & HPCIO_INTR_HIGH)
			reg |= mask2;	/* high */
		else
			reg &= ~mask2;	/* low */
	}
	write_4(sc, intlv_reg, reg);

#ifdef VRC2GPIODEBUG
	if (DBG(DBG_INFO)) {
		vrc4172gpio_intr_dump(sc, port);
		printf("\n");
	}
#endif

	/* XXX, Vrc4172 doesn't have register to set hold or through */

	/*
	 *  clear interrupt status and enable interrupt
	 */
	vrc4172gpio_intr_clear(&sc->sc_iochip, ih);
	sc->sc_intr_mask |= mask;
	write_4(sc, VRC2_EXGPINTEN, sc->sc_intr_mask);

	splx(s);

	DPRINTF(DBG_INFO, "\n");

	return (ih);
}

void
vrc4172gpio_intr_disestablish(hpcio_chip_t hc, void *arg)
{
	struct vrc4172gpio_intr_entry *ihe = arg;
	struct vrc4172gpio_softc *sc = hc->hc_sc;
	int port = ihe->ih_port;
	struct vrc4172gpio_intr_entry *ih;
	int s;

	s = splhigh();
	TAILQ_FOREACH(ih, &sc->sc_intr_head[port], ih_link) {
		if (ih == ihe) {
			TAILQ_REMOVE(&sc->sc_intr_head[port], ih, ih_link);
			free(ih, M_DEVBUF);
			if (TAILQ_EMPTY(&sc->sc_intr_head[port])) {
				/* disable interrupt */
				sc->sc_intr_mask &= ~(1<<port);
				write_4(sc, VRC2_EXGPINTEN,
						    sc->sc_intr_mask);
			}
			splx(s);
			return;
		}
	}
	panic("%s: no such a handle.", __func__);
	/* NOTREACHED */
}

/* Clear interrupt */
void
vrc4172gpio_intr_clear(hpcio_chip_t hc, void *arg)
{
	struct vrc4172gpio_softc *sc = hc->hc_sc;
	struct vrc4172gpio_intr_entry *ihe = arg;

	write_4(sc, VRC2_EXGPINTST, 1 << ihe->ih_port);
	write_4(sc, VRC2_EXGPINTST, 0);
}

void
vrc4172gpio_register_iochip(hpcio_chip_t hc, hpcio_chip_t iochip)
{
	struct vrc4172gpio_softc *sc = hc->hc_sc;

	hpcio_register_iochip(sc->sc_hc, iochip);
}

/* interrupt handler */
int
vrc4172gpio_intr(void *arg)
{
	struct vrc4172gpio_softc *sc = arg;
	int i;
	u_int32_t reg;

	/* dispatch handler */
	reg = read_4(sc, VRC2_EXGPINTST);
	DPRINTF(DBG_INTR, "%s: EXGPINTST=%06x\n", __func__, reg);
	for (i = 0; i < VRC2_EXGP_NPORTS; i++) {
		if (reg & (1 << i)) {
			register struct vrc4172gpio_intr_entry *ih;

			/*
			 * call interrupt handler
			 */
			TAILQ_FOREACH(ih, &sc->sc_intr_head[i], ih_link) {
				ih->ih_fun(ih->ih_arg);
			}

			/*
			 * disable interrupt if no handler is installed
			 */
			if (TAILQ_EMPTY(&sc->sc_intr_head[i])) {
				sc->sc_intr_mask &= ~(1 << i);
				write_2(sc, VRC2_EXGPINTEN, sc->sc_intr_mask);

				/* dump EXGPDATA bits which changed */
				if (bootverbose || DBG(DBG_INFO))
					vrc4172gpio_diffport(sc);
			}
		}
	}

	return (0);
}
