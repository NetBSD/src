/*	$NetBSD: print.c,v 1.4 2009/08/20 11:07:42 plunky Exp $	*/

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
__RCSID("$NetBSD: print.c,v 1.4 2009/08/20 11:07:42 plunky Exp $");

#include <ctype.h>
#include <iconv.h>
#include <langinfo.h>
#include <sdp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid.h>
#include <vis.h>

#include "sdpquery.h"

typedef struct {
	uint16_t	id;
	const char *	desc;
	void		(*print)(sdp_data_t *);
} attr_t;

typedef struct {
	uint16_t	class;
	const char *	desc;
	attr_t *	attrs;
	size_t		nattr;
} service_t;

typedef struct {
	uint16_t	base;
	const char *	codeset;
} language_t;

static const char *string_uuid(uuid_t *);
static const char *string_vis(int, const char *, size_t);

static void print_hexdump(const char *, const uint8_t *, size_t);
static bool print_attribute(uint16_t, sdp_data_t *, attr_t *, int);
static bool print_universal_attribute(uint16_t, sdp_data_t *);
static bool print_language_attribute(uint16_t, sdp_data_t *);
static bool print_service_attribute(uint16_t, sdp_data_t *);

static void print_bool(sdp_data_t *);
static void print_uint8d(sdp_data_t *);
static void print_uint8x(sdp_data_t *);
static void print_uint16d(sdp_data_t *);
static void print_uint16x(sdp_data_t *);
static void print_uint32x(sdp_data_t *);
static void print_uint32d(sdp_data_t *);
static void print_uuid(sdp_data_t *);
static void print_uuid_list(sdp_data_t *);
static void print_string(sdp_data_t *);
static void print_url(sdp_data_t *);
static void print_profile_version(sdp_data_t *);
static void print_language_string(sdp_data_t *);

static void print_service_class_id_list(sdp_data_t *);
static void print_protocol_descriptor(sdp_data_t *);
static void print_protocol_descriptor_list(sdp_data_t *);
static void print_language_base_attribute_id_list(sdp_data_t *);
static void print_service_availability(sdp_data_t *);
static void print_bluetooth_profile_descriptor_list(sdp_data_t *);
static void print_additional_protocol_descriptor_lists(sdp_data_t *);
static void print_sds_version_number_list(sdp_data_t *);
static void print_ct_network(sdp_data_t *);
static void print_asrc_features(sdp_data_t *);
static void print_asink_features(sdp_data_t *);
static void print_avrcp_features(sdp_data_t *);
static void print_supported_data_stores(sdp_data_t *);
static void print_supported_formats(sdp_data_t *);
static void print_hid_version(sdp_data_t *);
static void print_hid_device_subclass(sdp_data_t *);
static void print_hid_descriptor_list(sdp_data_t *);
static void print_security_description(sdp_data_t *);
static void print_hf_features(sdp_data_t *);
static void print_hfag_network(sdp_data_t *);
static void print_hfag_features(sdp_data_t *);
static void print_net_access_type(sdp_data_t *);
static void print_pnp_source(sdp_data_t *);
static void print_mas_types(sdp_data_t *);
static void print_supported_repositories(sdp_data_t *);

static void print_rfcomm(sdp_data_t *);
static void print_bnep(sdp_data_t *);
static void print_avctp(sdp_data_t *);
static void print_avdtp(sdp_data_t *);
static void print_l2cap(sdp_data_t *);

attr_t protocol_list[] = {
	{ 0x0001, "SDP",				NULL },
	{ 0x0002, "UDP",				NULL },
	{ 0x0003, "RFCOMM",				print_rfcomm },
	{ 0x0004, "TCP",				NULL },
	{ 0x0005, "TCS_BIN",				NULL },
	{ 0x0006, "TCS_AT",				NULL },
	{ 0x0008, "OBEX",				NULL },
	{ 0x0009, "IP",					NULL },
	{ 0x000a, "FTP",				NULL },
	{ 0x000c, "HTTP",				NULL },
	{ 0x000e, "WSP",				NULL },
	{ 0x000f, "BNEP",				print_bnep },
	{ 0x0010, "UPNP",				NULL },
	{ 0x0011, "HIDP",				NULL },
	{ 0x0012, "HARDCOPY_CONTROL_CHANNEL",		NULL },
	{ 0x0014, "HARDCOPY_DATA_CHANNEL",		NULL },
	{ 0x0016, "HARDCOPY_NOTIFICATION",		NULL },
	{ 0x0017, "AVCTP",				print_avctp },
	{ 0x0019, "AVDTP",				print_avdtp },
	{ 0x001b, "CMTP",				NULL },
	{ 0x001d, "UDI_C_PLANE",			NULL },
	{ 0x001e, "MCAP_CONTROL_CHANNEL",		NULL },
	{ 0x001f, "MCAP_DATA_CHANNEL",			NULL },
	{ 0x0100, "L2CAP",				print_l2cap },
};

attr_t universal_attrs[] = {
	{ 0x0000, "ServiceRecordHandle",		print_uint32x },
	{ 0x0001, "ServiceClassIDList",			print_service_class_id_list },
	{ 0x0002, "ServiceRecordState",			print_uint32x },
	{ 0x0003, "ServiceID",				print_uuid },
	{ 0x0004, "ProtocolDescriptorList",		print_protocol_descriptor_list },
	{ 0x0005, "BrowseGroupList",			print_uuid_list },
	{ 0x0006, "LanguageBaseAttributeIDList",	print_language_base_attribute_id_list },
	{ 0x0007, "ServiceInfoTimeToLive",		print_uint32d },
	{ 0x0008, "ServiceAvailability",		print_service_availability },
	{ 0x0009, "BluetoothProfileDescriptorList",	print_bluetooth_profile_descriptor_list },
	{ 0x000a, "DocumentationURL",			print_url },
	{ 0x000b, "ClientExecutableURL",		print_url },
	{ 0x000c, "IconURL",				print_url },
	{ 0x000d, "AdditionalProtocolDescriptorLists",	print_additional_protocol_descriptor_lists },
};

attr_t language_attrs[] = { /* Language Attribute Offsets */
	{ 0x0000, "ServiceName",			print_language_string },
	{ 0x0001, "ServiceDescription",			print_language_string },
	{ 0x0002, "ProviderName",			print_language_string },
};

attr_t sds_attrs[] = {	/* Service Discovery Server */
	{ 0x0200, "VersionNumberList",			print_sds_version_number_list },
	{ 0x0201, "ServiceDatabaseState",		print_uint32x },
};

attr_t bgd_attrs[] = {	/* Browse Group Descriptor */
	{ 0x0200, "GroupID",				print_uuid },
};

attr_t ct_attrs[] = { /* Cordless Telephony */
	{ 0x0301, "ExternalNetwork",			print_ct_network },
};

attr_t asrc_attrs[] = { /* Audio Source */
	{ 0x0311, "SupportedFeatures",			print_asrc_features },
};

attr_t asink_attrs[] = { /* Audio Sink */
	{ 0x0311, "SupportedFeatures",			print_asink_features },
};

attr_t avrcp_attrs[] = { /* Audio Video Remote Control Profile */
	{ 0x0311, "SupportedFeatures",			print_avrcp_features },
};

attr_t lan_attrs[] = {	/* LAN Access Using PPP */
	{ 0x0200, "IPSubnet",				print_string },
};

attr_t dun_attrs[] = {	/* Dialup Networking */
	{ 0x0305, "AudioFeedbackSupport",		print_bool },
};

attr_t irmc_sync_attrs[] = { /* IrMC Sync */
	{ 0x0301, "SupportedDataStoresList",		print_supported_data_stores },
};

attr_t opush_attrs[] = { /* Object Push */
	{ 0x0303, "SupportedFormatsList",		print_supported_formats },
};

attr_t hset_attrs[] = {	/* Headset */
	{ 0x0302, "RemoteAudioVolumeControl",		print_bool },
};

attr_t fax_attrs[] = {	/* Fax */
	{ 0x0302, "FAXClass1",				print_bool },
	{ 0x0303, "FAXClass2.0",			print_bool },
	{ 0x0304, "FAXClass2",				print_bool },
	{ 0x0305, "AudioFeedbackSupport",		print_bool },
};

attr_t panu_attrs[] = {	/* Personal Area Networking User */
	{ 0x030a, "SecurityDescription",		print_security_description },
};

attr_t nap_attrs[] = {	/* Network Access Point */
	{ 0x030a, "SecurityDescription",		print_security_description },
	{ 0x030b, "NetAccessType",			print_net_access_type },
	{ 0x030c, "MaxNetAccessRate",			print_uint32d },
	{ 0x030d, "IPv4Subnet",				print_string },
	{ 0x030e, "IPv6Subnet",				print_string },
};

attr_t gn_attrs[] = {	/* Group Network */
	{ 0x030a, "SecurityDescription",		print_security_description },
	{ 0x030d, "IPv4Subnet",				print_string },
	{ 0x030e, "IPv6Subnet",				print_string },
};

attr_t hf_attrs[] = {	/* Handsfree */
	{ 0x0311, "SupportedFeatures",			print_hf_features },
};

attr_t hfag_attrs[] = {	/* Handsfree Audio Gateway */
	{ 0x0301, "Network",				print_hfag_network },
	{ 0x0311, "SupportedFeatures",			print_hfag_features },
};

attr_t hid_attrs[] = {	/* Human Interface Device */
	{ 0x0200, "HIDDeviceReleaseNumber",		print_hid_version },
	{ 0x0201, "HIDParserVersion",			print_hid_version },
	{ 0x0202, "HIDDeviceSubClass",			print_hid_device_subclass },
	{ 0x0203, "HIDCountryCode",			print_uint8x },
	{ 0x0204, "HIDVirtualCable",			print_bool },
	{ 0x0205, "HIDReconnectInitiate",		print_bool },
	{ 0x0206, "HIDDescriptorList",			print_hid_descriptor_list },
	{ 0x0207, "HIDLANGIDBaseList",			NULL },
	{ 0x0208, "HIDSDPDisable",			print_bool },
	{ 0x0209, "HIDBatteryPower",			print_bool },
	{ 0x020a, "HIDRemoteWake",			print_bool },
	{ 0x020b, "HIDProfileVersion",			print_profile_version },
	{ 0x020c, "HIDSupervisionTimeout",		print_uint16d },
	{ 0x020d, "HIDNormallyConnectable",		print_bool },
	{ 0x020e, "HIDBootDevice",			print_bool },
};

attr_t pnp_attrs[] = {	/* Device ID */
	{ 0x0200, "SpecificationID",			print_profile_version },
	{ 0x0201, "VendorID",				print_uint16x },
	{ 0x0202, "ProductID",				print_uint16x },
	{ 0x0203, "Version",				print_hid_version },
	{ 0x0204, "PrimaryRecord",			print_bool },
	{ 0x0205, "VendorIDSource",			print_pnp_source },
};

attr_t mas_attrs[] = {	/* Message Access Server */
	{ 0x0315, "InstanceID",				print_uint8d },
	{ 0x0316, "SupportedMessageTypes",		print_mas_types },
};

attr_t pse_attrs[] = {	/* Phonebook Access Server */
	{ 0x0314, "SupportedRepositories",		print_supported_repositories },
};

#define A(a)	a, __arraycount(a)
service_t service_list[] = {
	{ 0x1000, "Service Discovery Server",		A(sds_attrs) },
	{ 0x1001, "Browse Group Descriptor",		A(bgd_attrs) },
	{ 0x1002, "Public Browse Root",			NULL, 0 },
	{ 0x1101, "Serial Port",			NULL, 0 },
	{ 0x1102, "LAN Access Using PPP",		A(lan_attrs) },
	{ 0x1103, "Dialup Networking",			A(dun_attrs) },
	{ 0x1104, "IrMC Sync",				A(irmc_sync_attrs) },
	{ 0x1105, "Object Push",			A(opush_attrs) },
	{ 0x1106, "File Transfer",			NULL, 0 },
	{ 0x1107, "IrMC Sync Command",			NULL, 0 },
	{ 0x1108, "Headset",				A(hset_attrs) },
	{ 0x1109, "Cordless Telephony",			A(ct_attrs) },
	{ 0x110a, "Audio Source",			A(asrc_attrs) },
	{ 0x110b, "Audio Sink",				A(asink_attrs) },
	{ 0x110c, "A/V Remote Control Target",		A(avrcp_attrs) },
	{ 0x110d, "Advanced Audio Distribution",	NULL, 0 },
	{ 0x110e, "A/V Remote Control",			A(avrcp_attrs) },
	{ 0x110f, "Video Conferencing",			NULL, 0 },
	{ 0x1110, "Intercom",				NULL, 0 },
	{ 0x1111, "Fax",				A(fax_attrs) },
	{ 0x1112, "Headset Audio Gateway",		NULL, 0 },
	{ 0x1113, "WAP",				NULL, 0 },
	{ 0x1114, "WAP Client",				NULL, 0 },
	{ 0x1115, "Personal Area Networking User",	A(panu_attrs) },
	{ 0x1116, "Network Access Point",		A(nap_attrs) },
	{ 0x1117, "Group Network",			A(gn_attrs) },
	{ 0x1118, "Direct Printing",			NULL, 0 },
	{ 0x1119, "Reference Printing",			NULL, 0 },
	{ 0x111a, "Imaging",				NULL, 0 },
	{ 0x111b, "Imaging Responder",			NULL, 0 },
	{ 0x111c, "Imaging Automatic Archive",		NULL, 0 },
	{ 0x111d, "Imaging Referenced Objects",		NULL, 0 },
	{ 0x111e, "Handsfree",				A(hf_attrs) },
	{ 0x111f, "Handsfree Audio Gateway",		A(hfag_attrs) },
	{ 0x1120, "Direct Printing Reference Objects",	NULL, 0 },
	{ 0x1121, "Reflected User Interface",		NULL, 0 },
	{ 0x1122, "Basic Printing",			NULL, 0 },
	{ 0x1123, "Printing Status",			NULL, 0 },
	{ 0x1124, "Human Interface Device",		A(hid_attrs) },
	{ 0x1125, "Hardcopy Cable Replacement",		NULL, 0 },
	{ 0x1126, "Hardcopy Cable Replacement Print",	NULL, 0 },
	{ 0x1127, "Hardcopy Cable Replacement Scan",	NULL, 0 },
	{ 0x1128, "Common ISDN Access",			NULL, 0 },
	{ 0x1129, "Video Conferencing GW",		NULL, 0 },
	{ 0x112a, "UDI MT",				NULL, 0 },
	{ 0x112b, "UDI TA",				NULL, 0 },
	{ 0x112c, "Audio/Video",			NULL, 0 },
	{ 0x112d, "SIM Access",				NULL, 0 },
	{ 0x112e, "Phonebook Access Client",		NULL, 0 },
	{ 0x112f, "Phonebook Access Server",		A(pse_attrs) },
	{ 0x1130, "Phonebook Access",			NULL, 0 },
	{ 0x1131, "Headset HS",				NULL, 0 },
	{ 0x1132, "Message Access Server",		A(mas_attrs) },
	{ 0x1133, "Message Notification Server",	NULL, 0 },
	{ 0x1134, "Message Access Profile",		NULL, 0 },
	{ 0x1200, "PNP Information",			A(pnp_attrs) },
	{ 0x1201, "Generic Networking",			NULL, 0 },
	{ 0x1202, "Generic File Transfer",		NULL, 0 },
	{ 0x1203, "Generic Audio",			NULL, 0 },
	{ 0x1204, "Generic Telephony",			NULL, 0 },
	{ 0x1205, "UPNP",				NULL, 0 },
	{ 0x1206, "UPNP IP",				NULL, 0 },
	{ 0x1300, "UPNP IP PAN",			NULL, 0 },
	{ 0x1301, "UPNP IP LAP",			NULL, 0 },
	{ 0x1302, "UPNP IP L2CAP",			NULL, 0 },
	{ 0x1303, "Video Source",			NULL, 0 },
	{ 0x1304, "Video Sink",				NULL, 0 },
	{ 0x1305, "Video Distribution",			NULL, 0 },
	{ 0x1400, "HDP",				NULL, 0 },
	{ 0x1401, "HDP Source",				NULL, 0 },
	{ 0x1402, "HDP Sink",				NULL, 0 },
};
#undef A

/* extracted Service Class ID List */
#define MAX_SERVICES		16
static size_t nservices;
static uint16_t service_class[MAX_SERVICES];

/* extracted Language Base Attribute ID List */
#define MAX_LANGUAGES		16
static int nlanguages;
static language_t language[MAX_LANGUAGES];
static int current;

static bool
sdp_get_uint8(sdp_data_t *d, uint8_t *vp)
{
	uintmax_t v;

	if (sdp_data_type(d) != SDP_DATA_UINT8
	    || !sdp_get_uint(d, &v))
		return false;

	*vp = (uint8_t)v;
	return true;
}

static bool
sdp_get_uint16(sdp_data_t *d, uint16_t *vp)
{
	uintmax_t v;

	if (sdp_data_type(d) != SDP_DATA_UINT16
	    || !sdp_get_uint(d, &v))
		return false;

	*vp = (uint16_t)v;
	return true;
}

static bool
sdp_get_uint32(sdp_data_t *d, uint32_t *vp)
{
	uintmax_t v;

	if (sdp_data_type(d) != SDP_DATA_UINT32
	    || !sdp_get_uint(d, &v))
		return false;

	*vp = (uint32_t)v;
	return true;
}

void
print_record(sdp_data_t *rec)
{
	sdp_data_t value;
	uint16_t id;

	nservices = 0;
	nlanguages = 0;
	current = -1;

	while (sdp_get_attr(rec, &id, &value)) {
		if (Xflag) {
			printf("AttributeID 0x%04x:\n", id);
			print_hexdump("     ", value.next, value.end - value.next);
		} else if (Rflag) {
			printf("AttributeID 0x%04x:\n", id);
			sdp_data_print(&value, 4);
		} else if (print_universal_attribute(id, &value)
		    || print_language_attribute(id, &value)
		    || print_service_attribute(id, &value)) {
			if (value.next != value.end)
			    printf("    [additional data ignored]\n");
		} else {
			printf("AttributeID 0x%04x:\n", id);
			sdp_data_print(&value, 4);
		}
	}
}

static const char *
string_uuid(uuid_t *uuid)
{
	static char buf[64];
	const char *desc;
	uuid_t u;
	size_t i;

	u = *uuid;
	u.time_low = 0;
	if (!uuid_equal(&u, &BLUETOOTH_BASE_UUID, NULL)) {
		snprintf(buf, sizeof(buf),
		    "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		    uuid->time_low, uuid->time_mid, uuid->time_hi_and_version,
		    uuid->clock_seq_hi_and_reserved, uuid->clock_seq_low,
		    uuid->node[0], uuid->node[1], uuid->node[2],
		    uuid->node[3], uuid->node[4], uuid->node[5]);

		return buf;
	}

	desc = NULL;
	for (i = 0; i < __arraycount(service_list); i++) {
		if (uuid->time_low == service_list[i].class) {
			desc = service_list[i].desc;
			break;
		}
	}

	for (i = 0; i < __arraycount(protocol_list); i++) {
		if (uuid->time_low == protocol_list[i].id) {
			desc = protocol_list[i].desc;
			break;
		}
	}

	if (!Nflag && desc) {
		snprintf(buf, sizeof(buf), "%s", desc);
		return buf;
	}

	snprintf(buf, sizeof(buf), "%s%s(0x%*.*x)",
	    (desc == NULL ? "" : desc),
	    (desc == NULL ? "" : " "),
	    (uuid->time_low > UINT16_MAX ? 8 : 4),
	    (uuid->time_low > UINT16_MAX ? 8 : 4),
	    uuid->time_low);

	return buf;
}

static const char *
string_vis(int style, const char *src, size_t len)
{
	static char buf[50];
	char *dst = buf;

	style |= VIS_NL;
	while (len > 0 && (dst + 5) < (buf + sizeof(buf))) {
		dst = vis(dst, src[0], style, (len > 1 ? src[1] : 0));
		src++;
		len--;
	}

	return buf;
}

static void
print_hexdump(const char *title, const uint8_t *data, size_t len)
{
	int n, i;

	i = 0;
	n = printf("%s", title);

	while (len-- > 0) {
		if (++i > 8) {
			printf("\n%*s", n, "");
			i = 1;
		}

		printf(" 0x%02x", *data++);
	}

	printf("\n");
}

static bool
print_attribute(uint16_t id, sdp_data_t *value, attr_t *attr, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (id == attr[i].id) {
			printf("%s", attr[i].desc);

			if (Nflag) {
				printf(" (");

				if (current != -1)
					printf("0x%04x + ", language[current].base);

				printf("0x%04x)", id);
			}

			printf(": ");

			if (attr[i].print == NULL) {
				printf("\n");
				sdp_data_print(value, 4);
				value->next = value->end;
			} else {
				(attr[i].print)(value);
			}

			return true;
		}
	}

	return false;
}

static bool
print_universal_attribute(uint16_t id, sdp_data_t *value)
{

	return print_attribute(id, value,
	    universal_attrs, __arraycount(universal_attrs));
}

static bool
print_language_attribute(uint16_t id, sdp_data_t *value)
{
	bool done = false;

	for (current = 0; current < nlanguages && !done; current++)
		done = print_attribute(id - language[current].base, value,
		    language_attrs, __arraycount(language_attrs));

	current = -1;
	return done;
}

static bool
print_service_attribute(uint16_t id, sdp_data_t *value)
{
	size_t i, j;

	for (i = 0; i < nservices; i++) {
		for (j = 0; j < __arraycount(service_list); j++) {
			if (service_class[i] == service_list[j].class)
				return print_attribute(id, value,
				    service_list[j].attrs,
				    service_list[j].nattr);
		}
	}

	return false;
}

static void
print_bool(sdp_data_t *data)
{
	bool v;

	if (!sdp_get_bool(data, &v))
		return;

	printf("%s\n", (v ? "true" : "false"));
}

static void
print_uint8d(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	printf("%d\n", v);
}

static void
print_uint8x(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	printf("0x%02x\n", v);
}

static void
print_uint16d(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	printf("%d\n", v);
}

static void
print_uint16x(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	printf("0x%04x\n", v);
}

static void
print_uint32x(sdp_data_t *data)
{
	uint32_t v;

	if (!sdp_get_uint32(data, &v))
		return;

	printf("0x%08x\n", v);
}

static void
print_uint32d(sdp_data_t *data)
{
	uint32_t v;

	if (!sdp_get_uint32(data, &v))
		return;

	printf("%d\n", v);
}

static void
print_uuid(sdp_data_t *data)
{
	uuid_t uuid;

	if (!sdp_get_uuid(data, &uuid))
		return;

	printf("%s\n", string_uuid(&uuid));
}

static void
print_uuid_list(sdp_data_t *data)
{
	sdp_data_t seq;
	uuid_t uuid;

	if (!sdp_get_seq(data, &seq))
		return;

	printf("\n");
	while (sdp_get_uuid(&seq, &uuid))
		printf("    %s\n", string_uuid(&uuid));

	if (seq.next != seq.end)
		printf("    [additional data]\n");
}

static void
print_string(sdp_data_t *data)
{
	char *str;
	size_t len;

	if (!sdp_get_str(data, &str, &len))
		return;

	printf("\"%s\"\n", string_vis(VIS_CSTYLE, str, len));
}

static void
print_url(sdp_data_t *data)
{
	char *url;
	size_t len;

	if (!sdp_get_url(data, &url, &len))
		return;

	printf("\"%s\"\n", string_vis(VIS_HTTPSTYLE, url, len));
}

static void
print_profile_version(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	printf("v%d.%d\n", (v >> 8), (v & 0xff));
}

/*
 * This should only be called through print_language_attribute() which
 * sets codeset of the string to be printed.
 */
static void
print_language_string(sdp_data_t *data)
{
	char buf[50], *dst, *src;
	iconv_t ih;
	size_t n, srcleft, dstleft;

	if (!sdp_get_str(data, &src, &srcleft))
		return;

	dst = buf;
	dstleft = sizeof(buf);

	ih = iconv_open(nl_langinfo(CODESET), language[current].codeset);
	if (ih == (iconv_t)-1) {
		printf("Can't convert %s string\n", language[current].codeset);
		return;
	}

	n = iconv(ih, (const char **)&src, &srcleft, &dst, &dstleft);

	iconv_close(ih);

	if (Nflag || n > 0)
		printf("(%s) ", language[current].codeset);

	printf("\"%.*s%s\n", (int)(sizeof(buf) - dstleft), buf,
	    (srcleft > 0 ? " ..." : "\""));
}

static void
print_service_class_id_list(sdp_data_t *data)
{
	sdp_data_t seq;
	uuid_t uuid;

	if (!sdp_get_seq(data, &seq))
		return;

	printf("\n");
	while (sdp_get_uuid(&seq, &uuid)) {
		printf("    %s\n", string_uuid(&uuid));

		if (nservices < MAX_SERVICES) {
			service_class[nservices] = uuid.time_low;
			uuid.time_low = 0;
			if (uuid_equal(&uuid, &BLUETOOTH_BASE_UUID, NULL))
				nservices++;
		}
	}

	if (seq.next != seq.end)
		printf("    [additional data]\n");
}

static void
print_protocol_descriptor(sdp_data_t *data)
{
	uuid_t u0, uuid;
	size_t i;

	if (!sdp_get_uuid(data, &uuid))
		return;

	u0 = uuid;
	u0.time_low = 0;
	if (uuid_equal(&u0, &BLUETOOTH_BASE_UUID, NULL)) {
		for (i = 0; i < __arraycount(protocol_list); i++) {
			if (uuid.time_low == protocol_list[i].id) {
				printf("    %s", protocol_list[i].desc);

				if (Nflag)
					printf(" (0x%04x)", protocol_list[i].id);

				if (protocol_list[i].print)
					(protocol_list[i].print)(data);

				if (data->next != data->end)
					printf(" [additional data ignored]");

				printf("\n");
				return;
			}
		}
	}

	printf("    %s\n", string_uuid(&uuid));
	sdp_data_print(data, 4);
	data->next = data->end;
}

static void
print_protocol_descriptor_list(sdp_data_t *data)
{
	sdp_data_t seq, proto;

	printf("\n");
	sdp_get_alt(data, data);	/* strip [optional] alt header */

	while (sdp_get_seq(data, &seq))
		while (sdp_get_seq(&seq, &proto))
			print_protocol_descriptor(&proto);
}

static void
print_language_base_attribute_id_list(sdp_data_t *data)
{
	sdp_data_t list;
	uint16_t v;
	const char *codeset;
	char lang[2];

	if (!sdp_get_seq(data, &list))
		return;

	printf("\n");
	while (list.next < list.end) {
		/*
		 * ISO-639-1 natural language values are published at
		 *	http://www.loc.gov/standards/iso639-2/php/code-list.php
		 */
		if (!sdp_get_uint16(&list, &v))
			break;

		be16enc(lang, v);
		if (!islower((int)lang[0]) || !islower((int)lang[1]))
			break;

		/*
		 * MIBenum values are published at
		 *	http://www.iana.org/assignments/character-sets
		 */
		if (!sdp_get_uint16(&list, &v))
			break;

		switch(v) {
		case 3:		codeset = "US-ASCII";		break;
		case 4:		codeset = "ISO-8859-1";		break;
		case 5:		codeset = "ISO-8859-2";		break;
		case 106:	codeset = "UTF-8";		break;
		case 1013:	codeset = "UTF-16BE";		break;
		case 1014:	codeset = "UTF-16LE";		break;
		default:	codeset = "Unknown";		break;
		}

		if (!sdp_get_uint16(&list, &v))
			break;

		printf("    %.2s.%s base 0x%04x\n", lang, codeset, v);

		if (nlanguages < MAX_LANGUAGES) {
			language[nlanguages].base = v;
			language[nlanguages].codeset = codeset;
			nlanguages++;
		}
	}

	if (list.next != list.end)
		printf("    [additional data]\n");
}

static void
print_service_availability(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	printf("%d/%d\n", v, UINT8_MAX);
}

static void
print_bluetooth_profile_descriptor_list(sdp_data_t *data)
{
	sdp_data_t seq, profile;
	uuid_t uuid;
	uint16_t v;

	if (!sdp_get_seq(data, &seq))
		return;

	printf("\n");
	while (seq.next < seq.end) {
		if (!sdp_get_seq(&seq, &profile)
		    || !sdp_get_uuid(&profile, &uuid)
		    || !sdp_get_uint16(&profile, &v))
			break;

		printf("    %s, v%d.%d", string_uuid(&uuid),
		    (v >> 8), (v & 0xff));

		if (profile.next != profile.end)
			printf(" [additional profile data]");

		printf("\n");
	}

	if (seq.next != seq.end)
		printf("    [additional data]\n");
}

static void
print_additional_protocol_descriptor_lists(sdp_data_t *data)
{
	sdp_data_t seq, stack, proto;

	printf("\n");
	sdp_get_seq(data, &seq);

	while (sdp_get_seq(&seq, &stack))
		while (sdp_get_seq(&stack, &proto))
			print_protocol_descriptor(&proto);

	if (seq.next != seq.end)
		printf("    [additional data]\n");
}

static void
print_sds_version_number_list(sdp_data_t *data)
{
	sdp_data_t list;
	const char *sep;
	uint16_t v;

	if (!sdp_get_seq(data, &list))
		return;

	sep = "";
	while (sdp_get_uint16(&list, &v)) {
		printf("%sv%d.%d", sep, (v >> 8), (v & 0xff));
		sep = ", ";
	}

	if (list.next != list.end)
		printf(" [additional data]");

	printf("\n");
}

static void
print_ct_network(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	switch (v) {
	case 0x01:	printf("PSTN");			break;
	case 0x02:	printf("ISDN");			break;
	case 0x03:	printf("GSM");			break;
	case 0x04:	printf("CDMA");			break;
	case 0x05:	printf("Analogue Cellular");	break;
	case 0x06:	printf("Packet Switched");	break;
	case 0x07:	printf("Other");		break;
	default:	printf("0x%02x", v);		break;
	}

	printf("\n");
}

static void
print_asrc_features(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	if (Nflag)
		printf("(0x%04x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    Player\n");
	if (v & (1<<1))	printf("    Microphone\n");
	if (v & (1<<2))	printf("    Tuner\n");
	if (v & (1<<3))	printf("    Mixer\n");
}

static void
print_asink_features(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	if (Nflag)
		printf("(0x%04x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    Headphone\n");
	if (v & (1<<1))	printf("    Speaker\n");
	if (v & (1<<2))	printf("    Recorder\n");
	if (v & (1<<3))	printf("    Amplifier\n");
}

static void
print_avrcp_features(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	if (Nflag)
		printf("(0x%04x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    Category 1\n");
	if (v & (1<<1))	printf("    Category 2\n");
	if (v & (1<<2))	printf("    Category 3\n");
	if (v & (1<<3))	printf("    Category 4\n");
}

static void
print_supported_data_stores(sdp_data_t *data)
{
	sdp_data_t list;
	const char *sep;
	uint8_t v;

	if (!sdp_get_seq(data, &list))
		return;

	sep = "\n    ";
	while (sdp_get_uint8(&list, &v)) {
		printf(sep);
		sep = ", ";

		switch(v) {
		case 0x01:	printf("Phonebook");	break;
		case 0x03:	printf("Calendar");	break;
		case 0x05:	printf("Notes");	break;
		case 0x06:	printf("Messages");	break;
		default:	printf("0x%02x", v);	break;
		}
	}

	if (list.next != list.end)
		printf("   [additional data]");

	printf("\n");
}

static void
print_supported_formats(sdp_data_t *data)
{
	sdp_data_t list;
	const char *sep;
	uint8_t v;

	if (!sdp_get_seq(data, &list))
		return;

	sep = "\n    ";
	while (sdp_get_uint8(&list, &v)) {
		printf(sep);
		sep = ", ";

		switch(v) {
		case 0x01:	printf("vCard 2.1");	break;
		case 0x02:	printf("vCard 3.0");	break;
		case 0x03:	printf("vCal 1.0");	break;
		case 0x04:	printf("iCal 2.0");	break;
		case 0x05:	printf("vNote");	break;
		case 0x06:	printf("vMessage");	break;
		case 0xff:	printf("Any");		break;
		default:	printf("0x%02x", v);	break;
		}
	}

	if (list.next != list.end)
		printf("   [additional data]");

	printf("\n");
}

static void
print_hid_version(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	printf("v%d.%d.%d\n",
	    ((v & 0xff00) >> 8), ((v & 0x00f0) >> 4), (v & 0x000f));
}

static void
print_hid_device_subclass(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	switch ((v & 0x3c) >> 2) {
	case 1:		printf("Joystick");		break;
	case 2:		printf("Gamepad");		break;
	case 3:		printf("Remote Control");	break;
	case 4:		printf("Sensing Device");	break;
	case 5:		printf("Digitiser Tablet");	break;
	case 6:		printf("Card Reader");		break;
	default:	printf("Peripheral");		break;
	}

	if (v & 0x40)	printf(" <Keyboard>");
	if (v & 0x80)	printf(" <Mouse>");

	printf("\n");
}

static void
print_hid_descriptor_list(sdp_data_t *data)
{
	sdp_data_t list, seq;
	uint8_t type;
	const char *name;
	char *str;
	size_t len;


	if (!sdp_get_seq(data, &list))
		return;

	printf("\n");
	while (list.next < list.end) {
		if (!sdp_get_seq(&list, &seq)
		    || !sdp_get_uint8(&seq, &type)
		    || !sdp_get_str(&seq, &str, &len))
			return;

		switch (type) {
		case 0x22:	name = "Report";		break;
		case 0x23:	name = "Physical Descriptor";	break;
		default:	name = "";			break;
		}

		printf("    Type 0x%02x: %s\n", type, name);
		print_hexdump("    Data", (uint8_t *)str, len);

		if (seq.next != seq.end)
			printf("    [additional data]\n");
	}
}

static void
print_security_description(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	switch (v) {
	case 0x0000:	printf("None");				break;
	case 0x0001:	printf("Service-level Security");	break;
	case 0x0002:	printf("802.1x Security");		break;
	default:	printf("0x%04x", v);			break;
	}

	printf("\n");
}

static void
print_hf_features(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	if (Nflag)
		printf("(0x%04x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    Echo Cancellation/Noise Reduction\n");
	if (v & (1<<1))	printf("    Call Waiting\n");
	if (v & (1<<2))	printf("    Caller Line Identification\n");
	if (v & (1<<3))	printf("    Voice Recognition\n");
	if (v & (1<<4))	printf("    Volume Control\n");
}

static void
print_hfag_network(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	switch (v) {
	case 0x01:	printf("Ability to reject a call");	break;
	case 0x02:	printf("No ability to reject a call");	break;
	default:	printf("0x%02x", v);			break;
	}

	printf("\n");
}

static void
print_hfag_features(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	if (Nflag)
		printf("(0x%04x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    3 Way Calling\n");
	if (v & (1<<1))	printf("    Echo Cancellation/Noise Reduction\n");
	if (v & (1<<2))	printf("    Voice Recognition\n");
	if (v & (1<<3))	printf("    In-band Ring Tone\n");
	if (v & (1<<4))	printf("    Voice Tags\n");
}

static void
print_net_access_type(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	switch(v) {
	case 0x0000:	printf("PSTN");			break;
	case 0x0001:	printf("ISDN");			break;
	case 0x0002:	printf("DSL");			break;
	case 0x0003:	printf("Cable Modem");		break;
	case 0x0004:	printf("10Mb Ethernet");	break;
	case 0x0005:	printf("100Mb Ethernet");	break;
	case 0x0006:	printf("4Mb Token Ring");	break;
	case 0x0007:	printf("16Mb Token Ring");	break;
	case 0x0008:	printf("100Mb Token Ring");	break;
	case 0x0009:	printf("FDDI");			break;
	case 0x000a:	printf("GSM");			break;
	case 0x000b:	printf("CDMA");			break;
	case 0x000c:	printf("GPRS");			break;
	case 0x000d:	printf("3G Cellular");		break;
	case 0xfffe:	printf("other");		break;
	default:	printf("0x%04x", v);		break;
	}

	printf("\n");
}

static void
print_pnp_source(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	switch (v) {
	case 0x0001:	printf("Bluetooth SIG");		break;
	case 0x0002:	printf("USB Implementers Forum");	break;
	default:	printf("0x%04x", v);			break;
	}

	printf("\n");
}

static void
print_mas_types(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	if (Nflag)
		printf("(0x%02x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    EMAIL\n");
	if (v & (1<<1))	printf("    SMS_GSM\n");
	if (v & (1<<2))	printf("    SMS_CDMA\n");
	if (v & (1<<3))	printf("    MMS\n");
}

static void
print_supported_repositories(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	if (Nflag)
		printf("(0x%02x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    Local Phonebook\n");
	if (v & (1<<1))	printf("    SIM Card\n");
}

static void
print_rfcomm(sdp_data_t *data)
{
	uint8_t v;

	if (sdp_get_uint8(data, &v))
		printf(" (channel %d)", v);
}

static void
print_bnep(sdp_data_t *data)
{
	sdp_data_t seq;
	uint16_t v;
	const char *sep;

	if (!sdp_get_uint16(data, &v)
	    || !sdp_get_seq(data, &seq))
		return;

	printf(" (v%d.%d", (v >> 8), (v & 0xff));
	sep = "; ";
	while (sdp_get_uint16(&seq, &v)) {
		printf(sep);
		sep = ", ";

		switch (v) {
		case 0x0800:	printf("IPv4");		break;
		case 0x0806:	printf("ARP");		break;
		case 0x86dd:	printf("IPv6");		break;
		default:	printf("0x%04x", v);	break;
		}
	}
	printf(")");

	if (seq.next != seq.end)
		printf(" [additional data]");
}

static void
print_avctp(sdp_data_t *data)
{
	uint16_t v;

	if (sdp_get_uint16(data, &v))
		printf(" (v%d.%d)", (v >> 8), (v & 0xff));
}

static void
print_avdtp(sdp_data_t *data)
{
	uint16_t v;

	if (sdp_get_uint16(data, &v))
		printf(" (v%d.%d)", (v >> 8), (v & 0xff));
}

static void
print_l2cap(sdp_data_t *data)
{
	uint16_t v;

	if (sdp_get_uint16(data, &v))
		printf(" (PSM 0x%04x)", v);
}
