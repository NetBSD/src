/*	$NetBSD: print.c,v 1.22 2015/12/11 21:05:18 plunky Exp $	*/

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
__RCSID("$NetBSD: print.c,v 1.22 2015/12/11 21:05:18 plunky Exp $");

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
static const char *string_vis(const char *, size_t);

static void print_hexdump(const char *, const uint8_t *, size_t);
static bool print_attribute(uint16_t, sdp_data_t *, attr_t *, size_t);
static bool print_universal_attribute(uint16_t, sdp_data_t *);
static bool print_language_attribute(uint16_t, sdp_data_t *);
static bool print_service_attribute(uint16_t, sdp_data_t *);

static void print_bool(sdp_data_t *);
static void print_uint8d(sdp_data_t *);
static void print_uint8x(sdp_data_t *);
static void print_uint16d(sdp_data_t *);
static void print_uint16x(sdp_data_t *);
static void print_uint32d(sdp_data_t *);
static void print_uint32x(sdp_data_t *);
static void print_uuid(sdp_data_t *);
static void print_uuid_list(sdp_data_t *);
static void print_string(sdp_data_t *);
static void print_string_list(sdp_data_t *);
static void print_url(sdp_data_t *);
static void print_profile_version(sdp_data_t *);
static void print_codeset_string(const char *, size_t, const char *);
static void print_language_string(sdp_data_t *);
static void print_utf8_string(sdp_data_t *);

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
static void print_wap_addr(sdp_data_t *);
static void print_wap_gateway(sdp_data_t *);
static void print_wap_type(sdp_data_t *);
static void print_hid_version(sdp_data_t *);
static void print_hid_device_subclass(sdp_data_t *);
static void print_hid_descriptor_list(sdp_data_t *);
static void print_hid_langid_base_list(sdp_data_t *);
static void print_security_description(sdp_data_t *);
static void print_hf_features(sdp_data_t *);
static void print_hfag_network(sdp_data_t *);
static void print_hfag_features(sdp_data_t *);
static void print_net_access_type(sdp_data_t *);
static void print_pnp_source(sdp_data_t *);
static void print_mas_types(sdp_data_t *);
static void print_map_features(sdp_data_t *);
static void print_pse_repositories(sdp_data_t *);
static void print_pse_features(sdp_data_t *);
static void print_hdp_features(sdp_data_t *);
static void print_hdp_specification(sdp_data_t *);
static void print_mcap_procedures(sdp_data_t *);
static void print_character_repertoires(sdp_data_t *);
static void print_bip_capabilities(sdp_data_t *);
static void print_bip_features(sdp_data_t *);
static void print_bip_functions(sdp_data_t *);
static void print_bip_capacity(sdp_data_t *);
static void print_1284id(sdp_data_t *);
static void print_ctn_features(sdp_data_t *);

static void print_rfcomm(sdp_data_t *);
static void print_att(sdp_data_t *);
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
	{ 0x0007, "ATT",				print_att },
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
	{ 0x0200, "GeopL2capPSM",			print_uint16x },
	{ 0x0303, "SupportedFormatsList",		print_supported_formats },
};

attr_t ft_attrs[] = { /* File Transfer */
	{ 0x0200, "GeopL2capPSM",			print_uint16x },
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

attr_t wap_attrs[] = {	/* WAP Bearer */
	{ 0x0306, "NetworkAddress",			print_wap_addr },
	{ 0x0307, "WAPGateway",				print_wap_gateway },
	{ 0x0308, "HomePageURL",			print_url },
	{ 0x0309, "WAPStackType",			print_wap_type },
};

attr_t panu_attrs[] = {	/* Personal Area Networking User */
	{ 0x0200, "IpSubnet",				print_string },
	{ 0x030a, "SecurityDescription",		print_security_description },
};

attr_t nap_attrs[] = {	/* Network Access Point */
	{ 0x0200, "IpSubnet",				print_string },
	{ 0x030a, "SecurityDescription",		print_security_description },
	{ 0x030b, "NetAccessType",			print_net_access_type },
	{ 0x030c, "MaxNetAccessRate",			print_uint32d },
	{ 0x030d, "IPv4Subnet",				print_string },
	{ 0x030e, "IPv6Subnet",				print_string },
};

attr_t gn_attrs[] = {	/* Group Network */
	{ 0x0200, "IpSubnet",				print_string },
	{ 0x030a, "SecurityDescription",		print_security_description },
	{ 0x030d, "IPv4Subnet",				print_string },
	{ 0x030e, "IPv6Subnet",				print_string },
};

attr_t bp_attrs[] = {	/* Basic Printing */
	{ 0x0350, "DocumentFormatsSupported",		print_string_list },
	{ 0x0352, "CharacterRepertoiresSupported",	print_character_repertoires },
	{ 0x0354, "XHTML-PrintImageFormatsSupported",	print_string_list },
	{ 0x0356, "ColorSupported",			print_bool },
	{ 0x0358, "1284ID",				print_1284id },
	{ 0x035a, "PrinterName",			print_utf8_string },
	{ 0x035c, "PrinterLocation",			print_utf8_string },
	{ 0x035e, "DuplexSupported",			print_bool },
	{ 0x0360, "MediaTypesSupported",		print_string_list },
	{ 0x0362, "MaxMediaWidth",			print_uint16d },
	{ 0x0364, "MaxMediaLength",			print_uint16d },
	{ 0x0366, "EnhancedLayoutSupport",		print_bool },
	{ 0x0368, "RUIFormatsSupported",		print_string_list },
	{ 0x0370, "ReferencePrintingRUISupported",	print_bool },
	{ 0x0372, "DirectPrintingRUISupported",		print_bool },
	{ 0x0374, "ReferencePrintingTopURL",		print_url },
	{ 0x0376, "DirectPrintingTopURL",		print_url },
	{ 0x037a, "DeviceName",				print_utf8_string },
};

attr_t bi_attrs[] = {	/* Basic Imaging */
	{ 0x0200, "GeopL2capPSM",			print_uint16x },
	{ 0x0310, "SupportedCapabilities",		print_bip_capabilities },
	{ 0x0311, "SupportedFeatures",			print_bip_features },
	{ 0x0312, "SupportedFunctions",			print_bip_functions },
	{ 0x0313, "TotalImagingDataCapacity",		print_bip_capacity },
};

attr_t hf_attrs[] = {	/* Handsfree */
	{ 0x0311, "SupportedFeatures",			print_hf_features },
};

attr_t hfag_attrs[] = {	/* Handsfree Audio Gateway */
	{ 0x0301, "Network",				print_hfag_network },
	{ 0x0311, "SupportedFeatures",			print_hfag_features },
};

attr_t rui_attrs[] = {	/* Reflected User Interface */
	{ 0x0368, "RUIFormatsSupported",		print_string_list },
	{ 0x0378, "PrinterAdminRUITopURL",		print_url },
};

attr_t hid_attrs[] = {	/* Human Interface Device */
	{ 0x0200, "HIDDeviceReleaseNumber",		print_hid_version },
	{ 0x0201, "HIDParserVersion",			print_hid_version },
	{ 0x0202, "HIDDeviceSubClass",			print_hid_device_subclass },
	{ 0x0203, "HIDCountryCode",			print_uint8x },
	{ 0x0204, "HIDVirtualCable",			print_bool },
	{ 0x0205, "HIDReconnectInitiate",		print_bool },
	{ 0x0206, "HIDDescriptorList",			print_hid_descriptor_list },
	{ 0x0207, "HIDLANGIDBaseList",			print_hid_langid_base_list },
	{ 0x0208, "HIDSDPDisable",			print_bool },
	{ 0x0209, "HIDBatteryPower",			print_bool },
	{ 0x020a, "HIDRemoteWake",			print_bool },
	{ 0x020b, "HIDProfileVersion",			print_profile_version },
	{ 0x020c, "HIDSupervisionTimeout",		print_uint16d },
	{ 0x020d, "HIDNormallyConnectable",		print_bool },
	{ 0x020e, "HIDBootDevice",			print_bool },
	{ 0x020f, "HIDHostMaxLatency",			print_uint16d },
	{ 0x0210, "HIDHostMinTimeout",			print_uint16d },
};

attr_t hcr_attrs[] = {	/* Hardcopy Cable Replacement */
	{ 0x0300, "1284ID",				print_1284id },
	{ 0x0302, "DeviceName",				print_utf8_string },
	{ 0x0304, "FriendlyName",			print_utf8_string },
	{ 0x0306, "DeviceLocation",			print_utf8_string },
};

attr_t mps_attrs[] = {	/* Multi-Profile Specification */
	{ 0x0200, "SingleDeviceSupportedScenarios",	NULL },
	{ 0x0201, "MultiDeviceSupportedScenarios",	NULL },
	{ 0x0202, "SupportedProfileAndProtocolDependencies", print_uint16x },
};

attr_t cas_attrs[] = { /* Calendar, Tasks & Notes Access */
	{ 0x0315, "InstanceID",				print_uint8d },
	{ 0x0317, "SupportedFeatures",			print_ctn_features },
};

attr_t cns_attrs[] = { /* Calendar, Tasks & Notes Notification */
	{ 0x0317, "SupportedFeatures",			print_ctn_features },
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
	{ 0x0200, "GeopL2capPSM",			print_uint16x },
	{ 0x0315, "InstanceID",				print_uint8d },
	{ 0x0316, "SupportedMessageTypes",		print_mas_types },
	{ 0x0317, "SupportedFeatures",			print_map_features },
};

attr_t mns_attrs[] = {	/* Message Notification Server */
	{ 0x0200, "GeopL2capPSM",			print_uint16x },
	{ 0x0317, "SupportedFeatures",			print_map_features },
};

attr_t gnss_attrs[] = {	/* Global Navigation Satellite System Server */
	{ 0x0200, "SupportedFeatures",			print_uint16x },
};

attr_t pse_attrs[] = {	/* Phonebook Access Server */
	{ 0x0200, "GeopL2capPSM",			print_uint16x },
	{ 0x0314, "SupportedRepositories",		print_pse_repositories },
	{ 0x0317, "SupportedFeatures",			print_pse_features },
};

attr_t hdp_attrs[] = {	/* Health Device Profile */
	{ 0x0200, "SupportedFeaturesList",		print_hdp_features },
	{ 0x0301, "DataExchangeSpecification",		print_hdp_specification },
	{ 0x0302, "MCAPSupportedProcedures",		print_mcap_procedures },
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
	{ 0x1106, "File Transfer",			A(ft_attrs) },
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
	{ 0x1113, "WAP",				A(wap_attrs) },
	{ 0x1114, "WAP Client",				NULL, 0 },
	{ 0x1115, "Personal Area Networking User",	A(panu_attrs) },
	{ 0x1116, "Network Access Point",		A(nap_attrs) },
	{ 0x1117, "Group Network",			A(gn_attrs) },
	{ 0x1118, "Direct Printing",			A(bp_attrs) },
	{ 0x1119, "Reference Printing",			A(bp_attrs) },
	{ 0x111a, "Imaging",				NULL, 0 },
	{ 0x111b, "Imaging Responder",			A(bi_attrs) },
	{ 0x111c, "Imaging Automatic Archive",		A(bi_attrs) },
	{ 0x111d, "Imaging Referenced Objects",		A(bi_attrs) },
	{ 0x111e, "Handsfree",				A(hf_attrs) },
	{ 0x111f, "Handsfree Audio Gateway",		A(hfag_attrs) },
	{ 0x1120, "Direct Printing Reference Objects",	NULL, 0 },
	{ 0x1121, "Reflected User Interface",		A(rui_attrs) },
	{ 0x1122, "Basic Printing",			NULL, 0 },
	{ 0x1123, "Printing Status",			A(bp_attrs) },
	{ 0x1124, "Human Interface Device",		A(hid_attrs) },
	{ 0x1125, "Hardcopy Cable Replacement",		NULL, 0 },
	{ 0x1126, "Hardcopy Cable Replacement Print",	A(hcr_attrs) },
	{ 0x1127, "Hardcopy Cable Replacement Scan",	A(hcr_attrs) },
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
	{ 0x1133, "Message Notification Server",	A(mns_attrs) },
	{ 0x1134, "Message Access Profile",		NULL, 0 },
	{ 0x1135, "Global Navigation Satellite System Profile", NULL, 0 },
	{ 0x1136, "Global Navigation Satellite System Server", A(gnss_attrs) },
	{ 0x1137, "3D Display",				NULL, 0 },
	{ 0x1138, "3D Glasses",				NULL, 0 },
	{ 0x1139, "3D Synchronization",			NULL, 0 },
	{ 0x113a, "Multi-Profile Specification Profile",NULL, 0 },
	{ 0x113b, "Multi-Profile Specification Server", A(mps_attrs) },
	{ 0x113c, "Calendar, Tasks & Notes Access",	A(cas_attrs) },
	{ 0x113d, "Calendar, Tasks & Notes Notification",A(cns_attrs) },
	{ 0x113e, "Calendar, Tasks & Notes Profile",	NULL, 0 },
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
	{ 0x1401, "HDP Source",				A(hdp_attrs) },
	{ 0x1402, "HDP Sink",				A(hdp_attrs) },
	{ 0x1800, "Generic Access Profile",		NULL, 0 },
	{ 0x1801, "Generic Attribute Server",		NULL, 0 },
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

static bool
sdp_get_uint64(sdp_data_t *d, uint64_t *vp)
{
	uintmax_t v;

	if (sdp_data_type(d) != SDP_DATA_UINT64
	    || !sdp_get_uint(d, &v))
		return false;

	*vp = (uint64_t)v;
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
			print_hexdump("     ", value.next,
			    (size_t)(value.end - value.next));
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
string_vis(const char *src, size_t len)
{
	static char buf[50];
	char *dst = buf;
	int style;

	buf[0] = '\0';
	style = VIS_CSTYLE | VIS_NL;
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
print_attribute(uint16_t id, sdp_data_t *value, attr_t *attr, size_t count)
{
	size_t i;

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
			if (service_class[i] == service_list[j].class
			    && print_attribute(id, value,
			    service_list[j].attrs, service_list[j].nattr))
				return true;
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

	printf("\"%s\"\n", string_vis(str, len));
}

static void
print_string_list(sdp_data_t *data)
{
	char *str, *ep;
	size_t len, l;

	if (!sdp_get_str(data, &str, &len))
		return;

	printf("\n");
	while (len > 0) {
		ep = memchr(str, (int)',', len);
		if (ep == NULL) {
			l = len;
			len = 0;
		} else {
			l = (size_t)(ep - str);
			len -= l + 1;
			ep++;
		}
		printf("    %s\n", string_vis(str, l));
		str = ep;
	}
}

static void
print_url(sdp_data_t *data)
{
	char *url;
	size_t len;

	if (!sdp_get_url(data, &url, &len))
		return;

	printf("\"%s\"\n", string_vis(url, len));
}

static void
print_profile_version(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	printf("v%d.%d\n", (v >> 8), (v & 0xff));
}

static void
print_codeset_string(const char *src, size_t srclen, const char *codeset)
{
	char buf[50], *dst;
	iconv_t ih;
	size_t dstlen;

	dst = buf;
	dstlen = sizeof(buf);

	ih = iconv_open(nl_langinfo(CODESET), codeset);
	if (ih == (iconv_t)-1) {
		printf("Can't convert %s string\n", codeset);
		return;
	}

	(void)iconv(ih, &src, &srclen, &dst, &dstlen);

	iconv_close(ih);

	printf("\"%.*s%s\n", (int)(sizeof(buf) - dstlen), buf,
	    (srclen > 0 ? " ..." : "\""));
}

/*
 * This should only be called through print_language_attribute() which
 * sets codeset of the string to be printed.
 */
static void
print_language_string(sdp_data_t *data)
{
	char *str;
	size_t len;

	if (!sdp_get_str(data, &str, &len))
		return;

	print_codeset_string(str, len, language[current].codeset);
}


static void
print_utf8_string(sdp_data_t *data)
{
	char *str;
	size_t len;

	if (!sdp_get_str(data, &str, &len))
		return;

	print_codeset_string(str, len, "UTF-8");
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
					printf(" [additional data]");

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

	while (sdp_get_seq(data, &seq)) {
		while (sdp_get_seq(&seq, &proto))
			print_protocol_descriptor(&proto);

		if (seq.next != seq.end)
			printf("    [additional protocol data]\n");
	}

	if (data->next != data->end)
		printf("    [additional data]\n");
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
		printf("%s", sep);
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
		printf("%s", sep);
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
print_wap_addr(sdp_data_t *data)
{
	uint32_t v;

	if (!sdp_get_uint32(data, &v))
		return;

	printf("%d.%d.%d.%d\n",
	    ((v & 0xff000000) >> 24), ((v & 0x00ff0000) >> 16),
	    ((v & 0x0000ff00) >> 8), (v & 0x000000ff));
}

static void
print_wap_gateway(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	switch(v) {
	case 0x01:	printf("Origin Server\n");	break;
	case 0x02:	printf("Proxy\n");		break;
	default:	printf("0x%02x\n", v);		break;
	}
}

static void
print_wap_type(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	switch(v) {
	case 0x01:	printf("Connectionless\n");	break;
	case 0x02:	printf("Connection Oriented\n");break;
	case 0x03:	printf("Both\n");		break;
	default:	printf("0x%02x\n", v);		break;
	}
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
print_hid_langid_base_list(sdp_data_t *data)
{
	sdp_data_t list, seq;
	uint16_t lang, base;

	if (!sdp_get_seq(data, &list))
		return;

	while (list.next < list.end) {
		if (!sdp_get_seq(&list, &seq)
		    || !sdp_get_uint16(&seq, &lang)
		    || !sdp_get_uint16(&seq, &base))
			return;

		printf("\n    ");
		/*
		 * The language is encoded according to the
		 *   "Universal Serial Bus Language Identifiers (LANGIDs)"
		 * specification. It does not seem worth listing them all
		 * here, but feel free to add if you notice any being used.
		 */
		switch (lang) {
		case 0x0409:	printf("English (US)");		break;
		case 0x0809:	printf("English (UK)");		break;
		default:	printf("0x%04x", lang);		break;
		}

		printf(" base 0x%04x%s\n", base,
		    (seq.next == seq.end ? "" : " [additional data]"));
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
print_map_features(sdp_data_t *data)
{
	uint32_t v;

	if (!sdp_get_uint32(data, &v))
		return;

	if (Nflag)
		printf("(0x%08x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    Notification Registration\n");
	if (v & (1<<1))	printf("    Notification\n");
	if (v & (1<<2))	printf("    Browsing\n");
	if (v & (1<<3))	printf("    Uploading\n");
	if (v & (1<<4))	printf("    Delete\n");
	if (v & (1<<5))	printf("    Instance Information\n");
	if (v & (1<<6))	printf("    Extended Event Report 1.1\n");
}

static void
print_pse_repositories(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	if (Nflag)
		printf("(0x%02x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    Local Phonebook\n");
	if (v & (1<<1))	printf("    SIM Card\n");
	if (v & (1<<2))	printf("    Speed Dial\n");
	if (v & (1<<3))	printf("    Favorites\n");
}

static void
print_pse_features(sdp_data_t *data)
{
	uint32_t v;

	if (!sdp_get_uint32(data, &v))
		return;

	if (Nflag)
		printf("(0x%08x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    Download\n");
	if (v & (1<<1))	printf("    Browsing\n");
	if (v & (1<<2))	printf("    Database Identifier\n");
	if (v & (1<<3))	printf("    Folder Version Counters\n");
	if (v & (1<<4))	printf("    vCard Selecting\n");
	if (v & (1<<5))	printf("    Enhanced Missed Calls\n");
	if (v & (1<<6))	printf("    X-BT-UCI vCard Property\n");
	if (v & (1<<7))	printf("    X-BT-UID vCard Property\n");
	if (v & (1<<8))	printf("    Contact Referencing\n");
	if (v & (1<<9))	printf("    Default Contact Image Format\n");
}

static void
print_hdp_features(sdp_data_t *data)
{
	sdp_data_t seq, feature;
	char *str;
	size_t len;
	uint16_t type;
	uint8_t id, role;

	if (!sdp_get_seq(data, &seq))
		return;

	printf("\n");
	while (sdp_get_seq(&seq, &feature)) {
		if (!sdp_get_uint8(&feature, &id)
		    || !sdp_get_uint16(&feature, &type)
		    || !sdp_get_uint8(&feature, &role))
			break;

		printf("    # %d: ", id);

		switch(type) {
		case 0x1004:	printf("Pulse Oximeter"); break;
		case 0x1006:	printf("Basic ECG"); break;
		case 0x1007:	printf("Blood Pressure Monitor"); break;
		case 0x1008:	printf("Body Thermometer"); break;
		case 0x100F:	printf("Body Weight Scale"); break;
		case 0x1011:	printf("Glucose Meter"); break;
		case 0x1012:	printf("International Normalized Ratio Monitor"); break;
		case 0x1014:	printf("Body Composition Analyzer"); break;
		case 0x1015:	printf("Peak Flow Monitor"); break;
		case 0x1029:	printf("Cardiovascular Fitness and Activity Monitor"); break;
		case 0x1068:	printf("Step Counter"); break;
		case 0x102A:	printf("Strength Fitness Equipment"); break;
		case 0x1047:	printf("Independent Living Activity Hub"); break;
		case 0x1075:	printf("Fall Sensor"); break;
		case 0x1076:	printf("Personal Emergency Response Sensor"); break;
		case 0x1077:	printf("Smoke Sensor"); break;
		case 0x1078:	printf("Carbon Monoxide Sensor"); break;
		case 0x1079:	printf("Water Sensor"); break;
		case 0x107A:	printf("Gas Sensor"); break;
		case 0x107B:	printf("Motion Sensor"); break;
		case 0x107C:	printf("Property Exit Sensor"); break;
		case 0x107D:	printf("Enuresis Sensor"); break;
		case 0x107E:	printf("Contact Closure Sensor"); break;
		case 0x107F:	printf("Usage Sensor"); break;
		case 0x1080:	printf("Switch Sensor"); break;
		case 0x1081:	printf("Medication Dosing Sensor"); break;
		case 0x1082:	printf("Temperature Sensor"); break;
		case 0x1048:	printf("Medication monitor"); break;
		default:	printf("Type 0x%04x", type);	break;
		}

		switch(role) {
		case 0x00:	printf(" [Source]");		break;
		case 0x01:	printf(" [Sink]");		break;
		default:	printf(" [Role 0x%02x]", role);	break;
		}

		printf("\n");

		if (sdp_get_str(&feature, &str, &len)) {
			int n;

			/* This optional human-readable description should
			 * be in the primary language encoding, which ought
			 * to have a base of 0x0100 or if there isn't one,
			 * use the first encoding listed
			 */
			for (n = 0; n < nlanguages; n++) {
				if (language[n].base == 0x0100)
					break;
			}

			printf("    # %d: ", id);
			if (n < nlanguages)
				print_codeset_string(str, len, language[n].codeset);
			else if (n > 0)
				print_codeset_string(str, len, language[0].codeset);
			else
				printf("%s", string_vis(str, len));

			printf("\n");
		}

		if (feature.next != feature.end)
			printf("    [additional data in feature]\n");
	}

	if (seq.next != seq.end)
		printf("    [additional data]\n");
}

static void
print_hdp_specification(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	switch(v) {
	case 0x01:	printf("ISO/IEEE 11073-20601\n");	break;
	default:	printf("0x%02x\n", v);			break;
	}
}

static void
print_mcap_procedures(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	if (Nflag)
		printf("(0x%02x)", v);

	printf("\n");
	if (v & (1<<1))	printf("    Reconnect Initiation\n");
	if (v & (1<<2))	printf("    Reconnect Acceptance\n");
	if (v & (1<<3))	printf("    Clock Synchronization Protocol\n");
	if (v & (1<<4))	printf("    Sync-Master Role\n");
}

static void
print_character_repertoires(sdp_data_t *data)
{
	uintmax_t v;

	/*
	 * we have no uint128 type so use uintmax as only
	 * only 17-bits are currently defined, and if the
	 * value is out of bounds it will be printed anyway
	 */
	if (sdp_data_type(data) != SDP_DATA_UINT128
	    || !sdp_get_uint(data, &v))
		return;

	if (Nflag)
		printf("(0x%016jx)", v);

	printf("\n");
	if (v & (1<< 0)) printf("    ISO-8859-1\n");
	if (v & (1<< 1)) printf("    ISO-8859-2\n");
	if (v & (1<< 2)) printf("    ISO-8859-3\n");
	if (v & (1<< 3)) printf("    ISO-8859-4\n");
	if (v & (1<< 4)) printf("    ISO-8859-5\n");
	if (v & (1<< 5)) printf("    ISO-8859-6\n");
	if (v & (1<< 6)) printf("    ISO-8859-7\n");
	if (v & (1<< 7)) printf("    ISO-8859-8\n");
	if (v & (1<< 8)) printf("    ISO-8859-9\n");
	if (v & (1<< 9)) printf("    ISO-8859-10\n");
	if (v & (1<<10)) printf("    ISO-8859-13\n");
	if (v & (1<<11)) printf("    ISO-8859-14\n");
	if (v & (1<<12)) printf("    ISO-8859-15\n");
	if (v & (1<<13)) printf("    GB18030\n");
	if (v & (1<<14)) printf("    JIS X0208-1990, JIS X0201-1976\n");
	if (v & (1<<15)) printf("    KSC 5601-1992\n");
	if (v & (1<<16)) printf("    Big5\n");
	if (v & (1<<17)) printf("    TIS-620\n");
}

static void
print_bip_capabilities(sdp_data_t *data)
{
	uint8_t v;

	if (!sdp_get_uint8(data, &v))
		return;

	if (Nflag)
		printf("(0x%02x)", v);

	printf("\n");
	if (v & (1<< 0)) printf("    Generic imaging\n");
	if (v & (1<< 1)) printf("    Capturing\n");
	if (v & (1<< 2)) printf("    Printing\n");
	if (v & (1<< 3)) printf("    Displaying\n");
}

static void
print_bip_features(sdp_data_t *data)
{
	uint16_t v;

	if (!sdp_get_uint16(data, &v))
		return;

	if (Nflag)
		printf("(0x%04x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    ImagePush\n");
	if (v & (1<<1))	printf("    ImagePush-Store\n");
	if (v & (1<<2))	printf("    ImagePush-Print\n");
	if (v & (1<<3))	printf("    ImagePush-Display\n");
	if (v & (1<<4))	printf("    ImagePull\n");
	if (v & (1<<5))	printf("    AdvancedImagePrinting\n");
	if (v & (1<<6))	printf("    AutomaticArchive\n");
	if (v & (1<<7))	printf("    RemoteCamera\n");
	if (v & (1<<8))	printf("    RemoteDisplay\n");
}

static void
print_bip_functions(sdp_data_t *data)
{
	uint32_t v;

	if (!sdp_get_uint32(data, &v))
		return;

	if (Nflag)
		printf("(0x%08x)", v);

	printf("\n");
	if (v & (1<< 0)) printf("    GetCapabilities\n");
	if (v & (1<< 1)) printf("    PutImage\n");
	if (v & (1<< 2)) printf("    PutLinkedAttachment\n");
	if (v & (1<< 3)) printf("    PutLinkedThumbnail\n");
	if (v & (1<< 4)) printf("    RemoteDisplay\n");
	if (v & (1<< 5)) printf("    GetImagesList\n");
	if (v & (1<< 6)) printf("    GetImageProperties\n");
	if (v & (1<< 7)) printf("    GetImage\n");
	if (v & (1<< 8)) printf("    GetLinkedThumbnail\n");
	if (v & (1<< 9)) printf("    GetLinkedAttachment\n");
	if (v & (1<<10)) printf("    DeleteImage\n");
	if (v & (1<<11)) printf("    StartPrint\n");
	if (v & (1<<12)) printf("    GetPartialImage\n");
	if (v & (1<<13)) printf("    StartArchive\n");
	if (v & (1<<14)) printf("    GetMonitoringImage\n");
	if (v & (1<<16)) printf("    GetStatus\n");
}

static void
print_bip_capacity(sdp_data_t *data)
{
	char buf[9];
	uint64_t v;

	if (!sdp_get_uint64(data, &v))
		return;

	if (v > INT64_MAX) {
		printf("more than ");
		v = INT64_MAX;
	}

	(void)humanize_number(buf, sizeof(buf), (int64_t)v,
	    "bytes", HN_AUTOSCALE, HN_NOSPACE);

	printf("%s\n", buf);
}

static void
print_1284id(sdp_data_t *data)
{
	char *str, *ep;
	size_t len, l;

	if (!sdp_get_str(data, &str, &len))
		return;

	if (len < 2 || len != be16dec(str)) {
		printf("[invalid IEEE 1284 Device ID]\n");
		return;
	}

	str += 2;
	len -= 2;

	printf("\n");
	while (len > 0) {
		ep = memchr(str, (int)';', len);
		if (ep == NULL) {
			printf("[invalid IEEE 1284 Device ID]\n");
			return;
		}

		l = (size_t)(ep - str + 1);
		printf("    %s\n", string_vis(str, l));
		str += l;
		len -= l;
	}
}

static void
print_ctn_features(sdp_data_t *data)
{
	uint32_t v;

	if (!sdp_get_uint32(data, &v))
		return;

	if (Nflag)
		printf("(0x%08x)", v);

	printf("\n");
	if (v & (1<<0))	printf("    Account Management\n");
	if (v & (1<<1))	printf("    Notification\n");
	if (v & (1<<2))	printf("    Browsing\n");
	if (v & (1<<3))	printf("    Downloading\n");
	if (v & (1<<4))	printf("    Uploading\n");
	if (v & (1<<5))	printf("    Delete\n");
	if (v & (1<<6))	printf("    Forward\n");
}

static void
print_rfcomm(sdp_data_t *data)
{
	uint8_t v;

	if (sdp_get_uint8(data, &v))
		printf(" (channel %d)", v);
}

static void
print_att(sdp_data_t *data)
{
	uint16_t s, e;

	if (sdp_get_uint16(data, &s) && sdp_get_uint16(data, &e))
		printf(" (0x%04x .. 0x%04x)", s, e);
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
		printf("%s", sep);
		sep = ", ";

		switch (v) {
		case 0x0800:	printf("IPv4");		break;
		case 0x0806:	printf("ARP");		break;
		case 0x8100:	printf("802.1Q");	break;
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
