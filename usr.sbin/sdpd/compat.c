/*	$NetBSD: compat.c,v 1.1.2.2 2009/05/13 19:20:39 jym Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Iain Hibbert.
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
__RCSID("$NetBSD: compat.c,v 1.1.2.2 2009/05/13 19:20:39 jym Exp $");

#include <arpa/inet.h>

#include <netbt/rfcomm.h>

#include <bluetooth.h>
#include <errno.h>
#include <sdp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sdpd.h"

/*
 * This file provides for compatibility with the old ABI. Clients send
 * a data structure and we generate a record based from that using the
 * server output buffer as temporary storage. The sdp_put functions will
 * not write invalid data or overflow the buffer which is big enough to
 * contain all these records, no need to worry about that.
 */

static bool
dun_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_dun_profile_t	*data = arg;
	uint8_t			*first = buf->next;

	if (len != sizeof(*data)
	    || data->server_channel < RFCOMM_CHANNEL_MIN
	    || data->server_channel > RFCOMM_CHANNEL_MAX)
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_DIALUP_NETWORKING);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 12);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_seq(buf, 5);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_RFCOMM);
	sdp_put_uint8(buf, data->server_channel);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_DIALUP_NETWORKING);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "Dialup Networking", -1);

	sdp_put_uint16(buf, SDP_ATTR_AUDIO_FEEDBACK_SUPPORT);
	sdp_put_bool(buf, data->audio_feedback_support);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

static bool
ftrn_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_ftrn_profile_t	*data = arg;
	uint8_t			*first = buf->next;

	if (len != sizeof(*data)
	    || data->server_channel < RFCOMM_CHANNEL_MIN
	    || data->server_channel > RFCOMM_CHANNEL_MAX)
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_OBEX_FILE_TRANSFER);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 17);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_seq(buf, 5);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_RFCOMM);
	sdp_put_uint8(buf, data->server_channel);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_OBEX);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_OBEX_FILE_TRANSFER);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "OBEX File Transfer", -1);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

static bool
hset_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_hset_profile_t	*data = arg;
	uint8_t			*first = buf->next;

	if (len != sizeof(*data)
	    || data->server_channel < RFCOMM_CHANNEL_MIN
	    || data->server_channel > RFCOMM_CHANNEL_MAX)
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_HEADSET_AUDIO_GATEWAY);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_GENERIC_AUDIO);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 12);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_seq(buf, 5);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_RFCOMM);
	sdp_put_uint8(buf, data->server_channel);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_HEADSET);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "Voice Gateway", -1);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

static bool
hf_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_hf_profile_t	*data = arg;
	uint8_t			*first = buf->next;

	if (len != sizeof(*data)
	    || data->server_channel < RFCOMM_CHANNEL_MIN
	    || data->server_channel > RFCOMM_CHANNEL_MAX
	    || (data->supported_features & ~0x001f))
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_HANDSFREE);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_GENERIC_AUDIO);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 12);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_seq(buf, 5);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_RFCOMM);
	sdp_put_uint8(buf, data->server_channel);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_HANDSFREE);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "Hands-Free unit", -1);

	sdp_put_uint16(buf, SDP_ATTR_SUPPORTED_FEATURES);
	sdp_put_uint16(buf, data->supported_features);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

static bool
irmc_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_irmc_profile_t	*data = arg;
	uint8_t			*first = buf->next;
	int			i;

	if (len != sizeof(*data)
	    || data->server_channel < RFCOMM_CHANNEL_MIN
	    || data->server_channel > RFCOMM_CHANNEL_MAX
	    || data->supported_formats_size == 0
	    || data->supported_formats_size > sizeof(data->supported_formats))
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_IR_MC_SYNC);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 17);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_seq(buf, 5);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_RFCOMM);
	sdp_put_uint8(buf, data->server_channel);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_OBEX);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_IR_MC_SYNC);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "IrMC Syncrhonization", -1);

	sdp_put_uint16(buf, SDP_ATTR_SUPPORTED_DATA_STORES_LIST);
	sdp_put_seq(buf, data->supported_formats_size * 2);
	for (i = 0; i < data->supported_formats_size; i++)
		sdp_put_uint8(buf, data->supported_formats[i]);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

static bool
irmc_cmd_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_irmc_command_profile_t	*data = arg;
	uint8_t				*first = buf->next;

	if (len != sizeof(*data)
	    || data->server_channel < RFCOMM_CHANNEL_MIN
	    || data->server_channel > RFCOMM_CHANNEL_MAX)
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_IR_MC_SYNC_COMMAND);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 17);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_seq(buf, 5);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_RFCOMM);
	sdp_put_uint8(buf, data->server_channel);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_OBEX);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_IR_MC_SYNC_COMMAND);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "Sync Command Service", -1);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

static bool
lan_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_lan_profile_t	*data = arg;
	uint8_t			*first = buf->next;
	struct in_addr		in;
	char			str[32];

	if (len != sizeof(*data)
	    || data->server_channel < RFCOMM_CHANNEL_MIN
	    || data->server_channel > RFCOMM_CHANNEL_MAX
	    || data->ip_subnet_radius > 32)
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_LAN_ACCESS_USING_PPP);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 12);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_seq(buf, 5);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_RFCOMM);
	sdp_put_uint8(buf, data->server_channel);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_AVAILABILITY);
	sdp_put_uint8(buf, data->load_factor);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_LAN_ACCESS_USING_PPP);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "LAN Access using PPP", -1);

	in.s_addr= data->ip_subnet;
	sprintf(str, "%s/%d", inet_ntoa(in), data->ip_subnet_radius);
	sdp_put_uint16(buf, SDP_ATTR_IP_SUBNET);
	sdp_put_str(buf, str, -1);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

static bool
opush_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_opush_profile_t	*data = arg;
	uint8_t			*first = buf->next;
	int			i;

	if (len != sizeof(*data)
	    || data->server_channel < RFCOMM_CHANNEL_MIN
	    || data->server_channel > RFCOMM_CHANNEL_MAX
	    || data->supported_formats_size == 0
	    || data->supported_formats_size > sizeof(data->supported_formats))
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_OBEX_OBJECT_PUSH);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 17);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_seq(buf, 5);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_RFCOMM);
	sdp_put_uint8(buf, data->server_channel);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_OBEX);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_OBEX_OBJECT_PUSH);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "OBEX Object Push", -1);

	sdp_put_uint16(buf, SDP_ATTR_SUPPORTED_FORMATS_LIST);
	sdp_put_seq(buf, data->supported_formats_size * 2);
	for (i = 0; i < data->supported_formats_size; i++)
		sdp_put_uint8(buf, data->supported_formats[i]);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

static bool
sp_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_sp_profile_t	*data = arg;
	uint8_t			*first = buf->next;

	if (len != sizeof(*data)
	    || data->server_channel < RFCOMM_CHANNEL_MIN
	    || data->server_channel > RFCOMM_CHANNEL_MAX)
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_SERIAL_PORT);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 12);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_seq(buf, 5);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_RFCOMM);
	sdp_put_uint8(buf, data->server_channel);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_SERIAL_PORT);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "Serial Port", -1);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

/* list of protocols used by PAN profiles.  */
static const uint16_t proto[] = {
	0x0800,	/* IPv4 */
	0x0806,	/* ARP  */
#ifdef INET6
	0x86dd,	/* IPv6 */
#endif
};

static bool
nap_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_nap_profile_t	*data = arg;
	uint8_t			*first = buf->next;
	size_t			i;

	if (len != sizeof(*data)
	    || L2CAP_PSM_INVALID(data->psm)
	    || data->security_description > 0x0002)
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_NAP);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 18 + 3 * __arraycount(proto));
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_uint16(buf, data->psm);
	sdp_put_seq(buf, 8 + 3 * __arraycount(proto));
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_BNEP);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */
	sdp_put_seq(buf, 3 * __arraycount(proto));
	for (i = 0; i < __arraycount(proto); i++)
		sdp_put_uint16(buf, proto[i]);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_AVAILABILITY);
	sdp_put_uint8(buf, data->load_factor);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_NAP);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "Network Access Point", -1);

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_DESCRIPTION_OFFSET);
	sdp_put_str(buf, "Personal Ad-hoc Network Service", -1);

	sdp_put_uint16(buf, SDP_ATTR_SECURITY_DESCRIPTION);
	sdp_put_uint16(buf, data->security_description);

	sdp_put_uint16(buf, SDP_ATTR_NET_ACCESS_TYPE);
	sdp_put_uint16(buf, data->net_access_type);

	sdp_put_uint16(buf, SDP_ATTR_MAX_NET_ACCESS_RATE);
	sdp_put_uint32(buf, data->max_net_access_rate);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

static bool
gn_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_gn_profile_t	*data = arg;
	uint8_t			*first = buf->next;
	size_t			i;

	if (len != sizeof(*data)
	    || L2CAP_PSM_INVALID(data->psm)
	    || data->security_description > 0x0002)
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_GN);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 18 + 3 * __arraycount(proto));
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_uint16(buf, data->psm);
	sdp_put_seq(buf, 8 + 3 * __arraycount(proto));
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_BNEP);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */
	sdp_put_seq(buf, 3 * __arraycount(proto));
	for (i = 0; i < __arraycount(proto); i++)
		sdp_put_uint16(buf, proto[i]);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_AVAILABILITY);
	sdp_put_uint8(buf, data->load_factor);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_GN);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "Group Ad-hoc Network", -1);

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_DESCRIPTION_OFFSET);
	sdp_put_str(buf, "Personal Group Ad-hoc Network Service", -1);

	sdp_put_uint16(buf, SDP_ATTR_SECURITY_DESCRIPTION);
	sdp_put_uint16(buf, data->security_description);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

static bool
panu_profile(sdp_data_t *buf, void *arg, ssize_t len)
{
	sdp_panu_profile_t	*data = arg;
	uint8_t			*first = buf->next;
	size_t			i;

	if (len != sizeof(*data)
	    || L2CAP_PSM_INVALID(data->psm)
	    || data->security_description > 0x0002)
		return false;

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(buf, 0x00000000);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PANU);

	sdp_put_uint16(buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 18 + 3 * __arraycount(proto));
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_uint16(buf, data->psm);
	sdp_put_seq(buf, 8 + 3 * __arraycount(proto));
	sdp_put_uuid16(buf, SDP_UUID_PROTOCOL_BNEP);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */
	sdp_put_seq(buf, 3 * __arraycount(proto));
	for (i = 0; i < __arraycount(proto); i++)
		sdp_put_uint16(buf, proto[i]);

	sdp_put_uint16(buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(buf, 3);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(buf, 9);
	sdp_put_uint16(buf, 0x656e);	/* "en" */
	sdp_put_uint16(buf, 106);	/* UTF-8 */
	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(buf, SDP_ATTR_SERVICE_AVAILABILITY);
	sdp_put_uint8(buf, data->load_factor);

	sdp_put_uint16(buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(buf, 8);
	sdp_put_seq(buf, 6);
	sdp_put_uuid16(buf, SDP_SERVICE_CLASS_PANU);
	sdp_put_uint16(buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(buf, "Personal Ad-hoc User Service", -1);

	sdp_put_uint16(buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_DESCRIPTION_OFFSET);
	sdp_put_str(buf, "Personal Ad-hoc User Service", -1);

	sdp_put_uint16(buf, SDP_ATTR_SECURITY_DESCRIPTION);
	sdp_put_uint16(buf, data->security_description);

	buf->end = buf->next;
	buf->next = first;
	return true;
}

static const struct {
	uint16_t	class;
	bool		(*create)(sdp_data_t *, void *, ssize_t);
} known[] = {
	{ SDP_SERVICE_CLASS_DIALUP_NETWORKING,		dun_profile	    },
	{ SDP_SERVICE_CLASS_OBEX_FILE_TRANSFER,		ftrn_profile	    },
	{ SDP_SERVICE_CLASS_HEADSET_AUDIO_GATEWAY,	hset_profile	    },
	{ SDP_SERVICE_CLASS_HANDSFREE,			hf_profile	    },
	{ SDP_SERVICE_CLASS_IR_MC_SYNC,			irmc_profile	    },
	{ SDP_SERVICE_CLASS_IR_MC_SYNC_COMMAND,		irmc_cmd_profile    },
	{ SDP_SERVICE_CLASS_LAN_ACCESS_USING_PPP,	lan_profile	    },
	{ SDP_SERVICE_CLASS_OBEX_OBJECT_PUSH,		opush_profile	    },
	{ SDP_SERVICE_CLASS_SERIAL_PORT,		sp_profile	    },
	{ SDP_SERVICE_CLASS_NAP,			nap_profile	    },
	{ SDP_SERVICE_CLASS_GN,				gn_profile	    },
	{ SDP_SERVICE_CLASS_PANU,			panu_profile	    },
};

uint16_t
compat_register_request(server_t *srv, int fd)
{
	sdp_data_t d, r;
	bdaddr_t bdaddr;
	uint16_t class;
	int i;

	if (!srv->fdidx[fd].control
	    || !srv->fdidx[fd].priv)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	srv->fdidx[fd].offset = 0;
	db_unselect(srv, fd);
	d.next = srv->ibuf;
	d.end = srv->ibuf + srv->pdu.len;

	if (d.next + sizeof(uint16_t) + sizeof(bdaddr_t) > d.end)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	class = be16dec(d.next);
	d.next += sizeof(uint16_t);

	memcpy(&bdaddr, d.next, sizeof(bdaddr_t));
	d.next += sizeof(bdaddr_t);

	for (i = 0;; i++) {
		if (i == __arraycount(known))
			return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

		if (known[i].class == class)
			break;
	}

	r.next = srv->obuf;
	r.end = srv->obuf + srv->omtu;
	if (!(known[i].create(&r, d.next, d.end - d.next)))
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	if (!db_create(srv, fd, &bdaddr, srv->handle, &r))
		return SDP_ERROR_CODE_INSUFFICIENT_RESOURCES;

	/* successful return */
	be16enc(srv->obuf, 0x0000);
	be32enc(srv->obuf + sizeof(uint16_t), srv->handle++);
	srv->pdu.pid = SDP_PDU_ERROR_RESPONSE;
	srv->pdu.len = sizeof(uint16_t) + sizeof(uint32_t);
	return 0;
}

uint16_t
compat_change_request(server_t *srv, int fd)
{
	record_t *rec;
	sdp_data_t d, r;
	int i;

	if (!srv->fdidx[fd].control
	    || !srv->fdidx[fd].priv)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	srv->fdidx[fd].offset = 0;
	db_unselect(srv, fd);
	d.next = srv->ibuf;
	d.end = srv->ibuf + srv->pdu.len;

	if (d.next + sizeof(uint32_t) > d.end)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	db_select_handle(srv, fd, be32dec(d.next));
	d.next += sizeof(uint32_t);

	rec = NULL;
	db_next(srv, fd, &rec);
	if (rec == NULL || rec->fd != fd)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	/*
	 * This is rather simplistic but it works. We don't keep a
	 * record of the ServiceClass but we do know the format of
	 * of the stored record and where it should be. If we dont
	 * find it there, just give up.
	 */
	r = rec->data;
	r.next += 3;		/* uint16 ServiceRecordHandle	*/
	r.next += 5;		/* uint32 %handle%		*/
	r.next += 3;		/* uint16 ServiceClassIDList	*/
	r.next += 2;		/* seq8				*/
	for (i = 0;; i++) {
		if (i == __arraycount(known))
			return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

		if (sdp_match_uuid16(&r, known[i].class))
			break;
	}

	r.next = srv->obuf;
	r.end = srv->obuf + srv->omtu;
	if (!(known[i].create(&r, d.next, d.end - d.next)))
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	if (!db_create(srv, fd, &rec->bdaddr, rec->handle, &r))
		return SDP_ERROR_CODE_INSUFFICIENT_RESOURCES;

	db_unselect(srv, fd);

	/* successful return */
	be16enc(srv->obuf, 0x0000);
	srv->pdu.pid = SDP_PDU_ERROR_RESPONSE;
	srv->pdu.len = sizeof(uint16_t);
	return 0;
}
