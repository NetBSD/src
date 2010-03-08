/*	$NetBSD: slapd-sha2.c,v 1.1.1.1 2010/03/08 02:14:15 lukem Exp $	*/

/* OpenLDAP: pkg/ldap/contrib/slapd-modules/passwd/sha2/slapd-sha2.c,v 1.1.2.3 2009/08/17 22:57:54 quanah Exp */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2009 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENT:
 * This work was initially developed by Jeff Turner for inclusion
 * in OpenLDAP Software.
 */

#include <lber.h>
#include <lber_pvt.h>
#include <ac/string.h>
#include "lutil.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "sha2.h"

#ifdef SLAPD_SHA2_DEBUG
#include <stdio.h>
#endif

char * sha256_hex_hash(const char * passwd) {

	SHA256_CTX ct;
	unsigned char hash[SHA256_DIGEST_LENGTH];
	static char real_hash[LUTIL_BASE64_ENCODE_LEN(SHA256_DIGEST_LENGTH)+1]; // extra char for \0

	SHA256_Init(&ct);
	SHA256_Update(&ct, (const uint8_t*)passwd, strlen(passwd));
	SHA256_Final(hash, &ct);

        /* base64 encode it */
	lutil_b64_ntop(
			hash,
			SHA256_DIGEST_LENGTH,
			real_hash,
			LUTIL_BASE64_ENCODE_LEN(SHA256_DIGEST_LENGTH)+1
			);

	return real_hash;
}


char * sha384_hex_hash(const char * passwd) {

	SHA384_CTX ct;
	unsigned char hash[SHA384_DIGEST_LENGTH];
	static char real_hash[LUTIL_BASE64_ENCODE_LEN(SHA384_DIGEST_LENGTH)+1]; // extra char for \0

	SHA384_Init(&ct);
	SHA384_Update(&ct, (const uint8_t*)passwd, strlen(passwd));
	SHA384_Final(hash, &ct);

        /* base64 encode it */
	lutil_b64_ntop(
			hash,
			SHA384_DIGEST_LENGTH,
			real_hash,
			LUTIL_BASE64_ENCODE_LEN(SHA384_DIGEST_LENGTH)+1
			);

	return real_hash;
}

char * sha512_hex_hash(const char * passwd) {

	SHA512_CTX ct;
	unsigned char hash[SHA512_DIGEST_LENGTH];
	static char real_hash[LUTIL_BASE64_ENCODE_LEN(SHA512_DIGEST_LENGTH)+1]; // extra char for \0

	SHA512_Init(&ct);
	SHA512_Update(&ct, (const uint8_t*)passwd, strlen(passwd));
	SHA512_Final(hash, &ct);

        /* base64 encode it */
	lutil_b64_ntop(
			hash,
			SHA512_DIGEST_LENGTH,
			real_hash,
			LUTIL_BASE64_ENCODE_LEN(SHA512_DIGEST_LENGTH)+1
			);

	return real_hash;
}

static int chk_sha256(
	const struct berval *scheme, // Scheme of hashed reference password
	const struct berval *passwd, // Hashed reference password to check against
	const struct berval *cred, // user-supplied password to check
	const char **text )
{
#ifdef SLAPD_SHA2_DEBUG
	fprintf(stderr, "Validating password\n");
	fprintf(stderr, "  Password to validate: %s\n", cred->bv_val);
	fprintf(stderr, "  Hashes to: %s\n", sha256_hex_hash(cred->bv_val));
	fprintf(stderr, "  Stored password scheme: %s\n", scheme->bv_val);
	fprintf(stderr, "  Stored password value: %s\n", passwd->bv_val);
	fprintf(stderr, "  -> Passwords %s\n", strcmp(sha256_hex_hash(cred->bv_val), passwd->bv_val) == 0 ? "match" : "do not match");
#endif
	return (strcmp(sha256_hex_hash(cred->bv_val), passwd->bv_val));
}

static int chk_sha384(
	const struct berval *scheme, // Scheme of hashed reference password
	const struct berval *passwd, // Hashed reference password to check against
	const struct berval *cred, // user-supplied password to check
	const char **text )
{
#ifdef SLAPD_SHA2_DEBUG
	fprintf(stderr, "Validating password\n");
	fprintf(stderr, "  Password to validate: %s\n", cred->bv_val);
	fprintf(stderr, "  Hashes to: %s\n", sha384_hex_hash(cred->bv_val));
	fprintf(stderr, "  Stored password scheme: %s\n", scheme->bv_val);
	fprintf(stderr, "  Stored password value: %s\n", passwd->bv_val);
	fprintf(stderr, "  -> Passwords %s\n", strcmp(sha384_hex_hash(cred->bv_val), passwd->bv_val) == 0 ? "match" : "do not match");
#endif
	return (strcmp(sha384_hex_hash(cred->bv_val), passwd->bv_val));
}

static int chk_sha512(
	const struct berval *scheme, // Scheme of hashed reference password
	const struct berval *passwd, // Hashed reference password to check against
	const struct berval *cred, // user-supplied password to check
	const char **text )
{
#ifdef SLAPD_SHA2_DEBUG
	fprintf(stderr, "  Password to validate: %s\n", cred->bv_val);
	fprintf(stderr, "  Hashes to: %s\n", sha512_hex_hash(cred->bv_val));
	fprintf(stderr, "  Stored password scheme: %s\n", scheme->bv_val);
	fprintf(stderr, "  Stored password value: %s\n", passwd->bv_val);
	fprintf(stderr, "  -> Passwords %s\n", strcmp(sha512_hex_hash(cred->bv_val), passwd->bv_val) == 0 ? "match" : "do not match");
#endif
	return (strcmp(sha512_hex_hash(cred->bv_val), passwd->bv_val));
}

const struct berval sha256scheme = BER_BVC("{SHA256}");
const struct berval sha384scheme = BER_BVC("{SHA384}");
const struct berval sha512scheme = BER_BVC("{SHA512}");

int init_module(int argc, char *argv[]) {
	int result = 0;
	result = lutil_passwd_add( (struct berval *)&sha256scheme, chk_sha256, NULL );
	if (result != 0) return result;
	result = lutil_passwd_add( (struct berval *)&sha384scheme, chk_sha384, NULL );
	if (result != 0) return result;
	result = lutil_passwd_add( (struct berval *)&sha512scheme, chk_sha512, NULL );
	return result;
}
