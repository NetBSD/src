/*	$NetBSD: uhub.c,v 1.10 1998/12/09 19:24:28 drochner Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@carlstedt.se) at
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


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/device.h>

#include <dev/usb/usb.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>

#ifdef USB_DEBUG
#define DPRINTF(x)	if (usbdebug) printf x
#define DPRINTFN(n,x)	if (usbdebug>(n)) printf x
extern int	usbdebug;
extern char 	*usbd_error_strs[];
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

struct uhub_softc {
	struct device sc_dev;		/* base device */
	usbd_device_handle sc_hub;	/* USB device */
	usbd_pipe_handle sc_ipipe;      /* interrupt pipe */
	u_int8_t sc_status[1];	/* XXX more ports */
	u_char sc_running;
};

int uhub_match __P((struct device *, struct cfdata *, void *));
void uhub_attach __P((struct device *, struct device *, void *));

usbd_status uhub_init_port __P((int, struct usbd_port *, usbd_device_handle));
void uhub_disconnect __P((struct uhub_softc *sc, struct device *parent, 
			  struct usbd_port *up, int portno));
usbd_status uhub_explore __P((struct device *parent, usbd_device_handle hub));
void uhub_intr __P((usbd_request_handle, usbd_private_handle, usbd_status));

/*void uhub_disco __P((void *));*/

extern struct cfdriver uhub_cd;

struct cfattach uhub_ca = {
	sizeof(struct uhub_softc), uhub_match, uhub_attach
};

struct cfattach uhub_uhub_ca = {
	sizeof(struct uhub_softc), uhub_match, uhub_attach
};

int
uhub_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct usb_attach_arg *uaa = aux;
	usb_device_descriptor_t *dd = usbd_get_device_descriptor(uaa->device);
	
	DPRINTFN(1,("uhub_match, dd=%p\n", dd));
	/* 
	 * The subclass for hubs seems to be 0 for some and 1 for others,
	 * so we just ignore the subclass.
	 */
	if (uaa->iface == 0 && dd->bDeviceClass == UCLASS_HUB)
		return (UMATCH_DEVCLASS_DEVSUBCLASS);
	return (UMATCH_NONE);
}

void
uhub_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct uhub_softc *sc = (struct uhub_softc *)self;
	struct usb_attach_arg *uaa = aux;
	usbd_device_handle dev = uaa->device;
	char devinfo[1024];
	usbd_status r;
	struct usbd_hub *hub;
	usb_device_request_t req;
	usb_hub_descriptor_t hubdesc;
	int port, nports;
	usbd_interface_handle iface;
	usb_endpoint_descriptor_t *ed;
	
	DPRINTFN(1,("uhub_attach\n"));
	sc->sc_hub = dev;
	usbd_devinfo(dev, 1, devinfo);
	printf("\n%s: %s\n", sc->sc_dev.dv_xname, devinfo);

	r = usbd_set_config_index(dev, 0, 1);
	if (r != USBD_NORMAL_COMPLETION) {
		DPRINTF(("%s: configuration failed, error=%d(%s)\n",
			 sc->sc_dev.dv_xname, r, usbd_error_strs[r]));
		return;
	}

	if (dev->depth > USB_HUB_MAX_DEPTH) {
		printf("%s: hub depth (%d) exceeded, hub ignored\n",
		       sc->sc_dev.dv_xname, USB_HUB_MAX_DEPTH);
		return;
	}

	/* Get hub descriptor. */
	req.bmRequestType = UT_READ_CLASS_DEVICE;
	req.bRequest = UR_GET_DESCRIPTOR;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, USB_HUB_DESCRIPTOR_SIZE);
	DPRINTFN(1,("usb_init_hub: getting hub descriptor\n"));
	/* XXX not correct for hubs with >7 ports */
	r = usbd_do_request(dev, &req, &hubdesc);
	if (r != USBD_NORMAL_COMPLETION) {
		DPRINTF(("%s: getting hub descriptor failed, error=%d(%s)\n",
			 sc->sc_dev.dv_xname, r, usbd_error_strs[r]));
		return;
	}

	nports = hubdesc.bNbrPorts;
	hub = malloc(sizeof(*hub) + (nports-1) * sizeof(struct usbd_port),
		     M_USB, M_NOWAIT);
	if (hub == 0)
		return;
	dev->hub = hub;
	dev->hub->hubdata = sc;
	hub->explore = uhub_explore;
	hub->hubdesc = hubdesc;
	
	DPRINTFN(1,("usbhub_init_hub: selfpowered=%d, parent=%p, "
		    "parent->selfpowered=%d\n",
		 dev->self_powered, dev->powersrc->parent,
		 dev->powersrc->parent ? 
		 dev->powersrc->parent->self_powered : 0));
	if (!dev->self_powered && dev->powersrc->parent &&
	    !dev->powersrc->parent->self_powered) {
		printf("%s: bus powered hub connected to bus powered hub, "
		       "ignored\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	/* Set up interrupt pipe. */
	r = usbd_device2interface_handle(dev, 0, &iface);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: no interface handle\n", sc->sc_dev.dv_xname);
		return;
	}
	ed = usbd_interface2endpoint_descriptor(iface, 0);
	if (ed == 0) {
		printf("%s: no endpoint descriptor\n", sc->sc_dev.dv_xname);
		return;
	}
	if ((ed->bmAttributes & UE_XFERTYPE) != UE_INTERRUPT) {
		printf("%s: bad interrupt endpoint\n", sc->sc_dev.dv_xname);
		return;
	}

	r = usbd_open_pipe_intr(iface, ed->bEndpointAddress,USBD_SHORT_XFER_OK,
				&sc->sc_ipipe, sc, sc->sc_status, 
				sizeof(sc->sc_status),
				uhub_intr);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: cannot open interrupt pipe\n",sc->sc_dev.dv_xname);
		return;
	}

	for (port = 1; port <= nports; port++) {
		r = uhub_init_port(port, &hub->ports[port-1], dev);
		if (r != USBD_NORMAL_COMPLETION)
			printf("%s: init of port %d failed\n", 
			       sc->sc_dev.dv_xname, port);
	}
	sc->sc_running = 1;
}

usbd_status
uhub_init_port(port, uport, dev)
	int port;
	struct usbd_port *uport;
	usbd_device_handle dev;
{
	usbd_status r;
	u_int16_t pstatus;

	uport->device = 0;
	uport->parent = dev;
	r = usbd_get_port_status(dev, port, &uport->status);
	if (r != USBD_NORMAL_COMPLETION)
		return r;
	pstatus = UGETW(uport->status.wPortStatus);
	DPRINTF(("usbd_init_port: adding hub port=%d status=0x%04x "
		 "change=0x%04x\n",
		 port, pstatus, UGETW(uport->status.wPortChange)));
	if ((pstatus & UPS_PORT_POWER) == 0) {
		/* Port lacks power, turn it on */
#if 0
		/* First let the device go through a good power cycle, */
		usbd_delay_ms(dev->bus, USB_POWER_DOWN_TIME);
#endif
		/* then turn the power on. */
		r = usbd_set_port_feature(dev, port, UHF_PORT_POWER);
		if (r != USBD_NORMAL_COMPLETION)
			return (r);
		r = usbd_get_port_status(dev, port, &uport->status);
		if (r != USBD_NORMAL_COMPLETION)
			return (r);
		DPRINTF(("usb_init_port: turn on port %d power status=0x%04x "
			 "change=0x%04x\n",
			 port, UGETW(uport->status.wPortStatus),
			 UGETW(uport->status.wPortChange)));
		/* Wait for stable power. */
		usbd_delay_ms(dev->bus, dev->hub->hubdesc.bPwrOn2PwrGood * 
			                UHD_PWRON_FACTOR);
	}
	if (dev->self_powered)
		/* Self powered hub, give ports maximum current. */
		uport->power = USB_MAX_POWER;
	else
		uport->power = USB_MIN_POWER;
	return (USBD_NORMAL_COMPLETION);
}

usbd_status
uhub_explore(parent, dev)
	struct device *parent;
	usbd_device_handle dev;
{
	usb_hub_descriptor_t *hd = &dev->hub->hubdesc;
	struct uhub_softc *sc = dev->hub->hubdata;
	struct usbd_port *up;
	usbd_status r;
	int port;
	int change, status;

	DPRINTF(("uhub_explore dev=%p addr=%d\n", dev, dev->address));

	if (!sc->sc_running)
		return (USBD_NOT_STARTED);

	/* Ignore hubs that are too deep. */
	if (dev->depth > USB_HUB_MAX_DEPTH)
		return (USBD_TOO_DEEP);

	for(port = 1; port <= hd->bNbrPorts; port++) {
		up = &dev->hub->ports[port-1];
		r = usbd_get_port_status(dev, port, &up->status);
		if (r != USBD_NORMAL_COMPLETION) {
			DPRINTF(("uhub_explore: get port status failed, "
				 "error=%d(%s)\n",
				 r, usbd_error_strs[r]));
			continue;
		}
		status = UGETW(up->status.wPortStatus);
		change = UGETW(up->status.wPortChange);
		DPRINTFN(5, ("uhub_explore: port %d status 0x%04x 0x%04x\n",
			     port, status, change));
		if (!(change & UPS_CURRENT_CONNECT_STATUS)) {
			/* No status change, just do recursive explore. */
			if (up->device && up->device->hub)
				up->device->hub->explore(parent, up->device);
			continue;
		}
		DPRINTF(("uhub_explore: status change hub=%d port=%d\n",
			 dev->address, port));
		usbd_clear_port_feature(dev, port, UHF_C_PORT_CONNECTION);
		usbd_clear_port_feature(dev, port, UHF_C_PORT_ENABLE);
		/*
		 * If there is already a device on the port the change status
		 * must mean that is has disconnected.  Looking at the
		 * current connect status is not enough to figure this out
		 * since a new unit may have been connected before we handle
		 * the disconnect.
		 */
		if (up->device) {
			/* Disconnected */
			DPRINTF(("uhub_explore: device %d disappeared "
				 "on port %d\n", 
				 up->device->address, port));
			uhub_disconnect(sc, parent, up, port);
			usbd_clear_port_feature(dev, port, 
						UHF_C_PORT_CONNECTION);
		}
		if (!(status & UPS_CURRENT_CONNECT_STATUS))
			continue;

		/* Connected */
		/* Wait for maximum device power up time. */
		usbd_delay_ms(dev->bus, USB_PORT_POWERUP_DELAY);
		/* Reset port, which implies enabling it. */
		if (usbd_reset_port(dev, port, &up->status) != 
		    USBD_NORMAL_COMPLETION)
			continue;

		/* Wait for power to settle in device. */
		usbd_delay_ms(dev->bus, USB_POWER_SETTLE);
			
		/* Get device info and set its address. */
		r = usbd_new_device((struct device *)sc, dev->bus, 
				    dev->depth + 1, status & UPS_LOW_SPEED, 
				    port, up);
		/* XXX retry a few times? */
		if (r != USBD_NORMAL_COMPLETION) {
			DPRINTFN(-1,("uhub_explore: usb_new_device failed, "
				     "error=%d(%s)\n", r, usbd_error_strs[r]));
			/* Avoid addressing problems by disabling. */
			/* usbd_reset_port(dev, port, &up->status); */
/* XXX
 * What should we do.  The device may or may not be at its
 * assigned address.  In any case we'd like to ignore it.
 * Maybe the port should be disabled until the device is
 * disconnected.
 */
			if (r == USBD_SET_ADDR_FAILED || 1) {/* XXX */
				/* The unit refused to accept a new
				 * address, and since we cannot leave
				 * at 0 we have to disable the port
				 * instead. */
				printf("%s: device problem, disabling "
				       "port %d\n",
				       parent->dv_xname, port);
				usbd_clear_port_feature(dev, port, 
							UHF_PORT_ENABLE);
			}
		} else {
			if (up->device->hub)
				up->device->hub->explore(parent, up->device);
		}
	}
	return (USBD_NORMAL_COMPLETION);
}

void
uhub_disconnect(sc, parent, up, portno)
	struct uhub_softc *sc;
	struct device *parent;
	struct usbd_port *up;
	int portno;
{
	usbd_device_handle dev = up->device;
	usbd_pipe_handle p, n;
	usb_hub_descriptor_t *hd;
	struct usbd_port *spi;
	int i, port;

	DPRINTFN(3,("uhub_disconnect: up=%p dev=%p port=%d\n", 
		    up, dev, portno));

	printf("%s: device addr %d%s on hub addr %d, port %d disconnected\n", 
	       parent->dv_xname, dev->address, dev->hub ? " (hub)" : "",
	       up->parent->address, portno);

	for (i = 0; i < dev->cdesc->bNumInterface; i++) {
		for (p = LIST_FIRST(&dev->ifaces[i].pipes); p; p = n) {
			n = LIST_NEXT(p, next);
			if (p->disco)
				p->disco(p->discoarg);
			usbd_abort_pipe(p);
			usbd_close_pipe(p);
		}
	}

	/* XXX Free all data structures and disable further I/O. */


	if (dev->hub) {
		DPRINTFN(3,("usb_disconnect: hub, recursing\n"));
		hd = &dev->hub->hubdesc;
		for(port = 1; port <= hd->bNbrPorts; port++) {
			spi = &dev->hub->ports[port-1];
			if (spi->device)
				uhub_disconnect(sc, parent, spi, port);
		}
	}

	dev->bus->devices[dev->address] = 0;
	up->device = 0;
	/* XXX free */
}

void
uhub_intr(reqh, addr, status)
	usbd_request_handle reqh;
	usbd_private_handle addr;
	usbd_status status;
{
	struct uhub_softc *sc = addr;

	DPRINTFN(1,("uhub_intr: sc=%p\n", sc));
	if (status != USBD_NORMAL_COMPLETION)
		usbd_clear_endpoint_stall_async(sc->sc_ipipe);
	else
		usb_needs_explore(sc->sc_hub->bus);
}
