/*	$NetBSD: ofw_network_subr.c,v 1.10 2021/01/27 02:31:35 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
__KERNEL_RCSID(0, "$NetBSD: ofw_network_subr.c,v 1.10 2021/01/27 02:31:35 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_media.h>

#include <dev/ofw/openfirm.h>

static const struct device_compatible_entry media_compat[] = {
	{ .compat = "ethernet,10,rj45,half",
	  .value = IFM_ETHER | IFM_10_T },

	{ .compat = "ethernet,10,rj45,full",
	  .value = IFM_ETHER | IFM_10_T | IFM_FDX },

	{ .compat = "ethernet,10,aui,half",
	  .value = IFM_ETHER | IFM_10_5 },

	{ .compat = "ethernet,10,bnc,half",
	  .value = IFM_ETHER | IFM_10_2 },

	{ .compat = "ethernet,100,rj45,half",
	  .value = IFM_ETHER | IFM_100_TX },

	{ .compat = "ethernet,100,rj45,full",
	  .value = IFM_ETHER | IFM_100_TX | IFM_FDX },

	DEVICE_COMPAT_EOL
};

/*
 * int of_network_decode_media(phandle, nmediap, defmediap)
 *
 * This routine decodes the OFW properties `supported-network-types'
 * and `chosen-network-type'.
 *
 * Arguments:
 *	phandle		OFW phandle of device whos network media properties
 *			are to be decoded.
 *	nmediap		Pointer to an integer which will be initialized
 *			with the number of returned media words.
 *	defmediap	Pointer to an integer which will be initialized
 *			with the default network media.
 *
 * Return Values:
 *	An array of integers, allocated with malloc(), containing the
 *	decoded media values.  The number of elements in the array will
 *	be stored in the location pointed to by the `nmediap' argument.
 *	The default media will be stored in the location pointed to by
 *	the `defmediap' argument.
 *
 * Side Effects:
 *	None.
 */
int *
of_network_decode_media(int phandle, int *nmediap, int *defmediap)
{
	const struct device_compatible_entry *dce;
	int nmedia, len, *rv = NULL;
	char *sl = NULL;
	const char *cp;
	size_t cursor;
	unsigned int count;

	len = OF_getproplen(phandle, "supported-network-types");
	if (len <= 0)
		return (NULL);

	sl = malloc(len, M_TEMP, M_WAITOK);

	/* `supported-network-types' should not change. */
	if (OF_getprop(phandle, "supported-network-types", sl, len) != len)
		goto bad;

	count = strlist_count(sl, len);

	if (count == 0)
		goto bad;

	/* Allocate the return value array. */
	rv = malloc(count * sizeof(int), M_DEVBUF, M_WAITOK);

	/* Parse each media string. */
	for (nmedia = 0, cursor = 0;
	     (cp = strlist_next(sl, len, &cursor)) != NULL; ) {
		dce = device_compatible_lookup(&cp, 1, media_compat);
		if (dce != NULL) {
			rv[nmedia++] = (int)dce->value;
		}
	}
	/* Sanity... */
	if (nmedia == 0)
		goto bad;

	free(sl, M_TEMP);
	sl = NULL;

	/*
	 * We now have the `supported-media-types' property decoded.
	 * Next step is to decode the `chosen-media-type' property,
	 * if it exists.
	 */
	*defmediap = -1;
	len = OF_getproplen(phandle, "chosen-network-type");
	if (len <= 0) {
		/* Property does not exist. */
		*defmediap = -1;
		goto done;
	}

	sl = malloc(len, M_TEMP, M_WAITOK);
	if (OF_getprop(phandle, "chosen-network-type", sl, len) != len) {
		/* Something went wrong... */
		goto done;
	}

	cp = sl;
	dce = device_compatible_lookup(&cp, 1, media_compat);
	if (dce != NULL) {
		*defmediap = (int)dce->value;
	}

 done:
	if (sl != NULL)
		free(sl, M_TEMP);
	*nmediap = nmedia;
	return (rv);

 bad:
	if (rv != NULL)
		free(rv, M_DEVBUF);
	if (sl != NULL)
		free(sl, M_TEMP);
	return (NULL);
}
