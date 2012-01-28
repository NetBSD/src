
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
#include "verifier.h"

#include "trousers/tss.h"
#include "spi_internal_types.h"
#include "spi_utils.h"

#include "anonymity_revocation.h"

DAA_VERIFIER_TRANSACTION *create_verifier_transaction( int length, char *base_name) {
	DAA_VERIFIER_TRANSACTION *verifier_transaction =
		malloc(sizeof(DAA_VERIFIER_TRANSACTION));

	if (verifier_transaction == NULL) {
		LogError("malloc of %d bytes failed", sizeof(DAA_VERIFIER_TRANSACTION));
		return NULL;
	}
	verifier_transaction->baseName = base_name;
	verifier_transaction->baseName_length = length;
	OpenSSL_add_all_digests();
	verifier_transaction->digest = DAA_PARAM_get_message_digest();
	return verifier_transaction;
}

static int verifyNonce( BYTE *nonce_verifier, int length) {
	//TODO check nonce_verifier with the current transaction nonce
	return 1;
}

BYTE *compute_bytes( int seedLength, BYTE *seed, int length, const EVP_MD *digest) {
	EVP_MD_CTX mdctx;
	int N;
	BYTE *hash;
	BYTE *result;
	int i, big_indian_i, len_hash;

	result = (BYTE *)malloc( length);
	if (result == NULL) {
		LogError("malloc of %d bytes failed", length);
		return NULL;
	}
	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, digest, NULL);
	len_hash = EVP_MD_size(digest);
	N = length / len_hash;
	hash = (BYTE *)malloc( len_hash);
	if (hash == NULL) {
		LogError("malloc of %d bytes failed", len_hash);
		return NULL;
	}
	for( i=0; i<N; i++) {
		EVP_DigestUpdate(&mdctx,  seed, seedLength);
		big_indian_i = htonl( i);
		EVP_DigestUpdate(&mdctx,  &big_indian_i, sizeof( int));
		EVP_DigestFinal_ex(&mdctx, &result[ i * len_hash], NULL);
		EVP_DigestInit_ex(&mdctx, digest, NULL);
	}
	// fill up the rest of the array (i=N)
	EVP_DigestUpdate(&mdctx,  seed, seedLength);
	big_indian_i = htonl( i);
	EVP_DigestUpdate(&mdctx,  &big_indian_i, sizeof( int));
	EVP_DigestFinal(&mdctx, hash, NULL);
	// copy the rest:  base_nameLength % len_hash bytes
	memcpy( &result[ i * len_hash], hash, length - N * len_hash);
	free( hash);
	return result;
}

/* from DAAUtil */
bi_ptr project_into_group_gamma( bi_ptr base, TSS_DAA_PK_internal *issuer_pk) {
	bi_t exponent; bi_new( exponent);
	bi_ptr capital_gamma  = issuer_pk->capitalGamma;
	bi_ptr rho = issuer_pk->rho;
	bi_ptr zeta = bi_new_ptr();

	if( capital_gamma == NULL ||
		rho == NULL ||
		zeta == NULL) return NULL;
	// exponent = capital_gamma - 1
	bi_sub( exponent, capital_gamma, bi_1);
	// exponent = exponent / rho
	bi_div( exponent, exponent, rho);
	// zeta = ( base ^ exponent) % capital_gamma
	LogDebug("project_into_group_gamma:           rho      [%ld]:%s",
			bi_nbin_size( rho), bi_2_hex_char( rho));
	LogDebug("project_into_group_gamma:               base[%ld]:%s",
			bi_nbin_size( base), bi_2_hex_char( base));
	LogDebug("project_into_group_gamma: exponent       [%ld]:%s",
			bi_nbin_size( exponent), bi_2_hex_char( exponent));
	LogDebug("project_into_group_gamma: capitalGamma[%ld]:%s",
		bi_nbin_size( capital_gamma),
		bi_2_hex_char( capital_gamma));
	bi_mod_exp( zeta, base, exponent, capital_gamma);
	LogDebug("project_into_group_gamma: result:%s", bi_2_hex_char( zeta));
	bi_free( exponent);
	return zeta;
}

bi_ptr compute_zeta( int nameLength, unsigned char *name, TSS_DAA_PK_internal *issuer_pk) {
	BYTE *bytes;
	bi_ptr base;
	bi_ptr result;

	LogDebug("compute_zeta: %d [%s] pk:%x", nameLength, name, (int)issuer_pk);
	bytes = compute_bytes( nameLength,
				name,
				DAA_PARAM_LENGTH_MFG1_GAMMA,
				DAA_PARAM_get_message_digest());
	if( bytes == NULL) return NULL;
	base = bi_set_as_nbin( DAA_PARAM_LENGTH_MFG1_GAMMA, bytes);
	if( base == NULL) return NULL;
	LogDebug("base: %ld [%s]", bi_nbin_size( base), bi_2_hex_char( base));
	result = project_into_group_gamma( base, issuer_pk);
	if( result == NULL) return NULL;
	bi_free_ptr( base);
	free( bytes);
	LogDebug("return zeta:%s\n", bi_2_hex_char( result));
	return result;
}

bi_ptr compute_parameterized_gamma(int k, TSS_DAA_PK_internal *issuer_pk) {
	int length;
	int hashLength = bi_nbin_size( issuer_pk->gamma) + sizeof(int);
	BYTE *hash;
	int big_indian_k = htonl( k);
	BYTE *bytes;
	bi_ptr value, result;

	hash = (BYTE *)malloc( hashLength);
	if (hash == NULL) {
		LogError("malloc of %d bytes failed", hashLength);
		return NULL;
	}
	// hash[0-3] = big_indian(k)
	memcpy( hash, &big_indian_k, sizeof(int));
	// hash[4-end] = issuer_pk->gamma
	bi_2_nbin1( &length, &hash[sizeof(int)], issuer_pk->gamma);
	// allocation
	bytes = compute_bytes( hashLength, hash,
				DAA_PARAM_LENGTH_MFG1_GAMMA,
				DAA_PARAM_get_message_digest());
	if( bytes == NULL) return NULL;
	// allocation
	value = bi_set_as_nbin( DAA_PARAM_LENGTH_MFG1_GAMMA, bytes);
	if( value == NULL) return NULL;
	result = project_into_group_gamma( value, issuer_pk); // allocation
	if (result == NULL) {
		LogError("malloc of %d bytes failed", hashLength);
		return NULL;
	}
	bi_free_ptr( value);
	free( bytes);
	return result;
}

inline bi_ptr apply_challenge( bi_ptr value, bi_ptr delta, bi_ptr c, bi_ptr capital_gamma) {
	bi_ptr delta_tilde = bi_new_ptr();
	bi_t c_negate;

	if( delta_tilde == NULL) return NULL;
	bi_new( c_negate);
	bi_set( c_negate, c);
	bi_negate( c_negate);
	// delta_tilde = ( delta ^ (-c)) % capital_gamma
	bi_mod_exp( delta_tilde, delta, c_negate, capital_gamma);
	bi_free( c_negate);
	// delta_tilde = (delta_tilde * value) % capital_gamma
	return bi_mod( delta_tilde, bi_mul( delta_tilde, delta_tilde, value), capital_gamma);
}

DAA_VERIFIER_TRANSACTION *createTransaction(int baseName_length, BYTE* baseName) {
	DAA_VERIFIER_TRANSACTION *result =
		(DAA_VERIFIER_TRANSACTION *)malloc( sizeof(DAA_VERIFIER_TRANSACTION));

	if (result == NULL) {
		LogError("malloc of %d bytes failed", sizeof(DAA_VERIFIER_TRANSACTION));
		return NULL;
	}
	result->baseName = baseName;
	result->baseName_length = baseName_length;
	return result;
}

void update( EVP_MD_CTX *mdctx, char *name, bi_ptr integer, int bitLength) {
	int length = bitLength / 8;
	BYTE buffer[length];

	bi_2_byte_array( buffer, length, integer);
	LogDebug("[update] %s:%s", name, dump_byte_array( length, buffer));
	EVP_DigestUpdate(mdctx, buffer, length);
}

BYTE *compute_sign_challenge_host(
	int *result_length,
	EVP_MD *digest,
	TSS_DAA_PK_internal *issuer_pk,
	int nonce_verifierLength,
	BYTE *nonce_verifier,
	int selected_attributes2commitLength,
	TSS_DAA_SELECTED_ATTRIB **selected_attributes2commit,
	int is_anonymity_revocation_enabled,
	bi_ptr zeta,
	bi_ptr capital_t,
	bi_ptr capital_tilde,
	int attribute_commitmentsLength,
	TSS_DAA_ATTRIB_COMMIT_internal **attribute_commitments,
	TSS_DAA_ATTRIB_COMMIT_internal **attribute_commitment_proofs,
	bi_ptr capital_nv,
	bi_ptr capital_tilde_v,
	CS_PUBLIC_KEY *anonymity_revocator_pk,
	CS_ENCRYPTION_RESULT *encryption_result_rand,
	CS_ENCRYPTION_RESULT *encryption_result_proof
) {
	EVP_MD_CTX mdctx;
	int i, length;
	unsigned int big_indian;
	BYTE *buffer;
	int length_gamma_modulus;
	BYTE *buffer1;

	LogDebug("issuer_pk basename[%d]:%s",
		issuer_pk->issuerBaseNameLength,
		dump_byte_array( issuer_pk->issuerBaseNameLength,
				issuer_pk->issuerBaseName));
	LogDebug("nonce_verifier[%d]:%s",
		nonce_verifierLength,
		dump_byte_array( nonce_verifierLength, nonce_verifier));
	LogDebug("selected_attributes2commitLength:%d", selected_attributes2commitLength);
	LogDebug("is_anonymity_revocation_enabled:%d", is_anonymity_revocation_enabled);
	LogDebug("zeta[%ld]:%s", bi_nbin_size( zeta), bi_2_hex_char( zeta));
	LogDebug("capital_t[%ld]:%s", bi_nbin_size( capital_t), bi_2_hex_char( capital_t));
	LogDebug("capital_tilde[%ld]:%s",
		bi_nbin_size( capital_tilde),
		bi_2_hex_char( capital_tilde));
	LogDebug("attribute_commitmentsLength:%d", attribute_commitmentsLength);
	LogDebug("attribute_commitments:%d", (int)attribute_commitments);
	LogDebug("attribute_commitment_proofs:%d", (int)attribute_commitment_proofs);
	LogDebug("capital_nv[%ld]:%s", bi_nbin_size( capital_nv), bi_2_hex_char( capital_nv));
	LogDebug("capital_tilde_v[%ld]:%s",
		bi_nbin_size( capital_tilde_v),
		bi_2_hex_char( capital_tilde_v));
	LogDebug("anonymity_revocator_pk:%d", (int)anonymity_revocator_pk);
	LogDebug("encryption_result_rand:%d", (int)encryption_result_rand);
	LogDebug("encryption_result_proof:%d", (int)encryption_result_proof);

	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, digest, NULL);
	// update with encoded PK
	buffer = encoded_DAA_PK_internal( &length, issuer_pk);
	if( buffer == NULL) return NULL;
	LogDebug("encoded issuer_pk[%d]:%s", length, dump_byte_array( length, buffer));
	EVP_DigestUpdate(&mdctx, buffer , length);
	free( buffer);
	// nonce verifier
	EVP_DigestUpdate(&mdctx, nonce_verifier , nonce_verifierLength);
	// length Commitments
	big_indian = attribute_commitmentsLength;
	EVP_DigestUpdate(&mdctx, &big_indian, sizeof(int));
	// Anonymity enabled
	big_indian = is_anonymity_revocation_enabled;
	EVP_DigestUpdate(&mdctx, &big_indian, sizeof(int));

	update( &mdctx, "zeta", zeta, DAA_PARAM_SIZE_MODULUS_GAMMA);
	update( &mdctx, "capitalT", capital_t, DAA_PARAM_SIZE_RSA_MODULUS);
	update( &mdctx, "capitalTTilde", capital_tilde, DAA_PARAM_SIZE_RSA_MODULUS);

	length_gamma_modulus = DAA_PARAM_SIZE_MODULUS_GAMMA / 8;
	buffer = (BYTE *)malloc( length_gamma_modulus);// allocation
	if (buffer == NULL) {
		LogError("malloc of %d bytes failed", length_gamma_modulus);
		return NULL;
	}
	if( selected_attributes2commitLength > 0) {
		for( i=0; i<selected_attributes2commitLength; i++) {
			buffer1 = to_bytes_TSS_DAA_SELECTED_ATTRIB_internal(
				&length,
				selected_attributes2commit[i]);
			EVP_DigestUpdate(&mdctx, buffer1, length);
			free( buffer1);
			bi_2_byte_array( buffer,
					length_gamma_modulus,
					attribute_commitments[i]->beta);
			EVP_DigestUpdate(&mdctx,
					buffer,
					length_gamma_modulus);
			bi_2_byte_array( buffer,
					length_gamma_modulus,
					attribute_commitment_proofs[i]->beta);
			EVP_DigestUpdate(&mdctx,
					buffer,
					length_gamma_modulus);
		}
	}
	if( !is_anonymity_revocation_enabled) {
		// Nv, N~v
		bi_2_byte_array( buffer, length_gamma_modulus, capital_nv);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, capital_tilde_v);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
	} else {
		bi_2_byte_array( buffer, length_gamma_modulus, anonymity_revocator_pk->eta);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, anonymity_revocator_pk->lambda1);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, anonymity_revocator_pk->lambda2);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, anonymity_revocator_pk->lambda3);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, encryption_result_rand->c1);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, encryption_result_rand->c2);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, encryption_result_rand->c3);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, encryption_result_rand->c4);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, encryption_result_proof->c1);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, encryption_result_proof->c2);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, encryption_result_proof->c3);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
		bi_2_byte_array( buffer, length_gamma_modulus, encryption_result_proof->c4);
		EVP_DigestUpdate(&mdctx, buffer, length_gamma_modulus);
	}
	free(buffer);
	buffer = (BYTE *)malloc(EVP_MD_size(digest)); // allocation
	if (buffer == NULL) {
		LogError("malloc of %d bytes failed", EVP_MD_size(digest));
		return NULL;
	}
	EVP_DigestFinal_ex(&mdctx, buffer, result_length);
	EVP_MD_CTX_cleanup(&mdctx);
	LogDebug("compute_sign_challenge_host[%d]:%s",
		*result_length,
		dump_byte_array( *result_length, buffer));
	return buffer;
}

inline int is_element_gamma( bi_ptr capital_nv, TSS_DAA_PK_internal *issuer_pk) {
	bi_ptr tmp1 = bi_new_ptr();
	int result;

	//	( ( capital_nv ^ issuer_pk->rho ) % issuer_pk->capitalGamma ) == 1
	result = bi_equals( bi_mod_exp( tmp1,
					capital_nv,
					issuer_pk->rho,
					issuer_pk->capitalGamma),
				bi_1);
	bi_free_ptr( tmp1);
	return result;
}

/* implementation  derived from isValid (VerifierTransaction.java) */
TSPICALL Tspi_DAA_VerifySignature_internal
(	TSS_HDAA hDAA,	// in
	TSS_DAA_SIGNATURE signature_ext, // in
	TSS_HKEY hPubKeyIssuer,	// in
	TSS_DAA_SIGN_DATA sign_data,	// in
	UINT32 attributesLength,	// in
	BYTE **attributes,	// in
	UINT32 nonce_verifierLength,	// out
	BYTE *nonce_verifier,	// out
	UINT32 base_nameLength,	// out
	BYTE *base_name,	// out
	TSS_BOOL *isCorrect	// out
) {
	int i, j;
	DAA_VERIFIER_TRANSACTION *verifier_transaction = NULL;
	TSS_DAA_ATTRIB_COMMIT *commitments;
	TSS_DAA_PK_internal *issuer_pk;
	TSS_DAA_SIGNATURE_internal *signature = NULL;
	bi_ptr tmp1;
	bi_array_ptr sA;
	bi_ptr n = NULL;
	bi_ptr c = NULL;
	bi_ptr capital_gamma = NULL;
	bi_ptr zeta_2_verify = NULL;
	bi_ptr capital_z = NULL;
	bi_array_ptr capital_R = NULL;
	bi_ptr product_r = NULL;
	bi_ptr exp = NULL;
	bi_ptr capital_THat = NULL;
	bi_ptr beta_tilde = NULL;
	bi_ptr gamma_i = NULL;
	bi_ptr capital_nv = NULL;
	bi_ptr capital_ntilde_v = NULL;
	bi_ptr pseudonym_projected = NULL;
	bi_ptr s_tau = NULL;
	bi_ptr delta_tilde1 = NULL;
	bi_ptr delta_tilde2 = NULL;
	bi_ptr delta_tilde3 = NULL;
	bi_ptr delta_tilde4 = NULL;
	bi_ptr attribute_i;
	TSS_DAA_PSEUDONYM_PLAIN *pseudonym_plain;
	CS_ENCRYPTION_RESULT *pseudonym_enc = NULL;
	CS_ENCRYPTION_RESULT *pseudonym_encryption_proof = NULL;
	TSS_DAA_PSEUDONYM_ENCRYPTED_internal *sig_pseudonym_encrypted = NULL;
	CS_ENCRYPTION_RESULT_RANDOMNESS *result_random = NULL;
	CS_ENCRYPTION_RESULT *encryption_result = NULL;
	TSS_DAA_ATTRIB_COMMIT_internal **commitment_proofs = NULL;
	TCS_CONTEXT_HANDLE tcsContext;
	TSS_RESULT result = TSS_SUCCESS;
	EVP_MD_CTX mdctx;
	int length_ch, len_hash, bits;
	BYTE *ch = NULL, *hash = NULL;
	TSS_BOOL *indices;

	tmp1 = bi_new_ptr();
	if( tmp1 == NULL) {
		LogError("malloc of BI <%s> failed", "tmp1");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	*isCorrect = FALSE;
	if( (result = obj_daa_get_tsp_context( hDAA, &tcsContext)) != TSS_SUCCESS)
		goto close;
	// allocation of issuer_pk
	issuer_pk = e_2_i_TSS_DAA_PK( (TSS_DAA_PK *)hPubKeyIssuer);
	if( issuer_pk == NULL) {
		LogError("malloc of TSS_DAA_PK_internal failed");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	// allocation of signature
	signature = e_2_i_TSS_DAA_SIGNATURE( &signature_ext);
	if( signature == NULL) {
		LogError("malloc of TSS_DAA_SIGNATURE_internal failed");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	commitments = signature_ext.attributeCommitments;
	// TODO verify consistency of sig.getSA() with selectedAttributes,..
	sA = signature->sA;
	if( sA->length != (int)attributesLength) {
		LogError("Verifier Error: lengths of attributes and sA must be equal");
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	for ( i = 0; i < (int)attributesLength; i++) {
		if ( (attributes[i] == NULL && bi_equals( sA->array[i], bi_0)) ||
			(attributes[i] != NULL && !bi_equals( sA->array[i], bi_0))) {
			LogError( "Verifier Error: illegal argument content in attributes\
 and sA[%d]", i);
			result = TSS_E_BAD_PARAMETER;
			goto close;
		}
	}
	// TODO: implement verify nonce
	if ( verifyNonce(nonce_verifier, nonce_verifierLength) == 0) {
		LogError("Verifier Error: nonce invalid");
		result = TSS_E_INTERNAL_ERROR;
		goto close;
	}
	n = issuer_pk->modulus;
	c = bi_set_as_nbin( signature->challenge_length, signature->challenge);
	capital_gamma = issuer_pk->capitalGamma;
	if( base_name != NULL) { // isRandomBaseName
		zeta_2_verify = compute_zeta( base_nameLength, base_name, issuer_pk);
		if( bi_equals( signature->zeta, zeta_2_verify) == 0) {
			LogError("Verifier Error: Verification of zeta failed - Step 1");
			result = TSS_E_INTERNAL_ERROR;
			goto close;
		}
	}
	LogDebug( "step 2");
	capital_z = issuer_pk->capitalZ;
	capital_R = issuer_pk->capitalY;
	product_r = bi_new_ptr();
	if( product_r == NULL) {
		LogError("malloc of BI <%s> failed", "product_r");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_set( product_r, bi_1); // product_r = 1
	for( i=0; i<(int)attributesLength; i++) {
		if( attributes[i] != NULL) {
			 // allocation
			attribute_i = bi_set_as_nbin( DAA_PARAM_SIZE_F_I / 8, attributes[i]);
			if( attribute_i == NULL) {
				LogError("malloc of %d bytes failed", DAA_PARAM_SIZE_F_I / 8);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto close;
			}
			// tmp1 = (capital_R[i] ^ attributes[i]) mod n
			bi_mod_exp( tmp1, capital_R->array[i], attribute_i, n);
			// product_r = product_r * tmp1
			bi_mul( product_r, product_r, tmp1);
			// product_r = product_r mod n
			bi_mod( product_r, product_r, n);
			bi_free_ptr( attribute_i);
		}
	}
	exp = bi_new_ptr();
	if( exp == NULL) {
		LogError("malloc of BI <%s> failed", "product_r");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	capital_THat = bi_new_ptr();
	// tmp1 = product_r invmod n
	bi_invert_mod( tmp1, product_r, n);
	// capital_THat = capital_z * tmp1
	bi_mul( capital_THat, capital_z, tmp1);
	// capital_THat = capital_THat % n
	bi_mod( capital_THat, capital_THat, n);
	// capital_THat = (capital_THat ^ (-c)) mod n = ( 1 / (capital_That ^ c) ) % n
	bi_mod_exp( capital_THat, capital_THat, c, n);
	bi_invert_mod( capital_THat, capital_THat, n);
	// tmp1 = c << (SizeExponentCertificate - 1)
	bi_shift_left( tmp1, c, DAA_PARAM_SIZE_EXPONENT_CERTIFICATE - 1);
	// exp = signature->sE + tmp1
	bi_add( exp, signature->sE, tmp1);
	// tmp1 = (signature->capitalT ^ exp) mod n
	bi_mod_exp( tmp1, signature->capitalT, exp, n);
	//  capital_THat = ( capital_THat * tmp1 ) % n
	bi_mul( capital_THat, capital_THat, tmp1);
	bi_mod( capital_THat, capital_THat, n);
	// tmp1=( issuer_pk->capitalR0 ^ signature->sF0) %  n
	bi_mod_exp( tmp1, issuer_pk->capitalR0, signature->sF0, n);
	//  capital_THat = ( capital_THat * tmp1 ) % n
	bi_mul( capital_THat, capital_THat, tmp1);
	bi_mod( capital_THat, capital_THat, n);
	// tmp1=( issuer_pk->capitalR1 ^ signature->sF1) %  n
	bi_mod_exp( tmp1, issuer_pk->capitalR1, signature->sF1, n);
	//  capital_THat = ( capital_THat * tmp1 ) % n
	bi_mul( capital_THat, capital_THat, tmp1);
	bi_mod( capital_THat, capital_THat, n);
	// tmp1=( issuer_pk->capitalS ^ signature->sV) %  n
	bi_mod_exp( tmp1, issuer_pk->capitalS, signature->sV, n);
	//  capital_THat = ( capital_THat * tmp1 ) % n
	bi_mul( capital_THat, capital_THat, tmp1);
	bi_mod( capital_THat, capital_THat, n);

	bi_set( product_r, bi_1);	// product_r = 1
	for( i=0; i<(int)attributesLength; i++) {
		if( attributes[i] == NULL) {
			// tmp1=(capital_R->array[i] ^ sA->array[i]) %  n
			bi_mod_exp( tmp1, capital_R->array[i], sA->array[i], n);
			//  product_r = ( product_r * tmp1 ) % n
			bi_mul( product_r, product_r, tmp1);
			bi_mod( product_r, product_r, n);
		}
	}
	// capital_THat = (capital_THat * product_r) % n
	bi_mod( capital_THat, bi_mul( tmp1, capital_THat, product_r), n);
	LogDebug("Step 3 - Commitments");

	//TODO when enabling the commitment feature, verifier_transaction should be set
	#ifdef ANONYMITY_REVOCATION
	if( verifier_transaction != NULL &&
	    verifier_transaction->selected_attributes2commitLength > 0) {
		commitment_proofs = (TSS_DAA_ATTRIB_COMMIT_internal **)
			malloc(verifier_transaction->selected_attributes2commitLength *
					sizeof(TSS_DAA_ATTRIB_COMMIT_internal*));
		if (commitment_proofs == NULL) {
			LogError("malloc of %d bytes failed",
				verifier_transaction->selected_attributes2commitLength *
				sizeof(TSS_DAA_ATTRIB_COMMIT_internal*));
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		for( j=0; j<verifier_transaction->selected_attributes2commitLength; j++) {
			if( bi_cmp( commitments[j].sMu, issuer_pk->rho) >= 0 ||
					 bi_cmp_si( commitments[j].sMu, 0) < 0) {
				LogError("sMu >= rho || sMu  < 0");
				result = TSS_E_INTERNAL_ERROR;
				goto close;
			}
			beta_tilde = bi_new_ptr();
			if( beta_tilde == NULL) {
				LogError("malloc of BI <%s> failed", "beta_tilde");
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto close;
			}
			bi_set( tmp1, c);
			bi_negate( tmp1);
			// beta_tilde=(commitments[j]->beta ^ (-c)) % capitalGamma
			bi_mod_exp( beta_tilde,  commitments[j]->beta, tmp1, capital_gamma);
			// tmp1=(issuer_pk->gamma ^ commitments[j]->sMu) % capital_gamma
			bi_mod_exp( tmp1, issuer_pk->gamma, commitments[j]->sMu, capital_gamma);
			// beta_tilde=beta_tilde * tmp1
			bi_mul( beta_tilde, beta_tilde, tmp1);
			// beta_tilde=beta_tilde % capital_gamma
			bi_mod( beta_tilde, beta_tilde, capital_gamma);
			indices = (verifier_transaction->selected_attributes2commit[j])->
				indicesList;
			if( verifier_transaction->selected_attributes2commit[j]->
				indicesListLength != (UINT32)(issuer_pk->capitalY->length) ) {
				LogError("indicesList of selected_attribs[%d] (%d) \
and issuer_pk are not consistent (%d)\n",
					j,
					verifier_transaction->selected_attributes2commit[j]->
									indicesListLength,
					issuer_pk->capitalY->length);
				result = TSS_E_INTERNAL_ERROR;
				goto close;
			}
			for( i=0; i<issuer_pk->capitalY->length; i++) {
				if( indices[i]) {
					gamma_i = compute_parameterized_gamma( i, issuer_pk);
					if( gamma_i == NULL) {
						LogError("malloc of BI <%s> failed", "gamma_i");
						result = TSPERR(TSS_E_OUTOFMEMORY);
						goto close;
					}
					// tmp1=(gamma_i ^ sA[j]) % capital_gamma
					bi_mod_exp( tmp1, gamma_i, sA->array[i], capital_gamma);
					// beta_tilde=beta_tilde * tmp1
					bi_mul( beta_tilde, beta_tilde, tmp1);
					// beta_tilde=beta_tilde % capital_gamma
					bi_mod( beta_tilde, beta_tilde, capital_gamma);
				}
			}
			commitment_proofs[j] = create_TSS_DAA_ATTRIB_COMMIT( beta_tilde, NULL);
		}
	}
	#endif
	LogDebug("Step 4 - Pseudonym");
	capital_nv = bi_new_ptr();
	if( capital_nv == NULL) {
		LogError("malloc of BI <%s> failed", "capital_nv");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	capital_ntilde_v = bi_new_ptr();
	if( capital_ntilde_v == NULL) {
		LogError("malloc of BI <%s> failed", "capital_ntilde_v");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_shift_left( tmp1, signature->sF1, DAA_PARAM_SIZE_F_I);
	bi_add( exp, signature->sF0, tmp1);
	pseudonym_projected = bi_new_ptr();
	// pseudonym_projected = (signature->zeta ^ exp) % capital_gamma
	bi_mod_exp( pseudonym_projected, signature->zeta, exp, capital_gamma);
	pseudonym_enc = NULL;
	pseudonym_encryption_proof = NULL;
	//TODO when enabling the commitment feature, verifier_transaction should be set
	if( verifier_transaction == NULL ||
		verifier_transaction->is_anonymity_revocation_enabled ==0) {
		// anonymity revocation not enabled
		pseudonym_plain = (TSS_DAA_PSEUDONYM_PLAIN *)signature_ext.signedPseudonym;
		capital_nv = bi_set_as_nbin( pseudonym_plain->capitalNvLength,
							pseudonym_plain->capitalNv);
//TODO
		// capital_ntilde_v = ( capital_nv ^ ( - c) ) % capital_gamma
		//		= ( 1 / (capital_nv ^ c) % capital_gamma) % capital_gamma
		bi_mod_exp( tmp1, capital_nv, c, capital_gamma);
		bi_invert_mod( capital_ntilde_v, tmp1, capital_gamma);
		// capital_ntilde_v = ( capital_ntilde_v * pseudonym_projected ) % capital_gamma
		bi_mul(capital_ntilde_v, capital_ntilde_v, pseudonym_projected);
		bi_mod( capital_ntilde_v, capital_ntilde_v, capital_gamma);
	} else {
#ifdef ANONYMITY_REVOCATION
		// anonymity revocation enabled
		sig_pseudonym_encrypted = (TSS_DAA_PSEUDONYM_ENCRYPTED_internal *)pseudonym;
		s_tau = sig_pseudonym_encrypted->sTau;
		pseudonym_enc = sig_pseudonym_encrypted->cs_enc_result;
		// Note: It verifies if s_tau <= rho
		result_random = compute_ecryption_proof(
			pseudonym_projected,
			pseudonym_enc->c1,
			pseudonym_enc->c2,
			pseudonym_enc->c3,
			s_tau,
			verifier_transaction->anonymity_revocator_pk, issuer_pk,
			verifier_transaction->anonymity_revocation_condition,
			verifier_transaction->anonymity_revocation_condition_length,
			DAA_PARAM_get_message_digest() );
		encryption_result = result_random->result;
		delta_tilde1 = apply_challenge( encryption_result->c1,
								pseudonym_enc->c1,
								c,
								capital_gamma);
		delta_tilde2 = apply_challenge( encryption_result->c2,
								pseudonym_enc->c2,
								c,
								capital_gamma);
		delta_tilde3 = apply_challenge( encryption_result->c3,
								pseudonym_enc->c3,
								c,
								capital_gamma);
		delta_tilde4 = apply_challenge( encryption_result->c4,
								pseudonym_enc->c4,
								c,
								capital_gamma);
		pseudonym_encryption_proof = create_CS_ENCRYPTION_RESULT( delta_tilde1,
														delta_tilde2,
														delta_tilde3,
														delta_tilde4);
#endif
	}

	// TODO: Step 5 - Callback
	LogDebug("Step 5 - Callback");

	LogDebug("Step 6 - Hash");
	ch = compute_sign_challenge_host(
		&length_ch,
		DAA_PARAM_get_message_digest(),
		issuer_pk,
		nonce_verifierLength,
		nonce_verifier,
		0, // verifier_transaction->selected_attributes2commitLength,
		NULL, //verifier_transaction->selected_attributes2commit,
		0, // verifier_transaction->is_anonymity_revocation_enabled,
		signature->zeta,
		signature->capitalT,
		capital_THat,
		0, //signature_ext.attributeCommitmentsLength,
		NULL, // signature_ext.attributeCommitments,
		commitment_proofs,
		capital_nv,
		capital_ntilde_v,
		NULL, // verifier_transaction->anonymity_revocator_pk,
		pseudonym_enc,
		pseudonym_encryption_proof);
	LogDebug("calculation of c: ch[%d]%s", length_ch, dump_byte_array( length_ch, ch));
	LogDebug("calculation of c: nonce_tpm[%d]%s",
			signature->nonce_tpm_length,
			dump_byte_array( signature->nonce_tpm_length, signature->nonce_tpm));
	LogDebug("calculation of c: sign_data.payloadFlag[%d]%x", 1, sign_data.payloadFlag);
	LogDebug("calculation of c: signdata.payload[%d]%s",
			sign_data.payloadLength,
			dump_byte_array( sign_data.payloadLength, sign_data.payload));
	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, DAA_PARAM_get_message_digest(), NULL);
	EVP_DigestUpdate(&mdctx, ch, length_ch);
	EVP_DigestUpdate(&mdctx, signature->nonce_tpm, signature->nonce_tpm_length);
	len_hash = EVP_MD_size( DAA_PARAM_get_message_digest());
	hash = (BYTE *)malloc( len_hash);// allocation
	if (hash == NULL) {
		LogError("malloc of %d bytes failed", len_hash);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	EVP_DigestFinal_ex(&mdctx, hash, NULL);
	EVP_DigestInit_ex(&mdctx, DAA_PARAM_get_message_digest(), NULL);
	EVP_DigestUpdate(&mdctx, hash, EVP_MD_size( DAA_PARAM_get_message_digest()));
	EVP_DigestUpdate(&mdctx, &sign_data.payloadFlag, 1);
	EVP_DigestUpdate(&mdctx,  sign_data.payload, sign_data.payloadLength);
	len_hash = EVP_MD_size( DAA_PARAM_get_message_digest());
	free( hash);
	hash = (BYTE *)malloc( len_hash);// allocation
	if (hash == NULL) {
		LogError("malloc of %d bytes failed", len_hash);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	EVP_DigestFinal(&mdctx, hash, NULL);

	if( signature->challenge_length != len_hash ||
		memcmp( signature->challenge, hash, len_hash) != 0) {
		LogError( "Verification of c failed - Step 6.c.i");
		LogError(" - challenge[%d] : %s",
				signature->challenge_length,
				dump_byte_array( signature->challenge_length, signature->challenge));
		LogError(" -        hash[%d] : %s",
				len_hash,
				dump_byte_array( len_hash, hash));
		result = TSS_E_INTERNAL_ERROR;
		goto close;
	}
	if( verifier_transaction == NULL ||
		 !verifier_transaction->is_anonymity_revocation_enabled) {
		// Nv element <gamma> ?
		if( !is_element_gamma( capital_nv, issuer_pk)	) {
			LogError( "Verification of Nv failed - Step 4.b.i");
			result = TSS_E_INTERNAL_ERROR;
			goto close;
		}
	} else {
		// are delta1-4 element <gamma> ?
		if( !(	is_element_gamma( pseudonym_enc->c1, issuer_pk) &&
			is_element_gamma( pseudonym_enc->c2, issuer_pk) &&
			is_element_gamma( pseudonym_enc->c3, issuer_pk) &&
			is_element_gamma( pseudonym_enc->c4, issuer_pk))) {
			LogError( "Verification of delta1-4 failed - Step 4.c.i");
			result = TSS_E_INTERNAL_ERROR;
			goto close;
		}
	}
	// zeta element <gamma>
	if( !is_element_gamma( signature->zeta, issuer_pk)) {
		LogError( "Verification of zeta failed - Step 4.b/c.i");
		result = TSS_E_INTERNAL_ERROR;
		goto close;
	}
	bits = DAA_PARAM_SIZE_F_I + DAA_PARAM_SAFETY_MARGIN +
			DAA_PARAM_SIZE_MESSAGE_DIGEST + 1;
	if( bi_length( signature->sF0) > bits) {
		LogError("Verification of sF0 failed - Step 6.c.ii");
		result = TSS_E_INTERNAL_ERROR;
		goto close;
	}
	if( bi_length( signature->sF1) > bits) {
		LogError("Verification of sF1 failed - Step 6.c.ii");
		result = TSS_E_INTERNAL_ERROR;
		goto close;
	}
	// attributes extension
	for( i=0; i<sA->length; i++) {
		if( sA->array[i] != NULL && bi_length(sA->array[i]) > bits) {
			LogError( "Verification of sA[%d] failed - Step 6.c.ii", i);
			result = TSS_E_INTERNAL_ERROR;
			goto close;
		}
	}
	bits = DAA_PARAM_SIZE_INTERVAL_EXPONENT_CERTIFICATE +
			DAA_PARAM_SAFETY_MARGIN + DAA_PARAM_SIZE_MESSAGE_DIGEST + 1;
	if( bi_length( signature->sE) > bits) {
		LogError("Verification of sE failed - Step 6.c.iii");
		result = TSS_E_INTERNAL_ERROR;
		goto close;
	}
	// step 4
	// TODO: implement revocation list
	*isCorrect = TRUE;
close:
	bi_free_ptr( tmp1);
	if( ch != NULL) free( ch);
	if( hash != NULL) free( hash);
	free_TSS_DAA_PK_internal( issuer_pk);
	free_TSS_DAA_SIGNATURE_internal( signature);
	// n not allocated, refere to issuer_pk->modulus
	FREE_BI( c);
	// capital_gamma not allocated, refere to issuer_pk->capitalGamma
	FREE_BI( zeta_2_verify);
	// capital_z not allocated, refere to issuer_pk->capitalZ
	// capital_R not allocated, refere to issuer_pk->capitalY
	FREE_BI( product_r);
	FREE_BI( exp);
	FREE_BI( capital_THat);
	// beta_tilde kept on TSS_DAA_ATTRIB_COMMIT
	FREE_BI( gamma_i);
	FREE_BI( capital_nv);
	FREE_BI( capital_ntilde_v);
	FREE_BI( pseudonym_projected);
	FREE_BI( s_tau);
	// delta_tilde1 kept on CS_ENCRYPTION_RESULT
	// delta_tilde2 kept on CS_ENCRYPTION_RESULT
	// delta_tilde3 kept on CS_ENCRYPTION_RESULT
	// delta_tilde4 kept on CS_ENCRYPTION_RESULT
	return result;
}
