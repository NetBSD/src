/*	$NetBSD: otp.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

/* otp.c - OATH 2-factor authentication module */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2015-2021 The OpenLDAP Foundation.
 * Portions Copyright 2015 by Howard Chu, Symas Corp.
 * Portions Copyright 2016-2017 by Michael Ströder <michael@stroeder.com>
 * Portions Copyright 2018 by Ondřej Kuzník, Symas Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work includes code from the lastbind overlay.
 */

#include <portable.h>

#ifdef SLAPD_OVER_OTP

#if HAVE_STDINT_H
#include <stdint.h>
#endif

#include <lber.h>
#include <lber_pvt.h>
#include "lutil.h"
#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/string.h>
/* include socket.h to get sys/types.h and/or winsock2.h */
#include <ac/socket.h>

#if HAVE_OPENSSL
#include <openssl/sha.h>
#include <openssl/hmac.h>

#define TOTP_SHA512_DIGEST_LENGTH SHA512_DIGEST_LENGTH
#define TOTP_SHA1 EVP_sha1()
#define TOTP_SHA224 EVP_sha224()
#define TOTP_SHA256 EVP_sha256()
#define TOTP_SHA384 EVP_sha384()
#define TOTP_SHA512 EVP_sha512()
#define TOTP_HMAC_CTX HMAC_CTX *

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static HMAC_CTX *
HMAC_CTX_new( void )
{
	HMAC_CTX *ctx = OPENSSL_malloc( sizeof(*ctx) );
	if ( ctx != NULL ) {
		HMAC_CTX_init( ctx );
	}
	return ctx;
}

static void
HMAC_CTX_free( HMAC_CTX *ctx )
{
	if ( ctx != NULL ) {
		HMAC_CTX_cleanup( ctx );
		OPENSSL_free( ctx );
	}
}
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

#define HMAC_setup( ctx, key, len, hash ) \
	ctx = HMAC_CTX_new(); \
	HMAC_Init_ex( ctx, key, len, hash, 0 )
#define HMAC_crunch( ctx, buf, len ) HMAC_Update( ctx, buf, len )
#define HMAC_finish( ctx, dig, dlen ) \
	HMAC_Final( ctx, dig, &dlen ); \
	HMAC_CTX_free( ctx )

#elif HAVE_GNUTLS
#include <nettle/hmac.h>

#define TOTP_SHA512_DIGEST_LENGTH SHA512_DIGEST_SIZE
#define TOTP_SHA1 &nettle_sha1
#define TOTP_SHA224 &nettle_sha224
#define TOTP_SHA256 &nettle_sha256
#define TOTP_SHA384 &nettle_sha384
#define TOTP_SHA512 &nettle_sha512
#define TOTP_HMAC_CTX struct hmac_sha512_ctx

#define HMAC_setup( ctx, key, len, hash ) \
	const struct nettle_hash *h = hash; \
	hmac_set_key( &ctx.outer, &ctx.inner, &ctx.state, h, len, key )
#define HMAC_crunch( ctx, buf, len ) hmac_update( &ctx.state, h, len, buf )
#define HMAC_finish( ctx, dig, dlen ) \
	hmac_digest( &ctx.outer, &ctx.inner, &ctx.state, h, h->digest_size, dig ); \
	dlen = h->digest_size

#else
#error Unsupported crypto backend.
#endif

#include "slap.h"
#include "slap-config.h"

/* Schema from OATH-LDAP project by Michael Ströder */

static struct {
	char *name, *oid;
} otp_oid[] = {
	{ "oath-ldap", "1.3.6.1.4.1.5427.1.389.4226" },
	{ "oath-ldap-at", "oath-ldap:4" },
	{ "oath-ldap-oc", "oath-ldap:6" },
	{ NULL }
};

AttributeDescription *ad_oathOTPToken;
AttributeDescription *ad_oathSecret;
AttributeDescription *ad_oathOTPLength;
AttributeDescription *ad_oathHMACAlgorithm;

AttributeDescription *ad_oathHOTPParams;
AttributeDescription *ad_oathHOTPToken;
AttributeDescription *ad_oathHOTPCounter;
AttributeDescription *ad_oathHOTPLookahead;

AttributeDescription *ad_oathTOTPTimeStepPeriod;
AttributeDescription *ad_oathTOTPParams;
AttributeDescription *ad_oathTOTPToken;
AttributeDescription *ad_oathTOTPLastTimeStep;
AttributeDescription *ad_oathTOTPTimeStepWindow;
AttributeDescription *ad_oathTOTPTimeStepDrift;

static struct otp_at {
	char					*schema;
	AttributeDescription	**adp;
} otp_at[] = {
	{ "( oath-ldap-at:1 "
		"NAME 'oathSecret' "
		"DESC 'OATH-LDAP: Shared Secret (possibly encrypted with public key in oathEncKey)' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"EQUALITY octetStringMatch "
		"SUBSTR octetStringSubstringsMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40 )",
		&ad_oathSecret },

	{ "( oath-ldap-at:2 "
		"NAME 'oathTokenSerialNumber' "
		"DESC 'OATH-LDAP: Proprietary hardware token serial number assigned by vendor' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"EQUALITY caseIgnoreMatch "
		"SUBSTR caseIgnoreSubstringsMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.44{64})" },

	{ "( oath-ldap-at:3 "
		"NAME 'oathTokenIdentifier' "
		"DESC 'OATH-LDAP: Globally unique OATH token identifier' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"EQUALITY caseIgnoreMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15{256} )" },

	{ "( oath-ldap-at:4 "
		"NAME 'oathParamsEntry' "
		"DESC 'OATH-LDAP: DN pointing to OATH parameter/policy object' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"SUP distinguishedName )" },
	{ "( oath-ldap-at:4.1 "
		"NAME 'oathTOTPTimeStepPeriod' "
		"DESC 'OATH-LDAP: Time window for TOTP (seconds)' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"EQUALITY integerMatch "
		"ORDERING integerOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 )",
		&ad_oathTOTPTimeStepPeriod },

	{ "( oath-ldap-at:5 "
		"NAME 'oathOTPLength' "
		"DESC 'OATH-LDAP: Length of OTP (number of digits)' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"EQUALITY integerMatch "
		"ORDERING integerOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 )",
		&ad_oathOTPLength },
	{ "( oath-ldap-at:5.1 "
		"NAME 'oathHOTPParams' "
		"DESC 'OATH-LDAP: DN pointing to HOTP parameter object' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
        "SUP oathParamsEntry )",
		&ad_oathHOTPParams },
	{ "( oath-ldap-at:5.2 "
		"NAME 'oathTOTPParams' "
		"DESC 'OATH-LDAP: DN pointing to TOTP parameter object' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"SUP oathParamsEntry )",
		&ad_oathTOTPParams },

	{ "( oath-ldap-at:6 "
		"NAME 'oathHMACAlgorithm' "
		"DESC 'OATH-LDAP: HMAC algorithm used for generating OTP values' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"EQUALITY objectIdentifierMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 )",
		&ad_oathHMACAlgorithm },

	{ "( oath-ldap-at:7 "
		"NAME 'oathTimestamp' "
		"DESC 'OATH-LDAP: Timestamp (not directly used).' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"EQUALITY generalizedTimeMatch "
		"ORDERING generalizedTimeOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 )" },
	{ "( oath-ldap-at:7.1 "
        "NAME 'oathLastFailure' "
        "DESC 'OATH-LDAP: Timestamp of last failed OATH validation' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "SUP oathTimestamp )" },
	{ "( oath-ldap-at:7.2 "
        "NAME 'oathLastLogin' "
        "DESC 'OATH-LDAP: Timestamp of last successful OATH validation' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "SUP oathTimestamp )" },
	{ "( oath-ldap-at:7.3 "
        "NAME 'oathSecretTime' "
        "DESC 'OATH-LDAP: Timestamp of generation of oathSecret attribute.' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "SUP oathTimestamp )" },

	{ "( oath-ldap-at:8 "
		"NAME 'oathSecretMaxAge' "
		"DESC 'OATH-LDAP: Time in seconds for which the shared secret (oathSecret) will be valid from oathSecretTime value.' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"EQUALITY integerMatch "
		"ORDERING integerOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 )" },

	{ "( oath-ldap-at:9 "
		"NAME 'oathToken' "
		"DESC 'OATH-LDAP: DN pointing to OATH token object' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"SUP distinguishedName )" },
	{ "( oath-ldap-at:9.1 "
        "NAME 'oathHOTPToken' "
        "DESC 'OATH-LDAP: DN pointing to OATH/HOTP token object' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "SUP oathToken )",
		&ad_oathHOTPToken },
	{ "( oath-ldap-at:9.2 "
		"NAME 'oathTOTPToken' "
		"DESC 'OATH-LDAP: DN pointing to OATH/TOTP token object' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"SUP oathToken )",
		&ad_oathTOTPToken },

	{ "( oath-ldap-at:10 "
		"NAME 'oathCounter' "
		"DESC 'OATH-LDAP: Counter for OATH data (not directly used)' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"EQUALITY integerMatch "
		"ORDERING integerOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 )" },
	{ "( oath-ldap-at:10.1 "
        "NAME 'oathFailureCount' "
        "DESC 'OATH-LDAP: OATH failure counter' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "SUP oathCounter )" },
	{ "( oath-ldap-at:10.2 "
        "NAME 'oathHOTPCounter' "
              "DESC 'OATH-LDAP: Counter for HOTP' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "SUP oathCounter )",
		&ad_oathHOTPCounter },
	{ "( oath-ldap-at:10.3 "
        "NAME 'oathHOTPLookAhead' "
        "DESC 'OATH-LDAP: Look-ahead window for HOTP' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "SUP oathCounter )",
		&ad_oathHOTPLookahead },
	{ "( oath-ldap-at:10.5 "
        "NAME 'oathThrottleLimit' "
        "DESC 'OATH-LDAP: Failure throttle limit' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "SUP oathCounter )" },
	{ "( oath-ldap-at:10.6 "
		"NAME 'oathTOTPLastTimeStep' "
		"DESC 'OATH-LDAP: Last time step seen for TOTP (time/period)' "
		"X-ORIGIN 'OATH-LDAP' "
		"SINGLE-VALUE "
		"SUP oathCounter )",
		&ad_oathTOTPLastTimeStep },
	{ "( oath-ldap-at:10.7 "
        "NAME 'oathMaxUsageCount' "
        "DESC 'OATH-LDAP: Maximum number of times a token can be used' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "SUP oathCounter )" },
	{ "( oath-ldap-at:10.8 "
        "NAME 'oathTOTPTimeStepWindow' "
        "DESC 'OATH-LDAP: Size of time step +/- tolerance window used for TOTP validation' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "SUP oathCounter )",
		&ad_oathTOTPTimeStepWindow },
	{ "( oath-ldap-at:10.9 "
        "NAME 'oathTOTPTimeStepDrift' "
        "DESC 'OATH-LDAP: Last observed time step shift seen for TOTP' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "SUP oathCounter )",
		&ad_oathTOTPTimeStepDrift },

	{ "( oath-ldap-at:11 "
        "NAME 'oathSecretLength' "
        "DESC 'OATH-LDAP: Length of plain-text shared secret (number of bytes)' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "EQUALITY integerMatch "
        "ORDERING integerOrderingMatch "
        "SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 )" },

	{ "( oath-ldap-at:12 "
        "NAME 'oathEncKey' "
        "DESC 'OATH-LDAP: public key to be used for encrypting new shared secrets' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "EQUALITY caseIgnoreMatch "
        "SUBSTR caseIgnoreSubstringsMatch "
        "SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 )" },

	{ "( oath-ldap-at:13 "
        "NAME 'oathResultCode' "
        "DESC 'OATH-LDAP: LDAP resultCode to use in response' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "EQUALITY integerMatch "
        "ORDERING integerOrderingMatch "
        "SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 )" },
	{ "( oath-ldap-at:13.1 "
        "NAME 'oathSuccessResultCode' "
        "DESC 'OATH-LDAP: success resultCode to use in bind/compare response' "
        "X-ORIGIN 'OATH-LDAP' "
        "SUP oathResultCode )" },
	{ "( oath-ldap-at:13.2 "
        "NAME 'oathFailureResultCode' "
        "DESC 'OATH-LDAP: failure resultCode to use in bind/compare response' "
        "X-ORIGIN 'OATH-LDAP' "
        "SUP oathResultCode )" },

	{ "( oath-ldap-at:14 "
        "NAME 'oathTokenPIN' "
        "DESC 'OATH-LDAP: Configuration PIN (possibly encrypted with oathEncKey)' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "EQUALITY caseIgnoreMatch "
        "SUBSTR caseIgnoreSubstringsMatch "
        "SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 )" },

	{ "( oath-ldap-at:15 "
        "NAME 'oathMessage' "
        "DESC 'OATH-LDAP: success diagnosticMessage to use in bind/compare response' "
        "X-ORIGIN 'OATH-LDAP' "
        "SINGLE-VALUE "
        "EQUALITY caseIgnoreMatch "
        "SUBSTR caseIgnoreSubstringsMatch "
        "SYNTAX 1.3.6.1.4.1.1466.115.121.1.15{1024} )" },
	{ "( oath-ldap-at:15.1 "
        "NAME 'oathSuccessMessage' "
        "DESC 'OATH-LDAP: success diagnosticMessage to use in bind/compare response' "
        "X-ORIGIN 'OATH-LDAP' "
        "SUP oathMessage )" },
	{ "( oath-ldap-at:15.2 "
        "NAME 'oathFailureMessage' "
        "DESC 'OATH-LDAP: failure diagnosticMessage to use in bind/compare response' "
        "X-ORIGIN 'OATH-LDAP' "
        "SUP oathMessage )" },

	{ NULL }
};

ObjectClass *oc_oathOTPUser;
ObjectClass *oc_oathHOTPToken;
ObjectClass *oc_oathTOTPToken;
ObjectClass *oc_oathHOTPParams;
ObjectClass *oc_oathTOTPParams;

static struct otp_oc {
	char			*schema;
	ObjectClass		**ocp;
} otp_oc[] = {
	{ "( oath-ldap-oc:1 "
		"NAME 'oathUser' "
		"DESC 'OATH-LDAP: User Object' "
		"X-ORIGIN 'OATH-LDAP' "
		"ABSTRACT )",
		&oc_oathOTPUser },
	{ "( oath-ldap-oc:1.1 "
		"NAME 'oathHOTPUser' "
		"DESC 'OATH-LDAP: HOTP user object' "
		"X-ORIGIN 'OATH-LDAP' "
		"AUXILIARY "
		"SUP oathUser "
		"MAY ( oathHOTPToken ) )" },
	{ "( oath-ldap-oc:1.2 "
		"NAME 'oathTOTPUser' "
		"DESC 'OATH-LDAP: TOTP user object' "
		"X-ORIGIN 'OATH-LDAP' "
		"AUXILIARY "
		"SUP oathUser "
		"MUST ( oathTOTPToken ) )" },
	{ "( oath-ldap-oc:2 "
		"NAME 'oathParams' "
		"DESC 'OATH-LDAP: Parameter object' "
		"X-ORIGIN 'OATH-LDAP' "
		"ABSTRACT "
		"MUST ( oathOTPLength $ oathHMACAlgorithm ) "
		"MAY ( oathSecretMaxAge $ oathSecretLength $ "
			"oathMaxUsageCount $ oathThrottleLimit $ oathEncKey $ "
			"oathSuccessResultCode $ oathSuccessMessage $ "
			"oathFailureResultCode $ oathFailureMessage ) )" },
	{ "( oath-ldap-oc:2.1 "
		"NAME 'oathHOTPParams' "
		"DESC 'OATH-LDAP: HOTP parameter object' "
		"X-ORIGIN 'OATH-LDAP' "
		"AUXILIARY "
		"SUP oathParams "
		"MUST ( oathHOTPLookAhead ) )",
		&oc_oathHOTPParams },
	{ "( oath-ldap-oc:2.2 "
		"NAME 'oathTOTPParams' "
		"DESC 'OATH-LDAP: TOTP parameter object' "
		"X-ORIGIN 'OATH-LDAP' "
		"AUXILIARY "
		"SUP oathParams "
		"MUST ( oathTOTPTimeStepPeriod ) "
		"MAY ( oathTOTPTimeStepWindow ) )",
		&oc_oathTOTPParams },
	{ "( oath-ldap-oc:3 "
		"NAME 'oathToken' "
		"DESC 'OATH-LDAP: User Object' "
		"X-ORIGIN 'OATH-LDAP' "
		"ABSTRACT "
		"MAY ( oathSecret $ oathSecretTime $ "
			"oathLastLogin $ oathFailureCount $ oathLastFailure $ "
			"oathTokenSerialNumber $ oathTokenIdentifier $ oathTokenPIN ) )" },
	{ "( oath-ldap-oc:3.1 "
		"NAME 'oathHOTPToken' "
		"DESC 'OATH-LDAP: HOTP token object' "
		"X-ORIGIN 'OATH-LDAP' "
		"AUXILIARY "
		"SUP oathToken "
		"MAY ( oathHOTPParams $ oathHOTPCounter ) )",
		&oc_oathHOTPToken },
	{ "( oath-ldap-oc:3.2 "
		"NAME 'oathTOTPToken' "
		"DESC 'OATH-LDAP: TOTP token' "
		"X-ORIGIN 'OATH-LDAP' "
		"AUXILIARY "
		"SUP oathToken "
		"MAY ( oathTOTPParams $ oathTOTPLastTimeStep $ oathTOTPTimeStepDrift ) )",
		&oc_oathTOTPToken },
	{ NULL }
};

typedef struct myval {
	ber_len_t mv_len;
	void *mv_val;
} myval;

static void
do_hmac( const void *hash, myval *key, myval *data, myval *out )
{
	TOTP_HMAC_CTX ctx;
	unsigned int digestLen;

	HMAC_setup( ctx, key->mv_val, key->mv_len, hash );
	HMAC_crunch( ctx, data->mv_val, data->mv_len );
	HMAC_finish( ctx, out->mv_val, digestLen );
	out->mv_len = digestLen;
}

#define MAX_DIGITS 8
static const int DIGITS_POWER[] = {
	1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000,
};

static const void *
otp_choose_mech( struct berval *oid )
{
	/* RFC 8018 OIDs */
	const struct berval oid_hmacwithSHA1 = BER_BVC("1.2.840.113549.2.7");
	const struct berval oid_hmacwithSHA224 = BER_BVC("1.2.840.113549.2.8");
	const struct berval oid_hmacwithSHA256 = BER_BVC("1.2.840.113549.2.9");
	const struct berval oid_hmacwithSHA384 = BER_BVC("1.2.840.113549.2.10");
	const struct berval oid_hmacwithSHA512 = BER_BVC("1.2.840.113549.2.11");

	if ( !ber_bvcmp( &oid_hmacwithSHA1, oid ) ) {
		return TOTP_SHA1;
	} else if ( !ber_bvcmp( &oid_hmacwithSHA224, oid ) ) {
		return TOTP_SHA224;
	} else if ( !ber_bvcmp( &oid_hmacwithSHA256, oid ) ) {
		return TOTP_SHA256;
	} else if ( !ber_bvcmp( &oid_hmacwithSHA384, oid ) ) {
		return TOTP_SHA384;
	} else if ( !ber_bvcmp( &oid_hmacwithSHA512, oid ) ) {
		return TOTP_SHA512;
	}

	Debug( LDAP_DEBUG_TRACE, "otp_choose_mech: "
			"hmac OID %s unsupported\n",
			oid->bv_val );
	return NULL;
}

static void
generate(
		struct berval *bv,
		uint64_t tval,
		int digits,
		struct berval *out,
		const void *mech )
{
	unsigned char digest[TOTP_SHA512_DIGEST_LENGTH];
	myval digval;
	myval key, data;
	unsigned char msg[8];
	int i, offset, res, otp;

#if WORDS_BIGENDIAN
	*(uint64_t *)msg = tval;
#else
	for ( i = 7; i >= 0; i-- ) {
		msg[i] = tval & 0xff;
		tval >>= 8;
	}
#endif

	key.mv_len = bv->bv_len;
	key.mv_val = bv->bv_val;

	data.mv_val = msg;
	data.mv_len = sizeof(msg);

	digval.mv_val = digest;
	digval.mv_len = sizeof(digest);
	do_hmac( mech, &key, &data, &digval );

	offset = digest[digval.mv_len - 1] & 0xf;
	res = ( (digest[offset] & 0x7f) << 24 ) |
			( ( digest[offset + 1] & 0xff ) << 16 ) |
			( ( digest[offset + 2] & 0xff ) << 8 ) |
			( digest[offset + 3] & 0xff );

	otp = res % DIGITS_POWER[digits];
	out->bv_len = snprintf( out->bv_val, out->bv_len, "%0*d", digits, otp );
}

static int
otp_bind_response( Operation *op, SlapReply *rs )
{
	if ( rs->sr_err == LDAP_SUCCESS ) {
		/* If the bind succeeded, return our result */
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
	}
	return SLAP_CB_CONTINUE;
}

static long
otp_hotp( Operation *op, Entry *token )
{
	char outbuf[MAX_DIGITS + 1];
	Entry *params = NULL;
	Attribute *a;
	BerValue *secret, client_otp;
	const void *mech;
	long last_step = -1, found = -1;
	int i, otp_len, window;

	a = attr_find( token->e_attrs, ad_oathSecret );
	secret = &a->a_vals[0];

	a = attr_find( token->e_attrs, ad_oathHOTPCounter );
	if ( a && lutil_atol( &last_step, a->a_vals[0].bv_val ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "otp_hotp: "
				"could not parse oathHOTPCounter value %s\n",
				a->a_vals[0].bv_val );
		goto done;
	}

	a = attr_find( token->e_attrs, ad_oathHOTPParams );
	if ( !a ||
			be_entry_get_rw( op, &a->a_nvals[0], oc_oathHOTPParams, NULL, 0,
					&params ) ) {
		goto done;
	}

	a = attr_find( params->e_attrs, ad_oathOTPLength );
	if ( lutil_atoi( &otp_len, a->a_vals[0].bv_val ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "otp_hotp: "
				"could not parse oathOTPLength value %s\n",
				a->a_vals[0].bv_val );
		goto done;
	}
	if ( otp_len > MAX_DIGITS || op->orb_cred.bv_len < otp_len ) {
		/* Client didn't even send the token, fail immediately */
		goto done;
	}

	a = attr_find( params->e_attrs, ad_oathHOTPLookahead );
	if ( lutil_atoi( &window, a->a_vals[0].bv_val ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "otp_hotp: "
				"could not parse oathHOTPLookAhead value %s\n",
				a->a_vals[0].bv_val );
		goto done;
	}
	window++;

	a = attr_find( params->e_attrs, ad_oathHMACAlgorithm );
	if ( !(mech = otp_choose_mech( &a->a_vals[0] )) ) {
		goto done;
	}
	be_entry_release_r( op, params );
	params = NULL;

	/* We are provided "password" + "OTP", split accordingly */
	client_otp.bv_len = otp_len;
	client_otp.bv_val = op->orb_cred.bv_val + op->orb_cred.bv_len - otp_len;

	/* If check succeeds, advance the step counter accordingly */
	for ( i = 1; i <= window; i++ ) {
		BerValue out = { .bv_val = outbuf, .bv_len = sizeof(outbuf) };

		generate( secret, last_step + i, otp_len, &out, mech );
		if ( !ber_bvcmp( &out, &client_otp ) ) {
			found = last_step + i;
			/* Would we leak information if we stopped right now? */
		}
	}

	if ( found >= 0 ) {
		/* OTP check passed, trim the password */
		op->orb_cred.bv_len -= otp_len;
		Debug( LDAP_DEBUG_STATS, "%s HOTP token %s no. %ld redeemed\n",
				op->o_log_prefix, token->e_name.bv_val, found );
	}

done:
	memset( outbuf, 0, sizeof(outbuf) );
	if ( params ) {
		be_entry_release_r( op, params );
	}
	return found;
}

static long
otp_totp( Operation *op, Entry *token, long *drift )
{
	char outbuf[MAX_DIGITS + 1];
	Entry *params = NULL;
	Attribute *a;
	BerValue *secret, client_otp;
	const void *mech;
	long t, last_step = -1, found = -1, window = 0, old_drift;
	int i, otp_len, time_step;

	a = attr_find( token->e_attrs, ad_oathSecret );
	secret = &a->a_vals[0];

	a = attr_find( token->e_attrs, ad_oathTOTPLastTimeStep );
	if ( a && lutil_atol( &last_step, a->a_vals[0].bv_val ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "otp_totp: "
				"could not parse oathTOTPLastTimeStep value %s\n",
				a->a_vals[0].bv_val );
		goto done;
	}

	a = attr_find( token->e_attrs, ad_oathTOTPParams );
	if ( !a ||
			be_entry_get_rw( op, &a->a_nvals[0], oc_oathTOTPParams, NULL, 0,
					&params ) ) {
		goto done;
	}

	a = attr_find( params->e_attrs, ad_oathTOTPTimeStepPeriod );
	if ( lutil_atoi( &time_step, a->a_vals[0].bv_val ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "otp_totp: "
				"could not parse oathTOTPTimeStepPeriod value %s\n",
				a->a_vals[0].bv_val );
		goto done;
	}

	a = attr_find( params->e_attrs, ad_oathTOTPTimeStepWindow );
	if ( a && lutil_atol( &window, a->a_vals[0].bv_val ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "otp_totp: "
				"could not parse oathTOTPTimeStepWindow value %s\n",
				a->a_vals[0].bv_val );
		goto done;
	}

	a = attr_find( params->e_attrs, ad_oathTOTPTimeStepDrift );
	if ( a && lutil_atol( drift, a->a_vals[0].bv_val ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "otp_totp: "
				"could not parse oathTOTPTimeStepDrift value %s\n",
				a->a_vals[0].bv_val );
		goto done;
	}
	old_drift = *drift;
	t = op->o_time / time_step + *drift;

	a = attr_find( params->e_attrs, ad_oathOTPLength );
	if ( lutil_atoi( &otp_len, a->a_vals[0].bv_val ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "otp_totp: "
				"could not parse oathOTPLength value %s\n",
				a->a_vals[0].bv_val );
		goto done;
	}
	if ( otp_len > MAX_DIGITS || op->orb_cred.bv_len < otp_len ) {
		/* Client didn't even send the token, fail immediately */
		goto done;
	}

	a = attr_find( params->e_attrs, ad_oathHMACAlgorithm );
	if ( !(mech = otp_choose_mech( &a->a_vals[0] )) ) {
		goto done;
	}
	be_entry_release_r( op, params );
	params = NULL;

	/* We are provided "password" + "OTP", split accordingly */
	client_otp.bv_len = otp_len;
	client_otp.bv_val = op->orb_cred.bv_val + op->orb_cred.bv_len - otp_len;

	/* If check succeeds, advance the step counter accordingly */
	/* Negation of A001057 series that enumerates all integers:
	 * (0, -1, 1, -2, 2, ...) */
	for ( i = 0; i >= -window; i = ( i < 0 ) ? -i : ~i ) {
		BerValue out = { .bv_val = outbuf, .bv_len = sizeof(outbuf) };

		if ( t + i <= last_step ) continue;

		generate( secret, t + i, otp_len, &out, mech );
		if ( !ber_bvcmp( &out, &client_otp ) ) {
			found = t + i;
			*drift = old_drift + i;
			/* Would we leak information if we stopped right now? */
		}
	}

	/* OTP check passed, trim the password */
	if ( found >= 0 ) {
		assert( found > last_step );

		op->orb_cred.bv_len -= otp_len;
		Debug( LDAP_DEBUG_TRACE, "%s TOTP token %s redeemed with new drift of %ld\n",
				op->o_log_prefix, token->e_name.bv_val, *drift );
	}

done:
	memset( outbuf, 0, sizeof(outbuf) );
	if ( params ) {
		be_entry_release_r( op, params );
	}
	return found;
}

static int
otp_op_bind( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	BerValue totpdn = BER_BVNULL, hotpdn = BER_BVNULL, ndn;
	Entry *user = NULL, *token = NULL;
	AttributeDescription *ad = NULL, *drift_ad = NULL;
	Attribute *a;
	long t = -1, drift = 0;
	int rc = SLAP_CB_CONTINUE;

	if ( op->oq_bind.rb_method != LDAP_AUTH_SIMPLE ) {
		return rc;
	}

	op->o_bd->bd_info = (BackendInfo *)on->on_info;

	if ( be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &user ) ) {
		goto done;
	}

	if ( !is_entry_objectclass_or_sub( user, oc_oathOTPUser ) ) {
		be_entry_release_r( op, user );
		goto done;
	}

	if ( (a = attr_find( user->e_attrs, ad_oathTOTPToken )) ) {
		ber_dupbv_x( &totpdn, &a->a_nvals[0], op->o_tmpmemctx );
	}

	if ( (a = attr_find( user->e_attrs, ad_oathHOTPToken )) ) {
		ber_dupbv_x( &hotpdn, &a->a_nvals[0], op->o_tmpmemctx );
	}
	be_entry_release_r( op, user );

	if ( !BER_BVISNULL( &totpdn ) &&
			be_entry_get_rw( op, &totpdn, oc_oathTOTPToken, ad_oathSecret, 0,
					&token ) == LDAP_SUCCESS ) {
		ndn = totpdn;
		ad = ad_oathTOTPLastTimeStep;
		drift_ad = ad_oathTOTPTimeStepDrift;
		t = otp_totp( op, token, &drift );
		be_entry_release_r( op, token );
		token = NULL;
	}
	if ( t < 0 && !BER_BVISNULL( &hotpdn ) &&
			be_entry_get_rw( op, &hotpdn, oc_oathHOTPToken, ad_oathSecret, 0,
					&token ) == LDAP_SUCCESS ) {
		ndn = hotpdn;
		ad = ad_oathHOTPCounter;
		t = otp_hotp( op, token );
		be_entry_release_r( op, token );
		token = NULL;
	}

	/* If check succeeds, advance the step counter and drift accordingly */
	if ( t >= 0 ) {
		char outbuf[32], drift_buf[32];
		Operation op2;
		Opheader oh;
		Modifications mod[2], *m = mod;
		SlapReply rs2 = { REP_RESULT };
		slap_callback cb = { .sc_response = &slap_null_cb };
		BerValue bv[2], bv_drift[2];

		bv[0].bv_val = outbuf;
		bv[0].bv_len = snprintf( bv[0].bv_val, sizeof(outbuf), "%ld", t );
		BER_BVZERO( &bv[1] );

		m->sml_numvals = 1;
		m->sml_values = bv;
		m->sml_nvalues = NULL;
		m->sml_desc = ad;
		m->sml_op = LDAP_MOD_REPLACE;
		m->sml_flags = SLAP_MOD_INTERNAL;

		if ( drift_ad ) {
			m->sml_next = &mod[1];

			bv_drift[0].bv_val = drift_buf;
			bv_drift[0].bv_len = snprintf(
					bv_drift[0].bv_val, sizeof(drift_buf), "%ld", drift );
			BER_BVZERO( &bv_drift[1] );

			m++;
			m->sml_numvals = 1;
			m->sml_values = bv_drift;
			m->sml_nvalues = NULL;
			m->sml_desc = drift_ad;
			m->sml_op = LDAP_MOD_REPLACE;
			m->sml_flags = SLAP_MOD_INTERNAL;
		}
		m->sml_next = NULL;

		op2 = *op;
		oh = *op->o_hdr;
		op2.o_hdr = &oh;

		op2.o_callback = &cb;

		op2.o_tag = LDAP_REQ_MODIFY;
		op2.orm_modlist = mod;
		op2.o_dn = op->o_bd->be_rootdn;
		op2.o_ndn = op->o_bd->be_rootndn;
		op2.o_req_dn = ndn;
		op2.o_req_ndn = ndn;
		op2.o_opid = -1;

		op2.o_bd->be_modify( &op2, &rs2 );
		if ( rs2.sr_err != LDAP_SUCCESS ) {
			rc = LDAP_OTHER;
			goto done;
		}
	} else {
		/* Client failed the bind, but we still have to pass it over to the
		 * backend and fail the Bind later */
		slap_callback *cb;
		cb = op->o_tmpcalloc( 1, sizeof(slap_callback), op->o_tmpmemctx );
		cb->sc_response = otp_bind_response;
		cb->sc_next = op->o_callback;
		op->o_callback = cb;
	}

done:
	if ( !BER_BVISNULL( &hotpdn ) ) {
		ber_memfree_x( hotpdn.bv_val, op->o_tmpmemctx );
	}
	if ( !BER_BVISNULL( &totpdn ) ) {
		ber_memfree_x( totpdn.bv_val, op->o_tmpmemctx );
	}
	op->o_bd->bd_info = (BackendInfo *)on;
	return rc;
}

static slap_overinst otp;

int
otp_initialize( void )
{
	ConfigArgs ca;
	char *argv[4];
	int i;

	otp.on_bi.bi_type = "otp";
	otp.on_bi.bi_op_bind = otp_op_bind;

	ca.argv = argv;
	argv[0] = "otp";
	ca.argv = argv;
	ca.argc = 3;
	ca.fname = argv[0];

	argv[3] = NULL;
	for ( i = 0; otp_oid[i].name; i++ ) {
		argv[1] = otp_oid[i].name;
		argv[2] = otp_oid[i].oid;
		parse_oidm( &ca, 0, NULL );
	}

	/* schema integration */
	for ( i = 0; otp_at[i].schema; i++ ) {
		if ( register_at( otp_at[i].schema, otp_at[i].adp, 0 ) ) {
			Debug( LDAP_DEBUG_ANY, "otp_initialize: "
					"register_at failed\n" );
			return -1;
		}
	}

	for ( i = 0; otp_oc[i].schema; i++ ) {
		if ( register_oc( otp_oc[i].schema, otp_oc[i].ocp, 0 ) ) {
			Debug( LDAP_DEBUG_ANY, "otp_initialize: "
					"register_oc failed\n" );
			return -1;
		}
	}

	return overlay_register( &otp );
}

#if SLAPD_OVER_OTP == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return otp_initialize();
}
#endif /* SLAPD_OVER_OTP == SLAPD_MOD_DYNAMIC */

#endif /* defined(SLAPD_OVER_OTP) */
