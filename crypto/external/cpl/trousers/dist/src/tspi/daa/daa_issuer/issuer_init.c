
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// for message digest
#include <openssl/evp.h>

#include <stdlib.h>
#include "daa_structs.h"
#include "daa_parameter.h"
#include "trousers/tss.h"
#include "spi_internal_types.h"
#include "spi_utils.h"
#include <trousers/trousers.h>
#include <spi_utils.h>
#include <obj.h>
#include "tsplog.h"
#include "tss/tcs.h"

/*
Verifies if the key is a valid endorsement key of a TPM. (TPM is good)
return 0 if correct
 */
int verify_ek_and_daaCounter(
	UINT32 endorsementLength,
	BYTE *endorsementCredential,
	UINT32 daaCounter
) {
	// TODO
	return 0;
}


TSS_RESULT Tspi_DAA_IssueInit_internal(
	TSS_HDAA	hDAA,	// in
	TSS_HKEY	issuerAuthPK,	// in
	TSS_HKEY	issuerKeyPair,	// in (TSS_DAA_KEY_PAIR *)
	TSS_DAA_IDENTITY_PROOF	identityProof,	// in
	UINT32	capitalUprimeLength,	// in
	BYTE*	capitalUprime,	// in
	UINT32	daaCounter,	// in
	UINT32*	nonceIssuerLength,	// out
	BYTE**	nonceIssuer,	// out
	UINT32*	authenticationChallengeLength,	// out
	BYTE**	authenticationChallenge,	// out
	TSS_DAA_JOIN_ISSUER_SESSION*	joinSession	// out
) {
	TCS_CONTEXT_HANDLE tcsContext;
	TSS_RESULT result;
	BYTE *ne, *buffer;
	bi_t random;
	int length_ne;

	if( (result = obj_daa_get_tsp_context( hDAA, &tcsContext)) != TSS_SUCCESS)
		return result;
	// 1 & 2 : verify EK (and associated credentials) of the platform
	if( verify_ek_and_daaCounter( identityProof.endorsementLength,
				identityProof.endorsementCredential, daaCounter) != 0) {
		LogError("EK verification failed");
		return TSS_E_INTERNAL_ERROR;
	}

	// 3 : choose a random nonce for the platform (ni)
	bi_new( random);
	bi_urandom( random, DAA_PARAM_LENGTH_MESSAGE_DIGEST * 8);
	buffer = bi_2_nbin( nonceIssuerLength, random);
	if( buffer == NULL) {
		LogError("malloc of %d bytes failed", *nonceIssuerLength);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	*nonceIssuer =  convert_alloc( tcsContext, *nonceIssuerLength, buffer);
	if (*nonceIssuer == NULL) {
		LogError("malloc of %d bytes failed", *nonceIssuerLength);
		free( buffer);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	LogDebug("nonce Issuer[%d:%d]:%s", DAA_PARAM_LENGTH_MESSAGE_DIGEST,
		*nonceIssuerLength,
		dump_byte_array( *nonceIssuerLength , *nonceIssuer));

	// 4 : choose a random nonce ne and encrypt it under EK
	bi_urandom( random, DAA_PARAM_LENGTH_MESSAGE_DIGEST * 8);
	ne = convert_alloc( tcsContext, length_ne, bi_2_nbin( &length_ne, random));
	if (ne == NULL) {
		LogError("malloc of %d bytes failed", length_ne);
		free( buffer);
		free( nonceIssuer);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	bi_free( random);
	*authenticationChallenge = (BYTE *)calloc_tspi( tcsContext, 256); // 256: RSA size
	if (*authenticationChallenge == NULL) {
		LogError("malloc of %d bytes failed", 256);
		free( buffer);
		free( nonceIssuer);
		free( ne);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	result = Trspi_RSA_Encrypt(
		ne,	// message to encrypt
		length_ne,	// length message to encrypt
		*authenticationChallenge,	// destination
		authenticationChallengeLength, // length destination
		identityProof.endorsementCredential, // public key
		identityProof.endorsementLength); // public key size
	if( result != TSS_SUCCESS) {
		LogError("Can not encrypt the Authentication Challenge");
		free( buffer);
		free( nonceIssuer);
		free( ne);
		return TSS_E_INTERNAL_ERROR;
	}
	LogDebug("authenticationChallenge[%d:%d]:%s", DAA_PARAM_LENGTH_MESSAGE_DIGEST,
		*authenticationChallengeLength,
		dump_byte_array( *authenticationChallengeLength , *authenticationChallenge));

	// 5 : save PK, PKDAA, (p', q'), U', daaCounter, ni, ne in joinSession
	// EK is not a member of joinSession but is already saved in identityProof
	joinSession->issuerAuthPK = issuerAuthPK;
	joinSession->issuerKeyPair = issuerKeyPair;
	memcpy( &(joinSession->identityProof), &identityProof, sizeof(TSS_DAA_IDENTITY_PROOF));
	joinSession->capitalUprimeLength = capitalUprimeLength;
	joinSession->capitalUprime = capitalUprime;
	joinSession->daaCounter = daaCounter;
	joinSession->nonceIssuerLength = *nonceIssuerLength;
	joinSession->nonceIssuer = *nonceIssuer;
	joinSession->nonceEncryptedLength = length_ne;
	joinSession->nonceEncrypted = ne;
	return result;
}
