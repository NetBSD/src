/*	$NetBSD: ualea.c,v 1.1 2017/04/17 08:59:37 riastradh Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__KERNEL_RCSID(0, "$NetBSD: ualea.c,v 1.1 2017/04/17 08:59:37 riastradh Exp $");

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/device_if.h>
#include <sys/kmem.h>
#include <sys/rndpool.h>
#include <sys/rndsource.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>

struct ualea_softc {
	device_t		sc_dev;
	struct usbd_device	*sc_udev;
	struct usbd_interface	*sc_uif;
	kmutex_t		sc_lock;
	krndsource_t		sc_rnd;
	uint16_t		sc_maxpktsize;
	struct usbd_pipe	*sc_pipe;
	struct usbd_xfer	*sc_xfer;
	bool			sc_attached:1;
	bool			sc_inflight:1;
	/*
	 * attached true->false only under lock
	 * inflight false->true only under lock
	 * issue xfer only under lock, if attached=true and inflight=false
	 */
};

static int	ualea_match(device_t, cfdata_t, void *);
static void	ualea_attach(device_t, device_t, void *);
static int	ualea_detach(device_t, int);
static void	ualea_get(size_t, void *);
static void	ualea_xfer_done(struct usbd_xfer *, void *, usbd_status);

CFATTACH_DECL_NEW(ualea, sizeof(struct ualea_softc),
    ualea_match, ualea_attach, ualea_detach, NULL);

static const struct usb_devno ualea_devs[] = {
	{ USB_VENDOR_ARANEUS,	USB_PRODUCT_ARANEUS_ALEA_II },
};

static int
ualea_match(device_t parent, cfdata_t match, void *aux)
{
	struct usbif_attach_arg *uiaa = aux;

	if (usb_lookup(ualea_devs, uiaa->uiaa_vendor, uiaa->uiaa_product))
		return UMATCH_VENDOR_PRODUCT;

	return UMATCH_NONE;
}

static void
ualea_attach(device_t parent, device_t self, void *aux)
{
	struct usbif_attach_arg *uiaa = aux;
	struct ualea_softc *sc = device_private(self);
	struct usbd_device *udev = uiaa->uiaa_device;
	struct usbd_interface *uif = uiaa->uiaa_iface;
	const usb_endpoint_descriptor_t *ed;
	char *devinfop;
	usbd_status status;

	aprint_naive("\n");
	aprint_normal("\n");

	devinfop = usbd_devinfo_alloc(udev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	sc->sc_dev = self;
	sc->sc_udev = udev;
	sc->sc_uif = uif;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_USB);
	rndsource_setcb(&sc->sc_rnd, ualea_get, sc);
	rnd_attach_source(&sc->sc_rnd, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);

	ed = usbd_interface2endpoint_descriptor(uif, 0);
	if (ed == NULL) {
		aprint_error_dev(sc->sc_dev, "failed to read endpoint 0\n");
		return;
	}
	if (UE_GET_DIR(ed->bEndpointAddress) != UE_DIR_IN ||
	    UE_GET_XFERTYPE(ed->bmAttributes) != UE_BULK) {
		aprint_error_dev(sc->sc_dev, "invalid endpoint\n");
		return;
	}

	sc->sc_maxpktsize = UGETW(ed->wMaxPacketSize);

	status = usbd_open_pipe(uif, ed->bEndpointAddress,
	    USBD_EXCLUSIVE_USE|USBD_MPSAFE, &sc->sc_pipe);
	if (status) {
		aprint_error_dev(sc->sc_dev, "failed to open pipe: %d\n",
		    status);
		return;
	}

	status = usbd_create_xfer(sc->sc_pipe, sc->sc_maxpktsize,
	    USBD_SHORT_XFER_OK, 0, &sc->sc_xfer);
	if (status) {
		aprint_error_dev(sc->sc_dev, "failed to create xfer: %d\n",
		    status);
		return;
	}

	usbd_setup_xfer(sc->sc_xfer, sc, usbd_get_buffer(sc->sc_xfer),
	    sc->sc_maxpktsize, USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT,
	    ualea_xfer_done);

	/* Success!  We are ready to run.  */
	sc->sc_attached = true;

	/* Get some initial entropy now.  */
	ualea_get(RND_POOLBITS/NBBY, sc);
}

static int
ualea_detach(device_t self, int flags)
{
	struct ualea_softc *sc = device_private(self);

	/* Prevent new use of xfer.  */
	mutex_enter(&sc->sc_lock);
	sc->sc_attached = false;
	mutex_exit(&sc->sc_lock);

	/* Cancel pending xfer.  */
	if (sc->sc_pipe)
		(void)usbd_abort_pipe(sc->sc_pipe);
	KASSERT(!sc->sc_inflight);

	/* All users have drained.  Tear it all down.  */
	if (sc->sc_xfer)
		usbd_destroy_xfer(sc->sc_xfer);
	if (sc->sc_pipe)
		(void)usbd_close_pipe(sc->sc_pipe);
	rnd_detach_source(&sc->sc_rnd);
	mutex_destroy(&sc->sc_lock);

	return 0;
}

static void
ualea_get(size_t nbytes, void *cookie)
{
	struct ualea_softc *sc = cookie;
	usbd_status status;

	mutex_enter(&sc->sc_lock);
	if (!sc->sc_attached)
		goto out;
	if (sc->sc_inflight)
		goto out;
	sc->sc_inflight = true;
	status = usbd_transfer(sc->sc_xfer);
	if (status && status != USBD_IN_PROGRESS) {
		aprint_error_dev(sc->sc_dev, "failed to issue xfer: %d\n",
		    status);
		/* We failed -- let someone else have a go.  */
		sc->sc_inflight = false;
	}
out:	mutex_exit(&sc->sc_lock);
}

static void
ualea_xfer_done(struct usbd_xfer *xfer, void *cookie, usbd_status status)
{
	struct ualea_softc *sc = cookie;
	void *pkt;
	uint32_t pktsize;

	/* Check the transfer status.  */
	if (status) {
		aprint_error_dev(sc->sc_dev, "xfer failed: %d\n", status);
		goto out;
	}

	/* Get and sanity-check the transferred size.  */
	usbd_get_xfer_status(xfer, NULL, &pkt, &pktsize, NULL);
	if (pktsize > sc->sc_maxpktsize) {
		aprint_error_dev(sc->sc_dev,
		    "bogus packet size: %"PRIu32" > %"PRIu16" (max), ignoring"
		    "\n",
		    pktsize, sc->sc_maxpktsize);
		goto out;
	}

	/* Add the data to the pool.  */
	rnd_add_data(&sc->sc_rnd, pkt, pktsize, NBBY*pktsize);

out:
	/* Allow subsequent transfers.  */
	sc->sc_inflight = false;
}
