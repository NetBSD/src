/*	$NetBSD: uhub.c,v 1.136.2.2 2018/09/27 14:52:26 martin Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/uhub.c,v 1.18 1999/11/17 22:33:43 n_hibma Exp $	*/
/*	$OpenBSD: uhub.c,v 1.86 2015/06/29 18:27:40 mpi Exp $ */

/*
 * Copyright (c) 1998, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
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
 * USB spec: http://www.usb.org/developers/docs/usbspec.zip
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uhub.c,v 1.136.2.2 2018/09/27 14:52:26 martin Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/systm.h>


#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbhist.h>

#ifdef USB_DEBUG
#ifndef UHUB_DEBUG
#define uhubdebug 0
#else
static int uhubdebug = 0;

SYSCTL_SETUP(sysctl_hw_uhub_setup, "sysctl hw.uhub setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "uhub",
	    SYSCTL_DESCR("uhub global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &uhubdebug, sizeof(uhubdebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* UHUB_DEBUG */
#endif /* USB_DEBUG */

#define DPRINTF(FMT,A,B,C,D)	USBHIST_LOGN(uhubdebug,1,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(uhubdebug,N,FMT,A,B,C,D)
#define UHUBHIST_FUNC() USBHIST_FUNC()
#define UHUBHIST_CALLED(name) USBHIST_CALLED(uhubdebug)

struct uhub_softc {
	device_t		 sc_dev;	/* base device */
	struct usbd_device	*sc_hub;	/* USB device */
	int			 sc_proto;	/* device protocol */
	struct usbd_pipe	*sc_ipipe;	/* interrupt pipe */

	kmutex_t		 sc_lock;

	uint8_t			*sc_statusbuf;
	uint8_t			*sc_statuspend;
	uint8_t			*sc_status;
	size_t			 sc_statuslen;
	bool			 sc_explorepending;
	bool			 sc_first_explore;
	bool			 sc_running;
};

#define UHUB_IS_HIGH_SPEED(sc) \
    ((sc)->sc_proto == UDPROTO_HSHUBSTT || (sc)->sc_proto == UDPROTO_HSHUBMTT)
#define UHUB_IS_SINGLE_TT(sc) ((sc)->sc_proto == UDPROTO_HSHUBSTT)

#define PORTSTAT_ISSET(sc, port) \
	((sc)->sc_status[(port) / 8] & (1 << ((port) % 8)))

Static usbd_status uhub_explore(struct usbd_device *);
Static void uhub_intr(struct usbd_xfer *, void *, usbd_status);


/*
 * We need two attachment points:
 * hub to usb and hub to hub
 * Every other driver only connects to hubs
 */

int uhub_match(device_t, cfdata_t, void *);
void uhub_attach(device_t, device_t, void *);
int uhub_rescan(device_t, const char *, const int *);
void uhub_childdet(device_t, device_t);
int uhub_detach(device_t, int);
extern struct cfdriver uhub_cd;
CFATTACH_DECL3_NEW(uhub, sizeof(struct uhub_softc), uhub_match,
    uhub_attach, uhub_detach, NULL, uhub_rescan, uhub_childdet,
    DVF_DETACH_SHUTDOWN);
CFATTACH_DECL2_NEW(uroothub, sizeof(struct uhub_softc), uhub_match,
    uhub_attach, uhub_detach, NULL, uhub_rescan, uhub_childdet);

/*
 * Setting this to 1 makes sure than an uhub attaches even at higher
 * priority than ugen when ugen_override is set to 1.  This allows to
 * probe the whole USB bus and attach functions with ugen.
 */
int uhub_ubermatch = 0;

static usbd_status
usbd_get_hub_desc(struct usbd_device *dev, usb_hub_descriptor_t *hd, int speed)
{
	usb_device_request_t req;
	usbd_status err;
	int nports;

	UHUBHIST_FUNC(); UHUBHIST_CALLED();

	/* don't issue UDESC_HUB to SS hub, or it would stall */
	if (dev->ud_depth != 0 && USB_IS_SS(dev->ud_speed)) {
		usb_hub_ss_descriptor_t hssd;
		int rmvlen;

		memset(&hssd, 0, sizeof(hssd));
		req.bmRequestType = UT_READ_CLASS_DEVICE;
		req.bRequest = UR_GET_DESCRIPTOR;
		USETW2(req.wValue, UDESC_SS_HUB, 0);
		USETW(req.wIndex, 0);
		USETW(req.wLength, USB_HUB_SS_DESCRIPTOR_SIZE);
		DPRINTFN(1, "getting sshub descriptor", 0, 0, 0, 0);
		err = usbd_do_request(dev, &req, &hssd);
		nports = hssd.bNbrPorts;
		if (dev->ud_depth != 0 && nports > UHD_SS_NPORTS_MAX) {
			DPRINTF("num of ports %jd exceeds maxports %jd",
			    nports, UHD_SS_NPORTS_MAX, 0, 0);
			nports = hd->bNbrPorts = UHD_SS_NPORTS_MAX;
		}
		rmvlen = (nports + 7) / 8;
		hd->bDescLength = USB_HUB_DESCRIPTOR_SIZE +
		    (rmvlen > 1 ? rmvlen : 1) - 1;
		memcpy(hd->DeviceRemovable, hssd.DeviceRemovable, rmvlen);
		hd->bDescriptorType		= hssd.bDescriptorType;
		hd->bNbrPorts			= hssd.bNbrPorts;
		hd->wHubCharacteristics[0]	= hssd.wHubCharacteristics[0];
		hd->wHubCharacteristics[1]	= hssd.wHubCharacteristics[1];
		hd->bPwrOn2PwrGood		= hssd.bPwrOn2PwrGood;
		hd->bHubContrCurrent		= hssd.bHubContrCurrent;
	} else {
		req.bmRequestType = UT_READ_CLASS_DEVICE;
		req.bRequest = UR_GET_DESCRIPTOR;
		USETW2(req.wValue, UDESC_HUB, 0);
		USETW(req.wIndex, 0);
		USETW(req.wLength, USB_HUB_DESCRIPTOR_SIZE);
		DPRINTFN(1, "getting hub descriptor", 0, 0, 0, 0);
		err = usbd_do_request(dev, &req, hd);
		nports = hd->bNbrPorts;
		if (!err && nports > 7) {
			USETW(req.wLength,
			    USB_HUB_DESCRIPTOR_SIZE + (nports+1) / 8);
			err = usbd_do_request(dev, &req, hd);
		}
	}

	return err;
}

static usbd_status
usbd_set_hub_depth(struct usbd_device *dev, int depth)
{
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_CLASS_DEVICE;
	req.bRequest = UR_SET_HUB_DEPTH;
	USETW(req.wValue, depth);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);
	return usbd_do_request(dev, &req, 0);
}

int
uhub_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;
	int matchvalue;

	UHUBHIST_FUNC(); UHUBHIST_CALLED();

	if (uhub_ubermatch)
		matchvalue = UMATCH_HIGHEST+1;
	else
		matchvalue = UMATCH_DEVCLASS_DEVSUBCLASS;

	DPRINTFN(5, "uaa=%#jx", (uintptr_t)uaa, 0, 0, 0);
	/*
	 * The subclass for hubs seems to be 0 for some and 1 for others,
	 * so we just ignore the subclass.
	 */
	if (uaa->uaa_class == UDCLASS_HUB)
		return matchvalue;
	return UMATCH_NONE;
}

void
uhub_attach(device_t parent, device_t self, void *aux)
{
	struct uhub_softc *sc = device_private(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	char *devinfop;
	usbd_status err;
	struct usbd_hub *hub = NULL;
	usb_hub_descriptor_t hubdesc;
	int p, port, nports, nremov, pwrdly;
	struct usbd_interface *iface;
	usb_endpoint_descriptor_t *ed;
	struct usbd_tt *tts = NULL;

	config_pending_incr(self);

	UHUBHIST_FUNC(); UHUBHIST_CALLED();

	sc->sc_dev = self;
	sc->sc_hub = dev;
	sc->sc_proto = uaa->uaa_proto;

	devinfop = usbd_devinfo_alloc(dev, 1);
	aprint_naive("\n");
	aprint_normal(": %s\n", devinfop);
	usbd_devinfo_free(devinfop);

	if (dev->ud_depth > 0 && UHUB_IS_HIGH_SPEED(sc)) {
		aprint_normal_dev(self, "%s transaction translator%s\n",
		       UHUB_IS_SINGLE_TT(sc) ? "single" : "multiple",
		       UHUB_IS_SINGLE_TT(sc) ? "" : "s");
	}

	err = usbd_set_config_index(dev, 0, 1);
	if (err) {
		DPRINTF("configuration failed, sc %#jx error %jd",
		    (uintptr_t)sc, err, 0, 0);
		goto bad2;
	}

	if (dev->ud_depth > USB_HUB_MAX_DEPTH) {
		aprint_error_dev(self,
		    "hub depth (%d) exceeded, hub ignored\n",
		    USB_HUB_MAX_DEPTH);
		goto bad2;
	}

	/* Get hub descriptor. */
	memset(&hubdesc, 0, sizeof(hubdesc));
	err = usbd_get_hub_desc(dev, &hubdesc, dev->ud_speed);
	nports = hubdesc.bNbrPorts;
	if (err) {
		DPRINTF("getting hub descriptor failed, uhub%jd error %jd",
		    device_unit(self), err, 0, 0);
		goto bad2;
	}

	for (nremov = 0, port = 1; port <= nports; port++)
		if (!UHD_NOT_REMOV(&hubdesc, port))
			nremov++;
	aprint_verbose_dev(self, "%d port%s with %d removable, %s powered\n",
	    nports, nports != 1 ? "s" : "", nremov,
	    dev->ud_selfpowered ? "self" : "bus");

	if (nports == 0) {
		aprint_debug_dev(self, "no ports, hub ignored\n");
		goto bad;
	}

	hub = kmem_alloc(sizeof(*hub) + (nports-1) * sizeof(struct usbd_port),
	    KM_SLEEP);
	dev->ud_hub = hub;
	dev->ud_hub->uh_hubsoftc = sc;
	hub->uh_explore = uhub_explore;
	hub->uh_hubdesc = hubdesc;

	if (USB_IS_SS(dev->ud_speed) && dev->ud_depth != 0) {
		aprint_debug_dev(self, "setting hub depth %u\n",
		    dev->ud_depth - 1);
		err = usbd_set_hub_depth(dev, dev->ud_depth - 1);
		if (err) {
			aprint_error_dev(self, "can't set depth\n");
			goto bad;
		}
	}

	/* Set up interrupt pipe. */
	err = usbd_device2interface_handle(dev, 0, &iface);
	if (err) {
		aprint_error_dev(self, "no interface handle\n");
		goto bad;
	}

	if (UHUB_IS_HIGH_SPEED(sc) && !UHUB_IS_SINGLE_TT(sc)) {
		err = usbd_set_interface(iface, 1);
		if (err)
			aprint_error_dev(self, "can't enable multiple TTs\n");
	}

	ed = usbd_interface2endpoint_descriptor(iface, 0);
	if (ed == NULL) {
		aprint_error_dev(self, "no endpoint descriptor\n");
		goto bad;
	}
	if ((ed->bmAttributes & UE_XFERTYPE) != UE_INTERRUPT) {
		aprint_error_dev(self, "bad interrupt endpoint\n");
		goto bad;
	}

	sc->sc_statuslen = (nports + 1 + 7) / 8;
	sc->sc_statusbuf = kmem_alloc(sc->sc_statuslen, KM_SLEEP);
	sc->sc_statuspend = kmem_zalloc(sc->sc_statuslen, KM_SLEEP);
	sc->sc_status = kmem_alloc(sc->sc_statuslen, KM_SLEEP);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	memset(sc->sc_statuspend, 0, sc->sc_statuslen);

	/* force initial scan */
	memset(sc->sc_status, 0xff, sc->sc_statuslen);
	sc->sc_explorepending = true;

	err = usbd_open_pipe_intr(iface, ed->bEndpointAddress,
		  USBD_SHORT_XFER_OK|USBD_MPSAFE, &sc->sc_ipipe, sc,
		  sc->sc_statusbuf, sc->sc_statuslen,
		  uhub_intr, USBD_DEFAULT_INTERVAL);
	if (err) {
		aprint_error_dev(self, "cannot open interrupt pipe\n");
		goto bad;
	}

	/* Wait with power off for a while. */
	usbd_delay_ms(dev, USB_POWER_DOWN_TIME);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, dev, sc->sc_dev);

	/*
	 * To have the best chance of success we do things in the exact same
	 * order as Windows 98.  This should not be necessary, but some
	 * devices do not follow the USB specs to the letter.
	 *
	 * These are the events on the bus when a hub is attached:
	 *  Get device and config descriptors (see attach code)
	 *  Get hub descriptor (see above)
	 *  For all ports
	 *     turn on power
	 *     wait for power to become stable
	 * (all below happens in explore code)
	 *  For all ports
	 *     clear C_PORT_CONNECTION
	 *  For all ports
	 *     get port status
	 *     if device connected
	 *        wait 100 ms
	 *        turn on reset
	 *        wait
	 *        clear C_PORT_RESET
	 *        get port status
	 *        proceed with device attachment
	 */

	if (UHUB_IS_HIGH_SPEED(sc) && nports > 0) {
		tts = kmem_alloc((UHUB_IS_SINGLE_TT(sc) ? 1 : nports) *
			     sizeof(struct usbd_tt), KM_SLEEP);
	}
	/* Set up data structures */
	for (p = 0; p < nports; p++) {
		struct usbd_port *up = &hub->uh_ports[p];
		up->up_dev = NULL;
		up->up_parent = dev;
		up->up_portno = p + 1;
		if (dev->ud_selfpowered)
			/* Self powered hub, give ports maximum current. */
			up->up_power = USB_MAX_POWER;
		else
			up->up_power = USB_MIN_POWER;
		up->up_restartcnt = 0;
		up->up_reattach = 0;
		if (UHUB_IS_HIGH_SPEED(sc)) {
			up->up_tt = &tts[UHUB_IS_SINGLE_TT(sc) ? 0 : p];
			up->up_tt->utt_hub = hub;
		} else {
			up->up_tt = NULL;
		}
	}

	/* XXX should check for none, individual, or ganged power? */

	pwrdly = dev->ud_hub->uh_hubdesc.bPwrOn2PwrGood * UHD_PWRON_FACTOR
	    + USB_EXTRA_POWER_UP_TIME;
	for (port = 1; port <= nports; port++) {
		/* Turn the power on. */
		err = usbd_set_port_feature(dev, port, UHF_PORT_POWER);
		if (err)
			aprint_error_dev(self, "port %d power on failed, %s\n",
			    port, usbd_errstr(err));
		DPRINTF("uhub%jd turn on port %jd power", device_unit(self),
		    port, 0, 0);
	}

	/* Wait for stable power if we are not a root hub */
	if (dev->ud_powersrc->up_parent != NULL)
		usbd_delay_ms(dev, pwrdly);

	/* The usual exploration will finish the setup. */
	sc->sc_running = true;
	sc->sc_first_explore = true;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;

 bad:
	if (sc->sc_status)
		kmem_free(sc->sc_status, sc->sc_statuslen);
	if (sc->sc_statuspend)
		kmem_free(sc->sc_statuspend, sc->sc_statuslen);
	if (sc->sc_statusbuf)
		kmem_free(sc->sc_statusbuf, sc->sc_statuslen);
	if (hub)
		kmem_free(hub,
		    sizeof(*hub) + (nports-1) * sizeof(struct usbd_port));
	dev->ud_hub = NULL;
 bad2:
	config_pending_decr(self);
}

usbd_status
uhub_explore(struct usbd_device *dev)
{
	usb_hub_descriptor_t *hd = &dev->ud_hub->uh_hubdesc;
	struct uhub_softc *sc = dev->ud_hub->uh_hubsoftc;
	struct usbd_port *up;
	usbd_status err;
	int speed;
	int port;
	int change, status, reconnect;

	UHUBHIST_FUNC(); UHUBHIST_CALLED();

	DPRINTFN(10, "uhub%jd dev=%#jx addr=%jd speed=%ju",
	    device_unit(sc->sc_dev), (uintptr_t)dev, dev->ud_addr,
	    dev->ud_speed);

	if (!sc->sc_running)
		return USBD_NOT_STARTED;

	/* Ignore hubs that are too deep. */
	if (dev->ud_depth > USB_HUB_MAX_DEPTH)
		return USBD_TOO_DEEP;

	if (PORTSTAT_ISSET(sc, 0)) { /* hub status change */
		usb_hub_status_t hs;

		err = usbd_get_hub_status(dev, &hs);
		if (err) {
			DPRINTF("uhub%jd get hub status failed, err %jd",
			    device_unit(sc->sc_dev), err, 0, 0);
		} else {
			/* just acknowledge */
			status = UGETW(hs.wHubStatus);
			change = UGETW(hs.wHubChange);
			DPRINTF("uhub%jd s/c=%jx/%jx", device_unit(sc->sc_dev),
			    status, change, 0);

			if (change & UHS_LOCAL_POWER)
				usbd_clear_hub_feature(dev,
						       UHF_C_HUB_LOCAL_POWER);
			if (change & UHS_OVER_CURRENT)
				usbd_clear_hub_feature(dev,
						       UHF_C_HUB_OVER_CURRENT);
		}
	}

	for (port = 1; port <= hd->bNbrPorts; port++) {
		up = &dev->ud_hub->uh_ports[port - 1];

		/* reattach is needed after firmware upload */
		reconnect = up->up_reattach;
		up->up_reattach = 0;

		status = change = 0;

		/* don't check if no change summary notification */
		if (PORTSTAT_ISSET(sc, port) || reconnect) {
			err = usbd_get_port_status(dev, port, &up->up_status);
			if (err) {
				DPRINTF("uhub%jd get port stat failed, err %jd",
				    device_unit(sc->sc_dev), err, 0, 0);
				continue;
			}
			status = UGETW(up->up_status.wPortStatus);
			change = UGETW(up->up_status.wPortChange);

			DPRINTF("uhub%jd port %jd: s/c=%jx/%jx",
			    device_unit(sc->sc_dev), port, status, change);
		}
		if (!change && !reconnect) {
			/* No status change, just do recursive explore. */
			if (up->up_dev != NULL && up->up_dev->ud_hub != NULL)
				up->up_dev->ud_hub->uh_explore(up->up_dev);
			continue;
		}

		if (change & UPS_C_PORT_ENABLED) {
			DPRINTF("uhub%jd port %jd C_PORT_ENABLED",
			    device_unit(sc->sc_dev), port, 0, 0);
			usbd_clear_port_feature(dev, port, UHF_C_PORT_ENABLE);
			if (change & UPS_C_CONNECT_STATUS) {
				/* Ignore the port error if the device
				   vanished. */
			} else if (status & UPS_PORT_ENABLED) {
				aprint_error_dev(sc->sc_dev,
				    "illegal enable change, port %d\n", port);
			} else {
				/* Port error condition. */
				if (up->up_restartcnt) /* no message first time */
					aprint_error_dev(sc->sc_dev,
					    "port error, restarting port %d\n",
					    port);

				if (up->up_restartcnt++ < USBD_RESTART_MAX)
					goto disco;
				else
					aprint_error_dev(sc->sc_dev,
					    "port error, giving up port %d\n",
					    port);
			}
		}
		if (change & UPS_C_PORT_RESET) {
			/*
			 * some xHCs set PortResetChange instead of CSC
			 * when port is reset.
			 */
			if ((status & UPS_CURRENT_CONNECT_STATUS) != 0) {
				change |= UPS_C_CONNECT_STATUS;
			}
			usbd_clear_port_feature(dev, port, UHF_C_PORT_RESET);
		}
		if (change & UPS_C_BH_PORT_RESET) {
			/*
			 * some xHCs set WarmResetChange instead of CSC
			 * when port is reset.
			 */
			if ((status & UPS_CURRENT_CONNECT_STATUS) != 0) {
				change |= UPS_C_CONNECT_STATUS;
			}
			usbd_clear_port_feature(dev, port,
			    UHF_C_BH_PORT_RESET);
		}
		if (change & UPS_C_PORT_LINK_STATE)
			usbd_clear_port_feature(dev, port,
			    UHF_C_PORT_LINK_STATE);
		if (change & UPS_C_PORT_CONFIG_ERROR)
			usbd_clear_port_feature(dev, port,
			    UHF_C_PORT_CONFIG_ERROR);

		/* XXX handle overcurrent and resume events! */

		if (!reconnect && !(change & UPS_C_CONNECT_STATUS)) {
			/* No status change, just do recursive explore. */
			if (up->up_dev != NULL && up->up_dev->ud_hub != NULL)
				up->up_dev->ud_hub->uh_explore(up->up_dev);
			continue;
		}

		/* We have a connect status change, handle it. */

		DPRINTF("uhub%jd status change port %jd",
		    device_unit(sc->sc_dev), port, 0, 0);
		usbd_clear_port_feature(dev, port, UHF_C_PORT_CONNECTION);
		/*
		 * If there is already a device on the port the change status
		 * must mean that is has disconnected.  Looking at the
		 * current connect status is not enough to figure this out
		 * since a new unit may have been connected before we handle
		 * the disconnect.
		 */
	disco:
		if (up->up_dev != NULL) {
			/* Disconnected */
			DPRINTF("uhub%jd device addr=%jd disappeared on "
			    "port %jd",
			    device_unit(sc->sc_dev), up->up_dev->ud_addr, port,
			    0);

			usb_disconnect_port(up, sc->sc_dev, DETACH_FORCE);
			usbd_clear_port_feature(dev, port,
						UHF_C_PORT_CONNECTION);
		}
		if (!(status & UPS_CURRENT_CONNECT_STATUS)) {
			/* Nothing connected, just ignore it. */
			DPRINTFN(3, "uhub%jd port %jd !CURRENT_CONNECT_STATUS",
			    device_unit(sc->sc_dev), port, 0, 0);
			usb_disconnect_port(up, sc->sc_dev, DETACH_FORCE);
			usbd_clear_port_feature(dev, port,
						UHF_C_PORT_CONNECTION);
			continue;
		}

		/* Connected */
		DPRINTF("unit %jd dev->speed=%ju dev->depth=%ju",
		    device_unit(sc->sc_dev), dev->ud_speed, dev->ud_depth, 0);

		/* Wait for maximum device power up time. */
		usbd_delay_ms(dev, USB_PORT_POWERUP_DELAY);

		/* Reset port, which implies enabling it. */
		if (usbd_reset_port(dev, port, &up->up_status)) {
			aprint_error_dev(sc->sc_dev,
			    "port %d reset failed\n", port);
			continue;
		}
#if 0
		/* Get port status again, it might have changed during reset */
		err = usbd_get_port_status(dev, port, &up->up_status);
		if (err) {
			DPRINTF("uhub%jd port %jd get port status failed, "
			    "err %jd", device_unit(sc->sc_dev), port, err, 0);
			continue;
		}
#endif
		/*
		 * Use the port status from the reset to check for the device
		 * disappearing, the port enable status, and the port speed
		 */
		status = UGETW(up->up_status.wPortStatus);
		change = UGETW(up->up_status.wPortChange);
		DPRINTF("uhub%jd port %jd after reset: s/c=%jx/%jx",
		    device_unit(sc->sc_dev), port, status, change);

		if (!(status & UPS_CURRENT_CONNECT_STATUS)) {
			/* Nothing connected, just ignore it. */
#ifdef DIAGNOSTIC
			aprint_debug_dev(sc->sc_dev,
			    "port %d, device disappeared after reset\n", port);
#endif
			continue;
		}
		if (!(status & UPS_PORT_ENABLED)) {
			/* Not allowed send/receive packet. */
#ifdef DIAGNOSTIC
			printf("%s: port %d, device not enabled\n",
			       device_xname(sc->sc_dev), port);
#endif
			continue;
		}
		/* port reset may cause Warm Reset Change, drop it. */
		if (change & UPS_C_BH_PORT_RESET)
			usbd_clear_port_feature(dev, port,
			    UHF_C_BH_PORT_RESET);

		/*
		 * Figure out device speed from power bit of port status.
		 *  USB 2.0 ch 11.24.2.7.1
		 *  USB 3.1 ch 10.16.2.6.1
		 */
		int sts = status;
		if ((sts & UPS_PORT_POWER) == 0)
			sts &= ~UPS_PORT_POWER_SS;

		if (sts & UPS_HIGH_SPEED)
			speed = USB_SPEED_HIGH;
		else if (sts & UPS_LOW_SPEED)
			speed = USB_SPEED_LOW;
		else {
			/*
			 * If there is no power bit set, it is certainly
			 * a Super Speed device, so use the speed of its
			 * parent hub.
			 */
			if (sts & UPS_PORT_POWER)
				speed = USB_SPEED_FULL;
			else
				speed = dev->ud_speed;
		}

		/*
		 * Reduce the speed, otherwise we won't setup the proper
		 * transfer methods.
		 */
		if (speed > dev->ud_speed)
			speed = dev->ud_speed;

		DPRINTF("uhub%jd speed %ju", device_unit(sc->sc_dev), speed, 0,
		    0);

		/*
		 * To check whether port has power,
		 *  check UPS_PORT_POWER_SS bit if port speed is SS, and
		 *  check UPS_PORT_POWER bit if port speed is HS/FS/LS.
		 */
		if (USB_IS_SS(speed)) {
			/* SS hub port */
			if (!(status & UPS_PORT_POWER_SS))
				aprint_normal_dev(sc->sc_dev,
				    "strange, connected port %d has no power\n",
				    port);
		} else {
			/* HS/FS/LS hub port */
			if (!(status & UPS_PORT_POWER))
				aprint_normal_dev(sc->sc_dev,
				    "strange, connected port %d has no power\n",
				    port);
		}

		/* Get device info and set its address. */
		err = usbd_new_device(sc->sc_dev, dev->ud_bus,
			  dev->ud_depth + 1, speed, port, up);
		/* XXX retry a few times? */
		if (err) {
			DPRINTF("usbd_new_device failed, error %jd", err, 0, 0,
			    0);
			/* Avoid addressing problems by disabling. */
			/* usbd_reset_port(dev, port, &up->status); */

			/*
			 * The unit refused to accept a new address, or had
			 * some other serious problem.  Since we cannot leave
			 * at 0 we have to disable the port instead.
			 */
			aprint_error_dev(sc->sc_dev,
			    "device problem, disabling port %d\n", port);
			usbd_clear_port_feature(dev, port, UHF_PORT_ENABLE);
		} else {
			/* The port set up succeeded, reset error count. */
			up->up_restartcnt = 0;

			if (up->up_dev->ud_hub)
				up->up_dev->ud_hub->uh_explore(up->up_dev);
		}
	}
	mutex_enter(&sc->sc_lock);
	sc->sc_explorepending = false;
	for (int i = 0; i < sc->sc_statuslen; i++) {
		if (sc->sc_statuspend[i] != 0) {
			memcpy(sc->sc_status, sc->sc_statuspend,
			    sc->sc_statuslen);
			memset(sc->sc_statuspend, 0, sc->sc_statuslen);
			usb_needs_explore(sc->sc_hub);
			break;
		}
	}
	mutex_exit(&sc->sc_lock);
	if (sc->sc_first_explore) {
		config_pending_decr(sc->sc_dev);
		sc->sc_first_explore = false;
	}

	return USBD_NORMAL_COMPLETION;
}

/*
 * Called from process context when the hub is gone.
 * Detach all devices on active ports.
 */
int
uhub_detach(device_t self, int flags)
{
	struct uhub_softc *sc = device_private(self);
	struct usbd_hub *hub = sc->sc_hub->ud_hub;
	struct usbd_port *rup;
	int nports, port, rc;

	UHUBHIST_FUNC(); UHUBHIST_CALLED();

	DPRINTF("uhub%jd flags=%jd", device_unit(self), flags, 0, 0);

	if (hub == NULL)		/* Must be partially working */
		return 0;

	/* XXXSMP usb */
	KERNEL_LOCK(1, curlwp);

	nports = hub->uh_hubdesc.bNbrPorts;
	for (port = 0; port < nports; port++) {
		rup = &hub->uh_ports[port];
		if (rup->up_dev == NULL)
			continue;
		if ((rc = usb_disconnect_port(rup, self, flags)) != 0) {
			/* XXXSMP usb */
			KERNEL_UNLOCK_ONE(curlwp);

			return rc;
		}
	}

	pmf_device_deregister(self);
	usbd_abort_pipe(sc->sc_ipipe);
	usbd_close_pipe(sc->sc_ipipe);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_hub, sc->sc_dev);

	if (hub->uh_ports[0].up_tt)
		kmem_free(hub->uh_ports[0].up_tt,
		    (UHUB_IS_SINGLE_TT(sc) ? 1 : nports) *
		    sizeof(struct usbd_tt));
	kmem_free(hub,
	    sizeof(*hub) + (nports-1) * sizeof(struct usbd_port));
	sc->sc_hub->ud_hub = NULL;
	if (sc->sc_status)
		kmem_free(sc->sc_status, sc->sc_statuslen);
	if (sc->sc_statuspend)
		kmem_free(sc->sc_statuspend, sc->sc_statuslen);
	if (sc->sc_statusbuf)
		kmem_free(sc->sc_statusbuf, sc->sc_statuslen);

	mutex_destroy(&sc->sc_lock);

	/* XXXSMP usb */
	KERNEL_UNLOCK_ONE(curlwp);

	return 0;
}

int
uhub_rescan(device_t self, const char *ifattr, const int *locators)
{
	struct uhub_softc *sc = device_private(self);
	struct usbd_hub *hub = sc->sc_hub->ud_hub;
	struct usbd_device *dev;
	int port;

	for (port = 0; port < hub->uh_hubdesc.bNbrPorts; port++) {
		dev = hub->uh_ports[port].up_dev;
		if (dev == NULL)
			continue;
		usbd_reattach_device(sc->sc_dev, dev, port, locators);
	}
	return 0;
}

/* Called when a device has been detached from it */
void
uhub_childdet(device_t self, device_t child)
{
	struct uhub_softc *sc = device_private(self);
	struct usbd_device *devhub = sc->sc_hub;
	struct usbd_device *dev;
	int nports;
	int port;
	int i;

	if (!devhub->ud_hub)
		/* should never happen; children are only created after init */
		panic("hub not fully initialised, but child deleted?");

	nports = devhub->ud_hub->uh_hubdesc.bNbrPorts;
	for (port = 0; port < nports; port++) {
		dev = devhub->ud_hub->uh_ports[port].up_dev;
		if (!dev || dev->ud_subdevlen == 0)
			continue;
		for (i = 0; i < dev->ud_subdevlen; i++) {
			if (dev->ud_subdevs[i] == child) {
				dev->ud_subdevs[i] = NULL;
				dev->ud_nifaces_claimed--;
			}
		}
		if (dev->ud_nifaces_claimed == 0) {
			kmem_free(dev->ud_subdevs,
			    dev->ud_subdevlen * sizeof(device_t));
			dev->ud_subdevs = NULL;
			dev->ud_subdevlen = 0;
		}
	}
}


/*
 * Hub interrupt.
 * This an indication that some port has changed status.
 * Notify the bus event handler thread that we need
 * to be explored again.
 */
void
uhub_intr(struct usbd_xfer *xfer, void *addr, usbd_status status)
{
	struct uhub_softc *sc = addr;

	UHUBHIST_FUNC(); UHUBHIST_CALLED();

	DPRINTFN(5, "uhub%jd", device_unit(sc->sc_dev), 0, 0, 0);

	if (status == USBD_STALLED)
		usbd_clear_endpoint_stall_async(sc->sc_ipipe);
	else if (status == USBD_NORMAL_COMPLETION) {

		mutex_enter(&sc->sc_lock);

		DPRINTFN(5, "uhub%jd: explore pending %jd",
		    device_unit(sc->sc_dev), sc->sc_explorepending, 0, 0);

		/* merge port bitmap into pending interrupts list */
		for (size_t i = 0; i < sc->sc_statuslen; i++) {
			sc->sc_statuspend[i] |= sc->sc_statusbuf[i];

			DPRINTFN(5, "uhub%jd: pending/new ports "
			    "[%jd] %#jx/%#jx", device_unit(sc->sc_dev),
			    i, sc->sc_statuspend[i], sc->sc_statusbuf[i]);
		}

		if (!sc->sc_explorepending) {
			sc->sc_explorepending = true;

			memcpy(sc->sc_status, sc->sc_statuspend,
			    sc->sc_statuslen);
			memset(sc->sc_statuspend, 0, sc->sc_statuslen);

			for (size_t i = 0; i < sc->sc_statuslen; i++) {
				DPRINTFN(5, "uhub%jd: exploring ports "
				    "[%jd] %#jx", device_unit(sc->sc_dev),
				    i, sc->sc_status[i], 0);
			}

			usb_needs_explore(sc->sc_hub);
		}
		mutex_exit(&sc->sc_lock);
	}
}
