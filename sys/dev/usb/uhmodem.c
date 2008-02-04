/*	$NetBSD: uhmodem.c,v 1.1.4.2 2008/02/04 09:23:39 yamt Exp $	*/

/*
 * Copyright (c) 2008 Yojiro UO <yuo@nui.org>.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*-
 * Copyright (c) 2002, Alexander Kabaev <kan.FreeBSD.org>.
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
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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
__KERNEL_RCSID(0, "$NetBSD: uhmodem.c,v 1.1.4.2 2008/02/04 09:23:39 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/ioccom.h>
#include <sys/fcntl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/poll.h>
#include <sys/sysctl.h>
#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>

#include <dev/usb/usbcdc.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>
#include <dev/usb/ucomvar.h>
#include <dev/usb/ubsavar.h>


#define UHMODEMIBUFSIZE	4096
#define UHMODEMOBUFSIZE	4096

#ifdef UHMODEM_DEBUG
Static int	uhmodemdebug = 0;
#define DPRINTFN(n, x)  do { \
				if (uhmodemdebug > (n)) \
					logprintf x; \
			} while (0)
#else
#define DPRINTFN(n, x)
#endif
#define DPRINTF(x) DPRINTFN(0, x)

Static int uhmodem_open(void *, int);
Static  usbd_status e220_modechange_request(usbd_device_handle);
Static	usbd_status e220_endpointhalt(usbd_device_handle);
#if 0
Static  usbd_status e220_testreq(usbd_device_handle);
#endif
void e220_modechange_request_test(usbd_device_handle);

#define UHMODEM_MAXCONN		2

struct	uhmodem_softc {
	struct ubsa_softc	sc_ubsa;	
};

struct	ucom_methods uhmodem_methods = {
	ubsa_get_status,
	ubsa_set,
	ubsa_param,
	NULL,
	uhmodem_open,
	ubsa_close,
	NULL,
	NULL
};

Static const struct usb_devno uhmodem_devs[] = {
	/* HUAWEI E220 / Emobile D0[12]HW */
	{ USB_VENDOR_HUAWEI, USB_PRODUCT_HUAWEI_E220 },
};
#define uhmodem_lookup(v, p) usb_lookup(uhmodem_devs, v, p)

USB_DECLARE_DRIVER(uhmodem);

USB_MATCH(uhmodem)
{
	USB_IFMATCH_START(uhmodem, uaa);

	if (uhmodem_lookup(uaa->vendor, uaa->product) != NULL)
		/* XXX interface# 0,1 provide modem function, but this driver
		   handles all modem in single device.  */
		if (uaa->ifaceno == 0)
			return (UMATCH_VENDOR_PRODUCT);
	return (UMATCH_NONE);
}

USB_ATTACH(uhmodem)
{
	USB_IFATTACH_START(uhmodem, sc, uaa);
	usbd_device_handle dev = uaa->device;
	usb_config_descriptor_t *cdesc;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	const char *devname = USBDEVNAME(sc->sc_ubsa.sc_dev);
	usbd_status err;
	struct ucom_attach_args uca;
	int i;
	int j;
	char comname[16];

	devinfop = usbd_devinfo_alloc(dev, 0);
	USB_ATTACH_SETUP;
	printf("%s: %s\n", devname, devinfop);
	usbd_devinfo_free(devinfop);

        sc->sc_ubsa.sc_udev = dev;
	sc->sc_ubsa.sc_config_index = UBSA_DEFAULT_CONFIG_INDEX;
	sc->sc_ubsa.sc_numif = 1; /* defaut device has one interface */

	/* Hauwei E220 need special request to change its mode to modem */
	if ((uaa->ifaceno == 0) && (uaa->class != 255)) {
		err = e220_modechange_request(dev);
		if (err) {
			printf("%s: failed to change mode: %s\n", 
				devname, usbd_errstr(err));
			sc->sc_ubsa.sc_dying = 1;
			goto error;
		}
		printf("%s: mass storage only mode, reattach to enable modem\n",
			devname);
		sc->sc_ubsa.sc_dying = 1;
		goto error;
	}

	/*
	 * initialize rts, dtr variables to something
	 * different from boolean 0, 1
	 */
	sc->sc_ubsa.sc_dtr = -1;
	sc->sc_ubsa.sc_rts = -1;

	sc->sc_ubsa.sc_quadumts = 1;
	sc->sc_ubsa.sc_config_index = 0;
	sc->sc_ubsa.sc_numif = 2; /* E220 has 2coms */

	DPRINTF(("uhmodem attach: sc = %p\n", sc));

	/* Move the device into the configured state. */
	err = usbd_set_config_index(dev, sc->sc_ubsa.sc_config_index, 1);
	if (err) {
		printf("%s: failed to set configuration: %s\n",
		    devname, usbd_errstr(err));
		sc->sc_ubsa.sc_dying = 1;
		goto error;
	}

	/* get the config descriptor */
	cdesc = usbd_get_config_descriptor(sc->sc_ubsa.sc_udev);
	if (cdesc == NULL) {
		printf("%s: failed to get configuration descriptor\n",
		    devname);
		sc->sc_ubsa.sc_dying = 1;
		goto error;
	}

	sc->sc_ubsa.sc_intr_number = -1;
	sc->sc_ubsa.sc_intr_pipe = NULL;

	/* get the interfaces */
	for (i = 0; i < sc->sc_ubsa.sc_numif; i++) {
		err = usbd_device2interface_handle(dev, UBSA_IFACE_INDEX_OFFSET+i,
				 &sc->sc_ubsa.sc_iface[i]);
		if (err) {
			if (i == 0){
				/* can not get main interface */
				sc->sc_ubsa.sc_dying = 1;
				goto error;
			} else
				break;
		}

		/* Find the endpoints */
		id = usbd_get_interface_descriptor(sc->sc_ubsa.sc_iface[i]);
		sc->sc_ubsa.sc_iface_number[i] = id->bInterfaceNumber;

		/* initialize endpoints */
		uca.bulkin = uca.bulkout = -1;

		for (j = 0; j < id->bNumEndpoints; j++) {
			ed = usbd_interface2endpoint_descriptor(
				sc->sc_ubsa.sc_iface[i], j);
			if (ed == NULL) {
				printf("%s: no endpoint descriptor for %d (interface: %d)\n",
			    		devname, j, i);
				break;
			}

			if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			    UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
				sc->sc_ubsa.sc_intr_number = ed->bEndpointAddress;
				sc->sc_ubsa.sc_isize = UGETW(ed->wMaxPacketSize);
			} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
				uca.bulkin = ed->bEndpointAddress;
			} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
				uca.bulkout = ed->bEndpointAddress;
			}
		} /* end of Endpoint loop */

		if (sc->sc_ubsa.sc_intr_number == -1) {
			printf("%s: HUAWEI E220 need to re-attach to enable modem function\n", devname);
			if (i == 0) {
				/* could not get intr for main tty */
				sc->sc_ubsa.sc_dying = 1;
				goto error;
			} else
				break;
		}
		if (uca.bulkin == -1) {
			printf("%s: Could not find data bulk in\n", devname);
			sc->sc_ubsa.sc_dying = 1;
			goto error;
		}

		if (uca.bulkout == -1) {
			printf("%s: Could not find data bulk out\n", devname);
			sc->sc_ubsa.sc_dying = 1;
			goto error;
		}

		switch (i) {
		case 0:
			sprintf(comname, "modem");
			break;
		case 1:
			sprintf(comname, "monitor");
			break;
		case 2:
			sprintf(comname, "unknown");
			break;
		default:
			sprintf(comname, "int#%d", i);
		}

		uca.portno = i;
		/* bulkin, bulkout set above */
		uca.ibufsize = UHMODEMIBUFSIZE;
		uca.obufsize = UHMODEMOBUFSIZE;
		uca.ibufsizepad = UHMODEMIBUFSIZE;
		uca.opkthdrlen = 0;
		uca.device = dev;
		uca.iface = sc->sc_ubsa.sc_iface[i];
		uca.methods = &uhmodem_methods;
		uca.arg = &sc->sc_ubsa;
		uca.info = comname;
		DPRINTF(("uhmodem: int#=%d, in = 0x%x, out = 0x%x, intr = 0x%x\n",
	    		i, uca.bulkin, uca.bulkout, sc->sc_ubsa.sc_intr_number));
		sc->sc_ubsa.sc_subdevs[i] = config_found_sm_loc(self, "ucombus", NULL,
				 &uca, ucomprint, ucomsubmatch);
	} /* end of Interface loop */

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_ubsa.sc_udev,
			   USBDEV(sc->sc_ubsa.sc_dev));

	USB_ATTACH_SUCCESS_RETURN;

error:
	USB_ATTACH_ERROR_RETURN;
}

USB_DETACH(uhmodem)
{
	USB_DETACH_START(uhmodem, sc);
	int i;
	int rv = 0;

	DPRINTF(("uhmodem_detach: sc = %p\n", sc));

	if (sc->sc_ubsa.sc_intr_pipe != NULL) {
		usbd_abort_pipe(sc->sc_ubsa.sc_intr_pipe);
		usbd_close_pipe(sc->sc_ubsa.sc_intr_pipe);
		free(sc->sc_ubsa.sc_intr_buf, M_USBDEV);
		sc->sc_ubsa.sc_intr_pipe = NULL;
	}

	sc->sc_ubsa.sc_dying = 1;
	for (i = 0; i < sc->sc_ubsa.sc_numif; i++) {
		if (sc->sc_ubsa.sc_subdevs[i] != NULL) {
			rv |= config_detach(sc->sc_ubsa.sc_subdevs[i], flags);
			sc->sc_ubsa.sc_subdevs[i] = NULL;
		}
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_ubsa.sc_udev,
			   USBDEV(sc->sc_ubsa.sc_dev));

	return (rv);
}

int
uhmodem_activate(device_ptr_t self, enum devact act)
{
	struct uhmodem_softc *sc = (struct uhmodem_softc *)self;
	int rv = 0;
	int i;

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);

	case DVACT_DEACTIVATE:
		for (i = 0; i < sc->sc_ubsa.sc_numif; i++) {
			if (sc->sc_ubsa.sc_subdevs[i] != NULL)
				rv |= config_deactivate(sc->sc_ubsa.sc_subdevs[i]);
		}
		sc->sc_ubsa.sc_dying = 1;
		break;
	}
	return (rv);
}

Static int
uhmodem_open(void *addr, int portno)
{
	struct ubsa_softc *sc = addr;
	usbd_status err;

	if (sc->sc_dying)
		return (ENXIO);

	DPRINTF(("%s: sc = %p\n", __func__, sc));

	err = e220_endpointhalt(sc->sc_udev);
	if (err) 
		printf("%s: endpointhalt fail\n", __func__);
	else
		usbd_delay_ms(sc->sc_udev, 50);
#if 0 /* currenly disable */
	err = e220_testreq(sc->sc_udev);
	if (err)
		printf("%s: send testreq fail\n", __func__);
	else
		usbd_delay_ms(sc->sc_udev, 50);
#endif

	if (sc->sc_intr_number != -1 && sc->sc_intr_pipe == NULL) {
		sc->sc_intr_buf = malloc(sc->sc_isize, M_USBDEV, M_WAITOK);
		/* XXX only iface# = 0 has intr line */
		/* XXX E220 specific? need to check */
		err = usbd_open_pipe_intr(sc->sc_iface[0],
		    sc->sc_intr_number,
		    USBD_SHORT_XFER_OK,
		    &sc->sc_intr_pipe,
		    sc,
		    sc->sc_intr_buf,
		    sc->sc_isize,
		    ubsa_intr,
		    UBSA_INTR_INTERVAL);
		if (err) {
			printf("%s: cannot open interrupt pipe (addr %d)\n",
			    USBDEVNAME(sc->sc_dev),
			    sc->sc_intr_number);
			return (EIO);
		}
	}

	return (0);
}

/*
 * Hauwei E220 needs special request to enable modem function.
 * -- DEVICE_REMOTE_WAKEUP ruquest to endpoint 2.
 */
Static  usbd_status 
e220_modechange_request(usbd_device_handle dev)
{
#define E220_MODE_CHANGE_REQUEST 0x2
	usb_device_request_t req;
	usbd_status err;

	req.bmRequestType = UT_WRITE_DEVICE;
	req.bRequest = UR_SET_FEATURE;
	USETW(req.wValue, UF_DEVICE_REMOTE_WAKEUP);
	USETW(req.wIndex, E220_MODE_CHANGE_REQUEST);
	USETW(req.wLength, 0);

	DPRINTF(("%s: send e220 mode change request\n", __func__));
	err = usbd_do_request(dev, &req, 0);
	if (err) {
		DPRINTF(("%s: E220 mode change fail\n", __func__));
		return (EIO);
	}

	return (0);
#undef E220_MODE_CHANGE_REQUEST
}

Static  usbd_status 
e220_endpointhalt(usbd_device_handle dev)
{
	usb_device_request_t req;
	usbd_status err;

	/* CLEAR feature / endpoint halt to modem i/o */
	req.bmRequestType = UT_WRITE_ENDPOINT;
	req.bRequest = UR_CLEAR_FEATURE;
	USETW(req.wValue, UF_ENDPOINT_HALT);
	USETW(req.wIndex, 0x0082); /* should get value from softc etc.*/
	USETW(req.wLength, 0);
	err = usbd_do_request(dev, &req, 0);
	if (err) {
		DPRINTF(("%s: E220 request test ENDPOINT_HALT fail\n", __func__));
		return (EIO);
	}
	req.bmRequestType = UT_WRITE_ENDPOINT;
	req.bRequest = UR_CLEAR_FEATURE;
	USETW(req.wValue, UF_ENDPOINT_HALT);
	USETW(req.wIndex, 0x0002); /* should get value from softc etc.*/
	USETW(req.wLength, 0);
	err = usbd_do_request(dev, &req, 0);
	if (err) {
		DPRINTF(("%s: E220 request test ENDPOINT_HALT fail\n", __func__));
		return (EIO);
	}

	return (0);
}

#if 0
/* 
 * Windows device driver send these sequens of USB requests.
 * However currently I can't understand what the messege is,
 * disable this code when I get more information about it.
 */ 
Static  usbd_status 
e220_testreq(usbd_device_handle dev)
{
	uint8_t data[8];
	usb_device_request_t req;
	usbd_status err;
	int i;

	/* vendor specific unknown requres */
	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = 0x02;
	USETW(req.wValue, 0x0001);
	USETW(req.wIndex, 0x0000);
	USETW(req.wLength, 2);
	data[0] = 0x0;
	data[1] = 0x0;
	err = usbd_do_request(dev, &req, data);
	if (err) 
		goto error;

	/* vendor specific unknown sequence */
#define E220_CLASS_SETUP	0x22
#define	E220_CLASS_READ		0x21
#define	E220_CLASS_WRITE	0x20

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = E220_CLASS_SETUP;	// 0x22
	USETW(req.wValue, 0x0001);
	USETW(req.wIndex, 0x0000);
	USETW(req.wLength, 0);
	err = usbd_do_request(dev, &req, 0);
	if (err) 
		goto error;

	req.bmRequestType = UT_READ_CLASS_INTERFACE;
	req.bRequest = E220_CLASS_READ;
	USETW(req.wValue, 0x0000);
	USETW(req.wIndex, 0x0000);
	USETW(req.wLength, 7);
	err = usbd_do_request(dev, &req, &data);
	if (err) 
		goto error;

	data[1] = 0x8;
	data[2] = 0x7;
	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = E220_CLASS_WRITE;
	USETW(req.wValue, 0x0000);
	USETW(req.wIndex, 0x0000);
	USETW(req.wLength, 7);
	err = usbd_do_request(dev, &req, data);
	if (err) 
		goto error;

	req.bmRequestType = UT_READ_CLASS_INTERFACE;
	req.bRequest = E220_CLASS_READ;
	USETW(req.wValue, 0x0000);
	USETW(req.wIndex, 0x0000);
	USETW(req.wLength, 7);
	err = usbd_do_request(dev, &req, &data);
	if (err) 
		goto error;
	printf("%s(0x21):", __func__);

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = E220_CLASS_SETUP;	// 0x22
	USETW(req.wValue, 0x0003);
	USETW(req.wIndex, 0x0000); 
	USETW(req.wLength, 0);
	err = usbd_do_request(dev, &req, 0);
	if (err) 
		goto error;

	return (0);
error:
	DPRINTF(("%s: E220 request test SETUP fail\n", __func__));
	return (EIO);
}
#endif

