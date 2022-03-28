/*	$NetBSD: uhidev.h,v 1.27 2022/03/28 12:44:37 riastradh Exp $	*/

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

#include <dev/usb/usbdi.h>

struct uhidev;

struct uhidev_attach_arg {
	struct usbif_attach_arg *uiaa;
	struct uhidev *parent;
	int reportid;
};

void uhidev_get_report_desc(struct uhidev *, void **, int *);
int uhidev_open(struct uhidev *, void (*)(void *, void *, unsigned), void *);
void uhidev_stop(struct uhidev *);
void uhidev_close(struct uhidev *);
usbd_status uhidev_set_report(struct uhidev *, int, void *, int);
usbd_status uhidev_get_report(struct uhidev *, int, void *, int);
usbd_status uhidev_write(struct uhidev *, void *, int);
usbd_status uhidev_write_async(struct uhidev *, void *, int, int, int,
    usbd_callback, void *);

#define	UHIDEV_OSIZE	64
#define	UHIDEV_MAXREPID	255

#endif	/* _DEV_USB_UHIDEV_H_ */
