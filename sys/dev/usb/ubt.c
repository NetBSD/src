/*	$NetBSD: ubt.c,v 1.3 2002/08/24 17:31:19 augustss Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ubt.c,v 1.3 2002/08/24 17:31:19 augustss Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/bluetooth/bluetooth.h>

#define UBT_DEBUG

#ifdef UBT_DEBUG
#define DPRINTF(x)	if (ubtdebug) logprintf x
#define DPRINTFN(n,x)	if (ubtdebug>(n)) logprintf x
int	ubtdebug = 1;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

/*
 * Protocol related definitions
 */

struct ubt_softc {
 	USBBASEDEVICE		sc_dev;
	usbd_device_handle	sc_udev;
	usbd_interface_handle	sc_ctl_iface;
	usbd_interface_handle	sc_isoc_iface;

	int			sc_rd_addr;
	usbd_pipe_handle	sc_rd_pipe;

	int			sc_wr_addr;
	usbd_pipe_handle	sc_wr_pipe;

	struct device		*sc_child;
	int			sc_refcnt;
	char			sc_dying;
};

USB_DECLARE_DRIVER(ubt);

USB_MATCH(ubt)
{
	USB_MATCH_START(ubt, uaa);
	usb_interface_descriptor_t *id;

	DPRINTFN(50,("ubt_match\n"));

	if (uaa->iface == NULL)
		return (UMATCH_NONE);

	id = usbd_get_interface_descriptor(uaa->iface);
	if (id != NULL &&
	    id->bInterfaceClass == UICLASS_WIRELESS &&
	    id->bInterfaceSubClass == UISUBCLASS_RF &&
	    id->bInterfaceProtocol == UIPROTO_BLUETOOTH)
		return (UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO);
	return (UMATCH_NONE);
}

USB_ATTACH(ubt)
{
	USB_ATTACH_START(ubt, sc, uaa);
	usbd_device_handle	dev = uaa->device;
	usbd_interface_handle	iface = uaa->iface;
	struct bt_attach_args	bt;
	usb_interface_descriptor_t *id;
	char			devinfo[1024];
	usb_endpoint_descriptor_t *ed;
	u_int8_t		epcount;
	int			i;

	DPRINTFN(10,("ubt_attach: sc=%p\n", sc));

	usbd_devinfo(dev, 0, devinfo);
	USB_ATTACH_SETUP;
	printf("%s: %s\n", USBDEVNAME(sc->sc_dev), devinfo);

	sc->sc_udev = dev;
	sc->sc_ctl_iface = iface;

	/* 
	 * The control interface comes before the isoc interface
	 * according to the spec, so we find it first.
	 */
	epcount = 0;
	(void)usbd_endpoint_count(iface, &epcount);
	sc->sc_rd_addr = -1;
	sc->sc_wr_addr = -1;
	for (i = 0; i < epcount; i++) {
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			printf("%s: couldn't get ep %d\n",
			    USBDEVNAME(sc->sc_dev), i);
			USB_ATTACH_ERROR_RETURN;
		}
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->sc_rd_addr = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->sc_wr_addr = ed->bEndpointAddress;
		}
	}
#if 0
	if (sc->sc_rd_addr == -1 || sc->sc_wr_addr == -1) {
		printf("%s: missing endpoint\n", USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}
#endif

	/* XXX works because isoc comes after ctl */
	/* Grab isoc interface as well. */
	for (i = 0; i < uaa->nifaces; i++) {
		if (uaa->ifaces[i] == NULL)
			continue;
		id = usbd_get_interface_descriptor(uaa->ifaces[i]);
		if (id != NULL &&
		    id->bInterfaceClass == UICLASS_WIRELESS &&
		    id->bInterfaceSubClass == UISUBCLASS_RF &&
		    id->bInterfaceProtocol == UIPROTO_BLUETOOTH) {
			sc->sc_isoc_iface = uaa->ifaces[i];
			uaa->ifaces[i] = NULL;
		}
	}

	printf("%s: has%s isoc data\n", USBDEVNAME(sc->sc_dev),
	       sc->sc_isoc_iface != NULL ? "" : " no");

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	sc->sc_child = config_found(self, &bt, bt_print);

	USB_ATTACH_SUCCESS_RETURN;
}

USB_DETACH(ubt)
{
	USB_DETACH_START(ubt, sc);
	int s;
	int rv = 0;

	DPRINTF(("ubt_detach: sc=%p flags=%d\n", sc, flags));

	sc->sc_dying = 1;
	/* Abort all pipes.  Causes processes waiting for transfer to wake. */
	if (sc->sc_rd_pipe != NULL) {
		usbd_abort_pipe(sc->sc_rd_pipe);
		usbd_close_pipe(sc->sc_rd_pipe);
		sc->sc_rd_pipe = NULL;
	}
	if (sc->sc_wr_pipe != NULL) {
		usbd_abort_pipe(sc->sc_wr_pipe);
		usbd_close_pipe(sc->sc_wr_pipe);
		sc->sc_wr_pipe = NULL;
	}
#if 0
	wakeup(&sc->sc_rd_count);
#endif

	s = splusb();
	if (--sc->sc_refcnt >= 0) {
		/* Wait for processes to go away. */
		usb_detach_wait(USBDEV(sc->sc_dev));
	}
	splx(s);

	if (sc->sc_child != NULL) {
		rv = config_detach(sc->sc_child, flags);
		sc->sc_child = NULL;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	return (rv);
}

int
ubt_activate(device_ptr_t self, enum devact act)
{
	struct ubt_softc *sc = (struct ubt_softc *)self;
	int error = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		if (sc->sc_child != NULL)
			error = config_deactivate(sc->sc_child);
		break;
	}
	return (error);
}
