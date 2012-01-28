
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
 *
 */


#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_ActivateTPMIdentity(TSS_HCONTEXT tspContext,
			      TCS_KEY_HANDLE idKey,        /* in */
			      UINT32 blobSize,     /* in */
			      BYTE * blob, /* in */
			      TPM_AUTH * idKeyAuth,        /* in, out */
			      TPM_AUTH * ownerAuth,        /* in, out */
			      UINT32 * SymmetricKeySize,   /* out */
			      BYTE ** SymmetricKey)        /* out */
{
	TSS_RESULT result;
	UINT32 handlesLen, decLen;
	TCS_HANDLE *handles, handle;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	BYTE *dec;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(idKey, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = idKey;
	handles = &handle;

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_ActivateIdentity, blobSize,
						    blob, &pubKeyHash, &handlesLen, &handles,
						    idKeyAuth, ownerAuth, &decLen, &dec)))
		return result;

	*SymmetricKeySize = decLen;
	*SymmetricKey = dec;

	return result;
}

TSS_RESULT
Transport_MakeIdentity2(TSS_HCONTEXT tspContext,
			TCPA_ENCAUTH identityAuth, /* in */
			TCPA_CHOSENID_HASH IDLabel_PrivCAHash,     /* in */
			UINT32 idKeyInfoSize,      /* in */
			BYTE * idKeyInfo,  /* in */
			TPM_AUTH * pSrkAuth,       /* in, out */
			TPM_AUTH * pOwnerAuth,     /* in, out */
			UINT32 * idKeySize,        /* out */
			BYTE ** idKey,     /* out */
			UINT32 * pcIdentityBindingSize,    /* out */
			BYTE ** prgbIdentityBinding)       /* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen = 0, decLen, dataLen;
	BYTE *dec, *data;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = sizeof(TCPA_ENCAUTH) + sizeof(TCPA_CHOSENID_HASH) + idKeyInfoSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob(&offset, sizeof(TCPA_ENCAUTH), data, identityAuth.authdata);
	Trspi_LoadBlob(&offset, sizeof(TCPA_CHOSENID_HASH), data, IDLabel_PrivCAHash.digest);
	Trspi_LoadBlob(&offset, idKeyInfoSize, data, idKeyInfo);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_MakeIdentity, dataLen,
						    data, NULL, &handlesLen, NULL, pSrkAuth,
						    pOwnerAuth, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	UnloadBlob_TSS_KEY(&offset, dec, NULL);
	*idKeySize = offset;

	if ((*idKey = malloc(*idKeySize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *idKeySize);
		*idKeySize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_UnloadBlob(&offset, *idKeySize, dec, *idKey);

	Trspi_UnloadBlob_UINT32(&offset, pcIdentityBindingSize, dec);
	if ((*prgbIdentityBinding = malloc(*pcIdentityBindingSize)) == NULL) {
		free(dec);
		free(*idKey);
		*idKey = NULL;
		*idKeySize = 0;
		LogError("malloc of %u bytes failed", *pcIdentityBindingSize);
		*pcIdentityBindingSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *pcIdentityBindingSize, dec, *prgbIdentityBinding);
	free(dec);

	return result;
}
#endif

