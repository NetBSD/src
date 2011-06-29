/*	$NetBSD: wsdisplay_util.c,v 1.1 2011/06/29 03:09:37 macallan Exp $ */

/*-
 * Copyright (c) 2009 Michael Lorenz
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

/* some utility functions for use with wsdisplay */
#include <sys/param.h>
#include <sys/stdint.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <dev/cons.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>

int
wsdisplayio_get_edid(device_t dev, struct wsdisplayio_edid_info *d)
{
	prop_data_t edid_data;
	int edid_size;

	edid_data = prop_dictionary_get(device_properties(dev), "EDID");
	if (edid_data != NULL) {
		edid_size = prop_data_size(edid_data);
		/* less than 128 bytes is bogus */
		if (edid_size < 128)
			return ENODEV;
		d->data_size = edid_size;
		if (d->buffer_size < edid_size)
			return EAGAIN;
		return copyout(prop_data_data_nocopy(edid_data),
			       d->edid_data, edid_size);
	}
	return ENODEV;
}
