/*	$NetBSD: usb.c,v 1.6 1998/12/09 00:18:11 augustss Exp $	*/

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

/*
 * USB spec: http://www.teleport.com/cgi-bin/mailmerge.cgi/~usb/cgiform.tpl
 * More USB specs at http://www.usb.org/developers/index.shtml
 */

#include "opt_usbverbose.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/select.h>

#include <dev/usb/usb.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_quirks.h>

#ifdef USB_DEBUG
#define DPRINTF(x)	if (usbdebug) printf x
#define DPRINTFN(n,x)	if (usbdebug>(n)) printf x
int	usbdebug = 0;
int	uhcidebug;
int	ohcidebug;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define USBUNIT(dev) (minor(dev))

struct usb_softc {
	struct device sc_dev;		/* base device */
	usbd_bus_handle sc_bus;		/* USB controller */
	struct usbd_port sc_port;	/* dummy port for root hub */
	char sc_running;
	char sc_exploring;
	struct selinfo sc_consel;	/* waiting for connect change */
};

int usb_match __P((struct device *, struct cfdata *, void *));
void usb_attach __P((struct device *, struct device *, void *));
int usbopen __P((dev_t, int, int, struct proc *));
int usbclose __P((dev_t, int, int, struct proc *));
int usbioctl __P((dev_t, u_long, caddr_t, int, struct proc *));
int usbpoll __P((dev_t, int, struct proc *));

usbd_status usb_discover __P((struct usb_softc *));

extern struct cfdriver usb_cd;

struct cfattach usb_ca = {
	sizeof(struct usb_softc), usb_match, usb_attach
};

int
usb_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	DPRINTF(("usbd_match\n"));
	return (1);
}

void
usb_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct usb_softc *sc = (struct usb_softc *)self;
	usbd_device_handle dev;
	usbd_status r;
	
	printf("\n");

	DPRINTF(("usbd_attach\n"));
	usbd_init();
	sc->sc_bus = aux;
	sc->sc_bus->usbctl = sc;
	sc->sc_running = 1;
	sc->sc_bus->use_polling = 1;
	sc->sc_port.power = USB_MAX_POWER;
	r = usbd_new_device(&sc->sc_dev, sc->sc_bus, 0, 0, 0, &sc->sc_port);
	if (r == USBD_NORMAL_COMPLETION) {
		dev = sc->sc_port.device;
		if (!dev->hub) {
			sc->sc_running = 0;
			printf("%s: root device is not a hub\n",
			       sc->sc_dev.dv_xname);
			return;
		}
		sc->sc_bus->root_hub = dev;
		dev->hub->explore(&sc->sc_dev, sc->sc_bus->root_hub);
	} else {
		printf("%s: root hub problem, error=%d\n", 
		       sc->sc_dev.dv_xname, r);
		sc->sc_running = 0;
	}
	sc->sc_bus->use_polling = 0;
}

int
usbctlprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	/* only "usb"es can attach to host controllers */
	if (pnp)
		printf("usb at %s", pnp);

	return (UNCONF);
}

int
usbopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = USBUNIT(dev);
	struct usb_softc *sc;

	if (unit >= usb_cd.cd_ndevs)
		return (ENXIO);
	sc = usb_cd.cd_devs[unit];
	if (sc == 0 || !sc->sc_running)
		return (ENXIO);

	return (0);
}

int
usbclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	return (0);
}

int
usbioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int unit = USBUNIT(dev);
	struct usb_softc *sc = usb_cd.cd_devs[unit];

	if (sc == 0 || !sc->sc_running)
		return (ENXIO);
	switch (cmd) {
#ifdef USB_DEBUG
	case USB_SETDEBUG:
		usbdebug = uhcidebug = ohcidebug = *(int *)data;
		break;
#endif
	case USB_DISCOVER:
		usb_discover(sc);
		break;
	case USB_REQUEST:
	{
		struct usb_ctl_request *ur = (void *)data;
		int len = UGETW(ur->request.wLength);
		struct iovec iov;
		struct uio uio;
		void *ptr = 0;
		int addr = ur->addr;
		usbd_status r;
		int error = 0;

		if (len < 0 || len > 32768)
			return EINVAL;
		if (addr < 0 || addr >= USB_MAX_DEVICES || 
		    sc->sc_bus->devices[addr] == 0)
			return EINVAL;
		if (len != 0) {
			iov.iov_base = (caddr_t)ur->data;
			iov.iov_len = len;
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			uio.uio_resid = len;
			uio.uio_offset = 0;
			uio.uio_segflg = UIO_USERSPACE;
			uio.uio_rw =
				ur->request.bmRequestType & UT_READ ? 
				UIO_READ : UIO_WRITE;
			uio.uio_procp = p;
			ptr = malloc(len, M_TEMP, M_WAITOK);
			if (uio.uio_rw == UIO_WRITE) {
				error = uiomove(ptr, len, &uio);
				if (error)
					goto ret;
			}
		}
		r = usbd_do_request(sc->sc_bus->devices[addr],
				    &ur->request, ptr);
		if (r) {
			error = EIO;
			goto ret;
		}
		if (len != 0) {
			if (uio.uio_rw == UIO_READ) {
				error = uiomove(ptr, len, &uio);
				if (error)
					goto ret;
			}
		}
	ret:
		if (ptr)
			free(ptr, M_TEMP);
		return (error);
		break;
	}

	case USB_DEVICEINFO:
	{
		struct usb_device_info *di = (void *)data;
		int addr = di->addr;
		usbd_device_handle dev;

		if (addr < 1 || addr >= USB_MAX_DEVICES)
			return (EINVAL);
		dev = sc->sc_bus->devices[addr];
		if (dev == 0)
			return (ENXIO);
		usbd_fill_deviceinfo(dev, di);
		break;
	}

	case USB_DEVICESTATS:
		*(struct usb_device_stats *)data = sc->sc_bus->stats;
		break;

	default:
		return (ENXIO);
	}
	return (0);
}

int
usbpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	int unit = USBUNIT(dev);
	struct usb_softc *sc = usb_cd.cd_devs[unit];
	int revents, s;

	DPRINTFN(2, ("usbpoll: sc=%p events=0x%x\n", sc, events));
	s = splusb();
	revents = 0;
	if (events & (POLLOUT | POLLWRNORM))
		if (sc->sc_bus->needs_explore)
			revents |= events & (POLLOUT | POLLWRNORM);
	DPRINTFN(2, ("usbpoll: revents=0x%x\n", revents));
	if (revents == 0) {
		if (events & (POLLOUT | POLLWRNORM)) {
			DPRINTFN(2, ("usbpoll: selrecord\n"));
			selrecord(p, &sc->sc_consel);
		}
	}
	splx(s);
	return (revents);
}

int
usb_bus_count()
{
	int i, n;

	for (i = n = 0; i < usb_cd.cd_ndevs; i++)
		if (usb_cd.cd_devs[i])
			n++;
	return (n);
}

usbd_status
usb_get_bus_handle(n, h)
	int n;
	usbd_bus_handle *h;
{
	int i;

	for (i = 0; i < usb_cd.cd_ndevs; i++)
		if (usb_cd.cd_devs[i] && n-- == 0) {
			*h = usb_cd.cd_devs[i];
			return (USBD_NORMAL_COMPLETION);
		}
	return (USBD_INVAL);
}

usbd_status
usb_discover(sc)
	struct usb_softc *sc;
{
	int s;

	/* Explore device tree from the root */
	/* We need mutual exclusion while traversing the device tree. */
	s = splusb();
	while (sc->sc_exploring)
		tsleep(&sc->sc_exploring, PRIBIO, "usbdis", 0);
	sc->sc_exploring = 1;
	sc->sc_bus->needs_explore = 0;
	splx(s);

	sc->sc_bus->root_hub->hub->explore(&sc->sc_dev, sc->sc_bus->root_hub);

	s = splusb();
	sc->sc_exploring = 0;
	wakeup(&sc->sc_exploring);
	splx(s);
	/* XXX should we start over if sc_needsexplore is set again? */
	return (0);
}

void
usb_needs_explore(bus)
	usbd_bus_handle bus;
{
	bus->needs_explore = 1;
	selwakeup(&bus->usbctl->sc_consel);
}
