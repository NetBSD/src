/*	$NetBSD: kbd.c,v 1.27 2001/09/16 16:34:28 wiz Exp $	*/

/*
 * Copyright (c) 1994-1997 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * kbd.c
 *
 * Keyboard driver functions
 *
 * Created      : 09/10/94
 */

#include "opt_ddb.h"
#include "opt_pmap_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/tty.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <sys/signalvar.h>
#include <sys/time.h>
#include <sys/poll.h>

#include <machine/bus.h>
#include <machine/kbd.h>
#include <machine/conf.h>
#include <arm32/dev/kbdvar.h>
#include "vt.h"
#include "kbd.h"

/* Special keycodes */

#define KEYCODE_UP     0x100
#define KEYCODE_DOWN   0x101
#define KEYCODE_LEFT   0x102
#define KEYCODE_RIGHT  0x103
#define KEYCODE_PGUP   0x104
#define KEYCODE_PGDN   0x105
#define KEYCODE_INSERT 0x108
#define KEYCODE_DELETE 0x109
#define KEYCODE_HOME   0x10a
#define KEYCODE_END    0x10b

/* Key modifiers flags */

#define MODIFIER_CTRL   0x01
#define MODIFIER_SHIFT  0x02
#define MODIFIER_ALT    0x04
#define MODIFIER_MASK   0x07

#define MODIFIER_CAPS   0x20
#define MODIFIER_NUM    0x10
#define MODIFIER_SCROLL 0x08
#define MODIFIER_LOCK_MASK   0x38

#define MODIFIER_CAPSLOCK 0x40
#define MODIFIER_NORETURN 0x80

/* Keyboard variables */

struct kbd_softc *console_kbd = NULL;

#define RAWKBD_BSIZE 128

static int autorepeatkey = -1;
static struct kbd_autorepeat kbdautorepeat = { 5, 20 };
static int rawkbd_device = 0;
int modifiers = 0;
static int kbd_ack = 0;
static int kbd_resend = 0;
static struct callout autorepeat_ch = CALLOUT_INITIALIZER;
static struct callout autorepeatstart_ch = CALLOUT_INITIALIZER;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

#define KBDUNIT(u)	(minor(u) / 2)
#define KBDFLAG(u)	(minor(u) % 2) 

#define KBDFLAG_RAWUNIT	0
#define KBDFLAG_CONUNIT	1

extern key_struct keys[];
extern key_struct E0keys[];

/* Local function prototypes */

int	kbdreset __P((struct kbd_softc *sc));
void	kbd_flush_input __P((struct kbd_softc *sc));
int	kbdcmd __P((struct kbd_softc *sc, u_char cmd));
void	kbdsetleds __P((struct kbd_softc *sc, int leds));

int PollKeyboard __P((int));
int kbddecodekey __P((struct kbd_softc *, int));
int kbdintr __P((void *arg));

void autorepeatstart __P((void *));
void autorepeat __P((void *));

extern int physconkbd		__P((int key));
extern void console_switch	__P((u_int number));
/*extern int console_switchdown	__P((void));
extern int console_switchup	__P((void));*/
extern int console_unblank	__P((void));
extern int console_scrollback	__P((void));
extern int console_scrollforward	__P((void));
#ifdef PMAP_DEBUG
extern void pmap_debug		__P((int level));
#endif
extern void halt		__P((void));

extern struct cfdriver kbd_cd;

/* keyboard commands */

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
#define	KBR_EXTENDED	0xE0	/* extended key sequence */
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

int
kbdopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct kbd_softc *sc;
	int unit = KBDUNIT(dev);

	if (unit >= kbd_cd.cd_ndevs)
		return(ENXIO);

	sc = kbd_cd.cd_devs[unit];

	if (!sc) return(ENXIO);

	switch (KBDFLAG(dev)) {
	case KBDFLAG_RAWUNIT :
		if (sc->sc_state & RAWKBD_OPEN)
			return(EBUSY);
		sc->sc_state |= RAWKBD_OPEN;
		if (clalloc(&sc->sc_q, RAWKBD_BSIZE, 0) == -1)
			return(ENOMEM);
		sc->sc_proc = p;
		rawkbd_device = 1;
		break;
	case KBDFLAG_CONUNIT :
		if (sc->sc_state & KBD_OPEN)
			return(EBUSY);
		sc->sc_state |= KBD_OPEN;
		break;
	}

/* Kill any active autorepeat */

	callout_stop(&autorepeatstart_ch);
	callout_stop(&autorepeat_ch);

	return(0);
}


int
kbdclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = KBDUNIT(dev);
	struct kbd_softc *sc = kbd_cd.cd_devs[unit];

	switch (KBDFLAG(dev)) {
	case KBDFLAG_RAWUNIT :
		if (!(sc->sc_state & RAWKBD_OPEN))
			return(EINVAL);
		sc->sc_state &= ~RAWKBD_OPEN;
		clfree(&sc->sc_q);
		sc->sc_proc = NULL;
		rawkbd_device = 0;
		break;
	case KBDFLAG_CONUNIT :
		if (!(sc->sc_state & KBD_OPEN))
			return(EINVAL);
		sc->sc_state &= ~KBD_OPEN;
		break;
	}

	return(0);
}


int
kbdread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct kbd_softc *sc = kbd_cd.cd_devs[KBDUNIT(dev)];
	int s;
	int error = 0;
	size_t length;
	u_char buffer[128];

	if (KBDFLAG(dev) == KBDFLAG_CONUNIT)
		return(ENXIO);

	/* Block until keyboard activity occurred. */

	s = spltty();
	while (sc->sc_q.c_cc == 0) {
		if (flag & IO_NDELAY) {
			splx(s);
			return EWOULDBLOCK;
		}
		sc->sc_state |= RAWKBD_ASLEEP;
		if ((error = tsleep((caddr_t)sc, PZERO | PCATCH, "kbdread", 0))) {
			sc->sc_state &= (~RAWKBD_ASLEEP);
			splx(s);
			return error;
		}
	}
	splx(s);

	/* Transfer as many chunks as possible. */

	while (sc->sc_q.c_cc > 0 && uio->uio_resid > 0) {
		length = min(sc->sc_q.c_cc, uio->uio_resid);
		if (length > sizeof(buffer))
			length = sizeof(buffer);

		/* Remove a small chunk from the input queue. */
		(void) q_to_b(&sc->sc_q, buffer, length);

		/* Copy the data to the user process. */
		if ((error = (uiomove(buffer, length, uio))))
			break;
	}
	return error;
}


int
kbdpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct kbd_softc *sc = kbd_cd.cd_devs[KBDUNIT(dev)];
	int revents = 0;
	int s = spltty();

	if (KBDFLAG(dev) == KBDFLAG_CONUNIT) {
		splx(s);
		return(ENXIO);
	}

	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_q.c_cc > 0)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &sc->sc_rsel);
	}

	splx(s);
	return (revents);
}


int
kbdioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct kbd_softc *sc = kbd_cd.cd_devs[KBDUNIT(dev)];
	struct kbd_autorepeat *kbdar = (void *)data;
	int *leds = (int *)data;
	int s;

	switch (cmd) {
	case KBD_GETAUTOREPEAT:
/*		if (KBDFLAG(dev) == KBDFLAG_RAWUNIT)
			return(EINVAL);*/

		*kbdar = kbdautorepeat;
		break;
	case KBD_SETAUTOREPEAT:
/*		if (KBDFLAG(dev) == KBDFLAG_RAWUNIT)
			return(EINVAL);*/
		s = spltty();
		kbdautorepeat = *kbdar;
		if (kbdautorepeat.ka_rate < 1)
			kbdautorepeat.ka_rate = 1;
		if (kbdautorepeat.ka_rate > 50)
			kbdautorepeat.ka_rate = 50;
		if (kbdautorepeat.ka_delay > 50)
			kbdautorepeat.ka_delay = 50;
		if (kbdautorepeat.ka_delay < 1)
			kbdautorepeat.ka_delay = 1;
		(void)splx(s);
		break;
	case KBD_SETLEDS:
		kbdsetleds(sc, *leds);
		break;
	default:
		return(ENXIO);
	}
	return(0);
}


/* Low level keyboard driver functions */

/*
 * kbdcmd
 *
 * Send a command to the keyboard
 * Returns 0 if command succeeded or 1 if it failed.
 */

int
kbdcmd(sc, cmd)
	struct kbd_softc *sc;
	u_char cmd;
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
		if ((c == KBR_ACK) || (c == KBR_ECHO))
			return(0);

		/* Failed if we have more reties to go flush kbd */

		if (retry)
			kbd_flush_input(sc);
	}
	printf("%s: command failed, cmd = %02x, status = %02x\n", sc->sc_device.dv_xname, cmd, c);
	return(1);
}


/*
 * kbd_flush_input()
 *
 * Flushes the keyboard input register
 */

void
kbd_flush_input(sc)
	struct kbd_softc *sc;
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


/*
 * kbdreset()
 *
 * Resets the keyboard.
 * Returns 0 if successful.
 * Returns 1 if keyboard could not be enabled.
 * Returns 2 if keyboard could not be reset.
 */

int
kbdreset(sc)
	struct kbd_softc *sc;
{
	int i;
	u_char c;

	kbd_flush_input(sc);

	/* Disable, wait then enable the keyboard interface */

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, KBD_CR, 0x00);
	delay(100);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, KBD_CR, KBD_CR_ENABLE);

	/* Wait for kdata and kclk to go high */

	for (i = 1000; i; i--) {
		if ((bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_STATUS)
		    & (KBD_ST_KDATAI | KBD_ST_KCLKI))
		    == (KBD_ST_KDATAI | KBD_ST_KCLKI))
			break;
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, KBD_CR,
		    (KBD_CR_KDATAO | KBD_CR_KCLKO));
		delay(200);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, KBD_CR, KBD_CR_ENABLE);
	}
	if (i == 0 || (bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_STATUS)
	    & KBD_ST_ENABLE) == 0)
		return(1);

	kbd_flush_input(sc);
	kbdcmd(sc, KBC_DISABLE);

retry:
	kbdcmd(sc, KBC_RESET);
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
	}
	if (i == 0)
		return(2);

	kbdcmd(sc, KBC_SETSCANTBL);
	kbdcmd(sc, KBD_SETSCAN_2);
	kbdcmd(sc, KBC_ENABLE);

	modifiers = MODIFIER_NUM;
	kbdsetleds(sc, (modifiers >> 3) & 7);
	return(0);
}


void
kbdsetleds(sc, leds)
	struct kbd_softc *sc;
	int leds;
{
	kbdcmd(sc, KBC_SETLEDS);
	kbdcmd(sc, leds);
}

#if 0
void
kbdsetstate(sc, state)
	struct kbd_softc *sc;
	int state;
{
	modifiers = state & MODIFIER_LOCK_MASK;
	kbdsetleds(sc, (modifiers >> 3) & 7);
}


int
kdbgetstate(sc)
	struct kbd_softc *sc;
{
	return(modifiers);
}
#endif
  
int
getkey_polled()
{
	int code;
	int key;
	int up;
	key_struct *ks;
	int s;

	if (console_kbd == NULL) {
		/*
		 * Cannot panic here as that will either call the debugger or
		 * cpu_reboot() which will try and read a key.
		 */
		printf("console_kbd = 0\n");
		halt();
	}
	
	s = splhigh();

	key = 0;

	do {
		while ((bus_space_read_1(console_kbd->sc_iot, console_kbd->sc_ioh, KBD_STATUS) & KBD_ST_RXF) == 0) ;
		delay(10);

		/* Read the IOMD keyboard register and process the key */

		code = PollKeyboard(bus_space_read_1(console_kbd->sc_iot, console_kbd->sc_ioh, KBD_DATA));

		if (code != 0) {
			up = (code & 0x100);
			key = code & 0xff;

/*			printf("code=%04x mod=%04x\n", code, modifiers);*/

			/* By default we use the main keycode lookup table */

			ks = keys;

			/* If we have an E0 or E1 sqeuence we use the extended table */

			if (code > 0x1ff)
				ks = E0keys;

			/* Is the key a temporary modifier ? */

			if (ks[key].flags & MODIFIER_MASK) {
				if (up)
					modifiers &= ~ks[key].flags;
				else
					modifiers |= ks[key].flags;
				key = 0;
				continue;
			}

/* Is the key a locking modifier ? */

			if (ks[key].flags & MODIFIER_LOCK_MASK) {
				if (!up) {
					modifiers ^= ks[key].flags;
					kbdsetleds(console_kbd, (modifiers >> 3) & 7);
				}
				key = 0;
				continue;
			}

/* Lookup the correct key code */

			if (modifiers & 0x01)
				key = ks[key].ctrl_code;
			else if (modifiers & 0x02)
				key = ks[key].shift_code;
			else if (modifiers & 0x04)
				key = ks[key].alt_code;
			else
				key = ks[key].base_code;

			if (modifiers & MODIFIER_CAPS) {
				if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z'))
					key ^= 0x20;
			}

			if (up)
				key = 0;
			if (!up && key >= 0x200) {

#if (NVT > 0)
				if ((key & ~0x0f) == 0x480)
					console_switch((key & 0x0f) - 1);
				else
#endif
				switch (key) {
#if (NVT > 0)
				case 0x201:
					console_scrollforward();
					break;
				case 0x200:
					console_scrollback();
					break;
#endif
				default:
					break;
				}
				key = 0;
			}
		}
	} while (key == 0);

	if (key == '\r')
		key = '\n';

	splx(s);
	return(key);
}


/* Keyboard IRQ handler */

int
kbdintr(arg)
	void *arg;
{
	struct kbd_softc *sc = arg;
	int key;

/* Read the IOMD keyboard register and process the key */

	key = PollKeyboard(bus_space_read_1(sc->sc_iot, sc->sc_ioh, KBD_DATA));

/* If we have a raw keycode convert it to an ASCII code */

	if (key != 0)
		kbddecodekey(sc, key);

	return(1);	/* Claim interrupt */
}


/* Flags used to decode the raw keys */

#define FLAG_KEYUP	0x01
#define FLAG_E0		0x02
#define FLAG_E1		0x04

static int flags = 0;

/*
 * This function is now misnamed.
 * It processes the raw key codes from the keyboard and generates
 * a unique code that can be decoded with the key translation array.
 */

int
PollKeyboard(code)
	int code;
{
/*	printf("%02x.", code);*/

/*
 * Set the keyup flag if this is the release code.
 */

	if (code == 0xf0) {
		flags |= FLAG_KEYUP;
		return(0);
	}

/* If it is a special code ignore it */

	if (code == 0xff || code == 0x00) {
		flags = 0;
		return(0);
	}

/* If it is a resend code note it down */

	if (code == KBR_RESEND) {
		printf("kbd:resend\n");
		kbd_resend = 1;
		return(0);
	}

/* If it is an ack code note it down */

	if (code == KBR_ACK) {
/*		printf("kbd:ack\n");*/
		kbd_ack = 1;
		return(0);
	}

/* Flag the start of an E0 sequence. */

	if (code == 0xe0) {
		flags |= FLAG_E0;
		return(0);
	}

/* Flag the start of an E1 sequence. */

	if (code == 0xe1) {
		flags |= FLAG_E1;
		return(0);
	}

/* Ignore any other invalid codes */

	if (code > 0x8f) {
		flags = 0;
		return(0);
	}

/*	printf("%02x:%02x.", code, flags);*/

/* Mark the code appropriately if it is part of an E0 sequence */

	if (flags & FLAG_E0) {
		flags &= ~FLAG_E0;
		if (code == 0x12) {
			flags &= ~FLAG_KEYUP;
			return(0);
		}
		code |= 0x200;
	}

/* Mark the key if it is the upcode */

	if (flags & FLAG_KEYUP) {
		flags &= ~FLAG_KEYUP;
		code |= 0x100;
	}

/* Mark the code appropriately if it is part of an E1 sequence */

	if (flags & FLAG_E1) {
		if ((code & 0xff) == 0x14) {
			return(0);
		}
		flags &= ~FLAG_E1;
		code |= 0x400;
		flags &= ~FLAG_KEYUP;
	}

	return(code);
}


/*
 * This routine decodes the unique keycode and generates an ASCII code
 * if necessary.
 */

int
kbddecodekey(sc, code)
	struct kbd_softc *sc;
	int code;
{
	key_struct *ks;
	int up;
	int key;

	console_unblank();

/* Do we have the raw kbd device open ... */

	if (rawkbd_device == 1 && code != 0) {
		struct kbd_data buffer;
		int s;
  
  /* Add this event to the queue. */
 
 		buffer.keycode = code;
 		microtime(&buffer.event_time);
		s=spltty();
 		(void) b_to_q((char *)&buffer, sizeof(buffer), &sc->sc_q);
 		splx(s);
		selwakeup(&sc->sc_rsel);

		if (sc->sc_state & RAWKBD_ASLEEP) {
			sc->sc_state &= ~RAWKBD_ASLEEP;
			wakeup((caddr_t)sc);
		}

		psignal(sc->sc_proc, SIGIO);
		return(1);
	}

	up = (code & 0x100);
	key = code & 0xff;

/* By default we use the main keycode lookup table */

	ks = keys;

/* If we have an E0 or E1 sqeuence we use the extended table */

	if (code > 0x1ff)
		ks = E0keys;

/* Is the key a temporary modifier ? */

	if (ks[key].flags & MODIFIER_MASK) {
		if (up)
			modifiers &= ~ks[key].flags;
		else
			modifiers |= ks[key].flags;
		return(0);
	}

/* Is the key a locking modifier ? */

	if (ks[key].flags & MODIFIER_LOCK_MASK) {
		if (!up) {
			modifiers ^= ks[key].flags;
			kbdsetleds(sc, (modifiers >> 3) & 7);
		}
		return(0);
	}

/* Lookup the correct key code */

	if (modifiers & 0x01)
		key = ks[key].ctrl_code;
	else if (modifiers & 0x02)
		key = ks[key].shift_code;
	else if (modifiers & 0x04)
		key = ks[key].alt_code;
	else
		key = ks[key].base_code;

	if (modifiers & MODIFIER_CAPS) {
		if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z'))
			key ^= 0x20;
	}

/* If no valid code the key is not yet mapped so report error */

#ifdef DEBUG_TERM
/*	if (key == 0) {
		char err[80];

		sprintf(err, "\n\rUnknown keycode %04x\n\r", code);
		dprintf(err);
 
	}*/
#endif

/* If we have an ASCII code insert it into the keyboard buffer */

	if (!up && key != 0) {
		if (key >= 0x200) {

			callout_stop(&autorepeatstart_ch);
			callout_stop(&autorepeat_ch);
			autorepeatkey = -1;
#if (NVT > 0)
			if ((key & ~0x0f) == 0x480)
				console_switch((key & 0x0f) - 1);
			else
#endif
				switch (key) {
#ifdef PMAP_DEBUG
				case 0x22b:
					pmap_debug(pmap_debug_level + 1);
					break;
				case 0x22d:
					pmap_debug(pmap_debug_level - 1);
					break;
#endif
#if (NVT > 0)
				case 0x201:
					console_scrollforward();
					break;
				case 0x200:
					console_scrollback();
					break;
/*
				case 0x202:
					console_switchdown();
					break;
				case 0x203:
					console_switchup();
					break;
*/
#endif
				case 0x204:
					--kbdautorepeat.ka_rate;
					if (kbdautorepeat.ka_rate < 1)
						kbdautorepeat.ka_rate = 1;
					break;
				case 0x205:
					++kbdautorepeat.ka_rate;
					if (kbdautorepeat.ka_rate > 50)
						kbdautorepeat.ka_rate = 50;
					break;
				case 0x20a:
					++kbdautorepeat.ka_delay;
					if (kbdautorepeat.ka_delay > 50)
						kbdautorepeat.ka_delay = 50;
					break;
				case 0x20b:
					--kbdautorepeat.ka_delay;
					if (kbdautorepeat.ka_delay < 1)
						kbdautorepeat.ka_delay = 1;
					break;
#ifdef DDB
				case 0x208:
					Debugger();
					break;
#endif
				case 0x21b:
					printf("Kernel interruption\n");
					cpu_reboot(RB_HALT, NULL);
					break;
				case 0x209:
					printf("Kernel interruption - nosync\n");
					cpu_reboot(RB_NOSYNC | RB_HALT, NULL);
					break;
	
				default:
					printf("Special key %04x\n", key);
					break;
				}
		} else {
			/*
			 * Check rawkbd_device first, in case we stick in the
			 * physconkbd().
			 */
			if (rawkbd_device == 0 && physconkbd(key) == 0) {
				if (autorepeatkey != key) {
					callout_stop(&autorepeatstart_ch);
					callout_stop(&autorepeat_ch);
					autorepeatkey = key;
					callout_reset(&autorepeatstart_ch,
					    hz / kbdautorepeat.ka_delay,
					    autorepeatstart, &autorepeatkey);
				}
			}

			return(1);
		}
	} else {
		callout_stop(&autorepeatstart_ch);
		callout_stop(&autorepeat_ch);
		autorepeatkey = -1;
	}
	return(0);
}


void
autorepeatstart(key)
	void	*key;
{
	physconkbd(*((int *)key));
	callout_reset(&autorepeat_ch, hz / kbdautorepeat.ka_rate,
	    autorepeat, key);
}


void
autorepeat(key)
	void	*key;
{
	physconkbd(*((int *)key));
	callout_reset(&autorepeat_ch, hz / kbdautorepeat.ka_rate,
	    autorepeat, key);
}

/* End of kbd.c */
