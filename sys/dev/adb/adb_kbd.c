/*	$NetBSD: adb_kbd.c,v 1.15.2.1 2012/04/17 00:07:28 yamt Exp $	*/

/*
 * Copyright (C) 1998	Colin Wood
 * Copyright (C) 2006, 2007 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: adb_kbd.c,v 1.15.2.1 2012/04/17 00:07:28 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wsmousevar.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <machine/autoconf.h>
#include <machine/keyboard.h>
#include <machine/adbsys.h>

#include <dev/adb/adbvar.h>
#include <dev/adb/adb_keymap.h>

#include "opt_wsdisplay_compat.h"
#include "adbdebug.h"
#include "wsmouse.h"

struct adbkbd_softc {
	device_t sc_dev;
	struct adb_device *sc_adbdev;
	struct adb_bus_accessops *sc_ops;
	device_t sc_wskbddev;
#if NWSMOUSE > 0
	device_t sc_wsmousedev;
#endif
	struct sysmon_pswitch sc_sm_pbutton;
	int sc_leds;
	int sc_have_led_control;
	int sc_power_button_delay;
	int sc_msg_len;
	int sc_event;
	int sc_poll;
	int sc_polled_chars;
	int sc_trans[3];
	int sc_capslock;
	uint32_t sc_timestamp;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	int sc_rawkbd;
#endif
	uint8_t sc_buffer[16];
	uint8_t sc_pollbuf[16];
	uint8_t sc_us;
	uint8_t sc_power, sc_pe;
};	

/*
 * Function declarations.
 */
static int	adbkbd_match(device_t, cfdata_t, void *);
static void	adbkbd_attach(device_t, device_t, void *);

static void	adbkbd_initleds(struct adbkbd_softc *);
static void	adbkbd_keys(struct adbkbd_softc *, uint8_t, uint8_t);
static inline void adbkbd_key(struct adbkbd_softc *, uint8_t);
static int	adbkbd_wait(struct adbkbd_softc *, int);

/* Driver definition. */
CFATTACH_DECL_NEW(adbkbd, sizeof(struct adbkbd_softc),
    adbkbd_match, adbkbd_attach, NULL, NULL);

extern struct cfdriver adbkbd_cd;

static int adbkbd_enable(void *, int);
static int adbkbd_ioctl(void *, u_long, void *, int, struct lwp *);
static void adbkbd_set_leds(void *, int);
static void adbkbd_handler(void *, int, uint8_t *);
static void adbkbd_powerbutton(void *);

struct wskbd_accessops adbkbd_accessops = {
	adbkbd_enable,
	adbkbd_set_leds,
	adbkbd_ioctl,
};

static void adbkbd_cngetc(void *, u_int *, int *);
static void adbkbd_cnpollc(void *, int);

struct wskbd_consops adbkbd_consops = {
	adbkbd_cngetc,
	adbkbd_cnpollc,
};

struct wskbd_mapdata adbkbd_keymapdata = {
	akbd_keydesctab,
#ifdef AKBD_LAYOUT
	AKBD_LAYOUT,
#else
	KB_US,
#endif
};

#if NWSMOUSE > 0
static int adbkms_enable(void *);
static int adbkms_ioctl(void *, u_long, void *, int, struct lwp *);
static void adbkms_disable(void *);

const struct wsmouse_accessops adbkms_accessops = {
	adbkms_enable,
	adbkms_ioctl,
	adbkms_disable,
};

static int  adbkbd_sysctl_mid(SYSCTLFN_ARGS);
static int  adbkbd_sysctl_right(SYSCTLFN_ARGS);
static void adbkbd_setup_sysctl(struct adbkbd_softc *);

#endif /* NWSMOUSE > 0 */

#ifdef ADBKBD_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

static int adbkbd_is_console = 0;
static int adbkbd_console_attached = 0;

static int
adbkbd_match(device_t parent, cfdata_t cf, void *aux)
{
	struct adb_attach_args *aaa = aux;

	if (aaa->dev->original_addr == ADBADDR_KBD)
		return 1;
	else
		return 0;
}

static void
adbkbd_attach(device_t parent, device_t self, void *aux)
{
	struct adbkbd_softc *sc = device_private(self);
	struct adb_attach_args *aaa = aux;
	short cmd;
	struct wskbddev_attach_args a;
#if NWSMOUSE > 0
	struct wsmousedev_attach_args am;
#endif

	sc->sc_dev = self;
	sc->sc_ops = aaa->ops;
	sc->sc_adbdev = aaa->dev;
	sc->sc_adbdev->cookie = sc;
	sc->sc_adbdev->handler = adbkbd_handler;
	sc->sc_us = ADBTALK(sc->sc_adbdev->current_addr, 0);

	sc->sc_leds = 0;	/* initially off */
	sc->sc_have_led_control = 0;

	/*
	 * If this is != 0 then pushing the power button will not immadiately
	 * send a shutdown event to sysmon but instead require another key
	 * press within 5 seconds with a gap of at least two seconds. The 
	 * reason to do this is the fact that some PowerBook keyboards,
	 * like the 2400, 3400 and original G3 have their power buttons
	 * right next to the backspace key and it's extremely easy to hit
	 * it by accident.
	 * On most other keyboards the power button is sufficiently far out
	 * of the way so we don't need this.
	 */
	sc->sc_power_button_delay = 0;
	sc->sc_msg_len = 0;
	sc->sc_poll = 0;
	sc->sc_capslock = 0;
	sc->sc_trans[1] = 103;	/* F11 */
	sc->sc_trans[2] = 111;	/* F12 */
	sc->sc_power = 0x7f;
	sc->sc_timestamp = 0;

	printf(" addr %d: ", sc->sc_adbdev->current_addr);

	switch (sc->sc_adbdev->handler_id) {
	case ADB_STDKBD:
		printf("standard keyboard\n");
		break;
	case ADB_ISOKBD:
		printf("standard keyboard (ISO layout)\n");
		break;
	case ADB_EXTKBD:
		cmd = ADBTALK(sc->sc_adbdev->current_addr, 1);
		sc->sc_msg_len = 0;
		sc->sc_ops->send(sc->sc_ops->cookie, sc->sc_poll, cmd, 0, NULL);
		adbkbd_wait(sc, 10);

		/* Ignore Logitech MouseMan/Trackman pseudo keyboard */
		/* XXX needs testing */
		if (sc->sc_buffer[2] == 0x9a && sc->sc_buffer[3] == 0x20) {
			printf("Mouseman (non-EMP) pseudo keyboard\n");
			return;
		} else if (sc->sc_buffer[2] == 0x9a && 
		    sc->sc_buffer[3] == 0x21) {
			printf("Trackman (non-EMP) pseudo keyboard\n");
			return;
		} else {
			printf("extended keyboard\n");
			adbkbd_initleds(sc);
		}
		break;
	case ADB_EXTISOKBD:
		printf("extended keyboard (ISO layout)\n");
		adbkbd_initleds(sc);
		break;
	case ADB_KBDII:
		printf("keyboard II\n");
		break;
	case ADB_ISOKBDII:
		printf("keyboard II (ISO layout)\n");
		break;
	case ADB_PBKBD:
		printf("PowerBook keyboard\n");
		sc->sc_power = 0x7e;
		sc->sc_power_button_delay = 1;
		break;
	case ADB_PBISOKBD:
		printf("PowerBook keyboard (ISO layout)\n");
		sc->sc_power = 0x7e;
		sc->sc_power_button_delay = 1;
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
		sc->sc_power_button_delay = 1;
		sc->sc_power = 0x7e;
		break;
	case ADB_PBEXTJAPKBD:
		printf("PowerBook extended keyboard (Japanese layout)\n");
		sc->sc_power_button_delay = 1;
		sc->sc_power = 0x7e;
		break;
	case ADB_JPKBDII:
		printf("keyboard II (Japanese layout)\n");
		break;
	case ADB_PBEXTKBD:
		printf("PowerBook extended keyboard\n");
		sc->sc_power_button_delay = 1;
		sc->sc_power = 0x7e;
		break;
	case ADB_DESIGNKBD:
		printf("extended keyboard\n");
		adbkbd_initleds(sc);
		break;
	case ADB_PBJPKBD:
		printf("PowerBook keyboard (Japanese layout)\n");
		sc->sc_power_button_delay = 1;
		sc->sc_power = 0x7e;
		break;
	case ADB_PBG3KBD:
		printf("PowerBook G3 keyboard\n");
		sc->sc_power = 0x7e;
		break;
	case ADB_PBG3JPKBD:
		printf("PowerBook G3 keyboard (Japanese layout)\n");
		sc->sc_power = 0x7e;
		break;
	case ADB_IBOOKKBD:
		printf("iBook keyboard\n");
		break;
	default:
		printf("mapped device (%d)\n", sc->sc_adbdev->handler_id);
		break;
	}

	if (adbkbd_is_console && (adbkbd_console_attached == 0)) {
		wskbd_cnattach(&adbkbd_consops, sc, &adbkbd_keymapdata);
		adbkbd_console_attached = 1;
		a.console = 1;
	} else {
		a.console = 0;
	}
	a.keymap = &adbkbd_keymapdata;
	a.accessops = &adbkbd_accessops;
	a.accesscookie = sc;

	sc->sc_wskbddev = config_found_ia(self, "wskbddev", &a, wskbddevprint);

#if NWSMOUSE > 0
	/* attach the mouse device */
	am.accessops = &adbkms_accessops;
	am.accesscookie = sc;
	sc->sc_wsmousedev = config_found_ia(self, "wsmousedev", &am, 
	    wsmousedevprint);

	if (sc->sc_wsmousedev != NULL)
		adbkbd_setup_sysctl(sc);
#endif

	/* finally register the power button */
	sysmon_task_queue_init();
	memset(&sc->sc_sm_pbutton, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_pbutton.smpsw_name = device_xname(sc->sc_dev);
	sc->sc_sm_pbutton.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&sc->sc_sm_pbutton) != 0)
		aprint_error_dev(sc->sc_dev,
		    "unable to register power button with sysmon\n");
}

static void
adbkbd_handler(void *cookie, int len, uint8_t *data)
{
	struct adbkbd_softc *sc = cookie;

#ifdef ADBKBD_DEBUG
	int i;
	printf("%s: %02x - ", device_xname(sc->sc_dev), sc->sc_us);
	for (i = 0; i < len; i++) {
		printf(" %02x", data[i]);
	}
	printf("\n");
#endif
	if (len >= 2) {
		if (data[1] == sc->sc_us) {
			adbkbd_keys(sc, data[2], data[3]);
			return;
		} else {
			memcpy(sc->sc_buffer, data, len);
		}
		sc->sc_msg_len = len;
		wakeup(&sc->sc_event);
	} else {
		DPRINTF("bogus message\n");
	}
}

static int
adbkbd_wait(struct adbkbd_softc *sc, int timeout)
{
	int cnt = 0;
	
	if (sc->sc_poll) {
		while (sc->sc_msg_len == 0) {
			sc->sc_ops->poll(sc->sc_ops->cookie);
		}
	} else {
		while ((sc->sc_msg_len == 0) && (cnt < timeout)) {
			tsleep(&sc->sc_event, 0, "adbkbdio", hz);
			cnt++;
		}
	}
	return (sc->sc_msg_len > 0);
}

static void
adbkbd_keys(struct adbkbd_softc *sc, uint8_t k1, uint8_t k2)
{

	/* keyboard event processing */

	DPRINTF("[%02x %02x]", k1, k2);

	if (((k1 == k2) && (k1 == 0x7f)) || (k1 == sc->sc_power)) {
		uint32_t now = time_second;
		uint32_t diff = now - sc->sc_timestamp;

		sc->sc_timestamp = now;
		if (((diff > 1) && (diff < 5)) ||
		     (sc->sc_power_button_delay == 0)) {

			/* power button, report to sysmon */
			sc->sc_pe = k1;
		
			sysmon_task_queue_sched(0, adbkbd_powerbutton, sc);
		}
	} else {

		adbkbd_key(sc, k1);
		if (k2 != 0xff)
			adbkbd_key(sc, k2);
	}
}

static void
adbkbd_powerbutton(void *cookie)
{
	struct adbkbd_softc *sc = cookie;

	sysmon_pswitch_event(&sc->sc_sm_pbutton, 
	    ADBK_PRESS(sc->sc_pe) ? PSWITCH_EVENT_PRESSED :
	    PSWITCH_EVENT_RELEASED);
}

static inline void
adbkbd_key(struct adbkbd_softc *sc, uint8_t k)
{

	if (sc->sc_poll) {
		if (sc->sc_polled_chars >= 16) {
			aprint_error_dev(sc->sc_dev,"polling buffer is full\n");
		}
		sc->sc_pollbuf[sc->sc_polled_chars] = k;
		sc->sc_polled_chars++;
		return;
	}

#if NWSMOUSE > 0
	/* translate some keys to mouse events */
	if (sc->sc_wsmousedev != NULL) {
		if (ADBK_KEYVAL(k) == sc->sc_trans[1]) {
			wsmouse_input(sc->sc_wsmousedev, ADBK_PRESS(k) ? 2 : 0,
			      0, 0, 0, 0,
			      WSMOUSE_INPUT_DELTA);
			return;
		}			
		if (ADBK_KEYVAL(k) == sc->sc_trans[2]) {
			wsmouse_input(sc->sc_wsmousedev, ADBK_PRESS(k) ? 4 : 0,
			      0, 0, 0, 0,
			      WSMOUSE_INPUT_DELTA);
			return;
		}			
	}
#endif

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (sc->sc_rawkbd) {
		char cbuf[2];
		int s;

		cbuf[0] = k;

		s = spltty();
		wskbd_rawinput(sc->sc_wskbddev, cbuf, 1);
		splx(s);
	} else {
#endif

	if (ADBK_KEYVAL(k) == 0x39) {
		/* caps lock - send up and down */
		if (ADBK_PRESS(k) != sc->sc_capslock) {
			sc->sc_capslock = ADBK_PRESS(k);
			wskbd_input(sc->sc_wskbddev,
			    WSCONS_EVENT_KEY_DOWN, 0x39);
			wskbd_input(sc->sc_wskbddev,
			    WSCONS_EVENT_KEY_UP, 0x39);
		}
	} else {
		/* normal event */
		int type;

		type = ADBK_PRESS(k) ? 
		    WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP;
		wskbd_input(sc->sc_wskbddev, type, ADBK_KEYVAL(k));
	}
#ifdef WSDISPLAY_COMPAT_RAWKBD
	}
#endif
}

/*
 * Set the keyboard LED's.
 * 
 * Automatically translates from ioctl/softc format to the
 * actual keyboard register format
 */
static void
adbkbd_set_leds(void *cookie, int leds)
{
	struct adbkbd_softc *sc = cookie;
	int aleds;
	short cmd;
	uint8_t buffer[2];

	DPRINTF("adbkbd_set_leds: %02x\n", leds);
	if ((leds & 0x07) == (sc->sc_leds & 0x07))
		return;

 	if (sc->sc_have_led_control) {

		aleds = (~leds & 0x04) | 3;
		if (leds & 1)
			aleds &= ~2;
		if (leds & 2)
			aleds &= ~1;

		buffer[0] = 0xff;
		buffer[1] = aleds | 0xf8;
	
		cmd = ADBLISTEN(sc->sc_adbdev->current_addr, 2);
		sc->sc_ops->send(sc->sc_ops->cookie, sc->sc_poll, cmd, 2, 
		    buffer);
	}

	sc->sc_leds = leds & 7;
}

static void 
adbkbd_initleds(struct adbkbd_softc *sc)
{
	short cmd;

	/* talk R2 */
	cmd = ADBTALK(sc->sc_adbdev->current_addr, 2);
	sc->sc_msg_len = 0;
	sc->sc_ops->send(sc->sc_ops->cookie, sc->sc_poll, cmd, 0, NULL);
	if (!adbkbd_wait(sc, 10)) {
		printf("unable to read LED state\n");
		return;
	}
	sc->sc_have_led_control = 1;
	DPRINTF("have LED control\n");	
	return;
}

static int
adbkbd_enable(void *v, int on)
{
	return 0;
}

static int
adbkbd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct adbkbd_softc *sc = (struct adbkbd_softc *) v;

	switch (cmd) {

	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_ADB;
		return 0;
	case WSKBDIO_SETLEDS:
		adbkbd_set_leds(sc, *(int *)data);
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = sc->sc_leds;
		return 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = *(int *)data == WSKBD_RAW;
		return 0;
#endif
	}

	return EPASSTHROUGH;
}

int
adbkbd_cnattach(void)
{

	adbkbd_is_console = 1;
	return 0;
}

static void
adbkbd_cngetc(void *v, u_int *type, int *data)
{
	struct adbkbd_softc *sc = v;
	int key, press, val;
	int s;

	s = splhigh();

	KASSERT(sc->sc_poll);

	DPRINTF("polling...");
	while (sc->sc_polled_chars == 0) {
		sc->sc_ops->poll(sc->sc_ops->cookie);
	}
	DPRINTF(" got one\n");
	splx(s);

	key = sc->sc_pollbuf[0];
	sc->sc_polled_chars--;
	memmove(sc->sc_pollbuf, sc->sc_pollbuf + 1,
		sc->sc_polled_chars);

	press = ADBK_PRESS(key);
	val = ADBK_KEYVAL(key);

	*data = val;
	*type = press ? WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP;
}

static void
adbkbd_cnpollc(void *v, int on)
{
	struct adbkbd_softc *sc = v;

	sc->sc_poll = on;
	if (!on) {
		int i;

		/* feed the poll buffer's content to wskbd */
		for (i = 0; i < sc->sc_polled_chars; i++) {
			adbkbd_key(sc, sc->sc_pollbuf[i]);
		}
		sc->sc_polled_chars = 0;
	}
}

#if NWSMOUSE > 0
/* stuff for the pseudo mouse */
static int
adbkms_enable(void *v)
{
	return 0;
}

static int
adbkms_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PSEUDO;
		break;

	default:
		return (EPASSTHROUGH);
	}
	return (0);
}

static void
adbkms_disable(void *v)
{
}

static void
adbkbd_setup_sysctl(struct adbkbd_softc *sc)
{
	const struct sysctlnode *me, *node;
	int ret;

	DPRINTF("%s: sysctl setup\n", device_xname(sc->sc_dev));
	ret = sysctl_createv(NULL, 0, NULL, &me,
	       CTLFLAG_READWRITE,
	       CTLTYPE_NODE, device_xname(sc->sc_dev), NULL,
	       NULL, 0, NULL, 0,
	       CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	ret = sysctl_createv(NULL, 0, NULL,
	    (void *)&node, 
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "middle", "middle mouse button", adbkbd_sysctl_mid, 
		    1, sc, 0, CTL_MACHDEP, me->sysctl_num, CTL_CREATE, 
		    CTL_EOL);

	ret = sysctl_createv(NULL, 0, NULL, 
	    (void *)&node, 
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "right", "right mouse button", adbkbd_sysctl_right, 
		    2, sc, 0, CTL_MACHDEP, me->sysctl_num, CTL_CREATE, 
		    CTL_EOL);
}

static int
adbkbd_sysctl_mid(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adbkbd_softc *sc=(struct adbkbd_softc *)node.sysctl_data;
	const int *np = newp;
	int reg;

	DPRINTF("adbkbd_sysctl_mid\n");
	reg = sc->sc_trans[1];
	if (np) {
		/* we're asked to write */	
		node.sysctl_data = &reg;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
			
			sc->sc_trans[1] = *(int *)node.sysctl_data;
			return 0;
		}
		return EINVAL;
	} else {
		node.sysctl_data = &reg;
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}
}

static int
adbkbd_sysctl_right(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adbkbd_softc *sc=(struct adbkbd_softc *)node.sysctl_data;
	const int *np = newp;
	int reg;

	DPRINTF("adbkbd_sysctl_right\n");
	reg = sc->sc_trans[2];
	if (np) {
		/* we're asked to write */	
		node.sysctl_data = &reg;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
			
			sc->sc_trans[2] = *(int *)node.sysctl_data;
			return 0;
		}
		return EINVAL;
	} else {
		node.sysctl_data = &reg;
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}
}

SYSCTL_SETUP(sysctl_adbkbdtrans_setup, "adbkbd translator setup")
{

	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
}
#endif /* NWSMOUSE > 0 */
