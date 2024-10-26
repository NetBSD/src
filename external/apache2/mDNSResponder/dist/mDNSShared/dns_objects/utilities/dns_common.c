/*
 * Copyright (c) 2022-2024 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//======================================================================================================================
// MARK: - Headers

#include "dns_obj_log.h"
#include "dns_common.h"
#include "dns_sd.h"

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - Public Functions

uint16_t
get_uint16_from_bytes(const uint8_t * const bytes)
{
	return (uint16_t)((uint16_t)bytes[0] << 8 |
					  (uint16_t)bytes[1]);
}

//======================================================================================================================

uint32_t
get_uint32_from_bytes(const uint8_t * const bytes)
{
	return (uint32_t)((uint32_t)bytes[0] << 24	|
					  (uint32_t)bytes[1] << 16	|
					  (uint32_t)bytes[2] << 8	|
					  (uint32_t)bytes[3]);
}

//======================================================================================================================

static uint8_t *
_write_uint16_to_bytes(uint8_t *ptr, const uint16_t u16)
{
	*ptr++ = (uint8_t)((u16 >> 8)	& 0xFF);
	*ptr++ = (uint8_t)( u16			& 0xFF);
	return ptr;
}

void
put_uint16_to_bytes(const uint16_t u16, uint8_t ** const ptr)
{
	*ptr = _write_uint16_to_bytes(*ptr, u16);
}

//======================================================================================================================

static uint8_t *
_write_uint32_to_bytes(uint8_t *ptr, const uint32_t u32)
{
	*ptr++ = (uint8_t)((u32 >> 24)	& 0xFF);
	*ptr++ = (uint8_t)((u32 >> 16)	& 0xFF);
	*ptr++ = (uint8_t)((u32 >>  8)	& 0xFF);
	*ptr++ = (uint8_t)( u32			& 0xFF);
	return ptr;
}

void
put_uint32_to_bytes(const uint32_t u32, uint8_t ** const ptr)
{
	*ptr = _write_uint32_to_bytes(*ptr, u32);
}

//======================================================================================================================

char *
put_hex_from_bytes(const uint8_t * const bytes, const size_t len, char * const buffer, const size_t buffer_len)
{
	if (bytes == NULL || len == 0) {
		return buffer;
	}

	if (buffer_len <= len * 2) {
		return buffer;
	}

	char *ptr = buffer;
	const char * const limit = buffer + buffer_len;
	for (size_t i = 0; i < len; i++) {
		ptr += snprintf(ptr, (size_t)(limit - ptr), "%02X", bytes[i]);
	}

	return ptr;
}

//======================================================================================================================

const char *
dns_record_type_value_to_string(const uint16_t type)
{
	switch(type)
	{
		case kDNSRecordType_A:			return("A");
		case kDNSRecordType_NS:			return("NS");
		case kDNSRecordType_MD:			return("MD");
		case kDNSRecordType_MF:			return("MF");
		case kDNSRecordType_CNAME:		return("CNAME");
		case kDNSRecordType_SOA:		return("SOA");
		case kDNSRecordType_MB:			return("MB");
		case kDNSRecordType_MG:			return("MG");
		case kDNSRecordType_MR:			return("MR");
		case kDNSRecordType_NULL:		return("NULL");
		case kDNSRecordType_WKS:		return("WKS");
		case kDNSRecordType_PTR:		return("PTR");
		case kDNSRecordType_HINFO:		return("HINFO");
		case kDNSRecordType_MINFO:		return("MINFO");
		case kDNSRecordType_MX:			return("MX");
		case kDNSRecordType_TXT:		return("TXT");
		case kDNSRecordType_RP:			return("RP");
		case kDNSRecordType_AFSDB:		return("AFSDB");
		case kDNSRecordType_X25:		return("X25");
		case kDNSRecordType_ISDN:		return("ISDN");
		case kDNSRecordType_RT:			return("RT");
		case kDNSRecordType_NSAP:		return("NSAP");
		case kDNSRecordType_NSAP_PTR:	return("NSAP-PTR");
		case kDNSRecordType_SIG:		return("SIG");
		case kDNSRecordType_KEY:		return("KEY");
		case kDNSRecordType_PX:			return("PX");
		case kDNSRecordType_GPOS:		return("GPOS");
		case kDNSRecordType_AAAA:		return("AAAA");
		case kDNSRecordType_LOC:		return("LOC");
		case kDNSRecordType_NXT:		return("NXT");
		case kDNSRecordType_EID:		return("EID");
		case kDNSRecordType_NIMLOC:		return("NIMLOC");
		case kDNSRecordType_SRV:		return("SRV");
		case kDNSRecordType_ATMA:		return("ATMA");
		case kDNSRecordType_NAPTR:		return("NAPTR");
		case kDNSRecordType_KX:			return("KX");
		case kDNSRecordType_CERT:		return("CERT");
		case kDNSRecordType_A6:			return("A6");
		case kDNSRecordType_DNAME:		return("DNAME");
		case kDNSRecordType_SINK:		return("SINK");
		case kDNSRecordType_OPT:		return("OPT");
		case kDNSRecordType_APL:		return("APL");
		case kDNSRecordType_DS:			return("DS");
		case kDNSRecordType_SSHFP:		return("SSHFP");
		case kDNSRecordType_IPSECKEY:	return("IPSECKEY");
		case kDNSRecordType_RRSIG:		return("RRSIG");
		case kDNSRecordType_NSEC:		return("NSEC");
		case kDNSRecordType_DNSKEY:		return("DNSKEY");
		case kDNSRecordType_DHCID:		return("DHCID");
		case kDNSRecordType_NSEC3:		return("NSEC3");
		case kDNSRecordType_NSEC3PARAM:	return("NSEC3PARAM");
		case kDNSRecordType_TLSA:		return("TLSA");
		case kDNSRecordType_SMIMEA:		return("SMIMEA");
		case kDNSRecordType_HIP:		return("HIP");
		case kDNSRecordType_NINFO:		return("NINFO");
		case kDNSRecordType_RKEY:		return("RKEY");
		case kDNSRecordType_TALINK:		return("TALINK");
		case kDNSRecordType_CDS:		return("CDS");
		case kDNSRecordType_CDNSKEY:	return("CDNSKEY");
		case kDNSRecordType_OPENPGPKEY:	return("OPENPGPKEY");
		case kDNSRecordType_CSYNC:		return("CSYNC");
		case kDNSRecordType_ZONEMD:		return("ZONEMD");
		case kDNSRecordType_SVCB:		return("SVCB");
		case kDNSRecordType_HTTPS:		return("HTTPS");
		case kDNSRecordType_SPF:		return("SPF");
		case kDNSRecordType_UINFO:		return("UINFO");
		case kDNSRecordType_UID:		return("UID");
		case kDNSRecordType_GID:		return("GID");
		case kDNSRecordType_UNSPEC:		return("UNSPEC");
		case kDNSRecordType_NID:		return("NID");
		case kDNSRecordType_L32:		return("L32");
		case kDNSRecordType_L64:		return("L64");
		case kDNSRecordType_LP:			return("LP");
		case kDNSRecordType_EUI48:		return("EUI48");
		case kDNSRecordType_EUI64:		return("EUI64");
		case kDNSRecordType_TKEY:		return("TKEY");
		case kDNSRecordType_TSIG:		return("TSIG");
		case kDNSRecordType_IXFR:		return("IXFR");
		case kDNSRecordType_AXFR:		return("AXFR");
		case kDNSRecordType_MAILB:		return("MAILB");
		case kDNSRecordType_MAILA:		return("MAILA");
		case kDNSRecordType_ANY:		return("ANY");
		case kDNSRecordType_URI:		return("URI");
		case kDNSRecordType_CAA:		return("CAA");
		case kDNSRecordType_AVC:		return("AVC");
		case kDNSRecordType_DOA:		return("DOA");
		case kDNSRecordType_AMTRELAY:	return("AMTRELAY");
		case kDNSRecordType_TA:			return("TA");
		case kDNSRecordType_DLV:		return("DLV");
		case kDNSRecordType_Reserved:	return("Reserved");
		default:						return NULL;
	}
}

//======================================================================================================================

const char *
dns_obj_error_get_error_description(const dns_obj_error_t error)
{
	switch (error) {
		case DNS_OBJ_ERROR_NO_ERROR:
			return "No error.";
		default:
			return "DNS object error.";
	}
}
