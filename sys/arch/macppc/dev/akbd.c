/*	$NetBSD: akbd.c,v 1.44.2.1 2012/10/30 17:19:57 yamt Exp $	*/

/*
 * Copyright (C) 1998	Colin Wood
 * All rights reserved.
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
 *	This product includes software developed by Colin Wood.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: akbd.c,v 1.44.2.1 2012/10/30 17:19:57 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/ofw/openfirm.h>

#include <dev/adb/adb_keymap.h>

#include <machine/autoconf.h>
#define KEYBOARD_ARRAY
#include <machine/keyboard.h>

#include <macppc/dev/adbvar.h>
#include <macppc/dev/aedvar.h>
#include <macppc/dev/akbdvar.h>
#include <macppc/dev/pm_direct.h>

#include "aed.h"

/*
 * Function declarations.
 */
static int	akbdmatch(device_t, cfdata_t, void *);
static void	akbdattach(device_t, device_t, void *);
static void	kbd_processevent(adb_event_t *event, struct akbd_softc *);
#ifdef notyet
static u_char	getleds(int);
static int	setleds(struct akbd_softc *, u_char);
static void	blinkleds(struct akbd_softc *);
#endif

/* Driver definition. */
CFATTACH_DECL_NEW(akbd, sizeof(struct akbd_softc),
    akbdmatch, akbdattach, NULL, NULL);

extern struct cfdriver akbd_cd;

int akbd_enable(void *, int);
void akbd_set_leds(void *, int);
int akbd_ioctl(void *, u_long, void *, int, struct lwp *);

struct wskbd_accessops akbd_accessops = {
	akbd_enable,
	akbd_set_leds,
	akbd_ioctl,
};

void akbd_cngetc(void *, u_int *, int *);
void akbd_cnpollc(void *, int);

struct wskbd_consops akbd_consops = {
	akbd_cngetc,
	akbd_cnpollc,
};

struct wskbd_mapdata akbd_keymapdata = {
	akbd_keydesctab,
#ifdef AKBD_LAYOUT
	AKBD_LAYOUT,
#else
	KB_US,
#endif
};

static int akbd_is_console;
static int akbd_console_attached;
static int pcmcia_soft_eject;

static int
akbdmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct adb_attach_args *aa_args = aux;

	if (aa_args->origaddr == ADBADDR_KBD)
		return 1;
	else
		return 0;
}

static void
akbdattach(device_t parent, device_t self, void *aux)
{
	ADBSetInfoBlock adbinfo;
	struct akbd_softc *sc = device_private(self);
	struct adb_attach_args *aa_args = aux;
	int error, kbd_done;
	short cmd;
	u_char buffer[9];
	struct wskbddev_attach_args a;

	/* ohare based models have soft ejectable card slot. */
	if (OF_finddevice("/bandit/ohare") != -1)
		pcmcia_soft_eject = 1;

	sc->origaddr = aa_args->origaddr;
	sc->adbaddr = aa_args->adbaddr;
	sc->handler_id = aa_args->handler_id;

	sc->sc_leds = (u_int8_t)0x00;	/* initially off */

	adbinfo.siServiceRtPtr = (Ptr)kbd_adbcomplete;
	adbinfo.siDataAreaAddr = (void *)sc;

	switch (sc->handler_id) {
	case ADB_STDKBD:
		printf("standard keyboard\n");
		break;
	case ADB_ISOKBD:
		printf("standard keyboard (ISO layout)\n");
		break;
	case ADB_EXTKBD:
		cmd = ADBTALK(sc->adbaddr, 1);
		kbd_done =
		    (adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd) == 0);

		/* Ignore Logitech MouseMan/Trackman pseudo keyboard */
		if (kbd_done && buffer[1] == 0x9a && buffer[2] == 0x20) {
			printf("Mouseman (non-EMP) pseudo keyboard\n");
			adbinfo.siServiceRtPtr = (Ptr)0;
			adbinfo.siDataAreaAddr = (Ptr)0;
		} else if (kbd_done && buffer[1] == 0x9a && buffer[2] == 0x21) {
			printf("Trackman (non-EMP) pseudo keyboard\n");
			adbinfo.siServiceRtPtr = (Ptr)0;
			adbinfo.siDataAreaAddr = (Ptr)0;
		} else {
			printf("extended keyboard\n");
#ifdef notyet
			blinkleds(sc);
#endif
		}
		break;
	case ADB_EXTISOKBD:
		printf("extended keyboard (ISO layout)\n");
#ifdef notyet
		blinkleds(sc);
#endif
		break;
	case ADB_KBDII:
		printf("keyboard II\n");
		break;
	case ADB_ISOKBDII:
		printf("keyboard II (ISO layout)\n");
		break;
	case ADB_PBKBD:
		printf("PowerBook keyboard\n");
		break;
	case ADB_PBISOKBD:
		printf("PowerBook keyboard (ISO layout)\n");
		break;
	case ADB_ADJKPD:
		printf("adjustable keypad\n");
		break;
	case ADB_ADJKBD:
		printf("adjustable keyboard\n");
		break;
	case ADB_ADJISOKBD:
		printf("adjustable keyboard (ISO layout)\n");
		break;
	case ADB_ADJJAPKBD:
		printf("adjustable keyboard (Japanese layout)\n");
		break;
	case ADB_PBEXTISOKBD:
		printf("PowerBook extended keyboard (ISO layout)\n");
		break;
	case ADB_PBEXTJAPKBD:
		printf("PowerBook extended keyboard (Japanese layout)\n");
		break;
	case ADB_JPKBDII:
		printf("keyboard II (Japanese layout)\n");
		break;
	case ADB_PBEXTKBD:
		printf("PowerBook extended keyboard\n");
		break;
	case ADB_DESIGNKBD:
		printf("extended keyboard\n");
#ifdef notyet
		blinkleds(sc);
#endif
		break;
	case ADB_PBJPKBD:
		printf("PowerBook keyboard (Japanese layout)\n");
		break;
	case ADB_PBG3KBD:
		printf("PowerBook G3 keyboard\n");
		break;
	case ADB_PBG3JPKBD:
		printf("PowerBook G3 keyboard (Japanese layout)\n");
		break;
	default:
		printf("mapped device (%d)\n", sc->handler_id);
		break;
	}
	error = SetADBInfo(&adbinfo, sc->adbaddr);
#ifdef ADB_DEBUG
	if (adb_debug)
		printf("akbd: returned %d from SetADBInfo\n", error);
#endif

	if (akbd_is_console && !akbd_console_attached) {
		wskbd_cnattach(&akbd_consops, sc, &akbd_keymapdata);
		akbd_console_attached = 1;
	}

	a.console = akbd_is_console;
	a.keymap = &akbd_keymapdata;
	a.accessops = &akbd_accessops;
	a.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
}


/*
 * Handle putting the keyboard data received from the ADB into
 * an ADB event record.
 */
void 
kbd_adbcomplete(uint8_t *buffer, uint8_t *data_area, int adb_command)
{
	adb_event_t event;
	struct akbd_softc *ksc;
	int adbaddr;
#ifdef ADB_DEBUG
	int i;

	if (adb_debug)
		printf("adb: transaction completion\n");
#endif

	adbaddr = ADB_CMDADDR(adb_command);
	ksc = (struct akbd_softc *)data_area;

	event.addr = adbaddr;
	event.hand_id = ksc->handler_id;
	event.def_addr = ksc->origaddr;
	event.byte_count = buffer[0];
	memcpy(event.bytes, buffer + 1, event.byte_count);

#ifdef ADB_DEBUG
	if (adb_debug) {
		printf("akbd: from %d at %d (org %d) %d:", event.addr,
		    event.hand_id, event.def_addr, buffer[0]);
		for (i = 1; i <= buffer[0]; i++)
			printf(" %x", buffer[i]);
		printf("\n");
	}
#endif

	microtime(&event.timestamp);

	kbd_processevent(&event, ksc);
}

/*
 * Given a keyboard ADB event, record the keycodes and call the key 
 * repeat handler, optionally passing the event through the mouse
 * button emulation handler first.
 */
static void
kbd_processevent(adb_event_t *event, struct akbd_softc *ksc)
{
        adb_event_t new_event;

        new_event = *event;
	new_event.u.k.key = event->bytes[0];
	new_event.bytes[1] = 0xff;
	kbd_intr(&new_event);
#if NAED > 0
	aed_input(&new_event);
#endif
	if (event->bytes[1] != 0xff) {
		new_event.u.k.key = event->bytes[1];
		new_event.bytes[0] = event->bytes[1];
		new_event.bytes[1] = 0xff;
		kbd_intr(&new_event);
#if NAED > 0
		aed_input(&new_event);
#endif
	}

}

#ifdef notyet
/*
 * Get the actual hardware LED state and convert it to softc format.
 */
static u_char
getleds(int addr)
{
	short cmd;
	u_char buffer[9], leds;

	leds = 0x00;	/* all off */
	buffer[0] = 0;

	/* talk R2 */
	cmd = ADBTALK(addr, 2);
	if (adb_op_sync((Ptr)buffer, (Ptr)0, (Ptr)0, cmd) == 0 &&
	    buffer[0] > 0)
		leds = ~(buffer[2]) & 0x07;

	return (leds);
}

/*
 * Set the keyboard LED's.
 * 
 * Automatically translates from ioctl/softc format to the
 * actual keyboard register format
 */
static int 
setleds(struct akbd_softc *ksc, u_char leds)
{
	int addr;
	short cmd;
	u_char buffer[9];

	if ((leds & 0x07) == (ksc->sc_leds & 0x07))
		return (0);

	addr = ksc->adbaddr;
	buffer[0] = 0;

	cmd = ADBTALK(addr, 2);
	if (adb_op_sync((Ptr)buffer, (Ptr)0, (Ptr)0, cmd) || buffer[0] == 0)
		return (EIO);

	leds = ~leds & 0x07;
	buffer[2] &= 0xf8;
	buffer[2] |= leds;

	cmd = ADBLISTEN(addr, 2);
	adb_op_sync((Ptr)buffer, (Ptr)0, (Ptr)0, cmd);

	cmd = ADBTALK(addr, 2);
	if (adb_op_sync((Ptr)buffer, (Ptr)0, (Ptr)0, cmd) || buffer[0] == 0)
		return (EIO);

	ksc->sc_leds = ~((u_int8_t)buffer[2]) & 0x07;

	if ((buffer[2] & 0xf8) != leds)
		return (EIO);
	else
		return (0); 
}

/*
 * Toggle all of the LED's on and off, just for show.
 */
static void 
blinkleds(struct akbd_softc *ksc)
{
	int addr, i;
	u_char blinkleds, origleds;

	addr = ksc->adbaddr;
	origleds = getleds(addr);
	blinkleds = LED_NUMLOCK | LED_CAPSLOCK | LED_SCROLL_LOCK;

	(void)setleds(ksc, blinkleds);

	for (i = 0; i < 10000; i++)
		delay(50);

	/* make sure that we restore the LED settings */
	i = 10;
	do {
		(void)setleds(ksc, (u_char)0x00);
	} while (setleds(ksc, (u_char)0x00) && (i-- > 0)); 

	return;
}
#endif

int
akbd_enable(void *v, int on)
{
	return 0;
}

void
akbd_set_leds(void *v, int on)
{
}

int
akbd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
#ifdef WSDISPLAY_COMPAT_RAWKBD
	struct akbd_softc *sc = (struct akbd_softc *) v;
#endif

	switch (cmd) {

	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_ADB;
		return 0;
	case WSKBDIO_SETLEDS:
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = *(int *)data == WSKBD_RAW;
		return 0;
#endif
	}
	/* kbdioctl(...); */

	return EPASSTHROUGH;
}

extern int adb_polling;

void
kbd_passup(struct akbd_softc *sc,int key)
{
	if (sc->sc_polling) {
		if (sc->sc_npolledkeys <
			(sizeof(sc->sc_polledkeys)/sizeof(unsigned char))) {
			sc->sc_polledkeys[sc->sc_npolledkeys++] = key;
		}
#ifdef ADB_DEBUG
		else {
			printf("akbd: dumping polled key 0x%02x\n",key);
		}
#endif
#ifdef WSDISPLAY_COMPAT_RAWKBD
	} else if (sc->sc_rawkbd) {
		char cbuf[2];
		int s;
		int j = 0;
		int c = keyboard[ADBK_KEYVAL(key)][3];

		if (c == 0)			/* XXX */
			return;

		if (c & 0x80)
			cbuf[j++] = 0xe0;

		cbuf[j++] = (c & 0x7f) | (ADBK_PRESS(key)? 0 : 0x80);

		s = spltty();
		wskbd_rawinput(sc->sc_wskbddev, cbuf, j);
		splx(s);
#endif
	} else {
		int press, val;
		int type;
    
		press = ADBK_PRESS(key);
		val = ADBK_KEYVAL(key);
    
		type = press ? WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP;

		wskbd_input(sc->sc_wskbddev, type, val);
	}
}

int
kbd_intr(void *arg)
{
	adb_event_t *event = arg;
	int key;
#ifdef CAPS_IS_CONTROL
	static int shift;
#endif

	struct akbd_softc *sc = device_lookup_private(&akbd_cd, 0);

	key = event->u.k.key;

#ifdef CAPS_IS_CONTROL
	/*
	 * Caps lock is weird. The key sequence generated is:
	 * press:   down(57) [57]  (LED turns on)
	 * release: up(127)  [255]
	 * press:   up(127)  [255]
	 * release: up(57)   [185] (LED turns off)
	 */
	if ((key == 57) || (key == 185))
		shift = 0;
	
	if (key == 255) {
		if (shift == 0) {
			key = 185;
			shift = 1;
		} else {
			key = 57;
			shift = 0;
		}
	}
#endif

	switch (key) {
#ifndef CAPS_IS_CONTROL
	case 57:	/* Caps Lock pressed */
	case 185:	/* Caps Lock released */
		key = ADBK_KEYDOWN(ADBK_KEYVAL(key));
		kbd_passup(sc,key);
		key = ADBK_KEYUP(ADBK_KEYVAL(key));
		break;
#endif
	case 245:
		if (pcmcia_soft_eject)
			pm_eject_pcmcia(0);
		break;
	case 244:
		if (pcmcia_soft_eject)
			pm_eject_pcmcia(1);
		break;
	}

	kbd_passup(sc,key);

	return 0;
}

int
akbd_cnattach(void)
{

	akbd_is_console = 1;
	return 0;
}

void
akbd_cngetc(void *v, u_int *type, int *data)
{
	int key, press, val;
	int s;
	struct akbd_softc *sc = v;

	s = splhigh();

	KASSERT(sc->sc_polling);
	KASSERT(adb_polling);

	while (sc->sc_npolledkeys == 0) {
		adb_intr(NULL);
		DELAY(10000);				/* XXX */
	}

	splx(s);

	key = sc->sc_polledkeys[0];
	sc->sc_npolledkeys--;
	memmove(sc->sc_polledkeys,sc->sc_polledkeys+1,
		sc->sc_npolledkeys * sizeof(unsigned char));

	press = ADBK_PRESS(key);
	val = ADBK_KEYVAL(key);

	*data = val;
	*type = press ? WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP;
}

void
akbd_cnpollc(void *v, int on)
{
	struct akbd_softc *sc = v;
	sc->sc_polling = on;
	if (!on) {
		int i;
		for(i=0;i<sc->sc_npolledkeys;i++) {
			kbd_passup(sc,sc->sc_polledkeys[i]);
		}
		sc->sc_npolledkeys = 0;
	}
	adb_polling = on;
}
