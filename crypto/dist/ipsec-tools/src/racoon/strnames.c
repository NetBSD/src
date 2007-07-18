/*	$NetBSD: strnames.c,v 1.8 2007/07/18 12:07:52 vanhu Exp $	*/

/*	$KAME: strnames.c,v 1.25 2003/11/13 10:53:26 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h> 
#include PATH_IPSEC_H
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#ifdef ENABLE_HYBRID
#include <resolv.h>
#endif

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"

#include "isakmp_var.h"
#include "isakmp.h"
#ifdef ENABLE_HYBRID
#  include "isakmp_xauth.h"
#  include "isakmp_unity.h"
#  include "isakmp_cfg.h"
#endif
#include "ipsec_doi.h"
#include "oakley.h"
#include "handler.h"
#include "pfkey.h"
#include "strnames.h"
#include "algorithm.h"

struct ksmap {
	int key;
	char *str;
	char *(*f) __P((int));
};

char *
num2str(n)
	int n;
{
	static char buf[20];

	snprintf(buf, sizeof(buf), "%d", n);

	return buf;
}

/* isakmp.h */
char *
s_isakmp_state(t, d, s)
	int t, d, s;
{
	switch (t) {
	case ISAKMP_ETYPE_AGG:
		switch (d) {
		case INITIATOR:
			switch (s) {
			case PHASE1ST_MSG1SENT:
				return "agg I msg1";
			case PHASE1ST_ESTABLISHED:
				return "agg I msg2";
			default:
				break;
			}
		case RESPONDER:
			switch (s) {
			case PHASE1ST_MSG1SENT:
				return "agg R msg1";
			default:
				break;
			}
		}
		break;
	case ISAKMP_ETYPE_BASE:
		switch (d) {
		case INITIATOR:
			switch (s) {
			case PHASE1ST_MSG1SENT:
				return "base I msg1";
			case PHASE1ST_MSG2SENT:
				return "base I msg2";
			default:
				break;
			}
		case RESPONDER:
			switch (s) {
			case PHASE1ST_MSG1SENT:
				return "base R msg1";
			case PHASE1ST_ESTABLISHED:
				return "base R msg2";
			default:
				break;
			}
		}
		break;
	case ISAKMP_ETYPE_IDENT:
		switch (d) {
		case INITIATOR:
			switch (s) {
			case PHASE1ST_MSG1SENT:
				return "ident I msg1";
			case PHASE1ST_MSG2SENT:
				return "ident I msg2";
			case PHASE1ST_MSG3SENT:
				return "ident I msg3";
			default:
				break;
			}
		case RESPONDER:
			switch (s) {
			case PHASE1ST_MSG1SENT:
				return "ident R msg1";
			case PHASE1ST_MSG2SENT:
				return "ident R msg2";
			case PHASE1ST_ESTABLISHED:
				return "ident R msg3";
			default:
				break;
			}
		}
		break;
	case ISAKMP_ETYPE_QUICK:
		switch (d) {
		case INITIATOR:
			switch (s) {
			case PHASE2ST_MSG1SENT:
				return "quick I msg1";
			case PHASE2ST_ADDSA:
				return "quick I msg2";
			default:
				break;
			}
		case RESPONDER:
			switch (s) {
			case PHASE2ST_MSG1SENT:
				return "quick R msg1";
			case PHASE2ST_COMMIT:
				return "quick R msg2";
			default:
				break;
			}
		}
		break;
	default:
	case ISAKMP_ETYPE_NONE:
	case ISAKMP_ETYPE_AUTH:
	case ISAKMP_ETYPE_INFO:
	case ISAKMP_ETYPE_NEWGRP:
	case ISAKMP_ETYPE_ACKINFO:
		break;
	}
	/*NOTREACHED*/

	return "???";
}

static struct ksmap name_isakmp_certtype[] = {
{ ISAKMP_CERT_NONE,	"NONE",					NULL },
{ ISAKMP_CERT_PKCS7,	"PKCS #7 wrapped X.509 certificate",	NULL },
{ ISAKMP_CERT_PGP,	"PGP Certificate",			NULL },
{ ISAKMP_CERT_DNS,	"DNS Signed Key",			NULL },
{ ISAKMP_CERT_X509SIGN,	"X.509 Certificate Signature",		NULL },
{ ISAKMP_CERT_X509KE,	"X.509 Certificate Key Exchange",	NULL },
{ ISAKMP_CERT_KERBEROS,	"Kerberos Tokens",			NULL },
{ ISAKMP_CERT_CRL,	"Certificate Revocation List (CRL)",	NULL },
{ ISAKMP_CERT_ARL,	"Authority Revocation List (ARL)",	NULL },
{ ISAKMP_CERT_SPKI,	"SPKI Certificate",			NULL },
{ ISAKMP_CERT_X509ATTR,	"X.509 Certificate Attribute",		NULL },
};

char *
s_isakmp_certtype(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_isakmp_certtype); i++)
		if (name_isakmp_certtype[i].key == k)
			return name_isakmp_certtype[i].str;
	return num2str(k);
}

static struct ksmap name_isakmp_etype[] = {
{ ISAKMP_ETYPE_NONE,	"None",			NULL },
{ ISAKMP_ETYPE_BASE,	"Base",			NULL },
{ ISAKMP_ETYPE_IDENT,	"Identity Protection",	NULL },
{ ISAKMP_ETYPE_AUTH,	"Authentication Only",	NULL },
{ ISAKMP_ETYPE_AGG,	"Aggressive",		NULL },
{ ISAKMP_ETYPE_INFO,	"Informational",	NULL },
{ ISAKMP_ETYPE_CFG,	"Mode config",		NULL },
{ ISAKMP_ETYPE_QUICK,	"Quick",		NULL },
{ ISAKMP_ETYPE_NEWGRP,	"New Group",		NULL },
{ ISAKMP_ETYPE_ACKINFO,	"Acknowledged Informational",	NULL },
};

char *
s_isakmp_etype(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_isakmp_etype); i++)
		if (name_isakmp_etype[i].key == k)
			return name_isakmp_etype[i].str;
	return num2str(k);
}

static struct ksmap name_isakmp_notify_msg[] = {
{ ISAKMP_NTYPE_INVALID_PAYLOAD_TYPE,	"INVALID-PAYLOAD-TYPE",		NULL },
{ ISAKMP_NTYPE_DOI_NOT_SUPPORTED,	"DOI-NOT-SUPPORTED",		NULL },
{ ISAKMP_NTYPE_SITUATION_NOT_SUPPORTED,	"SITUATION-NOT-SUPPORTED",	NULL },
{ ISAKMP_NTYPE_INVALID_COOKIE,		"INVALID-COOKIE",		NULL },
{ ISAKMP_NTYPE_INVALID_MAJOR_VERSION,	"INVALID-MAJOR-VERSION",	NULL },
{ ISAKMP_NTYPE_INVALID_MINOR_VERSION,	"INVALID-MINOR-VERSION",	NULL },
{ ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE,	"INVALID-EXCHANGE-TYPE",	NULL },
{ ISAKMP_NTYPE_INVALID_FLAGS,		"INVALID-FLAGS",		NULL },
{ ISAKMP_NTYPE_INVALID_MESSAGE_ID,	"INVALID-MESSAGE-ID",		NULL },
{ ISAKMP_NTYPE_INVALID_PROTOCOL_ID,	"INVALID-PROTOCOL-ID",		NULL },
{ ISAKMP_NTYPE_INVALID_SPI,		"INVALID-SPI",			NULL },
{ ISAKMP_NTYPE_INVALID_TRANSFORM_ID,	"INVALID-TRANSFORM-ID",		NULL },
{ ISAKMP_NTYPE_ATTRIBUTES_NOT_SUPPORTED, "ATTRIBUTES-NOT-SUPPORTED",	NULL },
{ ISAKMP_NTYPE_NO_PROPOSAL_CHOSEN,	"NO-PROPOSAL-CHOSEN",		NULL },
{ ISAKMP_NTYPE_BAD_PROPOSAL_SYNTAX,	"BAD-PROPOSAL-SYNTAX",		NULL },
{ ISAKMP_NTYPE_PAYLOAD_MALFORMED,	"PAYLOAD-MALFORMED",		NULL },
{ ISAKMP_NTYPE_INVALID_KEY_INFORMATION,	"INVALID-KEY-INFORMATION",	NULL },
{ ISAKMP_NTYPE_INVALID_ID_INFORMATION,	"INVALID-ID-INFORMATION",	NULL },
{ ISAKMP_NTYPE_INVALID_CERT_ENCODING,	"INVALID-CERT-ENCODING",	NULL },
{ ISAKMP_NTYPE_INVALID_CERTIFICATE,	"INVALID-CERTIFICATE",		NULL },
{ ISAKMP_NTYPE_BAD_CERT_REQUEST_SYNTAX,	"BAD-CERT-REQUEST-SYNTAX",	NULL },
{ ISAKMP_NTYPE_INVALID_CERT_AUTHORITY,	"INVALID-CERT-AUTHORITY",	NULL },
{ ISAKMP_NTYPE_INVALID_HASH_INFORMATION, "INVALID-HASH-INFORMATION",	NULL },
{ ISAKMP_NTYPE_AUTHENTICATION_FAILED,	"AUTHENTICATION-FAILED",	NULL },
{ ISAKMP_NTYPE_INVALID_SIGNATURE,	"INVALID-SIGNATURE",		NULL },
{ ISAKMP_NTYPE_ADDRESS_NOTIFICATION,	"ADDRESS-NOTIFICATION",		NULL },
{ ISAKMP_NTYPE_NOTIFY_SA_LIFETIME,	"NOTIFY-SA-LIFETIME",		NULL },
{ ISAKMP_NTYPE_CERTIFICATE_UNAVAILABLE,	"CERTIFICATE-UNAVAILABLE",	NULL },
{ ISAKMP_NTYPE_UNSUPPORTED_EXCHANGE_TYPE, "UNSUPPORTED-EXCHANGE-TYPE",	NULL },
{ ISAKMP_NTYPE_UNEQUAL_PAYLOAD_LENGTHS,	"UNEQUAL-PAYLOAD-LENGTHS",	NULL },
{ ISAKMP_NTYPE_CONNECTED,		"CONNECTED",			NULL },
{ ISAKMP_NTYPE_RESPONDER_LIFETIME,	"RESPONDER-LIFETIME",		NULL },
{ ISAKMP_NTYPE_REPLAY_STATUS,		"REPLAY-STATUS",		NULL },
{ ISAKMP_NTYPE_INITIAL_CONTACT,		"INITIAL-CONTACT",		NULL },
#ifdef ENABLE_HYBRID
{ ISAKMP_NTYPE_UNITY_HEARTBEAT,		"HEARTBEAT (Unity)",		NULL },
#endif
{ ISAKMP_LOG_RETRY_LIMIT_REACHED,	"RETRY-LIMIT-REACHED",		NULL },
};

char *
s_isakmp_notify_msg(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_isakmp_notify_msg); i++)
		if (name_isakmp_notify_msg[i].key == k)
			return name_isakmp_notify_msg[i].str;

	return num2str(k);
}

static struct ksmap name_isakmp_nptype[] = {
{ ISAKMP_NPTYPE_NONE,		"none",		NULL },
{ ISAKMP_NPTYPE_SA,		"sa",		NULL },
{ ISAKMP_NPTYPE_P,		"prop",		NULL },
{ ISAKMP_NPTYPE_T,		"trns",		NULL },
{ ISAKMP_NPTYPE_KE,		"ke",		NULL },
{ ISAKMP_NPTYPE_ID,		"id",		NULL },
{ ISAKMP_NPTYPE_CERT,		"cert",		NULL },
{ ISAKMP_NPTYPE_CR,		"cr",		NULL },
{ ISAKMP_NPTYPE_HASH,		"hash",		NULL },
{ ISAKMP_NPTYPE_SIG,		"sig",		NULL },
{ ISAKMP_NPTYPE_NONCE,		"nonce",	NULL },
{ ISAKMP_NPTYPE_N,		"notify",	NULL },
{ ISAKMP_NPTYPE_D,		"delete",	NULL },
{ ISAKMP_NPTYPE_VID,		"vid",		NULL },
{ ISAKMP_NPTYPE_ATTR,		"attr",		NULL },
{ ISAKMP_NPTYPE_GSS,		"gss id",	NULL },
{ ISAKMP_NPTYPE_NATD_RFC,	"nat-d",	NULL },
{ ISAKMP_NPTYPE_NATOA_RFC,	"nat-oa",	NULL },
{ ISAKMP_NPTYPE_NATD_DRAFT,	"nat-d",	NULL },
{ ISAKMP_NPTYPE_NATOA_DRAFT,	"nat-oa",	NULL },
{ ISAKMP_NPTYPE_FRAG,		"ike frag",	NULL },
};

char *
s_isakmp_nptype(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_isakmp_nptype); i++)
		if (name_isakmp_nptype[i].key == k)
			return name_isakmp_nptype[i].str;
	return num2str(k);
}

#ifdef ENABLE_HYBRID
/* isakmp_cfg.h / isakmp_unity.h / isakmp_xauth.h */
static struct ksmap name_isakmp_cfg_type[] = {
{ INTERNAL_IP4_ADDRESS,		"INTERNAL_IP4_ADDRESS",		NULL },
{ INTERNAL_IP4_NETMASK,		"INTERNAL_IP4_NETMASK",		NULL },
{ INTERNAL_IP4_DNS,		"INTERNAL_IP4_DNS",		NULL },
{ INTERNAL_IP4_NBNS,		"INTERNAL_IP4_NBNS",		NULL },
{ INTERNAL_ADDRESS_EXPIRY,	"INTERNAL_ADDRESS_EXPIRY",	NULL },
{ INTERNAL_IP4_DHCP,		"INTERNAL_IP4_DHCP",		NULL },
{ APPLICATION_VERSION,		"APPLICATION_VERSION",		NULL },
{ INTERNAL_IP6_ADDRESS,		"INTERNAL_IP6_ADDRESS",		NULL },
{ INTERNAL_IP6_NETMASK,		"INTERNAL_IP6_NETMASK",		NULL },
{ INTERNAL_IP6_DNS,		"INTERNAL_IP6_DNS",		NULL },
{ INTERNAL_IP6_NBNS,		"INTERNAL_IP6_NBNS",		NULL },
{ INTERNAL_IP6_DHCP,		"INTERNAL_IP6_DHCP",		NULL },
{ INTERNAL_IP4_SUBNET,		"INTERNAL_IP4_SUBNET",		NULL },
{ SUPPORTED_ATTRIBUTES,		"SUPPORTED_ATTRIBUTES",		NULL },
{ INTERNAL_IP6_SUBNET,		"INTERNAL_IP6_SUBNET",		NULL },
{ XAUTH_TYPE,			"XAUTH_TYPE",			NULL },
{ XAUTH_USER_NAME,		"XAUTH_USER_NAME",		NULL },
{ XAUTH_USER_PASSWORD,		"XAUTH_USER_PASSWORD",		NULL },
{ XAUTH_PASSCODE,		"XAUTH_PASSCODE",		NULL },
{ XAUTH_MESSAGE,		"XAUTH_MESSAGE",		NULL },
{ XAUTH_CHALLENGE,		"XAUTH_CHALLENGE",		NULL },
{ XAUTH_DOMAIN,			"XAUTH_DOMAIN",			NULL },
{ XAUTH_STATUS,			"XAUTH_STATUS",			NULL },
{ XAUTH_NEXT_PIN,		"XAUTH_NEXT_PIN",		NULL },
{ XAUTH_ANSWER,			"XAUTH_ANSWER",			NULL },
{ UNITY_BANNER,			"UNITY_BANNER",			NULL },
{ UNITY_SAVE_PASSWD,		"UNITY_SAVE_PASSWD",		NULL },
{ UNITY_DEF_DOMAIN,		"UNITY_DEF_DOMAIN",		NULL },
{ UNITY_SPLITDNS_NAME,		"UNITY_SPLITDNS_NAME",		NULL },
{ UNITY_SPLIT_INCLUDE,		"UNITY_SPLIT_INCLUDE",		NULL },
{ UNITY_NATT_PORT,		"UNITY_NATT_PORT",		NULL },
{ UNITY_LOCAL_LAN,		"UNITY_LOCAL_LAN",		NULL },
{ UNITY_PFS,			"UNITY_PFS",			NULL },
{ UNITY_FW_TYPE,		"UNITY_FW_TYPE",		NULL },
{ UNITY_BACKUP_SERVERS,		"UNITY_BACKUP_SERVERS",		NULL },
{ UNITY_DDNS_HOSTNAME,		"UNITY_DDNS_HOSTNAME",		NULL },
};

char *
s_isakmp_cfg_type(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_isakmp_cfg_type); i++)
		if (name_isakmp_cfg_type[i].key == k)
			return name_isakmp_cfg_type[i].str;
	return num2str(k);
}

/* isakmp_cfg.h / isakmp_unity.h / isakmp_xauth.h */
static struct ksmap name_isakmp_cfg_ptype[] = {
{ ISAKMP_CFG_ACK,		"mode config ACK",		NULL },
{ ISAKMP_CFG_SET,		"mode config SET",		NULL },
{ ISAKMP_CFG_REQUEST,		"mode config REQUEST",		NULL },
{ ISAKMP_CFG_REPLY,		"mode config REPLY",		NULL },
};

char *
s_isakmp_cfg_ptype(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_isakmp_cfg_ptype); i++)
		if (name_isakmp_cfg_ptype[i].key == k)
			return name_isakmp_cfg_ptype[i].str;
	return num2str(k);
}

#endif

/* ipsec_doi.h */
static struct ksmap name_ipsecdoi_proto[] = {
{ IPSECDOI_PROTO_ISAKMP,	"ISAKMP",	s_ipsecdoi_trns_isakmp },
{ IPSECDOI_PROTO_IPSEC_AH,	"AH",		s_ipsecdoi_trns_ah },
{ IPSECDOI_PROTO_IPSEC_ESP,	"ESP",		s_ipsecdoi_trns_esp },
{ IPSECDOI_PROTO_IPCOMP,	"IPCOMP",	s_ipsecdoi_trns_ipcomp },
};

char *
s_ipsecdoi_proto(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_ipsecdoi_proto); i++)
		if (name_ipsecdoi_proto[i].key == k)
			return name_ipsecdoi_proto[i].str;
	return num2str(k);
}

static struct ksmap name_ipsecdoi_trns_isakmp[] = {
{ IPSECDOI_KEY_IKE,	"IKE", NULL },
};

char *
s_ipsecdoi_trns_isakmp(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_ipsecdoi_trns_isakmp); i++)
		if (name_ipsecdoi_trns_isakmp[i].key == k)
			return name_ipsecdoi_trns_isakmp[i].str;
	return num2str(k);
}

static struct ksmap name_ipsecdoi_trns_ah[] = {
{ IPSECDOI_AH_MD5,	"MD5", NULL },
{ IPSECDOI_AH_SHA,	"SHA", NULL },
{ IPSECDOI_AH_DES,	"DES", NULL },
{ IPSECDOI_AH_SHA256,	"SHA256", NULL },
{ IPSECDOI_AH_SHA384,	"SHA384", NULL },
{ IPSECDOI_AH_SHA512,	"SHA512", NULL },
};

char *
s_ipsecdoi_trns_ah(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_ipsecdoi_trns_ah); i++)
		if (name_ipsecdoi_trns_ah[i].key == k)
			return name_ipsecdoi_trns_ah[i].str;
	return num2str(k);
}

static struct ksmap name_ipsecdoi_trns_esp[] = {
{ IPSECDOI_ESP_DES_IV64,	"DES_IV64",	NULL },
{ IPSECDOI_ESP_DES,		"DES",		NULL },
{ IPSECDOI_ESP_3DES,		"3DES",		NULL },
{ IPSECDOI_ESP_RC5,		"RC5",		NULL },
{ IPSECDOI_ESP_IDEA,		"IDEA",		NULL },
{ IPSECDOI_ESP_CAST,		"CAST",		NULL },
{ IPSECDOI_ESP_BLOWFISH,	"BLOWFISH",	NULL },
{ IPSECDOI_ESP_3IDEA,		"3IDEA",	NULL },
{ IPSECDOI_ESP_DES_IV32,	"DES_IV32",	NULL },
{ IPSECDOI_ESP_RC4,		"RC4",		NULL },
{ IPSECDOI_ESP_NULL,		"NULL",		NULL },
{ IPSECDOI_ESP_AES,		"AES",		NULL },
{ IPSECDOI_ESP_TWOFISH,		"TWOFISH",	NULL },
{ IPSECDOI_ESP_CAMELLIA,	"CAMELLIA",	NULL },
};

char *
s_ipsecdoi_trns_esp(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_ipsecdoi_trns_esp); i++)
		if (name_ipsecdoi_trns_esp[i].key == k)
			return name_ipsecdoi_trns_esp[i].str;
	return num2str(k);
}

static struct ksmap name_ipsecdoi_trns_ipcomp[] = {
{ IPSECDOI_IPCOMP_OUI,		"OUI",		NULL},
{ IPSECDOI_IPCOMP_DEFLATE,	"DEFLATE",	NULL},
{ IPSECDOI_IPCOMP_LZS,		"LZS",		NULL},
};

char *
s_ipsecdoi_trns_ipcomp(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_ipsecdoi_trns_ipcomp); i++)
		if (name_ipsecdoi_trns_ipcomp[i].key == k)
			return name_ipsecdoi_trns_ipcomp[i].str;
	return num2str(k);
}

char *
s_ipsecdoi_trns(proto, trns)
	int proto, trns;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_ipsecdoi_proto); i++)
		if (name_ipsecdoi_proto[i].key == proto
		 && name_ipsecdoi_proto[i].f)
			return (name_ipsecdoi_proto[i].f)(trns);
	return num2str(trns);
}

static struct ksmap name_attr_ipsec[] = {
{ IPSECDOI_ATTR_SA_LD_TYPE,	"SA Life Type",		s_ipsecdoi_ltype },
{ IPSECDOI_ATTR_SA_LD,		"SA Life Duration",	NULL },
{ IPSECDOI_ATTR_GRP_DESC,	"Group Description",	NULL },
{ IPSECDOI_ATTR_ENC_MODE,	"Encryption Mode",	s_ipsecdoi_encmode },
{ IPSECDOI_ATTR_AUTH,		"Authentication Algorithm", s_ipsecdoi_auth },
{ IPSECDOI_ATTR_KEY_LENGTH,	"Key Length",		NULL },
{ IPSECDOI_ATTR_KEY_ROUNDS,	"Key Rounds",		NULL },
{ IPSECDOI_ATTR_COMP_DICT_SIZE,	"Compression Dictionary Size",	NULL },
{ IPSECDOI_ATTR_COMP_PRIVALG,	"Compression Private Algorithm", NULL },
};

char *
s_ipsecdoi_attr(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_attr_ipsec); i++)
		if (name_attr_ipsec[i].key == k)
			return name_attr_ipsec[i].str;
	return num2str(k);
}

static struct ksmap name_attr_ipsec_ltype[] = {
{ IPSECDOI_ATTR_SA_LD_TYPE_SEC,	"seconds",	NULL },
{ IPSECDOI_ATTR_SA_LD_TYPE_KB,	"kilobytes",	NULL },
};

char *
s_ipsecdoi_ltype(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_attr_ipsec_ltype); i++)
		if (name_attr_ipsec_ltype[i].key == k)
			return name_attr_ipsec_ltype[i].str;
	return num2str(k);
}

static struct ksmap name_attr_ipsec_encmode[] = {
{ IPSECDOI_ATTR_ENC_MODE_ANY,		"Any",		NULL },
{ IPSECDOI_ATTR_ENC_MODE_TUNNEL,	"Tunnel",	NULL },
{ IPSECDOI_ATTR_ENC_MODE_TRNS,		"Transport",	NULL },
{ IPSECDOI_ATTR_ENC_MODE_UDPTUNNEL_RFC,	"UDP-Tunnel",	NULL },
{ IPSECDOI_ATTR_ENC_MODE_UDPTRNS_RFC,	"UDP-Transport",	NULL },
{ IPSECDOI_ATTR_ENC_MODE_UDPTUNNEL_DRAFT,	"UDP-Tunnel",	NULL },
{ IPSECDOI_ATTR_ENC_MODE_UDPTRNS_DRAFT,	"UDP-Transport",	NULL },
};

char *
s_ipsecdoi_encmode(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_attr_ipsec_encmode); i++)
		if (name_attr_ipsec_encmode[i].key == k)
			return name_attr_ipsec_encmode[i].str;
	return num2str(k);
}

static struct ksmap name_attr_ipsec_auth[] = {
{ IPSECDOI_ATTR_AUTH_HMAC_MD5,		"hmac-md5",	NULL },
{ IPSECDOI_ATTR_AUTH_HMAC_SHA1,		"hmac-sha",	NULL },
{ IPSECDOI_ATTR_AUTH_HMAC_SHA2_256,	"hmac-sha256",	NULL },
{ IPSECDOI_ATTR_AUTH_HMAC_SHA2_384,	"hmac-sha384",	NULL },
{ IPSECDOI_ATTR_AUTH_HMAC_SHA2_512,	"hmac-sha512",	NULL },
{ IPSECDOI_ATTR_AUTH_DES_MAC,		"des-mac",	NULL },
{ IPSECDOI_ATTR_AUTH_KPDK,		"kpdk",		NULL },
};

char *
s_ipsecdoi_auth(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_attr_ipsec_auth); i++)
		if (name_attr_ipsec_auth[i].key == k)
			return name_attr_ipsec_auth[i].str;
	return num2str(k);
}

char *
s_ipsecdoi_attr_v(type, val)
	int type, val;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_attr_ipsec); i++)
		if (name_attr_ipsec[i].key == type
		 && name_attr_ipsec[i].f)
			return (name_attr_ipsec[i].f)(val);
	return num2str(val);
}

static struct ksmap name_ipsecdoi_ident[] = {
{ IPSECDOI_ID_IPV4_ADDR,	"IPv4_address",	NULL },
{ IPSECDOI_ID_FQDN,		"FQDN",		NULL },
{ IPSECDOI_ID_USER_FQDN,	"User_FQDN",	NULL },
{ IPSECDOI_ID_IPV4_ADDR_SUBNET,	"IPv4_subnet",	NULL },
{ IPSECDOI_ID_IPV6_ADDR,	"IPv6_address",	NULL },
{ IPSECDOI_ID_IPV6_ADDR_SUBNET,	"IPv6_subnet",	NULL },
{ IPSECDOI_ID_IPV4_ADDR_RANGE,	"IPv4_address_range",	NULL },
{ IPSECDOI_ID_IPV6_ADDR_RANGE,	"IPv6_address_range",	NULL },
{ IPSECDOI_ID_DER_ASN1_DN,	"DER_ASN1_DN",	NULL },
{ IPSECDOI_ID_DER_ASN1_GN,	"DER_ASN1_GN",	NULL },
{ IPSECDOI_ID_KEY_ID,		"KEY_ID",	NULL },
};

char *
s_ipsecdoi_ident(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_ipsecdoi_ident); i++)
		if (name_ipsecdoi_ident[i].key == k)
			return name_ipsecdoi_ident[i].str;
	return num2str(k);
}

/* oakley.h */
static struct ksmap name_oakley_attr[] = {
{ OAKLEY_ATTR_ENC_ALG,		"Encryption Algorithm",	s_attr_isakmp_enc },
{ OAKLEY_ATTR_HASH_ALG,		"Hash Algorithm",	s_attr_isakmp_hash },
{ OAKLEY_ATTR_AUTH_METHOD,	"Authentication Method", s_oakley_attr_method },
{ OAKLEY_ATTR_GRP_DESC,		"Group Description",	s_attr_isakmp_desc },
{ OAKLEY_ATTR_GRP_TYPE,		"Group Type",		s_attr_isakmp_group },
{ OAKLEY_ATTR_GRP_PI,		"Group Prime/Irreducible Polynomial",	NULL },
{ OAKLEY_ATTR_GRP_GEN_ONE,	"Group Generator One",	NULL },
{ OAKLEY_ATTR_GRP_GEN_TWO,	"Group Generator Two",	NULL },
{ OAKLEY_ATTR_GRP_CURVE_A,	"Group Curve A",	NULL },
{ OAKLEY_ATTR_GRP_CURVE_B,	"Group Curve B",	NULL },
{ OAKLEY_ATTR_SA_LD_TYPE,	"Life Type",		s_attr_isakmp_ltype },
{ OAKLEY_ATTR_SA_LD,		"Life Duration",	NULL },
{ OAKLEY_ATTR_PRF,		"PRF",			NULL },
{ OAKLEY_ATTR_KEY_LEN,		"Key Length",		NULL },
{ OAKLEY_ATTR_FIELD_SIZE,	"Field Size",		NULL },
{ OAKLEY_ATTR_GRP_ORDER,	"Group Order",		NULL },
{ OAKLEY_ATTR_BLOCK_SIZE,	"Block Size",		NULL },
{ OAKLEY_ATTR_GSS_ID,		"GSS-API endpoint name",NULL },
};

char *
s_oakley_attr(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_oakley_attr); i++)
		if (name_oakley_attr[i].key == k)
			return name_oakley_attr[i].str;
	return num2str(k);
}

static struct ksmap name_attr_isakmp_enc[] = {
{ OAKLEY_ATTR_ENC_ALG_DES,	"DES-CBC",		NULL },
{ OAKLEY_ATTR_ENC_ALG_IDEA,	"IDEA-CBC",		NULL },
{ OAKLEY_ATTR_ENC_ALG_BLOWFISH,	"Blowfish-CBC",		NULL },
{ OAKLEY_ATTR_ENC_ALG_RC5,	"RC5-R16-B64-CBC",	NULL },
{ OAKLEY_ATTR_ENC_ALG_3DES,	"3DES-CBC",		NULL },
{ OAKLEY_ATTR_ENC_ALG_CAST,	"CAST-CBC",		NULL },
{ OAKLEY_ATTR_ENC_ALG_AES,	"AES-CBC",		NULL },
};

char *
s_attr_isakmp_enc(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_attr_isakmp_enc); i++)
		if (name_attr_isakmp_enc[i].key == k)
			return name_attr_isakmp_enc[i].str;
	return num2str(k);
}

static struct ksmap name_attr_isakmp_hash[] = {
{ OAKLEY_ATTR_HASH_ALG_MD5,	"MD5",		NULL },
{ OAKLEY_ATTR_HASH_ALG_SHA,	"SHA",		NULL },
{ OAKLEY_ATTR_HASH_ALG_TIGER,	"Tiger",	NULL },
{ OAKLEY_ATTR_HASH_ALG_SHA2_256,"SHA256",	NULL },
{ OAKLEY_ATTR_HASH_ALG_SHA2_384,"SHA384",	NULL },
{ OAKLEY_ATTR_HASH_ALG_SHA2_512,"SHA512",	NULL },
};

char *
s_attr_isakmp_hash(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_attr_isakmp_hash); i++)
		if (name_attr_isakmp_hash[i].key == k)
			return name_attr_isakmp_hash[i].str;
	return num2str(k);
}

static struct ksmap name_attr_isakmp_method[] = {
{ OAKLEY_ATTR_AUTH_METHOD_PSKEY,		"pre-shared key",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_DSSSIG,		"DSS signatures",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_RSASIG,		"RSA signatures",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_RSAENC,		"Encryption with RSA",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_RSAREV,		"Revised encryption with RSA",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_EGENC,		"Encryption with El-Gamal",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_EGREV,		"Revised encryption with El-Gamal",	NULL },
#ifdef HAVE_GSSAPI
{ OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB,		"GSS-API on Kerberos 5", NULL },
#endif
#ifdef ENABLE_HYBRID
{ OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_R,		"Hybrid DSS server",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_R,		"Hybrid RSA server",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_I,		"Hybrid DSS client",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_I,		"Hybrid RSA client",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_I,	"XAuth pskey client",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_R,	"XAuth pskey server",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_I,	"XAuth RSASIG client",	NULL },
{ OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_R,	"XAuth RSASIG server",	NULL },
#endif
};

char *
s_oakley_attr_method(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_attr_isakmp_method); i++)
		if (name_attr_isakmp_method[i].key == k)
			return name_attr_isakmp_method[i].str;
	return num2str(k);
}

static struct ksmap name_attr_isakmp_desc[] = {
{ OAKLEY_ATTR_GRP_DESC_MODP768,		"768-bit MODP group",	NULL },
{ OAKLEY_ATTR_GRP_DESC_MODP1024,	"1024-bit MODP group",	NULL },
{ OAKLEY_ATTR_GRP_DESC_EC2N155,		"EC2N group on GP[2^155]",	NULL },
{ OAKLEY_ATTR_GRP_DESC_EC2N185,		"EC2N group on GP[2^185]",	NULL },
{ OAKLEY_ATTR_GRP_DESC_MODP1536,	"1536-bit MODP group",	NULL },
{ OAKLEY_ATTR_GRP_DESC_MODP2048,	"2048-bit MODP group",	NULL },
{ OAKLEY_ATTR_GRP_DESC_MODP3072,	"3072-bit MODP group",	NULL },
{ OAKLEY_ATTR_GRP_DESC_MODP4096,	"4096-bit MODP group",	NULL },
{ OAKLEY_ATTR_GRP_DESC_MODP6144,	"6144-bit MODP group",	NULL },
{ OAKLEY_ATTR_GRP_DESC_MODP8192,	"8192-bit MODP group",	NULL },
};

char *
s_attr_isakmp_desc(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_attr_isakmp_desc); i++)
		if (name_attr_isakmp_desc[i].key == k)
			return name_attr_isakmp_desc[i].str;
	return num2str(k);
}

static struct ksmap name_attr_isakmp_group[] = {
{ OAKLEY_ATTR_GRP_TYPE_MODP,	"MODP",	NULL },
{ OAKLEY_ATTR_GRP_TYPE_ECP,	"ECP",	NULL },
{ OAKLEY_ATTR_GRP_TYPE_EC2N,	"EC2N",	NULL },
};

char *
s_attr_isakmp_group(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_attr_isakmp_group); i++)
		if (name_attr_isakmp_group[i].key == k)
			return name_attr_isakmp_group[i].str;
	return num2str(k);
}

static struct ksmap name_attr_isakmp_ltype[] = {
{ OAKLEY_ATTR_SA_LD_TYPE_SEC,	"seconds",	NULL },
{ OAKLEY_ATTR_SA_LD_TYPE_KB,	"kilobytes",	NULL },
};

char *
s_attr_isakmp_ltype(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_attr_isakmp_ltype); i++)
		if (name_attr_isakmp_ltype[i].key == k)
			return name_attr_isakmp_ltype[i].str;
	return num2str(k);
}

char *
s_oakley_attr_v(type, val)
	int type, val;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_oakley_attr); i++)
		if (name_oakley_attr[i].key == type
		 && name_oakley_attr[i].f)
			return (name_oakley_attr[i].f)(val);
	return num2str(val);
}

/* netinet6/ipsec.h */
static struct ksmap name_ipsec_level[] = {
{ IPSEC_LEVEL_USE,	"use",		NULL },
{ IPSEC_LEVEL_REQUIRE,	"require",	NULL },
{ IPSEC_LEVEL_UNIQUE,	"unique",	NULL },
};

char *
s_ipsec_level(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_ipsec_level); i++)
		if (name_ipsec_level[i].key == k)
			return name_ipsec_level[i].str;
	return num2str(k);
}

static struct ksmap name_algclass[] = {
{ algclass_ipsec_enc,	"ipsec enc",	s_ipsecdoi_trns_esp },
{ algclass_ipsec_auth,	"ipsec auth",	s_ipsecdoi_trns_ah },
{ algclass_ipsec_comp,	"ipsec comp",	s_ipsecdoi_trns_ipcomp },
{ algclass_isakmp_enc,	"isakmp enc",	s_attr_isakmp_enc },
{ algclass_isakmp_hash,	"isakmp hash",	s_attr_isakmp_hash },
{ algclass_isakmp_dh,	"isakmp dh",	s_attr_isakmp_desc },
{ algclass_isakmp_ameth, "isakmp auth method",	s_oakley_attr_method },
};

char *
s_algclass(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_algclass); i++)
		if (name_algclass[i].key == k)
			return name_algclass[i].str;
	return num2str(k);
}

char *
s_algtype(class, n)
	int class, n;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_algclass); i++)
		if (name_algclass[i].key == class
		 && name_algclass[i].f)
			return (name_algclass[i].f)(n);
	return num2str(n);
}

/* pfkey.h */
static struct ksmap name_pfkey_type[] = {
{ SADB_GETSPI,		"GETSPI",	NULL },
{ SADB_UPDATE,		"UPDATE",	NULL },
{ SADB_ADD,		"ADD",		NULL },
{ SADB_DELETE,		"DELETE",	NULL },
{ SADB_GET,		"GET",		NULL },
{ SADB_ACQUIRE,		"ACQUIRE",	NULL },
{ SADB_REGISTER,	"REGISTER",	NULL },
{ SADB_EXPIRE,		"EXPIRE",	NULL },
{ SADB_FLUSH,		"FLUSH",	NULL },
{ SADB_DUMP,		"DUMP",		NULL },
{ SADB_X_PROMISC,	"X_PROMISC",	NULL },
{ SADB_X_PCHANGE,	"X_PCHANGE",	NULL },
{ SADB_X_SPDUPDATE,	"X_SPDUPDATE",	NULL },
{ SADB_X_SPDADD,	"X_SPDADD",	NULL },
{ SADB_X_SPDDELETE,	"X_SPDDELETE",	NULL },
{ SADB_X_SPDGET,	"X_SPDGET",	NULL },
{ SADB_X_SPDACQUIRE,	"X_SPDACQUIRE",	NULL },
{ SADB_X_SPDDUMP,	"X_SPDDUMP",	NULL },
{ SADB_X_SPDFLUSH,	"X_SPDFLUSH",	NULL },
{ SADB_X_SPDSETIDX,	"X_SPDSETIDX",	NULL },
{ SADB_X_SPDEXPIRE,	"X_SPDEXPIRE",	NULL },
{ SADB_X_SPDDELETE2,	"X_SPDDELETE2",	NULL },
#ifdef SADB_X_NAT_T_NEW_MAPPING
{ SADB_X_NAT_T_NEW_MAPPING, "X_NAT_T_NEW_MAPPING", NULL },
#endif
#ifdef SADB_X_MIGRATE
{ SADB_X_MIGRATE,	"X_MIGRATE",	NULL },
#endif
};

char *
s_pfkey_type(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_pfkey_type); i++)
		if (name_pfkey_type[i].key == k)
			return name_pfkey_type[i].str;
	return num2str(k);
}

static struct ksmap name_pfkey_satype[] = {
{ SADB_SATYPE_UNSPEC,	"UNSPEC",	NULL },
{ SADB_SATYPE_AH,	"AH",		NULL },
{ SADB_SATYPE_ESP,	"ESP",		NULL },
{ SADB_SATYPE_RSVP,	"RSVP",		NULL },
{ SADB_SATYPE_OSPFV2,	"OSPFV2",	NULL },
{ SADB_SATYPE_RIPV2,	"RIPV2",	NULL },
{ SADB_SATYPE_MIP,	"MIP",		NULL },
{ SADB_X_SATYPE_IPCOMP,	"IPCOMP",	NULL },
};

char *
s_pfkey_satype(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_pfkey_satype); i++)
		if (name_pfkey_satype[i].key == k)
			return name_pfkey_satype[i].str;
	return num2str(k);
}

static struct ksmap name_direction[] = {
{ IPSEC_DIR_INBOUND,	"in",	NULL },
{ IPSEC_DIR_OUTBOUND,	"out",	NULL },
#ifdef HAVE_POLICY_FWD
{ IPSEC_DIR_FWD,	"fwd",	NULL },
#endif
};

char *
s_direction(k)
	int k;
{
	int i;
	for (i = 0; i < ARRAYLEN(name_direction); i++)
		if (name_direction[i].key == k)
			return name_direction[i].str;
	return num2str(k);
}

char *
s_proto(k)
	int k;
{
	switch (k) {
	case IPPROTO_ICMP:
		return "icmp";
	case IPPROTO_TCP:
		return "tcp";
	case IPPROTO_UDP:
		return "udp";
	case IPPROTO_ICMPV6:
		return "icmpv6";
	case IPSEC_ULPROTO_ANY:
		return "any";
	}

	return num2str(k);
}

char *
s_doi(int k)
{
  switch (k) {
    case IPSEC_DOI:
      return "ipsec_doi";
    default:
      return num2str(k);
  }
}

char *
s_etype (int k)
{
  switch (k) {
    case ISAKMP_ETYPE_NONE:
      return "_none";
    case ISAKMP_ETYPE_BASE:
      return "base";
    case ISAKMP_ETYPE_IDENT:
      return "main";
    case ISAKMP_ETYPE_AUTH:
      return "_auth";
    case ISAKMP_ETYPE_AGG:
      return "aggressive";
    case ISAKMP_ETYPE_INFO:
      return "_info";
    case ISAKMP_ETYPE_QUICK:
      return "_quick";
    case ISAKMP_ETYPE_NEWGRP:
      return "_newgrp";
    case ISAKMP_ETYPE_ACKINFO:
      return "_ackinfo";
    default:
      return num2str(k);
  }
}

char *
s_idtype (int k)
{
  switch (k) {
    case IDTYPE_FQDN:
      return "fqdn";
    case IDTYPE_USERFQDN:
      return "user_fqdn";
    case IDTYPE_KEYID:
      return "keyid";
    case IDTYPE_ADDRESS:
      return "address";
    case IDTYPE_ASN1DN:
      return "asn1dn";
    default:
      return num2str(k);
  }
}

char *
s_switch (int k)
{
  switch (k) {
    case FALSE:
      return "off";
    case TRUE:
      return "on";
    default:
      return num2str(k);
  }
}
