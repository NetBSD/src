/*	$NetBSD: uhidev.h,v 1.23 2022/03/28 12:43:03 riastradh Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#ifndef	_DEV_USB_UHIDEV_H_
#define	_DEV_USB_UHIDEV_H_

#include <sys/rndsource.h>

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

struct uhidev {
	device_t sc_dev;		/* base device */
	struct uhidev_softc *sc_parent;
	uByte sc_report_id;
	uint8_t sc_state;	/* read/written under sc_parent->sc_lock */
#define	UHIDEV_OPEN	0x01	/* device is open */
	int sc_in_rep_size;
	void (*sc_intr)(struct uhidev *, void *, u_int);
	krndsource_t     rnd_source;
};

struct uhidev_attach_arg {
	struct usbif_attach_arg *uiaa;
	struct uhidev_softc *parent;
	int reportid;
	int reportsize;
};

void uhidev_get_report_desc(struct uhidev_softc *, void **, int *);
int uhidev_open(struct uhidev *);
void uhidev_stop(struct uhidev *);
void uhidev_close(struct uhidev *);
usbd_status uhidev_set_report(struct uhidev *, int, void *, int);
usbd_status uhidev_get_report(struct uhidev *, int, void *, int);
usbd_status uhidev_write(struct uhidev *, void *, int);
usbd_status uhidev_write_async(struct uhidev *, void *, int, int, int,
    usbd_callback, void *);

#define	UHIDEV_OSIZE	64

#endif	/* _DEV_USB_UHIDEV_H_ */
