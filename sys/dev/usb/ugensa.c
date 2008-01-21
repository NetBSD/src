/*	$NetBSD: ugensa.c,v 1.3.2.7 2008/01/21 09:44:45 yamt Exp $	*/

/*
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland C. Dowdeswell <elric@netbsd.org>.
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
__KERNEL_RCSID(0, "$NetBSD: ugensa.c,v 1.3.2.7 2008/01/21 09:44:45 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/tty.h>

#include <dev/usb/usb.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/ucomvar.h>

/* XXXrcd: heh */
#define UGENSA_DEBUG 1

#ifdef UGENSA_DEBUG
#define DPRINTF(x)	if (ugensadebug) printf x
#define DPRINTFN(n,x)	if (ugensadebug>(n)) printf x
int ugensadebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

struct ugensa_softc {
	USBBASEDEVICE		sc_dev;		/* base device */
	usbd_device_handle	sc_udev;	/* device */
	usbd_interface_handle	sc_iface;	/* interface */

	device_ptr_t		sc_subdev;
	int			sc_numcon;

	u_char			sc_dying;
};

struct ucom_methods ugensa_methods = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

#define UGENSA_CONFIG_INDEX	0
#define UGENSA_IFACE_INDEX	0
#define UGENSA_BUFSIZE		1024

static const struct usb_devno ugensa_devs[] = {
	{ USB_VENDOR_AIRPRIME, USB_PRODUCT_AIRPRIME_PC5220 },
	{ USB_VENDOR_NOVATEL, USB_PRODUCT_NOVATEL_FLEXPACKGPS },
	{ USB_VENDOR_QUALCOMM_K, USB_PRODUCT_QUALCOMM_K_CDMA_MSM_K },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_AIRCARD580 },
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_MINI5725},
	{ USB_VENDOR_NOVATEL2, USB_PRODUCT_NOVATEL2_CDMA_MODEM },
	{ USB_VENDOR_DELL, USB_PRODUCT_DELL_HSDPA }
};
#define ugensa_lookup(v, p) usb_lookup(ugensa_devs, v, p)

USB_DECLARE_DRIVER(ugensa);

USB_MATCH(ugensa)
{
	USB_MATCH_START(ugensa, uaa);

	DPRINTFN(20,("ugensa: vendor=0x%x, product=0x%x\n",
		     uaa->vendor, uaa->product));

	return (ugensa_lookup(uaa->vendor, uaa->product) != NULL ?
		UMATCH_VENDOR_PRODUCT : UMATCH_NONE);
}

USB_ATTACH(ugensa)
{
	USB_ATTACH_START(ugensa, sc, uaa);
	usbd_device_handle dev = uaa->device;
	usbd_interface_handle iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	char *devname = USBDEVNAME(sc->sc_dev);
	usbd_status err;
	struct ucom_attach_args uca;
	int i;

	DPRINTFN(10,("\nugensa_attach: sc=%p\n", sc));

	/* Move the device into the configured state. */
	err = usbd_set_config_index(dev, UGENSA_CONFIG_INDEX, 1);
	if (err) {
		printf("\n%s: failed to set configuration, err=%s\n",
		       devname, usbd_errstr(err));
		goto bad;
	}

	err = usbd_device2interface_handle(dev, UGENSA_IFACE_INDEX, &iface);
	if (err) {
		printf("\n%s: failed to get interface, err=%s\n",
		       devname, usbd_errstr(err));
		goto bad;
	}

	devinfop = usbd_devinfo_alloc(dev, 0);
	USB_ATTACH_SETUP;
	printf("%s: %s\n", devname, devinfop);
	usbd_devinfo_free(devinfop);

	id = usbd_get_interface_descriptor(iface);

	sc->sc_udev = dev;
	sc->sc_iface = iface;

	uca.info = "Generic Serial Device";
	uca.ibufsize = UGENSA_BUFSIZE;
	uca.obufsize = UGENSA_BUFSIZE;
	uca.ibufsizepad = UGENSA_BUFSIZE;
	uca.portno = UCOM_UNK_PORTNO;
	uca.opkthdrlen = 0;
	uca.device = dev;
	uca.iface = iface;
	uca.methods = &ugensa_methods;
	uca.arg = sc;

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	uca.bulkin = uca.bulkout = -1;
	for (i = 0; i < id->bNumEndpoints; i++) {
		int addr, dir, attr;

		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			printf("%s: could not read endpoint descriptor"
			       ": %s\n", devname, usbd_errstr(err));
			goto bad;
		}

		addr = ed->bEndpointAddress;
		dir = UE_GET_DIR(ed->bEndpointAddress);
		attr = ed->bmAttributes & UE_XFERTYPE;
		if (dir == UE_DIR_IN && attr == UE_BULK)
			uca.bulkin = addr;
		else if (dir == UE_DIR_OUT && attr == UE_BULK)
			uca.bulkout = addr;
		else
			printf("%s: unexpected endpoint\n", devname);
	}
	if (uca.bulkin == -1) {
		printf("%s: Could not find data bulk in\n",
		       USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	if (uca.bulkout == -1) {
		printf("%s: Could not find data bulk out\n",
		       USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	DPRINTF(("ugensa: in=0x%x out=0x%x\n", uca.bulkin, uca.bulkout));
	sc->sc_subdev = config_found_sm_loc(self, "ucombus", NULL, &uca,
					    ucomprint, ucomsubmatch);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
	USB_ATTACH_SUCCESS_RETURN;

bad:
	DPRINTF(("ugensa_attach: ATTACH ERROR\n"));
	sc->sc_dying = 1;
	USB_ATTACH_ERROR_RETURN;
}

int
ugensa_activate(device_ptr_t self, enum devact act)
{
	struct ugensa_softc *sc = (struct ugensa_softc *)self;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		if (sc->sc_subdev)
			rv = config_deactivate(sc->sc_subdev);
		break;
	}
	return (rv);
}

USB_DETACH(ugensa)
{
	USB_DETACH_START(ugensa, sc);
	int rv = 0;

	DPRINTF(("ugensa_detach: sc=%p flags=%d\n", sc, flags));

	sc->sc_dying = 1;
	pmf_device_deregister(self);

	if (sc->sc_subdev != NULL)
		rv = config_detach(sc->sc_subdev, flags);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	return (rv);
}
