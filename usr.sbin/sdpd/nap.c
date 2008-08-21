/*	$NetBSD: nap.c,v 1.1.10.2 2008/08/21 20:31:06 bouyer Exp $	*/

/*-
 * Copyright (c) 2008 Iain Hibbert
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: nap.c,v 1.1.10.2 2008/08/21 20:31:06 bouyer Exp $");

#include <sys/queue.h>
#include <bluetooth.h>
#include <sdp.h>
#include <string.h>
#include "profile.h"
#include "provider.h"

static int32_t
nap_profile_create_service_class_id_list(uint8_t *buf,
    uint8_t const * const eob, uint8_t const *data, uint32_t datalen)
{
	const uint16_t service_classes[] = {
		SDP_SERVICE_CLASS_NAP,
	};

	return common_profile_create_service_class_id_list(buf, eob,
	    (uint8_t const *) service_classes, sizeof(service_classes));
}

static int32_t
nap_profile_create_protocol_descriptor_list(uint8_t *buf,
    uint8_t const *const eob, uint8_t const *data, uint32_t datalen)
{
	provider_t const *provider = (provider_t const *) data;
	sdp_nap_profile_t *nap = (sdp_nap_profile_t *) provider->data;

	return bnep_profile_create_protocol_descriptor_list(buf, eob,
	    (uint8_t const *) &nap->psm, 2);
}

static int32_t
nap_profile_create_bluetooth_profile_descriptor_list(uint8_t *buf,
    uint8_t const * const eob, uint8_t const *data, uint32_t datalen)
{
	const uint16_t profile_descriptor_list[] = {
		SDP_SERVICE_CLASS_NAP,
		0x0100,
	};

	return common_profile_create_bluetooth_profile_descriptor_list(buf, eob,
	    (uint8_t const *) profile_descriptor_list,
	    sizeof(profile_descriptor_list));
}

static int32_t
nap_profile_create_service_availability(uint8_t *buf,
    uint8_t const * const eob, uint8_t const *data, uint32_t datalen)
{
	provider_t const *provider = (provider_t const *) data;
	sdp_nap_profile_t *nap = (sdp_nap_profile_p) provider->data;

	if (buf + 2 > eob)
		return -1;

	SDP_PUT8(SDP_DATA_UINT8, buf);
	SDP_PUT8(nap->load_factor, buf);

	return 2;
}

static int32_t
nap_profile_create_service_name(uint8_t *buf,
    uint8_t const * const eob, uint8_t const *data, uint32_t datalen)
{
	const char service_name[] = "Network Access Point";

	return common_profile_create_string8(buf, eob,
	    (uint8_t const *) service_name, strlen(service_name));
}

static int32_t
nap_profile_create_service_description(uint8_t *buf,
    uint8_t const * const eob, uint8_t const *data, uint32_t datalen)
{
	const char service_descr[] = "Personal Ad-hoc Network Service";

	return common_profile_create_string8(buf, eob,
	    (uint8_t const *) service_descr, strlen(service_descr));
}

static int32_t
nap_profile_create_security_description(uint8_t *buf,
    uint8_t const * const eob, uint8_t const *data, uint32_t datalen)
{
	provider_t const *provider = (provider_t const *) data;
	sdp_nap_profile_t *nap = (sdp_nap_profile_t *) provider->data;

	if (buf + 3 > eob)
		return -1;

	SDP_PUT8(SDP_DATA_UINT16, buf);
	SDP_PUT16(nap->security_description, buf);

	return 3;
}

static int32_t
nap_profile_create_net_access_type(uint8_t *buf,
    uint8_t const * const eob, uint8_t const *data, uint32_t datalen)
{
	provider_t const *provider = (provider_t const *) data;
	sdp_nap_profile_t *nap = (sdp_nap_profile_t *) provider->data;

	if (buf + 3 > eob)
		return -1;

	SDP_PUT8(SDP_DATA_UINT16, buf);
	SDP_PUT16(nap->net_access_type, buf);

	return 3;
}

static int32_t
nap_profile_create_max_net_access_rate(uint8_t *buf,
    uint8_t const * const eob, uint8_t const *data, uint32_t datalen)
{
	provider_t const *provider = (provider_t const *) data;
	sdp_nap_profile_t *nap = (sdp_nap_profile_t *) provider->data;

	if (buf + 5 > eob)
		return -1;

	SDP_PUT8(SDP_DATA_UINT32, buf);
	SDP_PUT32(nap->max_net_access_rate, buf);

	return 5;
}

static int32_t
nap_profile_data_valid(uint8_t const *data, uint32_t datalen)
{
	sdp_nap_profile_t const *nap = (sdp_nap_profile_t const *) data;

	if (L2CAP_PSM_INVALID(nap->psm)
	    || nap->security_description > 0x0002)
		return 0;

	return 1;
}

static attr_t nap_profile_attrs[] = {
	{ SDP_ATTR_SERVICE_RECORD_HANDLE,
	  common_profile_create_service_record_handle },
	{ SDP_ATTR_SERVICE_CLASS_ID_LIST,
	  nap_profile_create_service_class_id_list },
	{ SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST,
	  nap_profile_create_protocol_descriptor_list },
	{ SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST,
	  common_profile_create_language_base_attribute_id_list },
	{ SDP_ATTR_SERVICE_AVAILABILITY,
	  nap_profile_create_service_availability },
	{ SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST,
	  nap_profile_create_bluetooth_profile_descriptor_list },
	{ SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET,
	  nap_profile_create_service_name },
	{ SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_DESCRIPTION_OFFSET,
	  nap_profile_create_service_description },
	{ SDP_ATTR_SECURITY_DESCRIPTION,
	  nap_profile_create_security_description },
	{ SDP_ATTR_NET_ACCESS_TYPE,
	  nap_profile_create_net_access_type },
	{ SDP_ATTR_MAX_NET_ACCESS_RATE,
	  nap_profile_create_max_net_access_rate },
	{ 0, NULL } /* end entry */
};

static uint16_t nap_profile_uuids[] = {
	SDP_SERVICE_CLASS_NAP,
	SDP_UUID_PROTOCOL_L2CAP,
	SDP_UUID_PROTOCOL_BNEP,
};

profile_t nap_profile_descriptor = {
	nap_profile_uuids,
	sizeof(nap_profile_uuids),
	sizeof(sdp_nap_profile_t),
	nap_profile_data_valid,
	(attr_t const * const) &nap_profile_attrs,
};
