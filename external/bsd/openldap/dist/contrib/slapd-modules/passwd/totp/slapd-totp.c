/*	$NetBSD: slapd-totp.c,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/* slapd-totp.c - Password module and overlay for TOTP */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2015-2021 The OpenLDAP Foundation.
 * Portions Copyright 2015 by Howard Chu, Symas Corp.
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

#define TOTP_SHA512_DIGEST_LENGTH	SHA512_DIGEST_LENGTH
#define TOTP_SHA1	EVP_sha1()
#define TOTP_SHA256	EVP_sha256()
#define TOTP_SHA512	EVP_sha512()
#define TOTP_HMAC_CTX	HMAC_CTX *

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static HMAC_CTX *HMAC_CTX_new(void)
{
	HMAC_CTX *ctx = OPENSSL_malloc(sizeof(*ctx));
	if (ctx != NULL) {
		HMAC_CTX_init(ctx);
	}
	return ctx;
}

static void HMAC_CTX_free(HMAC_CTX *ctx)
{
	if (ctx != NULL) {
		HMAC_CTX_cleanup(ctx);
		OPENSSL_free(ctx);
	}
}
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

#define HMAC_setup(ctx, key, len, hash) \
	ctx = HMAC_CTX_new(); \
	HMAC_Init_ex(ctx, key, len, hash, 0)
#define HMAC_crunch(ctx, buf, len)	HMAC_Update(ctx, buf, len)
#define HMAC_finish(ctx, dig, dlen) \
	HMAC_Final(ctx, dig, &dlen); \
	HMAC_CTX_free(ctx)

#elif HAVE_GNUTLS
#include <nettle/hmac.h>

#define TOTP_SHA512_DIGEST_LENGTH	SHA512_DIGEST_SIZE
#define TOTP_SHA1	&nettle_sha1
#define TOTP_SHA256	&nettle_sha256
#define TOTP_SHA512	&nettle_sha512
#define TOTP_HMAC_CTX	struct hmac_sha512_ctx

#define HMAC_setup(ctx, key, len, hash)	\
	const struct nettle_hash *h=hash;\
	hmac_set_key(&ctx.outer, &ctx.inner, &ctx.state, h, len, key)
#define HMAC_crunch(ctx, buf, len)	hmac_update(&ctx.state, h, len, buf)
#define HMAC_finish(ctx, dig, dlen) \
	hmac_digest(&ctx.outer, &ctx.inner, &ctx.state, h, h->digest_size, dig);\
	dlen = h->digest_size

#else
# error Unsupported crypto backend.
#endif

#include "slap.h"
#include "slap-config.h"

static LUTIL_PASSWD_CHK_FUNC chk_totp1, chk_totp256, chk_totp512,
	chk_totp1andpw, chk_totp256andpw, chk_totp512andpw;
static LUTIL_PASSWD_HASH_FUNC hash_totp1, hash_totp256, hash_totp512,
	hash_totp1andpw, hash_totp256andpw, hash_totp512andpw;
static const struct berval scheme_totp1 = BER_BVC("{TOTP1}");
static const struct berval scheme_totp256 = BER_BVC("{TOTP256}");
static const struct berval scheme_totp512 = BER_BVC("{TOTP512}");
static const struct berval scheme_totp1andpw = BER_BVC("{TOTP1ANDPW}");
static const struct berval scheme_totp256andpw = BER_BVC("{TOTP256ANDPW}");
static const struct berval scheme_totp512andpw = BER_BVC("{TOTP512ANDPW}");

static AttributeDescription *ad_authTimestamp;

/* This is the definition used by ISODE, as supplied to us in
 * ITS#6238 Followup #9
 */
static struct schema_info {
	char *def;
	AttributeDescription **ad;
} totp_OpSchema[] = {
	{	"( 1.3.6.1.4.1.453.16.2.188 "
		"NAME 'authTimestamp' "
		"DESC 'last successful authentication using any method/mech' "
		"EQUALITY generalizedTimeMatch "
		"ORDERING generalizedTimeOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
		"SINGLE-VALUE NO-USER-MODIFICATION USAGE dsaOperation )",
		&ad_authTimestamp},
	{ NULL, NULL }
};

/* RFC3548 base32 encoding/decoding */

static const char Base32[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
static const char Pad32 = '=';

static int
totp_b32_ntop(
	u_char const *src,
	size_t srclength,
	char *target,
	size_t targsize)
{
	size_t datalength = 0;
	u_char input0;
	u_int input1;	/* assumed to be at least 32 bits */
	u_char output[8];
	int i;

	while (4 < srclength) {
		if (datalength + 8 > targsize)
			return (-1);
		input0 = *src++;
		input1 = *src++;
		input1 <<= 8;
		input1 |= *src++;
		input1 <<= 8;
		input1 |= *src++;
		input1 <<= 8;
		input1 |= *src++;
		srclength -= 5;

		for (i=7; i>1; i--) {
			output[i] = input1 & 0x1f;
			input1 >>= 5;
		}
		output[0] = input0 >> 3;
		output[1] = (input0 & 0x07) << 2 | input1;

		for (i=0; i<8; i++)
			target[datalength++] = Base32[output[i]];
	}
    
	/* Now we worry about padding. */
	if (0 != srclength) {
		static const int outlen[] = { 2,4,5,7 };
		int n;
		if (datalength + 8 > targsize)
			return (-1);

		/* Get what's left. */
		input1 = *src++;
		for (i = 1; i < srclength; i++) {
			input1 <<= 8;
			input1 |= *src++;
		}
		input1 <<= 8 * (4-srclength);
		n = outlen[srclength-1];
		for (i=0; i<n; i++) {
			target[datalength++] = Base32[(input1 & 0xf8000000) >> 27];
			input1 <<= 5;
		}
		for (; i<8; i++)
			target[datalength++] = Pad32;
	}
	if (datalength >= targsize)
		return (-1);
	target[datalength] = '\0';	/* Returned value doesn't count \0. */
	return (datalength);
}

/* converts characters, eight at a time, starting at src
   from base - 32 numbers into five 8 bit bytes in the target area.
   it returns the number of data bytes stored at the target, or -1 on error.
 */

static int
totp_b32_pton(
	char const *src,
	u_char *target, 
	size_t targsize)
{
	int tarindex, state, ch;
	char *pos;

	state = 0;
	tarindex = 0;

	while ((ch = *src++) != '\0') {
		if (ch == Pad32)
			break;

		pos = strchr(Base32, ch);
		if (pos == 0) 		/* A non-base32 character. */
			return (-1);

		switch (state) {
		case 0:
			if (target) {
				if ((size_t)tarindex >= targsize)
					return (-1);
				target[tarindex] = (pos - Base32) << 3;
			}
			state = 1;
			break;
		case 1:
			if (target) {
				if ((size_t)tarindex + 1 >= targsize)
					return (-1);
				target[tarindex]   |=  (pos - Base32) >> 2;
				target[tarindex+1]  = ((pos - Base32) & 0x3)
							<< 6 ;
			}
			tarindex++;
			state = 2;
			break;
		case 2:
			if (target) {
				target[tarindex]   |=  (pos - Base32) << 1;
			}
			state = 3;
			break;
		case 3:
			if (target) {
				if ((size_t)tarindex + 1 >= targsize)
					return (-1);
				target[tarindex] |= (pos - Base32) >> 4;
				target[tarindex+1]  = ((pos - Base32) & 0xf)
							<< 4 ;
			}
			tarindex++;
			state = 4;
			break;
		case 4:
			if (target) {
				if ((size_t)tarindex + 1 >= targsize)
					return (-1);
				target[tarindex] |= (pos - Base32) >> 1;
				target[tarindex+1]  = ((pos - Base32) & 0x1)
							<< 7 ;
			}
			tarindex++;
			state = 5;
			break;
		case 5:
			if (target) {
				target[tarindex]   |=  (pos - Base32) << 2;
			}
			state = 6;
			break;
		case 6:
			if (target) {
				if ((size_t)tarindex + 1 >= targsize)
					return (-1);
				target[tarindex] |= (pos - Base32) >> 3;
				target[tarindex+1]  = ((pos - Base32) & 0x7)
							<< 5 ;
			}
			tarindex++;
			state = 7;
			break;
		case 7:
			if (target) {
				target[tarindex]   |=  (pos - Base32);
			}
			state = 0;
			tarindex++;
			break;

		default:
			abort();
		}
	}

	/*
	 * We are done decoding Base-32 chars.  Let's see if we ended
	 * on a byte boundary, and/or with erroneous trailing characters.
	 */

	if (ch == Pad32) {		/* We got a pad char. */
		int i = 0;

		/* count pad chars */
		for (; ch; ch = *src++) {
			if (ch != Pad32)
				return (-1);
			i++;
		}
		/* there are only 4 valid ending states with a
		 * pad character, make sure the number of pads is valid.
		 */
		switch(state) {
		case 2:	if (i != 6) return -1;
			break;
		case 4: if (i != 4) return -1;
			break;
		case 5: if (i != 3) return -1;
			break;
		case 7: if (i != 1) return -1;
			break;
		default:
			return -1;
		}
		/*
		 * Now make sure that the "extra" bits that slopped past
		 * the last full byte were zeros.  If we don't check them,
		 * they become a subliminal channel.
		 */
		if (target && target[tarindex] != 0)
			return (-1);
	} else {
		/*
		 * We ended by seeing the end of the string.  Make sure we
		 * have no partial bytes lying around.
		 */
		if (state != 0)
			return (-1);
	}

	return (tarindex);
}

/* RFC6238 TOTP */


typedef struct myval {
	ber_len_t mv_len;
	void *mv_val;
} myval;

static void do_hmac(
	const void *hash,
	myval *key,
	myval *data,
	myval *out)
{
	TOTP_HMAC_CTX ctx;
	unsigned int digestLen;

	HMAC_setup(ctx, key->mv_val, key->mv_len, hash);
	HMAC_crunch(ctx, data->mv_val, data->mv_len);
	HMAC_finish(ctx, out->mv_val, digestLen);
	out->mv_len = digestLen;
}

static const int DIGITS_POWER[] = {
	1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };

static void generate(
	myval *key,
	uint64_t tval,
	int digits,
	myval *out,
	const void *mech)
{
	unsigned char digest[TOTP_SHA512_DIGEST_LENGTH];
	myval digval;
	myval data;
	unsigned char msg[8];
	int i, offset, res, otp;

#if WORDS_BIGENDIAN
	*(uint64_t *)msg = tval;
#else
	for (i=7; i>=0; i--) {
		msg[i] = tval & 0xff;
		tval >>= 8;
	}
#endif

	data.mv_val = msg;
	data.mv_len = sizeof(msg);

	digval.mv_val = digest;
	digval.mv_len = sizeof(digest);
	do_hmac(mech, key, &data, &digval);

	offset = digest[digval.mv_len-1] & 0xf;
	res = ((digest[offset] & 0x7f) << 24) |
			((digest[offset+1] & 0xff) << 16) |
			((digest[offset+2] & 0xff) << 8) |
			(digest[offset+3] & 0xff);

	otp = res % DIGITS_POWER[digits];
	out->mv_len = snprintf(out->mv_val, out->mv_len, "%0*d", digits, otp);
}

static int totp_op_cleanup( Operation *op, SlapReply *rs );
static int totp_bind_response( Operation *op, SlapReply *rs );

#define TIME_STEP	30
#define DIGITS	6
#define DELIM	'|'	/* a single character */
#define TOTP_AND_PW_HASH_SCHEME	"{SSHA}"

static int chk_totp(
	const struct berval *passwd,
	const struct berval *cred,
	const void *mech,
	const char **text)
{
	void *ctx, *op_tmp;
	Operation *op;
	Entry *e;
	Attribute *a;
	long t, told = 0;
	int rc;
	myval out, key;
	char outbuf[32];

	/* Find our thread context, find our Operation */
	ctx = ldap_pvt_thread_pool_context();
	if (ldap_pvt_thread_pool_getkey(ctx, totp_op_cleanup, &op_tmp, NULL) ||
		!op_tmp)
		return LUTIL_PASSWD_ERR;
	op = op_tmp;

	rc = be_entry_get_rw(op, &op->o_req_ndn, NULL, NULL, 0, &e);
	if (rc != LDAP_SUCCESS) return LUTIL_PASSWD_ERR;

	/* Make sure previous login is older than current time */
	t = op->o_time / TIME_STEP;
	a = attr_find(e->e_attrs, ad_authTimestamp);
	if (a) {
		struct lutil_tm tm;
		struct lutil_timet tt;
		if (lutil_parsetime(a->a_vals[0].bv_val, &tm) == 0 &&
			lutil_tm2time(&tm, &tt) == 0) {
			told = tt.tt_sec / TIME_STEP;
			if (told >= t)
				rc = LUTIL_PASSWD_ERR;
		}
		if (!rc) {	/* seems OK, remember old stamp */
			slap_callback *sc;
			for (sc = op->o_callback; sc; sc = sc->sc_next) {
				if (sc->sc_response == totp_bind_response) {
					sc->sc_private = ber_dupbv_x(NULL, &a->a_vals[0], op->o_tmpmemctx);
					break;
				}
			}
		}
	}	/* else no previous login, 1st use is OK */

	be_entry_release_r(op, e);
	if (rc) return rc;

	/* Key is stored in base32 */
	key.mv_len = passwd->bv_len * 5 / 8;
	key.mv_val = ber_memalloc(key.mv_len+1);

	if (!key.mv_val)
		return LUTIL_PASSWD_ERR;

	rc = totp_b32_pton(passwd->bv_val, key.mv_val, key.mv_len);
	if (rc < 1) {
		rc = LUTIL_PASSWD_ERR;
		goto out;
	}

	out.mv_val = outbuf;
	out.mv_len = sizeof(outbuf);
	generate(&key, t, DIGITS, &out, mech);

	/* compare */
	if (out.mv_len != cred->bv_len) {
		rc = LUTIL_PASSWD_ERR;
		goto out;
	}

	rc = memcmp(out.mv_val, cred->bv_val, out.mv_len) ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;

	/* If current value doesn't match, try again with previous value
	 * but only if the most recent login is older than the previous
	 * time step but still set */
	if (rc == LUTIL_PASSWD_ERR && told < t - 1 && told > 0) {
		out.mv_val = outbuf;
		out.mv_len = sizeof(outbuf);
		generate(&key, t - 1, DIGITS, &out, mech);
		/* compare */
		if (out.mv_len != cred->bv_len)
			goto out;
		rc = memcmp(out.mv_val, cred->bv_val, out.mv_len) ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;
	}

out:
	memset(key.mv_val, 0, key.mv_len);
	ber_memfree(key.mv_val);
	return rc;
}

static int chk_totp_and_pw(
	const struct berval *scheme,
	const struct berval *passwd,
	const struct berval *cred,
	const char **text,
	const void *mech)
{
	char *s;
	int rc = LUTIL_PASSWD_ERR, rc_pass, rc_otp;
	ber_len_t len;
	struct berval cred_pass, cred_otp, passwd_pass, passwd_otp;

	/* Check credential length, no point to continue if too short */
	if (cred->bv_len <= DIGITS)
		return rc;

	/* The OTP seed of the stored password */
	s = strchr(passwd->bv_val, DELIM);
	if (s) {
		len = s - passwd->bv_val;
	} else {
		return rc;
	}
	if (!ber_str2bv(passwd->bv_val, len, 1, &passwd_otp))
		return rc;

	/* The password part of the stored password */
	s++;
	ber_str2bv(s, 0, 0, &passwd_pass);

	/* The OTP part of the entered credential */
	ber_str2bv(&cred->bv_val[cred->bv_len - DIGITS], DIGITS, 0, &cred_otp);

	/* The password part of the entered credential */
	if (!ber_str2bv(cred->bv_val, cred->bv_len - DIGITS, 0, &cred_pass)) {
		/* Cleanup */
		memset(passwd_otp.bv_val, 0, passwd_otp.bv_len);
		ber_memfree(passwd_otp.bv_val);
		return rc;
	}

	rc_otp = chk_totp(&passwd_otp, &cred_otp, mech, text);
	rc_pass = lutil_passwd(&passwd_pass, &cred_pass, NULL, text);
	if (rc_otp == LUTIL_PASSWD_OK && rc_pass == LUTIL_PASSWD_OK)
		rc = LUTIL_PASSWD_OK;

	/* Cleanup and return */
	memset(passwd_otp.bv_val, 0, passwd_otp.bv_len);
	ber_memfree(passwd_otp.bv_val);

	return rc;
}

static int chk_totp1(
	const struct berval *scheme,
	const struct berval *passwd,
	const struct berval *cred,
	const char **text)
{
	return chk_totp(passwd, cred, TOTP_SHA1, text);
}

static int chk_totp256(
	const struct berval *scheme,
	const struct berval *passwd,
	const struct berval *cred,
	const char **text)
{
	return chk_totp(passwd, cred, TOTP_SHA256, text);
}

static int chk_totp512(
	const struct berval *scheme,
	const struct berval *passwd,
	const struct berval *cred,
	const char **text)
{
	return chk_totp(passwd, cred, TOTP_SHA512, text);
}

static int chk_totp1andpw(
	const struct berval *scheme,
	const struct berval *passwd,
	const struct berval *cred,
	const char **text)
{
	return chk_totp_and_pw(scheme, passwd, cred, text, TOTP_SHA1);
}

static int chk_totp256andpw(
	const struct berval *scheme,
	const struct berval *passwd,
	const struct berval *cred,
	const char **text)
{
	return chk_totp_and_pw(scheme, passwd, cred, text, TOTP_SHA256);
}

static int chk_totp512andpw(
	const struct berval *scheme,
	const struct berval *passwd,
	const struct berval *cred,
	const char **text)
{
	return chk_totp_and_pw(scheme, passwd, cred, text, TOTP_SHA512);
}

static int passwd_string32(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash)
{
	int b32len = (passwd->bv_len + 4)/5 * 8;
	int rc;
	hash->bv_len = scheme->bv_len + b32len;
	hash->bv_val = ber_memalloc(hash->bv_len + 1);
	AC_MEMCPY(hash->bv_val, scheme->bv_val, scheme->bv_len);
	rc = totp_b32_ntop((unsigned char *)passwd->bv_val, passwd->bv_len,
		hash->bv_val + scheme->bv_len, b32len+1);
	if (rc < 0) {
		ber_memfree(hash->bv_val);
		hash->bv_val = NULL;
		return LUTIL_PASSWD_ERR;
	}
	return LUTIL_PASSWD_OK;
}

static int hash_totp_and_pw(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text)
{
	struct berval otp, pass, hash_otp, hash_pass;
	ber_len_t len;
	char *s;
	int rc = LUTIL_PASSWD_ERR;

	/* The OTP seed part */
	s = strchr(passwd->bv_val, DELIM);
	if (s) {
		len = s - passwd->bv_val;
	} else {
		return rc;
	}
	if (!ber_str2bv(passwd->bv_val, len, 0, &otp))
		return rc;

	/* The static password part */
	s++;
	ber_str2bv(s, 0, 0, &pass);

	/* Hash the OTP seed */
	rc = passwd_string32(scheme, &otp, &hash_otp);

	/* If successful, hash the static password, else cleanup and return */
	if (rc == LUTIL_PASSWD_OK) {
		rc = lutil_passwd_hash(&pass, TOTP_AND_PW_HASH_SCHEME,
			&hash_pass, text);
	} else {
		return LUTIL_PASSWD_ERR;
	}

	/* If successful, allocate memory to combine them, else cleanup
	 *  and return */
	if (rc == LUTIL_PASSWD_OK) {
		/* Add 1 character to bv_len to hold DELIM */
		hash->bv_len = hash_pass.bv_len + hash_otp.bv_len + 1;
		hash->bv_val = ber_memalloc(hash->bv_len + 1);
		if (!hash->bv_val)
			rc = LUTIL_PASSWD_ERR;
	} else {
		memset(hash_otp.bv_val, 0, hash_otp.bv_len);
		ber_memfree(hash_otp.bv_val);
		return LUTIL_PASSWD_ERR;
	}

	/* If successful, combine the two hashes with the delimiter */
	if (rc == LUTIL_PASSWD_OK) {
		AC_MEMCPY(hash->bv_val, hash_otp.bv_val, hash_otp.bv_len);
		hash->bv_val[hash_otp.bv_len] = DELIM;
		AC_MEMCPY(hash->bv_val + hash_otp.bv_len + 1,
			hash_pass.bv_val, hash_pass.bv_len);
		hash->bv_val[hash->bv_len] = '\0';
	}

	/* Cleanup and return */
	memset(hash_otp.bv_val, 0, hash_otp.bv_len);
	memset(hash_pass.bv_val, 0, hash_pass.bv_len);
	ber_memfree(hash_otp.bv_val);
	ber_memfree(hash_pass.bv_val);

	return rc;
}

static int hash_totp1(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text)
{
#if 0
	if (passwd->bv_len != SHA_DIGEST_LENGTH) {
		*text = "invalid key length";
		return LUTIL_PASSWD_ERR;
	}
#endif
	return passwd_string32(scheme, passwd, hash);
}

static int hash_totp256(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text)
{
#if 0
	if (passwd->bv_len != SHA256_DIGEST_LENGTH) {
		*text = "invalid key length";
		return LUTIL_PASSWD_ERR;
	}
#endif
	return passwd_string32(scheme, passwd, hash);
}

static int hash_totp512(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text)
{
#if 0
	if (passwd->bv_len != SHA512_DIGEST_LENGTH) {
		*text = "invalid key length";
		return LUTIL_PASSWD_ERR;
	}
#endif
	return passwd_string32(scheme, passwd, hash);
}

static int hash_totp1andpw(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text)
{
#if 0
	if (passwd->bv_len != SHA_DIGEST_LENGTH) {
		*text = "invalid key length";
		return LUTIL_PASSWD_ERR;
	}
#endif
	return hash_totp_and_pw(scheme, passwd, hash, text);
}

static int hash_totp256andpw(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text)
{
#if 0
	if (passwd->bv_len != SHA256_DIGEST_LENGTH) {
		*text = "invalid key length";
		return LUTIL_PASSWD_ERR;
	}
#endif
	return hash_totp_and_pw(scheme, passwd, hash, text);
}

static int hash_totp512andpw(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text)
{
#if 0
	if (passwd->bv_len != SHA512_DIGEST_LENGTH) {
		*text = "invalid key length";
		return LUTIL_PASSWD_ERR;
	}
#endif
	return hash_totp_and_pw(scheme, passwd, hash, text);
}

static int totp_op_cleanup(
	Operation *op,
	SlapReply *rs )
{
	slap_callback *cb;

	/* clear out the current key */
	ldap_pvt_thread_pool_setkey( op->o_threadctx, totp_op_cleanup,
		NULL, 0, NULL, NULL );

	/* free the callback */
	cb = op->o_callback;
	op->o_callback = cb->sc_next;
	if (cb->sc_private)
		ber_bvfree_x(cb->sc_private, op->o_tmpmemctx);
	op->o_tmpfree( cb, op->o_tmpmemctx );
	return 0;
}

static int
totp_bind_response( Operation *op, SlapReply *rs )
{
	Modifications *mod = NULL;
	BackendInfo *bi = op->o_bd->bd_info;
	Entry *e;
	int rc;

	/* we're only interested if the bind was successful */
	if ( rs->sr_err != LDAP_SUCCESS )
		return SLAP_CB_CONTINUE;

	rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &e );
	op->o_bd->bd_info = bi;

	if ( rc != LDAP_SUCCESS ) {
		return SLAP_CB_CONTINUE;
	}

	{
		time_t now;
		Attribute *a;
		Modifications *m;
		char nowstr[ LDAP_LUTIL_GENTIME_BUFSIZE ];
		struct berval timestamp;

		/* get the current time */
		now = op->o_time;

		/* update the authTimestamp in the user's entry with the current time */
		timestamp.bv_val = nowstr;
		timestamp.bv_len = sizeof(nowstr);
		slap_timestamp( &now, &timestamp );

		m = ch_calloc( sizeof(Modifications), 1 );
		m->sml_op = LDAP_MOD_REPLACE;
		m->sml_flags = 0;
		m->sml_type = ad_authTimestamp->ad_cname;
		m->sml_desc = ad_authTimestamp;
		m->sml_numvals = 1;
		m->sml_values = ch_calloc( sizeof(struct berval), 2 );
		m->sml_nvalues = ch_calloc( sizeof(struct berval), 2 );

		ber_dupbv( &m->sml_values[0], &timestamp );
		ber_dupbv( &m->sml_nvalues[0], &timestamp );
		m->sml_next = mod;
		mod = m;

		/* get authTimestamp attribute, if it exists */
		if ((a = attr_find( e->e_attrs, ad_authTimestamp)) != NULL && op->o_callback->sc_private) {
			struct berval *bv = op->o_callback->sc_private;
			m = ch_calloc( sizeof(Modifications), 1 );
			m->sml_op = LDAP_MOD_DELETE;
			m->sml_flags = 0;
			m->sml_type = ad_authTimestamp->ad_cname;
			m->sml_desc = ad_authTimestamp;
			m->sml_numvals = 1;
			m->sml_values = ch_calloc( sizeof(struct berval), 2 );
			m->sml_nvalues = ch_calloc( sizeof(struct berval), 2 );

			ber_dupbv( &m->sml_values[0], bv );
			ber_dupbv( &m->sml_nvalues[0], bv );
			m->sml_next = mod;
			mod = m;
		}
	}

	be_entry_release_r( op, e );

	/* perform the update */
	if ( mod ) {
		Operation op2 = *op;
		SlapReply r2 = { REP_RESULT };
		slap_callback cb = { NULL, slap_null_cb, NULL, NULL };

		/* This is a DSA-specific opattr, it never gets replicated. */
		op2.o_tag = LDAP_REQ_MODIFY;
		op2.o_callback = &cb;
		op2.orm_modlist = mod;
		op2.o_dn = op->o_bd->be_rootdn;
		op2.o_ndn = op->o_bd->be_rootndn;
		op2.o_dont_replicate = 1;
		rc = op->o_bd->be_modify( &op2, &r2 );
		slap_mods_free( mod, 1 );
		if (rc != LDAP_SUCCESS) {
			/* slapd has logged this as a success already, but we
			 * need to fail it because the authTimestamp changed
			 * out from under us.
			 */
			rs->sr_err = LDAP_INVALID_CREDENTIALS;
			connection2anonymous(op->o_conn);
			op2 = *op;
			op2.o_callback = NULL;
			send_ldap_result(&op2, rs);
			op->o_bd->bd_info = bi;
			return rs->sr_err;
		}
	}

	op->o_bd->bd_info = bi;
	return SLAP_CB_CONTINUE;
}

static int totp_op_bind(
	Operation *op,
	SlapReply *rs )
{
	/* If this is a simple Bind, stash the Op pointer so our chk
	 * function can find it. Set a cleanup callback to clear it
	 * out when the Bind completes.
	 */
	if ( op->oq_bind.rb_method == LDAP_AUTH_SIMPLE ) {
		slap_callback *cb;
		ldap_pvt_thread_pool_setkey( op->o_threadctx,
			totp_op_cleanup, op, 0, NULL, NULL );
		cb = op->o_tmpcalloc( 1, sizeof(slap_callback), op->o_tmpmemctx );
		cb->sc_response = totp_bind_response;
		cb->sc_cleanup = totp_op_cleanup;
		cb->sc_next = op->o_callback;
		op->o_callback = cb;
	}
	return SLAP_CB_CONTINUE;
}

static int totp_db_open(
	BackendDB *be,
	ConfigReply *cr
)
{
	int rc = 0;

	if (!ad_authTimestamp) {
		const char *text = NULL;
		rc = slap_str2ad("authTimestamp", &ad_authTimestamp, &text);
		if (rc) {
			rc = register_at(totp_OpSchema[0].def, totp_OpSchema[0].ad, 0 );
			if (rc) {
				snprintf(cr->msg, sizeof(cr->msg), "unable to find or register authTimestamp attribute: %s (%d)",
					text, rc);
				Debug(LDAP_DEBUG_ANY, "totp: %s.\n", cr->msg );
			}
			ad_authTimestamp->ad_type->sat_flags |= SLAP_AT_MANAGEABLE;
		}
	}
	return rc;
}

static slap_overinst totp;

int
totp_initialize(void)
{
	int rc;

	totp.on_bi.bi_type = "totp";

	totp.on_bi.bi_db_open = totp_db_open;
	totp.on_bi.bi_op_bind = totp_op_bind;

	rc = lutil_passwd_add((struct berval *) &scheme_totp1, chk_totp1, hash_totp1);
	if (!rc)
		rc = lutil_passwd_add((struct berval *) &scheme_totp256, chk_totp256, hash_totp256);
	if (!rc)
		rc = lutil_passwd_add((struct berval *) &scheme_totp512, chk_totp512, hash_totp512);
	if (!rc)
		rc = lutil_passwd_add((struct berval *) &scheme_totp1andpw, chk_totp1andpw, hash_totp1andpw);
	if (!rc)
		rc = lutil_passwd_add((struct berval *) &scheme_totp256andpw, chk_totp256andpw, hash_totp256andpw);
	if (!rc)
		rc = lutil_passwd_add((struct berval *) &scheme_totp512andpw, chk_totp512andpw, hash_totp512andpw);
	if (rc)
		return rc;

	return overlay_register(&totp);
}

int init_module(int argc, char *argv[]) {
	return totp_initialize();
}
