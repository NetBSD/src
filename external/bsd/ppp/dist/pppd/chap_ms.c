/*	$NetBSD: chap_ms.c,v 1.7 2025/01/08 19:59:39 christos Exp $	*/

/*
 * chap_ms.c - Microsoft MS-CHAP compatible implementation.
 *
 * Copyright (c) 1995 Eric Rosenquist.  All rights reserved.
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

/*
 * Modifications by Lauri Pesonen / lpesonen@clinet.fi, april 1997
 *
 *   Implemented LANManager type password response to MS-CHAP challenges.
 *   Now pppd provides both NT style and LANMan style blocks, and the
 *   prefered is set by option "ms-lanman". Default is to use NT.
 *   The hash text (StdText) was taken from Win95 RASAPI32.DLL.
 *
 *   You should also use DOMAIN\\USERNAME as described in README.MSCHAP80
 */

/*
 * Modifications by Frank Cusack, frank@google.com, March 2002.
 *
 *   Implemented MS-CHAPv2 functionality, heavily based on sample
 *   implementation in RFC 2759.  Implemented MPPE functionality,
 *   heavily based on sample implementation in RFC 3079.
 *
 * Copyright (c) 2002 Google, Inc.  All rights reserved.
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
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: chap_ms.c,v 1.7 2025/01/08 19:59:39 christos Exp $");

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#if defined(__linux__)
#include <linux/ppp-comp.h>
#else
#include <net/ppp-comp.h>
#endif

#include "pppd-private.h"
#include "options.h"
#include "chap.h"
#include "chap_ms.h"
#include "magic.h"
#include "mppe.h"
#include "crypto.h"
#include "crypto_ms.h"

#ifdef UNIT_TEST
#undef PPP_WITH_MPPE
#endif

static void	ascii2unicode (char[], int, u_char[]);
static void	NTPasswordHash (u_char *, int, unsigned char *);
static int	ChallengeResponse (u_char *, u_char *, u_char*);
static void	ChapMS_NT (u_char *, char *, int, u_char[24]);
static void	ChapMS2_NT (u_char *, u_char[16], char *, char *, int,
				u_char[24]);
static void	GenerateAuthenticatorResponsePlain
			(char*, int, u_char[24], u_char[16], u_char *,
			 char *, u_char[41]);
#ifdef PPP_WITH_MSLANMAN
static void	ChapMS_LANMan (u_char *, char *, int, u_char *);
#endif

#ifdef PPP_WITH_MSLANMAN
bool	ms_lanman = 0;    	/* Use LanMan password instead of NT */
			  	/* Has meaning only with MS-CHAP challenges */
#endif

#ifdef PPP_WITH_MPPE
#ifdef DEBUGMPPEKEY
/* For MPPE debug */
/* Use "[]|}{?/><,`!2&&(" (sans quotes) for RFC 3079 MS-CHAPv2 test value */
static char *mschap_challenge = NULL;
/* Use "!@\#$%^&*()_+:3|~" (sans quotes, backslash is to escape #) for ... */
static char *mschap2_peer_challenge = NULL;
#endif

#include "fsm.h"		/* Need to poke MPPE options */
#include "ccp.h"
#endif

/*
 * Command-line options.
 */
static struct option chapms_option_list[] = {
#ifdef PPP_WITH_MSLANMAN
	{ "ms-lanman", o_bool, &ms_lanman,
	  "Use LanMan passwd when using MS-CHAP", 1 },
#endif
#ifdef DEBUGMPPEKEY
	{ "mschap-challenge", o_string, &mschap_challenge,
	  "specify CHAP challenge" },
	{ "mschap2-peer-challenge", o_string, &mschap2_peer_challenge,
	  "specify CHAP peer challenge" },
#endif
	{ NULL }
};

/*
 * chapms_generate_challenge - generate a challenge for MS-CHAP.
 * For MS-CHAP the challenge length is fixed at 8 bytes.
 * The length goes in challenge[0] and the actual challenge starts
 * at challenge[1].
 */
static void
chapms_generate_challenge(unsigned char *challenge)
{
	*challenge++ = 8;
#ifdef DEBUGMPPEKEY
	if (mschap_challenge && strlen(mschap_challenge) == 8)
		memcpy(challenge, mschap_challenge, 8);
	else
#endif
		random_bytes(challenge, 8);
}

static void
chapms2_generate_challenge(unsigned char *challenge)
{
	*challenge++ = 16;
#ifdef DEBUGMPPEKEY
	if (mschap_challenge && strlen(mschap_challenge) == 16)
		memcpy(challenge, mschap_challenge, 16);
	else
#endif
		random_bytes(challenge, 16);
}

static int
chapms_verify_response(int id, char *name,
		       unsigned char *secret, int secret_len,
		       unsigned char *challenge, unsigned char *response,
		       char *message, int message_space)
{
	unsigned char md[MS_CHAP_RESPONSE_LEN];
	int diff;
	int challenge_len, response_len;

	challenge_len = *challenge++;	/* skip length, is 8 */
	response_len = *response++;
	if (response_len != MS_CHAP_RESPONSE_LEN)
		goto bad;

#ifndef PPP_WITH_MSLANMAN
	if (!response[MS_CHAP_USENT]) {
		/* Should really propagate this into the error packet. */
		notice("Peer request for LANMAN auth not supported");
		goto bad;
	}
#endif

	/* Generate the expected response. */
	ChapMS(challenge, (char *)secret, secret_len, md);

#ifdef PPP_WITH_MSLANMAN
	/* Determine which part of response to verify against */
	if (!response[MS_CHAP_USENT])
		diff = memcmp(&response[MS_CHAP_LANMANRESP],
			      &md[MS_CHAP_LANMANRESP], MS_CHAP_LANMANRESP_LEN);
	else
#endif
		diff = memcmp(&response[MS_CHAP_NTRESP], &md[MS_CHAP_NTRESP],
			      MS_CHAP_NTRESP_LEN);

	if (diff == 0) {
		slprintf(message, message_space, "Access granted");
		return 1;
	}

 bad:
	/* See comments below for MS-CHAP V2 */
	slprintf(message, message_space, "E=691 R=1 C=%0.*B V=0",
		 challenge_len, challenge);
	return 0;
}

static int
chapms2_verify_response(int id, char *name,
			unsigned char *secret, int secret_len,
			unsigned char *challenge, unsigned char *response,
			char *message, int message_space)
{
	unsigned char md[MS_CHAP2_RESPONSE_LEN];
	char saresponse[MS_AUTH_RESPONSE_LENGTH+1];
	int challenge_len, response_len;

	challenge_len = *challenge++;	/* skip length, is 16 */
	response_len = *response++;
	if (response_len != MS_CHAP2_RESPONSE_LEN)
		goto bad;	/* not even the right length */

	/* Generate the expected response and our mutual auth. */
	ChapMS2(challenge, &response[MS_CHAP2_PEER_CHALLENGE], name,
		(char *)secret, secret_len, md,
		(unsigned char *)saresponse, MS_CHAP2_AUTHENTICATOR);

	/* compare MDs and send the appropriate status */
	/*
	 * Per RFC 2759, success message must be formatted as
	 *     "S=<auth_string> M=<message>"
	 * where
	 *     <auth_string> is the Authenticator Response (mutual auth)
	 *     <message> is a text message
	 *
	 * However, some versions of Windows (win98 tested) do not know
	 * about the M=<message> part (required per RFC 2759) and flag
	 * it as an error (reported incorrectly as an encryption error
	 * to the user).  Since the RFC requires it, and it can be
	 * useful information, we supply it if the peer is a conforming
	 * system.  Luckily (?), win98 sets the Flags field to 0x04
	 * (contrary to RFC requirements) so we can use that to
	 * distinguish between conforming and non-conforming systems.
	 *
	 * Special thanks to Alex Swiridov <say@real.kharkov.ua> for
	 * help debugging this.
	 */
	if (memcmp(&md[MS_CHAP2_NTRESP], &response[MS_CHAP2_NTRESP],
		   MS_CHAP2_NTRESP_LEN) == 0) {
		if (response[MS_CHAP2_FLAGS])
			slprintf(message, message_space, "S=%s", saresponse);
		else
			slprintf(message, message_space, "S=%s M=%s",
				 saresponse, "Access granted");
		return 1;
	}

 bad:
	/*
	 * Failure message must be formatted as
	 *     "E=e R=r C=c V=v M=m"
	 * where
	 *     e = error code (we use 691, ERROR_AUTHENTICATION_FAILURE)
	 *     r = retry (we use 1, ok to retry)
	 *     c = challenge to use for next response, we reuse previous
	 *     v = Change Password version supported, we use 0
	 *     m = text message
	 *
	 * The M=m part is only for MS-CHAPv2.  Neither win2k nor
	 * win98 (others untested) display the message to the user anyway.
	 * They also both ignore the E=e code.
	 *
	 * Note that it's safe to reuse the same challenge as we don't
	 * actually accept another response based on the error message
	 * (and no clients try to resend a response anyway).
	 *
	 * Basically, this whole bit is useless code, even the small
	 * implementation here is only because of overspecification.
	 */
	slprintf(message, message_space, "E=691 R=1 C=%0.*B V=0 M=%s",
		 challenge_len, challenge, "Access denied");
	return 0;
}

static void
chapms_make_response(unsigned char *response, int id, char *our_name,
		     unsigned char *challenge, char *secret, int secret_len,
		     unsigned char *private)
{
	challenge++;	/* skip length, should be 8 */
	*response++ = MS_CHAP_RESPONSE_LEN;
	ChapMS(challenge, secret, secret_len, response);
}

struct chapms2_response_cache_entry {
	int id;
	unsigned char challenge[16];
	unsigned char response[MS_CHAP2_RESPONSE_LEN];
	unsigned char auth_response[MS_AUTH_RESPONSE_LENGTH];
};

#define CHAPMS2_MAX_RESPONSE_CACHE_SIZE 10
static struct chapms2_response_cache_entry
    chapms2_response_cache[CHAPMS2_MAX_RESPONSE_CACHE_SIZE];
static int chapms2_response_cache_next_index = 0;
static int chapms2_response_cache_size = 0;

static void
chapms2_add_to_response_cache(int id, unsigned char *challenge,
			      unsigned char *response,
			      unsigned char *auth_response)
{
	int i = chapms2_response_cache_next_index;

	chapms2_response_cache[i].id = id;
	memcpy(chapms2_response_cache[i].challenge, challenge, 16);
	memcpy(chapms2_response_cache[i].response, response,
	       MS_CHAP2_RESPONSE_LEN);
	memcpy(chapms2_response_cache[i].auth_response,
	       auth_response, MS_AUTH_RESPONSE_LENGTH);
	chapms2_response_cache_next_index =
		(i + 1) % CHAPMS2_MAX_RESPONSE_CACHE_SIZE;
	if (chapms2_response_cache_next_index > chapms2_response_cache_size)
		chapms2_response_cache_size = chapms2_response_cache_next_index;
	dbglog("added response cache entry %d", i);
}

static struct chapms2_response_cache_entry*
chapms2_find_in_response_cache(int id, unsigned char *challenge,
		      unsigned char *auth_response)
{
	int i;

	for (i = 0; i < chapms2_response_cache_size; i++) {
		if (id == chapms2_response_cache[i].id
		    && (!challenge
			|| memcmp(challenge,
				  chapms2_response_cache[i].challenge,
				  16) == 0)
		    && (!auth_response
			|| memcmp(auth_response,
				  chapms2_response_cache[i].auth_response,
				  MS_AUTH_RESPONSE_LENGTH) == 0)) {
			dbglog("response found in cache (entry %d)", i);
			return &chapms2_response_cache[i];
		}
	}
	return NULL;  /* not found */
}

static void
chapms2_make_response(unsigned char *response, int id, char *our_name,
		      unsigned char *challenge, char *secret, int secret_len,
		      unsigned char *private)
{
	const struct chapms2_response_cache_entry *cache_entry;
	unsigned char auth_response[MS_AUTH_RESPONSE_LENGTH+1];

	challenge++;	/* skip length, should be 16 */
	*response++ = MS_CHAP2_RESPONSE_LEN;
	cache_entry = chapms2_find_in_response_cache(id, challenge, NULL);
	if (cache_entry) {
		memcpy(response, cache_entry->response, MS_CHAP2_RESPONSE_LEN);
		return;
	}
	ChapMS2(challenge,
#ifdef DEBUGMPPEKEY
		mschap2_peer_challenge,
#else
		NULL,
#endif
		our_name, secret, secret_len, response, auth_response,
		MS_CHAP2_AUTHENTICATEE);
	chapms2_add_to_response_cache(id, challenge, response, auth_response);
}

static int
chapms2_check_success(int id, unsigned char *msg, int len)
{
	if ((len < MS_AUTH_RESPONSE_LENGTH + 2) ||
	    strncmp((char *)msg, "S=", 2) != 0) {
		/* Packet does not start with "S=" */
		error("MS-CHAPv2 Success packet is badly formed.");
		return 0;
	}
	msg += 2;
	len -= 2;
	if (len < MS_AUTH_RESPONSE_LENGTH
	    || !chapms2_find_in_response_cache(id, NULL /* challenge */, msg)) {
		/* Authenticator Response did not match expected. */
		error("MS-CHAPv2 mutual authentication failed.");
		return 0;
	}
	/* Authenticator Response matches. */
	msg += MS_AUTH_RESPONSE_LENGTH; /* Eat it */
	len -= MS_AUTH_RESPONSE_LENGTH;
	if ((len >= 3) && !strncmp((char *)msg, " M=", 3)) {
		msg += 3; /* Eat the delimiter */
	} else 	if ((len >= 2) && !strncmp((char *)msg, "M=", 2)) {
		msg += 2; /* Eat the delimiter */
	} else if (len) {
		/* Packet has extra text which does not begin " M=" */
		error("MS-CHAPv2 Success packet is badly formed.");
		return 0;
	}
	return 1;
}

static void
chapms_handle_failure(unsigned char *inp, int len)
{
	int err;
	char *p, *msg;

	/* We want a null-terminated string for strxxx(). */
	msg = malloc(len + 1);
	if (!msg) {
		notice("Out of memory in chapms_handle_failure");
		return;
	}
	BCOPY(inp, msg, len);
	msg[len] = 0;
	p = msg;

	/*
	 * Deal with MS-CHAP formatted failure messages; just print the
	 * M=<message> part (if any).  For MS-CHAP we're not really supposed
	 * to use M=<message>, but it shouldn't hurt.  See
	 * chapms[2]_verify_response.
	 */
	if (!strncmp(p, "E=", 2))
		err = strtol(p+2, NULL, 10); /* Remember the error code. */
	else
		goto print_msg; /* Message is badly formatted. */

	if (len && ((p = strstr(p, " M=")) != NULL)) {
		/* M=<message> field found. */
		p += 3;
	} else {
		/* No M=<message>; use the error code. */
		switch (err) {
		case MS_CHAP_ERROR_RESTRICTED_LOGON_HOURS:
			p = "E=646 Restricted logon hours";
			break;

		case MS_CHAP_ERROR_ACCT_DISABLED:
			p = "E=647 Account disabled";
			break;

		case MS_CHAP_ERROR_PASSWD_EXPIRED:
			p = "E=648 Password expired";
			break;

		case MS_CHAP_ERROR_NO_DIALIN_PERMISSION:
			p = "E=649 No dialin permission";
			break;

		case MS_CHAP_ERROR_AUTHENTICATION_FAILURE:
			p = "E=691 Authentication failure";
			break;

		case MS_CHAP_ERROR_CHANGING_PASSWORD:
			/* Should never see this, we don't support Change Password. */
			p = "E=709 Error changing password";
			break;

		default:
			free(msg);
			error("Unknown MS-CHAP authentication failure: %.*v",
			      len, inp);
			return;
		}
	}
print_msg:
	if (p != NULL)
		error("MS-CHAP authentication failed: %v", p);
	free(msg);
}

static int
ChallengeResponse(u_char *challenge,
		  u_char *PasswordHash,
		  u_char *response)
{
    u_char ZPasswordHash[24];

    BZERO(ZPasswordHash, sizeof(ZPasswordHash));
    BCOPY(PasswordHash, ZPasswordHash, MD4_DIGEST_LENGTH);

#if 0
    dbglog("ChallengeResponse - ZPasswordHash %.*B",
	   sizeof(ZPasswordHash), ZPasswordHash);
#endif

    if (DesEncrypt(challenge, ZPasswordHash + 0,  response + 0) &&
        DesEncrypt(challenge, ZPasswordHash + 7,  response + 8) &&
        DesEncrypt(challenge, ZPasswordHash + 14, response + 16))
        return 1;

#if 0
    dbglog("ChallengeResponse - response %.24B", response);
#endif
    return 0;
}

void
ChallengeHash(u_char PeerChallenge[16], u_char *rchallenge,
	      char *username, u_char Challenge[8])
    
{
    PPP_MD_CTX* ctx;
    u_char	hash[SHA_DIGEST_LENGTH];
    int     hash_len;
    const char *user;

    /* remove domain from "domain\username" */
    if ((user = strrchr(username, '\\')) != NULL)
	++user;
    else
	user = username;
    
    ctx = PPP_MD_CTX_new();
    if (ctx != NULL) {

        if (PPP_DigestInit(ctx, PPP_sha1())) {

            if (PPP_DigestUpdate(ctx, PeerChallenge, 16)) {

                if (PPP_DigestUpdate(ctx, rchallenge, 16)) {

                    if (PPP_DigestUpdate(ctx, user, strlen(user))) {
                        
                        hash_len = SHA_DIGEST_LENGTH;
                        if (PPP_DigestFinal(ctx, hash, &hash_len)) {

                            BCOPY(hash, Challenge, 8);
                        }
                    }
                }
            }
        }

        PPP_MD_CTX_free(ctx);
    }
}

/*
 * Convert the ASCII version of the password to Unicode.
 * This implicitly supports 8-bit ISO8859/1 characters.
 * This gives us the little-endian representation, which
 * is assumed by all M$ CHAP RFCs.  (Unicode byte ordering
 * is machine-dependent.)
 */
static void
ascii2unicode(char ascii[], int ascii_len, u_char unicode[])
{
    int i;

    BZERO(unicode, ascii_len * 2);
    for (i = 0; i < ascii_len; i++)
	unicode[i * 2] = (u_char) ascii[i];
}

static void
NTPasswordHash(u_char *secret, int secret_len, unsigned char* hash)
{
    PPP_MD_CTX* ctx = PPP_MD_CTX_new();
    if (ctx != NULL) {

        if (PPP_DigestInit(ctx, PPP_md4())) {

            if (PPP_DigestUpdate(ctx, secret, secret_len)) {

                int hash_len = MD4_DIGEST_LENGTH;
                PPP_DigestFinal(ctx, hash, &hash_len);
            }
        }
        
        PPP_MD_CTX_free(ctx);
    }
}

static void
ChapMS_NT(u_char *rchallenge, char *secret, int secret_len,
	  u_char NTResponse[24])
{
    u_char	unicodePassword[MAX_NT_PASSWORD * 2];
    u_char	PasswordHash[MD4_DIGEST_LENGTH];

    /* Hash the Unicode version of the secret (== password). */
    ascii2unicode(secret, secret_len, unicodePassword);
    NTPasswordHash(unicodePassword, secret_len * 2, PasswordHash);

    ChallengeResponse(rchallenge, PasswordHash, NTResponse);
}

static void
ChapMS2_NT(u_char *rchallenge, u_char PeerChallenge[16], char *username,
	   char *secret, int secret_len, u_char NTResponse[24])
{
    u_char	unicodePassword[MAX_NT_PASSWORD * 2];
    u_char	PasswordHash[MD4_DIGEST_LENGTH];
    u_char	Challenge[8];

    ChallengeHash(PeerChallenge, rchallenge, username, Challenge);

    /* Hash the Unicode version of the secret (== password). */
    ascii2unicode(secret, secret_len, unicodePassword);
    NTPasswordHash(unicodePassword, secret_len * 2, PasswordHash);

    ChallengeResponse(Challenge, PasswordHash, NTResponse);
}

#ifdef PPP_WITH_MSLANMAN
static u_char *StdText = (u_char *)"KGS!@#$%"; /* key from rasapi32.dll */

static void
ChapMS_LANMan(u_char *rchallenge, char *secret, int secret_len,
	      unsigned char *response)
{
    int			i;
    u_char		UcasePassword[MAX_NT_PASSWORD]; /* max is actually 14 */
    u_char		PasswordHash[MD4_DIGEST_LENGTH];

    /* LANMan password is case insensitive */
    BZERO(UcasePassword, sizeof(UcasePassword));
    for (i = 0; i < secret_len; i++)
       UcasePassword[i] = (u_char)toupper((u_char)secret[i]);

    if (DesEncrypt(StdText, UcasePassword + 0, PasswordHash + 0) &&
        DesEncrypt(StdText, UcasePassword + 7, PasswordHash + 8)) {

        ChallengeResponse(rchallenge, PasswordHash, &response[MS_CHAP_LANMANRESP]);
    }
}
#endif


void
GenerateAuthenticatorResponse(unsigned char* PasswordHashHash,
			      unsigned char *NTResponse, unsigned char *PeerChallenge,
			      unsigned char *rchallenge, char *username,
			      unsigned char *authResponse)
{
    /*
     * "Magic" constants used in response generation, from RFC 2759.
     */
    u_char Magic1[39] = /* "Magic server to client signing constant" */
	{ 0x4D, 0x61, 0x67, 0x69, 0x63, 0x20, 0x73, 0x65, 0x72, 0x76,
	  0x65, 0x72, 0x20, 0x74, 0x6F, 0x20, 0x63, 0x6C, 0x69, 0x65,
	  0x6E, 0x74, 0x20, 0x73, 0x69, 0x67, 0x6E, 0x69, 0x6E, 0x67,
	  0x20, 0x63, 0x6F, 0x6E, 0x73, 0x74, 0x61, 0x6E, 0x74 };
    u_char Magic2[41] = /* "Pad to make it do more than one iteration" */
	{ 0x50, 0x61, 0x64, 0x20, 0x74, 0x6F, 0x20, 0x6D, 0x61, 0x6B,
	  0x65, 0x20, 0x69, 0x74, 0x20, 0x64, 0x6F, 0x20, 0x6D, 0x6F,
	  0x72, 0x65, 0x20, 0x74, 0x68, 0x61, 0x6E, 0x20, 0x6F, 0x6E,
	  0x65, 0x20, 0x69, 0x74, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6F,
	  0x6E };

    int		i;
    PPP_MD_CTX *ctx;
    u_char	Digest[SHA_DIGEST_LENGTH] = {};
    int     hash_len;
    u_char	Challenge[8];

    ctx = PPP_MD_CTX_new();
    if (ctx != NULL) {

        if (PPP_DigestInit(ctx, PPP_sha1())) {

            if (PPP_DigestUpdate(ctx, PasswordHashHash, MD4_DIGEST_LENGTH)) {

                if (PPP_DigestUpdate(ctx, NTResponse, 24)) {

                    if (PPP_DigestUpdate(ctx, Magic1, sizeof(Magic1))) {
                        
                        hash_len = sizeof(Digest);
                        PPP_DigestFinal(ctx, Digest, &hash_len);
                    }
                }
            }
        }
        PPP_MD_CTX_free(ctx);
    }
    
    ChallengeHash(PeerChallenge, rchallenge, username, Challenge);

    ctx = PPP_MD_CTX_new();
    if (ctx != NULL) {

        if (PPP_DigestInit(ctx, PPP_sha1())) {

            if (PPP_DigestUpdate(ctx, Digest, sizeof(Digest))) {

                if (PPP_DigestUpdate(ctx, Challenge, sizeof(Challenge))) {

                    if (PPP_DigestUpdate(ctx, Magic2, sizeof(Magic2))) {
                        
                        hash_len = sizeof(Digest);
                        PPP_DigestFinal(ctx, Digest, &hash_len);
                    }
                }
            }
        }

        PPP_MD_CTX_free(ctx);
    }

    /* Convert to ASCII hex string. */
    for (i = 0; i < MAX((MS_AUTH_RESPONSE_LENGTH / 2), sizeof(Digest)); i++) {
        sprintf((char *)&authResponse[i * 2], "%02X", Digest[i]);
    }
}


static void
GenerateAuthenticatorResponsePlain
		(char *secret, int secret_len,
		 u_char NTResponse[24], u_char PeerChallenge[16],
		 u_char *rchallenge, char *username,
		 u_char authResponse[MS_AUTH_RESPONSE_LENGTH+1])
{
    u_char	unicodePassword[MAX_NT_PASSWORD * 2];
    u_char	PasswordHash[MD4_DIGEST_LENGTH];
    u_char	PasswordHashHash[MD4_DIGEST_LENGTH];

    /* Hash (x2) the Unicode version of the secret (== password). */
    ascii2unicode(secret, secret_len, unicodePassword);
    NTPasswordHash(unicodePassword, secret_len * 2, PasswordHash);
    NTPasswordHash(PasswordHash, sizeof(PasswordHash),
		   PasswordHashHash);

    GenerateAuthenticatorResponse(PasswordHashHash, NTResponse, PeerChallenge,
				  rchallenge, username, authResponse);
}


#ifdef PPP_WITH_MPPE

/*
 * Set mppe_xxxx_key from MS-CHAP credentials. (see RFC 3079)
 */
static void
Set_Start_Key(u_char *rchallenge, char *secret, int secret_len)
{
    u_char	unicodePassword[MAX_NT_PASSWORD * 2];
    u_char	PasswordHash[MD4_DIGEST_LENGTH];
    u_char	PasswordHashHash[MD4_DIGEST_LENGTH];

    /* Hash (x2) the Unicode version of the secret (== password). */
    ascii2unicode(secret, secret_len, unicodePassword);
    NTPasswordHash(unicodePassword, secret_len * 2, PasswordHash);
    NTPasswordHash(PasswordHash, sizeof(PasswordHash), PasswordHashHash);

    mppe_set_chapv1(rchallenge, PasswordHashHash);
}

/*
 * Set mppe_xxxx_key from MS-CHAPv2 credentials. (see RFC 3079)
 */
static void
SetMasterKeys(char *secret, int secret_len, u_char NTResponse[24], int IsServer)
{
    u_char	unicodePassword[MAX_NT_PASSWORD * 2];
    u_char	PasswordHash[MD4_DIGEST_LENGTH];
    u_char	PasswordHashHash[MD4_DIGEST_LENGTH];
    /* Hash (x2) the Unicode version of the secret (== password). */
    ascii2unicode(secret, secret_len, unicodePassword);
    NTPasswordHash(unicodePassword, secret_len * 2, PasswordHash);
    NTPasswordHash(PasswordHash, sizeof(PasswordHash), PasswordHashHash);
    mppe_set_chapv2(PasswordHashHash, NTResponse, IsServer);
}

#endif /* PPP_WITH_MPPE */


void
ChapMS(u_char *rchallenge, char *secret, int secret_len,
       unsigned char *response)
{
    BZERO(response, MS_CHAP_RESPONSE_LEN);

    ChapMS_NT(rchallenge, secret, secret_len, &response[MS_CHAP_NTRESP]);

#ifdef PPP_WITH_MSLANMAN
    ChapMS_LANMan(rchallenge, secret, secret_len,
		  &response[MS_CHAP_LANMANRESP]);

    /* preferred method is set by option  */
    response[MS_CHAP_USENT] = !ms_lanman;
#else
    response[MS_CHAP_USENT] = 1;
#endif

#ifdef PPP_WITH_MPPE
    Set_Start_Key(rchallenge, secret, secret_len);
#endif
}


/*
 * If PeerChallenge is NULL, one is generated and the PeerChallenge
 * field of response is filled in.  Call this way when generating a response.
 * If PeerChallenge is supplied, it is copied into the PeerChallenge field.
 * Call this way when verifying a response (or debugging).
 * Do not call with PeerChallenge = response.
 *
 * The PeerChallenge field of response is then used for calculation of the
 * Authenticator Response.
 */
void
ChapMS2(unsigned char *rchallenge, unsigned char *PeerChallenge,
	char *user, char *secret, int secret_len, unsigned char *response,
	u_char authResponse[MS_AUTH_RESPONSE_LENGTH+1], int authenticator)
{
    /* ARGSUSED */
    u_char *p = &response[MS_CHAP2_PEER_CHALLENGE];
    int i;

    BZERO(response, MS_CHAP2_RESPONSE_LEN);

    /* Generate the Peer-Challenge if requested, or copy it if supplied. */
    if (!PeerChallenge)
	for (i = 0; i < MS_CHAP2_PEER_CHAL_LEN; i++)
	    *p++ = (u_char) (drand48() * 0xff);
    else
	BCOPY(PeerChallenge, &response[MS_CHAP2_PEER_CHALLENGE],
	      MS_CHAP2_PEER_CHAL_LEN);

    /* Generate the NT-Response */
    ChapMS2_NT(rchallenge, &response[MS_CHAP2_PEER_CHALLENGE], user,
	       secret, secret_len, &response[MS_CHAP2_NTRESP]);

    /* Generate the Authenticator Response. */
    GenerateAuthenticatorResponsePlain(secret, secret_len,
				       &response[MS_CHAP2_NTRESP],
				       &response[MS_CHAP2_PEER_CHALLENGE],
				       rchallenge, user, authResponse);

#ifdef PPP_WITH_MPPE
    SetMasterKeys(secret, secret_len,
		  &response[MS_CHAP2_NTRESP], authenticator);
#endif
}


static struct chap_digest_type chapms_digest = {
	CHAP_MICROSOFT,		/* code */
	chapms_generate_challenge,
	chapms_verify_response,
	chapms_make_response,
	NULL,			/* check_success */
	chapms_handle_failure,
};

static struct chap_digest_type chapms2_digest = {
	CHAP_MICROSOFT_V2,	/* code */
	chapms2_generate_challenge,
	chapms2_verify_response,
	chapms2_make_response,
	chapms2_check_success,
	chapms_handle_failure,
};

#ifndef UNIT_TEST
void
chapms_init(void)
{
	chap_register_digest(&chapms_digest);
	chap_register_digest(&chapms2_digest);
	ppp_add_options(chapms_option_list);
}
#else

#include <time.h>

int debug = 1;
int error_count = 0;
int unsuccess = 0;

void random_bytes(unsigned char *bytes, int len)
{
    int i = 0;
    srand(time(NULL));
    while (i < len) {
        bytes[i++] = (unsigned char) rand();
    }
}


int test_chap_v1(void) {
    char *secret = "MyPw";

    unsigned char challenge[8] = {
        0x10, 0x2D, 0xB5, 0xDF, 0x08, 0x5D, 0x30, 0x41
    };
    unsigned char response[MS_CHAP_RESPONSE_LEN] = {
    };
    unsigned char result[MS_CHAP_RESPONSE_LEN] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        0x4E, 0x9D, 0x3C, 0x8F, 0x9C, 0xFD, 0x38, 0x5D,
        0x5B, 0xF4, 0xD3, 0x24, 0x67, 0x91, 0x95, 0x6C,
        0xA4, 0xC3, 0x51, 0xAB, 0x40, 0x9A, 0x3D, 0x61,

        0x01
    };

    ChapMS(challenge, secret, strlen(secret), response);
    return memcmp(response, result, MS_CHAP_RESPONSE_LEN);
}

int test_chap_v2(void) {
    char *secret = "clientPass";
    char *name = "User";

    char saresponse[MS_AUTH_RESPONSE_LENGTH+1];
    char *saresult = "407A5589115FD0D6209F510FE9C04566932CDA56";

    unsigned char authenticator[16] = {
        0x5B, 0x5D, 0x7C, 0x7D, 0x7B, 0x3F, 0x2F, 0x3E,
        0x3C, 0x2C, 0x60, 0x21, 0x32, 0x26, 0x26, 0x28
    };
    unsigned char peerchallenge[16] = {
        0x21, 0x40, 0x23, 0x24, 0x25, 0x5E, 0x26, 0x2A,
        0x28, 0x29, 0x5F, 0x2B, 0x3A, 0x33, 0x7C, 0x7E
    };
    unsigned char result[MS_CHAP_NTRESP_LEN] = {
        0x82, 0x30, 0x9E, 0xCD, 0x8D, 0x70, 0x8B, 0x5E,
        0xA0, 0x8F, 0xAA, 0x39, 0x81, 0xCD, 0x83, 0x54,
        0x42, 0x33, 0x11, 0x4A, 0x3D, 0x85, 0xD6, 0xDF
    };

    unsigned char response[MS_CHAP2_RESPONSE_LEN] = {
    };

	ChapMS2(authenticator, peerchallenge, name,
		secret, strlen(secret), response,
		(unsigned char *)saresponse, MS_CHAP2_AUTHENTICATOR);

    return memcmp(&response[MS_CHAP2_NTRESP], result, MS_CHAP2_NTRESP_LEN) ||
        strncmp(saresponse, saresult, MS_AUTH_RESPONSE_LENGTH);
}

int main(int argc, char *argv[]) {
    
    PPP_crypto_init();

    if (test_chap_v1()) {
        printf("CHAPv1 failed\n");
        return -1;
    }

    if (test_chap_v2()) {
        printf("CHAPv2 failed\n");
        return -1;
    }

    PPP_crypto_deinit();

    printf("Success\n");
    return 0;
}

#endif  /* UNIT_TEST */

