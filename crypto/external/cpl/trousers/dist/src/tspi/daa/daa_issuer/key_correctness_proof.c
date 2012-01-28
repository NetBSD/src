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

// for little-big endian conversion
#include <arpa/inet.h>
// for message digest
#include <openssl/evp.h>

#include "bi.h"

#include "daa_parameter.h"
#include "list.h"
#include "daa_structs.h"

#include "issuer.h"

//standard bit length extension to obtain a uniformly distributed number [0,element]
static const int SAFETY_PARAM = 80;

static bi_array_ptr get_generators( const TSS_DAA_PK_internal *pk) {
	bi_array_ptr result = ALLOC_BI_ARRAY();
	int i;

	bi_new_array( result, 3 + pk->capitalY->length );
	for(i = 0; i<result->length; i++) {
		result->array[i] = pk->capitalS;
	}
	return result;
}

static bi_array_ptr get_verifiable_numbers( const TSS_DAA_PK_internal *pk) {
	bi_array_ptr result = ALLOC_BI_ARRAY();
	int i;

	bi_new_array( result, 3 + pk->capitalY->length);
	result->array[0] = pk->capitalZ;
	result->array[1] = pk->capitalR0;
	result->array[2] = pk->capitalR1;
	// CAPITAL Y ( capitalRReceiver + capitalRIssuer)
	for( i=0; i<pk->capitalY ->length; i++)
		result->array[ 3+i] = pk->capitalY->array[i];
	return result;
}

/* computes an array of random numbers in the range of [1,element] */
void compute_random_numbers( bi_array_ptr result, int quantity, const bi_ptr element) {
	int i=0;

	for( i=0; i<quantity; i++) {
		compute_random_number( result->array[i], element);
		bi_inc( result->array[i]);							// array[i]++
	}
}

int test_bit( int pos, BYTE* array, int length) {
	return (((int)array[ length - (pos / 8) - 1]) & (1 << (pos % 8))) != 0;
}

void toByteArray( BYTE *result, int length, bi_ptr bi, char *logMsg) {
	LogDebug("-> toByteArray <%d> %s",(int)bi, logMsg);
	LogDebug("lenghts <%d|%d>",length, (int)bi_nbin_size(bi));
	bi_2_byte_array( result, length, bi);
	LogDebug("<- toByteArray result=%s [<%d|%d>] ",
		dump_byte_array( length, result),
		length,
		(int)bi_nbin_size(bi));
}

/*
Compute the message digest used in the proof. (from DAA_Param, the digest
algorithm is RSA, but this is not available in openssl, the successor of RSA is SHA1
*/
TSS_RESULT
generateMessageDigest(BYTE *md_value,
			int *md_len,
			const TSS_DAA_PK_internal *pk,
			bi_array_ptr *commitments,
			const int commitments_size
) {
	EVP_MD_CTX mdctx;
	const EVP_MD *md;
	int i, j;
	int length = DAA_PARAM_SIZE_RSA_MODULUS / 8;
	BYTE *array;

	// 10000 to be sure, and this memory will be released quite quickly
	array = (BYTE *)malloc( 10000);
	if (array == NULL) {
		LogError("malloc of %d bytes failed", 10000);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	OpenSSL_add_all_digests();
	md = EVP_get_digestbyname( DAA_PARAM_MESSAGE_DIGEST_ALGORITHM);
	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, md, NULL);
#ifdef DAA_DEBUG
	fprintf(stderr, "modulus=%s\n", bi_2_hex_char( pk->modulus));
#endif
	toByteArray( array, length, pk->modulus,
			"!! [generateMessageDigest modulus] current_size=%d  length=%d\n");
	EVP_DigestUpdate(&mdctx, array , length);
	toByteArray( array, length, pk->capitalS,
			"!! [generateMessageDigest capitalS] current_size=%d  length=%d\n");
	EVP_DigestUpdate(&mdctx, array , length);
	// add capitalZ, capitalR0, capitalR1, capitalY
	LogDebug("capitalZ capitalR0 capitalY");
	toByteArray( array, length, pk->capitalZ,
			"!! [generateMessageDigest capitalZ] current_size=%d  length=%d\n");
	EVP_DigestUpdate(&mdctx, array , length);
	toByteArray( array, length, pk->capitalR0,
			"!! [generateMessageDigest capitalR0] current_size=%d  length=%d\n");
	EVP_DigestUpdate(&mdctx, array , length);
	toByteArray( array, length, pk->capitalR1,
			"!! [generateMessageDigest capitalR1] current_size=%d  length=%d\n");
	EVP_DigestUpdate(&mdctx, array , length);
	// CAPITAL Y ( capitalRReceiver )
	LogDebug("capitalRReceiver");
	for( i=0; i<pk->capitalRReceiver->length; i++) {
		toByteArray( array, length, pk->capitalRReceiver->array[i],
			"!![generateMessageDigest capitalRReceiver] current_size=%d  length=%d\n");
		EVP_DigestUpdate(&mdctx, array , length);
	}
	LogDebug("capitalRIssuer");
	// CAPITAL Y ( capitalRIssuer)
	for( i=0; i<pk->capitalRIssuer->length; i++) {
		toByteArray( array, length, pk->capitalRIssuer->array[i],
			"!![generateMessageDigest capitalRReceiver] current_size=%d  length=%d\n");
		EVP_DigestUpdate(&mdctx, array , length);
	}
	LogDebug("commitments");
	for( i=0; i<commitments_size; i++) {
		for( j=0; j<commitments[i]->length; j++) {
			toByteArray( array, length, commitments[i]->array[j],
			"!! [generateMessageDigest commitments] current_size=%d  length=%d\n");
			EVP_DigestUpdate(&mdctx, array , length);
		}
	}
	EVP_DigestFinal_ex(&mdctx, md_value, md_len);
	EVP_MD_CTX_cleanup(&mdctx);
	free( array);
	return TSS_SUCCESS;
}

int is_range_correct( bi_ptr b, bi_ptr range) {
	return bi_cmp( b, range) < 0 && bi_cmp( b, bi_0) >= 0;
}

/*
Verifies if the parameters Z,R0,R1,RReceiver and RIssuer of the public key
were correctly computed.
pk: the public key, which one wants to verfy.
*/
TSS_RESULT
is_pk_correct( TSS_DAA_PK_internal *public_key,
		TSS_DAA_PK_PROOF_internal *proof,
		int *isCorrect
) {
	int bit_size_message_digest = DAA_PARAM_SIZE_MESSAGE_DIGEST;
	bi_ptr n = public_key->modulus;
	int num_of_variables;
	int i,j;
	TSS_RESULT result = TSS_SUCCESS;
	BYTE verifiable_challenge[EVP_MAX_MD_SIZE];
	int length_challenge;
	bi_array_ptr verifiable_numbers;
	bi_array_ptr *verification_commitments = NULL;
	bi_array_ptr generators = NULL;
	bi_t tmp;
	bi_t tmp1;
#ifdef DAA_DEBUG
	FILE *f;
	bi_array_ptr *commitments;
#endif

	bi_new( tmp);
	bi_new( tmp1);
	*isCorrect = 0;
#ifdef DAA_DEBUG
	f=fopen("/tmp/commits", "r");
	commitments = (bi_array_ptr *)malloc( sizeof(bi_array_ptr) * num_of_variables);
	if (commitments == NULL) {
		LogError("malloc of %d bytes failed", sizeof(bi_array_ptr) * num_of_variables);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	for( i=0; i<num_of_variables; i++) {
		commitments[i] = ALLOC_BI_ARRAY();
		BI_LOAD_ARRAY( commitments[i], f);
	}
	fclose(f);
	DUMP_BI( n);
#endif
	LogDebug("STEP 1 ( Tspi_DAA_IssuerKeyVerification spec.)");
	if( !bi_is_probable_prime( public_key->capitalGamma)) {
		LogError( "pk->capitalGamma not prime\ncapitalGamma=\n%s",
				bi_2_hex_char( public_key->capitalGamma));
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	if( !bi_is_probable_prime( public_key->rho)) {
		LogError( "pk->rho not prime\nrho=\n%s",
				bi_2_hex_char( public_key->rho));
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	// (capitalGamma - 1) %  rho should be equal to 0
	if( !bi_equals( bi_mod( tmp1, bi_sub( tmp1, public_key->capitalGamma, bi_1),
				public_key->rho),
			bi_0)) {
		LogError( "(capitalGamma - 1) %%  rho != 0\nActual value:\n%s",
				bi_2_hex_char( tmp1));
		result = TSS_E_BAD_PARAMETER;
	}
	// (gamma ^ rho) % capitalGamma should be equals to 1
	if ( !bi_equals( bi_mod_exp( tmp1, public_key->gamma,
						public_key->rho,
						public_key->capitalGamma),
				bi_1) ) {
		LogError( "(gamma ^ rho) %% capitalGamma != 1\nActual value:\n%s",
				bi_2_hex_char( tmp1));
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	// (gamma ^ rho) % capitalGamma should be equal to 1
	if ( !bi_equals( bi_mod_exp( tmp1,
						public_key->gamma,
						public_key->rho,
						public_key->capitalGamma),
				bi_1) ) {
		LogError( "(gamma ^ rho) %% capitalGamma != 1\nActual value:\n%s",
				bi_2_hex_char( tmp1));
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	LogDebug("STEP 2 check whether all public key parameters have the required length");
	if( bi_nbin_size( n) != DAA_PARAM_SIZE_RSA_MODULUS / 8) {
		LogError( "size( n)[%ld] != DAA_PARAM_SIZE_RSA_MODULUS[%d]",
			bi_nbin_size( n),
			DAA_PARAM_SIZE_RSA_MODULUS / 8);
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	if( bi_cmp( n, bi_shift_left( tmp1, bi_1, DAA_PARAM_SIZE_RSA_MODULUS))
		>= 0) {
		LogError( "n[%ld] != DAA_PARAM_SIZE_RSA_MODULUS[%d]",
			bi_nbin_size( n),
			DAA_PARAM_SIZE_RSA_MODULUS);
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	if( bi_cmp( n, bi_shift_left( tmp1, bi_1, DAA_PARAM_SIZE_RSA_MODULUS - 1 ))
		<= 0) {
		LogError( "n[%ld] != DAA_PARAM_SIZE_RSA_MODULUS[%d]",
			bi_nbin_size( n),
			DAA_PARAM_SIZE_RSA_MODULUS);
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	// rho
	if( bi_nbin_size( public_key->rho) * 8 != DAA_PARAM_SIZE_RHO) {
		LogError( "size( rho)[%ld] != DAA_PARAM_SIZE_RHO[%d]",
			bi_nbin_size( public_key->rho) * 8,
			DAA_PARAM_SIZE_RHO);
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	// Gamma
	if( bi_nbin_size( public_key->capitalGamma) * 8 != DAA_PARAM_SIZE_MODULUS_GAMMA) {
		LogError( "size( rho)[%ld] != DAA_PARAM_SIZE_MODULUS_GAMMA[%d]",
			bi_nbin_size( public_key->capitalGamma) * 8,
			DAA_PARAM_SIZE_MODULUS_GAMMA);
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	if( is_range_correct( public_key->capitalS, n) == 0) {
		LogError( "range not correct( pk->capitalS)\ncapitalS=\n%s\nn=\n%s",
			bi_2_hex_char( public_key->capitalS),
			bi_2_hex_char( n));
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	if( is_range_correct( public_key->capitalZ, n) == 0) {
		LogError( "range not correct( pk->capitalZ)\ncapitalZ=\n%s\nn=\n%s",
			bi_2_hex_char( public_key->capitalZ),
			bi_2_hex_char( n));
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	if( is_range_correct( public_key->capitalR0, n) == 0) {
		LogError( "range not correct( pk->capitalR0)\ncapitalR0=\n%s\nn=\n%s",
			bi_2_hex_char( public_key->capitalR0),
			bi_2_hex_char( n));
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	if( is_range_correct( public_key->capitalR1, n) == 0) {
		LogError( "range not correct( pk->capitalR1)\ncapitalR1=\n%s\nn=\n%s",
			bi_2_hex_char( public_key->capitalR1),
			bi_2_hex_char( n));
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	for( i=0; i<public_key->capitalY->length; i++) {
		if( is_range_correct( public_key->capitalY->array[i], n) == 0) {
			LogError( "range not correct(pk->capitalY[%d])\ncapitalY[%d]=\n%s\nn=\n%s",
				i, i,
				bi_2_hex_char( public_key->capitalY->array[i]),
				bi_2_hex_char( n));
			result = TSS_E_BAD_PARAMETER;
			goto close;
		}
	}
	LogDebug("STEP 3 - compute verification commitments");
	// only the array is allocated, but all refs are  pointing to public_key numbers
	generators = get_generators( public_key);
	verifiable_numbers = get_verifiable_numbers( public_key);
	num_of_variables = verifiable_numbers->length;
	verification_commitments = (bi_array_ptr *)malloc( sizeof(bi_array_ptr)*num_of_variables);
	if (verification_commitments == NULL) {
		LogError("malloc of %d bytes failed", sizeof(bi_array_ptr)*num_of_variables);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	for( i = 0; i<num_of_variables; i++) {
		verification_commitments[i] = ALLOC_BI_ARRAY();
		bi_new_array( verification_commitments[i], bit_size_message_digest);
		for( j = 0; j<bit_size_message_digest; j++) {
#ifdef DAA_DEBUG
			printf( "[# i=%d j=%d test_bit:%d]",
				i, j, test_bit( j, proof->challenge, proof->length_challenge));
#endif
			bi_mod_exp( verification_commitments[i]->array[j],
				generators->array[i],
				proof->response[i]->array[j], n);
			if( test_bit( j, proof->challenge, proof->length_challenge)) {
				bi_mul( verification_commitments[i]->array[j],
					verification_commitments[i]->array[j],
					verifiable_numbers->array[i]);
#ifdef DAA_DEBUG
				DUMP_BI( verification_commitments[i]->array[j]);
#endif
				bi_mod( verification_commitments[i]->array[j],
					verification_commitments[i]->array[j],
					n);
			}
#ifdef DAA_DEBUG
			if( commitments != NULL &&
				bi_equals( verification_commitments[i]->array[j],
						commitments[i]->array[j]) ==0) {
				LogError( "!! ERROR i=%d j=%d\n", i, j);
				DUMP_BI( commitments[i]->array[j]);
				DUMP_BI( verification_commitments[i]->array[j]);
				DUMP_BI( generators->array[i]);
				DUMP_BI( proof->response[i]->array[j]);
				DUMP_BI( verifiable_numbers->array[i]);
			}
			printf( "o"); fflush( stdout);
#endif
		}
	}
	// STEP 3 - d
	generateMessageDigest( verifiable_challenge,
						&length_challenge,
						public_key,
						verification_commitments,
						num_of_variables);
	LogDebug("verifiable challenge=%s",
			dump_byte_array( length_challenge, verifiable_challenge));
	LogDebug("           challenge=%s",
			dump_byte_array( proof->length_challenge, proof->challenge));
	if( length_challenge != proof->length_challenge) {
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	for( i=0; i<length_challenge; i++) {
		if( verifiable_challenge[i] != proof->challenge[i]) {
			result = TSS_E_BAD_PARAMETER;
			goto close;
		}
	}
	*isCorrect = ( memcmp( verifiable_challenge, proof->challenge, length_challenge) == 0);
close:
	if( verification_commitments != NULL) {
		for( i = 0; i<num_of_variables; i++) {
			bi_free_array( verification_commitments[i]);
		}
		free( verification_commitments);
	}
	if( generators != NULL) free( generators);
	bi_free( tmp1);
	bi_free( tmp);
	return result;
}


TSS_DAA_PK_PROOF_internal *generate_proof(const bi_ptr product_PQ_prime,
					const TSS_DAA_PK_internal *public_key,
					const bi_ptr xz,
					const bi_ptr x0,
					const bi_ptr x1,
					bi_array_ptr x
) {
	int i, j;
	bi_array_ptr generators = get_generators( public_key);
	bi_ptr n = public_key->modulus;
	int num_of_variables;
	int bit_size_message_digest = DAA_PARAM_SIZE_MESSAGE_DIGEST;
	bi_array_ptr *xTildes = NULL;
	BYTE *challenge_param;

	bi_array_ptr exponents = ALLOC_BI_ARRAY();
	bi_new_array2( exponents, 3 + x->length);
	exponents->array[0] = xz; 	exponents->array[1] = x0; exponents->array[2] = x1;
	bi_copy_array( x, 0, exponents, 3, x->length);
	num_of_variables = exponents->length;
	LogDebug("Step a - choose random numbers");
	LogDebug("\nchoose random numbers\n");
	xTildes = (bi_array_ptr *)malloc( sizeof(bi_array_ptr) * num_of_variables);
	if (xTildes == NULL) {
		LogError("malloc of %d bytes failed", sizeof(bi_array_ptr) * num_of_variables);
		return NULL;
	}
	for( i=0; i<num_of_variables; i++) {
#ifdef DAA_DEBUG
		printf("*");
		fflush(stdout);
#endif
		xTildes[i] = ALLOC_BI_ARRAY();
		bi_new_array( xTildes[i], bit_size_message_digest);
		compute_random_numbers( xTildes[i], bit_size_message_digest, product_PQ_prime);
	}
	// Compute commitments
	LogDebug("\ncompute commitments");
	bi_array_ptr *commitments =
		(bi_array_ptr *)malloc( sizeof(bi_array_ptr) * num_of_variables);
	if (commitments == NULL) {
		LogError("malloc of %d bytes failed", sizeof(bi_array_ptr) * num_of_variables);
		return NULL;
	}
	for( i=0; i<num_of_variables; i++) {
		commitments[i] = ALLOC_BI_ARRAY();
		bi_new_array( commitments[i], bit_size_message_digest);
		for( j=0; j<bit_size_message_digest; j++) {
#ifdef DAA_DEBUG
			printf("#");
			fflush(stdout);
#endif
			bi_mod_exp( commitments[i]->array[j],
				generators->array[i],
				xTildes[i]->array[j],
				n);
		}
	}
#ifdef DAA_DEBUG
	FILE *f=fopen("/tmp/commits", "w");
	for( i=0; i<num_of_variables; i++) {
		BI_SAVE_ARRAY( commitments[i], f);
	}
	fclose(f);
#endif
	LogDebug("Step b: compute challenge (message digest)");
	BYTE challenge[EVP_MAX_MD_SIZE];
	int length_challenge;
	generateMessageDigest( challenge,
				&length_challenge,
				public_key,
				commitments,
				num_of_variables);
	//     STEP c - compute response
	LogDebug("Step c: compute response\n");
	bi_array_ptr *response = (bi_array_ptr *)malloc( sizeof(bi_array_ptr) * num_of_variables);
	if (response == NULL) {
		LogError("malloc of %d bytes failed", sizeof(bi_array_ptr) * num_of_variables);
		return NULL;
	}
	for( i=0; i<num_of_variables; i++) {
		response[i] = ALLOC_BI_ARRAY();
		bi_new_array( response[i], bit_size_message_digest);
		for( j=0; j<bit_size_message_digest; j++) {
			if( test_bit( j, challenge, length_challenge)) {
				bi_sub( response[i]->array[j],
					xTildes[i]->array[j],
					exponents->array[i]);
			} else {
				bi_set( response[i]->array[j],
					xTildes[i]->array[j]);
			}
			bi_mod( response[i]->array[j], response[i]->array[j],  product_PQ_prime);
#ifdef DAA_DEBUG
			printf("#");
			fflush(stdout);
#endif
		}
	}
	challenge_param = (BYTE *)malloc( length_challenge);
	if (challenge_param == NULL) {
		LogError("malloc of %d bytes failed", length_challenge);
		return NULL;
	}
	memcpy( challenge_param, challenge, length_challenge);

	return create_DAA_PK_PROOF( challenge_param,
					length_challenge,
					response,
					num_of_variables);
}
