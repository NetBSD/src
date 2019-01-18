
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef VERIFIER_H_
#define VERIFIER_H_

#include "bi.h"
#include "daa_structs.h"
#include "anonymity_revocation.h"
#include "daa_parameter.h"
#include "tsplog.h"

/*
 * Transaction of a DAA Verifier to verify a signature (VerifierTransaction.java)
 */
typedef struct {
	BYTE *baseName;
	int baseName_length;
	EVP_MD *digest;
	BYTE *nonce;
	int nonce_length;
	int is_anonymity_revocation_enabled; // boolean
	BYTE *anonymity_revocation_condition;
	int anonymity_revocation_condition_length;
	CS_PUBLIC_KEY *anonymity_revocator_pk;
	// private TssDaaSelectedAttrib[] selectedAttributes2Commit;
	TSS_DAA_SELECTED_ATTRIB **selected_attributes2commit;
	int selected_attributes2commitLength;
} DAA_VERIFIER_TRANSACTION;

/* the return (BYTE *) should be free after usage */
BYTE *compute_bytes( int seedLength, BYTE *seed, int length, const EVP_MD *digest);

bi_ptr compute_zeta( int nameLength, unsigned char *name, TSS_DAA_PK_internal *issuer_pk);

bi_ptr project_into_group_gamma( bi_ptr base, TSS_DAA_PK_internal *issuer_pk);
#if 0
TSPICALL Tspi_DAA_VerifyInit_internal
(
	TSS_HDAA hDAA,	// in
	UINT32* nonceVerifierLength,	// out
	BYTE** nonceVerifier,	// out
	UINT32 baseNameLength,	// out
	BYTE ** baseName		// out
);

TSPICALL Tspi_DAA_VerifySignature_internal
(	TSS_HDAA hDAA,	// in
	TSS_DAA_SIGNATURE signature, // in
	TSS_HKEY hPubKeyIssuer,	// in
	TSS_DAA_SIGN_DATA sign_data,	// in
	UINT32 attributes_length,	// in
	BYTE **attributes,	// in
	UINT32 nonce_verifierLength,	// out
	BYTE *nonce_verifier,	// out
	UINT32 base_nameLength,	// out
	BYTE *base_name,	// out
	TSS_BOOL *isCorrect	// out
);
#else
TSS_RESULT
Tspi_DAA_VerifySignature
(
    TSS_HDAA_CREDENTIAL           hDAACredential,                // in
    TSS_HDAA_ISSUER_KEY           hIssuerKey,                    // in
    TSS_HDAA_ARA_KEY              hARAKey,                       // in
    TSS_HHASH                     hARACondition,                 // in
    UINT32                        attributesLength,              // in
    UINT32                        attributesLength2,             // in
    BYTE**                        attributes,                    // in
    UINT32                        verifierNonceLength,           // in
    BYTE*                         verifierNonce,                 // in
    UINT32                        verifierBaseNameLength,        // in
    BYTE*                         verifierBaseName,              // in
    TSS_HOBJECT                   signData,                      // in
    TSS_DAA_SIGNATURE*            daaSignature,                  // in
    TSS_BOOL*                     isCorrect                      // out
);

#endif

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
	CS_ENCRYPTION_RESULT *encryption_result_proof);

#endif /*VERIFIER_H_*/
