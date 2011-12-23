/*	$NetBSD: usb_verbose.c,v 1.6 2011/12/23 00:51:48 jakllsch Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: usb_verbose.c,v 1.6 2011/12/23 00:51:48 jakllsch Exp $");

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

void get_usb_vendor_real(char *, size_t, usb_vendor_id_t);
void get_usb_product_real(char *, size_t, usb_vendor_id_t, usb_product_id_t);

MODULE(MODULE_CLASS_MISC, usbverbose, NULL);
  
static int
usbverbose_modcmd(modcmd_t cmd, void *arg)
{
	static void (*saved_usb_vendor)(char *, size_t, usb_vendor_id_t);
	static void (*saved_usb_product)(char *, size_t, usb_vendor_id_t,
		usb_product_id_t);

	switch (cmd) {
	case MODULE_CMD_INIT:
		saved_usb_vendor = get_usb_vendor;
		saved_usb_product = get_usb_product;
		get_usb_vendor = get_usb_vendor_real;
		get_usb_product = get_usb_product_real;
		usb_verbose_loaded = 1;
		return 0;
	case MODULE_CMD_FINI:
		get_usb_vendor = saved_usb_vendor;
		get_usb_product = saved_usb_product;
		usb_verbose_loaded = 0;
		return 0;
	default:
		return ENOTTY;  
	}
}

void get_usb_vendor_real(char *v, size_t vl, usb_vendor_id_t v_id)
{
	int n;

	/* There is no need for strlcpy below. */
	for (n = 0; n < usb_nvendors; n++)
		if (usb_vendors[n].vendor == v_id) {
			strlcpy(v, usb_vendors[n].vendorname, vl);
			break;
		}
}

void get_usb_product_real(char *p, size_t pl, usb_vendor_id_t v_id,
    usb_product_id_t p_id)
{
	int n;

	/* There is no need for strlcpy below. */
	for (n = 0; n < usb_nproducts; n++)
		if (usb_products[n].vendor == v_id &&
		    usb_products[n].product == p_id) {
			strlcpy(p, usb_products[n].productname, pl);
			break;
		}
}
