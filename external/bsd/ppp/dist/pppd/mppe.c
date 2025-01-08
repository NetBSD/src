/* mppe.c - MPPE key implementation
 *
 * Copyright (c) 2020 Eivind Naess. All rights reserved.
 * Copyright (c) 2008-2024 Paul Mackerras. All rights reserved.
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
 * 3. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>


#include "pppd-private.h"
#include "fsm.h"
#include "ccp.h"
#include "chap_ms.h"
#include "mppe.h"
#include "crypto.h"

u_char mppe_send_key[MPPE_MAX_KEY_SIZE];
u_char mppe_recv_key[MPPE_MAX_KEY_SIZE];
int mppe_keys_set = 0;

void
mppe_set_keys(u_char *send_key, u_char *recv_key, int keylen)
{
	int length = keylen;
	if (length > MPPE_MAX_KEY_SIZE)
		length = MPPE_MAX_KEY_SIZE;
	
	if (send_key) {
	    BCOPY(send_key, mppe_send_key, length);
	    BZERO(send_key, keylen);
	}

	if (recv_key) {
	    BCOPY(recv_key, mppe_recv_key, length);
	    BZERO(recv_key, keylen);
	}

	mppe_keys_set = length;
}

bool
mppe_keys_isset()
{
	return !!mppe_keys_set;
}

int
mppe_get_recv_key(u_char *recv_key, int length)
{
	if (mppe_keys_isset()) {
		if (length > mppe_keys_set)
			length = mppe_keys_set;
		BCOPY(mppe_recv_key, recv_key, length);
		return length;
	}
	return 0;
}

int
mppe_get_send_key(u_char *send_key, int length)
{
	if (mppe_keys_isset()) {
		if (length > mppe_keys_set)
			length = mppe_keys_set;
		BCOPY(mppe_send_key, send_key, length);
		return length;
	}
	return 0;
}

void
mppe_clear_keys(void)
{
	mppe_keys_set = 0;
	BZERO(mppe_send_key, sizeof(mppe_send_key));
	BZERO(mppe_recv_key, sizeof(mppe_recv_key));
}

/*
 * Set mppe_xxxx_key from the NTPasswordHashHash.
 * RFC 2548 (RADIUS support) requires us to export this function (ugh).
 */
void
mppe_set_chapv1(unsigned char *rchallenge, unsigned char *PasswordHashHash)
{
    PPP_MD_CTX *ctx;
    u_char Digest[SHA_DIGEST_LENGTH];
    int DigestLen;

    ctx = PPP_MD_CTX_new();
    if (ctx != NULL) {

        if (PPP_DigestInit(ctx, PPP_sha1())) {

            if (PPP_DigestUpdate(ctx, PasswordHashHash, MD4_DIGEST_LENGTH)) {

                if (PPP_DigestUpdate(ctx, PasswordHashHash, MD4_DIGEST_LENGTH)) {

                    if (PPP_DigestUpdate(ctx, rchallenge, 8)) {
                        
                        DigestLen = SHA_DIGEST_LENGTH;
                        PPP_DigestFinal(ctx, Digest, &DigestLen);
                    }
                }
            }
        }
        
        PPP_MD_CTX_free(ctx);
    }


    /* Same key in both directions. */
    mppe_set_keys(Digest, Digest, sizeof(Digest));
}

/*
 * Set mppe_xxxx_key from MS-CHAPv2 credentials. (see RFC 3079)
 *
 * This helper function used in the Winbind module, which gets the
 * NTHashHash from the server.
 */
void
mppe_set_chapv2(unsigned char *PasswordHashHash, unsigned char *NTResponse,
        int IsServer)
{
    PPP_MD_CTX *ctx;
    
    u_char	MasterKey[SHA_DIGEST_LENGTH];
    u_char	SendKey[SHA_DIGEST_LENGTH];
    u_char	RecvKey[SHA_DIGEST_LENGTH];
    int KeyLen;

    u_char SHApad1[40] =
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    u_char SHApad2[40] =
	{ 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2,
	  0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2,
	  0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2,
	  0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2 };

    /* "This is the MPPE Master Key" */
    u_char Magic1[27] =
	{ 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74,
	  0x68, 0x65, 0x20, 0x4d, 0x50, 0x50, 0x45, 0x20, 0x4d,
	  0x61, 0x73, 0x74, 0x65, 0x72, 0x20, 0x4b, 0x65, 0x79 };
    /* "On the client side, this is the send key; "
       "on the server side, it is the receive key." */
    u_char Magic2[84] =
	{ 0x4f, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x69,
	  0x65, 0x6e, 0x74, 0x20, 0x73, 0x69, 0x64, 0x65, 0x2c, 0x20,
	  0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
	  0x65, 0x20, 0x73, 0x65, 0x6e, 0x64, 0x20, 0x6b, 0x65, 0x79,
	  0x3b, 0x20, 0x6f, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73,
	  0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x73, 0x69, 0x64, 0x65,
	  0x2c, 0x20, 0x69, 0x74, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
	  0x65, 0x20, 0x72, 0x65, 0x63, 0x65, 0x69, 0x76, 0x65, 0x20,
	  0x6b, 0x65, 0x79, 0x2e };
    /* "On the client side, this is the receive key; "
       "on the server side, it is the send key." */
    u_char Magic3[84] =
	{ 0x4f, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x69,
	  0x65, 0x6e, 0x74, 0x20, 0x73, 0x69, 0x64, 0x65, 0x2c, 0x20,
	  0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
	  0x65, 0x20, 0x72, 0x65, 0x63, 0x65, 0x69, 0x76, 0x65, 0x20,
	  0x6b, 0x65, 0x79, 0x3b, 0x20, 0x6f, 0x6e, 0x20, 0x74, 0x68,
	  0x65, 0x20, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x73,
	  0x69, 0x64, 0x65, 0x2c, 0x20, 0x69, 0x74, 0x20, 0x69, 0x73,
	  0x20, 0x74, 0x68, 0x65, 0x20, 0x73, 0x65, 0x6e, 0x64, 0x20,
	  0x6b, 0x65, 0x79, 0x2e };
    u_char *s;

    ctx = PPP_MD_CTX_new();
    if (ctx != NULL) {

        if (PPP_DigestInit(ctx, PPP_sha1())) {

            if (PPP_DigestUpdate(ctx, PasswordHashHash, MD4_DIGEST_LENGTH)) {

                if (PPP_DigestUpdate(ctx, NTResponse, 24)) {

                    if (PPP_DigestUpdate(ctx, Magic1, sizeof(Magic1))) {
                        
                        KeyLen = SHA_DIGEST_LENGTH;
                        PPP_DigestFinal(ctx, MasterKey, &KeyLen);
                    }
                }
            }
        }
        
        PPP_MD_CTX_free(ctx);
    }

    /*
     * generate send key
     */
    if (IsServer)
	s = Magic3;
    else
	s = Magic2;

    ctx = PPP_MD_CTX_new();
    if (ctx != NULL) {

        if (PPP_DigestInit(ctx, PPP_sha1())) {

            if (PPP_DigestUpdate(ctx, MasterKey, 16)) {

                if (PPP_DigestUpdate(ctx, SHApad1, sizeof(SHApad1))) {

                    if (PPP_DigestUpdate(ctx, s, 84)) {

                        if (PPP_DigestUpdate(ctx, SHApad2, sizeof(SHApad2))) {
                        
                            KeyLen = SHA_DIGEST_LENGTH;
                            PPP_DigestFinal(ctx, SendKey, &KeyLen);
                        }
                    }
                }
            }
        }
        
        PPP_MD_CTX_free(ctx);
    }


    /*
     * generate recv key
     */
    if (IsServer)
	s = Magic2;
    else
	s = Magic3;

    ctx = PPP_MD_CTX_new();
    if (ctx != NULL) {

        if (PPP_DigestInit(ctx, PPP_sha1())) {

            if (PPP_DigestUpdate(ctx, MasterKey, 16)) {

                if (PPP_DigestUpdate(ctx, SHApad1, sizeof(SHApad1))) {

                    if (PPP_DigestUpdate(ctx, s, 84)) {

                        if (PPP_DigestUpdate(ctx, SHApad2, sizeof(SHApad2))) {
                        
                            KeyLen = SHA_DIGEST_LENGTH;
                            PPP_DigestFinal(ctx, RecvKey, &KeyLen);
                        }
                    }
                }
            }
        }
        
        PPP_MD_CTX_free(ctx);
    }

    mppe_set_keys(SendKey, RecvKey, SHA_DIGEST_LENGTH);
}

#ifndef UNIT_TEST

/*
 * Set MPPE options from plugins.
 */
void
mppe_set_enc_types(int policy, int types)
{
    /* Early exit for unknown policies. */
    if (policy != MPPE_ENC_POL_ENC_ALLOWED &&
	policy != MPPE_ENC_POL_ENC_REQUIRED)
	return;

    /* Don't modify MPPE if it's optional and wasn't already configured. */
    if (policy == MPPE_ENC_POL_ENC_ALLOWED && !ccp_wantoptions[0].mppe)
	return;

    /*
     * Disable undesirable encryption types.  Note that we don't ENABLE
     * any encryption types, to avoid overriding manual configuration.
     */
    switch(types) {
	case MPPE_ENC_TYPES_RC4_40:
	    ccp_wantoptions[0].mppe &= ~MPPE_OPT_128;	/* disable 128-bit */
	    break;
	case MPPE_ENC_TYPES_RC4_128:
	    ccp_wantoptions[0].mppe &= ~MPPE_OPT_40;	/* disable 40-bit */
	    break;
	default:
	    break;
    }
}

#endif
