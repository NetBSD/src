
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004, 2007
 *
 */


#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "hosttable.h"
#include "tsplog.h"
#include "rpc_tcstp_tsp.h"
#include "obj_context.h"


TSS_RESULT
RPC_Error(TSS_HCONTEXT tspContext, ...)
{
	LogDebugFn("Context: 0x%x", tspContext);
	return TSPERR(TSS_E_INTERNAL_ERROR);
}

TSS_RESULT
RPC_OpenContext(TSS_HCONTEXT tspContext, BYTE *hostname, int type)
{
	TSS_RESULT result;
	TCS_CONTEXT_HANDLE tcsContext;
	struct host_table_entry *entry;
	UINT32 tpm_version;

	/* __tspi_add_table_entry() will make sure an entry doesn't already exist for this tsp context */
	if ((result = __tspi_add_table_entry(tspContext, hostname, type, &entry)))
		return result;

	switch (type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			if ((result = RPC_OpenContext_TP(entry, &tpm_version, &tcsContext)))
				remove_table_entry(tspContext);
			else {
				entry->tcsContext = tcsContext;
				if (obj_context_set_tpm_version(tspContext, tpm_version)) {
					remove_table_entry(tspContext);
					result = TSPERR(TSS_E_INTERNAL_ERROR);
				}
			}
			return result;
		default:
			break;
	}

	return TSPERR(TSS_E_INTERNAL_ERROR);
}

TSS_RESULT RPC_GetRegisteredKeyByPublicInfo(TSS_HCONTEXT tspContext,
					    TCPA_ALGORITHM_ID algID, /* in */
					    UINT32 ulPublicInfoLength, /* in */
					    BYTE * rgbPublicInfo, /* in */
					    UINT32 * keySize, BYTE ** keyBlob)
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetRegisteredKeyByPublicInfo_TP(entry, algID,
								     ulPublicInfoLength,
								     rgbPublicInfo, keySize,
								     keyBlob);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_CloseContext(TSS_HCONTEXT tspContext)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			if ((result = RPC_CloseContext_TP(entry)) == TSS_SUCCESS) {
				close(entry->socket);
				remove_table_entry(tspContext);
			}
			break;
		default:
			break;
	}

	if (result != TSS_SUCCESS)
		put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_FreeMemory(TSS_HCONTEXT tspContext,	/* in */
			  BYTE * pMemory)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_FreeMemory_TP(entry, pMemory);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_LogPcrEvent(TSS_HCONTEXT tspContext,	/* in */
			   TSS_PCR_EVENT Event,	/* in */
			   UINT32 * pNumber)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_LogPcrEvent_TP(entry, Event, pNumber);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_GetPcrEvent(TSS_HCONTEXT tspContext,	/* in */
			   UINT32 PcrIndex,	/* in */
			   UINT32 * pNumber,	/* in, out */
			   TSS_PCR_EVENT ** ppEvent)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
		result =
			RPC_GetPcrEvent_TP(entry, PcrIndex, pNumber, ppEvent);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_GetPcrEventsByPcr(TSS_HCONTEXT tspContext,	/* in */
				 UINT32 PcrIndex,	/* in */
				 UINT32 FirstEvent,	/* in */
				 UINT32 * pEventCount,	/* in,out */
				 TSS_PCR_EVENT ** ppEvents)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetPcrEventsByPcr_TP(entry, PcrIndex, FirstEvent,
							  pEventCount, ppEvents);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_GetPcrEventLog(TSS_HCONTEXT tspContext,	/* in */
			      UINT32 * pEventCount,	/* out */
			      TSS_PCR_EVENT ** ppEvents)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetPcrEventLog_TP(entry, pEventCount, ppEvents);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_RegisterKey(TSS_HCONTEXT tspContext,	/* in */
			   TSS_UUID WrappingKeyUUID,	/* in */
			   TSS_UUID KeyUUID,	/* in */
			   UINT32 cKeySize,	/* in */
			   BYTE * rgbKey,	/* in */
			   UINT32 cVendorData,	/* in */
			   BYTE * gbVendorData)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_RegisterKey_TP(entry, WrappingKeyUUID, KeyUUID, cKeySize,
						    rgbKey, cVendorData, gbVendorData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_UnregisterKey(TSS_HCONTEXT tspContext,	/* in */
			     TSS_UUID KeyUUID)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_UnregisterKey_TP(entry, KeyUUID);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_EnumRegisteredKeys(TSS_HCONTEXT tspContext,	/* in */
				  TSS_UUID * pKeyUUID,	/* in */
				  UINT32 * pcKeyHierarchySize,	/* out */
				  TSS_KM_KEYINFO ** ppKeyHierarchy)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_EnumRegisteredKeys_TP(entry, pKeyUUID, pcKeyHierarchySize,
							   ppKeyHierarchy);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_EnumRegisteredKeys2(TSS_HCONTEXT tspContext,	/* in */
				   TSS_UUID * pKeyUUID,	/* in */
				   UINT32 * pcKeyHierarchySize,	/* out */
				   TSS_KM_KEYINFO2 ** ppKeyHierarchy)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_EnumRegisteredKeys2_TP(entry, pKeyUUID, pcKeyHierarchySize,
							    ppKeyHierarchy);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}


TSS_RESULT RPC_GetRegisteredKey(TSS_HCONTEXT tspContext,	/* in */
				TSS_UUID KeyUUID,	/* in */
				TSS_KM_KEYINFO ** ppKeyInfo)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetRegisteredKey_TP(entry, KeyUUID, ppKeyInfo);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_GetRegisteredKeyBlob(TSS_HCONTEXT tspContext,	/* in */
				    TSS_UUID KeyUUID,	/* in */
				    UINT32 * pcKeySize,	/* out */
				    BYTE ** prgbKey)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetRegisteredKeyBlob_TP(entry, KeyUUID, pcKeySize, prgbKey);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_LoadKeyByBlob(TSS_HCONTEXT tspContext,	/* in */
			     TCS_KEY_HANDLE hUnwrappingKey,	/* in */
			     UINT32 cWrappedKeyBlobSize,	/* in */
			     BYTE * rgbWrappedKeyBlob,	/* in */
			     TPM_AUTH * pAuth,	/* in, out */
			     TCS_KEY_HANDLE * phKeyTCSI,	/* out */
			     TCS_KEY_HANDLE * phKeyHMAC)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_LoadKeyByBlob_TP(entry, hUnwrappingKey, cWrappedKeyBlobSize,
						      rgbWrappedKeyBlob, pAuth, phKeyTCSI,
						      phKeyHMAC);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_LoadKeyByUUID(TSS_HCONTEXT tspContext,	/* in */
			     TSS_UUID KeyUUID,	/* in */
			     TCS_LOADKEY_INFO * pLoadKeyInfo,	/* in, out */
			     TCS_KEY_HANDLE * phKeyTCSI)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_LoadKeyByUUID_TP(entry, KeyUUID, pLoadKeyInfo, phKeyTCSI);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_EvictKey(TSS_HCONTEXT tspContext,	/* in */
			TCS_KEY_HANDLE hKey)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_EvictKey_TP(entry, hKey);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_CreateWrapKey(TSS_HCONTEXT tspContext,	/* in */
			     TCS_KEY_HANDLE hWrappingKey,	/* in */
			     TCPA_ENCAUTH *KeyUsageAuth,	/* in */
			     TCPA_ENCAUTH *KeyMigrationAuth,	/* in */
			     UINT32 keyInfoSize,	/* in */
			     BYTE * keyInfo,	/* in */
			     UINT32 * keyDataSize,	/* out */
			     BYTE ** keyData,	/* out */
			     TPM_AUTH * pAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CreateWrapKey_TP(entry, hWrappingKey, KeyUsageAuth,
						      KeyMigrationAuth, keyInfoSize, keyInfo,
						      keyDataSize, keyData, pAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_GetPubKey(TSS_HCONTEXT tspContext,	/* in */
			 TCS_KEY_HANDLE hKey,	/* in */
			 TPM_AUTH * pAuth,	/* in, out */
			 UINT32 * pcPubKeySize,	/* out */
			 BYTE ** prgbPubKey)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetPubKey_TP(entry, hKey, pAuth, pcPubKeySize, prgbPubKey);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_MakeIdentity(TSS_HCONTEXT tspContext,	/* in */
			    TCPA_ENCAUTH identityAuth,	/* in */
			    TCPA_CHOSENID_HASH IDLabel_PrivCAHash,	/* in */
			    UINT32 idKeyInfoSize,	/* in */
			    BYTE * idKeyInfo,	/* in */
			    TPM_AUTH * pSrkAuth,	/* in, out */
			    TPM_AUTH * pOwnerAuth,	/* in, out */
			    UINT32 * idKeySize,	/* out */
			    BYTE ** idKey,	/* out */
			    UINT32 * pcIdentityBindingSize,	/* out */
			    BYTE ** prgbIdentityBinding,	/* out */
			    UINT32 * pcEndorsementCredentialSize,	/* out */
			    BYTE ** prgbEndorsementCredential,	/* out */
			    UINT32 * pcPlatformCredentialSize,	/* out */
			    BYTE ** prgbPlatformCredential,	/* out */
			    UINT32 * pcConformanceCredentialSize,	/* out */
			    BYTE ** prgbConformanceCredential)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_MakeIdentity_TP(entry, identityAuth,
						     IDLabel_PrivCAHash, idKeyInfoSize, idKeyInfo,
						     pSrkAuth, pOwnerAuth, idKeySize, idKey,
						     pcIdentityBindingSize, prgbIdentityBinding,
						     pcEndorsementCredentialSize,
						     prgbEndorsementCredential,
						     pcPlatformCredentialSize,
						     prgbPlatformCredential,
						     pcConformanceCredentialSize,
						     prgbConformanceCredential);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT RPC_GetCredential(TSS_HCONTEXT tspContext,	/* in */
			     UINT32 ulCredentialType,          /* in */
			     UINT32 ulCredentialAccessMode,    /* in */
			     UINT32 * pulCredentialSize,       /* out */
			     BYTE ** prgbCredentialData)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetCredential_TP(entry, ulCredentialType,
						      ulCredentialAccessMode, pulCredentialSize,
						      prgbCredentialData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

TSS_RESULT RPC_SetOwnerInstall(TSS_HCONTEXT tspContext,	/* in */
			       TSS_BOOL state)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_SetOwnerInstall_TP(entry, state);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_TakeOwnership(TSS_HCONTEXT tspContext,	/* in */
			     UINT16 protocolID,	/* in */
			     UINT32 encOwnerAuthSize,	/* in */
			     BYTE * encOwnerAuth,	/* in */
			     UINT32 encSrkAuthSize,	/* in */
			     BYTE * encSrkAuth,	/* in */
			     UINT32 srkInfoSize,	/* in */
			     BYTE * srkInfo,	/* in */
			     TPM_AUTH * ownerAuth,	/* in, out */
			     UINT32 * srkKeySize,
			     BYTE ** srkKey)
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_TakeOwnership_TP(entry, protocolID,
						      encOwnerAuthSize, encOwnerAuth,
						      encSrkAuthSize, encSrkAuth, srkInfoSize,
						      srkInfo, ownerAuth, srkKeySize, srkKey);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_OIAP(TSS_HCONTEXT tspContext,	/* in */
		    TCS_AUTHHANDLE * authHandle,	/* out */
		    TCPA_NONCE * nonce0)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_OIAP_TP(entry, authHandle, nonce0);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_OSAP(TSS_HCONTEXT tspContext,	/* in */
		    TCPA_ENTITY_TYPE entityType,	/* in */
		    UINT32 entityValue,	/* in */
		    TPM_NONCE *nonceOddOSAP,	/* in */
		    TCS_AUTHHANDLE * authHandle,	/* out */
		    TCPA_NONCE * nonceEven,	/* out */
		    TCPA_NONCE * nonceEvenOSAP)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_OSAP_TP(entry, entityType, entityValue, nonceOddOSAP,
					     authHandle, nonceEven, nonceEvenOSAP);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_ChangeAuth(TSS_HCONTEXT tspContext,	/* in */
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
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ChangeAuth_TP(entry, parentHandle, protocolID, newAuth,
						   entityType, encDataSize, encData, ownerAuth,
						   entityAuth, outDataSize, outData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_ChangeAuthOwner(TSS_HCONTEXT tspContext,	/* in */
				TCPA_PROTOCOL_ID protocolID,	/* in */
				TCPA_ENCAUTH *newAuth,	/* in */
				TCPA_ENTITY_TYPE entityType,	/* in */
				TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ChangeAuthOwner_TP(entry, protocolID, newAuth, entityType,
							ownerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_ChangeAuthAsymStart(TSS_HCONTEXT tspContext,	/* in */
				   TCS_KEY_HANDLE idHandle,	/* in */
				   TCPA_NONCE antiReplay,	/* in */
				   UINT32 KeySizeIn,	/* in */
				   BYTE * KeyDataIn,	/* in */
				   TPM_AUTH * pAuth,	/* in, out */
				   UINT32 * KeySizeOut,	/* out */
				   BYTE ** KeyDataOut,	/* out */
				   UINT32 * CertifyInfoSize,	/* out */
				   BYTE ** CertifyInfo,	/* out */
				   UINT32 * sigSize,	/* out */
				   BYTE ** sig,	/* out */
				   TCS_KEY_HANDLE * ephHandle)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ChangeAuthAsymStart_TP(entry, idHandle, antiReplay,
							    KeySizeIn, KeyDataIn, pAuth,
							    KeySizeOut, KeyDataOut,
							    CertifyInfoSize, CertifyInfo, sigSize,
							    sig, ephHandle);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_ChangeAuthAsymFinish(TSS_HCONTEXT tspContext,	/* in */
				    TCS_KEY_HANDLE parentHandle,	/* in */
				    TCS_KEY_HANDLE ephHandle,	/* in */
				    TCPA_ENTITY_TYPE entityType,	/* in */
				    TCPA_HMAC newAuthLink,	/* in */
				    UINT32 newAuthSize,	/* in */
				    BYTE * encNewAuth,	/* in */
				    UINT32 encDataSizeIn,	/* in */
				    BYTE * encDataIn,	/* in */
				    TPM_AUTH * ownerAuth,	/* in, out */
				    UINT32 * encDataSizeOut,	/* out */
				    BYTE ** encDataOut,	/* out */
				    TCPA_SALT_NONCE * saltNonce,	/* out */
				    TCPA_DIGEST * changeProof)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ChangeAuthAsymFinish_TP(entry, parentHandle, ephHandle,
							     entityType, newAuthLink,
							     newAuthSize, encNewAuth,
							     encDataSizeIn, encDataIn, ownerAuth,
							     encDataSizeOut, encDataOut, saltNonce,
							     changeProof);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_TerminateHandle(TSS_HCONTEXT tspContext,	/* in */
			       TCS_AUTHHANDLE handle)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_TerminateHandle_TP(entry, handle);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_ActivateTPMIdentity(TSS_HCONTEXT tspContext,	/* in */
				   TCS_KEY_HANDLE idKey,	/* in */
				   UINT32 blobSize,	/* in */
				   BYTE * blob,	/* in */
				   TPM_AUTH * idKeyAuth,	/* in, out */
				   TPM_AUTH * ownerAuth,	/* in, out */
				   UINT32 * SymmetricKeySize,	/* out */
				   BYTE ** SymmetricKey)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ActivateTPMIdentity_TP(entry, idKey, blobSize, blob, idKeyAuth,
							    ownerAuth, SymmetricKeySize,
							    SymmetricKey);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_Extend(TSS_HCONTEXT tspContext,	/* in */
		      TCPA_PCRINDEX pcrNum,	/* in */
		      TCPA_DIGEST inDigest,	/* in */
		      TCPA_PCRVALUE * outDigest)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Extend_TP(entry, pcrNum, inDigest, outDigest);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_PcrRead(TSS_HCONTEXT tspContext,	/* in */
		       TCPA_PCRINDEX pcrNum,	/* in */
		       TCPA_PCRVALUE * outDigest)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_PcrRead_TP(entry, pcrNum, outDigest);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_PcrReset(TSS_HCONTEXT tspContext,	/* in */
			UINT32 pcrDataSizeIn,		/* in */
			BYTE * pcrDataIn)		/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_PcrReset_TP(entry, pcrDataSizeIn, pcrDataIn);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}


TSS_RESULT RPC_Quote(TSS_HCONTEXT tspContext,	/* in */
		     TCS_KEY_HANDLE keyHandle,	/* in */
		     TCPA_NONCE *antiReplay,	/* in */
		     UINT32 pcrDataSizeIn,	/* in */
		     BYTE * pcrDataIn,	/* in */
		     TPM_AUTH * privAuth,	/* in, out */
		     UINT32 * pcrDataSizeOut,	/* out */
		     BYTE ** pcrDataOut,	/* out */
		     UINT32 * sigSize,	/* out */
		     BYTE ** sig)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Quote_TP(entry, keyHandle, antiReplay, pcrDataSizeIn,
					      pcrDataIn, privAuth, pcrDataSizeOut, pcrDataOut,
					      sigSize, sig);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT RPC_Quote2(TSS_HCONTEXT tspContext, /* in */
		      TCS_KEY_HANDLE keyHandle, /* in */
		      TCPA_NONCE *antiReplay, /* in */
		      UINT32 pcrDataSizeIn, /* in */
		      BYTE * pcrDataIn, /* in */
		      TSS_BOOL addVersion, /* in */
		      TPM_AUTH * privAuth, /* in,out */
		      UINT32 * pcrDataSizeOut, /* out */
		      BYTE ** pcrDataOut, /* out */
		      UINT32 * versionInfoSize, /* out */
		      BYTE ** versionInfo, /* out */
		      UINT32 * sigSize, /* out */
		      BYTE ** sig) /* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
	case CONNECTION_TYPE_TCP_PERSISTANT:
		result = RPC_Quote2_TP(entry, keyHandle, antiReplay, pcrDataSizeIn, pcrDataIn,
				       addVersion,privAuth, pcrDataSizeOut, pcrDataOut,
				       versionInfoSize, versionInfo,sigSize, sig);
		break;
	default:
		break;
	}

	put_table_entry(entry);

	return result;
}
#endif

TSS_RESULT RPC_DirWriteAuth(TSS_HCONTEXT tspContext,	/* in */
			    TCPA_DIRINDEX dirIndex,	/* in */
			    TCPA_DIRVALUE *newContents,	/* in */
			    TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_DirWriteAuth_TP(entry, dirIndex, newContents, ownerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_DirRead(TSS_HCONTEXT tspContext,	/* in */
		       TCPA_DIRINDEX dirIndex,	/* in */
		       TCPA_DIRVALUE * dirValue)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_DirRead_TP(entry, dirIndex, dirValue);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_Seal(TSS_HCONTEXT tspContext,	/* in */
		    TCS_KEY_HANDLE keyHandle,	/* in */
		    TCPA_ENCAUTH *encAuth,	/* in */
		    UINT32 pcrInfoSize,	/* in */
		    BYTE * PcrInfo,	/* in */
		    UINT32 inDataSize,	/* in */
		    BYTE * inData,	/* in */
		    TPM_AUTH * pubAuth,	/* in, out */
		    UINT32 * SealedDataSize,	/* out */
		    BYTE ** SealedData)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Seal_TP(entry, keyHandle, encAuth, pcrInfoSize, PcrInfo,
					     inDataSize, inData, pubAuth, SealedDataSize,
					     SealedData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

#ifdef TSS_BUILD_SEALX
TSS_RESULT RPC_Sealx(TSS_HCONTEXT tspContext,	/* in */
		     TCS_KEY_HANDLE keyHandle,	/* in */
		     TCPA_ENCAUTH *encAuth,	/* in */
		     UINT32 pcrInfoSize,	/* in */
		     BYTE * PcrInfo,	/* in */
		     UINT32 inDataSize,	/* in */
		     BYTE * inData,	/* in */
		     TPM_AUTH * pubAuth,	/* in, out */
		     UINT32 * SealedDataSize,	/* out */
		     BYTE ** SealedData)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Sealx_TP(entry, keyHandle, encAuth, pcrInfoSize, PcrInfo,
					      inDataSize, inData, pubAuth, SealedDataSize,
					      SealedData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

TSS_RESULT RPC_Unseal(TSS_HCONTEXT tspContext,	/* in */
		      TCS_KEY_HANDLE parentHandle,	/* in */
		      UINT32 SealedDataSize,	/* in */
		      BYTE * SealedData,	/* in */
		      TPM_AUTH * parentAuth,	/* in, out */
		      TPM_AUTH * dataAuth,	/* in, out */
		      UINT32 * DataSize,	/* out */
		      BYTE ** Data)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Unseal_TP(entry, parentHandle, SealedDataSize, SealedData,
					       parentAuth, dataAuth, DataSize, Data);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_UnBind(TSS_HCONTEXT tspContext,	/* in */
		       TCS_KEY_HANDLE keyHandle,	/* in */
		       UINT32 inDataSize,	/* in */
		       BYTE * inData,	/* in */
		       TPM_AUTH * privAuth,	/* in, out */
		       UINT32 * outDataSize,	/* out */
		       BYTE ** outData)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_UnBind_TP(entry, keyHandle, inDataSize, inData, privAuth,
					       outDataSize, outData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_CreateMigrationBlob(TSS_HCONTEXT tspContext,	/* in */
				   TCS_KEY_HANDLE parentHandle,	/* in */
				   TCPA_MIGRATE_SCHEME migrationType,	/* in */
				   UINT32 MigrationKeyAuthSize,	/* in */
				   BYTE * MigrationKeyAuth,	/* in */
				   UINT32 encDataSize,	/* in */
				   BYTE * encData,	/* in */
				   TPM_AUTH * parentAuth,	/* in, out */
				   TPM_AUTH * entityAuth,	/* in, out */
				   UINT32 * randomSize,	/* out */
				   BYTE ** random,	/* out */
				   UINT32 * outDataSize,	/* out */
				   BYTE ** outData)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CreateMigrationBlob_TP(entry, parentHandle,
							    migrationType, MigrationKeyAuthSize,
							    MigrationKeyAuth, encDataSize, encData,
							    parentAuth, entityAuth, randomSize,
							    random, outDataSize, outData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_ConvertMigrationBlob(TSS_HCONTEXT tspContext,	/* in */
				    TCS_KEY_HANDLE parentHandle,	/* in */
				    UINT32 inDataSize,	/* in */
				    BYTE * inData,	/* in */
				    UINT32 randomSize,	/* in */
				    BYTE * random,	/* in */
				    TPM_AUTH * parentAuth,	/* in, out */
				    UINT32 * outDataSize,	/* out */
				    BYTE ** outData)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ConvertMigrationBlob_TP(entry, parentHandle,
							     inDataSize, inData, randomSize,
							     random, parentAuth, outDataSize,
							     outData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_AuthorizeMigrationKey(TSS_HCONTEXT tspContext,	/* in */
				     TCPA_MIGRATE_SCHEME migrateScheme,	/* in */
				     UINT32 MigrationKeySize,	/* in */
				     BYTE * MigrationKey,	/* in */
				     TPM_AUTH * ownerAuth,	/* in, out */
				     UINT32 * MigrationKeyAuthSize,	/* out */
				     BYTE ** MigrationKeyAuth)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_AuthorizeMigrationKey_TP(entry, migrateScheme,
							      MigrationKeySize, MigrationKey,
							      ownerAuth, MigrationKeyAuthSize,
							      MigrationKeyAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_CertifyKey(TSS_HCONTEXT tspContext,	/* in */
			  TCS_KEY_HANDLE certHandle,	/* in */
			  TCS_KEY_HANDLE keyHandle,	/* in */
			  TPM_NONCE * antiReplay,	/* in */
			  TPM_AUTH * certAuth,	/* in, out */
			  TPM_AUTH * keyAuth,	/* in, out */
			  UINT32 * CertifyInfoSize,	/* out */
			  BYTE ** CertifyInfo,	/* out */
			  UINT32 * outDataSize,	/* out */
			  BYTE ** outData)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CertifyKey_TP(entry, certHandle, keyHandle, antiReplay,
						   certAuth, keyAuth, CertifyInfoSize, CertifyInfo,
						   outDataSize, outData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_Sign(TSS_HCONTEXT tspContext,	/* in */
		    TCS_KEY_HANDLE keyHandle,	/* in */
		    UINT32 areaToSignSize,	/* in */
		    BYTE * areaToSign,	/* in */
		    TPM_AUTH * privAuth,	/* in, out */
		    UINT32 * sigSize,	/* out */
		    BYTE ** sig)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Sign_TP(entry, keyHandle, areaToSignSize, areaToSign, privAuth,
					     sigSize, sig);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_GetRandom(TSS_HCONTEXT tspContext,	/* in */
			 UINT32 bytesRequested,	/* in */
			 BYTE ** randomBytes)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetRandom_TP(entry, bytesRequested, randomBytes);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_StirRandom(TSS_HCONTEXT tspContext,	/* in */
			  UINT32 inDataSize,	/* in */
			  BYTE * inData)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_StirRandom_TP(entry, inDataSize, inData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_GetTPMCapability(TSS_HCONTEXT tspContext,	/* in */
			        TCPA_CAPABILITY_AREA capArea,	/* in */
			        UINT32 subCapSize,	/* in */
			        BYTE * subCap,	/* in */
			        UINT32 * respSize,	/* out */
			        BYTE ** resp)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetTPMCapability_TP(entry, capArea, subCapSize, subCap,
							 respSize, resp);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_SetCapability(TSS_HCONTEXT tspContext,	/* in */
			     TCPA_CAPABILITY_AREA capArea,	/* in */
			     UINT32 subCapSize,	/* in */
			     BYTE * subCap,	/* in */
			     UINT32 valueSize,	/* in */
			     BYTE * value,	/* in */
			     TPM_AUTH *ownerAuth) /* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_SetCapability_TP(entry, capArea, subCapSize, subCap,
						      valueSize, value, ownerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_GetCapability(TSS_HCONTEXT tspContext,	/* in */
			     TCPA_CAPABILITY_AREA capArea,	/* in */
			     UINT32 subCapSize,	/* in */
			     BYTE * subCap,	/* in */
			     UINT32 * respSize,	/* out */
			     BYTE ** resp)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetCapability_TP(entry, capArea, subCapSize, subCap, respSize,
						      resp);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_GetCapabilitySigned(TSS_HCONTEXT tspContext,	/* in */
				   TCS_KEY_HANDLE keyHandle,	/* in */
				   TCPA_NONCE antiReplay,	/* in */
				   TCPA_CAPABILITY_AREA capArea,	/* in */
				   UINT32 subCapSize,	/* in */
				   BYTE * subCap,	/* in */
				   TPM_AUTH * privAuth,	/* in, out */
				   TCPA_VERSION * Version,	/* out */
				   UINT32 * respSize,	/* out */
				   BYTE ** resp,	/* out */
				   UINT32 * sigSize,	/* out */
				   BYTE ** sig)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetCapabilitySigned_TP(entry, keyHandle, antiReplay, capArea,
							    subCapSize, subCap, privAuth, Version,
							    respSize, resp, sigSize, sig);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_GetCapabilityOwner(TSS_HCONTEXT tspContext,	/* in */
				  TPM_AUTH * pOwnerAuth,	/* out */
				  TCPA_VERSION * pVersion,	/* out */
				  UINT32 * pNonVolatileFlags,	/* out */
				  UINT32 * pVolatileFlags)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetCapabilityOwner_TP(entry, pOwnerAuth, pVersion,
							   pNonVolatileFlags, pVolatileFlags);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_CreateEndorsementKeyPair(TSS_HCONTEXT tspContext,	/* in */
					TCPA_NONCE antiReplay,	/* in */
					UINT32 endorsementKeyInfoSize,	/* in */
					BYTE * endorsementKeyInfo,	/* in */
					UINT32 * endorsementKeySize,	/* out */
					BYTE ** endorsementKey,	/* out */
					TCPA_DIGEST * checksum)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CreateEndorsementKeyPair_TP(entry, antiReplay,
								 endorsementKeyInfoSize,
								 endorsementKeyInfo,
								 endorsementKeySize,
								 endorsementKey, checksum);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_ReadPubek(TSS_HCONTEXT tspContext,	/* in */
			 TCPA_NONCE antiReplay,	/* in */
			 UINT32 * pubEndorsementKeySize,	/* out */
			 BYTE ** pubEndorsementKey,	/* out */
			 TCPA_DIGEST * checksum)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ReadPubek_TP(entry, antiReplay, pubEndorsementKeySize,
						  pubEndorsementKey, checksum);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_DisablePubekRead(TSS_HCONTEXT tspContext,	/* in */
				TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_DisablePubekRead_TP(entry, ownerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_OwnerReadPubek(TSS_HCONTEXT tspContext,	/* in */
			      TPM_AUTH * ownerAuth,	/* in, out */
			      UINT32 * pubEndorsementKeySize,	/* out */
			      BYTE ** pubEndorsementKey)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_OwnerReadPubek_TP(entry, ownerAuth, pubEndorsementKeySize,
						       pubEndorsementKey);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT RPC_CreateRevocableEndorsementKeyPair(TSS_HCONTEXT tspContext,	/* in */
						 TPM_NONCE antiReplay,		/* in */
						 UINT32 endorsementKeyInfoSize,/* in */
						 BYTE * endorsementKeyInfo,	/* in */
						 TSS_BOOL genResetAuth,	/* in */
						 TPM_DIGEST * eKResetAuth,	/* in, out */
						 UINT32 * endorsementKeySize,	/* out */
						 BYTE ** endorsementKey,	/* out */
						 TPM_DIGEST * checksum)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CreateRevocableEndorsementKeyPair_TP(entry, antiReplay,
									  endorsementKeyInfoSize,
									  endorsementKeyInfo,
									  genResetAuth,
									  eKResetAuth,
									  endorsementKeySize,
									  endorsementKey, checksum);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_RevokeEndorsementKeyPair(TSS_HCONTEXT tspContext,	/* in */
					TPM_DIGEST *EKResetAuth)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_RevokeEndorsementKeyPair_TP(entry, EKResetAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

TSS_RESULT RPC_SelfTestFull(TSS_HCONTEXT tspContext)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_SelfTestFull_TP(entry);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_CertifySelfTest(TSS_HCONTEXT tspContext,	/* in */
			       TCS_KEY_HANDLE keyHandle,	/* in */
			       TCPA_NONCE antiReplay,	/* in */
			       TPM_AUTH * privAuth,	/* in, out */
			       UINT32 * sigSize,	/* out */
			       BYTE ** sig)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CertifySelfTest_TP(entry, keyHandle, antiReplay, privAuth,
							sigSize, sig);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_GetTestResult(TSS_HCONTEXT tspContext,	/* in */
			     UINT32 * outDataSize,	/* out */
			     BYTE ** outData)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetTestResult_TP(entry, outDataSize, outData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_OwnerSetDisable(TSS_HCONTEXT tspContext,	/* in */
			       TSS_BOOL disableState,	/* in */
			       TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_OwnerSetDisable_TP(entry, disableState, ownerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT RPC_ResetLockValue(TSS_HCONTEXT tspContext,	/* in */
			      TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ResetLockValue_TP(entry, ownerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

TSS_RESULT RPC_OwnerClear(TSS_HCONTEXT tspContext,	/* in */
			  TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_OwnerClear_TP(entry, ownerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_DisableOwnerClear(TSS_HCONTEXT tspContext,	/* in */
				 TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_DisableOwnerClear_TP(entry, ownerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_ForceClear(TSS_HCONTEXT tspContext)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ForceClear_TP(entry);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_DisableForceClear(TSS_HCONTEXT tspContext)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_DisableForceClear_TP(entry);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_PhysicalDisable(TSS_HCONTEXT tspContext)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_PhysicalDisable_TP(entry);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_PhysicalEnable(TSS_HCONTEXT tspContext)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_PhysicalEnable_TP(entry);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_PhysicalSetDeactivated(TSS_HCONTEXT tspContext,	/* in */
				      TSS_BOOL state)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_PhysicalSetDeactivated_TP(entry, state);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_PhysicalPresence(TSS_HCONTEXT tspContext,	/* in */
				TCPA_PHYSICAL_PRESENCE fPhysicalPresence)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_PhysicalPresence_TP(entry, fPhysicalPresence);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_SetTempDeactivated(TSS_HCONTEXT tspContext)	/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_SetTempDeactivated_TP(entry);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT RPC_SetTempDeactivated2(TSS_HCONTEXT tspContext,	/* in */
				   TPM_AUTH *operatorAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_SetTempDeactivated2_TP(entry, operatorAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

TSS_RESULT RPC_FieldUpgrade(TSS_HCONTEXT tspContext,	/* in */
			    UINT32 dataInSize,	/* in */
			    BYTE * dataIn,	/* in */
			    UINT32 * dataOutSize,	/* out */
			    BYTE ** dataOut,	/* out */
			    TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = (UINT32) TSPERR(TSS_E_INTERNAL_ERROR);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_SetRedirection(TSS_HCONTEXT tspContext,	/* in */
			      TCS_KEY_HANDLE keyHandle,	/* in */
			      UINT32 c1,	/* in */
			      UINT32 c2,	/* in */
			      TPM_AUTH * privAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = (UINT32) TSPERR(TSS_E_INTERNAL_ERROR);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_CreateMaintenanceArchive(TSS_HCONTEXT tspContext,	/* in */
					TSS_BOOL generateRandom,	/* in */
					TPM_AUTH * ownerAuth,	/* in, out */
					UINT32 * randomSize,	/* out */
					BYTE ** random,	/* out */
					UINT32 * archiveSize,	/* out */
					BYTE ** archive)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CreateMaintenanceArchive_TP(entry, generateRandom, ownerAuth,
								 randomSize, random, archiveSize,
								 archive);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_LoadMaintenanceArchive(TSS_HCONTEXT tspContext,	/* in */
				      UINT32 dataInSize,	/* in */
				      BYTE * dataIn, /* in */
				      TPM_AUTH * ownerAuth,	/* in, out */
				      UINT32 * dataOutSize,	/* out */
				      BYTE ** dataOut)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_LoadMaintenanceArchive_TP(entry, dataInSize, dataIn, ownerAuth,
							       dataOutSize, dataOut);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_KillMaintenanceFeature(TSS_HCONTEXT tspContext,	/* in */
				      TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_KillMaintenanceFeature_TP(entry, ownerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_LoadManuMaintPub(TSS_HCONTEXT tspContext,	/* in */
				TCPA_NONCE antiReplay,	/* in */
				UINT32 PubKeySize,	/* in */
				BYTE * PubKey,	/* in */
				TCPA_DIGEST * checksum)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_LoadManuMaintPub_TP(entry, antiReplay, PubKeySize, PubKey,
							 checksum);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT RPC_ReadManuMaintPub(TSS_HCONTEXT tspContext,	/* in */
				TCPA_NONCE antiReplay,	/* in */
				TCPA_DIGEST * checksum)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ReadManuMaintPub_TP(entry, antiReplay, checksum);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

#ifdef TSS_BUILD_DAA
TSS_RESULT
RPC_DaaJoin(TSS_HCONTEXT tspContext,	/* in */
	    TPM_HANDLE daa_session,		/* in */
	    BYTE stage,			/* in */
	    UINT32 inputSize0,			/* in */
	    BYTE* inputData0,			/* in */
	    UINT32 inputSize1,			/* in */
	    BYTE* inputData1,			/* in */
	    TPM_AUTH* ownerAuth,		/* in, out */
	    UINT32* outputSize,		/* out */
	    BYTE** outputData)			/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_DaaJoin_TP(entry, daa_session, stage, inputSize0, inputData0,
						inputSize1, inputData1, ownerAuth, outputSize,
						outputData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;

}

TSS_RESULT
RPC_DaaSign(TSS_HCONTEXT tspContext,	/* in */
	    TPM_HANDLE daa_session,		/* in */
	    BYTE stage,			/* in */
	    UINT32 inputSize0,			/* in */
	    BYTE* inputData0,			/* in */
	    UINT32 inputSize1,			/* in */
	    BYTE* inputData1,			/* in */
	    TPM_AUTH* ownerAuth,		/* in, out */
	    UINT32* outputSize,		/* out */
	    BYTE** outputData)			/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_DaaSign_TP(entry, daa_session, stage, inputSize0, inputData0,
						inputSize1, inputData1, ownerAuth, outputSize,
						outputData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

#ifdef TSS_BUILD_COUNTER
TSS_RESULT
RPC_ReadCounter(TSS_HCONTEXT       tspContext,		/* in */
		TSS_COUNTER_ID     idCounter,		/* in */
		TPM_COUNTER_VALUE* counterValue)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ReadCounter_TP(entry, idCounter, counterValue);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_CreateCounter(TSS_HCONTEXT       tspContext,	/* in */
		  UINT32             LabelSize,	/* in (=4) */
		  BYTE*              pLabel,		/* in */
		  TPM_ENCAUTH        CounterAuth,	/* in */
		  TPM_AUTH*          pOwnerAuth,	/* in, out */
		  TSS_COUNTER_ID*    idCounter,	/* out */
		  TPM_COUNTER_VALUE* counterValue)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CreateCounter_TP(entry, LabelSize, pLabel, CounterAuth,
						      pOwnerAuth, idCounter, counterValue);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_IncrementCounter(TSS_HCONTEXT       tspContext,	/* in */
		     TSS_COUNTER_ID     idCounter,	/* in */
		     TPM_AUTH*          pCounterAuth,	/* in, out */
		     TPM_COUNTER_VALUE* counterValue)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_IncrementCounter_TP(entry, idCounter, pCounterAuth,
							 counterValue);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_ReleaseCounter(TSS_HCONTEXT   tspContext,		/* in */
		   TSS_COUNTER_ID idCounter,		/* in */
		   TPM_AUTH*      pCounterAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ReleaseCounter_TP(entry, idCounter, pCounterAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_ReleaseCounterOwner(TSS_HCONTEXT   tspContext,	/* in */
			TSS_COUNTER_ID idCounter,	/* in */
			TPM_AUTH*      pOwnerAuth)	/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ReleaseCounterOwner_TP(entry, idCounter, pOwnerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

#ifdef TSS_BUILD_TICK
TSS_RESULT
RPC_ReadCurrentTicks(TSS_HCONTEXT tspContext,		/* in */
		     UINT32*      pulCurrentTime,	/* out */
		     BYTE**       prgbCurrentTime)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ReadCurrentTicks_TP(entry, pulCurrentTime, prgbCurrentTime);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_TickStampBlob(TSS_HCONTEXT   tspContext,		/* in */
		  TCS_KEY_HANDLE hKey,			/* in */
		  TPM_NONCE*     antiReplay,		/* in */
		  TPM_DIGEST*    digestToStamp,	/* in */
		  TPM_AUTH*      privAuth,		/* in, out */
		  UINT32*        pulSignatureLength,	/* out */
		  BYTE**         prgbSignature,	/* out */
		  UINT32*        pulTickCountLength,	/* out */
		  BYTE**         prgbTickCount)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_TickStampBlob_TP(entry, hKey, antiReplay, digestToStamp,
						      privAuth, pulSignatureLength,
						      prgbSignature, pulTickCountLength,
						      prgbTickCount);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
RPC_EstablishTransport(TSS_HCONTEXT            tspContext,
		       UINT32                  ulTransControlFlags,
		       TCS_KEY_HANDLE          hEncKey,
		       UINT32                  ulTransSessionInfoSize,
		       BYTE*                   rgbTransSessionInfo,
		       UINT32                  ulSecretSize,
		       BYTE*                   rgbSecret,
		       TPM_AUTH*               pEncKeyAuth,		/* in, out */
		       TPM_MODIFIER_INDICATOR* pbLocality,
		       TCS_HANDLE*             hTransSession,
		       UINT32*                 ulCurrentTicksSize,
		       BYTE**                  prgbCurrentTicks,
		       TPM_NONCE*              pTransNonce)
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_EstablishTransport_TP(entry, ulTransControlFlags, hEncKey,
							   ulTransSessionInfoSize,
							   rgbTransSessionInfo, ulSecretSize,
							   rgbSecret, pEncKeyAuth, pbLocality,
							   hTransSession, ulCurrentTicksSize,
							   prgbCurrentTicks, pTransNonce);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}


TSS_RESULT
RPC_ExecuteTransport(TSS_HCONTEXT            tspContext,
		     TPM_COMMAND_CODE        unWrappedCommandOrdinal,
		     UINT32                  ulWrappedCmdParamInSize,
		     BYTE*                   rgbWrappedCmdParamIn,
		     UINT32*                 pulHandleListSize,	/* in, out */
		     TCS_HANDLE**            rghHandles,		/* in, out */
		     TPM_AUTH*               pWrappedCmdAuth1,		/* in, out */
		     TPM_AUTH*               pWrappedCmdAuth2,		/* in, out */
		     TPM_AUTH*               pTransAuth,		/* in, out */
		     UINT64*                 punCurrentTicks,
		     TPM_MODIFIER_INDICATOR* pbLocality,
		     TPM_RESULT*             pulWrappedCmdReturnCode,
		     UINT32*                 ulWrappedCmdParamOutSize,
		     BYTE**                  rgbWrappedCmdParamOut)
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ExecuteTransport_TP(entry, unWrappedCommandOrdinal,
							 ulWrappedCmdParamInSize,
							 rgbWrappedCmdParamIn, pulHandleListSize,
							 rghHandles, pWrappedCmdAuth1,
							 pWrappedCmdAuth2, pTransAuth,
							 punCurrentTicks, pbLocality,
							 pulWrappedCmdReturnCode,
							 ulWrappedCmdParamOutSize,
							 rgbWrappedCmdParamOut);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_ReleaseTransportSigned(TSS_HCONTEXT            tspContext,
			   TCS_KEY_HANDLE          hSignatureKey,
			   TPM_NONCE*              AntiReplayNonce,
			   TPM_AUTH*               pKeyAuth,		/* in, out */
			   TPM_AUTH*               pTransAuth,		/* in, out */
			   TPM_MODIFIER_INDICATOR* pbLocality,
			   UINT32*                 pulCurrentTicksSize,
			   BYTE**                  prgbCurrentTicks,
			   UINT32*                 pulSignatureSize,
			   BYTE**                  prgbSignature)
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(tspContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_ReleaseTransportSigned_TP(entry, hSignatureKey,
							       AntiReplayNonce, pKeyAuth,
							       pTransAuth, pbLocality,
							       pulCurrentTicksSize,
							       prgbCurrentTicks, pulSignatureSize,
							       prgbSignature);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

#ifdef TSS_BUILD_NV
TSS_RESULT
RPC_NV_DefineOrReleaseSpace(TSS_HCONTEXT hContext,	/* in */
			    UINT32 cPubInfoSize,	/* in */
			    BYTE* pPubInfo,		/* in */
			    TCPA_ENCAUTH encAuth,	/* in */
			    TPM_AUTH* pAuth)		/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_NV_DefineOrReleaseSpace_TP(entry, cPubInfoSize, pPubInfo,
								encAuth, pAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_NV_WriteValue(TSS_HCONTEXT hContext,	/* in */
		  TSS_NV_INDEX hNVStore,	/* in */
		  UINT32 offset,		/* in */
		  UINT32 ulDataLength,		/* in */
		  BYTE* rgbDataToWrite,	/* in */
		  TPM_AUTH* privAuth)		/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_NV_WriteValue_TP(entry, hNVStore, offset, ulDataLength,
						      rgbDataToWrite, privAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}


TSS_RESULT
RPC_NV_WriteValueAuth(TSS_HCONTEXT hContext,	/* in */
		      TSS_NV_INDEX hNVStore,		/* in */
		      UINT32 offset,			/* in */
		      UINT32 ulDataLength,		/* in */
		      BYTE* rgbDataToWrite,		/* in */
		      TPM_AUTH* NVAuth)		/* in, out */
{

	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_NV_WriteValueAuth_TP(entry, hNVStore, offset, ulDataLength,
							  rgbDataToWrite, NVAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}


TSS_RESULT
RPC_NV_ReadValue(TSS_HCONTEXT hContext,	/* in */
		 TSS_NV_INDEX hNVStore,	/* in */
		 UINT32 offset,		/* in */
		 UINT32* pulDataLength,	/* in, out */
		 TPM_AUTH* privAuth,		/* in, out */
		 BYTE** rgbDataRead)		/* out */
{

	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_NV_ReadValue_TP(entry, hNVStore, offset, pulDataLength,
						     privAuth, rgbDataRead);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_NV_ReadValueAuth(TSS_HCONTEXT hContext,	/* in */
		     TSS_NV_INDEX hNVStore,    /* in */
		     UINT32 offset,		/* in */
		     UINT32* pulDataLength,    /* in, out */
		     TPM_AUTH* NVAuth,		/* in, out */
		     BYTE** rgbDataRead)       /* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_NV_ReadValueAuth_TP(entry, hNVStore, offset, pulDataLength,
							 NVAuth, rgbDataRead);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

#ifdef TSS_BUILD_AUDIT
TSS_RESULT
RPC_SetOrdinalAuditStatus(TSS_HCONTEXT hContext,	/* in */
			  TPM_AUTH *ownerAuth,		/* in/out */
			  UINT32 ulOrdinal,		/* in */
			  TSS_BOOL bAuditState)	/* in */
{
	TSS_RESULT result = TSS_SUCCESS;
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_SetOrdinalAuditStatus_TP(entry, ownerAuth, ulOrdinal,
							      bAuditState);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_GetAuditDigest(TSS_HCONTEXT hContext,	/* in */
		   UINT32 startOrdinal,		/* in */
		   TPM_DIGEST *auditDigest,		/* out */
		   UINT32 *counterValueSize,		/* out */
		   BYTE **counterValue,		/* out */
		   TSS_BOOL *more,			/* out */
		   UINT32 *ordSize,			/* out */
		   UINT32 **ordList)			/* out */
{
	TSS_RESULT result = TSS_SUCCESS;
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetAuditDigest_TP(entry, startOrdinal, auditDigest,
						       counterValueSize, counterValue, more,
						       ordSize, ordList);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_GetAuditDigestSigned(TSS_HCONTEXT hContext,		/* in */
			 TCS_KEY_HANDLE keyHandle,	/* in */
			 TSS_BOOL closeAudit,		/* in */
			 TPM_NONCE *antiReplay,		/* in */
			 TPM_AUTH *privAuth,		/* in/out */
			 UINT32 *counterValueSize,	/* out */
			 BYTE **counterValue,		/* out */
			 TPM_DIGEST *auditDigest,	/* out */
			 TPM_DIGEST *ordinalDigest,	/* out */
			 UINT32 *sigSize,		/* out */
			 BYTE **sig)			/* out */
{
	TSS_RESULT result = TSS_SUCCESS;
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_GetAuditDigestSigned_TP(entry, keyHandle, closeAudit,
							     antiReplay, privAuth,
							     counterValueSize, counterValue,
							     auditDigest, ordinalDigest,
							     sigSize, sig);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

#ifdef TSS_BUILD_TSS12
TSS_RESULT
RPC_SetOperatorAuth(TSS_HCONTEXT hContext,	/* in */
		    TCPA_SECRET *operatorAuth)		/* in */
{
	TSS_RESULT result = TSS_SUCCESS;
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_SetOperatorAuth_TP(entry, operatorAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_OwnerReadInternalPub(TSS_HCONTEXT hContext,	/* in */
			 TCS_KEY_HANDLE hKey,		/* in */
			 TPM_AUTH* pOwnerAuth,		/* in, out */
			 UINT32* punPubKeySize,	/* out */
			 BYTE** ppbPubKeyData)		/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_OwnerReadInternalPub_TP(entry, hKey, pOwnerAuth, punPubKeySize,
							     ppbPubKeyData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

#ifdef TSS_BUILD_DELEGATION
TSS_RESULT
RPC_Delegate_Manage(TSS_HCONTEXT hContext,		/* in */
		    TPM_FAMILY_ID familyID,		/* in */
		    TPM_FAMILY_OPERATION opFlag,	/* in */
		    UINT32 opDataSize,			/* in */
		    BYTE *opData,			/* in */
		    TPM_AUTH *ownerAuth,		/* in, out */
		    UINT32 *retDataSize,		/* out */
		    BYTE **retData)			/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Delegate_Manage_TP(entry, familyID, opFlag, opDataSize, opData,
							ownerAuth, retDataSize, retData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_Delegate_CreateKeyDelegation(TSS_HCONTEXT hContext,		/* in */
				 TCS_KEY_HANDLE hKey,		/* in */
				 UINT32 publicInfoSize,		/* in */
				 BYTE *publicInfo,		/* in */
				 TPM_ENCAUTH *encDelAuth,	/* in */
				 TPM_AUTH *keyAuth,		/* in, out */
				 UINT32 *blobSize,		/* out */
				 BYTE **blob)			/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Delegate_CreateKeyDelegation_TP(entry, hKey, publicInfoSize,
								     publicInfo, encDelAuth,
								     keyAuth, blobSize, blob);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_Delegate_CreateOwnerDelegation(TSS_HCONTEXT hContext,	/* in */
				   TSS_BOOL increment,		/* in */
				   UINT32 publicInfoSize,	/* in */
				   BYTE *publicInfo,		/* in */
				   TPM_ENCAUTH *encDelAuth,	/* in */
				   TPM_AUTH *ownerAuth,		/* in, out */
				   UINT32 *blobSize,		/* out */
				   BYTE **blob)			/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Delegate_CreateOwnerDelegation_TP(entry, increment,
								       publicInfoSize, publicInfo,
								       encDelAuth, ownerAuth,
								       blobSize, blob);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_Delegate_LoadOwnerDelegation(TSS_HCONTEXT hContext,	/* in */
				 TPM_DELEGATE_INDEX index,	/* in */
				 UINT32 blobSize,		/* in */
				 BYTE *blob,			/* in */
				 TPM_AUTH *ownerAuth)		/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Delegate_LoadOwnerDelegation_TP(entry, index, blobSize, blob,
								     ownerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_Delegate_ReadTable(TSS_HCONTEXT hContext,		/* in */
		       UINT32 *familyTableSize,		/* out */
		       BYTE **familyTable,		/* out */
		       UINT32 *delegateTableSize,	/* out */
		       BYTE **delegateTable)		/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Delegate_ReadTable_TP(entry, familyTableSize, familyTable,
							   delegateTableSize, delegateTable);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_Delegate_UpdateVerificationCount(TSS_HCONTEXT hContext,	/* in */
				     UINT32 inputSize,		/* in */
				     BYTE *input,		/* in */
				     TPM_AUTH *ownerAuth,	/* in, out */
				     UINT32 *outputSize,	/* out */
				     BYTE **output)		/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Delegate_UpdateVerificationCount_TP(entry, inputSize, input,
									 ownerAuth, outputSize,
									 output);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_Delegate_VerifyDelegation(TSS_HCONTEXT hContext,	/* in */
			      UINT32 delegateSize,	/* in */
			      BYTE *delegate)		/* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_Delegate_VerifyDelegation_TP(entry, delegateSize, delegate);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_DSAP(TSS_HCONTEXT hContext,		/* in */
	 TPM_ENTITY_TYPE entityType,	/* in */
	 TCS_KEY_HANDLE keyHandle,	/* in */
	 TPM_NONCE *nonceOddDSAP,	/* in */
	 UINT32 entityValueSize,	/* in */
	 BYTE * entityValue,		/* in */
	 TCS_AUTHHANDLE *authHandle,	/* out */
	 TPM_NONCE *nonceEven,		/* out */
	 TPM_NONCE *nonceEvenDSAP)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_DSAP_TP(entry, entityType, keyHandle, nonceOddDSAP,
					     entityValueSize, entityValue, authHandle, nonceEven,
					     nonceEvenDSAP);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

#endif

#ifdef TSS_BUILD_CMK
TSS_RESULT
RPC_CMK_SetRestrictions(TSS_HCONTEXT hContext,	/* in */
			TSS_CMK_DELEGATE restriction,	/* in */
			TPM_AUTH *ownerAuth)		/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CMK_SetRestrictions_TP(entry, restriction, ownerAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_CMK_ApproveMA(TSS_HCONTEXT hContext,		/* in */
		  TPM_DIGEST migAuthorityDigest,	/* in */
		  TPM_AUTH *ownerAuth,			/* in, out */
		  TPM_HMAC *migAuthorityApproval)	/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CMK_ApproveMA_TP(entry, migAuthorityDigest, ownerAuth,
					migAuthorityApproval);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_CMK_CreateKey(TSS_HCONTEXT hContext,		/* in */
		  TCS_KEY_HANDLE hWrappingKey,		/* in */
		  TPM_ENCAUTH *keyUsageAuth,		/* in */
		  TPM_HMAC *migAuthorityApproval,	/* in */
		  TPM_DIGEST *migAuthorityDigest,	/* in */
		  UINT32 *keyDataSize,			/* in, out */
		  BYTE **keyData,			/* in, out */
		  TPM_AUTH *pAuth)			/* in, out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CMK_CreateKey_TP(entry, hWrappingKey, keyUsageAuth,
					migAuthorityApproval, migAuthorityDigest, keyDataSize,
					keyData, pAuth);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_CMK_CreateTicket(TSS_HCONTEXT hContext,	/* in */
		     UINT32 publicVerifyKeySize,	/* in */
		     BYTE *publicVerifyKey,		/* in */
		     TPM_DIGEST signedData,		/* in */
		     UINT32 sigValueSize,		/* in */
		     BYTE *sigValue,			/* in */
		     TPM_AUTH *ownerAuth,		/* in, out */
		     TPM_HMAC *sigTicket)		/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CMK_CreateTicket_TP(entry, publicVerifyKeySize,
					publicVerifyKey, signedData, sigValueSize, sigValue,
					ownerAuth, sigTicket);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_CMK_CreateBlob(TSS_HCONTEXT hContext,	/* in */
		   TCS_KEY_HANDLE hParentKey,		/* in */
		   TSS_MIGRATE_SCHEME migrationType,	/* in */
		   UINT32 migKeyAuthSize,		/* in */
		   BYTE *migKeyAuth,			/* in */
		   TPM_DIGEST pubSourceKeyDigest,	/* in */
		   UINT32 msaListSize,			/* in */
		   BYTE *msaList,			/* in */
		   UINT32 restrictTicketSize,		/* in */
		   BYTE *restrictTicket,		/* in */
		   UINT32 sigTicketSize,		/* in */
		   BYTE *sigTicket,			/* in */
		   UINT32 encDataSize,			/* in */
		   BYTE *encData,			/* in */
		   TPM_AUTH *pAuth,			/* in, out */
		   UINT32 *randomSize,			/* out */
		   BYTE **random,			/* out */
		   UINT32 *outDataSize,			/* out */
		   BYTE **outData)			/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CMK_CreateBlob_TP(entry, hParentKey, migrationType,
					migKeyAuthSize, migKeyAuth, pubSourceKeyDigest,
					msaListSize, msaList, restrictTicketSize, restrictTicket,
					sigTicketSize, sigTicket, encDataSize, encData, pAuth,
					randomSize, random, outDataSize, outData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_CMK_ConvertMigration(TSS_HCONTEXT hContext,	/* in */
			 TCS_KEY_HANDLE hParentHandle,	/* in */
			 TPM_CMK_AUTH restrictTicket,	/* in */
			 TPM_HMAC sigTicket,		/* in */
			 UINT32 keyDataSize,		/* in */
			 BYTE *keyData,			/* in */
			 UINT32 msaListSize,		/* in */
			 BYTE *msaList,			/* in */	
			 UINT32 randomSize,		/* in */
			 BYTE *random,			/* in */
			 TPM_AUTH *pAuth,		/* in, out */
			 UINT32 *outDataSize,		/* out */
			 BYTE **outData)		/* out */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_CMK_ConvertMigration_TP(entry, hParentHandle, restrictTicket,
					sigTicket, keyDataSize, keyData, msaListSize, msaList,
					randomSize, random, pAuth, outDataSize, outData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

#ifdef TSS_BUILD_TSS12
TSS_RESULT
RPC_FlushSpecific(TSS_HCONTEXT hContext, /* in */
		  TCS_HANDLE hResHandle, /* in */
		  TPM_RESOURCE_TYPE resourceType) /* in */
{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_FlushSpecific_TP(entry, hResHandle, resourceType);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}

TSS_RESULT
RPC_KeyControlOwner(TCS_CONTEXT_HANDLE hContext,		/* in */
		    TCS_KEY_HANDLE     hKey,			/* in */
		    UINT32             ulPublicInfoLength,	/* in */
		    BYTE*              rgbPublicInfo,		/* in */
		    UINT32             attribName,		/* in */
		    TSS_BOOL           attribValue,		/* in */
		    TPM_AUTH*          pOwnerAuth,		/* in, out */
		    TSS_UUID*          pUuidData)		/* out */

{
	TSS_RESULT result = (TSS_E_INTERNAL_ERROR | TSS_LAYER_TSP);
	struct host_table_entry *entry = get_table_entry(hContext);

	if (entry == NULL)
		return TSPERR(TSS_E_NO_CONNECTION);

	switch (entry->type) {
		case CONNECTION_TYPE_TCP_PERSISTANT:
			result = RPC_KeyControlOwner_TP(entry, hKey,
							ulPublicInfoLength,
							rgbPublicInfo,
							attribName,
							attribValue,
							pOwnerAuth,
							pUuidData);
			break;
		default:
			break;
	}

	put_table_entry(entry);

	return result;
}
#endif

