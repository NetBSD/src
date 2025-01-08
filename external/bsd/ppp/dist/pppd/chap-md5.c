/*	$NetBSD: chap-md5.c,v 1.5 2025/01/08 19:59:38 christos Exp $	*/

/*
 * chap-md5.c - New CHAP/MD5 implementation.
 *
 * Copyright (c) 2003-2024 Paul Mackerras. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: chap-md5.c,v 1.5 2025/01/08 19:59:38 christos Exp $");

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include "pppd-private.h"
#include "chap.h"
#include "chap-md5.h"
#include "magic.h"
#include "crypto.h"

#define MD5_MIN_CHALLENGE	16
#define MD5_MAX_CHALLENGE	24

static void
chap_md5_generate_challenge(unsigned char *cp)
{
	int clen;

	clen = (int)(drand48() * (MD5_MAX_CHALLENGE - MD5_MIN_CHALLENGE))
		+ MD5_MIN_CHALLENGE;
	*cp++ = clen;
	random_bytes(cp, clen);
}

static int
chap_md5_verify_response(int id, char *name,
			 unsigned char *secret, int secret_len,
			 unsigned char *challenge, unsigned char *response,
			 char *message, int message_space)
{
	unsigned char idbyte = id;
	unsigned char hash[MD5_DIGEST_LENGTH];
	unsigned int  hash_len = MD5_DIGEST_LENGTH;
	int challenge_len, response_len;
	bool success = 0;

	challenge_len = *challenge++;
	response_len = *response++;
	if (response_len == MD5_DIGEST_LENGTH) {

		/* Generate hash of ID, secret, challenge */
		PPP_MD_CTX* ctx = PPP_MD_CTX_new();
		if (ctx) {

			if (PPP_DigestInit(ctx, PPP_md5())) {

				if (PPP_DigestUpdate(ctx, &idbyte, 1)) {

					if (PPP_DigestUpdate(ctx, secret, secret_len)) {

						if (PPP_DigestUpdate(ctx, challenge, challenge_len)) {

							if (PPP_DigestFinal(ctx, hash, &hash_len)) {

								success = 1;
							}
						}
					}
				}
			}
			PPP_MD_CTX_free(ctx);
		}
	}
	if (success && memcmp(hash, response, hash_len) == 0) {
		slprintf(message, message_space, "Access granted");
		return 1;
	}
	slprintf(message, message_space, "Access denied");
	return 0;
}

static void
chap_md5_make_response(unsigned char *response, int id, char *our_name,
		       unsigned char *challenge, char *secret, int secret_len,
		       unsigned char *private)
{
	unsigned char idbyte = id;
	int challenge_len = *challenge++;
	int hash_len = MD5_DIGEST_LENGTH;

	response[0] = 0;
	PPP_MD_CTX* ctx = PPP_MD_CTX_new();
	if (ctx) {

		if (PPP_DigestInit(ctx, PPP_md5())) {

			if (PPP_DigestUpdate(ctx, &idbyte, 1)) {

				if (PPP_DigestUpdate(ctx, secret, secret_len)) {

					if (PPP_DigestUpdate(ctx, challenge, challenge_len)) {

						if (PPP_DigestFinal(ctx, &response[1], &hash_len)) {

							response[0] = hash_len;
						}
					}
				}
			}
		}
		PPP_MD_CTX_free(ctx);
	}
	if (response[0] == 0)
		warn("Error occurred in preparing CHAP-Response");
}

static struct chap_digest_type md5_digest = {
	CHAP_MD5,		/* code */
	chap_md5_generate_challenge,
	chap_md5_verify_response,
	chap_md5_make_response,
	NULL,			/* check_success */
	NULL,			/* handle_failure */
};

void
chap_md5_init(void)
{
	chap_register_digest(&md5_digest);
}
