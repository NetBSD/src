/*	$NetBSD: ukyopon.c,v 1.5 2007/01/29 01:52:45 hubertf Exp $	*/

/*
 * Copyright (c) 1998, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by ITOH Yasufumi.
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
__KERNEL_RCSID(0, "$NetBSD: ukyopon.c,v 1.5 2007/01/29 01:52:45 hubertf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/poll.h>

#include <machine/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbcdc.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/ucomvar.h>
#include <dev/usb/umodemvar.h>
#include <dev/usb/ukyopon.h>

#ifdef UKYOPON_DEBUG
#define DPRINTFN(n, x)	if (ukyopondebug > (n)) logprintf x
int	ukyopondebug = 0;
#else
#define DPRINTFN(n, x)
#endif
#define DPRINTF(x) DPRINTFN(0, x)

struct ukyopon_softc {
	/* generic umodem device */
	struct umodem_softc	sc_umodem;

	/* ukyopon addition */
};

#define UKYOPON_MODEM_IFACE_INDEX	0
#define UKYOPON_DATA_IFACE_INDEX	3

Static void	ukyopon_get_status(void *, int, u_char *, u_char *);
Static int	ukyopon_ioctl(void *, int, u_long, caddr_t, int, usb_proc_ptr);

Static struct ucom_methods ukyopon_methods = {
	ukyopon_get_status,
	umodem_set,
	umodem_param,
	ukyopon_ioctl,
	umodem_open,
	umodem_close,
	NULL,
	NULL,
};

USB_DECLARE_DRIVER(ukyopon);

USB_MATCH(ukyopon)
{
	USB_MATCH_START(ukyopon, uaa);
	usb_device_descriptor_t *dd;
	usb_interface_descriptor_t *id;

	if (uaa->iface == NULL)
		return (UMATCH_NONE);

	id = usbd_get_interface_descriptor(uaa->iface);
	dd = usbd_get_device_descriptor(uaa->device);
	if (id == NULL || dd == NULL)
		return (UMATCH_NONE);

	if (UGETW(dd->idVendor) == USB_VENDOR_KYOCERA &&
	    UGETW(dd->idProduct) == USB_PRODUCT_KYOCERA_AHK3001V &&
	    (uaa->ifaceno == UKYOPON_MODEM_IFACE_INDEX ||
	     uaa->ifaceno == UKYOPON_DATA_IFACE_INDEX))
		return (UMATCH_VENDOR_PRODUCT);

	return (UMATCH_NONE);
}

USB_ATTACH(ukyopon)
{
	USB_ATTACH_START(ukyopon, sc, uaa);
	struct ucom_attach_args uca;

	uca.portno = (uaa->ifaceno == UKYOPON_MODEM_IFACE_INDEX) ?
		UKYOPON_PORT_MODEM : UKYOPON_PORT_DATA;
	uca.methods = &ukyopon_methods;
	uca.info = (uaa->ifaceno == UKYOPON_MODEM_IFACE_INDEX) ?
	    "modem port" : "data transfer port";

	if (umodem_common_attach(self, &sc->sc_umodem, uaa, &uca))
		USB_ATTACH_ERROR_RETURN;
	USB_ATTACH_SUCCESS_RETURN;
}

Static void
ukyopon_get_status(void *addr, int portno, u_char *lsr, u_char *msr)
{
	struct ukyopon_softc *sc = addr;

	/*
	 * The device doesn't set DCD (Data Carrier Detect) bit properly.
	 * Assume DCD is always present.
	 */
	if ((sc->sc_umodem.sc_msr & UMSR_DCD) == 0)
		sc->sc_umodem.sc_msr |= UMSR_DCD;

	return umodem_get_status(addr, portno, lsr, msr);
}

Static int
ukyopon_ioctl(void *addr, int portno, u_long cmd, caddr_t data, int flag,
	      usb_proc_ptr p)
{
	struct ukyopon_softc *sc = addr;
	struct ukyopon_identify *arg_id = (void*)data;
	int error = 0;

	switch (cmd) {
	case UKYOPON_IDENTIFY:
		strncpy(arg_id->ui_name, UKYOPON_NAME, sizeof arg_id->ui_name);
		arg_id->ui_busno =
		    USBDEVUNIT(*(device_ptr_t)sc->sc_umodem.sc_udev->bus->usbctl);
		arg_id->ui_address = sc->sc_umodem.sc_udev->address;
		arg_id->ui_model = UKYOPON_MODEL_UNKNOWN;
		arg_id->ui_porttype = portno;
		break;

	default:
		error = umodem_ioctl(addr, portno, cmd, data, flag, p);
		break;
	}

	return (error);
}

#ifdef __strong_alias
__strong_alias(ukyopon_activate,umodem_common_activate)
#else
int
ukyopon_activate(device_ptr_t self, enum devact act)
{
	struct ukyopon_softc *sc = (struct ukyopon_softc *)self;

	return umodem_common_activate(&sc->sc_umodem, act);
}
#endif

USB_DETACH(ukyopon)
{
	USB_DETACH_START(ukyopon, sc);
#ifdef __FreeBSD__
	int flags = 0;
#endif

	return umodem_common_detach(&sc->sc_umodem, flags);
}
