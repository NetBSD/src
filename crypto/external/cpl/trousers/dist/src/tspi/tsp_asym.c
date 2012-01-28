
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


/* encrypt some data with the RSA public key of 'key', using the padding appropriate for the key */
TSS_RESULT
__tspi_rsa_encrypt(TSS_HKEY key,
	    UINT32   inDataLen,
	    BYTE*    inData,
	    UINT32*  outDataLen,
	    BYTE*    outData)
{
	BYTE *blob;
	UINT32 blobLen;
	UINT64 offset;
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;
	TPM_PUBKEY pubKey;

	if (!inData || !outDataLen || !outData)
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if ((result = obj_rsakey_get_tsp_context(key, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_pub_blob(key, &blobLen, &blob)))
		return result;

	offset = 0;
	if ((result = Trspi_UnloadBlob_PUBKEY(&offset, blob, &pubKey))) {
		free_tspi(tspContext, blob);
		return result;
	}
	free_tspi(tspContext, blob);

	if (pubKey.pubKey.keyLength < inDataLen) {
		result = TSPERR(TSS_E_ENC_INVALID_LENGTH);
		goto done;
	}

	if (pubKey.algorithmParms.encScheme == TPM_ES_RSAESPKCSv15 ||
	    pubKey.algorithmParms.encScheme == TSS_ES_RSAESPKCSV15) {
		if ((result = Trspi_RSA_PKCS15_Encrypt(inData, inDataLen, outData, outDataLen,
						       pubKey.pubKey.key, pubKey.pubKey.keyLength)))
			goto done;
	} else {
		if ((result = Trspi_TPM_RSA_OAEP_Encrypt(inData, inDataLen, outData, outDataLen,
							 pubKey.pubKey.key,
							 pubKey.pubKey.keyLength)))
			goto done;
	}

done:
	free(pubKey.pubKey.key);
	free(pubKey.algorithmParms.parms);
	return result;
}

TSS_RESULT
__tspi_rsa_verify(TSS_HKEY key,
	   UINT32   type,
	   UINT32   hashLen,
	   BYTE*    hash,
	   UINT32   sigLen,
	   BYTE*    sig)
{
	BYTE *blob;
	UINT32 blobLen;
	UINT64 offset;
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;
	TPM_PUBKEY pubKey;

	if (!hash || !sig)
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if ((result = obj_rsakey_get_tsp_context(key, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_pub_blob(key, &blobLen, &blob)))
		return result;

	offset = 0;
	if ((result = Trspi_UnloadBlob_PUBKEY(&offset, blob, &pubKey))) {
		free_tspi(tspContext, blob);
		return result;
	}
	free_tspi(tspContext, blob);

	result = Trspi_Verify(type, hash, hashLen, pubKey.pubKey.key, pubKey.pubKey.keyLength,
			      sig, sigLen);

	free(pubKey.pubKey.key);
	free(pubKey.algorithmParms.parms);

	return result;
}
