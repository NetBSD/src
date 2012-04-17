
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
#include <openssl/hmac.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

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
#include "issuer.h"
#include "verifier.h"

#define EVP_SUCCESS 1

TSS_RESULT
Tcsip_TPM_DAA_Join(TCS_CONTEXT_HANDLE tcsContext, // in
			TSS_HDAA hDAA, // in
			BYTE stage, // in
			UINT32 inputSize0, // in
			BYTE* inputData0, // in
			UINT32 inputSize1, // in
			BYTE* inputData1, // in
			TPM_AUTH* ownerAuth, // in/out
			UINT32* outputSize, // out
			BYTE** outputData // out
) {
	TSS_RESULT result;
	TSS_HPOLICY hPolicy;
	TCPA_DIGEST digest;
	UINT16 offset = 0;
	BYTE hashblob[10000];
	TPM_HANDLE hTPM;
	TPM_HANDLE join_session;
	// TPM_HANDLE hTPM;

	if( (result = obj_daa_get_handle_tpm( hDAA, &hTPM)) != TSS_SUCCESS)
		return result;
	if( (result = obj_daa_get_session_handle( hDAA, &join_session)) != TSS_SUCCESS)
		return result;
	LogDebug("Tcsip_TPM_DAA_Join(tcsContext=%x,hDAA=%x,join_session=%x, hTPM=%x stage=%d)",
		tcsContext,
		hDAA,
		join_session,
		hTPM,
		stage);

	LogDebug("obj_tpm_get_policy(hTPM=%X)", hTPM);
	if( (result = obj_tpm_get_policy( hTPM, &hPolicy)) != TSS_SUCCESS)
		return result;
	LogDebug("Trspi_LoadBlob_UINT32(&offset, TPM_ORD_DAA_Join, hashblob)");
	// hash TPM_COMMAND_CODE
	Trspi_LoadBlob_UINT32(&offset, TPM_ORD_DAA_Join, hashblob);
	LogDebug("Trspi_Hash(TSS_HASH_SHA1, offset, hashblob, digest.digest)");
	// hash stage
	Trspi_LoadBlob_BYTE(&offset, stage, hashblob);
	LogDebug("Trspi_LoadBlob_UINT32(&offset, 0, hashblob)");
	// hash inputSize0
	Trspi_LoadBlob_UINT32(&offset, inputSize0, hashblob);
	LogDebug("Trspi_LoadBlob_UINT32(&offset, inputSize0:%d", inputSize0);
	// hash inputData0
	Trspi_LoadBlob( &offset, inputSize0, hashblob, inputData0);
	// hash inputSize1
	Trspi_LoadBlob_UINT32(&offset, inputSize1, hashblob);
	LogDebug("Trspi_LoadBlob_UINT32(&offset, inputSize1:%d", inputSize1);
	// hash inputData1
	Trspi_LoadBlob( &offset, inputSize1, hashblob, inputData1);
	Trspi_Hash(TSS_HASH_SHA1, offset, hashblob, digest.digest);

	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_DAA_Join,
	     hPolicy, &digest,
	     ownerAuth)) != TSS_SUCCESS) return result;
	LogDebug("secret_PerformAuth_OIAP(hTPM, TPM_ORD_DAA_Join ret=%d", result);
	LogDebug("TCSP_DAAJoin(%x,%x,stage=%x,%x,%x,%x,%x,%x)\n",
		tcsContext,
		hTPM,
		stage,
		inputSize0,
		(int)inputData0,
		inputSize1,
		(int)inputData1,
		(int)&ownerAuth);
	/* step of the following call:
	TCSP_DAAJoin 		tcsd_api/calltcsapi.c (define in spi_utils.h)
	TCSP_DAAJoin_TP 	tcsd_api/tcstp.c (define in	trctp.h)
	*/
	result =  TCSP_DaaJoin( tcsContext,
				join_session,
				stage,
				inputSize0, inputData0,
				inputSize1, inputData1,
				ownerAuth,
				outputSize, outputData);
	if( result != TSS_SUCCESS) return result;

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, result, hashblob);
	Trspi_LoadBlob_UINT32(&offset, TPM_ORD_DAA_Join, hashblob);
	Trspi_LoadBlob_UINT32(&offset, *outputSize, hashblob);
	Trspi_LoadBlob(&offset, *outputSize, hashblob, *outputData);
	LogDebug("TCSP_DAAJoin stage=%d outputSize=%d outputData=%x RESULT=%d",
		(int)stage, (int)*outputSize, (int)outputData, (int)result);
	Trspi_Hash(TSS_HASH_SHA1, offset, hashblob, digest.digest);
	if( (result = obj_policy_validate_auth_oiap( hPolicy, &digest, ownerAuth))) {
		LogError("obj_policy_validate_auth=%d", result);
	}
	return result;
}

TSS_RESULT Tcsip_TPM_DAA_Sign( TCS_CONTEXT_HANDLE hContext,	// in
				TPM_HANDLE handle,	// in
				BYTE stage,	// in
				UINT32 inputSize0,	// in
				BYTE* inputData0,	// in
				UINT32 inputSize1,	// in
				BYTE* inputData1,	// in
				TPM_AUTH* ownerAuth,	// in, out
				UINT32* outputSize,	// out
				BYTE** outputData	// out
) {
	TSS_RESULT result;
	TSS_HPOLICY hPolicy;
	TCPA_DIGEST digest;
	UINT16 offset = 0;
	BYTE hashblob[1000];
	TPM_HANDLE hTPM;
	TPM_HANDLE session_handle;
	TSS_HDAA hDAA = (TSS_HDAA)handle;
	// TPM_HANDLE hTPM;

	if( (result = obj_daa_get_handle_tpm( hDAA, &hTPM)) != TSS_SUCCESS)
		return result;
	if( (result = obj_daa_get_session_handle( hDAA, &session_handle)) != TSS_SUCCESS)
		return result;
	LogDebug("Tcsip_TPM_DAA_Sign(tcsContext=%x,hDAA=%x,sign_session=%x, hTPM=%x stage=%d)",
		hContext,
		hDAA,
		session_handle,
		hTPM,
		stage);

	LogDebug("obj_tpm_get_policy(hTPM=%X)", hTPM);
	if( (result = obj_tpm_get_policy( hTPM, &hPolicy)) != TSS_SUCCESS)
		return result;
	LogDebug("Trspi_LoadBlob_UINT32(&offset, TPM_ORD_DAA_Sign, hashblob)");
	// hash TPM_COMMAND_CODE
	Trspi_LoadBlob_UINT32(&offset, TPM_ORD_DAA_Sign, hashblob);
	LogDebug("Trspi_Hash(TSS_HASH_SHA1, offset, hashblob, digest.digest)");
	// hash stage
	Trspi_LoadBlob_BYTE(&offset, stage, hashblob);
	LogDebug("Trspi_LoadBlob_UINT32(&offset, 0, hashblob)");
	// hash inputSize0
	Trspi_LoadBlob_UINT32(&offset, inputSize0, hashblob);
	LogDebug("Trspi_LoadBlob_UINT32(&offset, inputSize0:%d", inputSize0);
	// hash inputData0
	Trspi_LoadBlob( &offset, inputSize0, hashblob, inputData0);
	// hash inputSize1
	Trspi_LoadBlob_UINT32(&offset, inputSize1, hashblob);
	LogDebug("Trspi_LoadBlob_UINT32(&offset, inputSize1:%d", inputSize1);
	// hash inputData1
	Trspi_LoadBlob( &offset, inputSize1, hashblob, inputData1);
	Trspi_Hash(TSS_HASH_SHA1, offset, hashblob, digest.digest);

	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_DAA_Join,
	     hPolicy, &digest,
	     ownerAuth)) != TSS_SUCCESS) return result;
	LogDebug("secret_PerformAuth_OIAP(hTPM, TPM_ORD_DAA_Join ret=%d", result);
	LogDebug("TCSP_DAASign(%x,%x,stage=%x,%x,%x,%x,%x,%x)",
		hContext,
		hTPM,
		stage,
		inputSize0,(int)inputData0,
		inputSize1,(int)inputData1,
		(int)&ownerAuth);
	/* step of the following call:
	TCSP_DAASign 		tcsd_api/calltcsapi.c (define in spi_utils.h)
	TCSP_DAASign_TP 	tcsd_api/tcstp.c (define in	trctp.h)
	*/
	result =  TCSP_DaaSign( hContext,
				session_handle,
				stage,
				inputSize0, inputData0,
				inputSize1, inputData1,
				ownerAuth,
				outputSize, outputData);
	if( result != TSS_SUCCESS) return result;

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, result, hashblob);
	Trspi_LoadBlob_UINT32(&offset, TPM_ORD_DAA_Sign, hashblob);
	Trspi_LoadBlob_UINT32(&offset, *outputSize, hashblob);
	Trspi_LoadBlob(&offset, *outputSize, hashblob, *outputData);
	LogDebug("TCSP_DAASign stage=%d outputSize=%d outputData=%x RESULT=%d",
		(int)stage, (int)*outputSize, (int)outputData, (int)result);
	Trspi_Hash(TSS_HASH_SHA1, offset, hashblob, digest.digest);
	if( (result = obj_policy_validate_auth_oiap( hPolicy, &digest, ownerAuth)))
	{
		LogError("obj_policy_validate_auth=%d", result);
	}
	return result;
}

/**
	Only used for the logging
*/
static TSS_RESULT
Tcsip_TPM_DAA_Join_encapsulate(TCS_CONTEXT_HANDLE tcsContext, // in
				TSS_HDAA	hDAA,				// in
				BYTE stage, // in
				UINT32 inputSize0, // in
				BYTE* inputData0, // in
				UINT32 inputSize1, // in
				BYTE* inputData1, // in
				TPM_AUTH* ownerAuth, // in/out
				UINT32* outputSize, // out
				BYTE** outputData // out
) {
	TSS_RESULT result;

	LogDebug("Tcsip_DAA_Join(TCS_CONTEXT=%X,TSS_HDAA=%X,stage=%d,\
				inputSize0=%u,inputData0=%s\ninputSize1=%u,inputData1=%s,ownerAuth=%X)",
		tcsContext, hDAA, stage,
		inputSize0, dump_byte_array(inputSize0, inputData0),
		inputSize1,dump_byte_array(inputSize1, inputData1),
		(int)ownerAuth);
	result = Tcsip_TPM_DAA_Join(
		tcsContext, // in
		hDAA,	// in
		stage, // in
		inputSize0, // in
		inputData0, // in
		inputSize1, // in
		inputData1, // in
		ownerAuth, // in/out
		outputSize, // out
		outputData // out
	);
	LogDebug("Tcsip_DAA_Join(stage=%d,outputSize=%u outputData=%s ownerAuth=%X) result=%d",
		 (int)stage, *outputSize, dump_byte_array( *outputSize, *outputData), (int)ownerAuth, result);
	return result;
}

#if 0
/* from TSS.java */
/* openssl RSA (struct rsa_st) could manage RSA Key */
TSS_RESULT
Tspi_TPM_DAA_JoinInit_internal(	TSS_HDAA hDAA,
				TSS_HTPM hTPM,
				int daa_counter,
				TSS_DAA_PK *issuer_pk,
				int issuer_authentication_PKLengh,
				RSA **issuer_authentication_PK,
				int issuer_authentication_PK_signaturesLength,
				BYTE **issuer_authentication_PK_signatures,
				int *capital_UprimeLength,
				BYTE **capital_Uprime,
				TSS_DAA_IDENTITY_PROOF *identity_proof,
				TSS_DAA_JOIN_SESSION *join_session)
{
	// Optional: verification of the PKDAA and issuer settings (authen. by the RSA Public Key chain)
	TSS_RESULT result;
	TCS_CONTEXT_HANDLE tcsContext;
	TPM_AUTH ownerAuth;
	int i, modulus_length, outputSize, length, length1;
	UINT32 return_join_session;
	BYTE *outputData, *issuer_settings_bytes;
	BYTE *buffer_modulus;
	TPM_DAA_ISSUER *issuer_settings;
	char buffer[1000], buffer1[1000];
	TSS_DAA_PK_internal *pk_internal = e_2_i_TSS_DAA_PK( issuer_pk);
	bi_ptr issuer_authentication_PK_i = NULL;

	if( (result = obj_tpm_is_connected(  hTPM, &tcsContext)) != TSS_SUCCESS)
		return result;
	obj_daa_set_handle_tpm( hDAA, hTPM);

	// stages 0-2 explained in the diagram "Keys of DAA Issuer"
	// issuer_authentication_PKLengh should be converted to Network Based integer
	i = htonl(issuer_authentication_PKLengh);
	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
			0,
			sizeof(int), (BYTE *)(&i),
			0, NULL,
			&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) { goto close;}
	// set the sessionHandle to the returned value
	return_join_session = ntohl( *((UINT32 *)outputData));
	free( outputData);
	obj_daa_set_session_handle( hDAA, return_join_session);

	LogDebug("done join 0 settings join_session:%x\n", return_join_session);
	modulus_length = DAA_PARAM_SIZE_RSA_MODULUS / 8;
	buffer_modulus = malloc(modulus_length);
	if (buffer_modulus == NULL) {
		LogError("malloc of %d bytes failed", modulus_length);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	issuer_authentication_PK_i = bi_new_ptr();
	for(i =0; i< issuer_authentication_PKLengh; i++) {
		bi_set_as_BIGNUM( issuer_authentication_PK_i, issuer_authentication_PK[i]->n);
		bi_2_byte_array( buffer_modulus,
				modulus_length,
				issuer_authentication_PK_i);
		if ( i==0) {
			result = Tcsip_TPM_DAA_Join_encapsulate(
				tcsContext, hDAA,
				1,
				modulus_length, buffer_modulus,
				0, NULL,
				&ownerAuth, &outputSize, &outputData);
			if( result != TSS_SUCCESS) {
				free( buffer_modulus);
				goto close;
			}
		} else {
			result = Tcsip_TPM_DAA_Join_encapsulate(
				tcsContext,  hDAA,
				1,
				modulus_length, buffer_modulus,
				DAA_PARAM_KEY_SIZE / 8, issuer_authentication_PK_signatures[i -1],
				&ownerAuth, &outputSize, &outputData);
			if( result != TSS_SUCCESS) {
				free( buffer_modulus);
				goto close;
			}
		}
	}
	free( buffer_modulus);
	LogDebug("done join 1-%d\n", issuer_authentication_PKLengh);
	// define issuer_settings
	issuer_settings = convert2issuer_settings( pk_internal);
	issuer_settings_bytes = issuer_2_byte_array( issuer_settings, &length);
	result = Tcsip_TPM_DAA_Join_encapsulate(
		tcsContext, hDAA,
		2,
		length, issuer_settings_bytes,
		modulus_length,
		issuer_authentication_PK_signatures[issuer_authentication_PKLengh-1],
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) { goto close;}
	LogDebug("done join 2\n");
	i = htonl( daa_counter);
	result = Tcsip_TPM_DAA_Join_encapsulate(
		tcsContext, hDAA,
		3,
		sizeof(UINT32), (BYTE *)(&i),
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) { goto close;}
	LogDebug("done join 3\n");
	// reserved another buffer for storing Big Integer
	bi_2_nbin1( &length, buffer, pk_internal->capitalR0);
	bi_2_nbin1( &length1, buffer1, pk_internal->modulus);
	result = Tcsip_TPM_DAA_Join_encapsulate(
		tcsContext, hDAA,
		4,
		length, buffer,
		length1, buffer1,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) { goto close;}
	LogDebug("done join 4\n");
	bi_2_nbin1( &length, buffer, pk_internal->capitalR1);
	result = Tcsip_TPM_DAA_Join_encapsulate(
		tcsContext, hDAA,
		5,
		length, buffer,
		length1, buffer1,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) { goto close;}
	LogDebug("done join 5\n");
	bi_2_nbin1( &length, buffer, pk_internal->capitalS);
	result = Tcsip_TPM_DAA_Join_encapsulate(
		tcsContext, hDAA,
		6,
		length, buffer,
		length1, buffer1,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) { goto close;}
	LogDebug("done join 6\n");
	bi_2_nbin1( &length, buffer, pk_internal->capitalSprime);
	// define Uprime
	result = Tcsip_TPM_DAA_Join_encapsulate(
		tcsContext, hDAA,
		7,
		length, buffer,
		length1, buffer1,
		&ownerAuth, &outputSize, &outputData);
	// 5 : save PKDAA, U, daaCount and sessionHandle in joinSession
	join_session->issuerPk = (TSS_HKEY)issuer_pk;
	if( result == TSS_SUCCESS) {
		*capital_UprimeLength = outputSize;
		*capital_Uprime = convert_alloc( tcsContext, outputSize, outputData);
		if (*capital_Uprime == NULL) {
			LogError("malloc of %d bytes failed", outputSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		join_session->capitalUPrime = copy_alloc( tcsContext,
							*capital_UprimeLength,
							*capital_Uprime);
		if (join_session->capitalUPrime == NULL) {
			LogError("malloc of %d bytes failed", *capital_UprimeLength);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		join_session->capitalUPrimeLength = *capital_UprimeLength;
	}
	join_session->sessionHandle = return_join_session;
	// get the endorsement Key (public part)
	result = get_public_EK(
		hTPM,	// in
		&( identity_proof->endorsementLength),
		&( identity_proof->endorsementCredential)
	);

close:
	FREE_BI( issuer_authentication_PK_i);
	LogDebug("result = %d", result);
	LogDebug("outputSize=%d", outputSize);
	LogDebug("outputData=%s", dump_byte_array( outputSize, outputData));
  	return result;
}
#else
TSS_RESULT
Tspi_TPM_DAA_JoinInit_internal(TSS_HTPM			hTPM,
			       TSS_HDAA_ISSUER_KEY	hIssuerKey
			       UINT32			daa_counter,
			       UINT32			issuerAuthPKsLength,
			       TSS_HKEY*		issuerAuthPKs,
			       UINT32			issuerAuthPKSignaturesLength,
			       UINT32			issuerAuthPKSignaturesLength2,
			       BYTE**			issuerAuthPKSignatures,
			       UINT32*			capitalUprimeLength,
			       BYTE**			capitalUprime,
			       TSS_DAA_IDENTITY_PROOF** identity_proof,
			       UINT32*			joinSessionLength,
			       BYTE**			joinSession)
{
	// Optional: verification of the PKDAA and issuer settings (authen. by the RSA Public Key chain)
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;
	TPM_AUTH ownerAuth;
	int length, length1;
	UINT32 i, modulus_length, outputSize, buf_len;
	BYTE *outputData, *issuer_settings_bytes;
	BYTE *modulus, stage, *buf;
	//TPM_DAA_ISSUER *issuer_settings;
	//char buffer[1000], buffer1[1000];
	//TSS_DAA_PK_internal *pk_internal = e_2_i_TSS_DAA_PK(issuer_pk);
	bi_ptr issuerAuthPK = NULL;
	TPM_HANDLE daaHandle;
	Trspi_HashCtx hashCtx;
	TPM_DIGEST digest;
	TSS_DAA_IDENTITY_PROOF daaIdentityProof;

	if ((result = obj_tpm_is_connected(hTPM, &tspContext)))
		return result;

	if ((result = obj_daaissuerkey_get_daa_handle(hIssuerKey, &daaHandle)))
		return result;

	stage = 0;
	inputSize0 = (UINT32)sizeof(UINT32);
	inputData0 = issuerAuthPKsLength;
	inputSize1 = 0;
	inputData1 = NULL;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_DAA_Join);
	result |= Trspi_Hash_BYTE(&hashCtx, stage);
	result |= Trspi_Hash_UINT32(&hashCtx, inputSize0);
	result |= Trspi_HashUpdate(&hashCtx, inputSize0, inputData0);
	result |= Trspi_Hash_UINT32(&hashCtx, inputSize1);
	result |= Trspi_HashUpdate(&hashCtx, inputSize1, inputData1);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	// stages 0-2 explained in the diagram "Keys of DAA Issuer"
	if ((result = TCS_API(tspContext)->DaaJoin(tspContext, daaHandle, stage, inputSize1,
						   inputData0, inputSize1, inputData1, &ownerAuth,
						   &outputSize, &outputData)))
		goto close;

	if (outputSize != sizeof(UINT32)) {
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto close;
	}

	// set the sessionHandle to the returned value
	Trspi_UnloadBlob_UINT32(&offset, &daaHandle, outputData);
	free(outputData);
	if ((result = obj_daaissuerkey_set_daa_handle(hIssuerKey, daaHandle)))
		goto close;

	LogDebug("done join 0 settings join_session:%x", daaHandle);

	modulus_length = DAA_PARAM_SIZE_RSA_MODULUS / 8;
	if ((buffer_modulus = malloc(modulus_length)) == NULL) {
		LogError("malloc of %d bytes failed", modulus_length);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}

	stage = 1;
	for (i = 0; i < issuerAuthPKsLength; i++) {
		if ((result = obj_rsakey_get_modulus(issuerAuthPKs[i], &modulus_length, &modulus)))
			goto close;

		outputData = NULL;
		if (i==0) {
			result = Tcsip_TPM_DAA_Join_encapsulate(tspContext, daaHandle, stage,
								modulus_length, modulus, 0, NULL,
								&ownerAuth, &outputSize,
								&outputData);
		} else {
			result = Tcsip_TPM_DAA_Join_encapsulate(tspContext, daaHandle, stage,
								modulus_length, modulus,
								issuerAuthPKSignaturesLength2,
								issuerAuthPKSignatures[i - 1],
								&ownerAuth, &outputSize,
								&outputData);
		}

		free(outputData);
		free_tspi(tspContext, modulus);
		if (result != TSS_SUCCESS) {
			LogDebugFn("Stage 1 iteration %u failed", i);
			goto close;
		}
	}

	LogDebug("done join 1-%d\n", issuer_authentication_PKLengh);

	stage = 2;
	// define issuer_settings
#if 0
	issuer_settings = convert2issuer_settings(pk_internal);
	issuer_settings_bytes = issuer_2_byte_array(issuer_settings, &length);
#else
	if ((result = obj_daaissuerkey_get_daa_issuer(hIssuerKey, &issuer_length, &issuer)))
		goto close;

	if ((result = obj_daaissuerkey_get_modulus(hIssuerKey, &modulus_length, &modulus))) {
		free_tspi(tspContext, issuer);
		goto close;
	}
#endif
	if ((result = Tcsip_TPM_DAA_Join_encapsulate(tspContext, daaHandle, stage, issuer_length,
						     issuer, issuerAuthPKSignaturesLength2,
						     issuerAuthPKSignatures[i - 1], &ownerAuth,
						     &outputSize, &outputData))) {
		free_tspi(tspContext, issuer);
		free_tspi(tspContext, modulus);
		goto close;
	}
	free_tspi(tspContext, issuer);

	LogDebug("done join 2\n");

	stage = 3;
	if ((result = Tcsip_TPM_DAA_Join_encapsulate(tspContext, daaHandle, stage, sizeof(UINT32),
						     (BYTE *)(&daa_counter), 0, NULL, &ownerAuth,
						     &outputSize, &outputData)))
		goto close;

	LogDebug("done join 3\n");

	stage = 4;
#if 0
	// reserved another buffer for storing Big Integer
	bi_2_nbin1( &length, buffer, pk_internal->capitalR0);
	bi_2_nbin1( &length1, buffer1, pk_internal->modulus);
#else
	if ((result = obj_daaissuerkey_get_capitalR0(hIssuerKey, &buf_len, &buf)))
		return result;
#endif
	if ((result = Tcsip_TPM_DAA_Join_encapsulate(tspContext, daaHandle, stage, buf_len, buf,
						     modulus_length, modulus, &ownerAuth,
						     &outputSize, &outputData))) {
		free_tspi(tspContext, buf);
		free_tspi(tspContext, modulus);
		goto close;
	}
	free_tspi(tspContext, buf);

	LogDebug("done join 4\n");

	stage = 5;
#if 0
	bi_2_nbin1( &length, buffer, pk_internal->capitalR1);
#else
	if ((result = obj_daaissuerkey_get_capitalR1(hIssuerKey, &buf_len, &buf)))
		return result;
#endif
	if ((result = Tcsip_TPM_DAA_Join_encapsulate(tspContext, daaHandle, stage, buf_len,
						     buf, modulus_length, modulus, &ownerAuth,
						     &outputSize, &outputData))) {
		free_tspi(tspContext, buf);
		free_tspi(tspContext, modulus);
		goto close;
	}
	free_tspi(tspContext, buf);

	LogDebug("done join 5\n");

	stage = 6;
#if 0
	bi_2_nbin1( &length, buffer, pk_internal->capitalS);
#else
	if ((result = obj_daaissuerkey_get_capitalS(hIssuerKey, &buf_len, &buf)))
		return result;
#endif
	if ((result = Tcsip_TPM_DAA_Join_encapsulate(tspContext, daaHandle, stage, buf_len, buf,
						     modulus_length, modulus, &ownerAuth,
						     &outputSize, &outputData))) {
		free_tspi(tspContext, buf);
		free_tspi(tspContext, modulus);
		goto close;
	}
	free_tspi(tspContext, buf);

	LogDebug("done join 6\n");

	stage = 7;
#if 0
	bi_2_nbin1( &length, buffer, pk_internal->capitalSprime);
#else
	if ((result = obj_daaissuerkey_get_capitalSprime(hIssuerKey, &buf_len, &buf)))
		return result;
#endif
	// define Uprime
	if ((result = Tcsip_TPM_DAA_Join_encapsulate(tspContext, daaHandle, stage, buf_len, buf,
						     modulus_length, modulus, &ownerAuth,
						     &outputSize, &outputData))) {
		free_tspi(tspContext, buf);
		free_tspi(tspContext, modulus);
		goto close;
	}
	free_tspi(tspContext, buf);

#if 0
	// 5 : save PKDAA, U, daaCount and sessionHandle in joinSession
	join_session->issuerPk = (TSS_HKEY)issuer_pk;
	if( result == TSS_SUCCESS) {
		*capital_UprimeLength = outputSize;
		*capital_Uprime = convert_alloc( tspContext, outputSize, outputData);
		if (*capital_Uprime == NULL) {
			LogError("malloc of %d bytes failed", outputSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		join_session->capitalUPrime = copy_alloc( tspContext,
							*capital_UprimeLength,
							*capital_Uprime);
		if (join_session->capitalUPrime == NULL) {
			LogError("malloc of %d bytes failed", *capital_UprimeLength);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		join_session->capitalUPrimeLength = *capital_UprimeLength;
	}
	join_session->sessionHandle = return_join_session;
	// get the endorsement Key (public part)
	result = get_public_EK(
		hTPM,	// in
		&( identity_proof->endorsementLength),
		&( identity_proof->endorsementCredential)
	);
#else
	/* fill out the identity proof struct */
	if ((result = TCS_API(obj->tspContext)->GetTPMCapability(obj->tspContext, TPM_CAP_VERSION,
								 0, NULL, &buf_len, &buf)))
		goto close;

	offset = 0;
	Trspi_UnloadBlob_VERSION(&offset, buf, &daaIdentityProof.versionInfo);
	free_tspi(tspContext, buf);

#error set all 3 credentials in the daaIdentityProof struct here

	/* set the U data */
	if ((result = __tspi_add_mem_entry(tspContext, outputData)))
		goto close;
	*capitalUPrime = outputData;
	*capitalUPrimeLength = outputSize;

	/* return the TSS specific stuff */
#endif

close:
	FREE_BI( issuer_authentication_PK_i);
	LogDebug("result = %d", result);
	LogDebug("outputSize=%d", outputSize);
	LogDebug("outputData=%s", dump_byte_array( outputSize, outputData));
  	return result;
}
#endif


/* allocation:
	endorsementKey as BYTE *
*/
TSS_RESULT
get_public_EK(TSS_HTPM hTPM,
		UINT32 *endorsementKeyLength,
		BYTE **endorsementKey

) {
	TSS_RESULT result;
	TSS_HKEY hEk;
	TSS_HPOLICY hTpmPolicy;
	UINT32 uiAttrSize;
	BYTE *pAttr;

	if( (result = obj_tpm_get_policy( hTPM, &hTpmPolicy)) != TSS_SUCCESS) {
		LogError("can not retrieve policy from the TPM handler");
		goto out_close;
	}

	if( (result = Tspi_TPM_GetPubEndorsementKey(hTPM, TRUE, NULL, &hEk)) != TSS_SUCCESS) {
		LogError("can not retrieve the Public endorsed Key");
		goto out_close;
	}

	result = Tspi_GetAttribData(
		hEk,
		TSS_TSPATTRIB_KEY_INFO,
		TSS_TSPATTRIB_KEYINFO_VERSION,
		&uiAttrSize,
		&pAttr);

	if (result != TSS_SUCCESS) goto out_close;
	LogDebug("keyinfo:%s", dump_byte_array( uiAttrSize, pAttr));

	result = Tspi_GetAttribData(
		hEk,
		TSS_TSPATTRIB_KEY_BLOB,
		TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY,
		endorsementKeyLength,
		endorsementKey);

	LogDebug("Public Endorsement Key:%s",
		dump_byte_array( *endorsementKeyLength, *endorsementKey));
out_close:
      return result;
}

// from TSS.java (479)
TSS_RESULT
compute_join_challenge_host(TSS_HDAA hDAA,
						TSS_DAA_PK_internal *pk_internal,
						bi_ptr capitalU,
						bi_ptr capital_Uprime,
						bi_ptr capital_utilde,
						bi_ptr capital_utilde_prime,
						bi_ptr capital_ni,
						bi_ptr capital_ni_tilde,
						UINT32 commitments_proofLength,
						TSS_DAA_ATTRIB_COMMIT_internal *
						commitments_proof,
						UINT32 nonceIssuerLength,
						BYTE* nonceIssuer,
						UINT32 *resultLength,
						BYTE **result) {
	EVP_MD_CTX mdctx;
	BYTE *encoded_pk = NULL, *buffer;
	UINT32 encoded_pkLength;
	int rv, length;

	buffer = (BYTE *)malloc( 10000); // to be sure, and it will be free quickly
	if (buffer == NULL) {
		LogError("malloc of %d bytes failed", 10000);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	// EVP_MD_CTX_init(&mdctx);
	rv = EVP_DigestInit(&mdctx, DAA_PARAM_get_message_digest());
	if (rv != EVP_SUCCESS) goto err;
	// allocation
	encoded_pk = encoded_DAA_PK_internal( &encoded_pkLength, pk_internal);
	LogDebug("encoded issuerPk[%d]: %s",
		encoded_pkLength,
		dump_byte_array( encoded_pkLength, encoded_pk));
	rv = EVP_DigestUpdate(&mdctx,  encoded_pk, encoded_pkLength);
	if (rv != EVP_SUCCESS) goto err;
	// capitalU
	length = DAA_PARAM_SIZE_RSA_MODULUS / 8;
	bi_2_byte_array( buffer, length, capitalU);
	LogDebug("capitalU[%ld]: %s",
		bi_nbin_size(capitalU) ,
		dump_byte_array( length, buffer));
	rv = EVP_DigestUpdate(&mdctx,  buffer, length);
	if (rv != EVP_SUCCESS) goto err;
	// capital UPrime
	bi_2_byte_array( buffer, length, capital_Uprime);
	LogDebug("capitalUPrime[%d]: %s",
		length,
		dump_byte_array( length, buffer));
	rv = EVP_DigestUpdate(&mdctx,  buffer, length);
	if (rv != EVP_SUCCESS) goto err;
	// capital Utilde
	bi_2_byte_array( buffer, length, capital_utilde);
	LogDebug("capitalUTilde[%d]: %s",
		length,
		dump_byte_array( length, buffer));
	rv = EVP_DigestUpdate(&mdctx,  buffer, length);
	if (rv != EVP_SUCCESS) goto err;
	// capital UtildePrime
	bi_2_byte_array( buffer, length, capital_utilde_prime);
	LogDebug("capital_utilde_prime[%d]: %s",
		length,
		dump_byte_array( length, buffer));
	rv = EVP_DigestUpdate(&mdctx,  buffer, length);
	if (rv != EVP_SUCCESS) goto err;
	//capital_ni
	length = DAA_PARAM_SIZE_MODULUS_GAMMA / 8;
	bi_2_byte_array( buffer, length, capital_ni);
	LogDebug("capital_ni[%d]: %s",
		length,
		dump_byte_array( length, buffer));
	rv = EVP_DigestUpdate(&mdctx,  buffer, length);
	if (rv != EVP_SUCCESS) goto err;
	//capital_ni_tilde
	bi_2_byte_array( buffer, length, capital_ni_tilde);
	LogDebug("capital_ni_tilde[%d]: %s",
		length,
		dump_byte_array( length, buffer));
	rv = EVP_DigestUpdate(&mdctx,  buffer, length);
	if (rv != EVP_SUCCESS) goto err;
	// TODO: commitments
	LogDebug("nonceIssuer[%d]: %s",
		nonceIssuerLength,
		dump_byte_array( nonceIssuerLength, nonceIssuer));
	rv = EVP_DigestUpdate(&mdctx,  nonceIssuer, nonceIssuerLength);
	if (rv != EVP_SUCCESS) goto err;
	*resultLength = EVP_MD_CTX_size(&mdctx);
	*result = (BYTE *)malloc( *resultLength);
	if (*result == NULL) {
		LogError("malloc of %d bytes failed", *resultLength);
		free( buffer);
		free( encoded_pk);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	rv = EVP_DigestFinal(&mdctx, *result, NULL);
	if (rv != EVP_SUCCESS) goto err;
	free( buffer);
	free( encoded_pk);
	return TSS_SUCCESS;
err:
	free( buffer);
	free( encoded_pk);
	DEBUG_print_openssl_errors();
	return TSPERR(TSS_E_INTERNAL_ERROR);
}

/*
This is the second out of 3 functions to execute in order to receive a DAA Credential. It
computes the credential request for the DAA Issuer, which also includes the Platforms & DAA
public key and the attributes that were chosen by the Platform, and which are not visible to \
the DAA Issuer. The Platform can commit to the attribute values it has chosen.
Code influenced by TSS.java (TssDaaCredentialRequest)
*/
TSPICALL
Tspi_TPM_DAA_JoinCreateDaaPubKey_internal(
	TSS_HDAA hDAA,	// in
	TSS_HTPM hTPM,	// in
	UINT32 authenticationChallengeLength,	// in
	BYTE* authenticationChallenge,	// in
	UINT32 nonceIssuerLength,	// in
	BYTE* nonceIssuer,	// in
	UINT32 attributesPlatformLength,	// in
	BYTE** attributesPlatform,	// in
	TSS_DAA_JOIN_SESSION* joinSession,	// in, out
	TSS_DAA_CREDENTIAL_REQUEST* credentialRequest	// out
) {
	TSS_RESULT result;
	TCS_CONTEXT_HANDLE tcsContext;
	TPM_AUTH ownerAuth;
	bi_ptr tmp1 = bi_new_ptr();
	bi_ptr tmp2 = bi_new_ptr();
	bi_ptr capital_utilde = bi_new_ptr();
	bi_ptr v_tilde_prime = bi_new_ptr();
	bi_ptr rv_tilde_prime = bi_new_ptr();
	bi_ptr capitalU = bi_new_ptr();
	bi_ptr product_attributes = bi_new_ptr();
	bi_ptr capital_ni = NULL;
	bi_ptr capital_utilde_prime = NULL;
	bi_ptr capital_ni_tilde = NULL;
	bi_ptr n = NULL;
	bi_ptr attributePlatform = NULL;
	bi_ptr c = NULL;
	bi_ptr zeta = NULL;
	bi_ptr capital_Uprime = NULL;
	bi_ptr sv_tilde_prime = NULL;
	bi_ptr s_f0 = NULL;
	bi_ptr s_f1 = NULL;
	bi_ptr sv_prime = NULL;
	bi_ptr sv_prime1 = NULL;
	bi_ptr sv_prime2 = NULL;
	bi_array_ptr ra = NULL;
	bi_array_ptr sa = NULL;
	TSS_DAA_PK* pk_extern = (TSS_DAA_PK *)joinSession->issuerPk;
	TSS_DAA_PK_internal* pk_internal = e_2_i_TSS_DAA_PK( pk_extern);
	UINT32 i, outputSize, authentication_proofLength, nonce_tpmLength;
	UINT32 capitalSprime_byte_arrayLength, size_bits, length, chLength, c_byteLength;
	UINT32 internal_cbyteLength, noncePlatformLength;
	BYTE *outputData, *authentication_proof, *capitalSprime_byte_array = NULL, *buffer;
	BYTE *ch = NULL;
	BYTE *c_byte, *noncePlatform, *nonce_tpm;
	BYTE *internal_cbyte = NULL;
	EVP_MD_CTX mdctx;

	if( tmp1 == NULL || tmp2 == NULL || capital_utilde == NULL ||
		v_tilde_prime == NULL || rv_tilde_prime == NULL ||
		capitalU == NULL || product_attributes == NULL) {
		LogError("malloc of bi(s) failed");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	if( pk_internal == NULL) {
		LogError("malloc of pk_internal failed");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	if( (result = obj_tpm_is_connected(  hTPM, &tcsContext)) != TSS_SUCCESS) {
		 goto close;
	}
	obj_daa_set_handle_tpm( hDAA, hTPM);
	// allocation
	n = bi_set_as_nbin( pk_extern->modulusLength, pk_extern->modulus);
	if( n == NULL) {
		LogError("malloc of %d bytes failed", pk_extern->modulusLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	// allocation
	capitalSprime_byte_array = bi_2_nbin( &capitalSprime_byte_arrayLength,
								pk_internal->capitalSprime);
	if( capitalSprime_byte_array == NULL) {
		LogError("malloc of %d bytes failed", capitalSprime_byte_arrayLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	// compute second part of the credential request
	// encode plateform attributes (the one visible only by the receiver)
	bi_set( product_attributes, bi_1);
	for( i=0; i<attributesPlatformLength; i++) {
		attributePlatform = bi_set_as_nbin( DAA_PARAM_SIZE_F_I / 8,
									attributesPlatform[i]); // allocation
		if( attributePlatform == NULL) {
			LogError("malloc of bi <%s> failed", "attributePlatform");
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		// bi_tmp1 = ( capitalRReceiver[i] ^ attributesPlatform ) % n
		bi_mod_exp( tmp1, pk_internal->capitalRReceiver->array[i], attributePlatform, n);
		// bi_tmp1 = bi_tmp1 * product_attributes
		bi_mul( tmp1, tmp1, product_attributes);
		// product_attributes = bi_tmp1 % n
		bi_mod( product_attributes, tmp1, n);
		bi_free_ptr( attributePlatform);
	}
	bi_urandom( v_tilde_prime, DAA_PARAM_SIZE_RSA_MODULUS +
							DAA_PARAM_SAFETY_MARGIN);
	// tmp1 = capitalUPrime * capitalS
	bi_free_ptr( tmp1);
	tmp1 = bi_set_as_nbin( joinSession->capitalUPrimeLength,
						joinSession->capitalUPrime); // allocation
	if( tmp1 == NULL) {
		LogError("malloc of %d bytes failed", joinSession->capitalUPrimeLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	// U = ( U' * ( ( pk->S ^ v~' ) % n) ) % n
	// tmp2 = ( pk->S ^ v~') % n
	bi_mod_exp( tmp2, pk_internal->capitalS, v_tilde_prime, n);
	// U = tmp1( U') * tmp2
	bi_mul( capitalU, tmp1, tmp2);
	bi_mod( capitalU, capitalU, n);
	// U = ( U * product_attributes ) % n
	bi_mul( capitalU, capitalU, product_attributes);
	bi_mod( capitalU, capitalU, n);
	// 2  : call the TPM to compute authentication proof with U'
	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		8,
		authenticationChallengeLength, authenticationChallenge,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 8");
	authentication_proof = calloc_tspi( tcsContext, outputSize);
	if( authentication_proof == NULL) {
		LogError("malloc of %d bytes failed", outputSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	memcpy( authentication_proof, outputData, outputSize);
	free( outputData);
	authentication_proofLength = outputSize;

	// 3 : call the TPM to compute U' (first part of correctness proof of the credential
	// request
	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		9,
		pk_extern->capitalR0Length, pk_extern->capitalR0,
		pk_extern->modulusLength, pk_extern->modulus,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 9: capitalR0");
	free( outputData);

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		10,
		pk_extern->capitalR1Length, pk_extern->capitalR1,
		pk_extern->modulusLength, pk_extern->modulus,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 10: capitalR1");
	free( outputData);

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		11,
		pk_extern->capitalSLength, pk_extern->capitalS,
		pk_extern->modulusLength, pk_extern->modulus,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 11: capitalS");
	free( outputData);

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		12,
		capitalSprime_byte_arrayLength, capitalSprime_byte_array,
		pk_extern->modulusLength, pk_extern->modulus,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 12: capitalUTildePrime");
	capital_utilde_prime = bi_set_as_nbin( outputSize, outputData); // allocation
	if( capital_utilde_prime == NULL) {
		LogError("malloc of %d bytes failed", outputSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);
	// 4 compute pseudonym with respect to the DAA Issuer
	// allocation
	zeta = compute_zeta( pk_internal->issuerBaseNameLength,
					pk_internal->issuerBaseName,
					pk_internal);
	if( zeta == NULL) {
		LogError("malloc of bi <%s> failed", "zeta");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	buffer = (BYTE *)malloc( TPM_DAA_SIZE_w);
	if( buffer == NULL) {
		LogError("malloc of %d bytes failed", TPM_DAA_SIZE_w);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	LogDebug("zeta[%ld] = %s", bi_nbin_size( zeta), bi_2_hex_char( zeta));
	bi_2_byte_array( buffer, TPM_DAA_SIZE_w, zeta);
	LogDebug("zeta[%d] = %s", TPM_DAA_SIZE_w, dump_byte_array( TPM_DAA_SIZE_w, buffer));

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		13,
		pk_extern->capitalGammaLength, pk_extern->capitalGamma,
		TPM_DAA_SIZE_w, buffer, // zeta
		&ownerAuth, &outputSize, &outputData);
	free( buffer);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 13: capitalGamma / zeta");
	free( outputData);

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		14,
		pk_extern->capitalGammaLength, pk_extern->capitalGamma,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 14: capitalGamma");
	capital_ni = bi_set_as_nbin( outputSize, outputData); // allocation
	if( capital_ni == NULL) {
		LogError("malloc of bi <%s> failed", "capital_ni");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		15,
		pk_extern->capitalGammaLength, pk_extern->capitalGamma,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 15: capitalGamma");
	capital_ni_tilde = bi_set_as_nbin( outputSize, outputData); // allocation
	if( capital_ni_tilde == NULL) {
		LogError("malloc of bi <%s> failed", "capital_ni_tilde");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);

	// 5 : compute the second part of the correctness proof of the credential request
	// (with attributes not visible to issuer)
	// randomize/blind attributesReceiver
	size_bits = DAA_PARAM_SIZE_RANDOMIZED_ATTRIBUTES;
	bi_set( product_attributes, bi_1);
	ra = (bi_array_ptr)malloc( sizeof( struct _bi_array));
	if( ra == NULL) {
		LogError("malloc of %d bytes failed", sizeof( struct _bi_array));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_new_array( ra, attributesPlatformLength);
	if( ra->array == NULL) {
		LogError("malloc of bi_array <%s> failed", "ra");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	for( i=0; i < attributesPlatformLength; i++) {
		bi_urandom( ra->array[i], size_bits);
		LogDebug("ra[i]=%s size=%d", bi_2_hex_char( ra->array[i]), size_bits);
		// product_attributes=(((capitalYplatform ^ ra[i]) % n)*<product_attributes-1>)%n
		bi_mod_exp( tmp1, pk_internal->capitalRReceiver->array[i], ra->array[i], n);
		bi_mul( tmp1, tmp1, product_attributes);
		bi_mod( product_attributes, tmp1, n);
	}
	size_bits = DAA_PARAM_SIZE_F_I+2*DAA_PARAM_SAFETY_MARGIN+DAA_PARAM_SIZE_MESSAGE_DIGEST;
	bi_urandom( rv_tilde_prime, size_bits);
	// capital_utilde = ( capitalS ^ rv_tilde_prime) % n
	bi_mod_exp( capital_utilde, pk_internal->capitalS, rv_tilde_prime, n);
	// capital_utilde = capital_utilde * product_attributes
	bi_mul( capital_utilde, capital_utilde, product_attributes);
	// capital_utilde = capital_utilde % n
	bi_mod( capital_utilde, capital_utilde, n);
	// 5e
	capital_Uprime = bi_set_as_nbin( joinSession->capitalUPrimeLength,
					joinSession->capitalUPrime); // allocation
	if( capital_Uprime == NULL) {
		LogError("malloc of bi <%s> failed", "capital_Uprime");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	LogDebug("calculation UTilde:                capitalS:%s\n", bi_2_hex_char( pk_internal->capitalS));
	LogDebug("calculation UTilde:       rv_tilde_prime:%s\n", bi_2_hex_char( rv_tilde_prime));
	LogDebug("calculation UTilde:                          n:%s\n", bi_2_hex_char( n));
	LogDebug("calculation UTilde: product_attributes:%s\n", bi_2_hex_char( product_attributes));
	LogDebug("calculation NItilde:                     ntilde:%s\n", bi_2_hex_char( capital_ni_tilde));

	result = compute_join_challenge_host(
		hDAA,
		pk_internal,
		capitalU,
		capital_Uprime,
		capital_utilde,
		capital_utilde_prime,
		capital_ni,
		capital_ni_tilde,
		0,		// TODO commitmentProofLength
		NULL,	// TODO commitment
		nonceIssuerLength,
		nonceIssuer,
		&chLength, // out
		&ch // out allocation
	);
	if( result != TSS_SUCCESS) goto close;

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		16,
		chLength, ch,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	nonce_tpm = outputData;
	nonce_tpmLength = outputSize;
	LogDebug("Done Join 16: compute_join_challenge_host return nonce_tpm:%s",
		dump_byte_array(nonce_tpmLength, nonce_tpm));

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		17,
		0, NULL,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	s_f0 = bi_set_as_nbin( outputSize, outputData); // allocation
	if( s_f0 == NULL) {
		LogError("malloc of <bi> %s failed", "s_f0");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	LogDebug("Done Join 17: return sF0:%s", dump_byte_array(outputSize, outputData) );
	free( outputData);

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		18,
		0, NULL,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	s_f1 = bi_set_as_nbin( outputSize, outputData); // allocation
	if( s_f1 == NULL) {
		LogError("malloc of <bi> %s failed", "s_f1");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	LogDebug("Done Join 18: return sF1:%s", dump_byte_array(outputSize, outputData) );
	free( outputData);

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		19,
		0, NULL,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 19: return sv_prime1");
	sv_prime1 = bi_set_as_nbin( outputSize, outputData); // allocation
	if( sv_prime1 == NULL) {
		LogError("malloc of <bi> %s failed", "sv_prime1");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		20,
		0, NULL,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 20: return cByte");
	c_byte = (BYTE *)calloc_tspi( tcsContext, outputSize);
	if( c_byte == NULL) {
		LogError("malloc of %d bytes failed", outputSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	memcpy( c_byte, outputData, outputSize);
	free( outputData);
	c_byteLength = outputSize;
	c = bi_set_as_nbin( c_byteLength, c_byte); // allocation
	if( c == NULL) {
		LogError("malloc of <bi> %s failed", "c");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}

	// verify computation of c by TPM
	EVP_DigestInit(&mdctx, DAA_PARAM_get_message_digest());
	EVP_DigestUpdate(&mdctx,  ch, chLength);
	EVP_DigestUpdate(&mdctx,  nonce_tpm, nonce_tpmLength);
	nonce_tpm = convert_alloc( tcsContext, nonce_tpmLength, nonce_tpm); // allocation
	if( nonce_tpm == NULL) {
		LogError("malloc of %d bytes failed", nonce_tpmLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	internal_cbyteLength = EVP_MD_CTX_size(&mdctx);
	internal_cbyte = (BYTE *)malloc( internal_cbyteLength);
	if( internal_cbyte == NULL) {
		LogError("malloc of %d bytes failed", internal_cbyteLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	EVP_DigestFinal(&mdctx, internal_cbyte, NULL);
	if( c_byteLength != internal_cbyteLength ||
		memcmp( c_byte, internal_cbyte, c_byteLength) != 0) {
		LogError( "Computation of c in TPM DAA Join command is incorrect. Affected stages: 16,20\n");
		LogError( "\t            c_byte[%d] %s",
				c_byteLength,
				dump_byte_array( c_byteLength, c_byte));
		LogError( "\tc_internal_byte[%d] %s",
				internal_cbyteLength,
				dump_byte_array( internal_cbyteLength, internal_cbyte));
		result = TSS_E_INTERNAL_ERROR;
		goto close;
	}

	// 5m) blind attributesReceiver
	sa = (bi_array_ptr)malloc( sizeof( struct _bi_array));
	if( sa == NULL) {
		LogError("malloc of %d bytes failed", sizeof( struct _bi_array));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_new_array( sa, attributesPlatformLength);
	for( i=0; i < attributesPlatformLength; i++) {
		attributePlatform = bi_set_as_nbin( DAA_PARAM_SIZE_F_I / 8,
								attributesPlatform[i]); // allocation
		if( attributePlatform == NULL) {
			LogError("malloc of <bi> %s failed", "attributePlatform");
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		LogDebug("calculating sa[%d]: raLength=%ld cLength=%ld attributesPlatformLength=%ld\n",
				i, bi_nbin_size( ra->array[i]), bi_nbin_size( c), bi_nbin_size( attributePlatform));
		bi_add( sa->array[i], ra->array[i], bi_mul( tmp1, c, attributePlatform));
		bi_free_ptr( attributePlatform);
	}
	attributePlatform = NULL;
	// 5o) Commitments
	// TODO

	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		21,
		0, NULL,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 21: return sv_prime2");
	sv_prime2 = bi_set_as_nbin( outputSize, outputData); // allocation
	if( sv_prime2 == NULL) {
		LogError("malloc of <bi> %s failed", "sv_prime2");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);
	sv_prime = bi_new_ptr();
	if( sv_prime == NULL) {
		LogError("malloc of <bi> %s failed", "sv_prime");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	// sv_prime = sv_prime2 << DAA_PARAM_SIZE_SPLIT_EXPONENT
	bi_shift_left( sv_prime, sv_prime2, DAA_PARAM_SIZE_SPLIT_EXPONENT);
	// sv_prime = sv_prime + sv_prime1
	bi_add( sv_prime, sv_prime, sv_prime1);

	sv_tilde_prime = bi_new_ptr();
	// tmp1 = c * v_tilde_prime
	bi_mul( tmp1, c, v_tilde_prime);
	// sv_tilde_prime = rv_tilde_prime + tmp1
	bi_add( sv_tilde_prime, rv_tilde_prime, tmp1);

	// step 6) - choose nonce
	bi_urandom( tmp1, DAA_PARAM_SAFETY_MARGIN * 8);
	noncePlatform = (BYTE *)calloc_tspi( tcsContext, DAA_PARAM_SAFETY_MARGIN);
	if( noncePlatform == NULL) {
		LogError("malloc of <bi> %s failed", "noncePlatform");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &noncePlatformLength, noncePlatform, tmp1);

	LogDebug("challenge:%s", dump_byte_array( c_byteLength, c_byte));
	LogDebug("sF0 [%ld]:%s", bi_length( s_f0), bi_2_hex_char( s_f0));
	LogDebug("sF1 [%ld]:%s", bi_length( s_f1), bi_2_hex_char( s_f1));
	LogDebug("sv_prime [%ld]:%s", bi_length( sv_prime), bi_2_hex_char( sv_prime));
	LogDebug("sv_tilde_prime [%ld]:%s", bi_length( sv_tilde_prime), bi_2_hex_char( sv_tilde_prime));
	// update joinSession
	joinSession->capitalU = calloc_tspi( tcsContext, bi_nbin_size( capitalU));
	if( joinSession->capitalU == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( capitalU));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(joinSession->capitalULength),
			joinSession->capitalU,
			capitalU);
	joinSession->attributesPlatformLength = attributesPlatformLength;
	joinSession->attributesPlatform = calloc_tspi( tcsContext, sizeof(BYTE *));
	for( i=0; i<joinSession->attributesPlatformLength; i++) {
		joinSession->attributesPlatform[i] =
				calloc_tspi( tcsContext,DAA_PARAM_SIZE_F_I / 8);
		memcpy( joinSession->attributesPlatform[i],
				attributesPlatform[i],
				DAA_PARAM_SIZE_F_I / 8);
	}
	joinSession->noncePlatform = noncePlatform;
	joinSession->noncePlatformLength = noncePlatformLength;
	joinSession->vTildePrime = calloc_tspi( tcsContext, bi_nbin_size( v_tilde_prime));
	if( joinSession->vTildePrime == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( v_tilde_prime));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(joinSession->vTildePrimeLength),
			joinSession->vTildePrime,
			v_tilde_prime);
	// update credentialRequest
	credentialRequest->capitalU = calloc_tspi( tcsContext, bi_nbin_size( capitalU));
	if( credentialRequest->capitalU == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( capitalU));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(credentialRequest->capitalULength),
			credentialRequest->capitalU,
			capitalU);
	credentialRequest->capitalNi = calloc_tspi( tcsContext, bi_nbin_size( capital_ni));
	if( credentialRequest->capitalNi == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( capital_ni));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(credentialRequest->capitalNiLength),
			credentialRequest->capitalNi,
			capital_ni);
	credentialRequest->authenticationProofLength = authentication_proofLength;
	credentialRequest->authenticationProof = authentication_proof;
	credentialRequest->challenge = c_byte;
	credentialRequest->challengeLength = c_byteLength;
	credentialRequest->nonceTpm = nonce_tpm;
	credentialRequest->nonceTpmLength = nonce_tpmLength;
	credentialRequest->noncePlatform = noncePlatform;
	credentialRequest->noncePlatformLength = DAA_PARAM_SAFETY_MARGIN;
	credentialRequest->sF0 = calloc_tspi( tcsContext, bi_nbin_size( s_f0));
	if( credentialRequest->sF0 == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( s_f0));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(credentialRequest->sF0Length), credentialRequest->sF0, s_f0);
	credentialRequest->sF1 = calloc_tspi( tcsContext, bi_nbin_size( s_f1));
	if( credentialRequest->sF1 == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( s_f1));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(credentialRequest->sF1Length), credentialRequest->sF1, s_f1);
	credentialRequest->sVprime = calloc_tspi( tcsContext, bi_nbin_size( sv_prime));
	if( credentialRequest->sVprime == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( sv_prime));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(credentialRequest->sVprimeLength),
			credentialRequest->sVprime,
			sv_prime);
	credentialRequest->sVtildePrime = calloc_tspi( tcsContext, bi_nbin_size( sv_tilde_prime));
	if( credentialRequest->sVtildePrime == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( sv_tilde_prime));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(credentialRequest->sVtildePrimeLength),
			credentialRequest->sVtildePrime,
			sv_tilde_prime);
	length = (DAA_PARAM_SIZE_RANDOMIZED_ATTRIBUTES + 7) / 8;
	LogDebug("SA length=%d", sa->length);
	credentialRequest->sA = calloc_tspi( tcsContext, sizeof( BYTE *) * sa->length);
	if( credentialRequest->sA == NULL) {
		LogError("malloc of %d bytes failed", sizeof( BYTE *) * sa->length);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}

	for( i=0; i<(UINT32)sa->length; i++) {
		LogDebug("sa[%d].size=%d", i, (int)bi_nbin_size( sa->array[i]));
		credentialRequest->sA[i] = calloc_tspi( tcsContext, length);
		if( credentialRequest->sA[i] == NULL) {
			LogError("malloc of %d bytes failed", length);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		// size used only as repository
		bi_2_byte_array( credentialRequest->sA[i], length, sa->array[i]);
	}
	credentialRequest->sALength = sa->length;
close:
	if( capitalSprime_byte_array!=NULL) free( capitalSprime_byte_array);
	if( ch!=NULL) free( ch);
	if( internal_cbyte != NULL) free( internal_cbyte);
	bi_free_ptr( rv_tilde_prime);
	bi_free_ptr( v_tilde_prime);
	bi_free_ptr( capital_utilde);
	bi_free_ptr( tmp1);
	bi_free_ptr( tmp2);
	bi_free_ptr( capitalU);
	if( ra != NULL) {
		bi_free_array( ra);
		free( ra);
	}
	if( sa != NULL) {
		bi_free_array( sa);
		free( sa);
	}
	FREE_BI( capital_ni);
	FREE_BI( capital_utilde_prime);
	FREE_BI( capital_ni_tilde);
	FREE_BI( n);
	FREE_BI( attributePlatform);
	FREE_BI( c);
	FREE_BI( zeta);
	FREE_BI( capital_Uprime);
	FREE_BI( sv_tilde_prime);
	FREE_BI( s_f0);
	FREE_BI( s_f1);
	FREE_BI( sv_prime);
	FREE_BI( sv_prime1);
	FREE_BI( sv_prime2);
	FREE_BI( product_attributes);
	free_TSS_DAA_PK_internal( pk_internal);
	return result;
}

/*
Code influenced by TSS.java (joinStoreCredential)
*/
TSPICALL Tspi_TPM_DAA_JoinStoreCredential_internal
(
	TSS_HDAA	hDAA,	// in
	TSS_HTPM	hTPM,	// in
	TSS_DAA_CRED_ISSUER	credIssuer,	// in
	TSS_DAA_JOIN_SESSION	joinSession,	// in
	TSS_HKEY*	hDaaCredential	// out
) {
	TCS_CONTEXT_HANDLE tcsContext;
	TPM_AUTH ownerAuth;
	bi_ptr tmp1 = bi_new_ptr();
	bi_ptr tmp2 = bi_new_ptr();
	bi_ptr n = NULL;
	bi_ptr e = NULL;
	bi_ptr fraction_A = NULL;
	bi_ptr v_prime_prime = NULL;
	bi_ptr capital_U = NULL;
	bi_ptr product_attributes = NULL;
	bi_ptr capital_Atilde = NULL;
	bi_ptr s_e = NULL;
	bi_ptr c_prime = NULL;
	bi_ptr capital_A = NULL;
	bi_ptr product = NULL;
	bi_ptr v_tilde_prime = NULL;
	bi_ptr v_prime_prime0 = NULL;
	bi_ptr v_prime_prime1 = NULL;
	TSS_DAA_PK *daa_pk_extern;
	TSS_DAA_PK_internal *pk_intern = NULL;
	TSS_DAA_CREDENTIAL *daaCredential;
	bi_array_ptr attributes_issuer;
	TSS_RESULT result = TSS_SUCCESS;
	UINT32 i;
	UINT32  c_byteLength, v0Length, v1Length, tpm_specificLength;
 	BYTE *c_byte = NULL;
	BYTE *v0 = NULL;
	BYTE *v1 = NULL;
	BYTE *tpm_specific = NULL;

	if( tmp1 == NULL || tmp2 == NULL) {
		LogError("malloc of bi(s) failed");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	if( (result = obj_tpm_is_connected(  hTPM, &tcsContext)) != TSS_SUCCESS) return result;
	obj_daa_set_handle_tpm( hDAA, hTPM);

	LogDebug("Converting issuer public");
	daa_pk_extern = (TSS_DAA_PK *)joinSession.issuerPk;
	pk_intern = e_2_i_TSS_DAA_PK( daa_pk_extern);
	if( pk_intern == NULL) {
		LogError("malloc of pk_intern failed");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	n = bi_new_ptr();
	if( n == NULL) {
		LogError("malloc of bi <%s> failed", "n");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_set( n, pk_intern->modulus);
	attributes_issuer = (bi_array_ptr)malloc( sizeof( struct _bi_array));
	if( attributes_issuer == NULL) {
		LogError("malloc of %d bytes failed", sizeof( struct _bi_array));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_new_array( attributes_issuer, credIssuer.attributesIssuerLength);
	if( attributes_issuer->array == NULL) {
		LogError("malloc of bi_array <%s> failed", "attributes_issuer->array");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	for( i=0; i < credIssuer.attributesIssuerLength; i++) {
		// allocation
		attributes_issuer->array[i] = bi_set_as_nbin( DAA_PARAM_SIZE_F_I / 8,
								credIssuer.attributesIssuer[i]);
		if( attributes_issuer->array[i] == NULL) {
			LogError("malloc of bi <attributes_issuer->array[%d]> failed", i);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
	}
	LogDebug("verify credential of issuer ( part 1)");
	e = bi_set_as_nbin( credIssuer.eLength, credIssuer.e); // allocation
	if( e == NULL) {
		LogError("malloc of bi <%s> failed", "e");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_set( tmp1, bi_0);
	bi_setbit( tmp1, DAA_PARAM_SIZE_EXPONENT_CERTIFICATE - 1);
	bi_set( tmp2, bi_0);
	bi_setbit( tmp1, DAA_PARAM_SIZE_INTERVAL_EXPONENT_CERTIFICATE - 1);
	bi_add( tmp1, tmp1, tmp2);
	if( bi_is_probable_prime( e) == 0 ||
		bi_length(e) < DAA_PARAM_SIZE_EXPONENT_CERTIFICATE ||
		bi_cmp( e, tmp1) > 0) {
			LogError("Verification e failed - Step 1.a");
			LogError("\tPrime(e):%d", bi_is_probable_prime( e));
			LogError("\tbit_length(e):%ld", bi_length(e));
			LogError("\te > (2^(l_e) + 2^(l_prime_e)):%d", bi_cmp( e, tmp1));
			result = TSS_E_DAA_CREDENTIAL_PROOF_ERROR;
			goto close;
	}
	LogDebug("verify credential of issuer (part 2) with proof");
	fraction_A = bi_new_ptr();
	if( fraction_A == NULL) {
		LogError("malloc of bi <%s> failed", "fraction_A");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	v_prime_prime = bi_set_as_nbin( credIssuer.vPrimePrimeLength,
						credIssuer.vPrimePrime); // allocation
	if( v_prime_prime == NULL) {
		LogError("malloc of bi <%s> failed", "v_prime_prime");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	capital_U = bi_set_as_nbin( joinSession.capitalULength,
						joinSession.capitalU); // allocation
	if( capital_U == NULL) {
		LogError("malloc of bi <%s> failed", "capital_U");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_mod_exp( fraction_A, pk_intern->capitalS, v_prime_prime, n);
	bi_mul( fraction_A, fraction_A, capital_U);
	bi_mod( fraction_A, fraction_A, n);
	LogDebug("encode attributes");
	product_attributes = bi_new_ptr();
	bi_set( product_attributes, bi_1);
	for( i=0; i<(UINT32)attributes_issuer->length; i++) {
		bi_mod_exp( tmp1, pk_intern->capitalRIssuer->array[i],
					attributes_issuer->array[i], n);
		bi_mul( product_attributes, tmp1, product_attributes);
		bi_mod( product_attributes, product_attributes, n);
	}
	bi_mul( fraction_A, fraction_A, product_attributes);
	bi_mod( fraction_A, fraction_A, n);
	capital_Atilde = bi_new_ptr();
	if( capital_Atilde == NULL) {
		LogError("malloc of bi <%s> failed", "capital_Atilde");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_invert_mod( capital_Atilde, fraction_A, n);
	bi_mul( capital_Atilde, capital_Atilde, pk_intern->capitalZ);
	bi_mod( capital_Atilde, capital_Atilde, n);
	s_e = bi_set_as_nbin( credIssuer.sELength, credIssuer.sE); // allocation
	if( s_e == NULL) {
		LogError("malloc of bi <%s> failed", "s_e");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_mod_exp( capital_Atilde, capital_Atilde, s_e, n);
	c_prime = bi_set_as_nbin( credIssuer.cPrimeLength, credIssuer.cPrime); // allocation
	if( c_prime == NULL) {
		LogError("malloc of bi <%s> failed", "c_prime");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	capital_A = bi_set_as_nbin( credIssuer.capitalALength, credIssuer.capitalA); // allocation
	if( capital_A == NULL) {
		LogError("malloc of bi <%s> failed", "capital_A");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_mod_exp( tmp1, capital_A, c_prime, n);
	bi_mul( capital_Atilde, capital_Atilde, tmp1);
	bi_mod( capital_Atilde, capital_Atilde, n);

	result = compute_join_challenge_issuer( pk_intern,
						v_prime_prime,
						capital_A,
						capital_Atilde,
						joinSession.noncePlatformLength,
						joinSession.noncePlatform,
						&c_byteLength,
						&c_byte); // out allocation
	if( result != TSS_SUCCESS) goto close;
	if( credIssuer.cPrimeLength != c_byteLength ||
		memcmp( credIssuer.cPrime, c_byte, c_byteLength)!=0) {
		LogError("Verification of c failed - Step 1.c.i");
		LogError("credentialRequest.cPrime[%d]=%s",
			credIssuer.cPrimeLength,
			dump_byte_array( credIssuer.cPrimeLength, credIssuer.cPrime) );
		LogError("challenge[%d]=%s",
			c_byteLength,
			dump_byte_array( c_byteLength, c_byte) );
		result = TSS_E_DAA_CREDENTIAL_PROOF_ERROR;
		goto close;
	}
	product = bi_new_ptr();
	if( product == NULL) {
		LogError("malloc of bi <%s> failed", "product");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_mod_exp( product, capital_A, e, n);
	bi_mul( product, product, fraction_A);
	bi_mod( product, product, n);
	if( bi_equals( pk_intern->capitalZ, product) == 0) {
		LogError("Verification of A failed - Step 1.c.ii");
		LogError("\tcapitalZ=%s", bi_2_hex_char( pk_intern->capitalZ));
		LogError("\tproduct=%s", bi_2_hex_char( product));
		result = TSS_E_DAA_CREDENTIAL_PROOF_ERROR;
		goto close;
	}
	v_tilde_prime = bi_set_as_nbin( joinSession.vTildePrimeLength,
					joinSession.vTildePrime); // allocation
	bi_add( v_prime_prime, v_prime_prime, v_tilde_prime);
	bi_shift_left( tmp1, bi_1, DAA_PARAM_SIZE_SPLIT_EXPONENT);
	v_prime_prime0 = bi_new_ptr();
	if( v_prime_prime0 == NULL) {
		LogError("malloc of bi <%s> failed", "v_prime_prime0");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_mod( v_prime_prime0, v_prime_prime, tmp1);
	v_prime_prime1 = bi_new_ptr();
	if( v_prime_prime1 == NULL) {
		LogError("malloc of bi <%s> failed", "v_prime_prime1");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_shift_right( v_prime_prime1, v_prime_prime, DAA_PARAM_SIZE_SPLIT_EXPONENT);
	free( c_byte);
	c_byte = (BYTE *)malloc( TPM_DAA_SIZE_v0);
	if( c_byte == NULL) {
		LogError("malloc of %d bytes failed", TPM_DAA_SIZE_v0);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_byte_array( c_byte, TPM_DAA_SIZE_v0, v_prime_prime0);
	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		22,
		TPM_DAA_SIZE_v0, c_byte,
		0, NULL,
		&ownerAuth, &v0Length, &v0);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 22: return v0");
	free( c_byte);
	c_byte = (BYTE *)malloc( TPM_DAA_SIZE_v1);
	if( c_byte == NULL) {
		LogError("malloc of %d bytes failed", TPM_DAA_SIZE_v1);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_byte_array( c_byte, TPM_DAA_SIZE_v1, v_prime_prime1);
	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		23,
		TPM_DAA_SIZE_v1, c_byte,
		0, NULL,
		&ownerAuth, &v1Length, &v1);
	if( result != TSS_SUCCESS) goto close;
	LogDebug("Done Join 23: return v1");
	result = Tcsip_TPM_DAA_Join_encapsulate( tcsContext, hDAA,
		24,
		0, NULL,
		0, NULL,
		&ownerAuth, &tpm_specificLength, &tpm_specific);
	if( result != TSS_SUCCESS) goto close;

	daaCredential = (TSS_DAA_CREDENTIAL *)hDaaCredential;
	daaCredential->capitalA = calloc_tspi( tcsContext, bi_nbin_size( capital_A));
	if( daaCredential->capitalA == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( capital_A));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(daaCredential->capitalALength), daaCredential->capitalA, capital_A);
	daaCredential->exponent = calloc_tspi( tcsContext, bi_nbin_size( e));
	if( daaCredential->exponent == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( e));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(daaCredential->exponentLength), daaCredential->exponent, e);
	daaCredential->vBar0 = calloc_tspi( tcsContext, v0Length);
	if( daaCredential->vBar0 == NULL) {
		LogError("malloc of %d bytes failed", v0Length);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	daaCredential->vBar0Length = v0Length;
	memcpy( daaCredential->vBar0, v0, v0Length);
	LogDebug("vBar0[%d]=%s",
			daaCredential->vBar0Length,
			dump_byte_array( daaCredential->vBar0Length, daaCredential->vBar0));
	daaCredential->vBar1 = calloc_tspi( tcsContext, v1Length);
	if( daaCredential->vBar1 == NULL) {
		LogError("malloc of %d bytes failed", v1Length);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	daaCredential->vBar1Length = v1Length;
	memcpy( daaCredential->vBar1, v1, v1Length);
	LogDebug("vBar1[%d]=%s",
		daaCredential->vBar1Length,
		dump_byte_array( daaCredential->vBar1Length, daaCredential->vBar1));
	//TODO remove
	LogDebug("[BUSS] joinSession.attributesPlatformLength=%d",
			joinSession.attributesPlatformLength);
	LogDebug("[BUSS]       credIssuer.attributesIssuerLength=%d",
			credIssuer.attributesIssuerLength);
	daaCredential->attributesLength = joinSession.attributesPlatformLength +
		credIssuer.attributesIssuerLength;
	daaCredential->attributes = (BYTE **)calloc_tspi( tcsContext,
						sizeof(BYTE *) * daaCredential->attributesLength);
	if( daaCredential->attributes == NULL) {
		LogError("malloc of %d bytes failed",
			sizeof(BYTE *) * daaCredential->attributesLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	for( i=0; i < joinSession.attributesPlatformLength; i++) {
		daaCredential->attributes[i] = calloc_tspi( tcsContext, DAA_PARAM_SIZE_F_I / 8);
		if( daaCredential->attributes[i] == NULL) {
			LogError("malloc of %d bytes failed", DAA_PARAM_SIZE_F_I / 8);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		LogDebug("allocation attributes[%d]=%lx", i, (long)daaCredential->attributes[i]);
		memcpy( daaCredential->attributes[i],
				joinSession.attributesPlatform[i],
				DAA_PARAM_SIZE_F_I / 8);
	}
	for( i=0; i < credIssuer.attributesIssuerLength; i++) {
		daaCredential->attributes[i+joinSession.attributesPlatformLength] =
			calloc_tspi( tcsContext, DAA_PARAM_SIZE_F_I / 8);
		if( daaCredential->attributes[i+joinSession.attributesPlatformLength] == NULL) {
			LogError("malloc of %d bytes failed", DAA_PARAM_SIZE_F_I / 8);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		memcpy( daaCredential->attributes[i+joinSession.attributesPlatformLength],
			credIssuer.attributesIssuer[i], DAA_PARAM_SIZE_F_I / 8);
	}
	memcpy( &(daaCredential->issuerPK), daa_pk_extern, sizeof( TSS_DAA_PK));
	daaCredential->tpmSpecificEnc = calloc_tspi( tcsContext, tpm_specificLength);
	if( daaCredential->tpmSpecificEnc == NULL) {
		LogError("malloc of %d bytes failed", tpm_specificLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	daaCredential->tpmSpecificEncLength = tpm_specificLength;
	memcpy( daaCredential->tpmSpecificEnc, tpm_specific, tpm_specificLength);
	// TODO store in TSS this TSS_DAA_CREDENTIAL_REQUEST
	*hDaaCredential = (TSS_HKEY)daaCredential;
close:
	if( v0 != NULL) free( v0);
	if( v1 != NULL) free( v1);
	if( tpm_specific != NULL) free( tpm_specific);
	if( c_byte != NULL) free( c_byte);
	bi_free_ptr( tmp2);
	bi_free_ptr( tmp1);
	if( attributes_issuer != NULL) {
		bi_free_array( attributes_issuer);
		free( attributes_issuer);
	}
	FREE_BI( v_prime_prime1);
	FREE_BI( v_prime_prime0);
	FREE_BI( v_tilde_prime);
	FREE_BI( product);
	FREE_BI( capital_A);
	FREE_BI( c_prime);
	FREE_BI( s_e);
	FREE_BI( capital_Atilde);
	FREE_BI( product_attributes);
	FREE_BI( capital_U);
	FREE_BI( v_prime_prime);
	FREE_BI( fraction_A);
	FREE_BI( e);
	FREE_BI( n);
	if( pk_intern!=NULL) free_TSS_DAA_PK_internal( pk_intern);
	return result;
}

static void add_splitet( bi_ptr result, bi_ptr a, bi_ptr b) {
	bi_shift_left( result, bi_1, DAA_PARAM_SIZE_SPLIT_EXPONENT);
	bi_mul( result, result, b);
	bi_add( result, result, a);
}

/* code influenced by TSS.java (signStep) */
TSS_RESULT Tspi_TPM_DAA_Sign_internal
(
	TSS_HDAA	hDAA,	// in
	TSS_HTPM	hTPM,	// in
	TSS_HKEY	hDaaCredential,	// in
	TSS_DAA_SELECTED_ATTRIB	revealAttributes,	// in
	UINT32	verifierBaseNameLength,	// in
	BYTE*	verifierBaseName,	// in
	UINT32	verifierNonceLength,	// in
	BYTE*	verifierNonce,	// in
	TSS_DAA_SIGN_DATA	signData,	// in
	TSS_DAA_SIGNATURE*	daaSignature	// out
)
{
	TCS_CONTEXT_HANDLE tcsContext;
	TSS_DAA_CREDENTIAL *daaCredential;
	TPM_DAA_ISSUER *tpm_daa_issuer;
	TPM_AUTH ownerAuth;
	TSS_RESULT result = TSS_SUCCESS;
	TSS_DAA_PK *pk;
	TSS_DAA_PK_internal *pk_intern;
	int i;
	bi_ptr tmp1 = bi_new_ptr(), tmp2;
	bi_ptr n = NULL;
	bi_ptr capital_gamma = NULL;
	bi_ptr gamma = NULL;
	bi_ptr zeta = NULL;
	bi_ptr r = NULL;
	bi_ptr t_tilde_T = NULL;
	bi_ptr capital_Nv = NULL;
	bi_ptr capital_N_tilde_v = NULL;
	bi_ptr w = NULL;
	bi_ptr capital_T = NULL;
	bi_ptr r_E = NULL;
	bi_ptr r_V = NULL;
	bi_ptr capital_T_tilde = NULL;
	bi_ptr capital_A = NULL;
	bi_array_ptr capital_R;
	bi_ptr product_R = NULL;
	bi_array_ptr r_A = NULL;
	bi_array_ptr s_A = NULL;
	bi_ptr c = NULL;
	bi_ptr sF0 = NULL;
	bi_ptr sF1 = NULL;
	bi_ptr sV1 = NULL;
	bi_ptr sV2 = NULL;
	bi_ptr e = NULL;
	bi_ptr s_E = NULL;
	bi_ptr s_V = NULL;
	CS_ENCRYPTION_RESULT_RANDOMNESS *encryption_result_rand = NULL;
	BYTE *issuer_settings = NULL, *outputData, byte;
	BYTE *buffer = NULL, *ch = NULL, *nonce_tpm = NULL, *c_bytes = NULL;
	UINT32 issuer_settingsLength, outputSize, length, size_bits, chLength;
	UINT32 nonce_tpmLength;
	UINT32 c_bytesLength, return_sign_session;
	// for anonymity revocation
	CS_ENCRYPTION_RESULT *encryption_result = NULL;
	CS_ENCRYPTION_RESULT *encryption_result_tilde = NULL;
	TSS_DAA_PSEUDONYM *signature_pseudonym;
	TSS_DAA_PSEUDONYM_PLAIN *pseudonym_plain = NULL;
	TSS_DAA_PSEUDONYM_PLAIN *pseudonym_plain_tilde = NULL;
	TSS_DAA_ATTRIB_COMMIT *signed_commitments;

	if( (result = obj_tpm_is_connected(  hTPM, &tcsContext)) != TSS_SUCCESS)
		return result;
	if( tmp1 == NULL) {
		LogError("malloc of bi <%s> failed", "tmp1");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	obj_daa_set_handle_tpm( hDAA, hTPM);
	// TODO retrieve the daaCredential from the persistence storage
	daaCredential = (TSS_DAA_CREDENTIAL *)hDaaCredential;
	pk = (TSS_DAA_PK *)&(daaCredential->issuerPK);
	pk_intern = e_2_i_TSS_DAA_PK( pk);
	if( pk_intern == NULL) {
		LogError("malloc of <%s> failed", "pk_intern");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	tpm_daa_issuer = convert2issuer_settings( pk_intern);
	if( daaCredential->attributesLength == 0 ||
		daaCredential->attributesLength != revealAttributes.indicesListLength) {
		LogDebug("Problem with the reveal attribs: attributes length:%d reveal length:%d",
			daaCredential->attributesLength, revealAttributes.indicesListLength);
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	if( verifierNonce == NULL ||
		verifierNonceLength != DAA_PARAM_LENGTH_MESSAGE_DIGEST) {
		LogDebug("Problem with the nonce verifier: nonce verifier length:%d",
			verifierNonceLength);
		result = TSS_E_BAD_PARAMETER;
		goto close;
	}
	n = bi_new_ptr();
	if( n == NULL) {
		LogError("malloc of bi <%s> failed", "n");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_set( n, pk_intern->modulus);
	capital_gamma = bi_new_ptr();
	if( capital_gamma == NULL) {
		LogError("malloc of bi <%s> failed", "capital_gamma");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_set( capital_gamma, pk_intern->capitalGamma);
	gamma = bi_new_ptr();
	if( gamma == NULL) {
		LogError("malloc of bi <%s> failed", "gamma");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_set( gamma, pk_intern->gamma);
	if( verifierBaseNameLength == 0 || verifierBaseName == NULL) {
		r = bi_new_ptr();
		compute_random_number( r, capital_gamma);
		zeta = project_into_group_gamma( r, pk_intern); // allocation
		if( zeta == NULL) {
			LogError("malloc of bi <%s> failed", "zeta");
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
	} else {
		zeta = compute_zeta( verifierBaseNameLength, verifierBaseName, pk_intern);
		if( zeta == NULL) {
			LogError("malloc of bi <%s> failed", "zeta");
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
	}
	issuer_settings = issuer_2_byte_array( tpm_daa_issuer,
						&issuer_settingsLength); // allocation
	if( issuer_settings == NULL) {
		LogError("malloc of %d bytes failed", issuer_settingsLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	LogDebug( "Issuer Settings:[%s]",
		dump_byte_array(issuer_settingsLength, issuer_settings) );
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		0,
		issuer_settingsLength, issuer_settings,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	// set the sessionHandle to the returned value
	return_sign_session = ntohl( *((UINT32 *)outputData));
	obj_daa_set_session_handle( hDAA, return_sign_session);
	free( outputData);
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		1,
		daaCredential->tpmSpecificEncLength, daaCredential->tpmSpecificEnc,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	free( outputData);
	LogDebug( "done Sign 1 - TPM specific");
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		2,
		pk->capitalR0Length, pk->capitalR0,
		pk->modulusLength, pk->modulus,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	free( outputData);
	LogDebug( "done Sign 2 - capitalR0");
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		3,
		pk->capitalR1Length, pk->capitalR1,
		pk->modulusLength, pk->modulus,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	free( outputData);
	LogDebug( "done Sign 3 - capitalR1");
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		4,
		pk->capitalSLength, pk->capitalS,
		pk->modulusLength, pk->modulus,
		&ownerAuth, &outputSize, &outputData);
	if( outputSize > 0 && outputData != NULL) free( outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug( "done Sign 4 - capitalS");
	buffer = bi_2_nbin( &length, pk_intern->capitalSprime); // allocation
	if( buffer == NULL) {
		LogError("malloc of %d bytes failed", length);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		5,
		length, buffer,
		pk->modulusLength, pk->modulus,
		&ownerAuth, &outputSize, &outputData);
	free( buffer);
	if( result != TSS_SUCCESS) goto close;
	LogDebug( "done Sign 5 - capitalSPrime. Return t_tilde_T");
	t_tilde_T = bi_set_as_nbin( outputSize, outputData); // allocation
	if( t_tilde_T == NULL) {
		LogError("malloc of bi <%s> failed", "t_tilde_T");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);
	// first precomputation until here possible (verifier independent)
	length = TPM_DAA_SIZE_w;
	buffer = (BYTE *)malloc( length);
	if( buffer == NULL) {
		LogError("malloc of %d bytes failed", length);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_byte_array( buffer, length, zeta);
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		6,
		pk->capitalGammaLength, pk->capitalGamma,
		length, buffer,
		&ownerAuth, &outputSize, &outputData);
	free( buffer);
	if( result != TSS_SUCCESS) goto close;
	LogDebug( "done Sign 6 - capitalGamma & zeta");
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		7,
		pk->capitalGammaLength, pk->capitalGamma,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug( "done Sign 7 - capitalGamma. Return capital_Nv");
	capital_Nv = bi_set_as_nbin( outputSize, outputData); // allocation
	if( capital_Nv == NULL) {
		LogError("malloc of bi <%s> failed", "capital_Nv");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);
	// TODO Step 6 a.b - anonymity revocation

	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		8,
		pk->capitalGammaLength, pk->capitalGamma,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug( "done Sign 8 - capitalGamma. Return capital_N_tilde_v");
	capital_N_tilde_v = bi_set_as_nbin( outputSize, outputData); // allocation
	if( capital_N_tilde_v == NULL) {
		LogError("malloc of bi <%s> failed", "capital_N_tilde_v");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);
	// TODO  Step 6 c,d - anonymity revocation

	// Second precomputation until here possible (verifier dependent)
	size_bits = DAA_PARAM_SIZE_RSA_MODULUS + DAA_PARAM_SAFETY_MARGIN;
	w = bi_new_ptr();
	if( w == NULL) {
		LogError("malloc of bi <%s> failed", "w");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_urandom( w, size_bits);
	capital_A = bi_set_as_nbin( daaCredential->capitalALength,
					daaCredential->capitalA); // allocation
	if( capital_A == NULL) {
		LogError("malloc of bi <%s> failed", "capital_A");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	capital_T = bi_new_ptr();
	if( capital_T == NULL) {
		LogError("malloc of bi <%s> failed", "capital_T");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_mod_exp( tmp1, pk_intern->capitalS, w, n);
	bi_mul( capital_T, capital_A, tmp1);
	bi_mod( capital_T, capital_T, n);
	size_bits = DAA_PARAM_SIZE_INTERVAL_EXPONENT_CERTIFICATE +
		DAA_PARAM_SAFETY_MARGIN + DAA_PARAM_SIZE_MESSAGE_DIGEST;
	r_E = bi_new_ptr();
	if( r_E == NULL) {
		LogError("malloc of bi <%s> failed", "r_E");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_urandom( r_E, size_bits);
	size_bits = DAA_PARAM_SIZE_EXPONENT_CERTIFICATE + DAA_PARAM_SIZE_RSA_MODULUS +
		2 * DAA_PARAM_SAFETY_MARGIN + DAA_PARAM_SIZE_MESSAGE_DIGEST + 1;
	r_V = bi_new_ptr();
	if( r_V == NULL) {
		LogError("malloc of bi <%s> failed", "r_V");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_urandom( r_V, size_bits);
	capital_T_tilde = bi_new_ptr();
	if( capital_T_tilde == NULL) {
		LogError("malloc of bi <%s> failed", "capital_T_tilde");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_mod_exp( tmp1, capital_T, r_E, n);
	bi_mul( capital_T_tilde, t_tilde_T, tmp1);
	bi_mod( capital_T_tilde, capital_T_tilde, n);

	bi_mod_exp( tmp1, pk_intern->capitalS, r_V, n);
	bi_mul( capital_T_tilde, capital_T_tilde, tmp1);
	bi_mod( capital_T_tilde, capital_T_tilde, n);

	// attributes extension
	size_bits = DAA_PARAM_SIZE_F_I +
			DAA_PARAM_SAFETY_MARGIN +
			DAA_PARAM_SIZE_MESSAGE_DIGEST;
	capital_R = pk_intern->capitalY;
	r_A = (bi_array_ptr)malloc( sizeof( struct _bi_array));
	if( r_A == NULL) {
		LogError("malloc of %d bytes failed", sizeof( struct _bi_array));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_new_array2( r_A, revealAttributes.indicesListLength);
	product_R = bi_new_ptr();
	if( product_R == NULL) {
		LogError("malloc of bi <%s> failed", "product_R");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_set( product_R, bi_1);
	for( i=0; i<(int)revealAttributes.indicesListLength; i++) {
		if( revealAttributes.indicesList[i] == 0) {
			// only non selected
			r_A->array[i] = bi_new_ptr();
			if( r_A->array[i] == NULL) {
				LogError("malloc of bi <%s> failed", "r_A->array[i]");
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto close;
			}
			bi_urandom( r_A->array[i] , size_bits);
			bi_mod_exp( tmp1, capital_R->array[i], r_A->array[i], n);
			bi_mul( product_R, product_R, tmp1);
			bi_mod( product_R, product_R, n);
		} else r_A->array[i] = NULL;
	}
	bi_mul( capital_T_tilde, capital_T_tilde, product_R);
	bi_mod( capital_T_tilde, capital_T_tilde, n);
	//TODO Step 8 - Commitments
	// compute commitment to attributes not revealed to the verifier

	//TODO Step 9 - callback functions

	// only when revocation not enabled
	pseudonym_plain = (TSS_DAA_PSEUDONYM_PLAIN *)calloc_tspi( tcsContext,
								sizeof(TSS_DAA_PSEUDONYM_PLAIN));
	if( pseudonym_plain == NULL) {
		LogError("malloc of %d bytes failed", sizeof(TSS_DAA_PSEUDONYM_PLAIN));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	init_tss_version( pseudonym_plain);
	pseudonym_plain->capitalNv = calloc_tspi( tcsContext,
						bi_nbin_size( capital_Nv));
	if( pseudonym_plain->capitalNv == NULL) {
		LogError("malloc of bi <%s> failed", "pseudonym_plain->capitalNv");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(pseudonym_plain->capitalNvLength),
				pseudonym_plain->capitalNv,
				capital_Nv);
	pseudonym_plain_tilde = (TSS_DAA_PSEUDONYM_PLAIN *)
		calloc_tspi( tcsContext,sizeof(TSS_DAA_PSEUDONYM_PLAIN));
	if( pseudonym_plain_tilde == NULL) {
		LogError("malloc of %d bytes failed", sizeof(TSS_DAA_PSEUDONYM_PLAIN));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	init_tss_version( pseudonym_plain_tilde);
	pseudonym_plain_tilde->capitalNv = calloc_tspi( tcsContext,
							bi_nbin_size( capital_N_tilde_v));
	if( pseudonym_plain_tilde->capitalNv == NULL) {
		LogError("malloc of bi <%s> failed", "pseudonym_plain_tilde->capitalNv");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(pseudonym_plain_tilde->capitalNvLength),
				pseudonym_plain_tilde->capitalNv,
				capital_N_tilde_v);

	// Step 10 - compute challenge
	ch = compute_sign_challenge_host(
		&chLength,
		DAA_PARAM_get_message_digest(),
		pk_intern,
		verifierNonceLength,
		verifierNonce,
		0,	// int selected_attributes2commitLength,
		NULL,	// TSS_DAA_SELECTED_ATTRIB **selected_attributes2commit,
		0,	// 	int is_anonymity_revocation_enabled,
		zeta,
		capital_T,
		capital_T_tilde,
		0, //	int attribute_commitmentsLength,
		NULL, //	TSS_DAA_ATTRIB_COMMIT_internal **attribute_commitments,
		NULL, //	TSS_DAA_ATTRIB_COMMIT_internal **attribute_commitment_proofs,
		capital_Nv,
		capital_N_tilde_v,
		NULL, // CS_PUBLIC_KEY *anonymity_revocator_pk,
		NULL, // CS_ENCRYPTION_RESULT *encryption_result_rand,
		NULL //CS_ENCRYPTION_RESULT *encryption_result_proof)
	);
	if( ch == NULL) {
		LogError("malloc in compute_sign_challenge_host failed");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	result = Tcsip_TPM_DAA_Sign( tcsContext, hDAA,
			9,
			chLength, ch,
			0, NULL,
			&ownerAuth, &nonce_tpmLength, &nonce_tpm);
	if( result != TSS_SUCCESS) goto close;
	LogDebug( "done Sign 9 - compute sign challenge host. Return nonce_tpm");
	byte = (BYTE)signData.payloadFlag; // 0 -> payload contains a handle to an AIK
					// 1 -> payload contains a hashed message
	result = Tcsip_TPM_DAA_Sign( tcsContext, hDAA,
			10,
			sizeof(BYTE), &byte,
			signData.payloadLength, signData.payload,
			&ownerAuth, &c_bytesLength, &c_bytes);
	LogDebug("calculation of c: ch[%d]%s", chLength, dump_byte_array( chLength, ch));
	LogDebug("calculation of c: nonce_tpm[%d]%s",
			nonce_tpmLength,
			dump_byte_array( nonce_tpmLength, nonce_tpm));
	LogDebug("calculation of c: sign_data.payloadFlag[%d]%x",
		1,
		(int)signData.payloadFlag);
	LogDebug("calculation of c: signdata.payload[%d]%s",
			signData.payloadLength,
			dump_byte_array( signData.payloadLength, signData.payload));

	if( result != TSS_SUCCESS) goto close;
	LogDebug( "done Sign 10 - compute signData.payload.");
	LogDebug(" Return c_bytes[%d]%s",
		c_bytesLength,
		dump_byte_array( c_bytesLength, c_bytes));
	c = bi_set_as_nbin( c_bytesLength, c_bytes); // allocation
	if( c == NULL) {
		LogError("malloc of bi <%s> failed", "c");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		11,
		0, NULL,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS)goto close;
	LogDebug( "done Sign 11. Return sF0");
	sF0 = bi_set_as_nbin( outputSize, outputData); // allocation
	if( sF0 == NULL) {
		LogError("malloc of bi <%s> failed", "sF0");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		12,
		0, NULL,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug( "done Sign 12. Return sF1");
	sF1 = bi_set_as_nbin( outputSize, outputData); // allocation
	if( sF1 == NULL) {
		LogError("malloc of bi <%s> failed", "sF1");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		13,
		daaCredential->vBar0Length,  daaCredential->vBar0,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug( "done Sign 13. Return sV1");
	sV1 = bi_set_as_nbin( outputSize, outputData); // allocation
	if( sV1 == NULL) {
		LogError("malloc of bi <%s> failed", "sV1");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		14,
		daaCredential->vBar0Length,  daaCredential->vBar0,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	free( outputData);
	LogDebug( "done Sign 14.");
	result = Tcsip_TPM_DAA_Sign(
		tcsContext, hDAA,
		15,
		daaCredential->vBar1Length,  daaCredential->vBar1,
		0, NULL,
		&ownerAuth, &outputSize, &outputData);
	if( result != TSS_SUCCESS) goto close;
	LogDebug( "done Sign 15. Return sV2");
	sV2 = bi_set_as_nbin( outputSize, outputData); // allocation
	if( sV2 == NULL) {
		LogError("malloc of bi <%s> failed", "sV2");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	free( outputData);
	// allocation
	e = bi_set_as_nbin( daaCredential->exponentLength, daaCredential->exponent);
	if( e == NULL) {
		LogError("malloc of bi <%s> failed", "e");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	// s_e = r_E + ( c * ( e - ( 1 << (sizeExponentCertificate -1))))
	s_E = bi_new_ptr();
	if( s_E == NULL) {
		LogError("malloc of bi <%s> failed", "s_E");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_shift_left( tmp1, bi_1, DAA_PARAM_SIZE_EXPONENT_CERTIFICATE - 1);
	bi_sub( tmp1, e, tmp1);
	bi_mul( tmp1, c, tmp1);
	bi_add( s_E, r_E, tmp1);
	s_V = bi_new_ptr();
	if( s_V == NULL) {
		LogError("malloc of bi <%s> failed", "s_V");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	add_splitet( s_V, sV1, sV2);
	bi_add( s_V, s_V, r_V);
	bi_mul( tmp1, c, w);
	bi_mul( tmp1, tmp1, e);
	bi_sub( s_V, s_V, tmp1);
	// attributes extension
	// TODO verify the size of each selected attributes
	s_A = (bi_array_ptr)malloc( sizeof( struct _bi_array));
	if( s_A == NULL) {
		LogError("malloc of bi_array <%s> failed", "s_A");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_new_array2( s_A, revealAttributes.indicesListLength);
	if( s_A->array == NULL) {
		LogError("malloc of bi_array <%s> failed", "s_A");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	for( i=0; i<(int)revealAttributes.indicesListLength; i++) {
		if( revealAttributes.indicesList[i] == 0) {
			s_A->array[i] = bi_new_ptr();
			if( s_A->array[i] == NULL) {
				LogError("malloc of bi <%s> failed", "s_A->array[i]");
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto close;
			}
			tmp2 = bi_set_as_nbin( DAA_PARAM_SIZE_F_I / 8,
						daaCredential->attributes[i]); // allocation
			if( tmp2 == NULL) {
				LogError("malloc of %d bytes failed", DAA_PARAM_SIZE_F_I / 8);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto close;
			}
			bi_mul( tmp1, c, tmp2);
			// TODEL
			LogDebug("daaCredential->attributes[i]=%ld", bi_nbin_size( tmp2));
			LogDebug("r_A:%ld", bi_nbin_size( r_A->array[i]));
			LogDebug("c:%ld", bi_nbin_size( c));
			LogDebug("c*daaCredential->attributes[i]=%ld", bi_nbin_size( tmp1));
			// END TODEL
			bi_add( s_A->array[i], r_A->array[i], tmp1);
			bi_free_ptr( tmp2);
		} else s_A->array[i] = NULL;
	}

	// Compose result structure

	// DAASignaturePseudonym TODO: implement anonymity revocation
	// if ( !revocation_enabled)
	signature_pseudonym = pseudonym_plain;

	// populate the signature (TSS_DAA_SIGNATURE)
	daaSignature->zeta = (BYTE *)calloc_tspi( tcsContext, bi_nbin_size( zeta));
	if (daaSignature->zeta == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( zeta));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(daaSignature->zetaLength), daaSignature->zeta, zeta);
	daaSignature->capitalT = (BYTE *)calloc_tspi( tcsContext, bi_nbin_size( capital_T));
	if (daaSignature->capitalT == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( capital_T));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(daaSignature->capitalTLength), daaSignature->capitalT, capital_T);
	daaSignature->challenge = (BYTE *)calloc_tspi( tcsContext, c_bytesLength);
	if (daaSignature->challenge == NULL) {
		LogError("malloc of %d bytes failed", c_bytesLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	daaSignature->challengeLength = c_bytesLength;
	memcpy( daaSignature->challenge, c_bytes, c_bytesLength);
	daaSignature->nonceTpm = (BYTE *)calloc_tspi( tcsContext, nonce_tpmLength);
	if (daaSignature->nonceTpm == NULL) {
		LogError("malloc of %d bytes failed", nonce_tpmLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	daaSignature->nonceTpmLength = nonce_tpmLength;
	memcpy( daaSignature->nonceTpm, nonce_tpm, nonce_tpmLength);
	daaSignature->sV = (BYTE *)calloc_tspi( tcsContext, bi_nbin_size( s_V));
	if (daaSignature->sV == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( s_V));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(daaSignature->sVLength), daaSignature->sV, s_V);
	daaSignature->sF0 = (BYTE *)calloc_tspi( tcsContext, bi_nbin_size( sF0));
	if (daaSignature->sF0 == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( sF0));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(daaSignature->sF0Length), daaSignature->sF0, sF0);
	daaSignature->sF1 = (BYTE *)calloc_tspi( tcsContext, bi_nbin_size( sF1));
	if (daaSignature->sF1 == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( sF1));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(daaSignature->sF1Length), daaSignature->sF1, sF1);
	daaSignature->sE = (BYTE *)calloc_tspi( tcsContext, bi_nbin_size( s_E));
	if (daaSignature->sE == NULL) {
		LogError("malloc of %ld bytes failed", bi_nbin_size( s_E));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	bi_2_nbin1( &(daaSignature->sELength), daaSignature->sE, s_E);
	daaSignature->sALength = revealAttributes.indicesListLength;
	daaSignature->sA = (BYTE **)calloc_tspi(
		tcsContext,
		sizeof(BYTE *)*revealAttributes.indicesListLength);
	if (daaSignature->sA == NULL) {
		LogError("malloc of %d bytes failed",
			sizeof(BYTE *)*revealAttributes.indicesListLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	length = (DAA_PARAM_SIZE_RANDOMIZED_ATTRIBUTES + 7) / 8;
	for( i=0; i<(int)revealAttributes.indicesListLength; i++) {
		daaSignature->sA[i] = calloc_tspi( tcsContext, length);
		if (daaSignature->sA[i] == NULL) {
			LogError("malloc of %d bytes failed", length);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto close;
		}
		if( s_A->array[i] == NULL)
			bi_2_byte_array( daaSignature->sA[i], length, bi_0);
		else {
			bi_2_byte_array( daaSignature->sA[i], length, s_A->array[i]);
			LogDebug("size big_integer s_A[i] = %ld   size daaSignature->sA[i]=%d",
				bi_nbin_size( s_A->array[i]), length);
		}
	}
	daaSignature->attributeCommitmentsLength = 0;
	daaSignature->signedPseudonym = signature_pseudonym;
close:
	bi_free_ptr( tmp1);
	if( c_bytes != NULL) free( c_bytes);
	if( ch != NULL) free( ch);
	if( nonce_tpm != NULL) free( nonce_tpm);
	if( encryption_result !=NULL) free( encryption_result);
	if( encryption_result_tilde !=NULL) free( encryption_result_tilde);
	if( issuer_settings != NULL) free( issuer_settings);
	if( r_A != NULL) {
		for( i=0; i<(int)revealAttributes.indicesListLength; i++)
			if( r_A->array[i] != NULL) bi_free_ptr( r_A->array[i]);
		free( r_A);
	}
	FREE_BI( s_V);
	FREE_BI( s_E);
	FREE_BI( e);
	FREE_BI( sV2);
	FREE_BI( sV1);
        FREE_BI( sF1);
	FREE_BI( sF0);
	FREE_BI( c);
	FREE_BI( product_R);
	FREE_BI( capital_A);
	FREE_BI( capital_T_tilde);
	FREE_BI( r_V);
	FREE_BI( r_E);
	FREE_BI( capital_T);
	FREE_BI( w);
	FREE_BI( capital_N_tilde_v);
	FREE_BI( capital_Nv);
	FREE_BI( t_tilde_T);
	FREE_BI( n);
	FREE_BI( capital_gamma);
	FREE_BI( gamma);
	FREE_BI( zeta);
	FREE_BI( r);
	free_TSS_DAA_PK_internal( pk_intern);
	free_TPM_DAA_ISSUER( tpm_daa_issuer);
	return result;
}
