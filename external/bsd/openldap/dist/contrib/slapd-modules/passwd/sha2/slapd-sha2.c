/*	$NetBSD: slapd-sha2.c,v 1.1.1.2 2010/12/12 15:19:14 adam Exp $	*/

/* OpenLDAP: pkg/ldap/contrib/slapd-modules/passwd/sha2/slapd-sha2.c,v 1.1.2.5 2010/04/15 18:40:14 quanah Exp */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2009-2010 The OpenLDAP Foundation.
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
 *
 * Hash methods for passwords generation added by Cédric Delfosse.
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

/* pw_string64 function taken from libraries/liblutil/passwd.c */
static int pw_string64(
	const struct berval *sc,
	const struct berval *hash,
	struct berval *b64,
	const struct berval *salt )
{
	int rc;
	struct berval string;
	size_t b64len;

	if( salt ) {
		/* need to base64 combined string */
		string.bv_len = hash->bv_len + salt->bv_len;
		string.bv_val = ber_memalloc( string.bv_len + 1 );

		if( string.bv_val == NULL ) {
			return LUTIL_PASSWD_ERR;
		}

		AC_MEMCPY( string.bv_val, hash->bv_val,
			hash->bv_len );
		AC_MEMCPY( &string.bv_val[hash->bv_len], salt->bv_val,
			salt->bv_len );
		string.bv_val[string.bv_len] = '\0';

	} else {
		string = *hash;
	}

	b64len = LUTIL_BASE64_ENCODE_LEN( string.bv_len ) + 1;
	b64->bv_len = b64len + sc->bv_len;
	b64->bv_val = ber_memalloc( b64->bv_len + 1 );

	if( b64->bv_val == NULL ) {
		if( salt ) ber_memfree( string.bv_val );
		return LUTIL_PASSWD_ERR;
	}

	AC_MEMCPY(b64->bv_val, sc->bv_val, sc->bv_len);

	rc = lutil_b64_ntop(
		(unsigned char *) string.bv_val, string.bv_len,
		&b64->bv_val[sc->bv_len], b64len );

	if( salt ) ber_memfree( string.bv_val );
	
	if( rc < 0 ) {
		return LUTIL_PASSWD_ERR;
	}

	/* recompute length */
	b64->bv_len = sc->bv_len + rc;
	assert( strlen(b64->bv_val) == b64->bv_len );
	return LUTIL_PASSWD_OK;
}

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

static int hash_sha256(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text )
{
	SHA256_CTX ct;
	unsigned char hash256[SHA256_DIGEST_LENGTH];

	SHA256_Init(&ct);
	SHA256_Update(&ct, (const uint8_t*)passwd->bv_val, passwd->bv_len);
	SHA256_Final(hash256, &ct);

	struct berval digest;
	digest.bv_val = (char *) hash256;
	digest.bv_len = sizeof(hash256);

	return pw_string64(scheme, &digest, hash, NULL);
}

static int hash_sha384(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text )
{
	SHA384_CTX ct;
	unsigned char hash384[SHA384_DIGEST_LENGTH];

#ifdef SLAPD_SHA2_DEBUG
	fprintf(stderr, "hashing password\n");
#endif
	SHA384_Init(&ct);
	SHA384_Update(&ct, (const uint8_t*)passwd->bv_val, passwd->bv_len);
	SHA384_Final(hash384, &ct);

	struct berval digest;
	digest.bv_val = (char *) hash384;
	digest.bv_len = sizeof(hash384);

	return pw_string64(scheme, &digest, hash, NULL);
}

static int hash_sha512(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text )
{
	SHA512_CTX ct;
	unsigned char hash512[SHA512_DIGEST_LENGTH];

	SHA512_Init(&ct);
	SHA512_Update(&ct, (const uint8_t*)passwd->bv_val, passwd->bv_len);
	SHA512_Final(hash512, &ct);

	struct berval digest;
	digest.bv_val = (char *) hash512;
	digest.bv_len = sizeof(hash512);

	return pw_string64(scheme, &digest, hash, NULL);
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
	result = lutil_passwd_add( (struct berval *)&sha256scheme, chk_sha256, hash_sha256 );
	if (result != 0) return result;
	result = lutil_passwd_add( (struct berval *)&sha384scheme, chk_sha384, hash_sha384 );
	if (result != 0) return result;
	result = lutil_passwd_add( (struct berval *)&sha512scheme, chk_sha512, hash_sha512 );
	return result;
}
