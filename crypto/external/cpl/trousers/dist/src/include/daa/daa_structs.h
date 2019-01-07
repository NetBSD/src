
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef DAA_STRUCT_H_
#define DAA_STRUCT_H_

#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "tss/tcs.h"
#include "bi.h"
#include "arpa/inet.h"

// for message digest
#include <openssl/evp.h>

#define init_tss_version(b) \
do {\
	(b)->versionInfo.bMajor = DAA_PARAM_TSS_VERSION[0];\
	(b)->versionInfo.bMinor = DAA_PARAM_TSS_VERSION[1];\
	(b)->versionInfo.bRevMajor = DAA_PARAM_TSS_VERSION[2];\
	(b)->versionInfo.bRevMinor = DAA_PARAM_TSS_VERSION[3];\
} while(0);

BYTE *convert_alloc( TCS_CONTEXT_HANDLE tcsContext,
			UINT32 length,
			BYTE *source);

BYTE *copy_alloc(  TCS_CONTEXT_HANDLE tcsContext,
			UINT32 length,
			BYTE *source);

void store_bi( UINT32 *length,
		BYTE **buffer,
		const bi_ptr i,
		void * (*daa_alloc)(size_t size, TSS_HOBJECT object),
		TSS_HOBJECT object);

/* length is in network format: big indian */
void dump_field( int length, BYTE *buffer);

/********************************************************************************************
	TSS_DAA_ATTRIB_COMMIT
 ********************************************************************************************/

typedef struct tdTSS_DAA_ATTRIB_COMMIT_internal {
	bi_ptr beta;
	bi_ptr sMu;
} TSS_DAA_ATTRIB_COMMIT_internal;

TSS_DAA_ATTRIB_COMMIT_internal *create_TSS_DAA_ATTRIB_COMMIT( bi_ptr beta, bi_ptr sMu);

/********************************************************************************************
 *   TSS_DAA_SELECTED_ATTRIB
 * this struct is used internally and externally, only a call to internal_2_DAA_SELECTED_ATTRIB
 * DAA_SELECTED_ATTRIB_2_internal will change the struct to be internal or external
 ********************************************************************************************/

void i_2_e_TSS_DAA_SELECTED_ATTRIB( TSS_DAA_SELECTED_ATTRIB *selected_attrib);

void e_2_i_TSS_DAA_SELECTED_ATTRIB( TSS_DAA_SELECTED_ATTRIB *selected_attrib);

/* work ONLY with internal format */
BYTE *to_bytes_TSS_DAA_SELECTED_ATTRIB_internal( int *length, TSS_DAA_SELECTED_ATTRIB *selected_attrib);

/*
create a TSS_DAA_SELECTED_ATTRIB of length <length> with given selected attributes.
example of selections of the second and third attributes upon 5:
create_TSS_DAA_SELECTED_ATTRIB( &selected_attrib, 5, 0, 1, 1, 0, 0);
*/
void create_TSS_DAA_SELECTED_ATTRIB( TSS_DAA_SELECTED_ATTRIB *attrib, int length, ...);

/********************************************************************************************
 *   DAA PRIVATE KEY
 ********************************************************************************************/

/**
 * DAA private key. Contains p', q' and the product of it, where n = p*q, p =
 * 2*p'+1 and q = 2*q'+1. n is part of the public key.
 * (from com.ibm.zurich.tcg.daa.issuer.DAAPrivateKey.java)
 */
typedef struct {
	bi_ptr p_prime;
	bi_ptr q_prime;
	bi_ptr productPQprime;
} DAA_PRIVATE_KEY_internal;

/**
 * allocate: 	ret->p_prime
 * 					ret->q_prime
 * 				  	ret->productPQprime
 */
DAA_PRIVATE_KEY_internal *create_TSS_DAA_PRIVATE_KEY(
	bi_ptr pPrime,
	bi_ptr qPrime
);
#if 0
int save_DAA_PRIVATE_KEY(
	FILE *file,
	const DAA_PRIVATE_KEY_internal *private_key
);

DAA_PRIVATE_KEY_internal *load_DAA_PRIVATE_KEY(
	FILE *file
);
TSS_DAA_PRIVATE_KEY* i_2_e_TSS_DAA_PRIVATE_KEY(
	DAA_PRIVATE_KEY_internal *private_key_internal,
	void * (*daa_alloc)(size_t size, TSS_HOBJECT object),
	TSS_HOBJECT object
);

DAA_PRIVATE_KEY_internal *e_2_i_TSS_DAA_PRIVATE_KEY(
	TSS_DAA_PRIVATE_KEY *private_key
);

#endif
/********************************************************************************************
 *   TSS_DAA_PK
 ********************************************************************************************/

typedef struct tdTSS_DAA_PK_internal {
	bi_ptr modulus;
	bi_ptr capitalS;
	bi_ptr capitalZ;
	bi_ptr capitalR0;
	bi_ptr capitalR1;
	bi_ptr gamma;
	bi_ptr capitalGamma;
	bi_ptr rho;
	bi_array_ptr capitalRReceiver;
	bi_array_ptr capitalRIssuer;
    	bi_array_ptr capitalY;
	int issuerBaseNameLength;
	BYTE *issuerBaseName;
 	// capitalSprime calculated at each init of this structure as :
 	//    (capitalS ^ ( 1 << DAA_PARAM_SIZE_SPLIT_EXPONENT)) % modulus
	bi_ptr capitalSprime;
} TSS_DAA_PK_internal;

TSS_DAA_PK_internal *create_DAA_PK(
	const bi_ptr modulus,
	const bi_ptr capitalS,
	const bi_ptr capitalZ,
	const bi_ptr capitalR0,
	const bi_ptr capitalR1,
	const bi_ptr gamma,
	const bi_ptr capitalGamma,
	const bi_ptr rho,
	const bi_array_ptr capitalRReceiver,
	const bi_array_ptr capitalRIssuer,
	int  issuerBaseNameLength,
	BYTE * const issuerBaseName);

/*
 * create anf feel a TSS_DAA_PK structures
 */
TSS_DAA_PK_internal *e_2_i_TSS_DAA_PK(
	TSS_DAA_PK *pk
);

TSS_DAA_PK	*i_2_e_TSS_DAA_PK(
	TSS_DAA_PK_internal *pk_internal,
	void * (*daa_alloc)(size_t size, TSS_HOBJECT object),
	TSS_HOBJECT param_alloc
);
#if 0

/* moved to daa_debug.h */
int save_DAA_PK_internal(
	FILE *file,
	const TSS_DAA_PK_internal *pk_internal
);

TSS_DAA_PK_internal *load_DAA_PK_internal(
	FILE *file
);

#endif

void dump_DAA_PK_internal(
	char *name,
	TSS_DAA_PK_internal *pk_internal
);

TPM_DAA_ISSUER *convert2issuer_settings(
	TSS_DAA_PK_internal *pk_internal
);

void free_TSS_DAA_PK_internal(
	TSS_DAA_PK_internal *pk_internal
);

void free_TSS_DAA_PK( TSS_DAA_PK *pk);

BYTE *issuer_2_byte_array(
	TPM_DAA_ISSUER *tpm_daa_issuer,
	int *length
);

/********************************************************************************************
 *   TSS_DAA_PK_PROOF
 ********************************************************************************************/

typedef struct tdTSS_DAA_PK_PROOF_internal {
	BYTE *challenge;
	int length_challenge;
	bi_array_ptr *response;
	int length_response;
} TSS_DAA_PK_PROOF_internal;

TSS_DAA_PK_PROOF_internal *create_DAA_PK_PROOF(
	BYTE* const challenge,
	const int length_challenge,
	bi_array_ptr *response,
	int length_reponse);

/*
 * create anf feel a TSS_DAA_PK structures
 */
TSS_DAA_PK *TSS_convert_DAA_PK_PROOF(
	TSS_DAA_PK_PROOF_internal *proof
);
#if 0
int save_DAA_PK_PROOF_internal(
	FILE *file,
	TSS_DAA_PK_PROOF_internal *pk_internal
);

TSS_DAA_PK_PROOF_internal *load_DAA_PK_PROOF_internal(
	FILE *file
);
#endif
TSS_DAA_PK_PROOF_internal *e_2_i_TSS_DAA_PK_PROOF(
	TSS_DAA_PK_PROOF *pk_proof
);

TSS_DAA_PK_PROOF *i_2_e_TSS_DAA_PK_PROOF(
	TSS_DAA_PK_PROOF_internal*pk_internal_proof,
	void * (*daa_alloc)(size_t size, TSS_HOBJECT object),
	TSS_HOBJECT param_alloc
);

/*
 * Encode the DAA_PK like java.security.Key#getEncoded
 */
BYTE *encoded_DAA_PK_internal(
	int *result_length,
	const TSS_DAA_PK_internal *pk
);

/********************************************************************************************
 *   KEY PAIR WITH PROOF
 ********************************************************************************************/

typedef struct tdKEY_PAIR_WITH_PROOF_internal {
	TSS_DAA_PK_internal *pk;
	DAA_PRIVATE_KEY_internal *private_key;
	TSS_DAA_PK_PROOF_internal *proof;
} KEY_PAIR_WITH_PROOF_internal;

#if 0

/* moved to daa_debug.h */

int save_KEY_PAIR_WITH_PROOF(
	FILE *file,
	KEY_PAIR_WITH_PROOF_internal *key_pair_with_proof
);

KEY_PAIR_WITH_PROOF_internal *load_KEY_PAIR_WITH_PROOF(
	FILE *file
);

#endif

TSS_DAA_KEY_PAIR *get_TSS_DAA_KEY_PAIR(
	KEY_PAIR_WITH_PROOF_internal *key_pair_with_proof,
	void * (*daa_alloc)(size_t size, TSS_HOBJECT object),
	TSS_HOBJECT param_alloc
);


/********************************************************************************************
 *   TSS_DAA_PSEUDONYM_PLAIN
 ********************************************************************************************/

typedef struct {
	bi_ptr nV;
} TSS_DAA_PSEUDONYM_PLAIN_internal;

TSS_DAA_PSEUDONYM_PLAIN_internal *create_TSS_DAA_PSEUDONYM_PLAIN(
	bi_ptr nV
);

/********************************************************************************************
 *   TSS_DAA_PSEUDONYM_ENCRYPTED
 ********************************************************************************************/

typedef struct {
	bi_ptr sTau;
	struct tdCS_ENCRYPTION_RESULT *cs_enc_result;
} TSS_DAA_PSEUDONYM_ENCRYPTED_internal;


/********************************************************************************************
 *   TSS_DAA_SIGNATURE
 ********************************************************************************************/

typedef struct {
	bi_ptr zeta;
	bi_ptr capitalT;
	int challenge_length;
	BYTE *challenge;
	int nonce_tpm_length;
	BYTE *nonce_tpm;
	bi_ptr sV;
	bi_ptr sF0;
	bi_ptr sF1;
	bi_ptr sE;
	int sA_length;
	bi_array_ptr sA;
} TSS_DAA_SIGNATURE_internal;

TSS_DAA_SIGNATURE_internal *e_2_i_TSS_DAA_SIGNATURE(
	TSS_DAA_SIGNATURE*signature
);

void free_TSS_DAA_SIGNATURE_internal(
	TSS_DAA_SIGNATURE_internal *signature
);

/********************************************************************************************
 *   TSS_DAA_JOIN_ISSUER_SESSION
 ********************************************************************************************/

typedef struct td_TSS_DAA_JOIN_ISSUER_SESSION_internal {
	TPM_DAA_ISSUER *issuerAuthKey;
	TSS_DAA_PK_PROOF_internal *issuerKeyPair;
	TSS_DAA_IDENTITY_PROOF *identityProof;
	bi_ptr capitalUprime;
	int daaCounter;
	int nonceIssuerLength;
	BYTE *nonceIssuer;
	int nonceEncryptedLength;
	BYTE *nonceEncrypted;
} TSS_DAA_JOIN_ISSUER_SESSION_internal;


/********************************************************************************************
	TSS_DAA_CRED_ISSUER
********************************************************************************************/
#if 0
TSS_DAA_CRED_ISSUER *load_TSS_DAA_CRED_ISSUER( FILE *file);

int save_TSS_DAA_CRED_ISSUER( FILE *file, TSS_DAA_CRED_ISSUER *credential);

#endif
/********************************************************************************************
	TSS_DAA_CREDENTIAL
********************************************************************************************/
#if 0
TSS_DAA_CREDENTIAL *load_TSS_DAA_CREDENTIAL( FILE *file);

int save_TSS_DAA_CREDENTIAL(
	FILE *file,
	TSS_DAA_CREDENTIAL *credential
);

#endif

/********************************************************************************************
	TPM_DAA_ISSUER
********************************************************************************************/

void free_TPM_DAA_ISSUER( TPM_DAA_ISSUER *tpm_daa_issuer);

#endif /*DAA_STRUCT_H_*/
