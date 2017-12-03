/*	$NetBSD: usbdi_util.h,v 1.44.2.2 2017/12/03 11:37:36 jdolecek Exp $	*/

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

#ifndef _USBDI_UTIL_H_
#define _USBDI_UTIL_H_

#include <dev/usb/usbhid.h>

usbd_status	usbd_get_desc(struct usbd_device *, int,
			      int, int, void *);
usbd_status	usbd_get_config_desc(struct usbd_device *, int,
				     usb_config_descriptor_t *);
usbd_status	usbd_get_config_desc_full(struct usbd_device *, int, void *, int);
usbd_status	usbd_get_bos_desc(struct usbd_device *, int,
				     usb_bos_descriptor_t *);
usbd_status	usbd_get_bos_desc_full(struct usbd_device *, int, void *, int);
usbd_status	usbd_get_device_desc(struct usbd_device *,
				     usb_device_descriptor_t *);
usbd_status	usbd_set_address(struct usbd_device *, int);
usbd_status	usbd_get_port_status(struct usbd_device *,
				     int, usb_port_status_t *);
usbd_status	usbd_get_port_status_ext(struct usbd_device *,
				     int, usb_port_status_ext_t *);
usbd_status	usbd_set_hub_feature(struct usbd_device *, int);
usbd_status	usbd_clear_hub_feature(struct usbd_device *, int);
usbd_status	usbd_set_port_feature(struct usbd_device *, int, int);
usbd_status	usbd_clear_port_feature(struct usbd_device *, int, int);
usbd_status	usbd_set_port_u1_timeout(struct usbd_device *, int, int);
usbd_status	usbd_set_port_u2_timeout(struct usbd_device *, int, int);
usbd_status	usbd_get_device_status(struct usbd_device *, usb_status_t *);
usbd_status	usbd_get_hub_status(struct usbd_device *, usb_hub_status_t *);
usbd_status	usbd_get_protocol(struct usbd_interface *, uint8_t *);
usbd_status	usbd_set_protocol(struct usbd_interface *, int);
usbd_status	usbd_get_report_descriptor(struct usbd_device *, int,
					   int, void *);
usb_hid_descriptor_t *usbd_get_hid_descriptor(struct usbd_interface *);
usbd_status	usbd_set_report(struct usbd_interface *, int, int,
				void *, int);
usbd_status	usbd_get_report(struct usbd_interface *, int, int,
				void *, int);
usbd_status	usbd_set_idle(struct usbd_interface *, int, int);
usbd_status	usbd_read_report_desc(struct usbd_interface *, void **,
				      int *);
usbd_status	usbd_get_config(struct usbd_device *, uint8_t *);
usbd_status	usbd_get_string_desc(struct usbd_device *, int,
				     int, usb_string_descriptor_t *,
				     int *);


usbd_status	usbd_set_config_no(struct usbd_device *, int, int);
usbd_status	usbd_set_config_index(struct usbd_device *, int, int);

usbd_status	usbd_bulk_transfer(struct usbd_xfer *, struct usbd_pipe *,
		uint16_t, uint32_t, void *, uint32_t *);

usbd_status	usbd_intr_transfer(struct usbd_xfer *, struct usbd_pipe *,
		    uint16_t, uint32_t, void *, uint32_t *);

void usb_detach_waitold(device_t);
void usb_detach_wakeupold(device_t);

/*
 * MPSAFE versions - mutex must be at IPL_USB.
 */
void usb_detach_wait(device_t dv, kcondvar_t *, kmutex_t *);
void usb_detach_broadcast(device_t, kcondvar_t *);


typedef struct {
	uByte		bLength;
	uByte		bDescriptorType;
	uByte		bDescriptorSubtype;
} UPACKED usb_cdc_descriptor_t;

const usb_cdc_descriptor_t *usb_find_desc(struct usbd_device *, int,
				      int);
const usb_cdc_descriptor_t *usb_find_desc_if(struct usbd_device *, int,
					 int,
					 usb_interface_descriptor_t *);
#define USBD_CDCSUBTYPE_ANY (~0)

#endif /* _USBDI_UTIL_H_ */
