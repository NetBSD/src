
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include "bi.h"
#include "daa_structs.h"

#if 0
// for RSA key
#include <openssl/rsa.h>

TSPICALL
Tspi_TPM_DAA_Sign_internal(TSS_HDAA hDAA,	// in
			TSS_HTPM hTPM,	// in
			TSS_HKEY hDaaCredential,	// in
			TSS_DAA_SELECTED_ATTRIB revealAttributes,	// in
			UINT32 verifierBaseNameLength,	// in
			BYTE* verifierBaseName,	// in
			UINT32 verifierNonceLength,	// in
			BYTE* verifierNonce,	// in
			TSS_DAA_SIGN_DATA signData,	// in
			TSS_DAA_SIGNATURE* daaSignature	// out
);

TSS_RESULT
Tspi_TPM_DAA_JoinInit_internal(TSS_HDAA hDAA,
				TSS_HTPM hTPM,
				int daa_counter,
				TSS_DAA_PK *issuer_pk,
				int issuer_authentication_PKLengh,
				RSA **issuer_authentication_PK,
				int issuer_authentication_PK_signaturesLength,
				BYTE **issuer_authentication_PK_signatures,
				int *capital_UprimeLength,
				BYTE **capital_Uprime,
				TSS_DAA_IDENTITY_PROOF *identity_proof,
				TSS_DAA_JOIN_SESSION *joinSession
);

TSPICALL Tspi_TPM_DAA_JoinCreateDaaPubKey_internal
(
	TSS_HDAA hDAA,	// in
	TSS_HTPM hTPM,	// in
	UINT32 authenticationChallengeLength,	// in
	BYTE* authenticationChallenge,	// in
	UINT32 nonceIssuerLength,	// in
	BYTE* nonceIssuer,	// in
	UINT32 attributesPlatformLength,	// in
	BYTE** attributesPlatform,	// in
	TSS_DAA_JOIN_SESSION* joinSession,	// in, out
	TSS_DAA_CREDENTIAL_REQUEST* credentialRequest	// out
);

// allocation:	endorsementKey as BYTE *
TSS_RESULT get_public_EK(
	TSS_HDAA hDAA,
	UINT32 *endorsementKeyLength,
	BYTE **endorsementKey
);

#endif

TSS_RESULT
compute_join_challenge_host(TSS_HDAA_CREDENTIAL,//TSS_HDAA hDAA,
				TSS_DAA_PK_internal *pk_internal,
				bi_ptr capitalU,
				bi_ptr capital_Uprime,
				bi_ptr capital_utilde,
				bi_ptr capital_utilde_prime,
				bi_ptr capital_ni,
				bi_ptr capital_ni_tilde,
				UINT32 commitments_proofLength,
				TSS_DAA_ATTRIB_COMMIT_internal *
				commitments_proof,
				UINT32 nonceIssuerLength,
				BYTE* nonceIssuer,
				UINT32 *resultLength,
				BYTE **result
);

#if 0

TSPICALL
Tspi_TPM_DAA_JoinStoreCredential_internal(TSS_HDAA hDAA,	// in
					TSS_HTPM hTPM,	// in
					TSS_DAA_CRED_ISSUER credIssuer,	// in
					TSS_DAA_JOIN_SESSION joinSession,	// in
					TSS_HKEY* hDaaCredential	// out
);

TSPICALL
Tspi_TPM_DAA_Sign_internal(TSS_HDAA hDAA,	// in
			TSS_HTPM hTPM,	// in
			TSS_HKEY hDaaCredential,	// in
			TSS_DAA_SELECTED_ATTRIB revealAttributes,	// in
			UINT32 verifierBaseNameLength,	// in
			BYTE* verifierBaseName,	// in
			UINT32 verifierNonceLength,	// in
			BYTE* verifierNonce,	// in
			TSS_DAA_SIGN_DATA signData,	// in
			TSS_DAA_SIGNATURE* daaSignature	// out
);

#endif

#endif
