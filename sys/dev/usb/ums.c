/*	$NetBSD: ums.c,v 1.14 1998/12/09 00:18:11 augustss Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@carlstedt.se) at
 * Carlstedt Research & Technology.
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
#include <dev/wscons/wsmousevar.h>

#ifdef USB_DEBUG
#define DPRINTF(x)	if (umsdebug) printf x
#define DPRINTFN(n,x)	if (umsdebug>(n)) printf x
int	umsdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define PS2LBUTMASK 0x01
#define PS2RBUTMASK 0x02
#define PS2MBUTMASK 0x04
#define PS2BUTMASK 0x0f

struct ums_softc {
	struct device sc_dev;		/* base device */
	usbd_interface_handle sc_iface;	/* interface */
	usbd_pipe_handle sc_intrpipe;	/* interrupt pipe */
	int sc_ep_addr;

	u_char *sc_ibuf;
	u_int8_t sc_iid;
	int sc_isize;
	struct hid_location sc_loc_x, sc_loc_y, sc_loc_z,
		sc_loc_btn1, sc_loc_btn2, sc_loc_btn3;

	u_char sc_buttons;	/* mouse button status */
	int sc_disconnected;	/* device is gone */

	int sc_enabled;
	struct device *sc_wsmousedev;
};

#define MOUSE_FLAGS_MASK (HIO_CONST|HIO_RELATIVE)
#define MOUSE_FLAGS (HIO_RELATIVE)

int ums_match __P((struct device *, struct cfdata *, void *));
void ums_attach __P((struct device *, struct device *, void *));

void ums_intr __P((usbd_request_handle, usbd_private_handle, usbd_status));
void ums_disco __P((void *));

int	ums_enable __P((void *));
int	ums_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
void	ums_disable __P((void *));

const struct wsmouse_accessops ums_accessops = {
	ums_enable,
	ums_ioctl,
	ums_disable,
};

extern struct cfdriver ums_cd;

struct cfattach ums_ca = {
	sizeof(struct ums_softc), ums_match, ums_attach
};

int
ums_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct usb_attach_arg *uaa = aux;
	usb_interface_descriptor_t *id;
	int size, ret;
	void *desc;
	usbd_status r;
	
	if (!uaa->iface)
		return (UMATCH_NONE);
	id = usbd_get_interface_descriptor(uaa->iface);
	if (!id || id->bInterfaceClass != UCLASS_HID)
		return (UMATCH_NONE);

	r = usbd_alloc_report_desc(uaa->iface, &desc, &size, M_TEMP);
	if (r != USBD_NORMAL_COMPLETION)
		return (UMATCH_NONE);

	if (hid_is_collection(desc, size, 
			      HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_MOUSE)))
		ret = UMATCH_IFACECLASS;
	else
		ret = UMATCH_NONE;

	free(desc, M_TEMP);
	return (ret);
}

void
ums_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ums_softc *sc = (struct ums_softc *)self;
	struct usb_attach_arg *uaa = aux;
	usbd_interface_handle iface = uaa->iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	struct wsmousedev_attach_args a;
	int size;
	void *desc;
	usbd_status r;
	char devinfo[1024];
	u_int32_t flags;
	
	sc->sc_disconnected = 1;
	sc->sc_iface = iface;
	id = usbd_get_interface_descriptor(iface);
	usbd_devinfo(uaa->device, 0, devinfo);
	printf("\n%s: %s, iclass %d/%d\n", sc->sc_dev.dv_xname,
	       devinfo, id->bInterfaceClass, id->bInterfaceSubClass);
	ed = usbd_interface2endpoint_descriptor(iface, 0);
	if (!ed) {
		printf("%s: could not read endpoint descriptor\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	DPRINTFN(10,("ums_attach: \
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

	r = usbd_alloc_report_desc(uaa->iface, &desc, &size, M_TEMP);
	if (r != USBD_NORMAL_COMPLETION)
		return;
	if (!hid_locate(desc, size, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_X),
		       hid_input, &sc->sc_loc_x, &flags)) {
		printf("%s: mouse has no X report\n", sc->sc_dev.dv_xname);
		return;
	}
	if ((flags & MOUSE_FLAGS_MASK) != MOUSE_FLAGS)
		printf("%s: sorry, X report 0x%04x not supported yet\n",
		       sc->sc_dev.dv_xname, flags);

	if (!hid_locate(desc, size, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Y),
		       hid_input, &sc->sc_loc_y, &flags)) {
		printf("%s: mouse has no Y report\n", sc->sc_dev.dv_xname);
		return;
	}
	if ((flags & MOUSE_FLAGS_MASK) != MOUSE_FLAGS)
		printf("%s: sorry, Y report 0x%04x not supported yet\n",
		       sc->sc_dev.dv_xname, flags);

	if (hid_locate(desc, size, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Z),
		       hid_input, &sc->sc_loc_z, &flags) ||
	    hid_locate(desc, size, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_WHEEL),
		       hid_input, &sc->sc_loc_z, &flags)) {
		if ((flags & MOUSE_FLAGS_MASK) != MOUSE_FLAGS)
			/* Bad Z coord, ignore it */
			sc->sc_loc_z.size = 0;
	}

	hid_locate(desc, size, HID_USAGE2(HUP_BUTTON, 1), /* left */
		   hid_input, &sc->sc_loc_btn1, 0);
	hid_locate(desc, size, HID_USAGE2(HUP_BUTTON, 2), /* right */
		   hid_input, &sc->sc_loc_btn2, 0);
	hid_locate(desc, size, HID_USAGE2(HUP_BUTTON, 3), /* middle */
		   hid_input, &sc->sc_loc_btn3, 0);
	DPRINTF(("ums_attach: sc=%p\n", sc));
	DPRINTF(("ums_attach: X  %d/%d\n", 
		 sc->sc_loc_x.pos, sc->sc_loc_x.size));
	DPRINTF(("ums_attach: Y  %d/%d\n", 
		 sc->sc_loc_x.pos, sc->sc_loc_x.size));
	DPRINTF(("ums_attach: Z  %d/%d\n", 
		 sc->sc_loc_z.pos, sc->sc_loc_z.size));
	DPRINTF(("ums_attach: B1 %d/%d\n", 
		 sc->sc_loc_btn1.pos,sc->sc_loc_btn1.size));
	DPRINTF(("ums_attach: B2 %d/%d\n", 
		 sc->sc_loc_btn2.pos,sc->sc_loc_btn2.size));
	DPRINTF(("ums_attach: B3 %d/%d\n", 
		 sc->sc_loc_btn3.pos,sc->sc_loc_btn3.size));

	sc->sc_isize = hid_report_size(desc, size, hid_input, &sc->sc_iid);
	DPRINTF(("ums_attach: size=%d, id=%d\n", sc->sc_isize, sc->sc_iid));
	sc->sc_ibuf = malloc(sc->sc_isize, M_USB, M_NOWAIT);
	if (!sc->sc_ibuf)
		return;

	sc->sc_ep_addr = ed->bEndpointAddress;
	sc->sc_disconnected = 0;
	free(desc, M_TEMP);

	a.accessops = &ums_accessops;
	a.accesscookie = sc;

	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);
}

void
ums_disco(p)
	void *p;
{
	struct ums_softc *sc = p;

	DPRINTF(("ums_disco: sc=%p\n", sc));
	usbd_abort_pipe(sc->sc_intrpipe);
	sc->sc_disconnected = 1;
}

void
ums_intr(reqh, addr, status)
	usbd_request_handle reqh;
	usbd_private_handle addr;
	usbd_status status;
{
	struct ums_softc *sc = addr;
	u_char *ibuf;
	int dx, dy, dz;
	u_char buttons;

	DPRINTFN(5, ("ums_intr: sc=%p status=%d\n", sc, status));
	DPRINTFN(5, ("ums_intr: data = %02x %02x %02x\n",
		     sc->sc_ibuf[0], sc->sc_ibuf[1], sc->sc_ibuf[2]));

	if (status == USBD_CANCELLED)
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		DPRINTF(("ums_intr: status=%d\n", status));
		usbd_clear_endpoint_stall_async(sc->sc_intrpipe);
		return;
	}

	ibuf = sc->sc_ibuf;
	if (sc->sc_iid) {
		if (*ibuf++ != sc->sc_iid)
			return;
	}
	dx =  hid_get_data(ibuf, &sc->sc_loc_x);
	dy = -hid_get_data(ibuf, &sc->sc_loc_y);
	dz =  hid_get_data(ibuf, &sc->sc_loc_z);
	buttons = 0;
	if (hid_get_data(ibuf, &sc->sc_loc_btn1)) buttons |= 1 << 0;
	if (hid_get_data(ibuf, &sc->sc_loc_btn2)) buttons |= 1 << 2;
	if (hid_get_data(ibuf, &sc->sc_loc_btn3)) buttons |= 1 << 1;

	if (dx || dy || buttons != sc->sc_buttons) {
		sc->sc_buttons = buttons;
		if (sc->sc_wsmousedev)
			wsmouse_input(sc->sc_wsmousedev, buttons, dx, dy, dz);
	}
}

/* wscons routines */
int
ums_enable(v)
	void *v;
{
	struct ums_softc *sc = v;
	usbd_status r;

	if (sc->sc_enabled)
		return EBUSY;

	sc->sc_enabled = 1;
	sc->sc_buttons = 0;

	/* Set up interrupt pipe. */
	r = usbd_open_pipe_intr(sc->sc_iface, sc->sc_ep_addr, 
				USBD_SHORT_XFER_OK, &sc->sc_intrpipe, sc, 
				sc->sc_ibuf, sc->sc_isize, ums_intr);
	if (r != USBD_NORMAL_COMPLETION) {
		DPRINTF(("ums_enable: usbd_open_pipe_intr failed, error=%d\n",
			 r));
		sc->sc_enabled = 0;
		return (EIO);
	}
	usbd_set_disco(sc->sc_intrpipe, ums_disco, sc);
	return (0);
}

void
ums_disable(v)
	void *v;
{
	struct ums_softc *sc = v;

	/* Disable interrupts. */
	usbd_abort_pipe(sc->sc_intrpipe);
	usbd_close_pipe(sc->sc_intrpipe);

	sc->sc_enabled = 0;
}

int
ums_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
#if 0
	struct ums_softc *sc = v;
#endif

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PS2;
		return (0);
	}
	return (-1);
}

