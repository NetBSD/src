/*	$NetBSD: chap_ms.h,v 1.6 2025/01/08 19:59:39 christos Exp $	*/

/*
 * chap_ms.h - Challenge Handshake Authentication Protocol definitions.
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

#ifndef PPP_CHAPMS_H
#define PPP_CHAPMS_H

#include "pppdconf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NT_PASSWORD		256	/* Max (Unicode) chars in an NT pass */

#define MS_CHAP_RESPONSE_LEN	49	/* Response length for MS-CHAP */
#define MS_CHAP2_RESPONSE_LEN	49	/* Response length for MS-CHAPv2 */
#define MS_AUTH_RESPONSE_LENGTH	40	/* MS-CHAPv2 authenticator response, */
#define MS_AUTH_NTRESP_LEN      24  /* Length of NT-response field */
					/* as ASCII */

/* E=eeeeeeeeee error codes for MS-CHAP failure messages. */
#define MS_CHAP_ERROR_RESTRICTED_LOGON_HOURS	646
#define MS_CHAP_ERROR_ACCT_DISABLED		647
#define MS_CHAP_ERROR_PASSWD_EXPIRED		648
#define MS_CHAP_ERROR_NO_DIALIN_PERMISSION	649
#define MS_CHAP_ERROR_AUTHENTICATION_FAILURE	691
#define MS_CHAP_ERROR_CHANGING_PASSWORD		709

/*
 * Offsets within the response field for MS-CHAP
 */
#define MS_CHAP_LANMANRESP	0
#define MS_CHAP_LANMANRESP_LEN	24
#define MS_CHAP_NTRESP		24
#define MS_CHAP_NTRESP_LEN	24
#define MS_CHAP_USENT		48

/*
 * Offsets within the response field for MS-CHAP2
 */
#define MS_CHAP2_PEER_CHALLENGE	0
#define MS_CHAP2_PEER_CHAL_LEN	16
#define MS_CHAP2_RESERVED_LEN	8
#define MS_CHAP2_NTRESP		24
#define MS_CHAP2_NTRESP_LEN	24
#define MS_CHAP2_FLAGS		48

/* Are we the authenticator or authenticatee?  For MS-CHAPv2 key derivation. */
#define MS_CHAP2_AUTHENTICATEE 0
#define MS_CHAP2_AUTHENTICATOR 1

void ChapMS (u_char *, char *, int, u_char *);
void ChapMS2 (u_char *, u_char *, char *, char *, int,
	      u_char *, u_char[MS_AUTH_RESPONSE_LENGTH+1], int);

void ChallengeHash (u_char[16], u_char *, char *, u_char[8]);


/**
 * PasswordHashHash - 16 bytes representing the NT Password Hash Hash
 * NTResponse - 24 bytes  represending the NTResponse parameter
 * PeerChallenge - 16 bytes challange for peer
 * rchallenge - 16 bytes challenge provided by peer
 * authResponse - 24 + 1 byte to store the authenticator response
 */
void GenerateAuthenticatorResponse(unsigned char *PasswordHashHash,
			unsigned char *NTResponse, unsigned char *PeerChallenge,
			unsigned char *rchallenge, char *username,
			unsigned char *authResponse);

void chapms_init(void);

#ifdef __cplusplus
}
#endif

#endif /* PPP_CHAPMS_H */
