/*	$NetBSD: sdp_compat.c,v 1.2 2009/05/14 19:12:45 plunky Exp $	*/

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
/*-
 * Copyright (c) 2001-2003 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This file provides compatibility with the original library API,
 * use -DSDP_COMPAT to access it.
 *
 * These functions are deprecated and will be removed eventually.
 *
 *	sdp_open(laddr, raddr)
 *	sdp_open_local(control)
 *	sdp_close(session)
 *	sdp_error(session)
 *	sdp_search(session, plen, protos, alen, attrs, vlen, values)
 *	sdp_register_service(session, uuid, bdaddr, data, datalen, handle)
 *	sdp_change_service(session, handle, data, datalen)
 *	sdp_unregister_service(session, handle)
 *	sdp_attr2desc(attribute)
 *	sdp_uuid2desc(uuid16)
 *	sdp_print(level, start, end)
 */
#define SDP_COMPAT

#include <sys/cdefs.h>
__RCSID("$NetBSD: sdp_compat.c,v 1.2 2009/05/14 19:12:45 plunky Exp $");

#include <errno.h>
#include <sdp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sdp-int.h"

struct sdp_compat {
	sdp_session_t	ss;
	int		error;
	uint8_t		buf[256];
};

void *
sdp_open(bdaddr_t const *l, bdaddr_t const *r)
{
	struct sdp_compat *sc;

	sc = malloc(sizeof(struct sdp_compat));
	if (sc == NULL)
		return NULL;

	if (l == NULL || r == NULL) {
		sc->error = EINVAL;
		return sc;
	}

	sc->ss = _sdp_open(l, r);
	if (sc->ss == NULL) {
		sc->error = errno;
		return sc;
	}

	sc->error = 0;
	return sc;
}

void *
sdp_open_local(char const *control)
{
	struct sdp_compat *sc;

	sc = malloc(sizeof(struct sdp_compat));
	if (sc == NULL)
		return NULL;

	sc->ss = _sdp_open_local(control);
	if (sc->ss == NULL) {
		sc->error = errno;
		return sc;
	}

	sc->error = 0;
	return sc;
}

int32_t
sdp_close(void *xss)
{
	struct sdp_compat *sc = xss;

	if (sc == NULL)
		return 0;

	if (sc->ss != NULL)
		_sdp_close(sc->ss);

	free(sc);

	return 0;
}

int32_t
sdp_error(void *xss)
{
	struct sdp_compat *sc = xss;

	if (sc == NULL)
		return EINVAL;

	return sc->error;
}

int32_t
sdp_search(void *xss, uint32_t plen, uint16_t const *pp, uint32_t alen,
    uint32_t const *ap, uint32_t vlen, sdp_attr_t *vp)
{
	struct sdp_compat *sc = xss;
	sdp_data_t seq, ssp, ail, rsp, value;
	uint16_t attr;
	size_t i;
	bool rv;

	if (sc == NULL)
		return -1;

	if (plen == 0 || pp == NULL || alen == 0 || ap == NULL) {
		sc->error = EINVAL;
		return -1;
	}

	/*
	 * encode ServiceSearchPattern
	 */
	ssp.next = sc->buf;
	ssp.end = sc->buf + sizeof(sc->buf);
	for (i = 0; i < plen; i++)
		sdp_put_uuid16(&ssp, pp[i]);

	ssp.end = ssp.next;
	ssp.next = sc->buf;

	/*
	 * encode AttributeIDList
	 */
	ail.next = ssp.end;
	ail.end = sc->buf + sizeof(sc->buf);
	for (i = 0; i < alen; i++)
		sdp_put_uint32(&ail, ap[i]);

	ail.end = ail.next;
	ail.next = ssp.end;

	/*
	 * perform ServiceSearchAttribute transaction
	 */
	rv = sdp_service_search_attribute(sc->ss, &ssp, &ail, &rsp);
	if (rv == false) {
		sc->error = errno;
		return -1;
	}

	if (vp == NULL)
		return 0;

	/*
	 * The response buffer is a list of data element sequences,
	 * each containing a list of attribute/value pairs. We want to
	 * parse those to the attribute array that the user passed in.
	 */
	while (vlen > 0 && sdp_get_seq(&rsp, &seq)) {
		while (vlen > 0 && sdp_get_attr(&seq, &attr, &value)) {
			vp->attr = attr;
			if (vp->value != NULL) {
				if (value.end - value.next > (ssize_t)vp->vlen) {
					vp->flags = SDP_ATTR_TRUNCATED;
				} else {
					vp->flags = SDP_ATTR_OK;
					vp->vlen = value.end - value.next;
				}
				memcpy(vp->value, value.next, vp->vlen);
			} else {
				vp->flags = SDP_ATTR_INVALID;
			}

			vp++;
			vlen--;
		}
	}

	while (vlen-- > 0)
		vp++->flags = SDP_ATTR_INVALID;

	return 0;
}

int32_t
sdp_register_service(void *xss, uint16_t uuid, bdaddr_t *bdaddr,
    uint8_t *data, uint32_t datalen, uint32_t *handle)
{
	struct sdp_compat *sc = xss;
	struct iovec req[4];
	ssize_t len;

	if (sc == NULL)
		return -1;

	if (bdaddr == NULL || data == NULL || datalen == 0) {
		sc->error = EINVAL;
		return -1;
	}

	uuid = htobe16(uuid);
	req[1].iov_base = &uuid;
	req[1].iov_len = sizeof(uint16_t);

	req[2].iov_base = bdaddr;
	req[2].iov_len = sizeof(bdaddr_t);

	req[3].iov_base = data;
	req[3].iov_len = datalen;

	if (!_sdp_send_pdu(sc->ss, SDP_PDU_SERVICE_REGISTER_REQUEST,
	    req, __arraycount(req))) {
		sc->error = errno;
		return -1;
	}

	len = _sdp_recv_pdu(sc->ss, SDP_PDU_ERROR_RESPONSE);
	if (len == -1) {
		sc->error = errno;
		return -1;
	}

	if (len != sizeof(uint16_t) + sizeof(uint32_t)
	    || be16dec(sc->ss->ibuf) != 0) {
		sc->error = EIO;
		return -1;
	}

	if (handle != NULL)
		*handle = be32dec(sc->ss->ibuf + sizeof(uint16_t));

	return 0;
}

int32_t
sdp_change_service(void *xss, uint32_t handle,
    uint8_t *data, uint32_t datalen)
{
	struct sdp_compat *sc = xss;
	struct iovec req[3];
	ssize_t len;

	if (data == NULL || datalen == 0) {
		sc->error = EINVAL;
		return -1;
	}

	handle = htobe32(handle);
	req[1].iov_base = &handle;
	req[1].iov_len = sizeof(uint32_t);

	req[2].iov_base = data;
	req[2].iov_len = datalen;

	if (!_sdp_send_pdu(sc->ss, SDP_PDU_SERVICE_CHANGE_REQUEST,
	    req, __arraycount(req))) {
		sc->error = errno;
		return -1;
	}

	len = _sdp_recv_pdu(sc->ss, SDP_PDU_ERROR_RESPONSE);
	if (len == -1) {
		sc->error = errno;
		return -1;
	}

	if (len != sizeof(uint16_t)
	    || be16dec(sc->ss->ibuf) != 0) {
		sc->error = EIO;
		return -1;
	}

	return 0;
}

int32_t
sdp_unregister_service(void *xss, uint32_t handle)
{
	struct sdp_compat *sc = xss;
	struct iovec req[2];
	ssize_t len;

	handle = htobe32(handle);
	req[1].iov_base = &handle;
	req[1].iov_len = sizeof(uint32_t);

	if (!_sdp_send_pdu(sc->ss, SDP_PDU_SERVICE_UNREGISTER_REQUEST,
	    req, __arraycount(req))) {
		sc->error = errno;
		return -1;
	}

	len = _sdp_recv_pdu(sc->ss, SDP_PDU_ERROR_RESPONSE);
	if (len == -1) {
		sc->error = errno;
		return -1;
	}

	if (len != sizeof(uint16_t)
	    || be16dec(sc->ss->ibuf) != 0) {
		sc->error = EIO;
		return -1;
	}

	return 0;
}

/*
 * SDP attribute description
 */

struct sdp_attr_desc {
	uint32_t	 attr;
	char const	*desc;
};
typedef struct sdp_attr_desc	sdp_attr_desc_t;
typedef struct sdp_attr_desc *	sdp_attr_desc_p;

static sdp_attr_desc_t	sdp_uuids_desc[] = {
{ SDP_UUID_PROTOCOL_SDP, "SDP", },
{ SDP_UUID_PROTOCOL_UDP, "UDP", },
{ SDP_UUID_PROTOCOL_RFCOMM, "RFCOMM", },
{ SDP_UUID_PROTOCOL_TCP, "TCP", },
{ SDP_UUID_PROTOCOL_TCS_BIN, "TCS BIN", },
{ SDP_UUID_PROTOCOL_TCS_AT, "TCS AT", },
{ SDP_UUID_PROTOCOL_OBEX, "OBEX", },
{ SDP_UUID_PROTOCOL_IP, "IP", },
{ SDP_UUID_PROTOCOL_FTP, "FTP", },
{ SDP_UUID_PROTOCOL_HTTP, "HTTP", },
{ SDP_UUID_PROTOCOL_WSP, "WSP", },
{ SDP_UUID_PROTOCOL_BNEP, "BNEP", },
{ SDP_UUID_PROTOCOL_UPNP, "UPNP", },
{ SDP_UUID_PROTOCOL_HIDP, "HIDP", },
{ SDP_UUID_PROTOCOL_HARDCOPY_CONTROL_CHANNEL, "Hardcopy Control Channel", },
{ SDP_UUID_PROTOCOL_HARDCOPY_DATA_CHANNEL, "Hardcopy Data Channel", },
{ SDP_UUID_PROTOCOL_HARDCOPY_NOTIFICATION, "Hardcopy Notification", },
{ SDP_UUID_PROTOCOL_AVCTP, "AVCTP", },
{ SDP_UUID_PROTOCOL_AVDTP, "AVDTP", },
{ SDP_UUID_PROTOCOL_CMPT, "CMPT", },
{ SDP_UUID_PROTOCOL_UDI_C_PLANE, "UDI C-Plane", },
{ SDP_UUID_PROTOCOL_L2CAP, "L2CAP", },
/* Service Class IDs/Bluetooth Profile IDs */
{ SDP_SERVICE_CLASS_SERVICE_DISCOVERY_SERVER, "Service Discovery Server", },
{ SDP_SERVICE_CLASS_BROWSE_GROUP_DESCRIPTOR, "Browse Group Descriptor", },
{ SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP, "Public Browse Group", },
{ SDP_SERVICE_CLASS_SERIAL_PORT, "Serial Port", },
{ SDP_SERVICE_CLASS_LAN_ACCESS_USING_PPP, "LAN Access Using PPP", },
{ SDP_SERVICE_CLASS_DIALUP_NETWORKING, "Dial-Up Networking", },
{ SDP_SERVICE_CLASS_IR_MC_SYNC, "IrMC Sync", },
{ SDP_SERVICE_CLASS_OBEX_OBJECT_PUSH, "OBEX Object Push", },
{ SDP_SERVICE_CLASS_OBEX_FILE_TRANSFER, "OBEX File Transfer", },
{ SDP_SERVICE_CLASS_IR_MC_SYNC_COMMAND, "IrMC Sync Command", },
{ SDP_SERVICE_CLASS_HEADSET, "Headset", },
{ SDP_SERVICE_CLASS_CORDLESS_TELEPHONY, "Cordless Telephony", },
{ SDP_SERVICE_CLASS_AUDIO_SOURCE, "Audio Source", },
{ SDP_SERVICE_CLASS_AUDIO_SINK, "Audio Sink", },
{ SDP_SERVICE_CLASS_AV_REMOTE_CONTROL_TARGET, "A/V Remote Control Target", },
{ SDP_SERVICE_CLASS_ADVANCED_AUDIO_DISTRIBUTION, "Advanced Audio Distribution", },
{ SDP_SERVICE_CLASS_AV_REMOTE_CONTROL, "A/V Remote Control", },
{ SDP_SERVICE_CLASS_VIDEO_CONFERENCING, "Video Conferencing", },
{ SDP_SERVICE_CLASS_INTERCOM, "Intercom", },
{ SDP_SERVICE_CLASS_FAX, "Fax", },
{ SDP_SERVICE_CLASS_HEADSET_AUDIO_GATEWAY, "Headset Audio Gateway", },
{ SDP_SERVICE_CLASS_WAP, "WAP", },
{ SDP_SERVICE_CLASS_WAP_CLIENT, "WAP Client", },
{ SDP_SERVICE_CLASS_PANU, "PANU", },
{ SDP_SERVICE_CLASS_NAP, "Network Access Point", },
{ SDP_SERVICE_CLASS_GN, "GN", },
{ SDP_SERVICE_CLASS_DIRECT_PRINTING, "Direct Printing", },
{ SDP_SERVICE_CLASS_REFERENCE_PRINTING, "Reference Printing", },
{ SDP_SERVICE_CLASS_IMAGING, "Imaging", },
{ SDP_SERVICE_CLASS_IMAGING_RESPONDER, "Imaging Responder", },
{ SDP_SERVICE_CLASS_IMAGING_AUTOMATIC_ARCHIVE, "Imaging Automatic Archive", },
{ SDP_SERVICE_CLASS_IMAGING_REFERENCED_OBJECTS, "Imaging Referenced Objects", },
{ SDP_SERVICE_CLASS_HANDSFREE, "Handsfree", },
{ SDP_SERVICE_CLASS_HANDSFREE_AUDIO_GATEWAY, "Handsfree Audio Gateway", },
{ SDP_SERVICE_CLASS_DIRECT_PRINTING_REFERENCE_OBJECTS, "Direct Printing Reference Objects", },
{ SDP_SERVICE_CLASS_REFLECTED_UI, "Reflected UI", },
{ SDP_SERVICE_CLASS_BASIC_PRINTING, "Basic Printing", },
{ SDP_SERVICE_CLASS_PRINTING_STATUS, "Printing Status", },
{ SDP_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE, "Human Interface Device", },
{ SDP_SERVICE_CLASS_HARDCOPY_CABLE_REPLACEMENT, "Hardcopy Cable Replacement", },
{ SDP_SERVICE_CLASS_HCR_PRINT, "HCR Print", },
{ SDP_SERVICE_CLASS_HCR_SCAN, "HCR Scan", },
{ SDP_SERVICE_CLASS_COMMON_ISDN_ACCESS, "Common ISDN Access", },
{ SDP_SERVICE_CLASS_VIDEO_CONFERENCING_GW, "Video Conferencing Gateway", },
{ SDP_SERVICE_CLASS_UDI_MT, "UDI MT", },
{ SDP_SERVICE_CLASS_UDI_TA, "UDI TA", },
{ SDP_SERVICE_CLASS_AUDIO_VIDEO, "Audio/Video", },
{ SDP_SERVICE_CLASS_SIM_ACCESS, "SIM Access", },
{ SDP_SERVICE_CLASS_PNP_INFORMATION, "PNP Information", },
{ SDP_SERVICE_CLASS_GENERIC_NETWORKING, "Generic Networking", },
{ SDP_SERVICE_CLASS_GENERIC_FILE_TRANSFER, "Generic File Transfer", },
{ SDP_SERVICE_CLASS_GENERIC_AUDIO, "Generic Audio", },
{ SDP_SERVICE_CLASS_GENERIC_TELEPHONY, "Generic Telephony", },
{ SDP_SERVICE_CLASS_UPNP, "UPNP", },
{ SDP_SERVICE_CLASS_UPNP_IP, "UPNP IP", },
{ SDP_SERVICE_CLASS_ESDP_UPNP_IP_PAN, "ESDP UPNP IP PAN", },
{ SDP_SERVICE_CLASS_ESDP_UPNP_IP_LAP, "ESDP UPNP IP LAP", },
{ SDP_SERVICE_CLASS_ESDP_UPNP_L2CAP, "ESDP UPNP L2CAP", },
{ 0xffff, NULL, }
};

static sdp_attr_desc_t	sdp_attrs_desc[] = {
{ SDP_ATTR_SERVICE_RECORD_HANDLE,
  "Record handle",
  },
{ SDP_ATTR_SERVICE_CLASS_ID_LIST,
  "Service Class ID list",
  },
{ SDP_ATTR_SERVICE_RECORD_STATE,
  "Service Record State",
  },
{ SDP_ATTR_SERVICE_ID,
  "Service ID",
  },
{ SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST,
  "Protocol Descriptor List",
  },
{ SDP_ATTR_BROWSE_GROUP_LIST,
  "Browse Group List",
  },
{ SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST,
  "Language Base Attribute ID List",
  },
{ SDP_ATTR_SERVICE_INFO_TIME_TO_LIVE,
  "Service Info Time-To-Live",
  },
{ SDP_ATTR_SERVICE_AVAILABILITY,
  "Service Availability",
  },
{ SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST,
  "Bluetooh Profile Descriptor List",
  },
{ SDP_ATTR_DOCUMENTATION_URL,
  "Documentation URL",
  },
{ SDP_ATTR_CLIENT_EXECUTABLE_URL,
  "Client Executable URL",
  },
{ SDP_ATTR_ICON_URL,
  "Icon URL",
  },
{ SDP_ATTR_ADDITIONAL_PROTOCOL_DESCRIPTOR_LISTS,
  "Additional Protocol Descriptor Lists" },
{ SDP_ATTR_GROUP_ID,
/*SDP_ATTR_IP_SUBNET,
  SDP_ATTR_VERSION_NUMBER_LIST*/
  "Group ID/IP Subnet/Version Number List",
  },
{ SDP_ATTR_SERVICE_DATABASE_STATE,
  "Service Database State",
  },
{ SDP_ATTR_SERVICE_VERSION,
  "Service Version",
  },
{ SDP_ATTR_EXTERNAL_NETWORK,
/*SDP_ATTR_NETWORK,
  SDP_ATTR_SUPPORTED_DATA_STORES_LIST*/
  "External Network/Network/Supported Data Stores List",
  },
{ SDP_ATTR_FAX_CLASS1_SUPPORT,
/*SDP_ATTR_REMOTE_AUDIO_VOLUME_CONTROL*/
  "Fax Class1 Support/Remote Audio Volume Control",
  },
{ SDP_ATTR_FAX_CLASS20_SUPPORT,
/*SDP_ATTR_SUPPORTED_FORMATS_LIST*/
  "Fax Class20 Support/Supported Formats List",
  },
{ SDP_ATTR_FAX_CLASS2_SUPPORT,
  "Fax Class2 Support",
  },
{ SDP_ATTR_AUDIO_FEEDBACK_SUPPORT,
  "Audio Feedback Support",
  },
{ SDP_ATTR_NETWORK_ADDRESS,
  "Network Address",
  },
{ SDP_ATTR_WAP_GATEWAY,
  "WAP Gateway",
  },
{ SDP_ATTR_HOME_PAGE_URL,
  "Home Page URL",
  },
{ SDP_ATTR_WAP_STACK_TYPE,
  "WAP Stack Type",
  },
{ SDP_ATTR_SECURITY_DESCRIPTION,
  "Security Description",
  },
{ SDP_ATTR_NET_ACCESS_TYPE,
  "Net Access Type",
  },
{ SDP_ATTR_MAX_NET_ACCESS_RATE,
  "Max Net Access Rate",
  },
{ SDP_ATTR_IPV4_SUBNET,
  "IPv4 Subnet",
  },
{ SDP_ATTR_IPV6_SUBNET,
  "IPv6 Subnet",
  },
{ SDP_ATTR_SUPPORTED_CAPABALITIES,
  "Supported Capabalities",
  },
{ SDP_ATTR_SUPPORTED_FEATURES,
  "Supported Features",
  },
{ SDP_ATTR_SUPPORTED_FUNCTIONS,
  "Supported Functions",
  },
{ SDP_ATTR_TOTAL_IMAGING_DATA_CAPACITY,
  "Total Imaging Data Capacity",
  },
{ 0xffff, NULL, }
};

char const *
sdp_attr2desc(uint16_t attr)
{
	register sdp_attr_desc_p	a = sdp_attrs_desc;

	for (; a->desc != NULL; a++)
		if (attr == a->attr)
			break;

	return ((a->desc != NULL)? a->desc : "Unknown");
}

char const *
sdp_uuid2desc(uint16_t uuid)
{
	register sdp_attr_desc_p	a = sdp_uuids_desc;

	for (; a->desc != NULL; a++)
		if (uuid == a->attr)
			break;

	return ((a->desc != NULL)? a->desc : "Unknown");
}

void
sdp_print(uint32_t level, uint8_t *start, uint8_t const *end)
{

	(void)_sdp_data_print(start, end, level);
}
