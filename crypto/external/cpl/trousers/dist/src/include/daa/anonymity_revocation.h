
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef ANONYMITY_REVOCATION_H_
#define ANONYMITY_REVOCATION_H_

#include "bi.h"
#include "daa_structs.h"

/**
 * Cramer Shoup public key (CSPublicKey.java)
 */
typedef struct tdCS_PUBLIC_KEY {
	bi_ptr eta;
	bi_ptr lambda1;
	bi_ptr lambda2;
	bi_ptr lambda3;
} CS_PUBLIC_KEY;

typedef struct tdCS_ENCRYPTION_RESULT {
	bi_ptr c1;
	bi_ptr c2;
	bi_ptr c3;
	bi_ptr c4;
}  CS_ENCRYPTION_RESULT;

CS_ENCRYPTION_RESULT *create_CS_ENCRYPTION_RESULT( bi_ptr c1, bi_ptr c2, bi_ptr c3, bi_ptr c4);

/*
 * Cramer-Shoup Encryption Result including randomness.
 *
 * from com.ibm.zurich.tcg.daa.anonymityrevocationCSEncryptionResultRandomness
*/
typedef struct tdCS_ENCRYPTION_RESULT_RANDOMNESS {
	bi_ptr randomness;
	CS_ENCRYPTION_RESULT *result;
} CS_ENCRYPTION_RESULT_RANDOMNESS;

/*
 * Cramer-Shoup EncryptionProof
 *  from com.ibm.zurich.tcg.daa.anonymityrevocation.CSEncryptionProof
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
	const EVP_MD *messageDigest);

#endif /*ANONYMITY_REVOCATION_H_*/
