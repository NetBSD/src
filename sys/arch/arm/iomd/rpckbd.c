/*	$NetBSD: rpckbd.c,v 1.1.4.3 2002/04/01 07:39:11 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 *
 *
 * rpckbd.c
 *
 * Keyboard driver functions
 * Loosly based on the origional `pckbd.c' by Charles M. Hannum.
 *
 * Created      : 04/03/01
 */

#include "opt_ddb.h"
#include "opt_pmap_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include <machine/bus.h>
#include <machine/conf.h>

#include "opt_pckbd_layout.h"
#include "opt_wsdisplay_compat.h"

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <arm/iomd/rpckbdvar.h>
#include <arm/iomd/wskbdmap_mfii.h>
#include <dev/cons.h>

#include "beep.h"

/* Keyboard commands */

#define	KBC_RESET	0xFF	/* reset the keyboard */
#define	KBC_RESEND	0xFE	/* request the keyboard resend the last byte */
#define KBC_SET_TMB	0xFA	/* What is this one ? */
#define	KBC_SETDEFAULT	0xF6	/* resets keyboard to its power-on defaults */
#define	KBC_DISABLE	0xF5	/* as per KBC_SETDEFAULT, but also disable key scanning */
#define	KBC_ENABLE	0xF4	/* enable key scanning */
#define	KBC_TYPEMATIC	0xF3	/* set typematic rate and delay */
#define KBD_READID	0xF2	/* Read keyboard ID */
#define	KBC_SETSCANTBL	0xF0	/* set scancode translation table */
#define	KBC_SETLEDS	0xED	/* set mode indicators (i.e. LEDs) */
#define	KBC_ECHO	0xEE	/* request an echo from the keyboard */

#define KBD_SETSCAN_2	0x02
#define KBD_SETSCAN_3	0x03

/* keyboard responses */
#define	KBR_EXTENDED0	0xE0	/* extended key sequence */
#define KBR_EXTENDED1	0xE1	/* extended key sequence */
#define	KBR_RESEND	0xFE	/* needs resend of command */
#define	KBR_ACK		0xFA	/* received a valid command */
#define	KBR_OVERRUN	0x00	/* flooded */
#define	KBR_FAILURE	0xFD	/* diagnosic failure */
#define	KBR_BREAK	0xF0	/* break code prefix - sent on key release */
#define	KBR_RSTDONE	0xAA	/* reset complete */
#define	KBR_ECHO	0xEE	/* echo response */

#define	KBD_DATA	0
#define	KBD_CR		1
#define	KBD_STATUS	1

#define KBD_CR_ENABLE	0x08
#define KBD_CR_KDATAO	0x02
#define KBD_CR_KCLKO	0x01

#define KBD_ST_TXE	0x80
#define KBD_ST_TXB	0x40
#define KBD_ST_RXF	0x20
#define KBD_ST_RXB	0x10
#define KBD_ST_ENABLE	0x08
#define KBD_ST_RXPARITY	0x04
#define KBD_ST_KDATAI	0x02
#define KBD_ST_KCLKI	0x01

/* Flags used to decode the raw keys */

#define FLAG_KEYUP        0x01
#define FLAG_E0           0x02
#define FLAG_E1           0x04
#define FLAG_BREAKPRELUDE 0x08


/* Declaration of datatypes and their associated function pointers */
int	rpckbd_enable	__P((void *, int));
void	rpckbd_set_leds	__P((void *, int));
int	rpckbd_ioctl	__P((void *, u_long, caddr_t, int, struct proc *));
int	sysbeep		__P((int, int));


const struct wskbd_accessops rpckbd_accessops = {
	rpckbd_enable,
	rpckbd_set_leds,
	rpckbd_ioctl,
};


void	rpckbd_cngetc	__P((void *, u_int *, int *));
void	rpckbd_cnpollc	__P((void *, int));
void	rpckbd_cnbell	__P((void *, u_int, u_int, u_int));

const struct wskbd_consops rpckbd_consops = {
	rpckbd_cngetc,
	rpckbd_cnpollc,
	rpckbd_cnbell,
};


const struct wskbd_mapdata rpckbd_keymapdata = {
	rpckbd_keydesctab,
	KB_UK,
};


/* Forward declaration of functions */
static int	kbdcmd 			__P((struct rpckbd_softc *sc, u_char cmd, int eat_ack));
static void	kbd_flush_input		__P((struct rpckbd_softc *sc));
static int	rpckbd_decode		__P((struct rpckbd_softc *id, int datain, u_int *type, int *dataout));
static int	rpckbd_led_encode	__P((int));
static int	rpckbd_led_decode	__P((int));
static void	rpckbd_bell		__P((int, int, int));


struct rpckbd_softc console_kbd;


/* modelled after the origional pckbd device code */
int
rpckbd_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct rpckbd_softc *sc = (struct rpckbd_softc *)v;
	int res, new_ledstate;

	switch (cmd) {
	    case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_RISCPC;
		return 0;
	    case WSKBDIO_SETLEDS:
		/* same as rpckbd_set_leds */
	
		/* check if we're allready in this state */
		new_ledstate = rpckbd_led_encode(*(int *)data);
		if (new_ledstate == sc->sc_ledstate)
			return (0);

		sc->sc_ledstate = new_ledstate;
		res = kbdcmd(sc, KBC_SETLEDS, 0);
		res = kbdcmd(sc, sc->sc_ledstate, 0);
		if (res == KBR_ACK)
			return (0);
		return (EIO);
 	   case WSKBDIO_GETLEDS:
		*(int *)data = rpckbd_led_decode(sc->sc_ledstate);
		return (0);
	   case WSKBDIO_COMPLEXBELL:
#define d ((struct wskbd_bell_data *)data)
		/*
		 * Keyboard can't beep directly; we have an
		 * externally-provided global hook to do this.
		 */
		rpckbd_bell(d->pitch, d->period, d->volume);
#undef d
		return (0);
#ifdef WSDISPLAY_COMPAT_RAWKBD
	    case WSKBDIO_SETMODE:
		sc->rawkbd = (*(int *)data == WSKBD_RAW);
		return (0);
#endif
	}
	return EPASSTHROUGH;
}



/* modelled after the origional pckbd device code */
int
rpckbd_enable(void *v, int on)
{
	struct rpckbd_softc *sc = v;
	int res;

	if (on) {
		if (sc->sc_enabled)
			return (EBUSY);
		res = kbdcmd(sc, KBC_ENABLE, 0);
		if (res != KBR_ACK) {
			printf("rpckbd_enable: command error; got response %d\n", res);
			return (EIO);
		};
		sc->sc_enabled = 1;
	} else {
		if (sc->t_isconsole)
			return (EBUSY);
		res = kbdcmd(sc, KBC_DISABLE, 0);
		if (res != KBR_ACK) {
			printf("rpckbd_disable: command error; got response %d\n", res);
			return (EIO);
		};
		sc->sc_enabled = 0;
	};
	return (0);
}


/*
 * kbdreset()
 * Modelled after kbd.c
 *
 * Resets the keyboard. 
 * Returns 0 if successful.
 * Returns 1 if keyboard could not be enabled.
 * Returns 2 if keyboard could not be reset. 
 */
int
rpckbd_reset(struct rpckbd_softc *sc)
{
	int i;
	u_char c;

	/* flush garbage */
	kbd_flush_input(sc);

	/* reset the keyboard ; more elaborate than the pcwskbd ! */

	/* Disable, wait then enable the keyboard interface */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, KBD_CR, 0x00);
	delay(100);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, KBD_CR, KBD_CR_ENABLE);

	/* Wait for kdata and kclk to go go high */
	for (i = 1000; i; i--) {
		if ((bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_STATUS)
		    & (KBD_ST_KDATAI | KBD_ST_KCLKI))
		    == (KBD_ST_KDATAI | KBD_ST_KCLKI))
			break;
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, KBD_CR,
		    (KBD_CR_KDATAO | KBD_CR_KCLKO));
		delay(200);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, KBD_CR, KBD_CR_ENABLE);
	};
	if (i == 0 || (bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_STATUS)
	    & KBD_ST_ENABLE) == 0)
		return(1);

	kbd_flush_input(sc);
	kbdcmd(sc, KBC_DISABLE, 0);

retry:
	kbdcmd(sc, KBC_RESET, 1);
	delay(500000);
	for (i = 100; i; i--) {
		c = bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_DATA);
		if (c == KBR_RSTDONE)
			break;
		delay(100000);
		if (c == KBR_RESEND) {
			printf(" [retry]");
			goto retry;
		}
	};
	if (i == 0)
		return(2);

	kbdcmd(sc, KBC_SETSCANTBL, 0);
	kbdcmd(sc, KBD_SETSCAN_2, 0);
	kbdcmd(sc, KBC_ENABLE, 0);

	return (0);

#if 0
	res = kbdcmd(sc, KBC_RESET, 1);
	if (res != KBR_RSTDONE) {
		printf(" : reset error, got 0x%02x ", res);
		return 1;
	};
	return 0;	/* flag success */
#endif
}


void
rpckbd_set_leds(void *context, int leds)    
{
	struct rpckbd_softc *sc = (struct rpckbd_softc *) context;
	int res, new_ledstate;


	/* check if we're allready in this state */
	new_ledstate = rpckbd_led_encode(leds);
	if (new_ledstate == sc->sc_ledstate)
		return;

	/* set state */
	sc->sc_ledstate = new_ledstate;;
        kbdcmd(sc, KBC_SETLEDS, 0);
        res = kbdcmd(sc, sc->sc_ledstate, 0);

	/* could fail .... */
	if (res != KBR_ACK) printf("kbd_set_leds: kbderr %d", res);
}


int
rpckbd_intr(void *context)
{
	struct rpckbd_softc *sc = (struct rpckbd_softc *) context;
        int data, key, type;

	/* read the key code */
        data = bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_DATA);

	if (data == 0) return (1);

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (sc->rawkbd) {
		wskbd_rawinput(sc->sc_wskbddev, (u_char *) &data, 1);
		return (1);	/* claim interrupt */
	}
#endif
	if (rpckbd_decode(sc, data, &type, &key))
		wskbd_input(sc->sc_wskbddev, type, key);

	return (1);	/* claim interrupt */
}


/* should really be renamed to attach using the normal attachment stuff */
int
rpckbd_init(struct device *self, int isconsole, vaddr_t data_port, vaddr_t cmd_port)
{
	struct rpckbd_softc *sc = (struct rpckbd_softc *) self;
	struct wskbddev_attach_args a;
	int return_code;

	/* init keyboard   */
	sc->data_port = data_port;
	sc->cmd_port  = cmd_port;

	/* init attachment stuff */
	sc->sc_enabled = 1;

	/* reset keyboard */
	return_code = rpckbd_reset(sc);

	printf("\n");

	/* attach to wskbd */

	a.console= isconsole;
	a.keymap = &rpckbd_keymapdata;
	a.accessops = &rpckbd_accessops;
	a.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);

	return return_code;
}


int
rpckbd_cnattach(struct device *self)
{
	struct rpckbd_softc *sc = (struct rpckbd_softc *) self;

	/* attach to wskbd ; the 2nd. arg. is a information pointer passed on to us */
	wskbd_cnattach(&rpckbd_consops, sc, &rpckbd_keymapdata);

	return (0);	/* flag success */
}


void
rpckbd_cngetc(void *v, u_int *type, int *data)
{
	struct rpckbd_softc *sc = v;
	int val;

	for (;;) {
		/* wait for a receive event */
		while ((bus_space_read_1(console_kbd.sc_iot, console_kbd.sc_ioh, KBD_STATUS) & KBD_ST_RXF) == 0) ;
		delay(10);

		val = bus_space_read_1(console_kbd.sc_iot, console_kbd.sc_ioh, KBD_DATA);
		if (rpckbd_decode(sc, val, type, data))
			return;
	}
}


void
rpckbd_cnpollc(void *v, int on)
{
	/* switched on/off polling ... what to do ? */
}


void
rpckbd_cnbell(void *v, u_int pitch, u_int period, u_int volume)
{
	/* dunno yet */
};


void
rpckbd_bell(int pitch, int period, int volume)
{
	/* dunno yet */
#if NBEEP>0
	sysbeep(pitch, period);
#endif
};


/* Code derived from the standard pckbd_decode driver */
static int
rpckbd_decode(struct rpckbd_softc *id, int datain, u_int *type, int *dataout)
{
	int key;

#if 0
	printf(" +%02x", datain);
#endif

	/* mark keyup flag */
	if (datain == 0xf0) {
		id->t_flags |= FLAG_KEYUP;
		return (0);
	};

	/* special code -> ignore */
	if ((datain == 0xff) || (datain == 0x00)) {
		id->t_flags = 0;
		return (0);
	};

	/* just note resend ... */
	if (datain == KBR_RESEND) {
/*		printf("rpckbd:resend\n"); */
		id->t_resend = 1;
		return (0);
	};

	/* just note ack code */
	if (datain == KBR_ACK) {
/*		printf("rpckbd:ack\n"); */
		id->t_ack = 1;
		return (0);
	};

	if (datain == KBR_EXTENDED0) {
		id->t_flags |= FLAG_E0;
		return(0);
	} else if (datain == KBR_EXTENDED1) {
		id->t_flags |= FLAG_E1;
		return(0);
	}

	key = datain;

	/* map extended keys to (unused) codes 256 and higher */
	if (id->t_flags & FLAG_E0) {
		id->t_flags &= ~FLAG_E0;
		if (datain == 0x12) {
			id->t_flags &= ~FLAG_KEYUP;
			return (0);
		};
		key |= 0x100;
	};

	/*              
	 * process BREAK key (EXT1 0x14 0x77):
	 * map to (unused) code 7F
	 */
	if ((id->t_flags & FLAG_E1) && (datain == 0x14)) {
		id->t_flags |= FLAG_BREAKPRELUDE;
		return(0);
	} else if ((id->t_flags & FLAG_BREAKPRELUDE) &&
		   (datain == 0x77)) {
		id->t_flags &= ~(FLAG_E1 | FLAG_BREAKPRELUDE);
		key = 0x7f;
	} else if (id->t_flags & FLAG_E1) {
		id->t_flags &= ~FLAG_E1;
	}


	if (id->t_flags & FLAG_KEYUP) {
		id->t_flags &= ~FLAG_KEYUP;
		id->t_lastchar = 0;
		*type = WSCONS_EVENT_KEY_UP;
#if 0
		printf(" [%d, UP]", key);
#endif
	} else {
		/* Always ignore typematic keys */
		if (key == id->t_lastchar)
			return(0);
		id->t_lastchar = key;
		*type = WSCONS_EVENT_KEY_DOWN;
#if 0
		printf(" [%d, DOWN]", key);
#endif
	}
	*dataout = key;
	return(1);
}


/* return wscons keyboard leds code for this kbd led byte */
static int
rpckbd_led_decode(int code)
{
	int scroll, num, caps;

	scroll = (code & 1) ? 1:0;
	num    = (code & 2) ? 1:0;
	caps   = (code & 4) ? 1:0;

	return (scroll*WSKBD_LED_SCROLL) | (num*WSKBD_LED_NUM) | (caps*WSKBD_LED_CAPS);
};


/* return kbd led byte for this wscons keyboard leds code */
static int
rpckbd_led_encode(int code)
{
	int scroll, num, caps;

	scroll = (code & WSKBD_LED_SCROLL) ? 1:0;
	num    = (code & WSKBD_LED_NUM)    ? 1:0;
	caps   = (code & WSKBD_LED_CAPS)   ? 1:0;
	/* cant do compose :( */

	return (caps<<2) | (num<<1) | (scroll<<0);
}


static void
kbd_flush_input(struct rpckbd_softc *sc)
{
	int i;

	/* Loop round reading bytes while the receive buffer is not empty */

	for (i = 0; i < 20; i++) {
		if ((bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_STATUS) & KBD_ST_RXF) == 0)
			break;
		delay(100);
		(void)bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_DATA);
	}
}


/* XXX fix me ... what to return */
static int
kbdcmd(struct rpckbd_softc *sc, u_char cmd, int eat_acks)
{
	u_char c;
	int i = 0;
	int retry;

	for (retry = 7; retry >= 0; --retry) {

		/* Wait for empty kbd transmit register */
		 
		for (i = 1000; i; i--) {
			if (bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_STATUS) & KBD_ST_TXE)
				break;
			delay(200);
		}
		if (i == 0)
			printf("%s: transmit not ready\n", sc->sc_device.dv_xname);

		bus_space_write_1(sc->sc_iot, sc->sc_ioh, KBD_DATA, cmd);

		do {	
			delay(200);
			/* Wait for full kbd receive register */

			for (i = 0; i < 1000; i++) {
				c = bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_STATUS);
				if ((c & KBD_ST_RXF) == KBD_ST_RXF)
					break;
				delay(100);
			}

			delay(100);

			/* Get byte from kbd receive register */
			c = bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_DATA);
			if ((c == KBR_ECHO) || (c == KBR_RSTDONE))
				return (c);
		} while ((c==KBR_ACK) && eat_acks);
		if (c == KBR_ACK)
			return (c);

		/* Failed if we have more reties to go flush kbd */

		if (retry) {
			kbd_flush_input(sc);
			printf(" [retry] ");
		};
	}
	printf("%s: command failed, cmd = %02x, status = %02x\n", sc->sc_device.dv_xname, cmd, c);
	return (1);
}

/* end of rpckbd.c */

