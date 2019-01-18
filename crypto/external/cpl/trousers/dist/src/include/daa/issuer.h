
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006-2007
 *
 */
#ifndef __ISSUER_H__
#define __ISSUER_H__

#include "daa/daa_structs.h"
#include "daa/daa_parameter.h"
#include "tsplog.h"


TSS_RESULT
generate_key_pair(int num_attributes_issuer,
		int num_attributes_receiver,
		int base_nameLength,
		BYTE *base_name,
		KEY_PAIR_WITH_PROOF_internal **key_pair_with_proof
);

TSS_DAA_PK_PROOF_internal *generate_proof(
	const bi_ptr product_PQ_prime,
	const TSS_DAA_PK_internal *public_key,
	const bi_ptr xz,
	const bi_ptr x0,
	const bi_ptr x1,
	bi_array_ptr x);

#if 0
TSPICALL
Tspi_DAA_IssueInit_internal(
	TSS_HDAA	hDAA,	// in
	TSS_HKEY	issuerAuthPK,	// in
	TSS_HKEY	issuerKeyPair,	// in
	TSS_DAA_IDENTITY_PROOF	identityProof,	// in
	UINT32	capitalUprimeLength,	// in
	BYTE*	capitalUprime,	// in
	UINT32	daaCounter,	// in
	UINT32*	nonceIssuerLength,	// out
	BYTE**	nonceIssuer,	// out
	UINT32*	authenticationChallengeLength,	// out
	BYTE**	authenticationChallenge,	// out
	TSS_DAA_JOIN_ISSUER_SESSION*  joinSession	// out
);

TSPICALL
Tspi_DAA_IssueCredential_internal(
	TSS_HDAA	hDAA,	// in
	UINT32	attributesIssuerLength,	// in
	BYTE**	attributesIssuer,	// in
	TSS_DAA_CREDENTIAL_REQUEST	credentialRequest,	// in
	TSS_DAA_JOIN_ISSUER_SESSION	joinSession,	// in
	TSS_DAA_CRED_ISSUER*	credIssuer	// out
);
#endif
TSS_RESULT
compute_join_challenge_issuer( TSS_DAA_PK_internal *pk_intern,
				bi_ptr v_prime_prime,
				bi_ptr capitalA,
				bi_ptr capital_Atilde,
				UINT32 nonceReceiverLength,
				BYTE *nonceReceiver,
				UINT32 *c_primeLength,
				BYTE **c_prime);  // out allocation

#endif
