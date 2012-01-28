
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#include "daa_parameter.h"
#include "daa_structs.h"
#include "anonymity_revocation.h"
#include "verifier.h"
#include "tsplog.h"

CS_ENCRYPTION_RESULT *create_CS_ENCRYPTION_RESULT(
	bi_ptr c1,
	bi_ptr c2,
	bi_ptr c3,
	bi_ptr c4
) {
	CS_ENCRYPTION_RESULT *result =
		(CS_ENCRYPTION_RESULT *)malloc( sizeof(CS_ENCRYPTION_RESULT));

	if (result == NULL) {
		LogError("malloc of %d bytes failed", sizeof(CS_ENCRYPTION_RESULT));
		return NULL;
	}
	result->c1 = c1;
	result->c2 = c2;
	result->c3 = c3;
	result->c4 = c4;
	return result;
}

CS_ENCRYPTION_RESULT_RANDOMNESS *create_CS_ENCRYPTION_RESULT_RANDOMNESS(
	CS_ENCRYPTION_RESULT *cs_encryption_result,
	bi_ptr randomness
) {
	CS_ENCRYPTION_RESULT_RANDOMNESS *result =
		(CS_ENCRYPTION_RESULT_RANDOMNESS *)
			malloc(sizeof(CS_ENCRYPTION_RESULT_RANDOMNESS));

	if (result == NULL) {
		LogError("malloc of %d bytes failed",
			sizeof(CS_ENCRYPTION_RESULT_RANDOMNESS));
		return NULL;
	}
	result->randomness = randomness;
	result->result = cs_encryption_result;
	return result;
}

bi_ptr compute_u( const EVP_MD *digest,
		const bi_ptr c1,
		const bi_ptr c2,
		const bi_ptr c3,
		const BYTE *condition,
		const int conditionLength
) {
	BYTE *buffer, *bytes;
	int c1_size = bi_nbin_size( c1);
	int c2_size = bi_nbin_size( c2);
	int c3_size = bi_nbin_size( c3);
	int index = 0;
	int length;
	bi_ptr value;
	int bufferLength = c1_size + c2_size + c3_size + conditionLength;

	buffer = (BYTE *)malloc( bufferLength);
	if (buffer == NULL) {
		LogError("malloc of %d bytes failed", bufferLength);
		return NULL;
	}
	bi_2_byte_array( &buffer[index], c1_size, c1);
	index += c1_size;
	bi_2_byte_array( &buffer[index], c2_size, c2);
	index += c2_size;
	bi_2_byte_array( &buffer[index], c3_size, c3);
	index += c3_size;
	memcpy( &buffer[index], condition, conditionLength);
	index += conditionLength;
	length = DAA_PARAM_LENGTH_MFG1_ANONYMITY_REVOCATION / 8; // 25 /8
	bytes = compute_bytes( bufferLength, buffer, length, digest);
	if( bytes == NULL) return NULL;
	value = bi_set_as_nbin( length, bytes);
	free( bytes);
	free( buffer);
	return value;
}

CS_ENCRYPTION_RESULT_RANDOMNESS* internal_compute_encryption_proof(
	const bi_ptr msg,
	const bi_ptr delta1,
	const bi_ptr delta2,
	const bi_ptr delta3,
	const bi_ptr randomness,
	const CS_PUBLIC_KEY *key,
	const struct tdTSS_DAA_PK_internal *daa_key,
	const BYTE *condition,
	const int conditionLength,
	const EVP_MD *digest
) {
	bi_ptr modulus = daa_key->modulus;
	bi_ptr gamma = daa_key->gamma;
	bi_ptr c1 = bi_new_ptr( );
	bi_ptr c2 = bi_new_ptr( );
	bi_ptr c3 = bi_new_ptr( );
	bi_ptr c4;
	bi_ptr exp;
	bi_t bi_tmp;
	bi_t bi_tmp1;

	bi_new( bi_tmp);
	bi_new( bi_tmp1);
	if( bi_cmp( msg, modulus) >= 0) {
		LogError("IllegalArgument: msg to big for key size");
		bi_free_ptr( c1);
		bi_free_ptr( c2);
		bi_free_ptr( c3);
		bi_free( bi_tmp);
		bi_free( bi_tmp1);
		return NULL;
	}
	bi_mod_exp( c1, gamma, randomness, modulus);
	bi_mod_exp( c2, key->eta, randomness, modulus);
	// c3=msg * (key->lambda3 ^ randomness) % mopdulus)
	bi_mul( c3, msg, bi_mod_exp( bi_tmp, key->lambda3, randomness, modulus));
	bi_mod( c3, c3, modulus);	// c3 = c3 % modulus
	if( delta1 != NULL) {
		if( !( delta2!=NULL && delta3!=NULL)) {
			LogError("Illegal Arguments: delta2==NULL or delta3==NULL");
			bi_free_ptr( c1);
			bi_free_ptr( c2);
			bi_free_ptr( c3);
			bi_free( bi_tmp);
			bi_free( bi_tmp1);
			return NULL;
		}
		exp = compute_u( digest, delta1, delta2, delta3, condition, conditionLength);
	} else {
		if( !( delta2==NULL && delta3==NULL)) {
			LogError("Illegal Arguments: delta2!=NULL or delta3!=NULL");
			bi_free_ptr( c1);
			bi_free_ptr( c2);
			bi_free_ptr( c3);
			bi_free( bi_tmp);
			bi_free( bi_tmp1);
			return NULL;
		}
		exp = compute_u( digest, c1, c2, c3, condition, conditionLength);
	}
	// exp = exp * randomness
	bi_mul( exp, exp, randomness);
	// exp = exp % daa_key->rho
	bi_mod( exp, exp, daa_key->rho);
	// bi_tmp = (key->lambda1 ^ randomness) % modulus
	bi_mod_exp( bi_tmp, key->lambda1, randomness, modulus);
	// bi_tmp1 = (key->lambda2 ^ exp) % modulus
	bi_mod_exp( bi_tmp1, key->lambda2, exp, modulus);
	c4 = bi_new_ptr();
	// c4 = bi_tmp * bi_tmp1
	bi_mul( c4, bi_tmp, bi_tmp1);
	// c4 = c4 % modulus
	bi_mod( c4, c4, modulus);
	bi_free_ptr( exp);
	bi_free( bi_tmp1);
	bi_free( bi_tmp);
	return create_CS_ENCRYPTION_RESULT_RANDOMNESS(
		create_CS_ENCRYPTION_RESULT( c1, c2, c3, c4),
		randomness);
}

/*
Cramer-Shoup EncryptionProof
from com.ibm.zurich.tcg.daa.anonymityrevocation.CSEncryptionProof
 */
CS_ENCRYPTION_RESULT_RANDOMNESS *compute_ecryption_proof(
	const bi_ptr msg,
	const bi_ptr delta1,
	const bi_ptr delta2,
	const bi_ptr delta3,
	const bi_ptr randomness,
	const CS_PUBLIC_KEY *key,
	const struct tdTSS_DAA_PK_internal *daa_key,
	const BYTE *condition,
	const int conditionLength,
	const EVP_MD *digest
) {
	if( delta1 == NULL || delta2 == NULL || delta3 == NULL) {
		LogError("Illegal Argument: deltas (delta1:%ld delta2:%ld delta3:%ld)",
				(long)delta1, (long)delta2, (long)delta3);
		return NULL;
	}
	if( bi_cmp( randomness, daa_key->rho) >=0 || bi_cmp_si( randomness, 0) < 0) {
		LogError("randomness >= rho || randomness < 0 \n\trandomness:%s\n\trho:%s\n",
				bi_2_dec_char( randomness), bi_2_dec_char( daa_key->rho));
		return NULL;
	}
	return internal_compute_encryption_proof( msg,
						delta1,
						delta2,
						delta3,
						randomness,
						key,
						daa_key,
						condition,
						conditionLength,
						digest);
}
