
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

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "tcs_tsp.h"
#include "tspps.h"
#include "hosttable.h"
#include "tcsd_wrap.h"
#include "tcsd.h"
#include "obj.h"

TSS_UUID owner_evict_uuid = {0, 0, 0, 0, 0, {0, 0, 0, 0, 1, 0}};

TSS_RESULT
Tspi_Context_LoadKeyByUUID(TSS_HCONTEXT tspContext,		/* in */
			   TSS_FLAG persistentStorageType,	/* in */
			   TSS_UUID uuidData,			/* in */
			   TSS_HKEY * phKey)			/* out */
{
	TSS_RESULT result;
	TSS_UUID parentUUID;
	UINT32 keyBlobSize, parentPSType;
	BYTE *keyBlob = NULL;
	TCS_KEY_HANDLE tcsKeyHandle;
	TSS_HKEY parentTspHandle;
	TCS_LOADKEY_INFO info;
	UINT32		ulPubKeyLength;
	BYTE		*rgbPubKey;

	if (phKey == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((!obj_is_context(tspContext)))
		return TSPERR(TSS_E_INVALID_HANDLE);

	/* This key is in the System Persistant storage */
	if (persistentStorageType == TSS_PS_TYPE_SYSTEM) {
#if 1
		memset(&info, 0, sizeof(TCS_LOADKEY_INFO));

		result = RPC_LoadKeyByUUID(tspContext, uuidData, &info, &tcsKeyHandle);

		if (TSS_ERROR_CODE(result) == TCS_E_KM_LOADFAILED) {
			TSS_HKEY keyHandle;
			TSS_HPOLICY hPolicy;

			/* load failed, due to some key in the chain needing auth
			 * which doesn't yet exist at the TCS level. However, the
			 * auth may already be set in policies at the TSP level.
			 * To find out, get the key handle of the key requiring
			 * auth. First, look at the list of keys in memory. */
			if ((obj_rsakey_get_by_uuid(&info.parentKeyUUID, &keyHandle))) {
				/* If that failed, look on disk, in User PS. */
				if (ps_get_key_by_uuid(tspContext, &info.parentKeyUUID,
						       &keyHandle))
					return result;
			}

			if (obj_rsakey_get_policy(keyHandle, TSS_POLICY_USAGE,
						  &hPolicy, NULL))
				return result;

			if (secret_PerformAuth_OIAP(keyHandle, TPM_ORD_LoadKey, hPolicy, FALSE,
						    &info.paramDigest, &info.authData))
				return result;

			if ((result = RPC_LoadKeyByUUID(tspContext, uuidData, &info,
							&tcsKeyHandle)))
				return result;
		} else if (result)
			return result;

		/*check if provided UUID has an owner evict key UUID prefix */
		if (!memcmp(&uuidData, &owner_evict_uuid, sizeof(TSS_UUID)-1)) {
			if ((result = obj_rsakey_add(tspContext, TSS_RSAKEY_FLAG_OWNEREVICT,
						      phKey)))
				return result;
			if ((result = obj_rsakey_set_tcs_handle(*phKey, tcsKeyHandle)))
				return result;

			//The cached public key portion of the owner evict key is used 
			//further by TPM_KEY_CONTROLOWNER command for sanity check
			if ((result = Tspi_Key_GetPubKey(*phKey, &ulPubKeyLength, &rgbPubKey)))
				return result;

			result = obj_rsakey_set_pubkey(*phKey, FALSE, rgbPubKey);

			free(rgbPubKey);		
			if (result != TSS_SUCCESS)
				return result;
		} else {
			if ((result = RPC_GetRegisteredKeyBlob(tspContext, uuidData, &keyBlobSize,
							       &keyBlob)))
				return result;

			if ((result = obj_rsakey_add_by_key(tspContext, &uuidData, keyBlob,
							    TSS_OBJ_FLAG_SYSTEM_PS, phKey))) {
				free (keyBlob);
				return result;
			}
	
			result = obj_rsakey_set_tcs_handle(*phKey, tcsKeyHandle);

			free (keyBlob);
		}
#else
		if ((result = load_from_system_ps(tspContext, &uuidData, phKey)))
			return result;
#endif
	} else if (persistentStorageType == TSS_PS_TYPE_USER) {
		if ((result = ps_get_parent_uuid_by_uuid(&uuidData, &parentUUID)))
			return result;

		/* If the parent is not in memory, recursively call ourselves on it */
		if (obj_rsakey_get_by_uuid(&parentUUID, &parentTspHandle) != TSS_SUCCESS) {
			if ((result = ps_get_parent_ps_type_by_uuid(&uuidData, &parentPSType)))
				return result;

			if ((result = Tspi_Context_LoadKeyByUUID(tspContext, parentPSType,
								 parentUUID, &parentTspHandle)))
				return result;
		}

		if ((result = ps_get_key_by_uuid(tspContext, &uuidData, phKey)))
			return result;

		/* The parent is loaded and we have the parent key handle, so call the TCS to
		 * actually load the child. */
		return Tspi_Key_LoadKey(*phKey, parentTspHandle);
	} else {
		return TSPERR(TSS_E_BAD_PARAMETER);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_Context_RegisterKey(TSS_HCONTEXT tspContext,		/* in */
			 TSS_HKEY hKey,				/* in */
			 TSS_FLAG persistentStorageType,	/* in */
			 TSS_UUID uuidKey,			/* in */
			 TSS_FLAG persistentStorageTypeParent,	/* in */
			 TSS_UUID uuidParentKey)		/* in */
{
	BYTE *keyBlob;
	UINT32 keyBlobSize;
	TSS_RESULT result;
	TSS_BOOL answer;

	if (!obj_is_context(tspContext) || !obj_is_rsakey(hKey))
		return TSPERR(TSS_E_INVALID_HANDLE);

	if (persistentStorageType == TSS_PS_TYPE_SYSTEM) {
		if (persistentStorageTypeParent == TSS_PS_TYPE_USER) {
			return TSPERR(TSS_E_NOTIMPL);
		} else if (persistentStorageTypeParent == TSS_PS_TYPE_SYSTEM) {
			if ((result = obj_rsakey_get_blob(hKey, &keyBlobSize,
							  &keyBlob)))
				return result;

			if ((result = RPC_RegisterKey(tspContext, uuidParentKey, uuidKey,
						      keyBlobSize, keyBlob,
						      strlen(PACKAGE_STRING) + 1,
						      (BYTE *)PACKAGE_STRING)))
				return result;
		} else {
			return TSPERR(TSS_E_BAD_PARAMETER);
		}
	} else if (persistentStorageType == TSS_PS_TYPE_USER) {
		if ((result = ps_is_key_registered(&uuidKey, &answer)))
			return result;

		if (answer == TRUE)
			return TSPERR(TSS_E_KEY_ALREADY_REGISTERED);

		if ((result = obj_rsakey_get_blob (hKey, &keyBlobSize, &keyBlob)))
			return result;

		if ((result = ps_write_key(&uuidKey, &uuidParentKey,
					   persistentStorageTypeParent,
					   keyBlobSize, keyBlob)))
			return result;
	} else {
		return TSPERR(TSS_E_BAD_PARAMETER);
	}

	if ((result = obj_rsakey_set_uuid(hKey, persistentStorageType, &uuidKey)))
		return result;

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_Context_UnregisterKey(TSS_HCONTEXT tspContext,		/* in */
			   TSS_FLAG persistentStorageType,	/* in */
			   TSS_UUID uuidKey,			/* in */
			   TSS_HKEY *phKey)			/* out */
{
	BYTE *keyBlob = NULL;
	UINT32 keyBlobSize;
	TSS_RESULT result;

	if (phKey == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((!obj_is_context(tspContext)))
		return TSPERR(TSS_E_INVALID_HANDLE);

	if (persistentStorageType == TSS_PS_TYPE_SYSTEM) {
		/* get the key first, so it doesn't disappear when we
		 * unregister it */
		if ((result = RPC_GetRegisteredKeyBlob(tspContext, uuidKey, &keyBlobSize,
						       &keyBlob)))
			return result;

		if ((obj_rsakey_add_by_key(tspContext, &uuidKey, keyBlob, TSS_OBJ_FLAG_SYSTEM_PS,
					   phKey))) {
			free(keyBlob);
			return result;
		}

		free(keyBlob);

		/* now unregister it */
		if ((result = RPC_UnregisterKey(tspContext, uuidKey)))
			return result;
	} else if (persistentStorageType == TSS_PS_TYPE_USER) {
		/* get the key first, so it doesn't disappear when we
		 * unregister it */
		if ((result = ps_get_key_by_uuid(tspContext, &uuidKey, phKey)))
			return result;

		/* now unregister it */
		if ((result = ps_remove_key(&uuidKey)))
			return result;
	} else {
		return TSPERR(TSS_E_BAD_PARAMETER);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_Context_GetKeyByUUID(TSS_HCONTEXT tspContext,		/* in */
			  TSS_FLAG persistentStorageType,	/* in */
			  TSS_UUID uuidData,			/* in */
			  TSS_HKEY * phKey)			/* out */
{
	TCPA_RESULT result;
	UINT32 keyBlobSize = 0;
	BYTE *keyBlob = NULL;

	if (phKey == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((!obj_is_context(tspContext)))
		return TSPERR(TSS_E_INVALID_HANDLE);

	if (persistentStorageType == TSS_PS_TYPE_SYSTEM) {
		if ((result = RPC_GetRegisteredKeyBlob(tspContext, uuidData, &keyBlobSize,
						       &keyBlob)))
			return result;

		if ((obj_rsakey_add_by_key(tspContext, &uuidData, keyBlob, TSS_OBJ_FLAG_SYSTEM_PS,
					   phKey))) {
			free(keyBlob);
			return result;
		}

		free(keyBlob);
	} else if (persistentStorageType == TSS_PS_TYPE_USER) {
		if (!obj_is_context(tspContext))
			return TSPERR(TSS_E_INVALID_HANDLE);

		if ((result = ps_get_key_by_uuid(tspContext, &uuidData, phKey)))
			return result;
	} else
		return TSPERR(TSS_E_BAD_PARAMETER);

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_Context_GetKeyByPublicInfo(TSS_HCONTEXT tspContext,	/* in */
				TSS_FLAG persistentStorageType,	/* in */
				TSS_ALGORITHM_ID algID,		/* in */
				UINT32 ulPublicInfoLength,	/* in */
				BYTE * rgbPublicInfo,		/* in */
				TSS_HKEY * phKey)		/* out */
{
	TCPA_ALGORITHM_ID tcsAlgID;
	UINT32 keyBlobSize;
	BYTE *keyBlob;
	TSS_RESULT result;
	TSS_HKEY keyOutHandle;
	UINT32 flag = 0;
	TSS_KEY keyContainer;
	UINT64 offset;

	if (phKey == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (!obj_is_context(tspContext))
		return TSPERR(TSS_E_INVALID_HANDLE);

	switch (algID) {
		case TSS_ALG_RSA:
			tcsAlgID = TCPA_ALG_RSA;
			break;
		default:
			LogError("Algorithm ID was not type RSA.");
			return TSPERR(TSS_E_BAD_PARAMETER);
	}

	if (persistentStorageType == TSS_PS_TYPE_SYSTEM) {
		if ((result = RPC_GetRegisteredKeyByPublicInfo(tspContext, tcsAlgID,
							       ulPublicInfoLength, rgbPublicInfo,
							       &keyBlobSize, &keyBlob)))
			return result;

	} else if (persistentStorageType == TSS_PS_TYPE_USER) {
		return ps_get_key_by_pub(tspContext, ulPublicInfoLength, rgbPublicInfo,
					 phKey);
	} else
		return TSPERR(TSS_E_BAD_PARAMETER);

	/* need to setup the init flags of the create object based on
	 * the size of the blob's pubkey */
	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, keyBlob, &keyContainer))) {
		free(keyBlob);
		return result;
	}

	/* begin setting up the key object */
	switch (keyContainer.pubKey.keyLength) {
		case 16384/8:
			flag |= TSS_KEY_SIZE_16384;
			break;
		case 8192/8:
			flag |= TSS_KEY_SIZE_8192;
			break;
		case 4096/8:
			flag |= TSS_KEY_SIZE_4096;
			break;
		case 2048/8:
			flag |= TSS_KEY_SIZE_2048;
			break;
		case 1024/8:
			flag |= TSS_KEY_SIZE_1024;
			break;
		case 512/8:
			flag |= TSS_KEY_SIZE_512;
			break;
		default:
			LogError("Key was not a known keylength.");
			free(keyBlob);
			free_key_refs(&keyContainer);
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if (keyContainer.keyUsage == TPM_KEY_SIGNING)
		flag |= TSS_KEY_TYPE_SIGNING;
	else if (keyContainer.keyUsage == TPM_KEY_STORAGE)
		flag |= TSS_KEY_TYPE_STORAGE;
	else if (keyContainer.keyUsage == TPM_KEY_IDENTITY)
		flag |= TSS_KEY_TYPE_IDENTITY;
	else if (keyContainer.keyUsage == TPM_KEY_AUTHCHANGE)
		flag |= TSS_KEY_TYPE_AUTHCHANGE;
	else if (keyContainer.keyUsage == TPM_KEY_BIND)
		flag |= TSS_KEY_TYPE_BIND;
	else if (keyContainer.keyUsage == TPM_KEY_LEGACY)
		flag |= TSS_KEY_TYPE_LEGACY;

	if (keyContainer.authDataUsage == TPM_AUTH_NEVER)
		flag |= TSS_KEY_NO_AUTHORIZATION;
	else
		flag |= TSS_KEY_AUTHORIZATION;

	if (keyContainer.keyFlags & TPM_MIGRATABLE)
		flag |= TSS_KEY_MIGRATABLE;
	else
		flag |= TSS_KEY_NOT_MIGRATABLE;

	if (keyContainer.keyFlags & TPM_VOLATILE)
		flag |= TSS_KEY_VOLATILE;
	else
		flag |= TSS_KEY_NON_VOLATILE;

#ifdef TSS_BUILD_CMK
	if (keyContainer.keyFlags & TPM_MIGRATEAUTHORITY)
		flag |= TSS_KEY_CERTIFIED_MIGRATABLE;
	else
		flag |= TSS_KEY_NOT_CERTIFIED_MIGRATABLE;
#endif

	/* Create a new Key Object */
	if ((result = obj_rsakey_add(tspContext, flag, &keyOutHandle))) {
		free(keyBlob);
		free_key_refs(&keyContainer);
		return result;
	}
	/* Stick the info into this net KeyObject */
	if ((result = obj_rsakey_set_tcpakey(keyOutHandle, keyBlobSize, keyBlob))) {
		free(keyBlob);
		free_key_refs(&keyContainer);
		return result;
	}

	free(keyBlob);
	free_key_refs(&keyContainer);
	*phKey = keyOutHandle;

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_Context_GetRegisteredKeysByUUID(TSS_HCONTEXT tspContext,		/* in */
				     TSS_FLAG persistentStorageType,	/* in */
				     TSS_UUID * pUuidData,		/* in */
				     UINT32 * pulKeyHierarchySize,	/* out */
				     TSS_KM_KEYINFO ** ppKeyHierarchy)	/* out */
{
	TSS_RESULT result;
	TSS_KM_KEYINFO *tcsHier, *tspHier;
	UINT32 tcsHierSize, tspHierSize;
	TSS_UUID tcs_uuid;

	if (pulKeyHierarchySize == NULL || ppKeyHierarchy == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (!obj_is_context(tspContext))
		return TSPERR(TSS_E_INVALID_HANDLE);

	if (pUuidData) {
		if (persistentStorageType == TSS_PS_TYPE_SYSTEM) {
			if ((result = RPC_EnumRegisteredKeys(tspContext, pUuidData,
							     pulKeyHierarchySize,
							     ppKeyHierarchy)))
				return result;
		} else if (persistentStorageType == TSS_PS_TYPE_USER) {
			if ((result = ps_get_registered_keys(pUuidData, &tcs_uuid,
							     &tspHierSize, &tspHier)))
				return result;

			if ((result = RPC_EnumRegisteredKeys(tspContext, &tcs_uuid, &tcsHierSize,
							     &tcsHier))) {
				free(tspHier);
				return result;
			}

			result = merge_key_hierarchies(tspContext, tspHierSize, tspHier,
						       tcsHierSize, tcsHier, pulKeyHierarchySize,
						       ppKeyHierarchy);
			free(tcsHier);
			free(tspHier);
		} else
			return TSPERR(TSS_E_BAD_PARAMETER);
	} else {
		if ((result = RPC_EnumRegisteredKeys(tspContext, pUuidData, &tcsHierSize,
						     &tcsHier)))
			return result;

		if ((result = ps_get_registered_keys(pUuidData, NULL, &tspHierSize, &tspHier))) {
			free(tcsHier);
			return result;
		}

		result = merge_key_hierarchies(tspContext, tspHierSize, tspHier, tcsHierSize,
					       tcsHier, pulKeyHierarchySize, ppKeyHierarchy);
		free(tcsHier);
		free(tspHier);
	}

	if ((result = __tspi_add_mem_entry(tspContext, *ppKeyHierarchy))) {
		free(*ppKeyHierarchy);
		*ppKeyHierarchy = NULL;
		*pulKeyHierarchySize = 0;
	}

	return result;
}

TSS_RESULT
Tspi_Context_GetRegisteredKeysByUUID2(TSS_HCONTEXT tspContext,		/* in */
				     TSS_FLAG persistentStorageType,	/* in */
				     TSS_UUID * pUuidData,		/* in */
				     UINT32 * pulKeyHierarchySize,	/* out */
				     TSS_KM_KEYINFO2 ** ppKeyHierarchy)	/* out */
{
	TSS_RESULT result;
	TSS_KM_KEYINFO2 *tcsHier, *tspHier;
	UINT32 tcsHierSize, tspHierSize;
	TSS_UUID tcs_uuid;

	/* If out parameters are NULL, return error */
	if (pulKeyHierarchySize == NULL || ppKeyHierarchy == NULL)
			return TSPERR(TSS_E_BAD_PARAMETER);

	if (!obj_is_context(tspContext))
			return TSPERR(TSS_E_INVALID_HANDLE);

	if (pUuidData) {
		/* TSS 1.2 Spec: If a certain key UUID is provided, the returned array of
		 * TSS_KM_KEYINFO2 structures only contains data reflecting the path of the key
		 * hierarchy regarding that key. The first array entry is the key addressed by the
		 * given UUID followed by its parent key up to and including the root key. */
		if (persistentStorageType == TSS_PS_TYPE_SYSTEM) {
			if ((result = RPC_EnumRegisteredKeys2(tspContext, pUuidData,
							      pulKeyHierarchySize, ppKeyHierarchy)))
				return result;
		} else if (persistentStorageType == TSS_PS_TYPE_USER) {
			if ((result = ps_get_registered_keys2(pUuidData, &tcs_uuid, &tspHierSize,
							      &tspHier)))
				return result;
			/* The tcs_uuid returned by ps_get_registered_key2 will always be a parent
			 * of some key into the system ps of a user key into the user ps. This key
			 * needs to be searched for in the system ps to be merged */
			if ((result = RPC_EnumRegisteredKeys2(tspContext, &tcs_uuid, &tcsHierSize,
							      &tcsHier))) {
				free(tspHier);
				return result;
			}

			result = merge_key_hierarchies2(tspContext, tspHierSize, tspHier,
							tcsHierSize, tcsHier, pulKeyHierarchySize,
							ppKeyHierarchy);
			free(tcsHier);
			free(tspHier);
		} else
			return TSPERR(TSS_E_BAD_PARAMETER);
	} else {
		/* If this field is set to NULL, the returned array of TSS_KM_KEYINFO2 structures
		 * contains data reflecting the entire key hierarchy starting with root key. The
		 * array will include keys from both the user and the system TSS key store. The
		 * persistentStorageType field will be ignored. */
		if ((result = RPC_EnumRegisteredKeys2(tspContext, pUuidData, &tcsHierSize,
						      &tcsHier)))
			return result;

		if ((result = ps_get_registered_keys2(pUuidData, NULL, &tspHierSize, &tspHier))) {
			free(tcsHier);
			return result;
		}

		result = merge_key_hierarchies2(tspContext, tspHierSize, tspHier, tcsHierSize,
						tcsHier, pulKeyHierarchySize, ppKeyHierarchy);
		free(tcsHier);
		free(tspHier);
	}

	if ((result = __tspi_add_mem_entry(tspContext, *ppKeyHierarchy))) {
		free(*ppKeyHierarchy);
		*ppKeyHierarchy = NULL;
		*pulKeyHierarchySize = 0;
	}

	return result;
}
