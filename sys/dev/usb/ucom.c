/*	$NetBSD: ucom.c,v 1.2 1998/12/09 00:18:10 augustss Exp $	*/

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

#ifdef USB_DEBUG
#define DPRINTF(x)	if (ucomdebug) printf x
#define DPRINTFN(n,x)	if (ucomdebug>(n)) printf x
int	ucomdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

struct ucom_softc {
	struct device sc_dev;		/* base device */
	usbd_interface_handle sc_iface;	/* interface */
};

int ucom_match __P((struct device *, struct cfdata *, void *));
void ucom_attach __P((struct device *, struct device *, void *));

void ucom_intr __P((usbd_request_handle, usbd_private_handle, usbd_status));
void ucom_disco __P((void *));

extern struct cfdriver ucom_cd;

struct cfattach ucom_ca = {
	sizeof(struct ucom_softc), ucom_match, ucom_attach
};

int
ucom_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct usb_attach_arg *uaa = aux;
	usb_interface_descriptor_t *id;
	
	if (!uaa->iface)
		return (UMATCH_NONE);
	id = usbd_get_interface_descriptor(uaa->iface);
	if (id &&
	    id->bInterfaceClass != UCLASS_CDC ||
	    id->bInterfaceSubClass != USUBCLASS_MODEM)
		return (UMATCH_NONE);
	return (UMATCH_IFACECLASS_IFACESUBCLASS);
}

void
ucom_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ucom_softc *sc = (struct ucom_softc *)self;
	struct usb_attach_arg *uaa = aux;
	usbd_interface_handle iface = uaa->iface;
	usb_interface_descriptor_t *id;
	char devinfo[1024];
	
	sc->sc_iface = iface;
	id = usbd_get_interface_descriptor(iface);
	usbd_devinfo(uaa->device, 0, devinfo);
	printf("\n%s: %s, iclass %d/%d\n", sc->sc_dev.dv_xname,
	       devinfo, id->bInterfaceClass, id->bInterfaceSubClass);
}
