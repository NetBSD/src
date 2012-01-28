
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "obj.h"
#include "tsplog.h"
#include "authsess.h"


#ifdef TSS_BUILD_SEALX
TSS_RESULT
sealx_mask_cb(PVOID lpAppData,
	      TSS_HKEY hEncKey,
	      TSS_HENCDATA hEncData,
	      TSS_ALGORITHM_ID algId,
	      UINT32 ulSizeNonces,
	      BYTE *rgbNonceEven,
	      BYTE *rgbNonceOdd,
	      BYTE *rgbNonceEvenOSAP,
	      BYTE *rgbNonceOddOSAP,
	      UINT32 ulDataLength,
	      BYTE *rgbDataToMask,
	      BYTE *rgbMaskedData)
{
	UINT32 mgf1SeedLen, sharedSecretLen = sizeof(TPM_DIGEST);
	BYTE *mgf1Seed, *mgf1Buffer;
	UINT32 i;
	TSS_RESULT result;
	struct authsess *sess = (struct authsess *)lpAppData;

	mgf1SeedLen = (ulSizeNonces * 2) + strlen("XOR") + sharedSecretLen;
	if ((mgf1Seed = (BYTE *)calloc(1, mgf1SeedLen)) == NULL) {
		LogError("malloc of %u bytes failed.", mgf1SeedLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	mgf1Buffer = mgf1Seed;
	memcpy(mgf1Buffer, rgbNonceEven, ulSizeNonces);
	mgf1Buffer += ulSizeNonces;
	memcpy(mgf1Buffer, rgbNonceOdd, ulSizeNonces);
	mgf1Buffer += ulSizeNonces;
	memcpy(mgf1Buffer, "XOR", strlen("XOR"));
	mgf1Buffer += strlen("XOR");
	memcpy(mgf1Buffer, sess->sharedSecret.digest, sharedSecretLen);

	if ((result = Trspi_MGF1(TSS_HASH_SHA1, mgf1SeedLen, mgf1Seed, ulDataLength,
				 rgbMaskedData)))
		goto done;

	for (i = 0; i < ulDataLength; i++)
		rgbMaskedData[i] ^= rgbDataToMask[i];

done:
	free(mgf1Seed);

	return result;
}
#endif

#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_Seal(TSS_HCONTEXT tspContext,    /* in */
	       TCS_KEY_HANDLE keyHandle,   /* in */
	       TCPA_ENCAUTH *encAuth,       /* in */
	       UINT32 pcrInfoSize, /* in */
	       BYTE * PcrInfo,     /* in */
	       UINT32 inDataSize,  /* in */
	       BYTE * inData,      /* in */
	       TPM_AUTH * pubAuth, /* in, out */
	       UINT32 * SealedDataSize,    /* out */
	       BYTE ** SealedData) /* out */
{
	TSS_RESULT result;
	UINT32 handlesLen, decLen, dataLen;
	TCS_HANDLE *handles, handle;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	UINT64 offset;
	BYTE *data, *dec;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(keyHandle, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = keyHandle;
	handles = &handle;

	dataLen = (2 * sizeof(UINT32)) + sizeof(TPM_ENCAUTH) + pcrInfoSize + inDataSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_DIGEST(&offset, data, (TPM_DIGEST *)encAuth);
	Trspi_LoadBlob_UINT32(&offset, pcrInfoSize, data);
	Trspi_LoadBlob(&offset, pcrInfoSize, data, PcrInfo);
	Trspi_LoadBlob_UINT32(&offset, inDataSize, data);
	Trspi_LoadBlob(&offset, inDataSize, data, inData);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_Seal, dataLen, data,
						    &pubKeyHash, &handlesLen, &handles, pubAuth,
						    NULL, &decLen, &dec)))
		return result;

	*SealedDataSize = decLen;
	*SealedData = dec;

	return result;
}

TSS_RESULT
Transport_Sealx(TSS_HCONTEXT tspContext,   /* in */
		TCS_KEY_HANDLE keyHandle,  /* in */
		TCPA_ENCAUTH *encAuth,      /* in */
		UINT32 pcrInfoSize,        /* in */
		BYTE * PcrInfo,    /* in */
		UINT32 inDataSize, /* in */
		BYTE * inData,     /* in */
		TPM_AUTH * pubAuth,        /* in, out */
		UINT32 * SealedDataSize,   /* out */
		BYTE ** SealedData)        /* out */
{
	TSS_RESULT result;
	UINT32 handlesLen, decLen, dataLen;
	TCS_HANDLE *handles, handle;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	UINT64 offset;
	BYTE *data, *dec;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(keyHandle, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = keyHandle;
	handles = &handle;

	dataLen = (2 * sizeof(UINT32)) + sizeof(TPM_ENCAUTH) + pcrInfoSize + inDataSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob(&offset, sizeof(TPM_ENCAUTH), data, encAuth->authdata);
	Trspi_LoadBlob_UINT32(&offset, pcrInfoSize, data);
	Trspi_LoadBlob(&offset, pcrInfoSize, data, PcrInfo);
	Trspi_LoadBlob_UINT32(&offset, inDataSize, data);
	Trspi_LoadBlob(&offset, inDataSize, data, inData);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_Sealx, dataLen, data,
						    &pubKeyHash, &handlesLen, &handles, pubAuth,
						    NULL, &decLen, &dec)))
		return result;

	*SealedDataSize = decLen;
	*SealedData = dec;

	return result;
}

TSS_RESULT
Transport_Unseal(TSS_HCONTEXT tspContext,  /* in */
		 TCS_KEY_HANDLE parentHandle,      /* in */
		 UINT32 SealedDataSize,    /* in */
		 BYTE * SealedData,        /* in */
		 TPM_AUTH * parentAuth,    /* in, out */
		 TPM_AUTH * dataAuth,      /* in, out */
		 UINT32 * DataSize,        /* out */
		 BYTE ** Data)     /* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen, decLen;
	TCS_HANDLE *handles, handle;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	BYTE *dec;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(parentHandle, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = parentHandle;
	handles = &handle;

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_Unseal, SealedDataSize,
						    SealedData, &pubKeyHash, &handlesLen, &handles,
						    parentAuth, dataAuth, &decLen, &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, DataSize, dec);

	if ((*Data = malloc(*DataSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *DataSize);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *DataSize, dec, *Data);

	free(dec);

	return result;
}
#endif
