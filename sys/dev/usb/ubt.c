/*	$NetBSD: ubt.c,v 1.7 2003/01/20 21:14:57 augustss Exp $	*/

/*
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) and
 * David Sainty (David.Sainty@dtsp.co.nz).
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
__KERNEL_RCSID(0, "$NetBSD: ubt.c,v 1.7 2003/01/20 21:14:57 augustss Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>

#include <dev/usb/ubtreg.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/bluetooth/bluetooth.h>

#ifdef UBT_DEBUG
#define DPRINTF(x)	if (ubtdebug) logprintf x
#define DPRINTFN(n,x)	if (ubtdebug>(n)) logprintf x
int	ubtdebug = 49;
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

	/* Control */
	usbd_pipe_handle        sc_ctl_pipe;
	usbd_xfer_handle        sc_ctl_xfer;
	u_int8_t                *sc_ctl_buf;

	/* Events */
	int			sc_evt_addr;
	usbd_pipe_handle	sc_evt_pipe;
	usbd_xfer_handle	sc_evt_xfer;
	u_int8_t		*sc_evt_buf;

	/* ACL data (in) */
	int			sc_aclrd_addr;
	usbd_pipe_handle	sc_aclrd_pipe;
	usbd_xfer_handle	sc_aclrd_xfer;
	u_int8_t		*sc_aclrd_buf;
	int			sc_aclrd_running;

	/* ACL data (out) */
	int			sc_aclwr_addr;
	usbd_pipe_handle	sc_aclwr_pipe;
	usbd_xfer_handle	sc_aclwr_xfer;
	u_int8_t		*sc_aclwr_buf;

	struct device		*sc_child;
	struct btframe_callback_methods const *sc_cb;

	unsigned int		sc_blocked;

	int			sc_refcnt;
	char			sc_dying;
};

static int ubt_open(void *h, int flag, int mode, usb_proc_ptr p);
static int ubt_close(void *h, int flag, int mode, usb_proc_ptr p);

static u_int8_t* ubt_alloc_control(void*, size_t, struct btframe_buffer**);
static int ubt_send_control(void*, struct btframe_buffer*, size_t);

static u_int8_t* ubt_alloc_acldata(void*, size_t, struct btframe_buffer**);
static int ubt_send_acldata(void*, struct btframe_buffer*, size_t);

static u_int8_t* ubt_alloc_scodata(void*, size_t, struct btframe_buffer**);
static int ubt_send_scodata(void*, struct btframe_buffer*, size_t);

static int ubt_splraise(void);
static unsigned int ubt_blockcb(void *h, unsigned int cbblocks);
static unsigned int ubt_unblockcb(void *h, unsigned int cbblocks);

static void ubt_event_cb(usbd_xfer_handle, usbd_private_handle, usbd_status);
static void ubt_aclrd_cb(usbd_xfer_handle, usbd_private_handle, usbd_status);
static void ubt_aclrd_request(struct ubt_softc *);

static struct btframe_methods const ubt_methods = {
	ubt_open, ubt_close,
	{ubt_alloc_control, ubt_send_control},
	{ubt_alloc_acldata, ubt_send_acldata},
	{ubt_alloc_scodata, ubt_send_scodata},
	ubt_splraise, ubt_blockcb, ubt_unblockcb
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
	usb_interface_descriptor_t const *id;
	char			devinfo[1024];
	usb_endpoint_descriptor_t const *ed;
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

	sc->sc_evt_addr = -1;
	sc->sc_aclrd_addr = -1;
	sc->sc_aclwr_addr = -1;

	for (i = 0; i < epcount; i++) {
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			printf("%s: couldn't get ep %d\n",
			    USBDEVNAME(sc->sc_dev), i);
			USB_ATTACH_ERROR_RETURN;
		}

		DPRINTFN(10, ("%s: addr=%d attr=%d\n", __func__,
			      (int)ed->bEndpointAddress,
			      (int)ed->bmAttributes));

		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
			sc->sc_evt_addr = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->sc_aclrd_addr = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->sc_aclwr_addr = ed->bEndpointAddress;
		}
	}

	if (sc->sc_evt_addr == -1 ||
	    sc->sc_aclrd_addr == -1 || sc->sc_aclwr_addr == -1) {
		printf("%s: missing endpoint\n", USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

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

	bt.bt_methods = &ubt_methods;
	bt.bt_cb = &sc->sc_cb;

	sc->sc_child = config_found(self, &bt, bt_print);

	USB_ATTACH_SUCCESS_RETURN;
}

static void
ubt_abortdealloc(struct ubt_softc *sc)
{
	DPRINTFN(0, ("%s: sc=%p\n", __func__, sc));

	if (sc->sc_evt_pipe != NULL) {
		usbd_abort_pipe(sc->sc_evt_pipe);
		usbd_close_pipe(sc->sc_evt_pipe);
		sc->sc_evt_pipe = NULL;
	}
	if (sc->sc_evt_buf != NULL) {
		free(sc->sc_evt_buf, M_USBDEV);
		sc->sc_evt_buf = NULL;
	}
	if (sc->sc_aclrd_pipe != NULL) {
		usbd_abort_pipe(sc->sc_aclrd_pipe);
		usbd_close_pipe(sc->sc_aclrd_pipe);
		sc->sc_aclrd_pipe = NULL;
	}
	if (sc->sc_aclwr_pipe != NULL) {
		usbd_abort_pipe(sc->sc_aclwr_pipe);
		usbd_close_pipe(sc->sc_aclwr_pipe);
		sc->sc_aclwr_pipe = NULL;
	}
	if (sc->sc_aclrd_xfer != NULL) {
		usbd_free_xfer(sc->sc_aclrd_xfer);
		sc->sc_aclrd_xfer = NULL;
		sc->sc_aclrd_buf = NULL;
	}
	if (sc->sc_aclwr_xfer != NULL) {
		usbd_free_xfer(sc->sc_aclwr_xfer);
		sc->sc_aclwr_xfer = NULL;
		sc->sc_aclwr_buf = NULL;
	}
}

USB_DETACH(ubt)
{
	USB_DETACH_START(ubt, sc);
	int s;
	int rv = 0;

	DPRINTF(("%s: sc=%p flags=%d\n", __func__, sc, flags));

	sc->sc_dying = 1;

	/* Abort all pipes.  Causes processes waiting for transfer to wake. */
	ubt_abortdealloc(sc);

	DPRINTFN(1, ("%s: waiting for USB detach\n", __func__));
	s = splusb();
	if (--sc->sc_refcnt >= 0) {
		/* Wait for processes to go away. */
		usb_detach_wait(USBDEV(sc->sc_dev));
	}
	splx(s);
	DPRINTFN(1, ("%s: USB detach complete\n", __func__));

	if (sc->sc_child != NULL) {
		DPRINTFN(1, ("%s: waiting for child detach\n", __func__));
		rv = config_detach(sc->sc_child, flags);
		sc->sc_child = NULL;
		DPRINTFN(1, ("%s: child detach complete\n", __func__));
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	DPRINTFN(1, ("%s: driver detached\n", __func__));

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

static int
ubt_open(void *h, int flag, int mode, usb_proc_ptr p)
{
	struct ubt_softc *sc = h;
	int error;
	usbd_status err;

	DPRINTFN(0, ("%s: sc=%p\n", __func__, sc));

	sc->sc_evt_buf = malloc(BTHCI_EVENT_MAX_LEN, M_USBDEV, M_NOWAIT);
	if (sc->sc_evt_buf == NULL) {
		error = ENOMEM;
		goto bad0;
	}

	err = usbd_open_pipe_intr(sc->sc_ctl_iface, sc->sc_evt_addr,
				  USBD_SHORT_XFER_OK, &sc->sc_evt_pipe,
				  sc, sc->sc_evt_buf, BTHCI_EVENT_MAX_LEN,
				  ubt_event_cb, UBT_EVENT_EP_INTERVAL);
	if (err != USBD_NORMAL_COMPLETION) {
		error = EIO;
		goto bad1;
	}

	err = usbd_open_pipe(sc->sc_ctl_iface, sc->sc_aclrd_addr,
			     0, &sc->sc_aclrd_pipe);
	if (err != USBD_NORMAL_COMPLETION) {
		error = EIO;
		goto bad2;
	}

	err = usbd_open_pipe(sc->sc_ctl_iface, sc->sc_aclwr_addr,
			     0, &sc->sc_aclwr_pipe);
	if (err != USBD_NORMAL_COMPLETION) {
		error = EIO;
		goto bad3;
	}

	sc->sc_ctl_xfer = usbd_alloc_xfer(sc->sc_udev);
	if (sc->sc_ctl_xfer == NULL) {
		error = ENOMEM;
		goto bad4;
	}
	sc->sc_aclrd_xfer = usbd_alloc_xfer(sc->sc_udev);
	if (sc->sc_aclrd_xfer == NULL) {
		error = ENOMEM;
		goto bad5;
	}
	sc->sc_aclwr_xfer = usbd_alloc_xfer(sc->sc_udev);
	if (sc->sc_aclwr_xfer == NULL) {
		error = ENOMEM;
		goto bad6;
	}

	/* Buffers */
	sc->sc_ctl_buf = usbd_alloc_buffer(sc->sc_ctl_xfer,
					   BTHCI_COMMAND_MAX_LEN);
	if (sc->sc_ctl_buf == NULL) {
		error = ENOMEM;
		goto bad7;
	}
	sc->sc_aclrd_buf = usbd_alloc_buffer(sc->sc_aclrd_xfer,
					     BTHCI_ACL_DATA_MAX_LEN);
	if (sc->sc_aclrd_buf == NULL) {
		error = ENOMEM;
		goto bad7;
	}
	sc->sc_aclwr_buf = usbd_alloc_buffer(sc->sc_aclwr_xfer,
					     BTHCI_ACL_DATA_MAX_LEN);
	if (sc->sc_aclwr_buf == NULL) {
		error = ENOMEM;
		goto bad7;
	}

	/* Start reading */
	ubt_aclrd_request(sc);

	return 0;

 bad7:
	usbd_free_xfer(sc->sc_aclwr_xfer);
	sc->sc_aclwr_xfer = NULL;
 bad6:
	usbd_free_xfer(sc->sc_aclrd_xfer);
	sc->sc_aclrd_xfer = NULL;
 bad5:
	usbd_free_xfer(sc->sc_ctl_xfer);
	sc->sc_ctl_xfer = NULL;
 bad4:
	usbd_close_pipe(sc->sc_aclwr_pipe);
	sc->sc_aclwr_pipe = NULL;
 bad3:
	usbd_close_pipe(sc->sc_aclrd_pipe);
	sc->sc_aclrd_pipe = NULL;
 bad2:
	usbd_close_pipe(sc->sc_evt_pipe);
	sc->sc_evt_pipe = NULL;
 bad1:
	free(sc->sc_evt_buf, M_USBDEV);
	sc->sc_evt_buf = NULL;
 bad0:
	return error;
}

static int
ubt_close(void *h, int flag, int mode, usb_proc_ptr p)
{
	struct ubt_softc *sc = h;

	DPRINTFN(0, ("%s: sc=%p\n", __func__, sc));

	ubt_abortdealloc(sc);

	return 0;
}

static u_int8_t*
ubt_alloc_control(void *h, size_t len, struct btframe_buffer **buf)
{
	struct ubt_softc *sc = h;

	/*
	 * We should be catching this earlier, but at the moment a
	 * user request can generate oversized allocations.
	 */
	if (len > BTHCI_COMMAND_MAX_LEN)
		return NULL;

	*buf = (struct btframe_buffer*)sc->sc_ctl_buf;
	return sc->sc_ctl_buf;
}

static int
ubt_send_control(void *h, struct btframe_buffer *buf, size_t len)
{
	struct ubt_softc *sc = h;
	usb_device_request_t req;
	usbd_status status;

	DPRINTFN(1,("%s: sc=%p\n", __func__, sc));

#ifdef DIAGNOSTIC
	if ((u_int8_t*)buf != sc->sc_ctl_buf)
		panic("ubt_control() called with wrong buffer");
#endif

	if (sc->sc_dying)
		return EIO;

	if (len < BTHCI_COMMAND_MIN_LEN || len > BTHCI_COMMAND_MAX_LEN)
		return EINVAL;

	sc->sc_refcnt++;

	memset(&req, 0, sizeof(req));
	req.bmRequestType = UT_WRITE_CLASS_DEVICE;
	USETW(req.wLength, len);

	usbd_setup_default_xfer(sc->sc_ctl_xfer,
				sc->sc_udev,
				sc,
				USBD_DEFAULT_TIMEOUT,
				&req, sc->sc_ctl_buf, len,
				USBD_SYNCHRONOUS | USBD_NO_COPY, NULL);

	status = usbd_transfer(sc->sc_ctl_xfer);

	if (--sc->sc_refcnt < 0)
		usb_detach_wakeup(USBDEV(sc->sc_dev));

	if (status != USBD_NORMAL_COMPLETION)
		return EIO;

	return 0;
}

static u_int8_t*
ubt_alloc_acldata(void *h, size_t len, struct btframe_buffer **buf)
{
	struct ubt_softc *sc = h;

	/*
	 * We should be catching this earlier, but at the moment a
	 * user request can generate oversized allocations.
	 */
	if (len > BTHCI_ACL_DATA_MAX_LEN)
		return NULL;

	*buf = (struct btframe_buffer*)sc->sc_aclwr_buf;
	return sc->sc_aclwr_buf;
}

static int
ubt_send_acldata(void *h, struct btframe_buffer *buf, size_t len)
{
	struct ubt_softc *sc = h;
	usbd_status status;

	DPRINTFN(1,("%s: sc=%p\n", __func__, sc));

#ifdef DIAGNOSTIC
	if ((u_int8_t*)buf != sc->sc_aclwr_buf)
		panic("ubt_sendacldata() called with wrong buffer");
#endif

	if (sc->sc_dying)
		return EIO;

	if (len < BTHCI_ACL_DATA_MIN_LEN || len > BTHCI_ACL_DATA_MAX_LEN)
		return EINVAL;

	sc->sc_refcnt++;

	usbd_setup_xfer(sc->sc_aclwr_xfer,
			sc->sc_aclwr_pipe,
			(usbd_private_handle)sc,
			sc->sc_aclwr_buf, len,
			USBD_SYNCHRONOUS | USBD_NO_COPY,
 			USBD_DEFAULT_TIMEOUT,
			NULL);

	status = usbd_transfer(sc->sc_aclwr_xfer);

	if (--sc->sc_refcnt < 0)
		usb_detach_wakeup(USBDEV(sc->sc_dev));

	if (status != USBD_NORMAL_COMPLETION)
		return EIO;

	return 0;
}

static u_int8_t*
ubt_alloc_scodata(void *h, size_t len, struct btframe_buffer **buf)
{
	return NULL;
}

static int
ubt_send_scodata(void *h, struct btframe_buffer *buf, size_t len)
{
	struct ubt_softc *sc = h;

	DPRINTFN(1,("%s: sc=%p\n", __func__, sc));

	if (sc->sc_dying)
		return EIO;

	return ENXIO;
}

static void
ubt_event_cb(usbd_xfer_handle xfer, usbd_private_handle h, usbd_status status)
{
	struct ubt_softc *sc = h;
	void *buf;
	u_int32_t size;

	DPRINTFN(1,("%s: sc=%p status=%s\n", __func__, sc,
		    usbd_errstr(status)));

	if (status != USBD_NORMAL_COMPLETION || sc->sc_dying ||
	    sc->sc_child == NULL)
		return;

	usbd_get_xfer_status(xfer, NULL, &buf, &size, NULL);

	sc->sc_cb->bt_recveventdata(sc->sc_child, buf, (size_t)size);
}

static void
ubt_aclrd_request(struct ubt_softc *sc)
{
	usbd_status status;
	int s;

	DPRINTFN(1,("%s: sc=%p\n", __func__, sc));

	if (sc->sc_dying)
		return;

	s = splusb();
	if (sc->sc_aclrd_running) {
		splx(s);
		return;
	}
	sc->sc_aclrd_running = 1;
	splx(s);

	usbd_setup_xfer(sc->sc_aclrd_xfer, sc->sc_aclrd_pipe,
			sc, sc->sc_aclrd_buf, BTHCI_ACL_DATA_MAX_LEN,
			USBD_SHORT_XFER_OK | USBD_NO_COPY,
			USBD_NO_TIMEOUT, ubt_aclrd_cb);

	status = usbd_transfer(sc->sc_aclrd_xfer);
	if (status == USBD_IN_PROGRESS || USBD_CANCELLED)
		return;

	DPRINTFN(1,("%s: read request failed: %s\n", __func__,
		    usbd_errstr(status)));

	sc->sc_aclrd_running = 0;
	sc->sc_blocked |= BT_CBBLOCK_ACL_DATA;
}

static void
ubt_aclrd_cb(usbd_xfer_handle xfer, usbd_private_handle h, usbd_status status)
{
	struct ubt_softc *sc = h;
	void *buf;
	u_int32_t size;

	DPRINTFN(1,("%s: sc=%p status=%s\n", __func__, sc,
		    usbd_errstr(status)));

	sc->sc_aclrd_running = 0;

	if (status != USBD_NORMAL_COMPLETION || sc->sc_dying ||
	    sc->sc_child == NULL)
		return;

	usbd_get_xfer_status(xfer, NULL, &buf, &size, NULL);

	sc->sc_cb->bt_recvacldata(sc->sc_child, buf, (size_t)size);

	/* Re-issue the request if not blocked */
	if (!sc->sc_dying && !(sc->sc_blocked & BT_CBBLOCK_ACL_DATA))
		ubt_aclrd_request(sc);
}

static unsigned int
ubt_blockcb(void *h, unsigned int cbblocks)
{
	struct ubt_softc *sc = h;

	sc->sc_blocked |= (cbblocks & BT_CBBLOCK_ACL_DATA);

	return sc->sc_blocked;
}

static unsigned int
ubt_unblockcb(void *h, unsigned int cbblocks)
{
	struct ubt_softc *sc = h;
	unsigned int oblocks, changes;

	oblocks = sc->sc_blocked;
	sc->sc_blocked = oblocks & ~cbblocks;

	changes = oblocks & cbblocks;

	if (changes & BT_CBBLOCK_ACL_DATA)
		/* Re-issue the request if action un-blocked reads */
		ubt_aclrd_request(sc);

	return sc->sc_blocked;
}

static int
ubt_splraise(void)
{
	return splusb();
}
