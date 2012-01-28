
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
//#include "trousers_types.h"
#include "spi_utils.h"
//#include "capabilities.h"
#include "tsplog.h"
//#include "tcs_tsp.h"
//#include "tspps.h"
//#include "hosttable.h"
//#include "tcsd_wrap.h"
//#include "tcsd.h"
#include "obj.h"
#include "daa/issuer.h"
#include "daa/platform.h"
#include "daa/verifier.h"
#include "daa/anonymity_revocation.h"

#include "daa/key_correct.h"
#include "daa/issuer.h"

// static TSS_HCONTEXT _hContext;

static void *tss_alloc( size_t size, TSS_HOBJECT hContext) {
	void *ret = calloc_tspi( hContext, size);

	LogDebug("[intern_alloc (%d)] -> %d", (int)size, (int)ret);
	return ret;
}

/*
static void *normal_malloc( size_t size, TSS_HOBJECT object) {
	void *ret = malloc( size);
	return ret;
}
*/

/**
This is the first out of 3 functions to execute in order to receive a DAA Credential.
It verifies the keys of the DAA Issuer and computes the TPM DAA public key.

Parameters
	- hDAA:		Handle of the DAA object
	- hTPM:		Handle of the TPM object
	- daaCounter:	DAA counter
	- issuerPk:	Handle of the DAA Issuer public key
	- issuerAuthPKsLength:	Length of the array of issuerAuthPKs
	- issuerAuthPKs:Handle of an array of RSA public keys (key chain) of the DAA Issuer used
				to authenticate the DAA Issuer public key. The size of the modulus
				must be TPM_DAA_SIZE_issuerModulus (256)
	- issuerAuthPKSignaturesLength:Length of the array of issuerAuthPKSignatures. It is equal
				to issuerAuthPKsLength . The length of an element of the array is
				TPM_DAA_SIZE_issuerModulus (256)
	- issuerAuthPKSignatures: An array of byte arrays representing signatures on the modulus
				of the above key chain (issuerAuthPKs) in more details, the array has the
				following content (S(K[1],K[0]),S(K[2],N[1]),..S(K[ k ],K[n-1]),
				S(TPM_DAA_ISSUER,K[ k ])), where S(msg,privateKey) denotes
				the signature function with msg being signed by the privateKey.
	- capitalUprimeLength: Length of capitalUprime which is ln/8. ln is defined as the size of the RSA modulus (2048).
	- capitalUprime: U?
	- identityProof: This structure contains the endorsement, platform and conformance credential.
	- joinSession: This structure contains DAA Join session information.
*/
#if 0
TSPICALL
Tspi_TPM_DAA_JoinInit(TSS_HDAA                 hDAA,				// in
		      TSS_HTPM                 hTPM,				// in
		      UINT32                   daaCounter,			// in
		      TSS_HDAA_DATA            issuerPk,			// in
		      UINT32                   issuerAuthPKsLength,		// in
		      TSS_HKEY*                issuerAuthPKs,			// in
		      UINT32                   issuerAuthPKSignaturesLength,	// in
		      BYTE**                   issuerAuthPKSignatures,		// in
		      UINT32*                  capitalUprimeLength,		// out
		      BYTE**                   capitalUprime,			// out
		      TSS_DAA_IDENTITY_PROOF** identityProof,			// out
		      TSS_DAA_JOIN_SESSION**   joinSession)			// out
{
	TSS_RESULT result;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	LogDebug("-> TSPI_TPM_DAA_joinInit hDAA=%x hTPM=%x daaCounter=%x issuerPk=%x",
		(int)hDAA, (int)hTPM, daaCounter, (int)issuerPk);
	result = Tspi_TPM_DAA_JoinInit_internal(
		hDAA,
		hTPM,
		daaCounter,
		(TSS_DAA_PK *)issuerPk,
		issuerAuthPKsLength,
		(RSA **)issuerAuthPKs,
		issuerAuthPKSignaturesLength,
		issuerAuthPKSignatures,
		capitalUprimeLength,
		capitalUprime,
		identityProof,
		joinSession);
	bi_flush_memory();

	LogDebug("TSPI_TPM_DAA_joinInit ALLOC DELTA:%d",mallinfo().uordblks-before);
	LogDebug("<- TSPI_TPM_DAA_joinInit result=%d", result);
	return result;
}
#else
TSS_RESULT
Tspi_TPM_DAA_JoinInit(TSS_HTPM                 hTPM,                          /* in */
		      TSS_HDAA_ISSUER_KEY      hIssuerKey,                    /* in */
		      UINT32                   daaCounter,                    /* in */
		      UINT32                   issuerAuthPKsLength,           /* in */
		      TSS_HKEY*                issuerAuthPKs,                 /* in */
		      UINT32                   issuerAuthPKSignaturesLength,  /* in */
		      UINT32                   issuerAuthPKSignaturesLength2, /* in */
		      BYTE**                   issuerAuthPKSignatures,        /* in */
		      UINT32*                  capitalUprimeLength,           /* out */
		      BYTE**                   capitalUprime,                 /* out */
		      TSS_DAA_IDENTITY_PROOF** identityProof,                 /* out */
		      UINT32*                  joinSessionLength,             /* out */
		      BYTE**                   joinSession)                   /* out */
{
	TSS_RESULT result;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	if (!capitalUprimeLength || !capitalUprime || !identityProof || !joinSessionLength ||
	    !joinSession)
		return TSPERR(TSS_E_BAD_PARAMETER);

	result = Tspi_TPM_DAA_JoinInit_internal(hTPM, hIssuerKey, daaCounter, issuerAuthPKsLength,
						issuerAuthPKs, issuerAuthPKSignaturesLength,
						issuerAuthPKSignaturesLength2,
						issuerAuthPKSignatures, capitalUprimeLength,
						capitalUprime, identityProof, joinSessionLength,
						joinSession);

	bi_flush_memory();

	LogDebug("TSPI_TPM_DAA_joinInit ALLOC DELTA:%d",mallinfo().uordblks-before);
	LogDebug("<- TSPI_TPM_DAA_joinInit result=%d", result);

	return result;
}
#endif
/**
This function is part of the DAA Issuer component. It defines the generation of a DAA Issuer
public and secret key. Further it defines the generation of a non-interactive proof (using
the Fiat-Shamir heuristic) that the public keys were chosen correctly. The latter will guarantee
the security requirements of the platform (respectively, its user), i.e., that the privacy and
anonymity of signatures will hold.
The generation of the authentication keys of the DAA Issuer, which are used to authenticate
(main) DAA Issuer keys, is not defined by this function.
This is an optional function and does not require a TPM or a TCS.

Parameters
	- hDAA:	Handle of the DAA object
	- issuerBaseNameLength: Length of issuerBaseName
	- issuerBaseName:	Unique name of the DAA Issuer
	- numberPlatformAttributes: Number of attributes that the Platform can choose and which
						will not be visible to the Issuer.
	- numberIssuerAttributes:Number of attributes that the Issuer can choose and which will
						be visible to both the Platform and the Issuer.
	- keyPair:	Handle of the main DAA Issuer key pair (private and public portion)
	- publicKeyProof:Handle of the proof of the main DAA Issuer public key
*/
#if 0
TSPICALL
Tspi_DAA_IssueSetup(TSS_HDAA       hDAA,			// in
		    UINT32         issuerBaseNameLength,	// in
		    BYTE*          issuerBaseName,		// in
		    UINT32         numberPlatformAttributes,	// in
		    UINT32         numberIssuerAttributes,	// in
		    TSS_HDAA_DATA* keyPair,			// out (TSS_KEY_PAIR)
		    TSS_HDAA_DATA* publicKeyProof)		// out (TSS_DAA_PK_PROOF)
{
	TSS_RESULT result;
	KEY_PAIR_WITH_PROOF_internal *key_proof;
	TSS_DAA_KEY_PAIR *tss_daa_key_pair;
	TSS_HCONTEXT hContext;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	LogDebug( "TSPI_DAA_IssueSetup hDAA=%d  ",hDAA);
	// TODO: lock access to _hContext
	if ((result = obj_daa_get_tsp_context(hDAA, &hContext)))
		return result;
	result = generate_key_pair(numberIssuerAttributes,
						numberPlatformAttributes,
						issuerBaseNameLength,
						issuerBaseName,
						&key_proof);
	if (result != TSS_SUCCESS)
		return result;
	LogDebug("TSPI_DAA_IssueSetup convert internal structure to public allocated using tspi_alloc");
	LogDebug("key_proof->proof->length_challenge=%d  key_proof->proof->length_response=%d",
			key_proof->proof->length_challenge, key_proof->proof->length_response);
	// prepare out parameters
	*publicKeyProof = i_2_e_TSS_DAA_PK_PROOF( key_proof->proof, &tss_alloc, hContext);

	tss_daa_key_pair = (TSS_DAA_KEY_PAIR *)tss_alloc( sizeof(TSS_DAA_KEY_PAIR), hContext);
	if (tss_daa_key_pair == NULL) {
		LogError("malloc of %d bytes failed", sizeof(TSS_DAA_KEY_PAIR));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	tss_daa_key_pair->private_key = i_2_e_TSS_DAA_PRIVATE_KEY( key_proof->private_key,
								&tss_alloc,
								hContext);
	tss_daa_key_pair->public_key = i_2_e_TSS_DAA_PK( key_proof->pk,
							&tss_alloc,
							hContext);
	*keyPair = (TSS_HKEY)tss_daa_key_pair;
close:
	bi_flush_memory();

	LogDebug("TSPI_DAA_IssueSetup ALLOC DELTA:%d", mallinfo().uordblks-before);
	LogDebug( "TSPI_DAA_IssueSetup end return=%d ",result);
	return result;
}
#else
TSS_RESULT
Tspi_DAA_Issuer_GenerateKey(TSS_HDAA_ISSUER_KEY hIssuerKey,           // in
			    UINT32              issuerBaseNameLength, // in
			    BYTE*               issuerBaseName)       // in
{
	TSS_RESULT result;
	KEY_PAIR_WITH_PROOF_internal *key_proof;
	TSS_DAA_KEY_PAIR *tss_daa_key_pair;
	TSS_HCONTEXT tspContext;
	UINT32 numberPlatformAttributes, numberIssuerAttributes;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	if ((result = obj_daaissuerkey_get_tsp_context(hIssuerKey, &tspContext)))
		return result;

	if ((result = obj_daaissuerkey_get_attribs(hIssuerKey, &numberIssuerAttributes,
						   &numberPlatformAttributes)))
		return result;

	if ((result = generate_key_pair(numberIssuerAttributes, numberPlatformAttributes,
					issuerBaseNameLength, issuerBaseName, &key_proof)))
		return result;

	LogDebugFn("convert internal structure to public allocated using tspi_alloc");
	LogDebug("key_proof->proof->length_challenge=%d  key_proof->proof->length_response=%d",
		 key_proof->proof->length_challenge, key_proof->proof->length_response);

	// prepare out parameters
	*publicKeyProof = i_2_e_TSS_DAA_PK_PROOF( key_proof->proof, &tss_alloc, tspContext);

	tss_daa_key_pair = (TSS_DAA_KEY_PAIR *)tss_alloc( sizeof(TSS_DAA_KEY_PAIR), tspContext);
	if (tss_daa_key_pair == NULL) {
		LogError("malloc of %d bytes failed", sizeof(TSS_DAA_KEY_PAIR));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	tss_daa_key_pair->private_key = i_2_e_TSS_DAA_PRIVATE_KEY( key_proof->private_key,
								&tss_alloc,
								tspContext);
	tss_daa_key_pair->public_key = i_2_e_TSS_DAA_PK( key_proof->pk,
							&tss_alloc,
							tspContext);
	*keyPair = (TSS_HKEY)tss_daa_key_pair;
close:
	bi_flush_memory();

	LogDebug("TSPI_DAA_IssueSetup ALLOC DELTA:%d", mallinfo().uordblks-before);
	LogDebug( "TSPI_DAA_IssueSetup end return=%d ",result);
	return result;
}
#endif
/**
This function is part of the DAA Issuer component. It's the first function out of 2 in order to
issue a DAA Credential for a TCG Platform. It assumes that the endorsement key and its
associated credentials are from a genuine and valid TPM. (Verification of the credentials is
 a process defined by the TCG Infrastructure WG.)
This is an optional function and does not require a TPM or a TCS.
*/
TSPICALL
Tspi_DAA_IssueInit(TSS_HDAA                      hDAA,                          // in
		   TSS_HKEY                      issuerAuthPK,                  // in
		   TSS_HDAA_DATA                 issuerKeyPair,		    // in (TSS_DAA_KEY_PAIR)
		   TSS_DAA_IDENTITY_PROOF*       identityProof,                 // in
		   UINT32                        capitalUprimeLength,           // in
		   BYTE*                         capitalUprime,                 // in
		   UINT32                        daaCounter,                    // in
		   UINT32*                       nonceIssuerLength,             // out
		   BYTE**                        nonceIssuer,                   // out
		   UINT32*                       authenticationChallengeLength, // out
		   BYTE**                        authenticationChallenge,       // out
		   TSS_DAA_JOIN_ISSUER_SESSION** joinSession)                   // out
{
	TSS_RESULT result;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	LogDebug("Tspi_DAA_IssueInit_internal hDAA=%d daaCounter=%d", (int)hDAA, (int)daaCounter);
	result = Tspi_DAA_IssueInit_internal(
		hDAA,			// in
		issuerAuthPK,		// in
		issuerKeyPair,		// in
		identityProof,		// in
		capitalUprimeLength,	// in
		capitalUprime,		// in
		daaCounter,	// in
		nonceIssuerLength,	// out
		nonceIssuer,	// out
		authenticationChallengeLength,	// out
		authenticationChallenge,	// out
		joinSession	// out
	);
	bi_flush_memory();

	LogDebug("Tspi_DAA_IssueInit_internal ALLOC DELTA:%d", mallinfo().uordblks-before);

	return result;
}

/**
This function verifies the DAA public key of a DAA Issuer with respect to its associated proof.
This is a resource consuming task. It can be done by trusted third party (certification).
This is an optional function and does not require a TPM or a TCS.
Parameters:
	- hDAA: Handle of the DAA object
	- issuerPk: DAA Issuer public key
	- issuerPkProof: Proofs the correctness of the DAA Issuer public key
	- isCorrect: Proofs the correctness of the DAA Issuer public key
*/
TSPICALL
Tspi_DAA_IssuerKeyVerification(TSS_HDAA          hDAA,		// in
			       TSS_HDAA_DATA     issuerPk,	// in (TSS_DAA_PK)
			       TSS_HDAA_DATA     issuerPkProof,	// in (TSS_DAA_PK_PROOF)
			       TSS_BOOL*         isCorrect)	// out
{
	TSS_RESULT result;
	int is_correct;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	LogDebug("TSPI_DAA_IssuerKeyVerification hDAA=%ld issuerPk=%ld issuerPkProof=%ld",
		(long)hDAA, (long)issuerPk, (long)issuerPkProof);
	TSS_DAA_PK_internal *pk_internal = e_2_i_TSS_DAA_PK( (TSS_DAA_PK *)issuerPk);
	TSS_DAA_PK_PROOF_internal *proof_internal = e_2_i_TSS_DAA_PK_PROOF( issuerPkProof);
	LogDebug( "challenge=[%s]", dump_byte_array( proof_internal->length_challenge,
							proof_internal->challenge));
	result = is_pk_correct( pk_internal, proof_internal, &is_correct );
	if( is_correct) *isCorrect = TRUE;
	else *isCorrect = FALSE;
	bi_flush_memory();
#ifdef TSS_DEBUG
	LogDebug("TSPI_DAA_IssuerKeyVerification ALLOC DELTA:%d", mallinfo().uordblks-before);
#endif
	return result;
}

/**
This function is part of the DAA Issuer component. It?s the last function out of 2 in order to
issue DAA Credential for a TCG Platform. It detects rogue TPM according to published rogue
TPM DAA keys.
This is an optional function and does not require a TPM or a TCS.
*/
TSPICALL
Tspi_DAA_IssueCredential(TSS_HDAA                      hDAA,			// in
			 UINT32                        attributesIssuerLength,	// in
			 BYTE**                        attributesIssuer,	// in
			 TSS_DAA_CREDENTIAL_REQUEST*   credentialRequest,	// in
			 TSS_DAA_JOIN_ISSUER_SESSION*  joinSession,		// in
			 TSS_DAA_CRED_ISSUER**         credIssuer)		// out
{
	TSS_RESULT result;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	LogDebug("Tspi_DAA_IssueCredential hDAA=%d attributesIssuerLength=%d",
		(int)hDAA,
		(int)attributesIssuerLength);
	result = Tspi_DAA_IssueCredential_internal(
		hDAA,
		attributesIssuerLength,
		attributesIssuer,
		credentialRequest,
		joinSession,
		credIssuer
	);
	bi_flush_memory();
#ifdef TSS_DEBUG
	LogDebug("Tspi_DAA_IssueCredential ALLOC DELTA:%d", mallinfo().uordblks-before);
#endif
	return result;
}


/**
This function is part of the DAA Verifier component. It is the first function out of 2 in order
to verify a DAA credential of a TCG platform. It creates a challenge for the TCG platform.last
function out of 2 in order to issue a This is an optional function and does not require a
TPM or a TCS.
*/
TSPICALL
Tspi_DAA_VerifyInit(TSS_HDAA hDAA,			// in
		    UINT32*  nonceVerifierLength,	// out
		    BYTE**   nonceVerifier,		// out
		    UINT32*  baseNameLength,		// out
		    BYTE**   baseName)			// out
{
	TSS_RESULT result;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	// TODO which interface to use ? with or without baseName ?
	LogDebug("Tspi_DAA_VerifyInit hDAA=%d", (int)hDAA);
	result = Tspi_DAA_VerifyInit_internal( hDAA,
						nonceVerifierLength,
						nonceVerifier,
						baseNameLength,
						baseName);
	bi_flush_memory();
#ifdef TSS_DEBUG
	LogDebug("Tspi_DAA_VerifyInit ALLOC DELTA:%d", mallinfo().uordblks-before);
#endif
	return result;
}


/**
This function is part of the DAA Verifier component. It is the last function out of 2 in
order to verify a DAA Credential of a TCG Platform. It verifies the DAA credential and
detects public rogue TPMs. This is an optional function and does not require a TPM
or a TCS.
*/
TSPICALL
Tspi_DAA_VerifySignature(TSS_HDAA           hDAA,		// in
			 TSS_DAA_SIGNATURE* daaSignature,	// in
			 TSS_HDAA_DATA      hPubKeyIssuer,	// in (TSS_DAA_PK)
			 TSS_DAA_SIGN_DATA* signData,		// in
			 UINT32             attributesLength,	// in
			 BYTE**             attributes,		// in
			 UINT32             nonceVerifierLength,// in
			 BYTE*              nonceVerifier,	// in
			 UINT32             baseNameLength,	// in
			 BYTE*              baseName,		// in
			 TSS_BOOL*          isCorrect)		// out
{
	TSS_RESULT result;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	LogDebug("Tspi_DAA_VerifySignature hDAA=%d", (int)hDAA);
	result = Tspi_DAA_VerifySignature_internal( hDAA,
						daaSignature,
						hPubKeyIssuer,
						signData,
						attributesLength,
						attributes,
						nonceVerifierLength,
						nonceVerifier,
						baseNameLength,
						baseName,
						isCorrect);
	bi_flush_memory();
#ifdef TSS_DEBUG
	LogDebug("Tspi_DAA_VerifySignature ALLOC DELTA:%d", mallinfo().uordblks-before);
#endif
	return result;
}


/**
This function is part of the DAA Issuer component. It is the last function out of 2 in
order to issue a DAA Credential for a TCG Platform. It detects rogue TPM according
to published rogue TPM DAA keys.
This is an optional function and does not require a TPM or a TCS.

Parameters
	- hDAA: 	Handle of the DAA object
	- daaPublicKey: 	daaPublickKey
	- keyPair: Public and private key of the DAA Anonymity Revocation Authority to encrypt
			the pseudonym of a DAA Signature
*/
TSPICALL
Tspi_DAA_RevokeSetup(TSS_HDAA       hDAA,		// in
		     TSS_HDAA_DATA  daaPublicKey,	// in
		     TSS_HDAA_DATA* arPublicKey,	// out (TSS_DAA_AR_PK)
		     TSS_HDAA_DATA* arPrivateKey)	// out (TSS_DAA_AR_SK)
{
	TSS_RESULT result;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	result = Tspi_DAA_RevokeSetup_internal(
		hDAA,			// in
//TODO: remove cast when the above interface is changed
		daaPublicKey,	// in
		keyPair		// out
	);
	bi_flush_memory();
#ifdef TSS_DEBUG
	LogDebug("Tspi_DAA_RevokeSetup ALLOC DELTA:%d", mallinfo().uordblks-before);
#endif
	return TSS_SUCCESS;
}


/**
This function is part of the DAA Anonymity Revocation Authority component. It defines the
Cramer-Shoup decryption algorithm to revoke the anonymity of a DAA Signature. The pseudonym,
with respect to either the DAA Verifier?s base name, the DAA Issuer?s base name or (just for
completeness) a random base name, can be revealed.
The pseudonym with respect to a DAA Signature and the used base name is V N . An encryption of
V N is the tuple (d1,d 2 ,d 3,d 4 ) and is decrypted using the secret key ( 0 5 x ,?, x ), the
decryption condition and the DAA public key.
This is an optional function and does not require a TPM or a TCS.

Parameters:
	- hDAA:					Handle of the DAA object
	- encryptedPseudonym: 			encryptedPseudonym
	- decryptCondition: 			Condition for the decryption of the pseudonym.
	- arPrivateKey:				arPrivateKey
	- daaPublicKey:				daaPublicKey
	- pseudonym:				pseudonym
*/
TSPICALL
Tspi_DAA_ARDecrypt(TSS_HDAA                     hDAA,			// in
		   TSS_DAA_PSEUDONYM_ENCRYPTED* encryptedPseudonym,	// in
		   TSS_HHASH                    decryptCondition,	// in
		   TSS_HDAA_DATA                arPrivateKey,		// in (TSS_DAA_AR_SK)
		   TSS_HDAA_DATA                daaPublicKey,		// in (TSS_DAA_PK)
		   TSS_DAA_PSEUDONYM_PLAIN**    pseudonym)		// out
{
	TSS_RESULT result;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	result = Tspi_DAA_ARDecrypt_internal(
		hDAA,			// in
		encryptedPseudonym,	// in
		decryptCondition,	// in
//TODO: remove cast when the above interface is changed
		(void *)arPrivateKey,		// in
		(void *)daaPublicKey,		// in
		pseudonym		// out
	);
	bi_flush_memory();
#ifdef TSS_DEBUG
	LogDebug("Tspi_DAA_ARDecrypt ALLOC DELTA:%d", mallinfo().uordblks-before);
#endif
	return result;
}

/**
This is the second out of 3 functions to execute in order to receive a DAA Credential. It
computes the credential request for the DAA Issuer, which also includes the Platform
DAA public key and the attributes that were chosen by the Platform, and which are not
visible to the DAA Issuer. The Platform can commit to the attribute values it has chosen.
*/
TSPICALL
Tspi_TPM_DAA_JoinCreateDaaPubKey(TSS_HDAA                     hDAA,	                    // in
				 TSS_HTPM                     hTPM,	                    // in
				 UINT32                       authenticationChallengeLength,// in
				 BYTE*                        authenticationChallenge,      // in
				 UINT32                       nonceIssuerLength,            // in
				 BYTE*                        nonceIssuer,                  // in
				 UINT32                       attributesPlatformLength,     // in
				 BYTE**                       attributesPlatform,           // in
				 TSS_DAA_JOIN_SESSION*        joinSession,                // in, out
				 TSS_DAA_CREDENTIAL_REQUEST** credentialRequest)            // out
{
	TSS_RESULT result;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	LogDebug("Tspi_TPM_DAA_JoinCreateDaaPubKey hDAA=%d joinSession=%d",
			(int)hDAA, (int)joinSession);
	result = Tspi_TPM_DAA_JoinCreateDaaPubKey_internal(
		hDAA,	// in
		hTPM,	// in
		authenticationChallengeLength,	// in
		authenticationChallenge,	// in
		nonceIssuerLength,	// in
		nonceIssuer,	// in
		attributesPlatformLength,	// in
		attributesPlatform,	// in
		joinSession,	// in, out
		credentialRequest	// out
	);
	bi_flush_memory();
#ifdef TSS_DEBUG
	LogDebug("Tspi_TPM_DAA_JoinCreateDaaPubKey ALLOC DELTA:%d", mallinfo().uordblks-before);
#endif
	return result;
}


/**
This is the last out of 3 functions to execute in order to receive a DAA Credential.
It verifies the issued credential from the DAA Issuer and computes the final DAA Credential.
*/
TSPICALL
Tspi_TPM_DAA_JoinStoreCredential(TSS_HDAA              hDAA,		// in
				 TSS_HTPM              hTPM,		// in
				 TSS_DAA_CRED_ISSUER*  credIssuer,	// in
				 TSS_DAA_JOIN_SESSION* joinSession,	// in
				 TSS_HDAA_DATA*        phDaaCredential)	// out (TSS_DAA_CREDENTIAL)
{
	TSS_RESULT result;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	LogDebug("Tspi_TPM_DAA_JoinStoreCredential hDAA=%d credIssuer=%d joinSession=%d",
		(int)hDAA, (int)&credIssuer, (int)&joinSession);
	result = Tspi_TPM_DAA_JoinStoreCredential_internal(hDAA,
							hTPM,
							credIssuer,
							joinSession,
							phDaaCredential);
	bi_flush_memory();
#ifdef TSS_DEBUG
	LogDebug("Tspi_TPM_DAA_JoinStoreCredential ALLOC DELTA:%d", mallinfo().uordblks-before);
#endif
	return result;
}


/**
This function creates a DAA Signature that proofs ownership of the DAA Credential and
includes a signature on either a public AIK or a message.
If anonymity revocation is enabled, the value Nv is not provided in the clear anymore but
encrypted under the public key of anonymity revocation authority, a trusted third party (TTP).
Thus the DAA Verifier cannot check for revocation or link a transaction/signature to prior ones.
Depending on how z is chosen, the protocol either allows to implementing anonymity revocation
(i.e., using the DAA Issuer long-term base name bsn I as the DAA Verifier base name bsnV ), or
having the TTP doing the linking of different signatures for the same DAA Verifier (i.e.,
using the DAA Verifier base name ).
*/
TSPICALL
Tspi_TPM_DAA_Sign(TSS_HDAA                 hDAA,			// in
		  TSS_HTPM                 hTPM,			// in
		  TSS_HDAA_DATA            hDaaCredential,		// in (TSS_DAA_CREDENTIAL)
		  TSS_DAA_SELECTED_ATTRIB* revealAttributes,		// in
		  UINT32                   verifierBaseNameLength,	// in
		  BYTE*                    verifierBaseName,		// in
		  UINT32                   verifierNonceLength,		// in
		  BYTE*                    verifierNonce,		// in
		  TSS_DAA_SIGN_DATA*       signData,			// in
		  TSS_DAA_SIGNATURE**      daaSignature)		// out
{
	TSS_RESULT result;
#ifdef TSS_DEBUG
	int before = mallinfo().uordblks;
#endif

	LogDebug("-> TSPI_TPM_DAA_Sign hDAA=%ld hTPM=%ld ", (long)hDAA, (long)hTPM);

	result = Tspi_TPM_DAA_Sign_internal(hDAA,
					hTPM,
					hDaaCredential,
					revealAttributes,
					verifierBaseNameLength,
					verifierBaseName,
					verifierNonceLength,
					verifierNonce,
					signData,
					daaSignature);
	bi_flush_memory();
#ifdef TSS_DEBUG
	LogDebug("TSPI_TPM_DAA_joinInit ALLOC DELTA:%d", mallinfo().uordblks-before);
#endif
	LogDebug("<- TSPI_TPM_DAA_joinInit result=%d", result);
	return result;
}
