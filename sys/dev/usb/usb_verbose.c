/*	$NetBSD: usb_verbose.c,v 1.1.2.2 2010/05/30 05:17:45 rmind Exp $ */
/*	$FreeBSD: src/sys/dev/usb/usb_subr.c,v 1.18 1999/11/17 22:33:47 n_hibma Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: usb_verbose.c,v 1.1.2.2 2010/05/30 05:17:45 rmind Exp $");

#include <sys/param.h>
#include <sys/module.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_verbose.h>

/*
 * Descriptions of known vendors and devices ("products").
 */
struct usb_vendor {
	usb_vendor_id_t		vendor;
	const char		*vendorname;
};
struct usb_product {
	usb_vendor_id_t		vendor;
	usb_product_id_t	product;
	const char		*productname;
};

#include <dev/usb/usbdevs_data.h>

void get_usb_vendor_real(char *, usb_vendor_id_t);
void get_usb_product_real(char *, usb_vendor_id_t, usb_product_id_t);

MODULE(MODULE_CLASS_MISC, usbverbose, NULL);
  
static int
usbverbose_modcmd(modcmd_t cmd, void *arg)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		get_usb_vendor = get_usb_vendor_real;
		get_usb_product = get_usb_product_real;
		return 0;
	case MODULE_CMD_FINI:
		get_usb_vendor = (void *)get_usb_none;
		get_usb_product = (void *)get_usb_none;
		return 0;
	default:
		return ENOTTY;  
	}
}

void get_usb_vendor_real(char *v, usb_vendor_id_t v_id)
{
	int n;

	/* There is no need for strlcpy below. */
	for (n = 0; n < usb_nvendors; n++)
		if (usb_vendors[n].vendor == v_id) {
			strcpy(v, usb_vendors[n].vendorname);
			break;
		}
}

void get_usb_product_real(char *p, usb_vendor_id_t v_id, usb_product_id_t p_id)
{
	int n;

	/* There is no need for strlcpy below. */
	for (n = 0; n < usb_nproducts; n++)
		if (usb_products[n].vendor == v_id &&
		    usb_products[n].product == p_id) {
			strcpy(p, usb_products[n].productname);
			break;
		}
}
