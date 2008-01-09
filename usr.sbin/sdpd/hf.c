/*	$NetBSD: hf.c,v 1.1.10.1 2008/01/09 02:02:25 matt Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: hf.c,v 1.1.10.1 2008/01/09 02:02:25 matt Exp $");

#include <sys/queue.h>
#include <bluetooth.h>
#include <sdp.h>
#include <stdio.h>
#include <string.h>

#include <netbt/rfcomm.h>

#include "profile.h"
#include "provider.h"

static int32_t
hf_profile_create_service_class_id_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	static uint16_t	service_classes[] = {
		SDP_SERVICE_CLASS_HANDSFREE,
		SDP_SERVICE_CLASS_GENERIC_AUDIO,
	};

	return (common_profile_create_service_class_id_list(
			buf, eob,
			(uint8_t const *) service_classes,
			sizeof(service_classes)));
}

static int32_t
hf_profile_create_protocol_descriptor_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	provider_t const	*provider = (provider_t const *) data;
	sdp_hf_profile_t	*hf = (sdp_hf_profile_t *) provider->data;

	return (rfcomm_profile_create_protocol_descriptor_list(
			buf, eob,
			(uint8_t const *) &hf->server_channel, 1));
}

static int32_t
hf_profile_create_bluetooth_profile_descriptor_list(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	static uint16_t	profile_descriptor_list[] = {
		SDP_SERVICE_CLASS_HANDSFREE,
		0x0105
	};

	return (common_profile_create_bluetooth_profile_descriptor_list(
			buf, eob,
			(uint8_t const *) profile_descriptor_list,
			sizeof(profile_descriptor_list)));
}

static int32_t
hf_profile_create_service_name(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	static char	service_name[] = "Hands-Free unit";

	return (common_profile_create_string8(
			buf, eob,
			(uint8_t const *) service_name, strlen(service_name)));
}

static int32_t
hf_profile_create_supported_features(
		uint8_t *buf, uint8_t const * const eob,
		uint8_t const *data, uint32_t datalen)
{
	provider_t const	*provider = (provider_t const *) data;
	sdp_hf_profile_t	*hf = (sdp_hf_profile_t *) provider->data;

	if (buf + 3 > eob)
		return (-1);

	SDP_PUT8(SDP_DATA_UINT16, buf);
	SDP_PUT16(hf->supported_features, buf);

	return (3);
}

static int32_t
hf_profile_data_valid(uint8_t const *data, uint32_t datalen)
{
	sdp_hf_profile_t const *hf = (sdp_hf_profile_t const *) data;

	if (hf->server_channel < RFCOMM_CHANNEL_MIN
	    || hf->server_channel > RFCOMM_CHANNEL_MAX
	    || (hf->supported_features & ~0x001f))
		return 0;

	return 1;
}

static attr_t	hf_profile_attrs[] = {
	{ SDP_ATTR_SERVICE_RECORD_HANDLE,
	  common_profile_create_service_record_handle },
	{ SDP_ATTR_SERVICE_CLASS_ID_LIST,
	  hf_profile_create_service_class_id_list },
	{ SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST,
	  hf_profile_create_protocol_descriptor_list },
	{ SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST,
	  hf_profile_create_bluetooth_profile_descriptor_list },
	{ SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET,
	  hf_profile_create_service_name },
	{ SDP_ATTR_SUPPORTED_FEATURES,
	  hf_profile_create_supported_features },
	{ 0, NULL } /* end entry */
};

static uint16_t	hf_profile_uuids[] = {
	SDP_SERVICE_CLASS_HANDSFREE,
	SDP_SERVICE_CLASS_GENERIC_AUDIO,
	SDP_UUID_PROTOCOL_L2CAP,
	SDP_UUID_PROTOCOL_RFCOMM,
};

profile_t	hf_profile_descriptor = {
	hf_profile_uuids,
	sizeof(hf_profile_uuids),
	sizeof(sdp_hf_profile_t),
	hf_profile_data_valid,
	(attr_t const * const) &hf_profile_attrs
};
