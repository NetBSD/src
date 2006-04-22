/* $NetBSD: smdk2410_kbd.c,v 1.2.6.1 2006/04/22 11:37:24 simonb Exp $ */

/*
 * Copyright (c) 2004  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Support SMDK2410's keyboard.
 *
 * On-board keyboard controller is Semtech SPICoder SA01.
 * (http://www.semtech.com/pdf/doc5-spi-sa01-ds.pdf)
 *
 * The controller is connected to SPI1.
 * _ATN signal from the SPICoder is connected to EINT1.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: smdk2410_kbd.c,v 1.2.6.1 2006/04/22 11:37:24 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <arm/s3c2xx0/s3c24x0var.h>
#include <arm/s3c2xx0/s3c24x0reg.h>
#include <arm/s3c2xx0/s3c2410reg.h>

#include <arm/s3c2xx0/s3c24x0_spi.h>

#include "locators.h"

/*
 * Keyboard driver for Semtech keyboard controller on SMDK2410.
 *
 * There are several keycoder products from Semtech.
 * This driver supports SPICoder(R) SA01 (UR5HCSPI-SA01) only.
 *
 * See http://www.semtech.com/products/ for detail.
 */

/*
 * Commands/responce 
 */
#define	KCDR_INITIALIZE	0xa0	/* Initialize request */
#define	KCDR_INITCOMP	0xa1	/* Initialize complete */
#define	KCDR_HEARTBEAT	0xa2	/* Heaartbeat request/response */
#define	KCDR_IDENTIFY	0xf2	/* Identification request/response */
#define	KCDR_LEDSTATUS	0xa3	/* LED status request/report */
#define	KCDR_LEDMODIFY	0xa6	/* LED mode modify */
#define	KCDR_RESENTREQ	0xa5	/* Re-send request upon error */
#define	KCDR_IOMODE	0xa7	/* Input/output mode modify/report */
#define	KCDR_OUTPUT	0xa8	/* output to GPIO0 pin */
#define	KCDR_SETWAKEUP	0xa9	/* define wake-up keys */

#define	KCDR_CONTROL	0x80	/* Commands from KeyCorder to Host starts with
				   this code. */
#define	KCDR_ESC	0x1b	/* Commands from host to KeyCorder starts with
				   this code. */

/*
 * GPIO ports
 */
#define	SSKBD_WUP	0	/* nWUP = GPB0 */
#define	SSKBD_SS	6	/* nSS = GPB6 */

/*
 * keymap
 */

#define _(col,row)	KS_KEYCODE((col)*8+(row))

static const keysym_t sskbd_keydesc_0[] = {
/*	_(col,row)      normal		shifted */
	_(0,0), 	KS_Alt_L,	KS_Alt_L,

	_(1,0), 	KS_grave,	KS_asciitilde,
	_(1,1),  	KS_backslash,	KS_bar,
	_(1,2), 	KS_Tab,  	KS_Tab,
	_(1,3),  	KS_z,    	KS_Z,
	_(1,4),  	KS_a,  		KS_A,
	_(1,5), 	KS_x,		KS_X,

	_(2,1), 	KS_Shift_L,	KS_Shift_L,

	_(3,0), 	KS_Control_L,	KS_Control_L,

	_(4,0), 	KS_Meta_L,	KS_Meta_L,

	_(5,0), 	KS_Escape, 	KS_Escape,
	_(5,1),  	KS_Delete,	KS_Delete,
	_(5,2), 	KS_q,		KS_Q,
	_(5,3), 	KS_Caps_Lock,	KS_Caps_Lock,
	_(5,4),  	KS_s,		KS_S,
	_(5,5),  	KS_c,		KS_C,
	_(5,6), 	KS_3,		KS_numbersign,

	_(6,0), 	KS_1,		KS_exclam,
	_(6,2),		KS_w,		KS_W,
	_(6,4),		KS_d,		KS_D,
	_(6,5),		KS_v,		KS_V,
	_(6,6),		KS_4,		KS_dollar,

	_(7,0),		KS_2,		KS_at,
	_(7,1),		KS_t,		KS_T,
	_(7,2),		KS_e,		KS_E,
	_(7,4),		KS_f,		KS_F,
	_(7,5),		KS_b,		KS_B,
	_(7,6),		KS_5,		KS_percent,

	_(8,0),		KS_9,		KS_parenleft,
	_(8,1),		KS_y,		KS_Y,
	_(8,2),		KS_r,		KS_R,
	_(8,3),		KS_k,		KS_K,
	_(8,4),		KS_g,		KS_G,
	_(8,5),		KS_n,		KS_N,
	_(8,6),		KS_6,		KS_asciicircum,

	_(9,0),		KS_0,		KS_parenright,
	_(9,1),		KS_u,		KS_U,
	_(9,2),		KS_o,		KS_O,
	_(9,3),		KS_l,		KS_L,
	_(9,4),		KS_h,		KS_H,
	_(9,5),		KS_m,		KS_M,
	_(9,6),		KS_7,		KS_ampersand,

	_(10,0),	KS_minus,	KS_underscore,
	_(10,1),	KS_i,		KS_I,
	_(10,2),	KS_p,		KS_P,
	_(10,3),	KS_l,		KS_L,
	_(10,4),	KS_j,		KS_J,
	_(10,5),	KS_comma,	KS_less,
	_(10,6),	KS_8,		KS_asterisk,

	_(11,0),	KS_equal,	KS_plus,
	_(11,1),	KS_Return,	KS_Return,
	_(11,2),	KS_bracketleft,	KS_braceleft,
	_(11,3),	KS_apostrophe,	KS_quotedbl,
	_(11,4),	KS_slash,	KS_question,
	_(11,5),	KS_period,	KS_greater,
	_(11,6),	KS_Menu,	KS_Menu,	/* Prog key */

	_(12,1),	KS_Shift_R,	KS_Shift_R,

	_(13,0),	KS_BackSpace,	KS_BackSpace,
	_(13,1),	KS_Down,	KS_Next,
	_(13,2),	KS_bracketright,  KS_braceright,
	_(13,3),	KS_Up,		KS_Prior,
	_(13,4),	KS_Left,	KS_Home,
	_(13,5),	KS_space,	KS_space,
	_(13,6),	KS_Right,	KS_End,
};

#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }

static const struct wscons_keydesc sskbd_keydesctab[] = {
	KBD_MAP(KB_MACHDEP, 0, sskbd_keydesc_0),
	{0, 0, 0, 0}
};

const struct wskbd_mapdata sskbd_keymapdata = {
	sskbd_keydesctab,
	KB_MACHDEP,
};


/*
 * SMDK2410 keyboard driver.
 */
struct sskbd_softc {
        struct  device dev;

	struct device *wskbddev;
	void *atn_ih;			/* interrupt handler for nATN */
	void *spi_ih;			/* interrupt handler for SPI rx */

	void *soft_ih;			/* soft interrupt */

	bus_space_tag_t  iot;
	bus_space_handle_t ioh;
	bus_space_handle_t gpioh;

#define RING_SIZE  16			/* must be power of 2 */
	short  inptr, outptr;
	unsigned char ring[RING_SIZE];
#define	advance_ring_ptr(p)	((p+1) & ~RING_SIZE)

	short reading, enable;
};


int sskbd_match(struct device *, struct cfdata *, void *);
void sskbd_attach(struct device *, struct device *, void *);

CFATTACH_DECL(sskbd, sizeof(struct sskbd_softc),
    sskbd_match, sskbd_attach, NULL, NULL);

static  int	sskbd_enable(void *, int);
static  void	sskbd_set_leds(void *, int);
static  int	sskbd_ioctl(void *, u_long, caddr_t, int, struct lwp *);
static	int	sskbd_atn_intr(void *);
static	int	sskbd_spi_intr(void *);
static	void	sskbd_soft_intr(void *);

const struct wskbd_accessops sskbd_accessops = {
	sskbd_enable,
	sskbd_set_leds,
	sskbd_ioctl,
};

#if 0
void	sskbd_cngetc(void *, u_int *, int *);
void	sskbd_cnpollc(void *, int);
void	sskbd_cnbell(void *, u_int, u_int, u_int);

const struct wskbd_consops sskbd_consops = {
	sskbd_cngetc,
	sskbd_cnpollc,
	sskbd_cnbell,
};
#endif

int
sskbd_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;
}

void
sskbd_attach(struct device *parent, struct device *self, void *aux)
{
	struct sskbd_softc *sc = (void *)self;
	struct ssspi_attach_args *spia = aux;
	uint32_t reg;
	bus_space_handle_t gpioh;
	bus_space_tag_t iot;
	struct wskbddev_attach_args a;

	aprint_normal("\n");

	sc->iot = iot = spia->spia_iot;
	sc->ioh = spia->spia_ioh;
	sc->gpioh = gpioh = spia->spia_gpioh;

	/* enable pullup register for MISO */
	reg = bus_space_read_2(iot, gpioh, GPIO_PGUP);
	bus_space_write_2(iot, gpioh, GPIO_PGUP, reg & ~(1<<5));

	/* nSS and wakeup */
	bus_space_write_2(iot, gpioh, GPIO_PBDAT,
			  (1<<SSKBD_SS) | (1<<SSKBD_WUP) |
			  bus_space_read_2(iot, gpioh, GPIO_PBDAT));
	reg = bus_space_read_4(iot, gpioh, GPIO_PBCON);
	reg = GPIO_SET_FUNC(reg, SSKBD_WUP, PCON_OUTPUT);
	reg = GPIO_SET_FUNC(reg, SSKBD_SS, PCON_OUTPUT);
	bus_space_write_4(iot, gpioh, GPIO_PBCON, reg);

	/* nATN input to EINT1 */
	reg = bus_space_read_4(iot, gpioh, GPIO_PFCON);
	reg = GPIO_SET_FUNC(reg, 1, PCON_ALTFUN);
	bus_space_write_4(iot, gpioh, GPIO_PFCON, reg);

#if 0 	/*  Controller doesn't seem to respond to this. */

	/* wakeup pulse */
	reg = bus_space_read_4(iot, gpioh, GPIO_PBDAT);
	reg &= ~(1<<SSKBD_WUP);
	bus_space_write_4(iot, gpioh, GPIO_PBDAT, reg);
	delay(100);
	reg |= (1<<SSKBD_WUP);
	bus_space_write_4(iot, gpioh, GPIO_PBDAT, reg);

	delay(1000);

	/* Send initialize command. */
	sskbd_send(sc, KCDR_ESC);
	sskbd_send(sc, KCDR_INITIALIZE);
	sskbd_send(sc, 0x7b);
#endif

	sc->inptr = sc->outptr = 0;
	sc->reading = sc->enable = 0;

	sc->atn_ih = s3c24x0_intr_establish(spia->spia_aux_intr, IPL_TTY,
	    IST_EDGE_FALLING, sskbd_atn_intr, sc);

	sc->spi_ih = s3c24x0_intr_establish(spia->spia_intr, IPL_SERIAL,
	    0, sskbd_spi_intr, sc);

	sc->soft_ih = softintr_establish(IPL_SOFTSERIAL, sskbd_soft_intr,
	    sc);

	if (sc->atn_ih == NULL || sc->spi_ih == NULL)
		aprint_error("%s: can't establish interrupt handler\n",
		    sc->dev.dv_xname);

	/* setup SPI control register, and prescaler */
	s3c24x0_spi_setup((struct ssspi_softc *)device_parent(self), 
			  SPCON_SMOD_INT | SPCON_ENSCK | 
			  SPCON_MSTR | SPCON_IDLELOW_RISING,
			  100*1000, 0);


	/* Attach the wskbd. */
	a.console = 0;
	a.keymap = &sskbd_keymapdata;
	a.accessops = &sskbd_accessops;
	a.accesscookie = sc;

	sc->wskbddev = config_found(self, &a, wskbddevprint);
}


/*
 * Interrupt handler for nATN signal.
 */
static int
sskbd_atn_intr(void *arg)
{
	struct sskbd_softc *sc = arg;
	int s;
	uint32_t reg;

	/* make sure SPI transmitter is ready */
	if (!(bus_space_read_1(sc->iot, sc->ioh, SPI_SPSTA) & SPSTA_REDY))
		return 1;


	if (advance_ring_ptr(sc->inptr) == sc->outptr) {
		/* ring buffer is full. ignore this nATN signale */
		softintr_schedule(sc->soft_ih);
		return 1;
	}

	/* nSS = L */
	s = splserial();
	sc->reading = 1;
	reg = bus_space_read_2(sc->iot, sc->gpioh, GPIO_PBDAT);
	bus_space_write_2(sc->iot, sc->gpioh, GPIO_PBDAT,
	    reg & ~(1<<SSKBD_SS));
	
	/* generate clock to receive data from the controller */
	bus_space_write_1(sc->iot, sc->ioh, SPI_SPTDAT, 0xff);

	splx(s);

	return 1;
}

/*
 * Interrupt handler for SPI rx
 */
static int
sskbd_spi_intr(void *arg)
{
	struct sskbd_softc *sc = arg;
	int data;
	uint32_t reg;

	if (sc->reading == 0)
		return 1;	/* Ignore garbate input. */

	sc->reading = 0;

	data = bus_space_read_1(sc->iot, sc->ioh, SPI_SPRDAT);

	/* nSS = H */
	reg = bus_space_read_2(sc->iot, sc->gpioh, GPIO_PBDAT);
	bus_space_write_2(sc->iot, sc->gpioh, GPIO_PBDAT, 
	    reg | (1<<SSKBD_SS));

	if (sc->enable) {
		sc->ring[sc->inptr] = data;
		sc->inptr = advance_ring_ptr(sc->inptr);

		softintr_schedule(sc->soft_ih);
	}
#ifdef KBD_DEBUG
	else {
		printf("discard %x\n", data);
	}
#endif

	return 1;
}

static void
sskbd_soft_intr(void *arg)
{
	struct sskbd_softc *sc = arg;
	int key, up;

	while (sc->outptr != sc->inptr) {
		key = sc->ring[sc->outptr];
		sc->outptr = advance_ring_ptr(sc->outptr);

		up = key & 0x80;
		key &= ~0x80;

		key -= 1;
		if (key < 0 || 8*14 < key)
			continue;

#ifdef KBD_DEBUG
		printf("key %d %s\n", key, up ? "up" : "down");
#endif
		wskbd_input(sc->wskbddev, 
		    up ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN,
		    key);
	}
}

static int
sskbd_enable(void *v, int on)
{
	struct sskbd_softc *sc = v;

#ifdef KBD_DEBUG
	printf("%s: enable\n", sc->dev.dv_xname);
#endif

#if 0
	if (!on && isconsole(sc))
		return EBUSY;
#endif

	sc->enable = on;
	return (0);
}


static void
sskbd_set_leds(void *v, int leds)
{
}

static int
sskbd_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	/*struct sskbd_softc *sc = v;*/

	switch (cmd) {
	    case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_HPC_KBD; /* XXX */
		return 0;
	    case WSKBDIO_COMPLEXBELL:
#ifdef notyet
#define d ((struct wskbd_bell_data *)data)
		/*
		 * Keyboard can't beep directly; we have an
		 * externally-provided global hook to do this.
		 */
		sskbd_bell(d->pitch, d->period, d->volume, 0);
#undef d
#endif
		return (0);
#ifdef WSDISPLAY_COMPAT_RAWKBD
	    case WSKBDIO_SETMODE:
		sc->rawkbd = (*(int *)data == WSKBD_RAW);
		return (0);
#endif

#if 0
	    case WSKBDIO_SETLEDS:
	    case WSKBDIO_GETLEDS:
		    /* no LED support */
#endif
	}
	return EPASSTHROUGH;
}
