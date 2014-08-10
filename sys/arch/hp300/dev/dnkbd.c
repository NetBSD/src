/*	$NetBSD: dnkbd.c,v 1.5.32.1 2014/08/10 06:53:57 tls Exp $	*/
/*	$OpenBSD: dnkbd.c,v 1.17 2009/07/23 21:05:56 blambert Exp $	*/

/*
 * Copyright (c) 2005, Miodrag Vallat
 * Copyright (c) 1997 Michael Smith.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

/*
 * Driver for the Apollo Domain keyboard and mouse.
 *
 * Random notes on the Apollo keyboard :
 *
 * - Powers up in 'cooked' mode, where the alpha keys generate ascii rather
 *   than make/break codes.  Other keys seem to behave OK though.
 *
 * - Alt L/R keys generate two-byte sequence :
 *             make     break
 *       L    0xfe,2   0xfe,3
 *       R    0xfe,0   0xfe,1
 *
 * - Mouse activity shows up inline in 4-byte packets introduced with 0xdf.
 *   Byte 1 is   1MRL0000    where M, R, L are the mouse buttons, and 0 is
 *                           down, 1 is up.
 *   Byte 2 is 2's complement X movement, +ve to the right.
 *   Byte 3 is 2's complement Y movement, +ve is up.
 *
 * - Keyboard recognises commands sent to it, all preceded by 0xff.  Commands
 *   are echoed once sent completely.
 *
 *   0x00	go to cooked mode.
 *   0x01	go to 'raw' (scancode) mode.
 *   0x12,0x21	status report as <id1>\r<id2>\r<model>\r followed by 0xff 
 *		and then the cooked/raw status.
 *   0x21,0x81	beep on
 *   0x21,0x82	beep off
 *
 * Some version examples :
 *
 * <3-@> <1-0> <SD-03687-MS>	Apollo p/n 007121 REV 00 ('old-style' US layout)
 * <3-@> <2-0> <SD-03683-MS>	Apollo p/n 007121 REV 01 ('old-style' US layout)
 * <3-@> <2-0> <SD-03980-MS>	Apollo 3500? keyboard.
 * <3-@> <X-X> <RX-60857-HW>	HP p/n A1630-82001 R2
 *				    ('new-style' off 425t, US layout),
 *				also Apollo p/n 014555-002
 *				    ('new-style' off DN5500, US layout).
 */

#include "opt_wsdisplay_compat.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <machine/autoconf.h>

#include <dev/cons.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include "wsmouse.h"
#if NWSMOUSE > 0
#include <dev/wscons/wsmousevar.h>
#endif

#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>

#include <hp300/dev/dnkbdmap.h>
#include <hp300/dev/frodoreg.h>
#include <hp300/dev/frodovar.h>

#include "hilkbd.h"
#include "ioconf.h"

/*
 * Keyboard key codes
 */

#define	DNKEY_CAPSLOCK	0x7e
#define	DNKEY_REPEAT	0x7f
#define	DNKEY_RELEASE	0x80
#define	DNKEY_CHANNEL	0xff

/*
 * Channels
 */

#define	DNCHANNEL_RESET	0x00
#define	DNCHANNEL_KBD	0x01
#define	DNCHANNEL_MOUSE	0x02

/*
 * Keyboard modes
 */

#define	DNMODE_COOKED	0x00
#define	DNMODE_RAW	0x01

/*
 * Keyboard commands
 */

#define	DNCMD_PREFIX	0xff
#define	DNCMD_COOKED	DNMODE_COOKED
#define	DNCMD_RAW	DNMODE_RAW
#define	DNCMD_IDENT_1	0x12
#define	DNCMD_IDENT_2	0x21

/*
 * Bell commands
 */

#define	DNCMD_BELL	0x21
#define	DNCMD_BELL_ON	0x81
#define	DNCMD_BELL_OFF	0x82

/*
 * Mouse status
 */

#define	DNBUTTON_L	0x10
#define	DNBUTTON_R	0x20
#define	DNBUTTON_M	0x40

struct dnkbd_softc {
	device_t	sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	int		sc_flags;
#define	SF_ENABLED	0x01		/* keyboard enabled */
#define	SF_CONSOLE	0x02		/* keyboard is console */
#define	SF_POLLING	0x04		/* polling mode */
#define	SF_PLUGGED	0x08		/* keyboard has been seen plugged */
#define	SF_ATTACHED	0x10		/* subdevices have been attached */
#define	SF_MOUSE	0x20		/* mouse enabled */
#define	SF_BELL		0x40		/* bell is active */
#define	SF_BELL_TMO	0x80		/* bell stop timeout is scheduled */

	u_int		sc_identlen;
#define	MAX_IDENTLEN	32
	char		sc_ident[MAX_IDENTLEN];
	kbd_t		sc_layout;

	enum { STATE_KEYBOARD, STATE_MOUSE, STATE_CHANNEL, STATE_ECHO }
			sc_state, sc_prevstate;
	u_int		sc_echolen;

	uint8_t		sc_mousepkt[3];	/* mouse packet being constructed */
	u_int		sc_mousepos;	/* index in above */

	struct callout	sc_bellstop_tmo;

	device_t	sc_wskbddev;
#if NWSMOUSE > 0
	device_t	sc_wsmousedev;
#endif

#ifdef WSDISPLAY_COMPAT_RAWKBD
	int		sc_rawkbd;
	int		sc_nrep;
	char		sc_rep[2];	/* at most, one key */
	struct callout	sc_rawrepeat_ch;
#define	REP_DELAY1	400
#define	REP_DELAYN	100
#endif
};

static int	dnkbd_match(device_t, cfdata_t, void *);
static void	dnkbd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(dnkbd, sizeof(struct dnkbd_softc),
    dnkbd_match, dnkbd_attach, NULL, NULL);

static void	dnkbd_init(struct dnkbd_softc *, uint16_t, uint16_t);
static int	dnkbd_enable(void *, int);
static void	dnkbd_set_leds(void *, int);
static int	dnkbd_ioctl(void *, u_long, void *, int, struct lwp *);

static const struct wskbd_accessops dnkbd_accessops = {
	dnkbd_enable,
	dnkbd_set_leds,
	dnkbd_ioctl
};

#if NWSMOUSE > 0
static int	dnmouse_enable(void *);
static int	dnmouse_ioctl(void *, u_long, void *, int, struct lwp *);
static void	dnmouse_disable(void *);

static const struct wsmouse_accessops dnmouse_accessops = {
	dnmouse_enable,
	dnmouse_ioctl,
	dnmouse_disable
};
#endif

static void	dnkbd_bell(void *, u_int, u_int, u_int);
static void	dnkbd_cngetc(void *, u_int *, int *);
static void	dnkbd_cnpollc(void *, int);

static const struct wskbd_consops dnkbd_consops = {
	dnkbd_cngetc,
	dnkbd_cnpollc,
	dnkbd_bell
};

static struct wskbd_mapdata dnkbd_keymapdata = {
	dnkbd_keydesctab,
#ifdef DNKBD_LAYOUT
	DNKBD_LAYOUT
#else
	KB_US
#endif
};

typedef enum { EVENT_NONE, EVENT_KEYBOARD, EVENT_MOUSE } dnevent;

#define APCIBRD(x)	(500000 / (x))

static void	dnevent_kbd(struct dnkbd_softc *, int);
static void	dnevent_kbd_internal(struct dnkbd_softc *, int);
static void	dnevent_mouse(struct dnkbd_softc *, uint8_t *);
static void	dnkbd_attach_subdevices(struct dnkbd_softc *);
static void	dnkbd_bellstop(void *);
static void	dnkbd_decode(int, u_int *, int *);
static dnevent	dnkbd_input(struct dnkbd_softc *, int);
static int	dnkbd_intr(void *);
static int	dnkbd_pollin(struct dnkbd_softc *, u_int);
static int	dnkbd_pollout(struct dnkbd_softc *, int);
static int	dnkbd_probe(struct dnkbd_softc *);
#ifdef WSDISPLAY_COMPAT_RAWKBD
static void	dnkbd_rawrepeat(void *);
#endif
static int	dnkbd_send(struct dnkbd_softc *, const uint8_t *, size_t);

int
dnkbd_match(device_t parent, cfdata_t cf, void *aux)
{
	struct frodo_attach_args *fa = aux;

	if (strcmp(fa->fa_name, dnkbd_cd.cd_name) != 0)
		return 0;

	if (machineid == HP_382) {
		/* 382 has frodo but no Domain keyboard connector. */
		return 0;
	}

	/* only attach to the first frodo port */
	return fa->fa_offset == FRODO_APCI_OFFSET(0);
}

void
dnkbd_attach(device_t parent, device_t self, void *aux)
{
	struct dnkbd_softc *sc = device_private(self);
	struct frodo_attach_args *fa = aux;

	aprint_normal(": ");

	sc->sc_dev = self;
	sc->sc_bst = fa->fa_bst;
	if (bus_space_map(sc->sc_bst, fa->fa_base + fa->fa_offset,
	    FRODO_APCISPACE, 0, &sc->sc_bsh) != 0) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	callout_init(&sc->sc_bellstop_tmo, 0);
	callout_setfunc(&sc->sc_bellstop_tmo, dnkbd_bellstop, sc);
#ifdef WSDISPLAY_COMPAT_RAWKBD
	callout_init(&sc->sc_rawrepeat_ch, 0);
	callout_setfunc(&sc->sc_rawrepeat_ch, dnkbd_rawrepeat, sc);
#endif

	/* reset the port */
	dnkbd_init(sc, 1200, LCR_8BITS | LCR_PEVEN | LCR_PENAB);

	frodo_intr_establish(parent, dnkbd_intr, sc, fa->fa_line, IPL_VM);

	/* probe for keyboard */
	if (dnkbd_probe(sc) != 0) {
		aprint_normal("no keyboard\n");
		return;
	}

	dnkbd_attach_subdevices(sc);
}

void
dnkbd_init(struct dnkbd_softc *sc, uint16_t rate, uint16_t lctl)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	u_int divisor;

	bst = sc->sc_bst;
	bsh = sc->sc_bsh;

	divisor = APCIBRD(rate);
	bus_space_write_1(bst, bsh, com_lctl, LCR_DLAB);
	bus_space_write_1(bst, bsh, com_dlbl, divisor & 0xff);
	bus_space_write_1(bst, bsh, com_dlbh, (divisor >> 8) & 0xff);
	bus_space_write_1(bst, bsh, com_lctl, lctl);
	bus_space_write_1(bst, bsh, com_ier, IER_ERXRDY | IER_ETXRDY);
	bus_space_write_1(bst, bsh, com_fifo,
	    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_1);
	bus_space_write_1(bst, bsh, com_mcr, MCR_DTR | MCR_RTS);

	delay(100);
	(void)bus_space_read_1(bst, bsh, com_iir);
}

void
dnkbd_attach_subdevices(struct dnkbd_softc *sc)
{
	struct wskbddev_attach_args ka;
#if NWSMOUSE > 0
	struct wsmousedev_attach_args ma;
#endif
#if NHILKBD > 0
	extern int hil_is_console;
#endif

	/*
	 * If both hilkbd and dnkbd are configured, prefer the Domain
	 * keyboard as console (if we are here, we know the keyboard is
	 * plugged), unless the console keyboard has been claimed already
	 * (i.e. late hotplug with hil keyboard plugged first).
	 */
	if (major(cn_tab->cn_dev) == devsw_name2chr("wsdisplay", NULL, 0)) {
#if NHILKBD > 0
		if (hil_is_console == -1) {
			ka.console = 1;
			hil_is_console = 0;
		} else
			ka.console = 0;
#else
		ka.console = 1;
#endif
	} else
		ka.console = 0;

	ka.keymap = &dnkbd_keymapdata;
	ka.accessops = &dnkbd_accessops;
	ka.accesscookie = sc;
#ifndef DKKBD_LAYOUT
	dnkbd_keymapdata.layout = sc->sc_layout;
#endif

	if (ka.console) {
		sc->sc_flags = SF_PLUGGED | SF_CONSOLE | SF_ENABLED;
		wskbd_cnattach(&dnkbd_consops, sc, &dnkbd_keymapdata);
	} else {
		sc->sc_flags = SF_PLUGGED;
	}

	sc->sc_wskbddev =
	    config_found_ia(sc->sc_dev, "wskbddev", &ka, wskbddevprint);

#if NWSMOUSE > 0
	ma.accessops = &dnmouse_accessops;
	ma.accesscookie = sc;

	sc->sc_wsmousedev =
	    config_found_ia(sc->sc_dev, "wsmousedev", &ma, wsmousedevprint);
#endif

	SET(sc->sc_flags, SF_ATTACHED);
}

int
dnkbd_probe(struct dnkbd_softc *sc)
{
	int dat, rc, flags;
	uint8_t cmdbuf[2];
	char rspbuf[MAX_IDENTLEN], *word, *end;
	u_int i;
	int s;

	s = spltty();
	flags = sc->sc_flags;
	SET(sc->sc_flags, SF_POLLING);
	sc->sc_state = STATE_CHANNEL;
	splx(s);

	/*
	 * Switch keyboard to raw mode.
	 */
	cmdbuf[0] = DNCMD_RAW;
	rc = dnkbd_send(sc, cmdbuf, 1);
	if (rc != 0)
		goto out;

	/*
	 * Send the identify command.
	 */
	cmdbuf[0] = DNCMD_IDENT_1;
	cmdbuf[1] = DNCMD_IDENT_2;
	rc = dnkbd_send(sc, cmdbuf, 2);
	if (rc != 0)
		goto out;

	for (i = 0; ; i++) {
		dat = dnkbd_pollin(sc, 10000);
		if (dat == -1)
			break;

		if (i < sizeof(rspbuf))
			rspbuf[i] = dat;
	}

	if (i > sizeof(rspbuf) || i == 0) {
		aprint_error_dev(sc->sc_dev,
		    "unexpected identify string length %d\n", i);
		rc = ENXIO;
		goto out;
	}

	/*
	 * Make sure the identification string is NULL terminated
	 * (overwriting the keyboard mode byte if necessary).
	 */
	i--;
	if (rspbuf[i] != 0)
		rspbuf[i] = 0;

	/*
	 * Now display the identification strings, if they changed.
	 */
	if (i != sc->sc_identlen || memcmp(rspbuf, sc->sc_ident, i) != 0) {
		sc->sc_layout = KB_US;
		sc->sc_identlen = i;
		memcpy(sc->sc_ident, rspbuf, i);

		if (cold == 0)
			aprint_normal_dev(sc->sc_dev, "");
		aprint_normal("model ");
		word = rspbuf;
		for (i = 0; i < 3; i++) {
			end = strchr(word, '\r');
			if (end == NULL)
				break;
			*end++ = '\0';
			aprint_normal("<%s> ", word);
			/*
			 * Parse the layout code if applicable
			 */
			if (i == 1 && *word++ == '3') {
				if (*word == '-')
					word++;
				switch (*word) {
#if 0
				default:
				case ' ':
					sc->sc_layout = KB_US;
					break;
#endif
				case 'a':
					sc->sc_layout = KB_DE;
					break;
				case 'b':
					sc->sc_layout = KB_FR;
					break;
				case 'c':
					sc->sc_layout = KB_DK;
					break;
				case 'd':
					sc->sc_layout = KB_SV;
					break;
				case 'e':
					sc->sc_layout = KB_UK;
					break;
				case 'f':
					sc->sc_layout = KB_JP;
					break;
				case 'g':
					sc->sc_layout = KB_SG;
					break;
				}
			}
			word = end;
		}
		aprint_normal("\n");
	}

	/*
	 * Ready to work, the default channel is the keyboard.
	 */
	sc->sc_state = STATE_KEYBOARD;

out:
	s = spltty();
	sc->sc_flags = flags;
	splx(s);

	return rc;
}

/*
 * State machine.
 *
 * In raw mode, the keyboard may feed us the following sequences:
 * - on the keyboard channel:
 *   + a raw key code, in the range 0x01-0x7e, or'ed with 0x80 if key release.
 *   + the key repeat sequence 0x7f.
 * - on the mouse channel:
 *   + a 3 byte mouse sequence (buttons state, dx move, dy move).
 * - at any time:
 *   + a 2 byte channel sequence (0xff followed by the channel number) telling
 *     us which device the following input will come from.
 *   + if we get 0xff but an invalid channel number, this is a command echo.
 *     Currently we only handle this for bell commands, which size are known.
 *     Other commands are issued through dnkbd_send() which ``eats'' the echo.
 *
 * Older keyboards reset the channel to the keyboard (by sending ff 01) after
 * every mouse packet.
 */

dnevent
dnkbd_input(struct dnkbd_softc *sc, int dat)
{
	dnevent event = EVENT_NONE;

	switch (sc->sc_state) {
	case STATE_KEYBOARD:
		switch (dat) {
		case DNKEY_REPEAT:
			/*
			 * We ignore event repeats, as wskbd does its own
			 * soft repeat processing.
			 */
			break;
		case DNKEY_CHANNEL:
			sc->sc_prevstate = sc->sc_state;
			sc->sc_state = STATE_CHANNEL;
			break;
		default:
			event = EVENT_KEYBOARD;
			break;
		}
		break;

	case STATE_MOUSE:
		if (dat == DNKEY_CHANNEL && sc->sc_mousepos == 0) {
			sc->sc_prevstate = sc->sc_state;
			sc->sc_state = STATE_CHANNEL;
		} else {
			sc->sc_mousepkt[sc->sc_mousepos++] = dat;
			if (sc->sc_mousepos == sizeof(sc->sc_mousepkt)) {
				sc->sc_mousepos = 0;
				event = EVENT_MOUSE;
			}
		}
		break;

	case STATE_CHANNEL:
		switch (dat) {
		case DNKEY_CHANNEL:
			/*
			 * During hotplug, we might get spurious 0xff bytes.
			 * Ignore them.
			 */
			break;
		case DNCHANNEL_RESET:
			/*
			 * Identify the keyboard again. This will switch it to
			 * raw mode again. If this fails, we'll consider the
			 * keyboard as unplugged (to ignore further events until
			 * a successful reset).
			 */
			if (dnkbd_probe(sc) == 0) {
				/*
				 * We need to attach wskbd and wsmouse children
				 * if this is a live first plug.
				 */
				if (!ISSET(sc->sc_flags, SF_ATTACHED))
					dnkbd_attach_subdevices(sc);
				SET(sc->sc_flags, SF_PLUGGED);
			} else {
				CLR(sc->sc_flags, SF_PLUGGED);
			}

			sc->sc_state = STATE_KEYBOARD;
			break;
		case DNCHANNEL_KBD:
			sc->sc_state = STATE_KEYBOARD;
			break;
		case DNCHANNEL_MOUSE:
			sc->sc_state = STATE_MOUSE;
			sc->sc_mousepos = 0;	/* just in case */
			break;
		case DNCMD_BELL:
			/*
			 * We are getting a bell command echoed to us.
			 * Ignore it.
			 */
			sc->sc_state = STATE_ECHO;
			sc->sc_echolen = 1;	/* one byte to follow */
			break;
		default:
			printf("%s: unexpected channel byte %02x\n",
			    device_xname(sc->sc_dev), dat);
			break;
		}
		break;

	case STATE_ECHO:
		if (--sc->sc_echolen == 0) {
			/* get back to the state we were in before the echo */
			sc->sc_state = sc->sc_prevstate;
		}
		break;
	}

	return event;
}

/*
 * Event breakers.
 */

void
dnkbd_decode(int keycode, u_int *type, int *key)
{
	*type = (keycode & DNKEY_RELEASE) ?
	    WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;
	*key = (keycode & ~DNKEY_RELEASE);
}

void
dnevent_kbd(struct dnkbd_softc *sc, int dat)
{
	if (!ISSET(sc->sc_flags, SF_PLUGGED))
		return;

	if (sc->sc_wskbddev == NULL)
		return;

	if (!ISSET(sc->sc_flags, SF_ENABLED))
		return;

	/*
	 * Even in raw mode, the caps lock key is treated specially:
	 * first key press causes event 0x7e, release causes no event;
	 * then a new key press causes nothing, and release causes
	 * event 0xfe. Moreover, while kept down, it does not produce
	 * repeat events.
	 *
	 * So the best we can do is fake the missed events, but this
	 * will not allow the capslock key to be remapped as a control
	 * key since it will not be possible to chord it with anything.
	 */
	dnevent_kbd_internal(sc, dat);
	if ((dat & ~DNKEY_RELEASE) == DNKEY_CAPSLOCK)
		dnevent_kbd_internal(sc, dat ^ DNKEY_RELEASE);
}

void
dnevent_kbd_internal(struct dnkbd_softc *sc, int dat)
{
	u_int type;
	int key;
	int s;

	dnkbd_decode(dat, &type, &key);

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (sc->sc_rawkbd) {
		u_char cbuf[2];
		int c, j = 0;

		c = dnkbd_raw[key];
		if (c != 0) {
			/* fake extended scancode if necessary */
			if (c & 0x80)
				cbuf[j++] = 0xe0;
			cbuf[j] = c & 0x7f;
			if (type == WSCONS_EVENT_KEY_UP)
				cbuf[j] |= 0x80;
			else {
				/* remember pressed key for autorepeat */
				memcpy(sc->sc_rep, cbuf, sizeof(sc->sc_rep));
			}
			j++;
		}

		if (j != 0) {
			s = spltty();
			wskbd_rawinput(sc->sc_wskbddev, cbuf, j);
			splx(s);
			callout_stop(&sc->sc_rawrepeat_ch);
			sc->sc_nrep = j;
			callout_schedule(&sc->sc_rawrepeat_ch,
			    mstohz(REP_DELAY1));
		}
	} else
#endif
	{
		s = spltty();
		wskbd_input(sc->sc_wskbddev, type, key);
		splx(s);
	}
}

#ifdef WSDISPLAY_COMPAT_RAWKBD
void
dnkbd_rawrepeat(void *v)
{
	struct dnkbd_softc *sc = v;
	int s;

	s = spltty();
	wskbd_rawinput(sc->sc_wskbddev, sc->sc_rep, sc->sc_nrep);
	splx(s);

	callout_schedule(&sc->sc_rawrepeat_ch, mstohz(REP_DELAYN));
}
#endif

#if NWSMOUSE > 0
void
dnevent_mouse(struct dnkbd_softc *sc, uint8_t *dat)
{
	if (!ISSET(sc->sc_flags, SF_PLUGGED))
		return;

	if (sc->sc_wsmousedev == NULL)
		return;

	if (!ISSET(sc->sc_flags, SF_MOUSE))
		return;

	/*
	 * First byte is button status. It has the 0x80 bit always set, and
	 * the next 3 bits are *cleared* when the mouse buttons are pressed.
	 */
#ifdef DEBUG
	if (!ISSET(*dat, 0x80)) {
		printf("%s: incorrect mouse packet %02x %02x %02x\n",
		    device_xname(sc->sc_dev), dat[0], dat[1], dat[2]);
		return;
	}
#endif

	wsmouse_input(sc->sc_wsmousedev,
	    (~dat[0] & (DNBUTTON_L | DNBUTTON_M | DNBUTTON_R)) >> 4,
	    (int8_t)dat[1], (int8_t)dat[2], 0, 0, WSMOUSE_INPUT_DELTA);
}
#endif

/*
 * Low-level communication routines.
 */

int
dnkbd_pollin(struct dnkbd_softc *sc, u_int tries)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	u_int cnt;

	bst = sc->sc_bst;
	bsh = sc->sc_bsh;

	for (cnt = tries; cnt != 0; cnt--) {
		if (bus_space_read_1(bst, bsh, com_lsr) & LSR_RXRDY)
			break;
		DELAY(10);
	}

	if (cnt == 0)
		return -1;
	else
		return (int)bus_space_read_1(bst, bsh, com_data);
}

int
dnkbd_pollout(struct dnkbd_softc *sc, int dat)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	u_int cnt;

	bst = sc->sc_bst;
	bsh = sc->sc_bsh;

	for (cnt = 10000; cnt != 0; cnt--) {
		if (bus_space_read_1(bst, bsh, com_lsr) & LSR_TXRDY)
			break;
		DELAY(10);
	}
	if (cnt == 0)
		return EBUSY;
	else {
		bus_space_write_1(bst, bsh, com_data, dat);
		return 0;
	}
}

int
dnkbd_send(struct dnkbd_softc *sc, const uint8_t *cmdbuf, size_t cmdlen)
{
	int cnt, rc, dat;
	u_int cmdpos;

	/* drain rxfifo */
	for (cnt = 10; cnt != 0; cnt--) {
		if (dnkbd_pollin(sc, 10) == -1)
			break;
	}
	if (cnt == 0)
		return EBUSY;

	/* send command escape */
	if ((rc = dnkbd_pollout(sc, DNCMD_PREFIX)) != 0)
		return rc;

	/* send command buffer */
	for (cmdpos = 0; cmdpos < cmdlen; cmdpos++) {
		if ((rc = dnkbd_pollout(sc, cmdbuf[cmdpos])) != 0)
			return rc;
	}

	/* wait for command echo */
	do {
		dat = dnkbd_pollin(sc, 10000);
		if (dat == -1)
			return EIO;
	} while (dat != DNCMD_PREFIX);

	for (cmdpos = 0; cmdpos < cmdlen; cmdpos++) {
		dat = dnkbd_pollin(sc, 10000);
		if (dat != cmdbuf[cmdpos])
			return EIO;
	}

	return 0;
}

int
dnkbd_intr(void *v)
{
	struct dnkbd_softc *sc = v;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint8_t iir, lsr, c;
	int claimed = 0;

	bst = sc->sc_bst;
	bsh = sc->sc_bsh;

	for (;;) {
		iir = bus_space_read_1(bst, bsh, com_iir);

		switch (iir & IIR_IMASK) {
		case IIR_RLS:
			/*
			 * Line status change. This should never happen,
			 * so silently ack the interrupt.
			 */
			c = bus_space_read_1(bst, bsh, com_lsr);
			break;

		case IIR_RXRDY:
		case IIR_RXTOUT:
			/*
			 * Data available. We process it byte by byte,
			 * unless we are doing polling work...
			 */
			if (ISSET(sc->sc_flags, SF_POLLING)) {
				return 1;
			}

			for (;;) {
				c = bus_space_read_1(bst, bsh, com_data);
				switch (dnkbd_input(sc, c)) {
				case EVENT_KEYBOARD:
					dnevent_kbd(sc, c);
					break;
#if NWSMOUSE > 0
				case EVENT_MOUSE:
					dnevent_mouse(sc, sc->sc_mousepkt);
					break;
#endif
				default:	/* appease gcc */
					break;
				}
				lsr = bus_space_read_1(bst, bsh, com_lsr) &
				    LSR_RCV_MASK;
				if (lsr == 0)
					break;
				else if (lsr != LSR_RXRDY) {
					/* ignore error */
					break;
				}
			}
			break;

		case IIR_TXRDY:
			/*
			 * Transmit available. Since we do all our commands
			 * in polling mode, we do not need to do anything here.
			 */
			break;

		default:
			if (iir & IIR_NOPEND)
				return claimed;
			/* FALLTHROUGH */

		case IIR_MLSC:
			/*
			 * Modem status change. This should never happen,
			 * so silently ack the interrupt.
			 */
			c = bus_space_read_1(bst, bsh, com_msr);
			break;
		}

		claimed = 1;
	}
}

/*
 * Wskbd callbacks
 */

int
dnkbd_enable(void *v, int on)
{
	struct dnkbd_softc *sc = v;

	if (on) {
		if (ISSET(sc->sc_flags, SF_ENABLED))
			return EBUSY;
		SET(sc->sc_flags, SF_ENABLED);
	} else {
		if (ISSET(sc->sc_flags, SF_CONSOLE))
			return EBUSY;
		CLR(sc->sc_flags, SF_ENABLED);
	}

	return 0;
}

void
dnkbd_set_leds(void *v, int leds)
{
	/*
	 * Not supported. There is only one LED on this keyboard, and
	 * is hardware tied to the caps lock key.
	 */
}

int
dnkbd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
#ifdef WSDISPLAY_COMPAT_RAWKBD
	struct dnkbd_softc *sc = v;
#endif

#define WSKBD_TYPE_UNKNOWN	0

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_UNKNOWN;	/* XXX */
		return 0;
	case WSKBDIO_SETLEDS:
		return ENXIO;
	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return 0;
	case WSKBDIO_COMPLEXBELL:
#define	d	((struct wskbd_bell_data *)data)
		dnkbd_bell(v, d->period, d->pitch, d->volume);
#undef d
		return 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = *(int *)data == WSKBD_RAW;
		callout_stop(&sc->sc_rawrepeat_ch);
		return 0;
#endif
	}

	return EPASSTHROUGH;
}

#if NWSMOUSE > 0
/*
 * Wsmouse callbacks
 */

int
dnmouse_enable(void *v)
{
	struct dnkbd_softc *sc = v;

	if (ISSET(sc->sc_flags, SF_MOUSE))
		return EBUSY;
	SET(sc->sc_flags, SF_MOUSE);

	return 0;
}

int
dnmouse_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
#if 0
	struct dnkbd_softc *sc = v;
#endif

#define WSMOUSE_TYPE_UNKNOWN	0

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(int *)data = WSMOUSE_TYPE_UNKNOWN;	/* XXX */
		return 0;
	}

	return -1;
}

void
dnmouse_disable(void *v)
{
	struct dnkbd_softc *sc = v;

	CLR(sc->sc_flags, SF_MOUSE);
}
#endif

/*
 * Console support
 */

void
dnkbd_cngetc(void *v, u_int *type, int *data)
{
	static int lastdat = 0;
	struct dnkbd_softc *sc = v;
	int s;
	int dat;

	/* Take care of caps lock */
	if ((lastdat & ~DNKEY_RELEASE) == DNKEY_CAPSLOCK) {
		dat = lastdat ^ DNKEY_RELEASE;
		lastdat = 0;
	} else {
		for (;;) {
			s = splhigh();
			dat = dnkbd_pollin(sc, 10000);
			if (dat != -1) {
				if (dnkbd_input(sc, dat) == EVENT_KEYBOARD) {
					splx(s);
					break;
				}
			}
			splx(s);
		}
		lastdat = dat;
	}

	dnkbd_decode(dat, type, data);
}

void
dnkbd_cnpollc(void *v, int on)
{
	struct dnkbd_softc *sc = v;

	if (on)
		SET(sc->sc_flags, SF_POLLING);
	else
		CLR(sc->sc_flags, SF_POLLING);
}

/*
 * Bell routines.
 */
void
dnkbd_bell(void *v, u_int period, u_int pitch, u_int volume)
{
	struct dnkbd_softc *sc = v;
	int s;

	s = spltty();

	if (pitch == 0 || period == 0 || volume == 0) {
		if (ISSET(sc->sc_flags, SF_BELL_TMO)) {
			callout_stop(&sc->sc_bellstop_tmo);
			dnkbd_bellstop(v);
		}
	} else {

		if (!ISSET(sc->sc_flags, SF_BELL)) {
			dnkbd_pollout(sc, DNCMD_PREFIX);
			dnkbd_pollout(sc, DNCMD_BELL);
			dnkbd_pollout(sc, DNCMD_BELL_ON);
			SET(sc->sc_flags, SF_BELL);
		}

		if (ISSET(sc->sc_flags, SF_BELL_TMO))
			callout_stop(&sc->sc_bellstop_tmo);
		callout_schedule(&sc->sc_bellstop_tmo, period);
		SET(sc->sc_flags, SF_BELL_TMO);
	}

	splx(s);
}

void
dnkbd_bellstop(void *v)
{
	struct dnkbd_softc *sc = v;
	int s;

	s = spltty();

	dnkbd_pollout(sc, DNCMD_PREFIX);
	dnkbd_pollout(sc, DNCMD_BELL);
	dnkbd_pollout(sc, DNCMD_BELL_OFF);
	CLR(sc->sc_flags, SF_BELL);
	CLR(sc->sc_flags, SF_BELL_TMO);

	splx(s);
}
