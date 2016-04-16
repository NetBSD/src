/*	$NetBSD: umodem.c,v 1.66.4.6 2016/04/16 13:22:00 skrll Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
 * Comm Class spec:  http://www.usb.org/developers/devclass_docs/usbccs10.pdf
 *                   http://www.usb.org/developers/devclass_docs/usbcdc11.pdf
 */

/*
 * TODO:
 * - Add error recovery in various places; the big problem is what
 *   to do in a callback if there is an error.
 * - Implement a Call Device for modems without multiplexed commands.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: umodem.c,v 1.66.4.6 2016/04/16 13:22:00 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/poll.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbcdc.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/ucomvar.h>
#include <dev/usb/umodemvar.h>

Static struct ucom_methods umodem_methods = {
	.ucom_get_status = umodem_get_status,
	.ucom_set = umodem_set,
	.ucom_param = umodem_param,
	.ucom_ioctl = umodem_ioctl,
	.ucom_open = umodem_open,
	.ucom_close = umodem_close,
	.ucom_read = NULL,
	.ucom_write = NULL,
};

int             umodem_match(device_t, cfdata_t, void *);
void            umodem_attach(device_t, device_t, void *);
int             umodem_detach(device_t, int);
int             umodem_activate(device_t, enum devact);

extern struct cfdriver umodem_cd;

CFATTACH_DECL_NEW(umodem, sizeof(struct umodem_softc), umodem_match,
    umodem_attach, umodem_detach, umodem_activate);

int
umodem_match(device_t parent, cfdata_t match, void *aux)
{
	struct usbif_attach_arg *uiaa = aux;
	usb_interface_descriptor_t *id;
	int cm, acm;

	if (uiaa->uiaa_class != UICLASS_CDC ||
	    uiaa->uiaa_subclass != UISUBCLASS_ABSTRACT_CONTROL_MODEL ||
	    !(uiaa->uiaa_proto == UIPROTO_CDC_NOCLASS || uiaa->uiaa_proto == UIPROTO_CDC_AT))
		return UMATCH_NONE;

	id = usbd_get_interface_descriptor(uiaa->uiaa_iface);
	if (umodem_get_caps(uiaa->uiaa_device, &cm, &acm, id) == -1)
		return UMATCH_NONE;

	return UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO;
}
//
void
umodem_attach(device_t parent, device_t self, void *aux)
{
	struct umodem_softc *sc = device_private(self);
	struct usbif_attach_arg *uiaa = aux;
	struct ucom_attach_args ucaa;

	ucaa.ucaa_portno = UCOM_UNK_PORTNO;
	ucaa.ucaa_methods = &umodem_methods;
	ucaa.ucaa_info = NULL;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler");

	if (umodem_common_attach(self, sc, uiaa, &ucaa))
		return;
	return;
}

int
umodem_activate(device_t self, enum devact act)
{
	struct umodem_softc *sc = device_private(self);

	return umodem_common_activate(sc, act);
}

int
umodem_detach(device_t self, int flags)
{
	struct umodem_softc *sc = device_private(self);

	pmf_device_deregister(self);

	return umodem_common_detach(sc, flags);
}
