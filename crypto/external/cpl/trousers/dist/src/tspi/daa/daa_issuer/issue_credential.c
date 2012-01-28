
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
#include "platform.h"
// to include compute_zeta
#include "verifier.h"

// from UBigInteger (computePrime)
// remark: the type bi_t (bi_ptr) can not used a certaintity for probable_prime
// as used in UbigInteger. (certaintity: DAA_PARAM_SAFETY)
void compute_prime( bi_ptr e, int length, int interval) {
	do {
		bi_urandom( e, interval - 1);
		bi_setbit( e, length - 1);
	} while( bi_is_probable_prime( e) == 0);
}

/*
 code derived from verifyAuthenticity (IssuerTransaction.java)
*/
TSS_RESULT verify_authentificity(TSS_DAA_CREDENTIAL_REQUEST *credentialRequest,
						TSS_DAA_JOIN_ISSUER_SESSION *joinSession) {
	EVP_MD_CTX mdctx;
	BYTE *modulus_N0_bytes;
	BYTE *digest_n0;
	BYTE *contextHash;
	BYTE *capitalUPrime_bytes;
	BYTE *hash;
	UINT32 digest_n0Length, contextHashLength, hashLength, daaCount;
	bi_ptr capitalUPrime  =NULL;
	bi_ptr modulus_N0 = NULL;
	TSS_RESULT result = TSS_SUCCESS;
	char *buffer;

	modulus_N0 = bi_new_ptr();
	buffer = BN_bn2hex( ((RSA *)joinSession->issuerAuthPK)->n);
	if( buffer == NULL) {
		LogError("malloc of hexadecimal representation failed");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_set_as_hex( modulus_N0, buffer);
	// in TPM, N0 is hashed by hashing the scratch (256 bytes) so it must
	// be formatted according to the scratch size (TPM_DAA_SIZE_issuerModulus)
	modulus_N0_bytes = (BYTE *)malloc( TPM_DAA_SIZE_issuerModulus);
	if (modulus_N0_bytes == NULL) {
		LogError("malloc of %d bytes failed", TPM_DAA_SIZE_issuerModulus);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_byte_array( modulus_N0_bytes, TPM_DAA_SIZE_issuerModulus, modulus_N0);
	bi_free_ptr( modulus_N0);

	if( TPM_DAA_SIZE_issuerModulus * 8 != DAA_PARAM_KEY_SIZE) {
		LogError("TPM_DAA_SIZE_issuerModulus * 8 (%d) != DAA_PARAM_KEY_SIZE(%d)",
			 TPM_DAA_SIZE_issuerModulus*8, DAA_PARAM_KEY_SIZE);
		return TSS_E_INTERNAL_ERROR;
	}
	EVP_DigestInit(&mdctx, DAA_PARAM_get_message_digest());
	// digestN0 = hash( modulus_N0) see Appendix B of spec. and TPM join stage 7 and 8
	EVP_DigestUpdate(&mdctx,  modulus_N0_bytes, TPM_DAA_SIZE_issuerModulus);
	digest_n0Length = EVP_MD_CTX_size(&mdctx);
	digest_n0 = (BYTE *)malloc( digest_n0Length);
	if (digest_n0 == NULL) {
		LogError("malloc of %d bytes failed", digest_n0Length);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	EVP_DigestFinal(&mdctx, digest_n0, NULL);

	// test if credentialRequest->authenticationProof =
	//				H( H( U, daaCount, H(n0), joinSession->nonceEncrypted))
	EVP_DigestInit(&mdctx, DAA_PARAM_get_message_digest());
	// enlarge capitalU to 256 (TPM_DAA_SIZE_issuerModulus)
	 // allocation
	capitalUPrime = bi_set_as_nbin( joinSession->capitalUprimeLength, joinSession->capitalUprime);
	capitalUPrime_bytes = (BYTE *)malloc( TPM_DAA_SIZE_issuerModulus);
	if (capitalUPrime_bytes == NULL) {
		LogError("malloc of %d bytes failed", TPM_DAA_SIZE_issuerModulus);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_byte_array( capitalUPrime_bytes, TPM_DAA_SIZE_issuerModulus, capitalUPrime);
	EVP_DigestUpdate(&mdctx,  capitalUPrime_bytes, TPM_DAA_SIZE_issuerModulus);
	bi_free_ptr( capitalUPrime);
	daaCount = htonl( joinSession->daaCounter);
	EVP_DigestUpdate(&mdctx,  &daaCount, sizeof(UINT32));
	EVP_DigestUpdate(&mdctx,  digest_n0, digest_n0Length);
	contextHashLength = EVP_MD_CTX_size(&mdctx);
	contextHash = (BYTE *)malloc( contextHashLength);
	if (contextHash == NULL) {
		LogError("malloc of %d bytes failed", contextHashLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	EVP_DigestFinal(&mdctx, contextHash, NULL);
	EVP_DigestInit(&mdctx, DAA_PARAM_get_message_digest());
	LogDebug("PK(0).n=%s", dump_byte_array( TPM_DAA_SIZE_issuerModulus, modulus_N0_bytes));
	LogDebug("digestN0h=%s", dump_byte_array( digest_n0Length, digest_n0));
	LogDebug("UPrime=%s", dump_byte_array( TPM_DAA_SIZE_issuerModulus, capitalUPrime_bytes));
	LogDebug("daaCount=%4x", daaCount);

	LogDebug("contextHash[%d]=%s", contextHashLength, dump_byte_array( contextHashLength, contextHash));
	EVP_DigestUpdate(&mdctx,  contextHash, contextHashLength);
	EVP_DigestUpdate(&mdctx,  joinSession->nonceEncrypted, joinSession->nonceEncryptedLength);
	hashLength = EVP_MD_CTX_size(&mdctx);
	hash = (BYTE *)malloc( hashLength);
	if (hash == NULL) {
		LogError("malloc of %d bytes failed", hashLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	EVP_DigestFinal(&mdctx, hash, NULL);
	if( credentialRequest->authenticationProofLength != hashLength ||
		memcmp( credentialRequest->authenticationProof, hash, hashLength) != 0) {
		LogError("Verification of authenticationProof failed - Step 2.b");
		LogError("credentialRequest->authenticationProof[%d]=%s",
			credentialRequest->authenticationProofLength,
			dump_byte_array( credentialRequest->authenticationProofLength,
				credentialRequest->authenticationProof));
		LogError("internal cByte[%d]=%s",
			hashLength,
			dump_byte_array( hashLength, hash));
		result = TSS_E_DAA_AUTHENTICATION_ERROR;
		goto close;
	} else
		LogDebug("verify_authenticity Done:%s",
			dump_byte_array( hashLength, hash));
close:
	free( contextHash);
	free( digest_n0);
	free( capitalUPrime_bytes);
	free( hash);
	return result;
}

TSS_RESULT
compute_join_challenge_issuer( TSS_DAA_PK_internal *pk_intern,
							bi_ptr v_prime_prime,
							bi_ptr capitalA,
							bi_ptr capital_Atilde,
							UINT32 nonceReceiverLength,
							BYTE *nonceReceiver,
							UINT32 *c_primeLength,
							BYTE **c_prime) { // out allocation
	EVP_MD_CTX mdctx;
	BYTE *encoded_pk;
	BYTE *byte_array;
	UINT32 encoded_pkLength;

	byte_array = (BYTE *)malloc( DAA_PARAM_SIZE_RND_VALUE_CERTIFICATE / 8); // allocation
	if (byte_array == NULL) {
		LogError("malloc of %d bytes failed", DAA_PARAM_SIZE_RND_VALUE_CERTIFICATE / 8);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	EVP_DigestInit(&mdctx, DAA_PARAM_get_message_digest());
	encoded_pk = encoded_DAA_PK_internal( &encoded_pkLength, pk_intern);
	EVP_DigestUpdate(&mdctx,  encoded_pk, encoded_pkLength);
	LogDebug( "issuerPk: %s", dump_byte_array( encoded_pkLength, encoded_pk));
	bi_2_byte_array( byte_array, DAA_PARAM_SIZE_RND_VALUE_CERTIFICATE / 8, v_prime_prime);
	EVP_DigestUpdate(&mdctx, byte_array, DAA_PARAM_SIZE_RND_VALUE_CERTIFICATE / 8);
	LogDebug( "vPrimePrime: %s",
			dump_byte_array( DAA_PARAM_SIZE_RND_VALUE_CERTIFICATE / 8, byte_array));
	free( byte_array);
 	// allocation
	byte_array = (BYTE *)malloc( DAA_PARAM_SIZE_RSA_MODULUS / 8);
	if (byte_array == NULL) {
		LogError("malloc of %d bytes failed", DAA_PARAM_SIZE_RSA_MODULUS / 8);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	bi_2_byte_array( byte_array, DAA_PARAM_SIZE_RSA_MODULUS / 8, capitalA);
	EVP_DigestUpdate(&mdctx, byte_array, DAA_PARAM_SIZE_RSA_MODULUS / 8);
	LogDebug( "capitalA: %s", dump_byte_array( DAA_PARAM_SIZE_RSA_MODULUS / 8, byte_array));
	bi_2_byte_array( byte_array, DAA_PARAM_SIZE_RSA_MODULUS / 8, capital_Atilde);
	EVP_DigestUpdate(&mdctx, byte_array, DAA_PARAM_SIZE_RSA_MODULUS / 8);
	LogDebug( "capital_Atilde: %s",
			dump_byte_array( DAA_PARAM_SIZE_RSA_MODULUS / 8, byte_array));
	EVP_DigestUpdate(&mdctx, nonceReceiver, nonceReceiverLength);
	LogDebug( "nonceReceiver: %s",
			dump_byte_array( nonceReceiverLength, nonceReceiver));
	*c_primeLength = EVP_MD_CTX_size(&mdctx);
	*c_prime = (BYTE *)malloc( *c_primeLength);
	if (*c_prime == NULL) {
		LogError("malloc of %d bytes failed", *c_primeLength);
		free( byte_array);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	LogDebug( "c_prime: %s", dump_byte_array( *c_primeLength, *c_prime));
	EVP_DigestFinal(&mdctx, *c_prime, NULL);
	free( byte_array);
	return TSS_SUCCESS;
}

// inspired by computeCredentialProof (IssuerTransaction.java)
TSS_RESULT
compute_credential_proof( TSS_DAA_PK_internal *pk_intern,
						bi_ptr capital_A,
						bi_ptr fraction_A,
						bi_ptr eInverse,
						bi_ptr v_prime_prime,
						bi_ptr productPQprime,
						UINT32 noncePlatformLength,
						BYTE *noncePlatform,
						bi_ptr *c_prime,	// out
						bi_ptr *s_e	// out
) {
	bi_ptr random_E = bi_new_ptr();
	bi_ptr capital_Atilde = bi_new_ptr();
	BYTE *c_prime_bytes;
	UINT32 c_primeLength;

	bi_urandom( random_E, bi_length( productPQprime) + DAA_PARAM_SAFETY_MARGIN * 8);
	bi_mod( random_E, random_E, productPQprime);
	bi_inc( random_E);
	bi_mod_exp( capital_Atilde, fraction_A, random_E, pk_intern->modulus);
	compute_join_challenge_issuer( pk_intern,
								v_prime_prime,
								capital_A,
								capital_Atilde,
								noncePlatformLength,
								noncePlatform,
								&c_primeLength,
								&c_prime_bytes); // allocation
	*c_prime = bi_set_as_nbin( c_primeLength, c_prime_bytes); // allocation
	*s_e = bi_new_ptr();
	bi_mul( *s_e, *c_prime, eInverse);
	bi_mod( *s_e, *s_e, productPQprime);
	bi_sub( *s_e, random_E, *s_e);
	bi_mod( *s_e, *s_e, productPQprime);
	bi_free_ptr( capital_Atilde);
	bi_free_ptr( random_E);
	free( c_prime_bytes);
	return TSS_SUCCESS;
}

// from IssuerTransaction.java (joinStep2)
// stacks: TCGApplication.java (retrieveDAACredential) -> Issuer.java(issueCredential)
TSPICALL Tspi_DAA_IssueCredential_internal
(
	TSS_HDAA	hDAA,	// in
	UINT32	attributesIssuerLength,	// in
	BYTE**	attributesIssuer,	// in
	TSS_DAA_CREDENTIAL_REQUEST	credentialRequest,	// in
	TSS_DAA_JOIN_ISSUER_SESSION	joinSession,	// in
	TSS_DAA_CRED_ISSUER*	credIssuer	// out
) {
	TSS_RESULT result = TSS_SUCCESS;
	TCS_CONTEXT_HANDLE tcsContext;
	bi_ptr capitalU_hat_prime = NULL;
	bi_ptr tmp1;
	bi_ptr tmp2;
	bi_ptr sa_i;
	bi_ptr capitalU_prime = NULL;
	bi_ptr c = NULL;
	bi_ptr n = NULL;
	bi_ptr sf0 = NULL;
	bi_ptr sf1 = NULL;
	bi_ptr sv_prime = NULL;
	bi_ptr capitalR0 = NULL;
	bi_ptr capitalR1 = NULL;
	bi_ptr capitalS = NULL;
	bi_ptr capitalU = NULL;
	bi_ptr capitalU_hat = NULL;
	bi_ptr capitalN_hat_i = NULL;
	bi_ptr exp = NULL;
	bi_ptr product_attr_receiver = NULL;
	bi_ptr product_attr_issuer = NULL;
	bi_ptr sv_tilde_prime = NULL;
	bi_ptr capital_ni = NULL;
	bi_ptr v_hat = NULL;
	bi_ptr fraction_A = NULL;
	bi_ptr capitalA = NULL;
	bi_ptr e = NULL;
	bi_ptr eInverse = NULL;
	bi_ptr v_prime_prime = NULL;
	bi_ptr c_prime = NULL;
	bi_ptr s_e = NULL;
	bi_ptr zeta = NULL;
	TSS_DAA_PK *daa_pk_extern;
	TSS_DAA_PK_internal *pk_intern;
	TSS_DAA_PRIVATE_KEY *private_key;
	UINT32 i, chLength, challengeLength, length, interval;
	EVP_MD_CTX mdctx;
	BYTE *ch = NULL, *challenge = NULL;

	tmp1 = bi_new_ptr();
	tmp2 = bi_new_ptr();
	if( tmp1 == NULL || tmp2 == NULL) {
		LogError("malloc of BI <%s> failed", "tmp1, tmp2");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	if( (result = obj_daa_get_tsp_context( hDAA, &tcsContext)) != TSS_SUCCESS) goto close;
	// 1 TODO Check the TPM rogue list

	// 2 verify the authentication proof of the TPM
	result = verify_authentificity(&credentialRequest, &joinSession);
	if( result != TSS_SUCCESS) goto close;
	daa_pk_extern = (TSS_DAA_PK *)(((TSS_DAA_KEY_PAIR *)joinSession.issuerKeyPair)->public_key);
	pk_intern = e_2_i_TSS_DAA_PK( daa_pk_extern);
	n = bi_set_as_nbin( daa_pk_extern->modulusLength,
		daa_pk_extern->modulus); // allocation
	if( n == NULL) {
		LogError("malloc of BI <%s> failed", "n");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	capitalR0 = bi_set_as_nbin( daa_pk_extern->capitalR0Length,
		daa_pk_extern->capitalR0); // allocation
	if( capitalR0 == NULL) {
		LogError("malloc of BI <%s> failed", "capitalR0");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	capitalR1 = bi_set_as_nbin( daa_pk_extern->capitalR1Length,
		daa_pk_extern->capitalR1); // allocation
	if( capitalR1 == NULL) {
		LogError("malloc of BI <%s> failed", "capitalR1");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	capitalS = bi_set_as_nbin( daa_pk_extern->capitalSLength,
		daa_pk_extern->capitalS); // allocation
	if( capitalS == NULL) {
		LogError("malloc of BI <%s> failed", "capitalS");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	capitalU = bi_set_as_nbin( credentialRequest.capitalULength,
		credentialRequest.capitalU); // allocation
	if( capitalU == NULL) {
		LogError("malloc of BI <%s> failed", "capitalU");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	sv_tilde_prime = bi_set_as_nbin( credentialRequest.sVtildePrimeLength,
		credentialRequest.sVtildePrime); // allocation
	if( sv_tilde_prime == NULL) {
		LogError("malloc of BI <%s> failed", "sv_tilde_prime");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	capital_ni = bi_set_as_nbin( credentialRequest.capitalNiLength,
		credentialRequest.capitalNi); // allocation
	if( capital_ni == NULL) {
		LogError("malloc of BI <%s> failed", "capital_ni");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	// 3 Verify the correctness proof of the credential request
	// 3.a TODO commitments

	// 3.b
	capitalU_prime = bi_set_as_nbin( joinSession.capitalUprimeLength,
		joinSession.capitalUprime); // allocation
	if( capitalU_prime == NULL) {
		LogError("malloc of BI <%s> failed", "capitalU_prime");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	sf0 = bi_set_as_nbin( credentialRequest.sF0Length,
		credentialRequest.sF0); // allocation
	if( sf0 == NULL) {
		LogError("malloc of BI <%s> failed", "sf0");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	sf1 = bi_set_as_nbin( credentialRequest.sF1Length,
		credentialRequest.sF1); // allocation
	if( sf1 == NULL) {
		LogError("malloc of BI <%s> failed", "sf1");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	sv_prime = bi_set_as_nbin( credentialRequest.sVprimeLength,
		credentialRequest.sVprime); // allocation
	if( sv_prime == NULL) {
		LogError("malloc of BI <%s> failed", "sv_prime");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	c = bi_set_as_nbin( credentialRequest.challengeLength,
		credentialRequest.challenge); // allocation
	if( c == NULL) {
		LogError("malloc of BI <%s> failed", "c");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	capitalU_hat_prime = bi_new_ptr();// allocation
	if( capitalU_hat_prime == NULL) {
		LogError("malloc of BI <%s> failed", "c");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	// capitalU_hat_prime = capitalU_prime ~% n
	bi_invert_mod( capitalU_hat_prime, capitalU_prime, n);
	// capitalU_hat_prime = ( capitalU_hat_prime ^ c ) % n
	bi_mod_exp( capitalU_hat_prime, capitalU_hat_prime, c, n);
	// capitalU_hat_prime = ( capitalU_hat_prime * ( capitalR0 ^ sf0)) % n
	bi_mod_exp( tmp1, capitalR0, sf0, n);
	bi_mul( capitalU_hat_prime, capitalU_hat_prime, tmp1);
	bi_mod( capitalU_hat_prime, capitalU_hat_prime, n);
	// capitalU_hat_prime = ( capitalU_hat_prime * ( capitalR1 ^ sf1)) % n
	bi_mod_exp( tmp1, capitalR1, sf1, n);
	bi_mul( capitalU_hat_prime, capitalU_hat_prime, tmp1);
	bi_mod( capitalU_hat_prime, capitalU_hat_prime, n);
	// capitalU_hat_prime = ( capitalU_hat_prime * ( capitalS ^ sv_prime)) % n
	bi_mod_exp( tmp1, capitalS, sv_prime, n);
	bi_mul( capitalU_hat_prime, capitalU_hat_prime, tmp1);
	bi_mod( capitalU_hat_prime, capitalU_hat_prime, n);
	// verify blinded encoded attributes of the Receiver
	product_attr_receiver = bi_new_ptr();
	bi_set( product_attr_receiver, bi_1);
	length = ( DAA_PARAM_SIZE_RANDOMIZED_ATTRIBUTES + 7) / 8;
	for( i=0; i<credentialRequest.sALength; i++) {
		sa_i = bi_set_as_nbin( length, credentialRequest.sA[i]); // allocation
		if( sa_i == NULL) {
			LogError("malloc of BI <%s> failed", "sa_i");
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		bi_mod_exp( tmp1, pk_intern->capitalRReceiver->array[i], sa_i, n);
		bi_mul( product_attr_receiver, product_attr_receiver, tmp1);
		bi_mod( product_attr_receiver, product_attr_receiver, n);
		bi_free_ptr( sa_i);
	}
	// tmp1 = ( 1 / capitalU ) % n
	bi_invert_mod( tmp1, capitalU, n);
	capitalU_hat = bi_new_ptr();
	if( capitalU_hat == NULL) {
		LogError("malloc of BI <%s> failed", "capitalU_hat");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_mul( capitalU_hat, capitalU_prime, tmp1);
	// capitalU_hat = capitalU_prime / capitalU
	bi_mod( capitalU_hat, capitalU_hat, n);
	// capital_Uhat = ( (capital_Uhat ^ c ) % n
	bi_mod_exp( capitalU_hat, capitalU_hat, c, n);
	// capital_Uhat = ( capital_Uhat * ( capitalS ^ sv_tilde_prime) % n ) % n
	bi_mod_exp( tmp1, pk_intern->capitalS, sv_tilde_prime, n);
	bi_mul( capitalU_hat, capitalU_hat, tmp1);
	bi_mod( capitalU_hat, capitalU_hat, n);
	bi_mul( capitalU_hat, capitalU_hat, product_attr_receiver);
	bi_mod( capitalU_hat, capitalU_hat, n);
	// capital_Nhat_i = (( capital_Ni ~% pk_intern->capitalGamma ) ^ c ) % pk_intern->capitalGamma
	capitalN_hat_i = bi_new_ptr();
	bi_invert_mod( capitalN_hat_i, capital_ni, pk_intern->capitalGamma);
	bi_mod_exp( capitalN_hat_i, capitalN_hat_i, c, pk_intern->capitalGamma);
	// exp = sf1 << (DAA_PARAM_SIZE_F_I) + sf0
	exp = bi_new_ptr();
	if( exp == NULL) {
		LogError("malloc of BI <%s> failed", "exp");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_shift_left( exp, sf1, DAA_PARAM_SIZE_F_I);
	bi_add( exp, exp, sf0);
	zeta = compute_zeta( pk_intern->issuerBaseNameLength,
					pk_intern->issuerBaseName,
					pk_intern);
	// capital_Nhat_i = ( capital_Nhat_i *
	//			( ( issuer.zeta ^ exp) % pk->capitalGamma) ) % pk->capitalGamma
	bi_mod_exp( tmp1, zeta, exp, pk_intern->capitalGamma);
	bi_mul( capitalN_hat_i, capitalN_hat_i, tmp1);
	bi_mod( capitalN_hat_i, capitalN_hat_i, pk_intern->capitalGamma);

	LogDebug("calculation Uhat:                capitalS:%s\n", bi_2_hex_char( pk_intern->capitalS));
	LogDebug("calculation Uhat:       sv_tilde_prime:%s\n", bi_2_hex_char( sv_tilde_prime));
	LogDebug("calculation Uhat:                          n:%s\n", bi_2_hex_char( n));
	LogDebug("calculation Uhat: product_attributes:%s\n", bi_2_hex_char( product_attr_receiver));
	LogDebug("calculation NhatI:                     zeta:%s\n", bi_2_hex_char( zeta));
	LogDebug("calculation NhatI:                      exp:%s\n", bi_2_hex_char( exp));
	LogDebug("calculation NhatI:      capitalGamma:%s\n", bi_2_hex_char( pk_intern->capitalGamma));
	// calculate challenge
	result = compute_join_challenge_host(hDAA,
							pk_intern,
							capitalU,
							capitalU_prime,
							capitalU_hat,
							capitalU_hat_prime,
							capital_ni,
							capitalN_hat_i,
							0, // TODO: commitmentsProofLength
							NULL, // TODO: commits
							joinSession.nonceIssuerLength,
							joinSession.nonceIssuer,
							&chLength,	// out
							&ch);		// out allocation
	if( result != TSS_SUCCESS) goto close;
	LogDebug("JoinChallengeHost: %s", dump_byte_array( chLength, ch));
	EVP_DigestInit(&mdctx, DAA_PARAM_get_message_digest());
	EVP_DigestUpdate(&mdctx,  ch, chLength);
	challengeLength = EVP_MD_CTX_size( &mdctx);
	challenge = (BYTE *)malloc( challengeLength);
	if( challenge == NULL) {
		LogError("malloc of %d bytes failed", challengeLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	EVP_DigestUpdate(&mdctx,  credentialRequest.nonceTpm, credentialRequest.nonceTpmLength);
	EVP_DigestFinal(&mdctx, challenge, NULL);
	// checks
	if( credentialRequest.challengeLength != challengeLength ||
		memcmp( credentialRequest.challenge, challenge, challengeLength)!=0) {
		LogError("Verification of c failed - Step 3.f.i");
		LogError("credentialRequest.challenge[%d]=%s",
			credentialRequest.challengeLength,
			dump_byte_array( credentialRequest.challengeLength,
				credentialRequest.challenge));
		LogError("challenge[%d]=%s",
			challengeLength,
			dump_byte_array( challengeLength, challenge));
		result = TSS_E_DAA_CREDENTIAL_REQUEST_PROOF_ERROR;
		goto close;
	}
	// + 1 because the result of ( rA(43 bits)  + c(20 bits) * a(13 bits)) can
	// shift 1 bit above the normal size (43 bits)
	length = DAA_PARAM_SIZE_RANDOMIZED_ATTRIBUTES + 1;
	if( bi_length( sf0) > (long)length) {
		LogError( "Verification of sF0 failed - Step 3.f.ii");
		LogError("\tsf0 bits length: %d  expected maximum length:%d\n",
				(int)bi_length( sf0), (int)length);
		result = TSS_E_DAA_CREDENTIAL_REQUEST_PROOF_ERROR;
		goto close;
	}
	if( bi_length( sf1) > (long)length) {
		LogError( "Verification of sF1 failed - Step 3.f.ii");
		LogError("\tsf1 length: %d  expected maximum length:%d\n",
				(int)bi_length( sf1), (int)length);
		result = TSS_E_DAA_CREDENTIAL_REQUEST_PROOF_ERROR;
		goto close;
	}
	// blinded attributes
	length = DAA_PARAM_SIZE_RANDOMIZED_ATTRIBUTES;
	for( i=0; i<credentialRequest.sALength; i++) {
		sa_i = bi_set_as_nbin( ( length + 7) / 8, credentialRequest.sA[i]); // allocation
		if( sa_i == NULL) {
			LogError("malloc of BI <%s> failed", "sa_i");
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		if( bi_length( sa_i) > (long)length) {
			LogError("Verification of sA[%d] failed - Step 3.f.ii", i);
			LogError("sA.length=%d length=%d", (int)bi_length( sa_i), length);
			result = TSS_E_DAA_CREDENTIAL_REQUEST_PROOF_ERROR;
			goto close;
		}
		bi_free_ptr( sa_i);
		if( result != TSS_SUCCESS) goto close;
	}
	length = DAA_PARAM_SIZE_RSA_MODULUS + 2 * DAA_PARAM_SAFETY_MARGIN +
			DAA_PARAM_SIZE_MESSAGE_DIGEST;
	if( bi_length( sv_prime) > (int)length) {
		LogError("Verification of sVprime failed - Step 3.f.iii\n");
		LogError("\tsv_prime bits length: %d  expected maximum length:%d\n",
				(int)bi_length( sv_prime), (int)length);
		result = TSS_E_DAA_CREDENTIAL_REQUEST_PROOF_ERROR;
		goto close;
	}
	if( bi_nbin_size( sv_tilde_prime) > (int)length) {
		LogError("Verification of sVtildePrime failed - Step 3.f.iii");
		LogError("\tsv_tilde_prime bits length: %d  expected maximum length:%d\n",
				(int)bi_length( sv_tilde_prime), (int)length);
		result = TSS_E_DAA_CREDENTIAL_REQUEST_PROOF_ERROR;
		goto close;
	}
	// compute credential
	v_hat = bi_new_ptr();
	if( v_hat == NULL) {
		LogError("malloc of BI <%s> failed", "v_hat");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_urandom( v_hat, DAA_PARAM_SIZE_RND_VALUE_CERTIFICATE - 1);
	length = DAA_PARAM_SIZE_EXPONENT_CERTIFICATE;
	interval = DAA_PARAM_SIZE_INTERVAL_EXPONENT_CERTIFICATE;
	e = bi_new_ptr();
	if( e == NULL) {
		LogError("malloc of BI <%s> failed", "e");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	compute_prime( e, length, interval);

	// v'' = ( 1 << DAA_PARAM_SIZE_RND_VALUE_CERTIFICATE) + v_hat
	v_prime_prime = bi_new_ptr();
	bi_shift_left( tmp1, bi_1, DAA_PARAM_SIZE_RND_VALUE_CERTIFICATE - 1);
	bi_add( v_prime_prime, tmp1, v_hat);

	// fraction_A = (( pk->capitalS ^ v``) % n) * capitalU
	fraction_A = bi_new_ptr();
	if( fraction_A == NULL) {
		LogError("malloc of BI <%s> failed", "fraction_A");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_mod_exp( fraction_A, pk_intern->capitalS, v_prime_prime, n);
	bi_mul( fraction_A, fraction_A, capitalU);
	bi_mod( fraction_A, fraction_A, n);

	// encode attributes
	bi_free_ptr( tmp1);
	product_attr_issuer = bi_new_ptr();
	if( product_attr_issuer == NULL) {
		LogError("malloc of BI <%s> failed", "product_attr_issuer");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_set( product_attr_issuer, bi_1);
	for( i=0; i< attributesIssuerLength; i++) {
		tmp1 = bi_set_as_nbin( DAA_PARAM_SIZE_F_I / 8, attributesIssuer[i]); // allocation
		bi_mod_exp( tmp2, pk_intern->capitalRIssuer->array[i], tmp1, n);
		bi_mul( product_attr_issuer, product_attr_issuer, tmp2);
		bi_mod( product_attr_issuer, product_attr_issuer, n);
		bi_free_ptr( tmp1);
	}
	tmp1 = bi_new_ptr();
	if( tmp1 == NULL) {
		LogError("malloc of BI <%s> failed", "tmp1");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_mul( fraction_A, fraction_A, product_attr_issuer);
	bi_mod( fraction_A, fraction_A, n);

	bi_invert_mod( fraction_A, fraction_A, n);
	bi_mul( fraction_A, fraction_A, pk_intern->capitalZ);
	bi_mod( fraction_A, fraction_A, n);

	private_key = (TSS_DAA_PRIVATE_KEY *)
				(((TSS_DAA_KEY_PAIR *)joinSession.issuerKeyPair)->private_key);
	bi_free_ptr( tmp2);
	tmp2 = bi_set_as_nbin( private_key->productPQprimeLength,
		private_key->productPQprime); // allocation
	if( tmp2 == NULL) {
		LogError("malloc of BI <%s> failed", "tmp2");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	eInverse = bi_new_ptr();
	if( eInverse == NULL) {
		LogError("malloc of BI <%s> failed", "eInverse");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_invert_mod( eInverse, e, tmp2);
	capitalA = bi_new_ptr();
	if( capitalA == NULL) {
		LogError("malloc of BI <%s> failed", "capitalA");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	LogDebug("fraction_A[%ld]=%s", bi_nbin_size( fraction_A), bi_2_hex_char( fraction_A));
	LogDebug("eInverse[%ld]=%s", bi_nbin_size( eInverse), bi_2_hex_char( eInverse));
	LogDebug("productPQprime[%ld]=%s", bi_nbin_size( tmp2), bi_2_hex_char( tmp2));
	LogDebug("eInverse[%ld]=%s", bi_nbin_size( eInverse), bi_2_hex_char( eInverse));
	LogDebug("e[%ld]=%s", bi_nbin_size( e), bi_2_hex_char( e));
	LogDebug("n[%ld]=%s", bi_nbin_size( n), bi_2_hex_char( n));
	bi_mod_exp( capitalA, fraction_A, eInverse, n);

	compute_credential_proof( pk_intern,
				capitalA,
				fraction_A,
				eInverse,
				v_prime_prime,
				tmp2, // productPQprime
				credentialRequest.noncePlatformLength,
				credentialRequest.noncePlatform,
				&c_prime,	// out: allocation
				&s_e);	// out: allocation
	// populate credIssuer (TSS_DAA_CRED_ISSUER *)
	credIssuer->capitalA = calloc_tspi( tcsContext, bi_nbin_size( capitalA));
	if( credIssuer->capitalA == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( capitalA));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(credIssuer->capitalALength), credIssuer->capitalA, capitalA);
	credIssuer->e = calloc_tspi( tcsContext, bi_nbin_size( e));
	if( credIssuer->e == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( e));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(credIssuer->eLength), credIssuer->e, e);
	credIssuer->vPrimePrime = calloc_tspi( tcsContext, bi_nbin_size( v_prime_prime));
	if( credIssuer->vPrimePrime == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( v_prime_prime));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(credIssuer->vPrimePrimeLength), credIssuer->vPrimePrime, v_prime_prime);
	// attributes issuer
	credIssuer->attributesIssuerLength = attributesIssuerLength;
	credIssuer->attributesIssuer = calloc_tspi( tcsContext,
						attributesIssuerLength * sizeof( BYTE *));
	if( credIssuer->attributesIssuer == NULL) {
		LogError("malloc of %d bytes failed", attributesIssuerLength * sizeof( BYTE *));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	for( i=0; i< attributesIssuerLength; i++) {
		credIssuer->attributesIssuer[i] = calloc_tspi( tcsContext, DAA_PARAM_SIZE_F_I / 8);
		if( credIssuer->attributesIssuer[i] == NULL) {
			LogError("malloc of %d bytes failed", DAA_PARAM_SIZE_F_I / 8);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		memcpy( credIssuer->attributesIssuer[i], attributesIssuer[i], DAA_PARAM_SIZE_F_I / 8);
	}
	credIssuer->cPrime = calloc_tspi( tcsContext, bi_nbin_size( c_prime));
	if( credIssuer->cPrime == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( c_prime));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(credIssuer->cPrimeLength), credIssuer->cPrime, c_prime);
	credIssuer->sE = calloc_tspi( tcsContext, bi_nbin_size( s_e));
	if( credIssuer->sE == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( s_e));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(credIssuer->sELength), credIssuer->sE, s_e);

close:
	//free_TSS_DAA_PK( daa_pk_extern);
	if( ch != NULL) free( ch);
	if( challenge != NULL) free( challenge);
	FREE_BI( tmp1);
	FREE_BI( tmp2);
	FREE_BI( s_e);
	FREE_BI( c_prime);
	FREE_BI( capitalA);
	FREE_BI( v_prime_prime);
	FREE_BI( eInverse);
	FREE_BI( e);
	FREE_BI( fraction_A);
	FREE_BI( v_hat);
	FREE_BI( capital_ni);
	FREE_BI( sv_tilde_prime);
	FREE_BI( product_attr_receiver);
	FREE_BI( product_attr_issuer);
	FREE_BI( capitalU_hat_prime);
	FREE_BI( capitalU_prime);
	FREE_BI( sv_prime);
	FREE_BI( exp);
	FREE_BI( capitalN_hat_i);
	FREE_BI( capitalU_hat);
	FREE_BI( capitalU);
	FREE_BI( capitalS);
	FREE_BI( capitalR1);
	FREE_BI( capitalR0);
	FREE_BI( sf1);
	FREE_BI( sf0);
	FREE_BI( n);
	FREE_BI( c);
	FREE_BI( zeta);
	return result;
}
