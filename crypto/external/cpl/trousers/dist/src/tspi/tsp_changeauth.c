
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"
#include "authsess.h"


TSS_RESULT
Trspi_UnloadBlob_STORED_DATA(UINT64 *offset, BYTE *blob, TCPA_STORED_DATA *data)
{
	Trspi_UnloadBlob_TCPA_VERSION(offset, blob, &data->ver);
	Trspi_UnloadBlob_UINT32(offset, &data->sealInfoSize, blob);

	if (data->sealInfoSize > 0) {
		data->sealInfo = malloc(data->sealInfoSize);
		if (data->sealInfo == NULL) {
			LogError("malloc of %d bytes failed.", data->sealInfoSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, data->sealInfoSize, blob, data->sealInfo);
	} else {
		data->sealInfo = NULL;
	}

	Trspi_UnloadBlob_UINT32(offset, &data->encDataSize, blob);

	if (data->encDataSize > 0) {
		data->encData = malloc(data->encDataSize);
		if (data->encData == NULL) {
			LogError("malloc of %d bytes failed.", data->encDataSize);
			free(data->sealInfo);
			data->sealInfo = NULL;
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		Trspi_UnloadBlob(offset, data->encDataSize, blob, data->encData);
	} else {
		data->encData = NULL;
	}

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_STORED_DATA(UINT64 *offset, BYTE *blob, TCPA_STORED_DATA *data)
{
	Trspi_LoadBlob_TCPA_VERSION(offset, blob, data->ver);
	Trspi_LoadBlob_UINT32(offset, data->sealInfoSize, blob);
	Trspi_LoadBlob(offset, data->sealInfoSize, blob, data->sealInfo);
	Trspi_LoadBlob_UINT32(offset, data->encDataSize, blob);
	Trspi_LoadBlob(offset, data->encDataSize, blob, data->encData);
}

TSS_RESULT
changeauth_owner(TSS_HCONTEXT tspContext,
		 TSS_HOBJECT hObjectToChange,
		 TSS_HOBJECT hParentObject,
		 TSS_HPOLICY hNewPolicy)
{
	TPM_DIGEST digest;
	TSS_RESULT result;
	Trspi_HashCtx hashCtx;
	struct authsess *xsap = NULL;

	if ((result = authsess_xsap_init(tspContext, hObjectToChange, hNewPolicy,
					TSS_AUTH_POLICY_REQUIRED, TPM_ORD_ChangeAuthOwner,
					TPM_ET_OWNER, &xsap)))
		return result;

	/* calculate auth data */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuthOwner);
	result |= Trspi_Hash_UINT16(&hashCtx, TCPA_PID_ADCP);
	result |= Trspi_Hash_ENCAUTH(&hashCtx, xsap->encAuthUse.authdata);
	result |= Trspi_Hash_UINT16(&hashCtx, TCPA_ET_OWNER);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	if ((result = authsess_xsap_hmac(xsap, &digest)))
		goto error;

	if ((result = TCS_API(tspContext)->ChangeAuthOwner(tspContext, TCPA_PID_ADCP,
							   &xsap->encAuthUse, TPM_ET_OWNER,
							   xsap->pAuth)))
		goto error;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_SUCCESS);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuthOwner);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	result = authsess_xsap_verify(xsap, &digest);
error:
	authsess_free(xsap);

	return result;
}

TSS_RESULT
changeauth_srk(TSS_HCONTEXT tspContext,
	       TSS_HOBJECT hObjectToChange,
	       TSS_HOBJECT hParentObject,
	       TSS_HPOLICY hNewPolicy)
{
	TPM_DIGEST digest;
	TSS_RESULT result;
	Trspi_HashCtx hashCtx;
	struct authsess *xsap = NULL;


	if ((result = authsess_xsap_init(tspContext, hParentObject, hNewPolicy,
					 TSS_AUTH_POLICY_REQUIRED, TPM_ORD_ChangeAuthOwner,
					 TPM_ET_OWNER, &xsap)))
		return result;

	/* calculate auth data */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuthOwner);
	result |= Trspi_Hash_UINT16(&hashCtx, TCPA_PID_ADCP);
	result |= Trspi_Hash_ENCAUTH(&hashCtx, xsap->encAuthUse.authdata);
	result |= Trspi_Hash_UINT16(&hashCtx, TCPA_ET_SRK);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	if ((result = authsess_xsap_hmac(xsap, &digest)))
		goto error;

	if ((result = TCS_API(tspContext)->ChangeAuthOwner(tspContext, TCPA_PID_ADCP,
							   &xsap->encAuthUse, TPM_ET_SRK,
							   xsap->pAuth)))
		goto error;

	/* Validate the Auths */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_SUCCESS);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuthOwner);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	result = authsess_xsap_verify(xsap, &digest);
error:
	authsess_free(xsap);

	return result;
}

TSS_RESULT
changeauth_encdata(TSS_HCONTEXT tspContext,
		   TSS_HOBJECT hObjectToChange,
		   TSS_HOBJECT hParentObject,
		   TSS_HPOLICY hNewPolicy)
{
	TPM_DIGEST digest;
	TSS_RESULT result;
	Trspi_HashCtx hashCtx;
	TSS_HPOLICY hPolicy;
	TCS_KEY_HANDLE keyHandle;
	UINT64 offset;
	struct authsess *xsap = NULL;
	TPM_STORED_DATA storedData;
	UINT32 dataBlobLength, newEncSize;
	BYTE *dataBlob, *newEncData;
	TPM_AUTH auth2;

	/*  get the secret for the parent */
	if ((result = obj_encdata_get_policy(hObjectToChange, TSS_POLICY_USAGE, &hPolicy)))
		return result;

	/*  get the data Object  */
	if ((result = obj_encdata_get_data(hObjectToChange, &dataBlobLength, &dataBlob)))
		return result;

	offset = 0;
	if ((result = Trspi_UnloadBlob_STORED_DATA(&offset, dataBlob, &storedData)))
		return result;

	if ((result = obj_rsakey_get_tcs_handle(hParentObject, &keyHandle)))
		return result;

	if ((result = authsess_xsap_init(tspContext, hParentObject, hNewPolicy,
					 TSS_AUTH_POLICY_REQUIRED, TPM_ORD_ChangeAuth,
					 TPM_ET_KEYHANDLE, &xsap)))
		return result;

	/* caluculate auth data */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuth);
	result |= Trspi_Hash_UINT16(&hashCtx, TPM_PID_ADCP);
	result |= Trspi_Hash_ENCAUTH(&hashCtx, xsap->encAuthUse.authdata);
	result |= Trspi_Hash_UINT16(&hashCtx, TPM_ET_DATA);
	result |= Trspi_Hash_UINT32(&hashCtx, storedData.encDataSize);
	result |= Trspi_HashUpdate(&hashCtx, storedData.encDataSize, storedData.encData);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	if ((result = authsess_xsap_hmac(xsap, &digest)))
		goto error;

	if ((result = secret_PerformAuth_OIAP(hObjectToChange, TPM_ORD_ChangeAuth,
					hPolicy, FALSE, &digest, &auth2)))
		goto error;

	if ((result = TCS_API(tspContext)->ChangeAuth(tspContext, keyHandle, TPM_PID_ADCP,
						      &xsap->encAuthUse, TPM_ET_DATA,
						      storedData.encDataSize, storedData.encData,
						      xsap->pAuth, &auth2, &newEncSize,
						      &newEncData)))
		goto error;

	/* Validate the Auths */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_SUCCESS);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuth);
	result |= Trspi_Hash_UINT32(&hashCtx, newEncSize);
	result |= Trspi_HashUpdate(&hashCtx, newEncSize, newEncData);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	if ((result = authsess_xsap_verify(xsap, &digest)))
		goto error;

	if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, &auth2)))
		goto error;

	memcpy(storedData.encData, newEncData, newEncSize);
	free(newEncData);
	storedData.encDataSize = newEncSize;

	offset = 0;
	Trspi_LoadBlob_STORED_DATA(&offset, dataBlob, &storedData);

	result = obj_encdata_set_data(hObjectToChange, offset, dataBlob);

error:
	authsess_free(xsap);
	free(storedData.sealInfo);
	free(storedData.encData);

	return result;

}

TSS_RESULT
changeauth_key(TSS_HCONTEXT tspContext,
	       TSS_HOBJECT hObjectToChange,
	       TSS_HOBJECT hParentObject,
	       TSS_HPOLICY hNewPolicy)
{
	TPM_DIGEST digest;
	Trspi_HashCtx hashCtx;
	TSS_RESULT result;
	TSS_KEY keyToChange;
	TCS_KEY_HANDLE keyHandle;
	struct authsess *xsap = NULL;
	UINT32 objectLength;
	TSS_HPOLICY hPolicy;
	BYTE *keyBlob;
	UINT32 newEncSize;
	BYTE *newEncData;
	TPM_AUTH auth2;
	UINT64 offset;


	if ((result = obj_rsakey_get_blob(hObjectToChange, &objectLength, &keyBlob)))
		return result;

	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, keyBlob, &keyToChange))) {
		LogDebug("UnloadBlob_TSS_KEY failed. "
				"result=0x%x", result);
		return result;
	}

	if ((result = obj_rsakey_get_policy(hObjectToChange, TSS_POLICY_USAGE, &hPolicy, NULL)))
		return result;

	if ((result = obj_rsakey_get_tcs_handle(hParentObject, &keyHandle)))
		return result;

	if ((result = authsess_xsap_init(tspContext, hParentObject, hNewPolicy,
					 TSS_AUTH_POLICY_REQUIRED, TPM_ORD_ChangeAuth,
					 keyHandle == TPM_KEYHND_SRK ?
					 TPM_ET_SRK : TPM_ET_KEYHANDLE, &xsap)))
		return result;

	/* caluculate auth data */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuth);
	result |= Trspi_Hash_UINT16(&hashCtx, TCPA_PID_ADCP);
	result |= Trspi_Hash_ENCAUTH(&hashCtx, xsap->encAuthUse.authdata);
	result |= Trspi_Hash_UINT16(&hashCtx, TCPA_ET_KEY);
	result |= Trspi_Hash_UINT32(&hashCtx, keyToChange.encSize);
	result |= Trspi_HashUpdate(&hashCtx, keyToChange.encSize,
			keyToChange.encData);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	if ((result = authsess_xsap_hmac(xsap, &digest)))
		goto error;

	if ((result = secret_PerformAuth_OIAP(hObjectToChange, TPM_ORD_ChangeAuth,
					hPolicy, FALSE, &digest, &auth2)))
		goto error;

	if ((result = TCS_API(tspContext)->ChangeAuth(tspContext, keyHandle, TPM_PID_ADCP,
						      &xsap->encAuthUse, TPM_ET_KEY,
						      keyToChange.encSize, keyToChange.encData,
						      xsap->pAuth, &auth2, &newEncSize,
						      &newEncData)))
		goto error;

	/* Validate the Auths */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuth);
	result |= Trspi_Hash_UINT32(&hashCtx, newEncSize);
	result |= Trspi_HashUpdate(&hashCtx, newEncSize, newEncData);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	if ((result = authsess_xsap_verify(xsap, &digest)))
		goto error;

	if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, &auth2)))
		return result;

	memcpy(keyToChange.encData, newEncData, newEncSize);
	free(newEncData);

	offset = 0;
	LoadBlob_TSS_KEY(&offset, keyBlob, &keyToChange);
	objectLength = offset;

	result = obj_rsakey_set_tcpakey(hObjectToChange, objectLength, keyBlob);
error:
	authsess_free(xsap);

	return result;
}


#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_ChangeAuth(TSS_HCONTEXT tspContext,	/* in */
		     TCS_KEY_HANDLE parentHandle,	/* in */
		     TCPA_PROTOCOL_ID protocolID,	/* in */
		     TCPA_ENCAUTH *newAuth,	/* in */
		     TCPA_ENTITY_TYPE entityType,	/* in */
		     UINT32 encDataSize,	/* in */
		     BYTE * encData,	/* in */
		     TPM_AUTH * ownerAuth,	/* in, out */
		     TPM_AUTH * entityAuth,	/* in, out */
		     UINT32 * outDataSize,	/* out */
		     BYTE ** outData)	/* out */
{
	TSS_RESULT result;
	UINT32 handlesLen, dataLen, decLen;
	TCS_HANDLE *handles, handle;
	BYTE *dec = NULL;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	UINT64 offset;
	BYTE *data;

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

	dataLen = sizeof(TCPA_PROTOCOL_ID) + sizeof(TCPA_ENCAUTH)
					   + sizeof(TCPA_ENTITY_TYPE)
					   + sizeof(UINT32)
					   + encDataSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_UINT16(&offset, protocolID, data);
	Trspi_LoadBlob(&offset, sizeof(TCPA_ENCAUTH), data, newAuth->authdata);
	Trspi_LoadBlob_UINT16(&offset, entityType, data);
	Trspi_LoadBlob_UINT32(&offset, encDataSize, data);
	Trspi_LoadBlob(&offset, encDataSize, data, encData);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_ChangeAuth, dataLen, data,
						    &pubKeyHash, &handlesLen, &handles,
						    ownerAuth, entityAuth, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, outDataSize, dec);

	if ((*outData = malloc(*outDataSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *outDataSize);
		*outDataSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *outDataSize, dec, *outData);

	free(dec);

	return result;
}

TSS_RESULT
Transport_ChangeAuthOwner(TSS_HCONTEXT tspContext,	/* in */
			  TCPA_PROTOCOL_ID protocolID,	/* in */
			  TCPA_ENCAUTH *newAuth,	/* in */
			  TCPA_ENTITY_TYPE entityType,	/* in */
			  TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;
	UINT64 offset;
	BYTE data[sizeof(TCPA_PROTOCOL_ID) + sizeof(TCPA_ENCAUTH) + sizeof(TCPA_ENTITY_TYPE)];

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	offset = 0;
	Trspi_LoadBlob_UINT16(&offset, protocolID, data);
	Trspi_LoadBlob(&offset, sizeof(TCPA_ENCAUTH), data, newAuth->authdata);
	Trspi_LoadBlob_UINT16(&offset, entityType, data);

	return obj_context_transport_execute(tspContext, TPM_ORD_ChangeAuthOwner, sizeof(data),
					     data, NULL, &handlesLen, NULL, ownerAuth, NULL, NULL,
					     NULL);
}
#endif
