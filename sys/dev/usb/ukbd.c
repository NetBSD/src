/*	$NetBSD: ukbd.c,v 1.2 1998/07/25 15:36:30 augustss Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author: Lennart Augustsson <augustss@carlstedt.se>
 *         Carlstedt Research & Technology
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


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/poll.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>
#include <dev/usb/hid.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wskbdmap_mfii.h>

#include "opt_pckbd_layout.h"
#include "opt_wsdisplay_compat.h"

#ifdef USB_DEBUG
#define DPRINTF(x)	if (ukbddebug) printf x
#define DPRINTFN(n,x)	if (ukbddebug>(n)) printf x
int	ukbddebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define UPROTO_BOOT_KEYBOARD 1

#define NKEYCODE 6

#define NUM_LOCK 0x01
#define CAPS_LOCK 0x02
#define SCROLL_LOCK 0x04

struct ukbd_data {
	u_int8_t	modifiers;
#define MOD_CONTROL_L	0x01
#define MOD_CONTROL_R	0x10
#define MOD_SHIFT_L	0x02
#define MOD_SHIFT_R	0x20
#define MOD_ALT_L	0x04
#define MOD_ALT_R	0x40
#define MOD_WIN_L	0x08
#define MOD_WIN_R	0x80
	u_int8_t	reserved;
	u_int8_t	keycode[NKEYCODE];
};

#define PRESS 0
#define RELEASE 0x80

#define NMOD 6
static struct {
	int mask, key;
} ukbd_mods[NMOD] = {
	{ MOD_CONTROL_L, 29 },
	{ MOD_CONTROL_R, 58 },
	{ MOD_SHIFT_L,   42 },
	{ MOD_SHIFT_R,   54 },
	{ MOD_ALT_L,     56 },
	{ MOD_ALT_R,     56 },
};

#define XX 0
/* Translate USB keycodes to US keyboard scancodes. */
/* XXX very incomplete */
static char ukbd_trtab[256] = {
	   0,   0,   0,   0,  30,  48,  46,  32,
	  18,  33,  34,  35,  23,  36,  37,  38,
	  50,  49,  24,  25,  16,  19,  31,  20,
	  22,  47,  17,  45,  21,  44,   2,   3,
	   4,   5,   6,   7,   8,   9,  10,  11,
	  28,   1,  14,  15,  57,  12,  13,  26,
	  27,  43,  XX,  39,  40,  41,  51,  52,
	  53,  58,  59,  60,  61,  62,  63,  64,
	  65,  66,  67,  68,  87,  88,  XX,  70,
	  XX,  XX,  XX,  XX,  XX,
};

#define KEY_ERROR 0x01

struct ukbd_softc {
	struct device sc_dev;		/* base device */
	usbd_interface_handle sc_iface;	/* interface */
	usbd_pipe_handle sc_intrpipe;	/* interrupt pipe */
	int sc_ep_addr;

	struct ukbd_data sc_ndata;
	struct ukbd_data sc_odata;

	int sc_state;
#define UKBD_NEEDCLEAR	0x01		/* needs clearing endpoint stall */
	int sc_disconnected;		/* device is gone */

	int sc_leds;
	struct device *sc_wskbddev;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	int sc_rawkbd;
#endif
};

#define	UKBDUNIT(dev)	(minor(dev))
#define	UKBD_CHUNK	128	/* chunk size for read */
#define	UKBD_BSIZE	1020	/* buffer size */

int	ukbd_match __P((struct device *, struct cfdata *, void *));
void	ukbd_attach __P((struct device *, struct device *, void *));

void	ukbd_intr __P((usbd_request_handle, usbd_private_handle, usbd_status));
void	ukbd_disco __P((void *));

void	ukbd_set_leds __P((void *, int));
int	ukbd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));

extern struct cfdriver ukbd_cd;

struct cfattach ukbd_ca = {
	sizeof(struct ukbd_softc), ukbd_match, ukbd_attach
};

int
ukbd_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct usb_attach_arg *uaa = (struct usb_attach_arg *)aux;
	usb_interface_descriptor_t *id;
	
	/* Check that this is a keyboard that speaks the boot protocol. */
	if (!uaa->iface)
		return (UMATCH_NONE);
	id = usbd_get_interface_descriptor(uaa->iface);
	if (id->bInterfaceClass != UCLASS_HID || 
	    id->bInterfaceSubClass != USUBCLASS_BOOT ||
	    id->bInterfaceProtocol != UPROTO_BOOT_KEYBOARD)
		return (UMATCH_NONE);
	return (UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO);
}

void
ukbd_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ukbd_softc *sc = (struct ukbd_softc *)self;
	struct usb_attach_arg *uaa = aux;
	usbd_interface_handle iface = uaa->iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	usbd_status r;
	char devinfo[1024];
	struct wskbddev_attach_args a;
	
	sc->sc_disconnected = 1;
	sc->sc_iface = iface;
	id = usbd_get_interface_descriptor(iface);
	usbd_devinfo(uaa->device, 0, devinfo);
	printf(": %s (interface class %d/%d)\n", devinfo,
	       id->bInterfaceClass, id->bInterfaceSubClass);
	ed = usbd_interface2endpoint_descriptor(iface, 0);
	if (!ed) {
		printf("%s: could not read endpoint descriptor\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	DPRINTFN(10,("ukbd_attach: \
bLength=%d bDescriptorType=%d bEndpointAddress=%d-%s bmAttributes=%d wMaxPacketSize=%d bInterval=%d\n",
	       ed->bLength, ed->bDescriptorType, ed->bEndpointAddress & UE_ADDR,
	       ed->bEndpointAddress & UE_IN ? "in" : "out",
	       ed->bmAttributes & UE_XFERTYPE,
	       UGETW(ed->wMaxPacketSize), ed->bInterval));

	if ((ed->bEndpointAddress & UE_IN) != UE_IN ||
	    (ed->bmAttributes & UE_XFERTYPE) != UE_INTERRUPT) {
		printf("%s: unexpected endpoint\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	if ((usbd_get_quirks(uaa->device)->uq_flags & UQ_NO_SET_PROTO) == 0) {
		r = usbd_set_protocol(iface, 0);
		DPRINTFN(5, ("ukbd_attach: protocol set\n"));
		if (r != USBD_NORMAL_COMPLETION) {
			printf("%s: set protocol failed\n",
			       sc->sc_dev.dv_xname);
			return;
		}
	}

	sc->sc_ep_addr = ed->bEndpointAddress;
	sc->sc_disconnected = 0;

	a.console = 0;	/* XXX */
#ifdef PCKBD_LAYOUT
	a.layout = PCKBD_LAYOUT;
#else
	a.layout = KB_US;
#endif
	a.keydesc = pckbd_keydesctab;
	a.num_keydescs = sizeof(pckbd_keydesctab)/sizeof(pckbd_keydesctab[0]);
	a.getc = NULL;
	a.pollc = NULL;
	a.set_leds = ukbd_set_leds;
	a.ioctl = ukbd_ioctl;
	a.accesscookie = sc;
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);

	/* Set up interrupt pipe. */
	r = usbd_open_pipe_intr(sc->sc_iface, sc->sc_ep_addr, 
				USBD_SHORT_XFER_OK,
				&sc->sc_intrpipe, sc, &sc->sc_ndata, 
				sizeof(sc->sc_ndata), ukbd_intr);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: usbd_open_pipe_intr failed, error=%d\n",
		       sc->sc_dev.dv_xname, r);
		return;
	}
	usbd_set_disco(sc->sc_intrpipe, ukbd_disco, sc);
}

void
ukbd_disco(p)
	void *p;
{
	struct ukbd_softc *sc = p;
	sc->sc_disconnected = 1;
}

void
ukbd_intr(reqh, addr, status)
	usbd_request_handle reqh;
	usbd_private_handle addr;
	usbd_status status;
{
	struct ukbd_softc *sc = addr;
	struct ukbd_data *ud = &sc->sc_ndata;
	int mod, omod;
	char ibuf[NMOD+2*NKEYCODE];	/* chars events */
	int nkeys, i, j;
	int key, c;
#define ADDKEY(c) ibuf[nkeys++] = (c)

	DPRINTFN(5, ("ukbd_intr: status=%d\n", status));
	if (status == USBD_CANCELLED)
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		DPRINTF(("ukbd_intr: status=%d\n", status));
		sc->sc_state |= UKBD_NEEDCLEAR;
		return;
	}

	DPRINTFN(5, ("          mod=0x%02x key0=0x%02x key1=0x%02x\n",
		     ud->modifiers, ud->keycode[0], ud->keycode[1]));

	if (ud->keycode[0] == KEY_ERROR)
		return;		/* ignore  */
	nkeys = 0;
	mod = ud->modifiers;
	omod = sc->sc_odata.modifiers;
	if (mod != omod)
		for (i = 0; i < NMOD; i++)
			if (( mod & ukbd_mods[i].mask) != 
			    (omod & ukbd_mods[i].mask))
				ADDKEY(ukbd_mods[i].key | 
				       (mod & ukbd_mods[i].mask 
					  ? PRESS : RELEASE));
	if (memcmp(ud->keycode, sc->sc_odata.keycode, NKEYCODE) != 0) {
		/* Check for released keys. */
		for (i = 0; i < NKEYCODE; i++) {
			key = sc->sc_odata.keycode[i];
			if (key == 0)
				continue;
			for (j = 0; j < NKEYCODE; j++)
				if (key == ud->keycode[j])
					goto rfound;
			c = ukbd_trtab[key];
			if (c)
				ADDKEY(c | RELEASE);
		rfound:
			;
		}
		
		/* Check for pressed keys. */
		for (i = 0; i < NKEYCODE; i++) {
			key = ud->keycode[i];
			if (key == 0)
				continue;
			for (j = 0; j < NKEYCODE; j++)
				if (key == sc->sc_odata.keycode[j])
					goto pfound;
			c = ukbd_trtab[key];
			if (c)
				ADDKEY(c | PRESS);
		pfound:
			;
		}
	}
	sc->sc_odata = *ud;

	for (i = 0; i < nkeys; i++) {
		c = ibuf[i];
		wskbd_input(sc->sc_wskbddev, 
			    c&0x80 ? WSCONS_EVENT_KEY_UP:WSCONS_EVENT_KEY_DOWN,
			    c & 0x7f);
	}
}

void
ukbd_set_leds(v, leds)
	void *v;
	int leds;
{
	struct ukbd_softc *sc = v;
	u_int8_t res;

	DPRINTF(("ukbd_set_leds: sc=%p leds=%d\n", sc, leds));

	sc->sc_leds = leds;
	res = 0;
	if (leds & WSKBD_LED_SCROLL)
		res |= SCROLL_LOCK;
	if (leds & WSKBD_LED_NUM)
		res |= NUM_LOCK;
	if (leds & WSKBD_LED_CAPS)
		res |= CAPS_LOCK;
	usbd_set_report(sc->sc_iface, UHID_OUTPUT_REPORT, 0, &res, 1);
}

int
ukbd_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct ukbd_softc *sc = v;

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_PC_XT;
		return 0;
	case WSKBDIO_SETLEDS:
		ukbd_set_leds(v, *(int *)data);
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = sc->sc_leds;
		return (0);
#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = *(int *)data == WSKBD_RAW;
		return (0);
#endif
	}
	return -1;
}
 
