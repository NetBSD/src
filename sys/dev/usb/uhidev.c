/*	$NetBSD: uhidev.c,v 1.86 2022/03/28 12:43:39 riastradh Exp $	*/

/*
 * Copyright (c) 2001, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology and Matthew R. Green (mrg@eterna.com.au).
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

/*
 * HID spec: http://www.usb.org/developers/devclass_docs/HID1_11.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uhidev.c,v 1.86 2022/03/28 12:43:39 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/rndsource.h>
#include <sys/signalvar.h>
#include <sys/systm.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/usbdevs.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/uhidev.h>
#include <dev/hid/hid.h>

/* Report descriptor for broken Wacom Graphire */
#include <dev/usb/ugraphire_rdesc.h>
/* Report descriptor for game controllers in "XInput" mode */
#include <dev/usb/xinput_rdesc.h>
/* Report descriptor for Xbox One controllers */
#include <dev/usb/x1input_rdesc.h>

#include "locators.h"

struct uhidev_softc {
	device_t sc_dev;		/* base device */
	struct usbd_device *sc_udev;
	struct usbd_interface *sc_iface;	/* interface */
	int sc_iep_addr;
	int sc_oep_addr;
	u_int sc_isize;

	int sc_repdesc_size;
	void *sc_repdesc;

	u_int sc_nrepid;
	device_t *sc_subdevs;

	kmutex_t sc_lock;
	kcondvar_t sc_cv;

	/* Read/written under sc_lock.  */
	struct lwp *sc_writelock;
	struct lwp *sc_configlock;
	int sc_refcnt;
	int sc_writereportid;
	int sc_stopreportid;
	u_char sc_dying;

	/*
	 * - Read under sc_lock, provided sc_refcnt > 0.
	 * - Written under sc_configlock only when transitioning to and
	 *   from sc_refcnt = 0.
	 */
	u_char *sc_ibuf;
	struct usbd_pipe *sc_ipipe;	/* input interrupt pipe */
	struct usbd_pipe *sc_opipe;	/* output interrupt pipe */
	struct usbd_xfer *sc_oxfer;	/* write request */
	usbd_callback sc_writecallback;	/* async write request callback */
	void *sc_writecookie;

	u_int sc_flags;
#define UHIDEV_F_XB1	0x0001	/* Xbox 1 controller */
};

#ifdef UHIDEV_DEBUG
#define DPRINTF(x)	if (uhidevdebug) printf x
#define DPRINTFN(n,x)	if (uhidevdebug>(n)) printf x
int	uhidevdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

Static void uhidev_intr(struct usbd_xfer *, void *, usbd_status);

Static int uhidev_maxrepid(void *, int);
Static int uhidevprint(void *, const char *);

static int uhidev_match(device_t, cfdata_t, void *);
static void uhidev_attach(device_t, device_t, void *);
static void uhidev_childdet(device_t, device_t);
static int uhidev_detach(device_t, int);
static int uhidev_activate(device_t, enum devact);

CFATTACH_DECL2_NEW(uhidev, sizeof(struct uhidev_softc), uhidev_match,
    uhidev_attach, uhidev_detach, uhidev_activate, NULL, uhidev_childdet);

static int
uhidev_match(device_t parent, cfdata_t match, void *aux)
{
	struct usbif_attach_arg *uiaa = aux;

	/* Game controllers in "XInput" mode */
	if (USBIF_IS_XINPUT(uiaa))
		return UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO;
	/* Xbox One controllers */
	if (USBIF_IS_X1INPUT(uiaa) && uiaa->uiaa_ifaceno == 0)
		return UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO;

	if (uiaa->uiaa_class != UICLASS_HID)
		return UMATCH_NONE;
	if (usbd_get_quirks(uiaa->uiaa_device)->uq_flags & UQ_HID_IGNORE)
		return UMATCH_NONE;
	return UMATCH_IFACECLASS_GENERIC;
}

static void
uhidev_attach(device_t parent, device_t self, void *aux)
{
	struct uhidev_softc *sc = device_private(self);
	struct usbif_attach_arg *uiaa = aux;
	struct usbd_interface *iface = uiaa->uiaa_iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	struct uhidev_attach_arg uha;
	device_t dev;
	struct uhidev *csc;
	int maxinpktsize, size, nrepid, repid, repsz;
	int *repsizes;
	int i;
	void *desc;
	const void *descptr;
	usbd_status err;
	char *devinfop;
	int locs[UHIDBUSCF_NLOCS];

	sc->sc_dev = self;
	sc->sc_udev = uiaa->uiaa_device;
	sc->sc_iface = iface;

	aprint_naive("\n");
	aprint_normal("\n");

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	cv_init(&sc->sc_cv, "uhidev");
	sc->sc_writelock = NULL;
	sc->sc_configlock = NULL;
	sc->sc_refcnt = 0;
	sc->sc_writereportid = -1;
	sc->sc_stopreportid = -1;
	sc->sc_dying = false;

	id = usbd_get_interface_descriptor(iface);

	devinfop = usbd_devinfo_alloc(uiaa->uiaa_device, 0);
	aprint_normal_dev(self, "%s, iclass %d/%d\n",
	       devinfop, id->bInterfaceClass, id->bInterfaceSubClass);
	usbd_devinfo_free(devinfop);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	if (uiaa->uiaa_vendor == USB_VENDOR_WACOM) {
		if (uiaa->uiaa_product == USB_PRODUCT_WACOM_XD0912U) {
		/*
		 * Wacom Intuos2 (XD-0912-U) requires longer idle time to
		 * initialize the device with 0x0202.
		 */
			DELAY(500000);
		}
	}
	(void)usbd_set_idle(iface, 0, 0);

#if 0
	/*
	 * HID 1.11 says we should do this, but the device firmware is
	 * supposed to come up in Report Protocol after reset anyway, and
	 * apparently explicitly requesting it confuses some devices.
	 */
	if ((usbd_get_quirks(sc->sc_udev)->uq_flags & UQ_NO_SET_PROTO) == 0 &&
	    id->bInterfaceSubClass == UISUBCLASS_BOOT)
		(void)usbd_set_protocol(iface, 1);
#endif

	maxinpktsize = 0;
	sc->sc_iep_addr = sc->sc_oep_addr = -1;
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			aprint_error_dev(self,
			    "could not read endpoint descriptor\n");
			sc->sc_dying = 1;
			return;
		}

		DPRINTFN(10,("uhidev_attach: bLength=%d bDescriptorType=%d "
		    "bEndpointAddress=%d-%s bmAttributes=%d wMaxPacketSize=%d"
		    " bInterval=%d\n",
		    ed->bLength, ed->bDescriptorType,
		    ed->bEndpointAddress & UE_ADDR,
		    UE_GET_DIR(ed->bEndpointAddress)==UE_DIR_IN? "in" : "out",
		    ed->bmAttributes & UE_XFERTYPE,
		    UGETW(ed->wMaxPacketSize), ed->bInterval));

		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    (ed->bmAttributes & UE_XFERTYPE) == UE_INTERRUPT) {
			maxinpktsize = UGETW(ed->wMaxPacketSize);
			sc->sc_iep_addr = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
		    (ed->bmAttributes & UE_XFERTYPE) == UE_INTERRUPT) {
			sc->sc_oep_addr = ed->bEndpointAddress;
		} else {
			aprint_verbose_dev(self, "endpoint %d: ignored\n", i);
		}
	}

	/*
	 * Check that we found an input interrupt endpoint. The output interrupt
	 * endpoint is optional
	 */
	if (sc->sc_iep_addr == -1) {
		aprint_error_dev(self, "no input interrupt endpoint\n");
		sc->sc_dying = 1;
		return;
	}

	/* XXX need to extend this */
	descptr = NULL;
	if (uiaa->uiaa_vendor == USB_VENDOR_WACOM) {
		static uByte reportbuf[3];

		/* The report descriptor for the Wacom Graphire is broken. */
		switch (uiaa->uiaa_product) {
		case USB_PRODUCT_WACOM_GRAPHIRE3_4X5:
		case USB_PRODUCT_WACOM_GRAPHIRE3_6X8:
		case USB_PRODUCT_WACOM_GRAPHIRE4_4X5: /* The 6x8 too? */
			/*
			 * The Graphire3 needs 0x0202 to be written to
			 * feature report ID 2 before it'll start
			 * returning digitizer data.
			 */
			reportbuf[0] = 0x02;
			reportbuf[1] = 0x02;
			usbd_set_report(uiaa->uiaa_iface, UHID_FEATURE_REPORT, 2,
			    &reportbuf, 2);

			size = sizeof(uhid_graphire3_4x5_report_descr);
			descptr = uhid_graphire3_4x5_report_descr;
			break;
		case USB_PRODUCT_WACOM_GRAPHIRE:
		case USB_PRODUCT_WACOM_GRAPHIRE2:
		case USB_PRODUCT_WACOM_XD0912U:
		case USB_PRODUCT_WACOM_CTH690K0:
			reportbuf[0] = 0x02;
			reportbuf[1] = 0x02;
			usbd_set_report(uiaa->uiaa_iface, UHID_FEATURE_REPORT, 2,
			    &reportbuf, 2);
			break;
		default:
			/* Keep descriptor */
			break;
		}
	}
	if (USBIF_IS_XINPUT(uiaa)) {
		size = sizeof(uhid_xinput_report_descr);
		descptr = uhid_xinput_report_descr;
	}
	if (USBIF_IS_X1INPUT(uiaa)) {
		sc->sc_flags |= UHIDEV_F_XB1;
		size = sizeof(uhid_x1input_report_descr);
		descptr = uhid_x1input_report_descr;
	}

	if (descptr) {
		desc = kmem_alloc(size, KM_SLEEP);
		err = USBD_NORMAL_COMPLETION;
		memcpy(desc, descptr, size);
	} else {
		desc = NULL;
		err = usbd_read_report_desc(uiaa->uiaa_iface, &desc, &size);
	}
	if (err) {
		aprint_error_dev(self, "no report descriptor\n");
		sc->sc_dying = 1;
		return;
	}

	if (uiaa->uiaa_vendor == USB_VENDOR_HOSIDEN &&
	    uiaa->uiaa_product == USB_PRODUCT_HOSIDEN_PPP) {
		static uByte reportbuf[] = { 1 };
		/*
		 *  This device was sold by Konami with its ParaParaParadise
		 *  game for PlayStation2.  It needs to be "turned on"
		 *  before it will send any reports.
		 */

		usbd_set_report(uiaa->uiaa_iface, UHID_FEATURE_REPORT, 0,
		    &reportbuf, sizeof(reportbuf));
	}

	if (uiaa->uiaa_vendor == USB_VENDOR_LOGITECH &&
	    uiaa->uiaa_product == USB_PRODUCT_LOGITECH_CBT44 && size == 0xb1) {
		uint8_t *data = desc;
		/*
		 * This device has a odd USAGE_MINIMUM value that would
		 * cause the multimedia keys to have their usage number
		 * shifted up one usage.  Adjust so the usages are sane.
		 */

		if (data[0x56] == 0x19 && data[0x57] == 0x01 &&
		    data[0x58] == 0x2a && data[0x59] == 0x8c)
			data[0x57] = 0x00;
	}

	/*
	 * Enable the Six Axis and DualShock 3 controllers.
	 * See http://ps3.jim.sh/sixaxis/usb/
	 */
	if (uiaa->uiaa_vendor == USB_VENDOR_SONY &&
	    uiaa->uiaa_product == USB_PRODUCT_SONY_PS3CONTROLLER) {
		usb_device_request_t req;
		char data[17];
		int actlen;

		req.bmRequestType = UT_READ_CLASS_INTERFACE;
		req.bRequest = 1;
		USETW(req.wValue, 0x3f2);
		USETW(req.wIndex, 0);
		USETW(req.wLength, sizeof(data));

		usbd_do_request_flags(sc->sc_udev, &req, data,
			USBD_SHORT_XFER_OK, &actlen, USBD_DEFAULT_TIMEOUT);
	}

	sc->sc_repdesc = desc;
	sc->sc_repdesc_size = size;

	uha.uiaa = uiaa;
	nrepid = uhidev_maxrepid(desc, size);
	if (nrepid < 0)
		return;
	if (nrepid > 0)
		aprint_normal_dev(self, "%d report ids\n", nrepid);
	nrepid++;
	repsizes = kmem_alloc(nrepid * sizeof(*repsizes), KM_SLEEP);
	sc->sc_subdevs = kmem_zalloc(nrepid * sizeof(sc->sc_subdevs[0]),
	    KM_SLEEP);

	/* Just request max packet size for the interrupt pipe */
	sc->sc_isize = maxinpktsize;
	sc->sc_nrepid = nrepid;

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev, sc->sc_dev);

	for (repid = 0; repid < nrepid; repid++) {
		repsz = hid_report_size(desc, size, hid_input, repid);
		DPRINTF(("uhidev_match: repid=%d, repsz=%d\n", repid, repsz));
		repsizes[repid] = repsz;
	}

	DPRINTF(("uhidev_attach: isize=%d\n", sc->sc_isize));

	uha.parent = sc;
	for (repid = 0; repid < nrepid; repid++) {
		DPRINTF(("uhidev_match: try repid=%d\n", repid));
		if (hid_report_size(desc, size, hid_input, repid) == 0 &&
		    hid_report_size(desc, size, hid_output, repid) == 0 &&
		    hid_report_size(desc, size, hid_feature, repid) == 0) {
			;	/* already NULL in sc->sc_subdevs[repid] */
		} else {
			uha.reportid = repid;
			locs[UHIDBUSCF_REPORTID] = repid;

			dev = config_found(self, &uha, uhidevprint,
			    CFARGS(.submatch = config_stdsubmatch,
				   .locators = locs));
			sc->sc_subdevs[repid] = dev;
			if (dev != NULL) {
				csc = device_private(dev);
				csc->sc_in_rep_size = repsizes[repid];
#ifdef DIAGNOSTIC
				DPRINTF(("uhidev_match: repid=%d dev=%p\n",
					 repid, dev));
				if (csc->sc_intr == NULL) {
					kmem_free(repsizes,
					    nrepid * sizeof(*repsizes));
					aprint_error_dev(self,
					    "sc_intr == NULL\n");
					return;
				}
#endif
				rnd_attach_source(&csc->rnd_source,
						  device_xname(dev),
						  RND_TYPE_TTY,
						  RND_FLAG_DEFAULT);
			}
		}
	}
	kmem_free(repsizes, nrepid * sizeof(*repsizes));

	return;
}

int
uhidev_maxrepid(void *buf, int len)
{
	struct hid_data *d;
	struct hid_item h;
	int maxid;

	maxid = -1;
	h.report_ID = 0;
	for (d = hid_start_parse(buf, len, hid_none); hid_get_item(d, &h); )
		if ((int)h.report_ID > maxid)
			maxid = h.report_ID;
	hid_end_parse(d);
	return maxid;
}

int
uhidevprint(void *aux, const char *pnp)
{
	struct uhidev_attach_arg *uha = aux;

	if (pnp)
		aprint_normal("uhid at %s", pnp);
	if (uha->reportid != 0)
		aprint_normal(" reportid %d", uha->reportid);
	return UNCONF;
}

static int
uhidev_activate(device_t self, enum devact act)
{
	struct uhidev_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static void
uhidev_childdet(device_t self, device_t child)
{
	int i;
	struct uhidev_softc *sc = device_private(self);

	for (i = 0; i < sc->sc_nrepid; i++) {
		if (sc->sc_subdevs[i] == child)
			break;
	}
	KASSERT(i < sc->sc_nrepid);
	sc->sc_subdevs[i] = NULL;
}

static int
uhidev_detach(device_t self, int flags)
{
	struct uhidev_softc *sc = device_private(self);
	int i, rv;
	struct uhidev *csc;

	DPRINTF(("uhidev_detach: sc=%p flags=%d\n", sc, flags));

	/* Notify that we are going away.  */
	mutex_enter(&sc->sc_lock);
	sc->sc_dying = 1;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);

	/*
	 * Try to detach all our children.  If anything fails, bail.
	 * Failure can happen if this is from drvctl -d; of course, if
	 * this is a USB device being yanked, flags will have
	 * DETACH_FORCE and the children will not have the option of
	 * refusing detachment.
	 */
	for (i = 0; i < sc->sc_nrepid; i++) {
		if (sc->sc_subdevs[i] == NULL)
			continue;
		/*
		 * XXX rnd_detach_source should go in uhidev_childdet,
		 * but the struct krndsource lives in the child's
		 * softc, which is gone by the time of childdet.  The
		 * parent uhidev_softc should be changed to allocate
		 * the struct krndsource, not the child.
		 */
		csc = device_private(sc->sc_subdevs[i]);
		rnd_detach_source(&csc->rnd_source);
		rv = config_detach(sc->sc_subdevs[i], flags);
		if (rv) {
			rnd_attach_source(&csc->rnd_source,
			    device_xname(sc->sc_dev),
			    RND_TYPE_TTY, RND_FLAG_DEFAULT);
			mutex_enter(&sc->sc_lock);
			sc->sc_dying = 0;
			mutex_exit(&sc->sc_lock);
			return rv;
		}
	}

	KASSERTMSG(sc->sc_refcnt == 0,
	    "%s: %d refs remain", device_xname(sc->sc_dev), sc->sc_refcnt);
	KASSERT(sc->sc_opipe == NULL);
	KASSERT(sc->sc_ipipe == NULL);
	KASSERT(sc->sc_ibuf == NULL);

	if (sc->sc_repdesc != NULL) {
		kmem_free(sc->sc_repdesc, sc->sc_repdesc_size);
		sc->sc_repdesc = NULL;
	}
	if (sc->sc_subdevs != NULL) {
		int nrepid = sc->sc_nrepid;
		kmem_free(sc->sc_subdevs, nrepid * sizeof(sc->sc_subdevs[0]));
		sc->sc_subdevs = NULL;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev, sc->sc_dev);

	pmf_device_deregister(self);
	KASSERT(sc->sc_configlock == NULL);
	KASSERT(sc->sc_writelock == NULL);
	cv_destroy(&sc->sc_cv);
	mutex_destroy(&sc->sc_lock);

	return rv;
}

void
uhidev_intr(struct usbd_xfer *xfer, void *addr, usbd_status status)
{
	struct uhidev_softc *sc = addr;
	device_t cdev;
	struct uhidev *scd;
	u_char *p;
	u_int rep;
	uint32_t cc;

	usbd_get_xfer_status(xfer, NULL, NULL, &cc, NULL);

#ifdef UHIDEV_DEBUG
	if (uhidevdebug > 5) {
		uint32_t i;

		DPRINTF(("uhidev_intr: status=%d cc=%d\n", status, cc));
		DPRINTF(("uhidev_intr: data ="));
		for (i = 0; i < cc; i++)
			DPRINTF((" %02x", sc->sc_ibuf[i]));
		DPRINTF(("\n"));
	}
#endif

	if (status == USBD_CANCELLED)
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		DPRINTF(("%s: interrupt status=%d\n", device_xname(sc->sc_dev),
			 status));
		usbd_clear_endpoint_stall_async(sc->sc_ipipe);
		return;
	}

	p = sc->sc_ibuf;
	if (sc->sc_nrepid != 1)
		rep = *p++, cc--;
	else
		rep = 0;
	if (rep >= sc->sc_nrepid) {
		printf("uhidev_intr: bad repid %d\n", rep);
		return;
	}
	cdev = sc->sc_subdevs[rep];
	if (!cdev)
		return;
	scd = device_private(cdev);
	DPRINTFN(5,("uhidev_intr: rep=%d, scd=%p state=%#x\n",
		    rep, scd, scd ? scd->sc_state : 0));
	if (!(scd->sc_state & UHIDEV_OPEN))
		return;
#ifdef UHIDEV_DEBUG
	if (scd->sc_in_rep_size != cc) {
		DPRINTF(("%s: expected %d bytes, got %d\n",
		       device_xname(sc->sc_dev), scd->sc_in_rep_size, cc));
	}
#endif
	if (cc == 0) {
		DPRINTF(("%s: 0-length input ignored\n",
			device_xname(sc->sc_dev)));
		return;
	}
	rnd_add_uint32(&scd->rnd_source, (uintptr_t)(sc->sc_ibuf));
	scd->sc_intr(scd, p, cc);
}

void
uhidev_get_report_desc(struct uhidev_softc *sc, void **desc, int *size)
{
	*desc = sc->sc_repdesc;
	*size = sc->sc_repdesc_size;
}

static int
uhidev_config_enter(struct uhidev_softc *sc)
{
	int error;

	KASSERT(mutex_owned(&sc->sc_lock));

	for (;;) {
		if (sc->sc_dying)
			return ENXIO;
		if (sc->sc_configlock == NULL)
			break;
		error = cv_wait_sig(&sc->sc_cv, &sc->sc_lock);
		if (error)
			return error;
	}

	sc->sc_configlock = curlwp;
	return 0;
}

static void
uhidev_config_enter_nointr(struct uhidev_softc *sc)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	while (sc->sc_configlock)
		cv_wait(&sc->sc_cv, &sc->sc_lock);
	sc->sc_configlock = curlwp;
}

static void
uhidev_config_exit(struct uhidev_softc *sc)
{

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERTMSG(sc->sc_configlock == curlwp, "%s: migrated from %p to %p",
	    device_xname(sc->sc_dev), curlwp, sc->sc_configlock);

	sc->sc_configlock = NULL;
	cv_broadcast(&sc->sc_cv);
}

/*
 * uhidev_open_pipes(sc)
 *
 *	Ensure the pipes of the softc are open.  Caller must hold
 *	sc_lock, which may be released and reacquired.
 */
static int
uhidev_open_pipes(struct uhidev_softc *sc)
{
	usbd_status err;
	int error;

	KASSERT(mutex_owned(&sc->sc_lock));

	/* If the device is dying, refuse.  */
	if (sc->sc_dying)
		return ENXIO;

	/*
	 * If the pipes are already open, just increment the reference
	 * count, or fail if it would overflow.
	 */
	if (sc->sc_refcnt) {
		if (sc->sc_refcnt == INT_MAX)
			return EBUSY;
		sc->sc_refcnt++;
		return 0;
	}

	/*
	 * If there's no input data to prepare, don't bother with the
	 * pipes.  We assume any device that does output also does
	 * input; if you have a device where this is wrong, then
	 * uhidev_write will fail gracefully (it checks sc->sc_opipe),
	 * and you can use that device to test the changes needed to
	 * open the output pipe here.
	 */
	if (sc->sc_isize == 0)
		return 0;

	/*
	 * Lock the configuration and release sc_lock we may sleep to
	 * allocate.  If someone else got in first, we're done;
	 * otherwise open the pipes.
	 */
	error = uhidev_config_enter(sc);
	if (error)
		goto out;
	if (sc->sc_refcnt) {
		if (sc->sc_refcnt == INT_MAX) {
			error = EBUSY;
		} else {
			sc->sc_refcnt++;
			error = 0;
		}
		goto out0;
	}
	mutex_exit(&sc->sc_lock);

	/* Allocate an input buffer.  */
	sc->sc_ibuf = kmem_alloc(sc->sc_isize, KM_SLEEP);

	/* Set up input interrupt pipe. */
	DPRINTF(("%s: isize=%d, ep=0x%02x\n", __func__, sc->sc_isize,
		 sc->sc_iep_addr));

	err = usbd_open_pipe_intr(sc->sc_iface, sc->sc_iep_addr,
		  USBD_SHORT_XFER_OK, &sc->sc_ipipe, sc, sc->sc_ibuf,
		  sc->sc_isize, uhidev_intr, USBD_DEFAULT_INTERVAL);
	if (err != USBD_NORMAL_COMPLETION) {
		DPRINTF(("uhidopen: usbd_open_pipe_intr failed, "
		    "error=%d\n", err));
		error = EIO;
		goto out1;
	}

	/*
	 * Set up output interrupt pipe if an output interrupt endpoint
	 * exists.
	 */
	if (sc->sc_oep_addr != -1) {
		DPRINTF(("uhidev_open: oep=0x%02x\n", sc->sc_oep_addr));

		err = usbd_open_pipe(sc->sc_iface, sc->sc_oep_addr,
		    0, &sc->sc_opipe);

		if (err != USBD_NORMAL_COMPLETION) {
			DPRINTF(("uhidev_open: usbd_open_pipe failed, "
			    "error=%d\n", err));
			error = EIO;
			goto out2;
		}
		DPRINTF(("uhidev_open: sc->sc_opipe=%p\n", sc->sc_opipe));

		error = usbd_create_xfer(sc->sc_opipe, UHIDEV_OSIZE, 0, 0,
		    &sc->sc_oxfer);
		if (error) {
			DPRINTF(("uhidev_open: couldn't allocate an xfer\n"));
			goto out3;
		}

		if (sc->sc_flags & UHIDEV_F_XB1) {
			uint8_t init_data[] = { 0x05, 0x20 };
			int init_data_len = sizeof(init_data);
			err = usbd_intr_transfer(sc->sc_oxfer, sc->sc_opipe, 0,
			    USBD_NO_TIMEOUT, init_data, &init_data_len);
			if (err != USBD_NORMAL_COMPLETION) {
				DPRINTF(("uhidev_open: xb1 init failed, "
				    "error=%d\n", err));
				error = EIO;
				goto out4;
			}
		}
	}

	/* Success!  */
	mutex_enter(&sc->sc_lock);
	KASSERTMSG(sc->sc_refcnt == 0, "%d refs spuriously acquired",
	    sc->sc_refcnt);
	sc->sc_refcnt++;
	goto out0;

out4:	if (sc->sc_oxfer) {
		usbd_abort_pipe(sc->sc_opipe);
		usbd_destroy_xfer(sc->sc_oxfer);
		sc->sc_oxfer = NULL;
	}
out3:	if (sc->sc_opipe) {
		usbd_close_pipe(sc->sc_opipe);
		sc->sc_opipe = NULL;
	}
out2:	if (sc->sc_ipipe) {
		usbd_abort_pipe(sc->sc_ipipe);
		usbd_close_pipe(sc->sc_ipipe);
		sc->sc_ipipe = NULL;
	}
out1:	kmem_free(sc->sc_ibuf, sc->sc_isize);
	sc->sc_ibuf = NULL;
	mutex_enter(&sc->sc_lock);
out0:	KASSERT(mutex_owned(&sc->sc_lock));
	uhidev_config_exit(sc);
out:	KASSERT(mutex_owned(&sc->sc_lock));
	return error;
}

static void
uhidev_close_pipes(struct uhidev_softc *sc)
{

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERTMSG(sc->sc_refcnt > 0, "%s: refcnt fouled: %d",
	    device_xname(sc->sc_dev), sc->sc_refcnt);

	/* If this isn't the last reference, just decrement.  */
	if (sc->sc_refcnt > 1) {
		sc->sc_refcnt--;
		return;
	}

	/*
	 * Lock the configuration and release sc_lock so we may sleep
	 * to free memory.  We're not waiting for anyone to allocate or
	 * free anything.
	 */
	uhidev_config_enter_nointr(sc);

	/*
	 * If someone else acquired a reference while we were waiting
	 * for the config lock, nothing more for us to do.
	 */
	if (sc->sc_refcnt > 1) {
		sc->sc_refcnt--;
		uhidev_config_exit(sc);
		return;
	}

	/*
	 * We're the last reference and committed to closing the pipes.
	 * Decrement the reference count before we release the lock --
	 * access to the pipes is allowed as long as the reference
	 * count is positive, so this forces all new opens to wait
	 * until the config lock is released.
	 */
	KASSERTMSG(sc->sc_refcnt == 1, "%s: refcnt fouled: %d",
	    device_xname(sc->sc_dev), sc->sc_refcnt);
	sc->sc_refcnt--;
	mutex_exit(&sc->sc_lock);

	if (sc->sc_oxfer) {
		usbd_abort_pipe(sc->sc_opipe);
		usbd_destroy_xfer(sc->sc_oxfer);
		sc->sc_oxfer = NULL;
	}
	if (sc->sc_opipe) {
		usbd_close_pipe(sc->sc_opipe);
		sc->sc_opipe = NULL;
	}
	if (sc->sc_ipipe) {
		usbd_abort_pipe(sc->sc_ipipe);
		usbd_close_pipe(sc->sc_ipipe);
		sc->sc_ipipe = NULL;
	}
	kmem_free(sc->sc_ibuf, sc->sc_isize);
	sc->sc_ibuf = NULL;

	mutex_enter(&sc->sc_lock);
	uhidev_config_exit(sc);
	KASSERTMSG(sc->sc_refcnt == 0, "%s: refcnt fouled: %d",
	    device_xname(sc->sc_dev), sc->sc_refcnt);
}

int
uhidev_open(struct uhidev *scd)
{
	struct uhidev_softc *sc = scd->sc_parent;
	int error;

	mutex_enter(&sc->sc_lock);

	DPRINTF(("uhidev_open(%s, report %d = %s): state=%x refcnt=%d\n",
		device_xname(sc->sc_dev),
		scd->sc_report_id,
		device_xname(scd->sc_dev),
		scd->sc_state,
		sc->sc_refcnt));

	/* Mark the report id open.  This is an exclusive lock.  */
	if (scd->sc_state & UHIDEV_OPEN) {
		error = EBUSY;
		goto fail0;
	}
	scd->sc_state |= UHIDEV_OPEN;

	/* Open the pipes which are shared by all report ids.  */
	error = uhidev_open_pipes(sc);
	if (error)
		goto fail1;

	mutex_exit(&sc->sc_lock);

	/* Success!  */
	return 0;

fail2: __unused
	uhidev_close_pipes(sc);
fail1:	KASSERTMSG(scd->sc_state & UHIDEV_OPEN,
	    "%s: report id %d: closed while opening",
	    device_xname(sc->sc_dev), scd->sc_report_id);
	scd->sc_state &= ~UHIDEV_OPEN;
fail0:	mutex_exit(&sc->sc_lock);
	return error;
}

/*
 * uhidev_stop(scd)
 *
 *	Make all current and future output reports or xfers by scd to
 *	the output pipe to fail.  Caller must then ensure no more will
 *	be submitted and then call uhidev_close.
 *
 *	Side effect: If uhidev_write was in progress for this scd,
 *	blocks all other uhidev_writes until uhidev_close on this scd.
 *
 *	May sleep but only for a short duration to wait for USB
 *	transfer completion callbacks to run.
 */
void
uhidev_stop(struct uhidev *scd)
{
	struct uhidev_softc *sc = scd->sc_parent;

	mutex_enter(&sc->sc_lock);

	/* Prevent further writes on this report from starting.  */
	scd->sc_state |= UHIDEV_STOPPED;

	/* If there's no output pipe at all, nothing to do.  */
	if (sc->sc_opipe == NULL)
		goto out;

	/*
	 * If there's no write on this report in progress, nothing to
	 * do -- any subsequent attempts will be prevented by
	 * UHIDEV_STOPPED.
	 */
	if (sc->sc_writereportid != scd->sc_report_id)
		goto out;

	/*
	 * Caller must wait for uhidev_open to succeed before calling
	 * uhidev_write, and must wait for all uhidev_writes to return
	 * before calling uhidev_close, so neither on can be in flight
	 * right now.
	 *
	 * Suspend the pipe, but hold up uhidev_write from any report
	 * until we confirm this one has finished.  We will resume the
	 * pipe only after all uhidev_writes on this report have
	 * finished -- when the caller calls uhidev_close.
	 */
	KASSERTMSG(sc->sc_stopreportid == -1, "%d", sc->sc_stopreportid);
	sc->sc_stopreportid = scd->sc_report_id;
	mutex_exit(&sc->sc_lock);

	usbd_suspend_pipe(sc->sc_opipe);

	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_stopreportid == scd->sc_report_id);
	sc->sc_stopreportid = scd->sc_report_id;
	cv_broadcast(&sc->sc_cv);
out:	mutex_exit(&sc->sc_lock);
}

/*
 * uhidev_close(scd)
 *
 *	Close a uhidev previously opened with uhidev_open.  If writes
 *	had been stopped with uhidev_stop, allow writes at other report
 *	ids again.
 */
void
uhidev_close(struct uhidev *scd)
{
	struct uhidev_softc *sc = scd->sc_parent;

	mutex_enter(&sc->sc_lock);

	DPRINTF(("uhidev_close(%s, report %d = %s): state=%x refcnt=%d\n",
		device_xname(sc->sc_dev),
		scd->sc_report_id,
		device_xname(scd->sc_dev),
		scd->sc_state,
		sc->sc_refcnt));

	KASSERTMSG(scd->sc_state & UHIDEV_OPEN,
	    "%s: report id %d: unpaired close",
	    device_xname(sc->sc_dev), scd->sc_report_id);

	/*
	 * If the caller had issued uhidev_stop to interrupt a write
	 * for this report, then resume the pipe now that no further
	 * uhidev_write on the same report is possible, and wake anyone
	 * trying to write on other reports.
	 */
	if (sc->sc_stopreportid == scd->sc_report_id) {
		KASSERT(scd->sc_state & UHIDEV_STOPPED);
		mutex_exit(&sc->sc_lock);

		usbd_resume_pipe(sc->sc_opipe);

		mutex_enter(&sc->sc_lock);
		KASSERT(sc->sc_stopreportid == scd->sc_report_id);
		KASSERT(scd->sc_state & UHIDEV_STOPPED);
		sc->sc_stopreportid = -1;
		cv_broadcast(&sc->sc_cv);
	}

	/*
	 * Close our reference to the pipes, and mark our report as no
	 * longer open.  If it was stopped, clear that too -- drivers
	 * are forbidden from issuing writes after uhidev_close anyway.
	 */
	KASSERT(scd->sc_state & UHIDEV_OPEN);
	uhidev_close_pipes(sc);
	KASSERT(scd->sc_state & UHIDEV_OPEN);
	scd->sc_state &= ~(UHIDEV_OPEN | UHIDEV_STOPPED);

	mutex_exit(&sc->sc_lock);
}

usbd_status
uhidev_set_report(struct uhidev *scd, int type, void *data, int len)
{
	char *buf;
	usbd_status retstat;

	if (scd->sc_report_id == 0)
		return usbd_set_report(scd->sc_parent->sc_iface, type,
				       scd->sc_report_id, data, len);

	buf = kmem_alloc(len + 1, KM_SLEEP);
	buf[0] = scd->sc_report_id;
	memcpy(buf+1, data, len);

	retstat = usbd_set_report(scd->sc_parent->sc_iface, type,
				  scd->sc_report_id, buf, len + 1);

	kmem_free(buf, len + 1);

	return retstat;
}

usbd_status
uhidev_get_report(struct uhidev *scd, int type, void *data, int len)
{
	return usbd_get_report(scd->sc_parent->sc_iface, type,
			       scd->sc_report_id, data, len);
}

usbd_status
uhidev_write(struct uhidev *scd, void *data, int len)
{
	struct uhidev_softc *sc = scd->sc_parent;
	usbd_status err;

	DPRINTF(("uhidev_write: data=%p, len=%d\n", data, len));

	if (sc->sc_opipe == NULL)
		return USBD_INVAL;

	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_refcnt);
	for (;;) {
		if (sc->sc_dying) {
			err = USBD_IOERROR;
			goto out;
		}
		if (scd->sc_state & UHIDEV_STOPPED) {
			err = USBD_CANCELLED;
			goto out;
		}
		if (sc->sc_writelock == NULL && sc->sc_stopreportid == -1)
			break;
		if (cv_wait_sig(&sc->sc_cv, &sc->sc_lock)) {
			err = USBD_INTERRUPTED;
			goto out;
		}
	}
	sc->sc_writelock = curlwp;
	sc->sc_writereportid = scd->sc_report_id;
	mutex_exit(&sc->sc_lock);

#ifdef UHIDEV_DEBUG
	if (uhidevdebug > 50) {

		uint32_t i;
		uint8_t *d = data;

		DPRINTF(("uhidev_write: data ="));
		for (i = 0; i < len; i++)
			DPRINTF((" %02x", d[i]));
		DPRINTF(("\n"));
	}
#endif
	err = usbd_intr_transfer(sc->sc_oxfer, sc->sc_opipe, 0,
	    USBD_NO_TIMEOUT, data, &len);

	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_refcnt);
	KASSERTMSG(sc->sc_writelock == curlwp, "%s: migrated from %p to %p",
	    device_xname(sc->sc_dev), curlwp, sc->sc_writelock);
	KASSERTMSG(sc->sc_writereportid == scd->sc_report_id,
	    "%s: changed write report ids from %d to %d",
	    device_xname(sc->sc_dev), scd->sc_report_id, sc->sc_writereportid);
	sc->sc_writereportid = -1;
	sc->sc_writelock = NULL;
	cv_broadcast(&sc->sc_cv);
out:	mutex_exit(&sc->sc_lock);
	return err;
}

static void
uhidev_write_callback(struct usbd_xfer *xfer, void *cookie, usbd_status err)
{
	struct uhidev_softc *sc = cookie;
	usbd_callback writecallback;
	void *writecookie;

	if (err) {
		if (err != USBD_CANCELLED)
			usbd_clear_endpoint_stall_async(sc->sc_opipe);
	}

	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_writelock == (void *)1);
	writecallback = sc->sc_writecallback;
	writecookie = sc->sc_writecookie;
	sc->sc_writereportid = -1;
	sc->sc_writelock = NULL;
	sc->sc_writecallback = NULL;
	sc->sc_writecookie = NULL;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);

	(*writecallback)(xfer, writecookie, err);
}

usbd_status
uhidev_write_async(struct uhidev *scd, void *data, int len, int flags,
    int timo, usbd_callback writecallback, void *writecookie)
{
	struct uhidev_softc *sc = scd->sc_parent;
	usbd_status err;

	DPRINTF(("%s: data=%p, len=%d\n", __func__, data, len));

	if (sc->sc_opipe == NULL)
		return USBD_INVAL;

	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_refcnt);
	if (sc->sc_dying) {
		err = USBD_IOERROR;
		goto out;
	}
	if (scd->sc_state & UHIDEV_STOPPED) {
		err = USBD_CANCELLED;
		goto out;
	}
	if (sc->sc_writelock != NULL || sc->sc_stopreportid != -1) {
		err = USBD_IN_USE;
		goto out;
	}
	sc->sc_writelock = (void *)1; /* XXX no lwp to attribute async xfer */
	sc->sc_writereportid = scd->sc_report_id;
	sc->sc_writecallback = writecallback;
	sc->sc_writecookie = writecookie;
	usbd_setup_xfer(sc->sc_oxfer, sc, data, len, flags, timo,
	    uhidev_write_callback);
	err = usbd_transfer(sc->sc_oxfer);
	switch (err) {
	case USBD_IN_PROGRESS:
		break;
	case USBD_NORMAL_COMPLETION:
		panic("unexpected normal completion of async xfer under lock");
	default:		/* error */
		sc->sc_writelock = NULL;
		sc->sc_writereportid = -1;
		sc->sc_writecallback = NULL;
		sc->sc_writecookie = NULL;
		cv_broadcast(&sc->sc_cv);
	}
out:	mutex_exit(&sc->sc_lock);
	return err;
}
