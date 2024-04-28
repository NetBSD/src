/*	$NetBSD: uftdi.c,v 1.76.6.3 2024/04/28 13:26:36 martin Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: uftdi.c,v 1.76.6.3 2024/04/28 13:26:36 martin Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/tty.h>

#include <dev/usb/usb.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/ucomvar.h>

#include <dev/usb/uftdireg.h>

#ifdef UFTDI_DEBUG
#define DPRINTF(x)	if (uftdidebug) printf x
#define DPRINTFN(n,x)	if (uftdidebug>(n)) printf x
int uftdidebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define UFTDI_CONFIG_NO		1

/*
 * These are the default number of bytes transferred per frame if the
 * endpoint doesn't tell us.  The output buffer size is a hard limit
 * for devices that use a 6-bit size encoding.
 */
#define UFTDIIBUFSIZE 64
#define UFTDIOBUFSIZE 64

/*
 * Magic constants!  Where do these come from?  They're what Linux uses...
 */
#define	UFTDI_MAX_IBUFSIZE	512
#define	UFTDI_MAX_OBUFSIZE	256

struct uftdi_softc {
	device_t		sc_dev;		/* base device */
	struct usbd_device *	sc_udev;	/* device */
	struct usbd_interface *	sc_iface;	/* interface */
	int			sc_iface_no;

	enum uftdi_type		sc_type;
	u_int			sc_flags;
#define FLAGS_BAUDCLK_12M	0x00000001
#define FLAGS_ROUNDOFF_232A	0x00000002
#define FLAGS_BAUDBITS_HINDEX	0x00000004
	u_int			sc_hdrlen;
	u_int			sc_chiptype;

	u_char			sc_msr;
	u_char			sc_lsr;

	device_t		sc_subdev;

	bool			sc_dying;

	u_int			last_lcr;
};

static void	uftdi_get_status(void *, int, u_char *, u_char *);
static void	uftdi_set(void *, int, int, int);
static int	uftdi_param(void *, int, struct termios *);
static int	uftdi_open(void *, int);
static void	uftdi_read(void *, int, u_char **, uint32_t *);
static void	uftdi_write(void *, int, u_char *, u_char *, uint32_t *);
static void	uftdi_break(void *, int, int);

static const struct ucom_methods uftdi_methods = {
	.ucom_get_status = uftdi_get_status,
	.ucom_set = uftdi_set,
	.ucom_param = uftdi_param,
	.ucom_open = uftdi_open,
	.ucom_read = uftdi_read,
	.ucom_write = uftdi_write,
};

/*
 * The devices default to UFTDI_TYPE_8U232AM.
 * Remember to update uftdi_attach() if it should be UFTDI_TYPE_SIO instead
 */
static const struct usb_devno uftdi_devs[] = {
	{ USB_VENDOR_BBELECTRONICS, USB_PRODUCT_BBELECTRONICS_USOTL4 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US101 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US159 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US235 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US257 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US279_12 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US279_34 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US279_56 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US279_78 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US313 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US320 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US324 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US346_12 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US346_34 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US701_12 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US701_34 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US842_12 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US842_34 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US842_56 },
	{ USB_VENDOR_BRAINBOXES, USB_PRODUCT_BRAINBOXES_US842_78 },
	{ USB_VENDOR_FALCOM, USB_PRODUCT_FALCOM_TWIST },
	{ USB_VENDOR_FALCOM, USB_PRODUCT_FALCOM_SAMBA },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_230X },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_232H },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_232RL },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_2232C },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_4232H },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_8U100AX },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL_8U232AM },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_KW },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_YS },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_Y6 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_Y8 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_IC },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_DB9 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_RS232 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MHAM_Y9 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_COASTAL_TNCX },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_CTI_485_MINI },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_CTI_NANO_485 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SEMC_DSS20 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_LK202_24_USB },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_LK204_24_USB },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_MX200_USB },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_MX4_MX5_USB },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_631 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_632 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_633 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_634 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_LCD_CFA_635 },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_OPENRD_JTAGKEY },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_BEAGLEBONE },
	{ USB_VENDOR_FTDI, USB_PRODUCT_FTDI_MAXSTREAM_PKG_U },
	{ USB_VENDOR_xxFTDI, USB_PRODUCT_xxFTDI_SHEEVAPLUG_JTAG },
	{ USB_VENDOR_INTREPIDCS, USB_PRODUCT_INTREPIDCS_VALUECAN },
	{ USB_VENDOR_INTREPIDCS, USB_PRODUCT_INTREPIDCS_NEOVI },
	{ USB_VENDOR_MELCO, USB_PRODUCT_MELCO_PCOPRS1 },
	{ USB_VENDOR_RATOC, USB_PRODUCT_RATOC_REXUSB60F },
	{ USB_VENDOR_RTSYS, USB_PRODUCT_RTSYS_CT57A },
	{ USB_VENDOR_RTSYS, USB_PRODUCT_RTSYS_RTS03 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_USBSERIAL },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_SEAPORT4P1 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_SEAPORT4P2 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_SEAPORT4P3 },
	{ USB_VENDOR_SEALEVEL, USB_PRODUCT_SEALEVEL_SEAPORT4P4 },
	{ USB_VENDOR_SIIG2, USB_PRODUCT_SIIG2_US2308 },
	{ USB_VENDOR_MISC, USB_PRODUCT_MISC_TELLSTICK },
	{ USB_VENDOR_MISC, USB_PRODUCT_MISC_TELLSTICK_DUO },
};
#define uftdi_lookup(v, p) usb_lookup(uftdi_devs, v, p)

static int	uftdi_match(device_t, cfdata_t, void *);
static void	uftdi_attach(device_t, device_t, void *);
static void	uftdi_childdet(device_t, device_t);
static int	uftdi_detach(device_t, int);

CFATTACH_DECL2_NEW(uftdi, sizeof(struct uftdi_softc), uftdi_match,
    uftdi_attach, uftdi_detach, NULL, NULL, uftdi_childdet);

struct uftdi_match_quirk_entry {
	uint16_t	vendor_id;
	uint16_t	product_id;
	int		iface_no;
	const char *	vendor_str;
	const char *	product_str;
	int		match_ret;
};

static const struct uftdi_match_quirk_entry uftdi_match_quirks[] = {
	/*
	 * The Tigard board (https://github.com/tigard-tools/tigard)
	 * has two interfaces, one of which is meant to act as a
	 * regular USB serial port (interface 0), the other of which
	 * is meant for other protocols (SWD, JTAG, etc.).  We must
	 * reject interface 1 so that ugenif matches, thus allowing
	 * full user-space control of that port.
	 */
	{
	  .vendor_id	= USB_VENDOR_FTDI,
	  .product_id	= USB_PRODUCT_FTDI_SERIAL_2232C,
	  .iface_no	= 1,
	  .vendor_str	= "SecuringHardware.com",
	  .product_str	= "Tigard V1.1",
	  .match_ret	= UMATCH_NONE,
	},
	/*
	 * The SiPEED Tang Nano 9K (and other SiPEED Tang FPGA development
	 * boards) have an FT2232 on-board, wired up only for JTAG.
	 */
	{
	  .vendor_id	= USB_VENDOR_FTDI,
	  .product_id	= USB_PRODUCT_FTDI_SERIAL_2232C,
	  .iface_no	= -1,
	  .vendor_str	= "SIPEED",
	  .product_str	= "JTAG Debugger",
	  .match_ret	= UMATCH_NONE,
	},
};

static int
uftdi_quirk_match(struct usbif_attach_arg *uiaa, int rv)
{
	struct usbd_device *dev = uiaa->uiaa_device;
	const struct uftdi_match_quirk_entry *q;
	int i;

	for (i = 0; i < __arraycount(uftdi_match_quirks); i++) {
		q = &uftdi_match_quirks[i];
		if (uiaa->uiaa_vendor != q->vendor_id ||
		    uiaa->uiaa_product != q->product_id ||
		    (q->iface_no != -1 && uiaa->uiaa_ifaceno != q->iface_no)) {
			continue;
		}
		if (q->vendor_str != NULL &&
		    (dev->ud_vendor == NULL ||
		     strcmp(dev->ud_vendor, q->vendor_str) != 0)) {
			continue;
		}
		if (q->product_str != NULL &&
		    (dev->ud_product == NULL ||
		     strcmp(dev->ud_product, q->product_str) != 0)) {
			continue;
		}
		/*
		 * Got a match!
		 */
		rv = q->match_ret;
		break;
	}
	return rv;
}

static int
uftdi_match(device_t parent, cfdata_t match, void *aux)
{
	struct usbif_attach_arg *uiaa = aux;
	int rv;

	DPRINTFN(20,("uftdi: vendor=%#x, product=%#x\n",
		     uiaa->uiaa_vendor, uiaa->uiaa_product));

	if (uiaa->uiaa_configno != UFTDI_CONFIG_NO)
		return UMATCH_NONE;

	rv = uftdi_lookup(uiaa->uiaa_vendor, uiaa->uiaa_product) != NULL ?
		UMATCH_VENDOR_PRODUCT_CONF_IFACE : UMATCH_NONE;
	if (rv != UMATCH_NONE) {
		rv = uftdi_quirk_match(uiaa, rv);
	}
	return rv;
}

static void
uftdi_attach(device_t parent, device_t self, void *aux)
{
	struct uftdi_softc *sc = device_private(self);
	struct usbif_attach_arg *uiaa = aux;
	struct usbd_device *dev = uiaa->uiaa_device;
	struct usbd_interface *iface = uiaa->uiaa_iface;
	usb_device_descriptor_t *ddesc;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	int i;
	struct ucom_attach_args ucaa;

	DPRINTFN(10,("\nuftdi_attach: sc=%p\n", sc));

	aprint_naive("\n");
	aprint_normal("\n");

	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	sc->sc_dev = self;
	sc->sc_udev = dev;
	sc->sc_dying = false;
	sc->sc_iface_no = uiaa->uiaa_ifaceno;
	sc->sc_type = UFTDI_TYPE_8U232AM; /* most devices are post-8U232AM */
	sc->sc_hdrlen = 0;

	ddesc = usbd_get_device_descriptor(dev);
	sc->sc_chiptype = UGETW(ddesc->bcdDevice);

	switch (sc->sc_chiptype) {
	case 0x0200:
		if (ddesc->iSerialNumber != 0)
			sc->sc_flags |= FLAGS_ROUNDOFF_232A;
		ucaa.ucaa_portno = 0;
		break;
	case 0x0400:
		ucaa.ucaa_portno = 0;
		break;
	case 0x0500:
		sc->sc_flags |= FLAGS_BAUDBITS_HINDEX;
		ucaa.ucaa_portno = FTDI_PIT_SIOA + sc->sc_iface_no;
		break;
	case 0x0600:
		ucaa.ucaa_portno = 0;
		break;
	case 0x0700:
	case 0x0800:
	case 0x0900:
		sc->sc_flags |= FLAGS_BAUDCLK_12M;
		sc->sc_flags |= FLAGS_BAUDBITS_HINDEX;
		ucaa.ucaa_portno = FTDI_PIT_SIOA + sc->sc_iface_no;
		break;
	case 0x1000:
		sc->sc_flags |= FLAGS_BAUDBITS_HINDEX;
		ucaa.ucaa_portno = FTDI_PIT_SIOA + sc->sc_iface_no;
		break;
	default:
		if (sc->sc_chiptype < 0x0200) {
			sc->sc_type = UFTDI_TYPE_SIO;
			sc->sc_hdrlen = 1;
		}
		ucaa.ucaa_portno = 0;
		break;
	}

	id = usbd_get_interface_descriptor(iface);

	sc->sc_iface = iface;

	ucaa.ucaa_bulkin = ucaa.ucaa_bulkout = -1;
	ucaa.ucaa_ibufsize = ucaa.ucaa_obufsize = 0;
	for (i = 0; i < id->bNumEndpoints; i++) {
		int addr, dir, attr;
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			aprint_error_dev(self,
			    "could not read endpoint descriptor\n");
			goto bad;
		}

		addr = ed->bEndpointAddress;
		dir = UE_GET_DIR(ed->bEndpointAddress);
		attr = ed->bmAttributes & UE_XFERTYPE;
		if (dir == UE_DIR_IN && attr == UE_BULK) {
			ucaa.ucaa_bulkin = addr;
			ucaa.ucaa_ibufsize = UGETW(ed->wMaxPacketSize);
			if (ucaa.ucaa_ibufsize >= UFTDI_MAX_IBUFSIZE)
				ucaa.ucaa_ibufsize = UFTDI_MAX_IBUFSIZE;
		} else if (dir == UE_DIR_OUT && attr == UE_BULK) {
			ucaa.ucaa_bulkout = addr;
			ucaa.ucaa_obufsize = UGETW(ed->wMaxPacketSize)
			    - sc->sc_hdrlen;
			if (ucaa.ucaa_obufsize >= UFTDI_MAX_OBUFSIZE)
				ucaa.ucaa_obufsize = UFTDI_MAX_OBUFSIZE;
			/* Limit length if we have a 6-bit header.  */
			if ((sc->sc_hdrlen > 0) &&
			    (ucaa.ucaa_obufsize > UFTDIOBUFSIZE))
				ucaa.ucaa_obufsize = UFTDIOBUFSIZE;
		} else {
			aprint_error_dev(self, "unexpected endpoint\n");
			goto bad;
		}
	}
	if (ucaa.ucaa_bulkin == -1) {
		aprint_error_dev(self, "Could not find data bulk in\n");
		goto bad;
	}
	if (ucaa.ucaa_bulkout == -1) {
		aprint_error_dev(self, "Could not find data bulk out\n");
		goto bad;
	}

	/* ucaa_bulkin, ucaa_bulkout set above */
	if (ucaa.ucaa_ibufsize == 0)
		ucaa.ucaa_ibufsize = UFTDIIBUFSIZE;
	ucaa.ucaa_ibufsizepad = ucaa.ucaa_ibufsize;
	if (ucaa.ucaa_obufsize == 0)
		ucaa.ucaa_obufsize = UFTDIOBUFSIZE - sc->sc_hdrlen;
	ucaa.ucaa_opkthdrlen = sc->sc_hdrlen;
	ucaa.ucaa_device = dev;
	ucaa.ucaa_iface = iface;
	ucaa.ucaa_methods = &uftdi_methods;
	ucaa.ucaa_arg = sc;
	ucaa.ucaa_info = NULL;

	DPRINTF(("uftdi: in=%#x out=%#x isize=%#x osize=%#x\n",
		ucaa.ucaa_bulkin, ucaa.ucaa_bulkout,
		ucaa.ucaa_ibufsize, ucaa.ucaa_obufsize));
	sc->sc_subdev = config_found(self, &ucaa, ucomprint,
	    CFARGS(.submatch = ucomsubmatch));

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev, sc->sc_dev);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;

bad:
	DPRINTF(("uftdi_attach: ATTACH ERROR\n"));
	sc->sc_dying = true;
	return;
}

static void
uftdi_childdet(device_t self, device_t child)
{
	struct uftdi_softc *sc = device_private(self);

	KASSERT(child == sc->sc_subdev);
	sc->sc_subdev = NULL;
}

static int
uftdi_detach(device_t self, int flags)
{
	struct uftdi_softc *sc = device_private(self);
	int rv = 0;

	DPRINTF(("uftdi_detach: sc=%p flags=%d\n", sc, flags));

	sc->sc_dying = true;

	if (sc->sc_subdev != NULL) {
		rv = config_detach(sc->sc_subdev, flags);
		sc->sc_subdev = NULL;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev, sc->sc_dev);

	return rv;
}

static int
uftdi_open(void *vsc, int portno)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	usbd_status err;
	struct termios t;

	DPRINTF(("uftdi_open: sc=%p\n", sc));

	if (sc->sc_dying)
		return EIO;

	/* Perform a full reset on the device */
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_RESET;
	USETW(req.wValue, FTDI_SIO_RESET_SIO);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return EIO;

	/* Set 9600 baud, 2 stop bits, no parity, 8 bits */
	t.c_ospeed = 9600;
	t.c_cflag = CSTOPB | CS8;
	(void)uftdi_param(sc, portno, &t);

	/* Turn on RTS/CTS flow control */
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_FLOW_CTRL;
	USETW(req.wValue, 0);
	USETW2(req.wIndex, FTDI_SIO_RTS_CTS_HS, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return EIO;

	return 0;
}

static void
uftdi_read(void *vsc, int portno, u_char **ptr, uint32_t *count)
{
	struct uftdi_softc *sc = vsc;
	u_char msr, lsr;

	DPRINTFN(15,("uftdi_read: sc=%p, port=%d count=%d\n", sc, portno,
		     *count));

	msr = FTDI_GET_MSR(*ptr);
	lsr = FTDI_GET_LSR(*ptr);

#ifdef UFTDI_DEBUG
	if (*count != 2)
		DPRINTFN(10,("uftdi_read: sc=%p, port=%d count=%d data[0]="
			    "0x%02x\n", sc, portno, *count, (*ptr)[2]));
#endif

	if (sc->sc_msr != msr ||
	    (sc->sc_lsr & FTDI_LSR_MASK) != (lsr & FTDI_LSR_MASK)) {
		DPRINTF(("uftdi_read: status change msr=0x%02x(0x%02x) "
			 "lsr=0x%02x(0x%02x)\n", msr, sc->sc_msr,
			 lsr, sc->sc_lsr));
		sc->sc_msr = msr;
		sc->sc_lsr = lsr;
		ucom_status_change(device_private(sc->sc_subdev));
	}

	/* Adjust buffer pointer to skip status prefix */
	*ptr += 2;
}

static void
uftdi_write(void *vsc, int portno, u_char *to, u_char *from, uint32_t *count)
{
	struct uftdi_softc *sc = vsc;

	DPRINTFN(10,("uftdi_write: sc=%p, port=%d count=%u data[0]=0x%02x\n",
		     vsc, portno, *count, from[0]));

	/* Make length tag and copy data */
	if (sc->sc_hdrlen > 0)
		*to = FTDI_OUT_TAG(*count, portno);

	memcpy(to + sc->sc_hdrlen, from, *count);
	*count += sc->sc_hdrlen;
}

static void
uftdi_set(void *vsc, int portno, int reg, int onoff)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	int ctl;

	DPRINTF(("uftdi_set: sc=%p, port=%d reg=%d onoff=%d\n", vsc, portno,
		 reg, onoff));

	if (sc->sc_dying)
		return;

	switch (reg) {
	case UCOM_SET_DTR:
		ctl = onoff ? FTDI_SIO_SET_DTR_HIGH : FTDI_SIO_SET_DTR_LOW;
		break;
	case UCOM_SET_RTS:
		ctl = onoff ? FTDI_SIO_SET_RTS_HIGH : FTDI_SIO_SET_RTS_LOW;
		break;
	case UCOM_SET_BREAK:
		uftdi_break(sc, portno, onoff);
		return;
	default:
		return;
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_MODEM_CTRL;
	USETW(req.wValue, ctl);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_set: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	(void)usbd_do_request(sc->sc_udev, &req, NULL);
}

/*
 * Return true if the given speed is within operational tolerance of the target
 * speed.  FTDI recommends that the hardware speed be within 3% of nominal.
 */
static inline bool
uftdi_baud_within_tolerance(uint64_t speed, uint64_t target)
{
	return ((speed >= (target * 100) / 103) &&
	    (speed <= (target * 100) / 97));
}

static int
uftdi_encode_baudrate(struct uftdi_softc *sc, int speed, int *rate, int *ratehi)
{
	static const uint8_t encoded_fraction[8] = {
	    0, 3, 2, 4, 1, 5, 6, 7
	};
	static const uint8_t roundoff_232a[16] = {
	    0,  1,  0,  1,  0, -1,  2,  1,
	    0, -1, -2, -3,  4,  3,  2,  1,
	};
	uint32_t clk, divisor, fastclk_flag, frac, hwspeed;

	/*
	 * If this chip has the fast clock capability and the speed is within
	 * range, use the 12MHz clock, otherwise the standard clock is 3MHz.
	 */
	if ((sc->sc_flags & FLAGS_BAUDCLK_12M) && speed >= 1200) {
		clk = 12000000;
		fastclk_flag = (1 << 17);
	} else {
		clk = 3000000;
		fastclk_flag = 0;
	}

	/*
	 * Make sure the requested speed is reachable with the available clock
	 * and a 14-bit divisor.
	 */
	if (speed < (clk >> 14) || speed > clk)
		return -1;

	/*
	 * Calculate the divisor, initially yielding a fixed point number with a
	 * 4-bit (1/16ths) fraction, then round it to the nearest fraction the
	 * hardware can handle.  When the integral part of the divisor is
	 * greater than one, the fractional part is in 1/8ths of the base clock.
	 * The FT8U232AM chips can handle only 0.125, 0.250, and 0.5 fractions.
	 * Later chips can handle all 1/8th fractions.
	 *
	 * If the integral part of the divisor is 1, a special rule applies: the
	 * fractional part can only be .0 or .5 (this is a limitation of the
	 * hardware).  We handle this by truncating the fraction rather than
	 * rounding, because this only applies to the two fastest speeds the
	 * chip can achieve and rounding doesn't matter, either you've asked for
	 * that exact speed or you've asked for something the chip can't do.
	 *
	 * For the FT8U232AM chips, use a roundoff table to adjust the result
	 * to the nearest 1/8th fraction that is supported by the hardware,
	 * leaving a fixed-point number with a 3-bit fraction which exactly
	 * represents the math the hardware divider will do.  For later-series
	 * chips that support all 8 fractional divisors, just round 16ths to
	 * 8ths by adding 1 and dividing by 2.
	 */
	divisor = (clk << 4) / speed;
	if ((divisor & 0xf) == 1)
		divisor &= 0xfffffff8;
	else if (sc->sc_flags & FLAGS_ROUNDOFF_232A)
		divisor += roundoff_232a[divisor & 0x0f];
	else
		divisor += 1;  /* Rounds odd 16ths up to next 8th. */
	divisor >>= 1;

	/*
	 * Ensure the resulting hardware speed will be within operational
	 * tolerance (within 3% of nominal).
	 */
	hwspeed = (clk << 3) / divisor;
	if (!uftdi_baud_within_tolerance(hwspeed, speed))
		return -1;

	/*
	 * Re-pack the divisor into hardware format. The lower 14-bits hold the
	 * integral part, while the upper bits specify the fraction by indexing
	 * a table of fractions within the hardware which is laid out as:
	 *    {0.0, 0.5, 0.25, 0.125, 0.325, 0.625, 0.725, 0.875}
	 * The A-series chips only have the first four table entries; the
	 * roundoff table logic above ensures that the fractional part for those
	 * chips will be one of the first four values.
	 *
	 * When the divisor is 1 a special encoding applies:  1.0 is encoded as
	 * 0.0, and 1.5 is encoded as 1.0.  The rounding logic above has already
	 * ensured that the fraction is either .0 or .5 if the integral is 1.
	 */
	frac = divisor & 0x07;
	divisor >>= 3;
	if (divisor == 1) {
		if (frac == 0)
			divisor = 0;	/* 1.0 becomes 0.0 */
		else
			frac = 0;	/* 1.5 becomes 1.0 */
	}
	divisor |= (encoded_fraction[frac] << 14) | fastclk_flag;

	*rate = (uint16_t)divisor;
	*ratehi = (uint16_t)(divisor >> 16);

	/*
	 * If this chip requires the baud bits to be in the high byte of the
	 * index word, move the bits up to that location.
	 */
	if (sc->sc_flags & FLAGS_BAUDBITS_HINDEX)
		*ratehi <<= 8;

	return 0;
}

static int
uftdi_param(void *vsc, int portno, struct termios *t)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	usbd_status err;
	int rate, ratehi, rerr, data, flow;

	DPRINTF(("uftdi_param: sc=%p\n", sc));

	if (sc->sc_dying)
		return EIO;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_BITMODE;
	USETW(req.wValue, FTDI_BITMODE_RESET << 8 | 0x00);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return EIO;

	switch (sc->sc_type) {
	case UFTDI_TYPE_SIO:
		switch (t->c_ospeed) {
		case 300: rate = ftdi_sio_b300; break;
		case 600: rate = ftdi_sio_b600; break;
		case 1200: rate = ftdi_sio_b1200; break;
		case 2400: rate = ftdi_sio_b2400; break;
		case 4800: rate = ftdi_sio_b4800; break;
		case 9600: rate = ftdi_sio_b9600; break;
		case 19200: rate = ftdi_sio_b19200; break;
		case 38400: rate = ftdi_sio_b38400; break;
		case 57600: rate = ftdi_sio_b57600; break;
		case 115200: rate = ftdi_sio_b115200; break;
		default:
			return EINVAL;
		}
		ratehi = 0;
		break;
	case UFTDI_TYPE_8U232AM:
		rerr = uftdi_encode_baudrate(sc, t->c_ospeed, &rate, &ratehi);
		if (rerr != 0)
			return EINVAL;
		break;
	default:
		return EINVAL;
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_BAUD_RATE;
	USETW(req.wValue, rate);
	USETW(req.wIndex, portno | ratehi);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_param: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return EIO;

	if (ISSET(t->c_cflag, CSTOPB))
		data = FTDI_SIO_SET_DATA_STOP_BITS_2;
	else
		data = FTDI_SIO_SET_DATA_STOP_BITS_1;
	if (ISSET(t->c_cflag, PARENB)) {
		if (ISSET(t->c_cflag, PARODD))
			data |= FTDI_SIO_SET_DATA_PARITY_ODD;
		else
			data |= FTDI_SIO_SET_DATA_PARITY_EVEN;
	} else
		data |= FTDI_SIO_SET_DATA_PARITY_NONE;
	switch (ISSET(t->c_cflag, CSIZE)) {
	case CS5:
		data |= FTDI_SIO_SET_DATA_BITS(5);
		break;
	case CS6:
		data |= FTDI_SIO_SET_DATA_BITS(6);
		break;
	case CS7:
		data |= FTDI_SIO_SET_DATA_BITS(7);
		break;
	case CS8:
		data |= FTDI_SIO_SET_DATA_BITS(8);
		break;
	}
	sc->last_lcr = data;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_DATA;
	USETW(req.wValue, data);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_param: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return EIO;

	if (ISSET(t->c_cflag, CRTSCTS)) {
		flow = FTDI_SIO_RTS_CTS_HS;
		USETW(req.wValue, 0);
	} else if (ISSET(t->c_iflag, IXON) && ISSET(t->c_iflag, IXOFF)) {
		flow = FTDI_SIO_XON_XOFF_HS;
		USETW2(req.wValue, t->c_cc[VSTOP], t->c_cc[VSTART]);
	} else {
		flow = FTDI_SIO_DISABLE_FLOW_CTRL;
		USETW(req.wValue, 0);
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_FLOW_CTRL;
	USETW2(req.wIndex, flow, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return EIO;

	return 0;
}

static void
uftdi_get_status(void *vsc, int portno, u_char *lsr, u_char *msr)
{
	struct uftdi_softc *sc = vsc;

	DPRINTF(("uftdi_status: msr=0x%02x lsr=0x%02x\n",
		 sc->sc_msr, sc->sc_lsr));

	if (sc->sc_dying)
		return;

	*msr = sc->sc_msr;
	*lsr = sc->sc_lsr;
}

static void
uftdi_break(void *vsc, int portno, int onoff)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	int data;

	DPRINTF(("uftdi_break: sc=%p, port=%d onoff=%d\n", vsc, portno,
		  onoff));

	if (onoff) {
		data = sc->last_lcr | FTDI_SIO_SET_BREAK;
	} else {
		data = sc->last_lcr;
	}

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_DATA;
	USETW(req.wValue, data);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	(void)usbd_do_request(sc->sc_udev, &req, NULL);
}
