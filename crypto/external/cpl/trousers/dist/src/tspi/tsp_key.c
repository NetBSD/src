
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */


#include <stdlib.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


void
free_key_refs(TSS_KEY *key)
{
	free(key->algorithmParms.parms);
	key->algorithmParms.parms = NULL;
	key->algorithmParms.parmSize = 0;

	free(key->pubKey.key);
	key->pubKey.key = NULL;
	key->pubKey.keyLength = 0;

	free(key->encData);
	key->encData = NULL;
	key->encSize = 0;

	free(key->PCRInfo);
	key->PCRInfo = NULL;
	key->PCRInfoSize = 0;
}

void
LoadBlob_TSS_KEY(UINT64 *offset, BYTE *blob, TSS_KEY *key)
{
	if (key->hdr.key12.tag == TPM_TAG_KEY12)
		Trspi_LoadBlob_KEY12(offset, blob, (TPM_KEY12 *)key);
	else
		Trspi_LoadBlob_KEY(offset, blob, (TCPA_KEY *)key);
}

TSS_RESULT
UnloadBlob_TSS_KEY(UINT64 *offset, BYTE *blob, TSS_KEY *key)
{
	UINT16 tag;
	UINT64 keyOffset = *offset;
	TSS_RESULT result;

	Trspi_UnloadBlob_UINT16(&keyOffset, &tag, blob);
	if (tag == TPM_TAG_KEY12)
		result = Trspi_UnloadBlob_KEY12(offset, blob, (TPM_KEY12 *)key);
	else
		result = Trspi_UnloadBlob_KEY(offset, blob, (TCPA_KEY *)key);

	return result;
}

TSS_RESULT
Hash_TSS_KEY(Trspi_HashCtx *c, TSS_KEY *key)
{
	TSS_RESULT result;

	if (key->hdr.key12.tag == TPM_TAG_KEY12)
		result = Trspi_Hash_KEY12(c, (TPM_KEY12 *)key);
	else
		result = Trspi_Hash_KEY(c, (TCPA_KEY *)key);

	return result;
}

void
LoadBlob_TSS_PRIVKEY_DIGEST(UINT64 *offset, BYTE *blob, TSS_KEY *key)
{
	if (key->hdr.key12.tag == TPM_TAG_KEY12)
		Trspi_LoadBlob_PRIVKEY_DIGEST12(offset, blob, (TPM_KEY12 *)key);
	else
		Trspi_LoadBlob_PRIVKEY_DIGEST(offset, blob, (TCPA_KEY *)key);
}

TSS_RESULT
Hash_TSS_PRIVKEY_DIGEST(Trspi_HashCtx *c, TSS_KEY *key)
{
	TSS_RESULT result;

	if (key->hdr.key12.tag == TPM_TAG_KEY12)
		result = Trspi_Hash_PRIVKEY_DIGEST12(c, (TPM_KEY12 *)key);
	else
		result = Trspi_Hash_PRIVKEY_DIGEST(c, (TCPA_KEY *)key);

	return result;
}

#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_EvictKey(TSS_HCONTEXT tspContext,
		   TCS_KEY_HANDLE hKey)
{
	TSS_RESULT result;
	UINT32 handlesLen;
	TCS_HANDLE *handles, handle;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(hKey, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = hKey;
	handles = &handle;

	result = obj_context_transport_execute(tspContext, TPM_ORD_EvictKey, 0, NULL, &pubKeyHash,
					       &handlesLen, &handles, NULL, NULL, NULL, NULL);

	return result;
}

TSS_RESULT
Transport_GetPubKey(TSS_HCONTEXT tspContext,
		    TCS_KEY_HANDLE hKey,
		    TPM_AUTH *pAuth,
		    UINT32 *pcPubKeySize,
		    BYTE **prgbPubKey)
{
	TSS_RESULT result;
	UINT32 handlesLen, decLen;
	TCS_HANDLE *handles, handle;
	BYTE *dec = NULL;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(hKey, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = hKey;
	handles = &handle;

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_GetPubKey, 0, NULL,
						    &pubKeyHash, &handlesLen, &handles, pAuth, NULL,
						    &decLen, &dec)))
		return result;

	*prgbPubKey = dec;
	*pcPubKeySize = decLen;

	return result;
}

TSS_RESULT
Transport_CreateWrapKey(TSS_HCONTEXT tspContext,	/* in */
			TCS_KEY_HANDLE hWrappingKey,	/* in */
			TPM_ENCAUTH *KeyUsageAuth,	/* in */
			TPM_ENCAUTH *KeyMigrationAuth,	/* in */
			UINT32 keyInfoSize,		/* in */
			BYTE * keyInfo,			/* in */
			UINT32 * keyDataSize,		/* out */
			BYTE ** keyData,		/* out */
			TPM_AUTH * pAuth)		/* in, out */
{
	TSS_RESULT result;
	UINT32 handlesLen, decLen;
	TCS_HANDLE *handles, handle;
	BYTE *dec = NULL;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	UINT64 offset;
	BYTE *data;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(hWrappingKey, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = hWrappingKey;
	handles = &handle;

	if ((data = malloc(2 * sizeof(TPM_ENCAUTH) + keyInfoSize)) == NULL) {
		LogError("malloc of %zd bytes failed", 2 * sizeof(TPM_ENCAUTH) + keyInfoSize);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob(&offset, sizeof(TPM_ENCAUTH), data, KeyUsageAuth->authdata);
	Trspi_LoadBlob(&offset, sizeof(TPM_ENCAUTH), data, KeyMigrationAuth->authdata);
	Trspi_LoadBlob(&offset, keyInfoSize, data, keyInfo);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_CreateWrapKey,
						    (2 * sizeof(TPM_ENCAUTH) + keyInfoSize), data,
						    &pubKeyHash, &handlesLen, &handles, pAuth, NULL,
						    &decLen, &dec)))
		goto done;

	*keyDataSize = decLen;
	*keyData = dec;
done:
	free(data);

	return result;
}

TSS_RESULT
Transport_LoadKeyByBlob(TSS_HCONTEXT     tspContext,
			TCS_KEY_HANDLE   hParentKey,
			UINT32           ulBlobLength,
			BYTE*            rgbBlobData,
			TPM_AUTH*        pAuth,
			TCS_KEY_HANDLE*  phKey,
			TPM_KEY_HANDLE*  phSlot)
{
	TSS_RESULT result;
	UINT32 handlesLen, decLen;
	TCS_HANDLE *handles, handle;
	BYTE *dec = NULL;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(hParentKey, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = hParentKey;
	handles = &handle;

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_LoadKey2, ulBlobLength,
						    rgbBlobData, &pubKeyHash, &handlesLen,
						    &handles, pAuth, NULL, &decLen, &dec)))
		return result;

	if (handlesLen == 1)
		*phKey = *(TCS_KEY_HANDLE *)handles;
	else
		result = TSPERR(TSS_E_INTERNAL_ERROR);

	free(dec);

	return result;
}

/* This function both encrypts the handle of the pubkey being requested and requires the hash
 * of that pubkey for the transport log when logging is enabled. */
TSS_RESULT
Transport_OwnerReadInternalPub(TSS_HCONTEXT tspContext,   /* in */
			       TCS_KEY_HANDLE hKey,           /* in */
			       TPM_AUTH* pOwnerAuth,          /* in, out */
			       UINT32* punPubKeySize, /* out */
			       BYTE** ppbPubKeyData)          /* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen = 0, decLen;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	BYTE *dec = NULL, data[sizeof(TCS_KEY_HANDLE)];


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(hKey, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, hKey, data);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_OwnerReadInternalPub,
						    sizeof(data), data, &pubKeyHash, &handlesLen,
						    NULL, pOwnerAuth, NULL, &decLen, &dec)))
		return result;

	*punPubKeySize = decLen;
	*ppbPubKeyData = dec;

	return result;
}
#endif

