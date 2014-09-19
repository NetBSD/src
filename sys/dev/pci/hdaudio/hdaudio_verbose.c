/*	$NetBSD: hdaudio_verbose.c,v 1.1 2014/09/19 17:23:35 christos Exp $ */

/*
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: hdaudio_verbose.c,v 1.1 2014/09/19 17:23:35 christos Exp $");

#include <sys/param.h>
#include <sys/module.h>

#include <dev/pci/hdaudio/hdaudio_verbose.h>

/*
 * Descriptions of known vendors and devices ("products").
 */
struct hdaudio_vendor {
	hdaudio_vendor_id_t		vendor;
	const char		*vendorname;
};
struct hdaudio_product {
	hdaudio_vendor_id_t		vendor;
	hdaudio_product_id_t	product;
	const char		*productname;
};

#include <dev/pci/hdaudio/hdaudiodevs.h>
#include <dev/pci/hdaudio/hdaudiodevs_data.h>

void get_hdaudio_vendor_real(char *, size_t, hdaudio_vendor_id_t);
void get_hdaudio_product_real(char *, size_t, hdaudio_vendor_id_t, hdaudio_product_id_t);

MODULE(MODULE_CLASS_MISC, hdaudioverbose, NULL);
  
static int
hdaudioverbose_modcmd(modcmd_t cmd, void *arg)
{
	static void (*saved_hdaudio_vendor)(char *, size_t, hdaudio_vendor_id_t);
	static void (*saved_hdaudio_product)(char *, size_t, hdaudio_vendor_id_t,
		hdaudio_product_id_t);

	switch (cmd) {
	case MODULE_CMD_INIT:
		saved_hdaudio_vendor = get_hdaudio_vendor;
		saved_hdaudio_product = get_hdaudio_product;
		get_hdaudio_vendor = get_hdaudio_vendor_real;
		get_hdaudio_product = get_hdaudio_product_real;
		hdaudio_verbose_loaded = 1;
		return 0;
	case MODULE_CMD_FINI:
		get_hdaudio_vendor = saved_hdaudio_vendor;
		get_hdaudio_product = saved_hdaudio_product;
		hdaudio_verbose_loaded = 0;
		return 0;
	default:
		return ENOTTY;  
	}
}

void get_hdaudio_vendor_real(char *v, size_t vl, hdaudio_vendor_id_t v_id)
{
	int n;

	/* There is no need for strlcpy below. */
	for (n = 0; n < hdaudio_nvendors; n++)
		if (hdaudio_vendors[n].vendor == v_id) {
			strlcpy(v, hdaudio_vendors[n].vendorname, vl);
			return;
		}
	snprintf(v, vl, "vendor 0x%.x", v_id);
}

void get_hdaudio_product_real(char *p, size_t pl, hdaudio_vendor_id_t v_id,
    hdaudio_product_id_t p_id)
{
	int n;

	/* There is no need for strlcpy below. */
	for (n = 0; n < hdaudio_nproducts; n++)
		if (hdaudio_products[n].vendor == v_id &&
		    hdaudio_products[n].product == p_id) {
			strlcpy(p, hdaudio_products[n].productname, pl);
			return;
		}
	snprintf(p, pl, "product 0x%.x", v_id);
}
