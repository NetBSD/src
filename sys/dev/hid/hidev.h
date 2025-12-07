/* $NetBSD: hidev.h,v 1.1 2025/12/07 10:05:10 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _DEV_HID_HIDEV_H_
#define _DEV_HID_HIDEV_H_

#include <dev/usb/usbdi.h>

typedef struct hidev_tag {
	/* Opaque cookie for use by back-end. */
	void	*_cookie;

	/* HID device methods. */
	void		(*_get_report_desc)(void *, void **, int *);
	int		(*_open)(void *, void (*)(void *, void *, u_int),
			   void *);
	void		(*_stop)(void *);
	void		(*_close)(void *);
	usbd_status	(*_set_report)(void *, int, void *, int);
	usbd_status	(*_get_report)(void *, int, void *, int);
	usbd_status	(*_write)(void *, void *, int);
} *hidev_tag_t;

void hidev_get_report_desc(hidev_tag_t, void **, int *);
int hidev_open(hidev_tag_t, void (*)(void *, void *, u_int), void *);
void hidev_stop(hidev_tag_t);
void hidev_close(hidev_tag_t);
usbd_status hidev_set_report(hidev_tag_t, int, void *, int);
usbd_status hidev_get_report(hidev_tag_t, int, void *, int);
usbd_status hidev_write(hidev_tag_t, void *, int);

#endif /* _DEV_HID_HIDEV_H_ */
