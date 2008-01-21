/*	$NetBSD: stuirda.c,v 1.2.16.4 2008/01/21 09:44:44 yamt Exp $	*/

/*
 * Copyright (c) 2001,2007 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: stuirda.c,v 1.2.16.4 2008/01/21 09:44:44 yamt Exp $");

#include <sys/param.h>

#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>


#include <dev/firmload.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_port.h>

#include <dev/ir/ir.h>
#include <dev/ir/irdaio.h>
#include <dev/ir/irframevar.h>

#include <dev/usb/uirdavar.h>

#ifdef UIRDA_DEBUG
#define DPRINTF(x)	if (stuirdadebug) logprintf x
#define DPRINTFN(n,x)	if (stuirdadebug>(n)) logprintf x
int	stuirdadebug = 1;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

struct stuirda_softc {
	struct uirda_softc sc_uirda;
};

int stuirda_fwload(struct uirda_softc *sc);

/* 
 * These devices need firmware download.
 */
Static const struct usb_devno stuirda_devs[] = {
	{ USB_VENDOR_SIGMATEL, USB_PRODUCT_SIGMATEL_SIR4116 },
	{ USB_VENDOR_SIGMATEL, USB_PRODUCT_SIGMATEL_FIR4210 },
	{ USB_VENDOR_SIGMATEL, USB_PRODUCT_SIGMATEL_VFIR4220 },
};
#define stuirda_lookup(v, p) (usb_lookup(stuirda_devs, v, p))

int stuirda_write(void *h, struct uio *uio, int flag);

struct irframe_methods stuirda_methods = {
	uirda_open, uirda_close, uirda_read, stuirda_write, uirda_poll,
	uirda_kqfilter, uirda_set_params, uirda_get_speeds,
	uirda_get_turnarounds
};

#define STUIRDA_HEADER_SIZE 3

#define stuirda_activate uirda_activate
#define stuirda_detach uirda_detach

USB_DECLARE_DRIVER(stuirda);

USB_MATCH(stuirda)
{
	USB_IFMATCH_START(stuirda, uaa);

	DPRINTFN(50,("stuirda_match\n"));

	if (stuirda_lookup(uaa->vendor, uaa->product) != NULL)
		return (UMATCH_VENDOR_PRODUCT);

	return (UMATCH_NONE);
}

void uirda_attach(struct device *,struct device *,void *);

USB_ATTACH(stuirda)
{
	USB_IFATTACH_START(stuirda, sc, uaa);

	(void)uaa;

	sc->sc_uirda.sc_loadfw = stuirda_fwload;
	sc->sc_uirda.sc_irm = &stuirda_methods;
	sc->sc_uirda.sc_hdszi = STUIRDA_HEADER_SIZE;

	uirda_attach(parent,self,aux);
}

int
stuirda_fwload(struct uirda_softc *sc) {


	int rc;
	firmware_handle_t fh;
	off_t fwsize;
	usb_device_descriptor_t usbddsc;
	usbd_xfer_handle	fwxfer;
	usbd_pipe_handle	fwpipe;
	usbd_status status;
	usb_device_request_t req;
	char *buffer;
	char *p;
	char fwname[12];
	int n;
	u_int8_t *usbbuf;
	/* size_t bsize; */

	printf("%s: needing to download firmware\n",
		USBDEVNAME(sc->sc_dev));

	status = usbd_get_device_desc(sc->sc_udev, &usbddsc);
	if (status) {
		printf("%s: can't get device descriptor, status %d\n",
		    USBDEVNAME(sc->sc_dev), status);
		return status;
	}

	rc = usbd_get_class_desc(sc->sc_udev, UDESC_IRDA, 0,
		USB_IRDA_DESCRIPTOR_SIZE, &sc->sc_irdadesc);
	printf("error %d reading class desc\n", rc);

	sprintf(fwname, "4210%02x%02x.sb",
		usbddsc.bcdDevice[1],
		usbddsc.bcdDevice[0]);

	printf("%s: Attempting to load firmware %s\n",
		USBDEVNAME(sc->sc_dev), fwname);
	
	rc = firmware_open("uirda", fwname, &fh);

	if (rc) {
		printf("%s: Cannot load firmware\n",
			USBDEVNAME(sc->sc_dev));
		return 0;
		return rc;
	}
	fwsize = firmware_get_size(fh);

	printf("%s: Firmware size %lld\n",
		USBDEVNAME(sc->sc_dev), (long long)fwsize);

	buffer = firmware_malloc(fwsize);
	if (buffer == NULL) {
		printf("%s: Cannot load firmware: out of memory\n",
			USBDEVNAME(sc->sc_dev));
		goto giveup2;
	}

	rc = firmware_read(fh, 0, buffer, (size_t)fwsize);

	if (rc) {
		printf("%s: Cannot read firmware\n", USBDEVNAME(sc->sc_dev));
		goto giveup3;
	}

	for (p = buffer + sizeof("Product Version:");
	    p < buffer + fwsize - 5; p++) {

		if (0x1A == *p)
			break;
	}
	if (0x1a != *p || memcmp(p+1, "STMP", 4) != 0) {
		/* firmware bad */
		printf("%s: Bad firmware\n", USBDEVNAME(sc->sc_dev));
		goto giveup3;
	}

	p += 5;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = 2 /* XXX magic */;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);
	rc = usbd_do_request(sc->sc_udev, &req, 0);
	if (rc) {
		printf("%s: Cannot switch to f/w d/l mode, error %d\n",
			USBDEVNAME(sc->sc_dev), rc);
		goto giveup4;
	}

	delay(100000);

	rc = usbd_open_pipe(sc->sc_iface, sc->sc_wr_addr, 0, &fwpipe);
	if (rc) {
		printf("%s: Cannot open pipe, rc=%d\n",
		    USBDEVNAME(sc->sc_dev), rc);
		goto giveup3;
	}
	fwxfer = usbd_alloc_xfer(sc->sc_udev);
	if (fwxfer == NULL) {
		printf("%s: Cannot alloc xfer\n", USBDEVNAME(sc->sc_dev));
		goto giveup4;
	}
	usbbuf = usbd_alloc_buffer(fwxfer, 1024);
	if (usbbuf == NULL) {
		printf("%s: Cannot alloc usb buf\n", USBDEVNAME(sc->sc_dev));
		goto giveup5;
	}
	n = (buffer + fwsize - p);
	while (n > 0) {
		if (n > 1023)
			n = 1023;
		memcpy(usbbuf, p, n);
		rc = usbd_bulk_transfer(fwxfer, fwpipe,
		    USBD_SYNCHRONOUS|USBD_FORCE_SHORT_XFER,
		    5000, usbbuf, &n, "uirda-fw-wr");
		printf("%s: write: rc=%d, %d left\n",
		    USBDEVNAME(sc->sc_dev), rc, n);
		if (rc) {
			printf("%s: write: rc=%d, %d bytes written\n",
			    USBDEVNAME(sc->sc_dev), rc, n);
			goto giveup4;
		}
		printf("%s: written %d\n", USBDEVNAME(sc->sc_dev), n);
		p += n; 
		n = (buffer + fwsize - p);
	}
	delay(100000);
	/* TODO: more code here */
	rc = 0;
	usbd_free_buffer(fwxfer);

	giveup5: usbd_free_xfer(fwxfer);	
	giveup4: usbd_close_pipe(fwpipe);
	giveup3: firmware_free(buffer, fwsize);
	giveup2: firmware_close(fh);

	return rc;
		
}

int
stuirda_write(void *h, struct uio *uio, int flag)
{
	struct uirda_softc *sc = h;
	usbd_status err;
	u_int32_t n;
	int error = 0;

	DPRINTFN(1,("%s: sc=%p\n", __func__, sc));

	if (sc->sc_dying)
		return (EIO);

#ifdef DIAGNOSTIC
	if (sc->sc_wr_buf == NULL)
		return (EINVAL);
#endif

	n = uio->uio_resid;
	if (n > sc->sc_params.maxsize)
		return (EINVAL);

	sc->sc_refcnt++;
	mutex_enter(&sc->sc_wr_buf_lk);

	sc->sc_wr_buf[0] = UIRDA_EB_NO_CHANGE | UIRDA_NO_SPEED;

	sc->sc_wr_buf[1] = 0;
	sc->sc_wr_buf[2] = 7; /* XXX turnaround - maximum for now */
	if ((n > 0 && (n % 128) == 0 && (n % 512) != 0)) {
		sc->sc_wr_buf[1] = 1;
	}

	error = uiomove(sc->sc_wr_buf + STUIRDA_HEADER_SIZE, n, uio);
	if (!error) {
		DPRINTFN(1, ("uirdawrite: transfer %d bytes\n", n));

		n += STUIRDA_HEADER_SIZE + sc->sc_wr_buf[1];
		err = usbd_bulk_transfer(sc->sc_wr_xfer, sc->sc_wr_pipe,
			  USBD_FORCE_SHORT_XFER|USBD_NO_COPY,
			  UIRDA_WR_TIMEOUT,
			  sc->sc_wr_buf, &n, "uirdawr");
		DPRINTFN(2, ("uirdawrite: err=%d\n", err));
		if (err) {
			if (err == USBD_INTERRUPTED)
				error = EINTR;
			else if (err == USBD_TIMEOUT)
				error = ETIMEDOUT;
			else
				error = EIO;
		}
	}

	mutex_exit(&sc->sc_wr_buf_lk);
	if (--sc->sc_refcnt < 0)
		usb_detach_wakeup(USBDEV(sc->sc_dev));

	DPRINTFN(1,("%s: sc=%p done\n", __func__, sc));
	return (error);
}
