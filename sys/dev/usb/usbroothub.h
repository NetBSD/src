/* $NetBSD: usbroothub.h,v 1.3.2.2 2017/12/03 11:37:36 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

int usb_makestrdesc(usb_string_descriptor_t *, int, const char *);
int usb_makelangtbl(usb_string_descriptor_t *, int);

struct usb_roothub_descriptors {
	usb_config_descriptor_t urh_confd;
	usb_interface_descriptor_t urh_ifcd;
	usb_endpoint_descriptor_t urh_endpd;
};

struct usb3_roothub_descriptors {
	usb_config_descriptor_t urh_confd;
	usb_interface_descriptor_t urh_ifcd;
	usb_endpoint_descriptor_t urh_endpd;
	usb_endpoint_ss_comp_descriptor_t urh_endpssd;
};

struct usb3_roothub_bos_descriptors {
	usb_bos_descriptor_t urh_bosd;
	usb_devcap_usb2ext_descriptor_t urh_usb2extd;
	usb_devcap_ss_descriptor_t urh_ssd;
	usb_devcap_container_id_descriptor_t urh_containerd;
};

#define	USBROOTHUB_INTR_ENDPT	1

extern const struct usbd_pipe_methods roothub_ctrl_methods;
