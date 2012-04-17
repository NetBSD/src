
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

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "tcsps.h"

TSS_RESULT
TCS_RegisterKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			 TSS_UUID *WrappingKeyUUID,	/* in */
			 TSS_UUID *KeyUUID,		/* in */
			 UINT32 cKeySize,		/* in */
			 BYTE * rgbKey,			/* in */
			 UINT32 cVendorData,		/* in */
			 BYTE * gbVendorData)		/* in */
{
	TSS_RESULT result;
	TSS_BOOL is_reg;

	if ((result = ctx_verify_context(hContext)))
		return result;

	/* Check if key is already regisitered */
	if (isUUIDRegistered(KeyUUID, &is_reg) != TSS_SUCCESS) {
		LogError("Failed checking if UUID is registered.");
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (is_reg == TRUE || TSS_UUID_IS_OWNEREVICT(KeyUUID)) {
		LogDebug("UUID is already registered");
		return TCSERR(TSS_E_KEY_ALREADY_REGISTERED);
	}

	LogDebugUnrollKey(rgbKey);

	/* Go ahead and store it in system persistant storage */
	if ((result = ps_write_key(KeyUUID, WrappingKeyUUID, gbVendorData, cVendorData, rgbKey,
				   cKeySize))) {
		LogError("Error writing key to file");
		return result;
	}

	return TSS_SUCCESS;
}

TSS_RESULT
TCS_UnregisterKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			   TSS_UUID KeyUUID)		/* in */
{
	TSS_RESULT result;

	if ((result = ctx_verify_context(hContext)))
		return result;

	return ps_remove_key(&KeyUUID);
}

TSS_RESULT
TCS_EnumRegisteredKeys_Internal(TCS_CONTEXT_HANDLE hContext,		/* in */
				TSS_UUID * pKeyUUID,			/* in */
				UINT32 * pcKeyHierarchySize,		/* out */
				TSS_KM_KEYINFO ** ppKeyHierarchy)	/* out */
{
	TSS_RESULT result = TSS_SUCCESS;
	UINT32 count = 0, i;
	TSS_KM_KEYINFO *ret = NULL;
	TSS_UUID tmp_uuid;
	struct key_disk_cache *disk_ptr, *tmp_ptrs[MAX_KEY_CHILDREN];
	struct key_mem_cache *mem_ptr;
	TSS_BOOL is_reg = FALSE;

	LogDebug("Enum Reg Keys");

	if (pcKeyHierarchySize == NULL || ppKeyHierarchy == NULL)
		return TCSERR(TSS_E_BAD_PARAMETER);

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (pKeyUUID != NULL) {
		/* First have to verify the key is registered */
		if ((result = isUUIDRegistered(pKeyUUID, &is_reg)))
			return result;

		if (is_reg == FALSE) {
			/* This return code is not listed as possible in the TSS 1.1 spec,
			 * but it makes more sense than just TCS_SUCCESS or TSS_E_FAIL */
			return TCSERR(TSS_E_PS_KEY_NOTFOUND);
		}
	}

	/* this entire operation needs to be atomic wrt registered keys. We must
	 * lock the mem cache as well to test if a given key is loaded. */
	MUTEX_LOCK(disk_cache_lock);
	MUTEX_LOCK(mem_cache_lock);

	/* return an array of all registered keys if pKeyUUID == NULL */
	if (pKeyUUID == NULL) {
		/*  determine the number of registered keys */
		for (disk_ptr = key_disk_cache_head; disk_ptr; disk_ptr = disk_ptr->next) {
			if (disk_ptr->flags & CACHE_FLAG_VALID)
				count++;
		}

		/* malloc a structure for each of them */
		if (count != 0) {
			ret = calloc(count, sizeof(TSS_KM_KEYINFO));
			if (ret == NULL) {
				LogError("malloc of %zd bytes failed.",
						(count * sizeof(TSS_KM_KEYINFO)));
				count = 0;
				result = TCSERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
		} else {
			goto done;
		}

		/* fill out the structure for each key */
		i = 0;
		for (disk_ptr = key_disk_cache_head; disk_ptr; disk_ptr = disk_ptr->next) {
			if (disk_ptr->flags & CACHE_FLAG_VALID) {
				/* look for a mem cache entry to check if its loaded */
				for (mem_ptr = key_mem_cache_head; mem_ptr; mem_ptr = mem_ptr->next) {
					if (!memcmp(&mem_ptr->uuid, &disk_ptr->uuid, sizeof(TSS_UUID))) {
						if ((result = fill_key_info(disk_ptr, mem_ptr, &ret[i]))) {
							free(ret);
							ret = NULL;
							count = 0;
							goto done;
						}
						break;
					}
				}
				/* if there is no mem cache entry for this key, go ahead and call
				 * fill_key_info(), it will pull everything from disk */
				if (mem_ptr == NULL) {
					if ((result = fill_key_info(disk_ptr, NULL, &ret[i]))) {
						free(ret);
						ret = NULL;
						count = 0;
						goto done;
					}
				}
				i++;
			}
		}
	} else {
		/* return a chain of a key and its parents up to the SRK */
		/*  determine the number of keys in the chain */
		memcpy(&tmp_uuid, pKeyUUID, sizeof(TSS_UUID));
		disk_ptr = key_disk_cache_head;
		while (disk_ptr != NULL && count < MAX_KEY_CHILDREN)
		{
			if (disk_ptr->flags & CACHE_FLAG_VALID &&
				!memcmp(&disk_ptr->uuid, &tmp_uuid, sizeof(TSS_UUID)))
			{
				/* increment count, then search for the parent */
				count++;
				/* save a pointer to this cache entry */
				tmp_ptrs[count - 1] = disk_ptr;
				/* if the parent of this key is NULL, we're at the root of the tree */
				if (!memcmp(&disk_ptr->parent_uuid, &NULL_UUID, sizeof(TSS_UUID)))
					break;
				/* overwrite tmp_uuid with the parent, which we will now search for */
				memcpy(&tmp_uuid, &disk_ptr->parent_uuid, sizeof(TSS_UUID));
				disk_ptr = key_disk_cache_head;
				continue;
			}
			disk_ptr = disk_ptr->next;
		}
		/* when we reach this point, we have an array of TSS_UUID's that leads from the
		 * requested key up to the SRK*/

		/* malloc a structure for each of them */
		if (count != 0) {
			ret = calloc(count, sizeof(TSS_KM_KEYINFO));
			if (ret == NULL) {
				LogError("malloc of %zd bytes failed.",
						(count * sizeof(TSS_KM_KEYINFO)));
				count = 0;
				result = TCSERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
		} else {
			goto done;
		}

		for (i = 0; i < count; i++) {
			/* look for a mem cache entry to check if its loaded */
			for (mem_ptr = key_mem_cache_head; mem_ptr; mem_ptr = mem_ptr->next) {
				if (!memcmp(&mem_ptr->uuid, &tmp_ptrs[i]->uuid, sizeof(TSS_UUID))) {
					if ((result = fill_key_info(tmp_ptrs[i], mem_ptr, &ret[i]))) {
						free(ret);
						ret = NULL;
						count = 0;
						goto done;
					}
					break;
				}
			}
			/* if there is no mem cache entry for this key, go ahead and call
			 * fill_key_info(), it will pull everything from disk */
			if (mem_ptr == NULL) {
				if ((result = fill_key_info(tmp_ptrs[i], NULL, &ret[i]))) {
					free(ret);
					ret = NULL;
					count = 0;
					goto done;
				}
			}
		}
	}
done:

	MUTEX_UNLOCK(disk_cache_lock);
	MUTEX_UNLOCK(mem_cache_lock);

	*ppKeyHierarchy = ret;
	*pcKeyHierarchySize = count;

	return result;
}

TSS_RESULT
TCS_EnumRegisteredKeys_Internal2(TCS_CONTEXT_HANDLE hContext,		/* in */
				TSS_UUID * pKeyUUID,			/* in */
				UINT32 * pcKeyHierarchySize,		/* out */
				TSS_KM_KEYINFO2 ** ppKeyHierarchy)	/* out */
{
	TSS_RESULT result = TSS_SUCCESS;
	UINT32 count = 0, i;
	TSS_KM_KEYINFO2 *ret = NULL;
	TSS_UUID tmp_uuid;
	struct key_disk_cache *disk_ptr, *tmp_ptrs[MAX_KEY_CHILDREN];
	struct key_mem_cache *mem_ptr;
	TSS_BOOL is_reg = FALSE;

	LogDebug("Enum Reg Keys2");

	if (pcKeyHierarchySize == NULL || ppKeyHierarchy == NULL)
		return TCSERR(TSS_E_BAD_PARAMETER);

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (pKeyUUID != NULL) {
		/* First have to verify the key is registered */
		if ((result = isUUIDRegistered(pKeyUUID, &is_reg)))
			return result;

		if (is_reg == FALSE) {
			/* This return code is not listed as possible in the TSS 1.1 spec,
			 * but it makes more sense than just TCS_SUCCESS or TSS_E_FAIL */
			return TCSERR(TSS_E_PS_KEY_NOTFOUND);
		}
	}

	/* this entire operation needs to be atomic wrt registered keys. We must
	 * lock the mem cache as well to test if a given key is loaded. */
	MUTEX_LOCK(disk_cache_lock);
	MUTEX_LOCK(mem_cache_lock);

	/* return an array of all registered keys if pKeyUUID == NULL */
	if (pKeyUUID == NULL) {
		/*  determine the number of registered keys */
		for (disk_ptr = key_disk_cache_head; disk_ptr; disk_ptr = disk_ptr->next) {
			if (disk_ptr->flags & CACHE_FLAG_VALID)
				count++;
		}

		/* malloc a structure for each of them */
		if (count != 0) {
			ret = calloc(count, sizeof(TSS_KM_KEYINFO2));
			if (ret == NULL) {
				LogError("malloc of %zd bytes failed.",
						(count * sizeof(TSS_KM_KEYINFO2)));
				count = 0;
				result = TCSERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
		} else {
			goto done;
		}

		/* fill out the structure for each key */
		i = 0;
		for (disk_ptr = key_disk_cache_head; disk_ptr; disk_ptr = disk_ptr->next) {
			if (disk_ptr->flags & CACHE_FLAG_VALID) {
				/* look for a mem cache entry to check if its loaded */
				for (mem_ptr = key_mem_cache_head; mem_ptr; mem_ptr = mem_ptr->next) {
					if (!memcmp(&mem_ptr->uuid, &disk_ptr->uuid, sizeof(TSS_UUID))) {
						if ((result = fill_key_info2(disk_ptr, mem_ptr, &ret[i]))) {
							free(ret);
							ret = NULL;
							count = 0;
							goto done;
						}
						break;
					}
				}
				/* if there is no mem cache entry for this key, go ahead and call
				 * fill_key_info2(), it will pull everything from disk */
				if (mem_ptr == NULL) {
					if ((result = fill_key_info2(disk_ptr, NULL, &ret[i]))) {
						free(ret);
						ret = NULL;
						count = 0;
						goto done;
					}
				}
				i++;
			}
		}
	} else {
		/* return a chain of a key and its parents up to the SRK */
		/*  determine the number of keys in the chain */
		memcpy(&tmp_uuid, pKeyUUID, sizeof(TSS_UUID));
		disk_ptr = key_disk_cache_head;
		while (disk_ptr != NULL && count < MAX_KEY_CHILDREN)
		{
			if (disk_ptr->flags & CACHE_FLAG_VALID &&
				!memcmp(&disk_ptr->uuid, &tmp_uuid, sizeof(TSS_UUID)))
			{
				/* increment count, then search for the parent */
				count++;
				/* save a pointer to this cache entry */
				tmp_ptrs[count - 1] = disk_ptr;
				/* if the parent of this key is NULL, we're at the root of the tree */
				if (!memcmp(&disk_ptr->parent_uuid, &NULL_UUID, sizeof(TSS_UUID)))
					break;
				/* overwrite tmp_uuid with the parent, which we will now search for */
				memcpy(&tmp_uuid, &disk_ptr->parent_uuid, sizeof(TSS_UUID));
				disk_ptr = key_disk_cache_head;
				continue;
			}
			disk_ptr = disk_ptr->next;
		}
		/* when we reach this point, we have an array of TSS_UUID's that leads from the
		 * requested key up to the SRK*/

		/* malloc a structure for each of them */
		if (count != 0) {
			ret = calloc(count, sizeof(TSS_KM_KEYINFO2));
			if (ret == NULL) {
				LogError("malloc of %zd bytes failed.",
						(count * sizeof(TSS_KM_KEYINFO2)));
				count = 0;
				result = TCSERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
		} else {
			goto done;
		}

		for (i = 0; i < count; i++) {
			/* look for a mem cache entry to check if its loaded */
			for (mem_ptr = key_mem_cache_head; mem_ptr; mem_ptr = mem_ptr->next) {
				if (!memcmp(&mem_ptr->uuid, &tmp_ptrs[i]->uuid, sizeof(TSS_UUID))) {
					if ((result = fill_key_info2(tmp_ptrs[i], mem_ptr, &ret[i]))) {
						free(ret);
						ret = NULL;
						count = 0;
						goto done;
					}
					break;
				}
			}
			/* if there is no mem cache entry for this key, go ahead and call
			 * fill_key_info(), it will pull everything from disk */
			if (mem_ptr == NULL) {
				if ((result = fill_key_info2(tmp_ptrs[i], NULL, &ret[i]))) {
					free(ret);
					ret = NULL;
					count = 0;
					goto done;
				}
			}
		}
	}
done:

	MUTEX_UNLOCK(disk_cache_lock);
	MUTEX_UNLOCK(mem_cache_lock);

	*ppKeyHierarchy = ret;
	*pcKeyHierarchySize = count;

	return result;
}

TSS_RESULT
TCS_GetRegisteredKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			      TSS_UUID *KeyUUID,		/* in */
			      TSS_KM_KEYINFO ** ppKeyInfo)	/* out */
{
	TSS_RESULT result;
	UINT64 offset;
	BYTE tcpaKeyBlob[1024];
	TSS_KEY tcpaKey;
	UINT16 keySize = sizeof (tcpaKeyBlob);
	TSS_UUID parentUUID;

	/* This should be set in case we return before the malloc */
	*ppKeyInfo = NULL;

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = ps_get_key_by_uuid(KeyUUID, tcpaKeyBlob, &keySize))) {
		return TCSERR(TSS_E_PS_KEY_NOTFOUND);
	}

	if ((result = getParentUUIDByUUID(KeyUUID, &parentUUID)))
		return TCSERR(TSS_E_FAIL);

	*ppKeyInfo = malloc(sizeof(TSS_KM_KEYINFO));
	if (*ppKeyInfo == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(TSS_KM_KEYINFO));
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	UnloadBlob_TSS_KEY(&offset, tcpaKeyBlob, &tcpaKey);

	(*ppKeyInfo)->bAuthDataUsage = tcpaKey.authDataUsage;

	(*ppKeyInfo)->fIsLoaded = FALSE;

	if (tcpaKey.hdr.key12.tag == TPM_TAG_KEY12) {
		(*ppKeyInfo)->versionInfo.bMajor = TSS_SPEC_MAJOR;
		(*ppKeyInfo)->versionInfo.bMinor = TSS_SPEC_MINOR;
		(*ppKeyInfo)->versionInfo.bRevMajor = 0;
		(*ppKeyInfo)->versionInfo.bRevMinor = 0;
	} else {
		(*ppKeyInfo)->versionInfo.bMajor = tcpaKey.hdr.key11.ver.major;
		(*ppKeyInfo)->versionInfo.bMinor = tcpaKey.hdr.key11.ver.minor;
		(*ppKeyInfo)->versionInfo.bRevMajor = tcpaKey.hdr.key11.ver.revMajor;
		(*ppKeyInfo)->versionInfo.bRevMinor = tcpaKey.hdr.key11.ver.revMinor;
	}

	memcpy(&((*ppKeyInfo)->keyUUID), KeyUUID, sizeof(TSS_UUID));

	(*ppKeyInfo)->ulVendorDataLength = 0;
	(*ppKeyInfo)->rgbVendorData = 0;

	memcpy(&((*ppKeyInfo)->parentKeyUUID), &parentUUID, sizeof(TSS_UUID));
	return TSS_SUCCESS;
}

TSS_RESULT
TCS_GetRegisteredKeyBlob_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				  TSS_UUID *KeyUUID,		/* in */
				  UINT32 * pcKeySize,		/* out */
				  BYTE ** prgbKey)		/* out */
{
	UINT16 keySize;
	BYTE buffer[4096];
	TSS_RESULT result;

	if ((result = ctx_verify_context(hContext)))
		return result;

	keySize = sizeof(buffer);
	if ((result = ps_get_key_by_uuid(KeyUUID, buffer, &keySize)))
		return TCSERR(TSS_E_PS_KEY_NOTFOUND);

	*prgbKey = calloc(1, keySize);
	if (*prgbKey == NULL) {
		LogError("malloc of %d bytes failed.", keySize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	} else {
		memcpy(*prgbKey, buffer, keySize);
	}
	*pcKeySize = keySize;

	return TSS_SUCCESS;
}

TSS_RESULT
TCSP_LoadKeyByUUID_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			    TSS_UUID *KeyUUID,			/* in */
			    TCS_LOADKEY_INFO * pLoadKeyInfo,	/* in, out */
			    TCS_KEY_HANDLE * phKeyTCSI)		/* out */
{
	UINT32 keyslot = 0, keySize;
	TSS_RESULT result;
	TSS_UUID parentUuid;
	BYTE keyBlob[0x1000];
	UINT16 blobSize = sizeof(keyBlob);
	UINT64 offset;
	TCS_KEY_HANDLE parentTCSKeyHandle;

	LogDebugFn("Enter: uuid: 0x%lx auth? 0x%x ***********", (unsigned long)KeyUUID,
		  pLoadKeyInfo == NULL ? 0xdeadbeef : pLoadKeyInfo->authData.AuthHandle);

	if ((result = ctx_verify_context(hContext)))
		return result;

	memset(&parentUuid, 0, sizeof(TSS_UUID));

	if (pLoadKeyInfo &&
	    memcmp(&pLoadKeyInfo->parentKeyUUID, &parentUuid, sizeof(TSS_UUID))) {
		if (ps_get_key_by_uuid(&pLoadKeyInfo->keyUUID, keyBlob, &blobSize))
			return TCSERR(TSS_E_PS_KEY_NOTFOUND);

		if (mc_get_handles_by_uuid(&pLoadKeyInfo->parentKeyUUID, &parentTCSKeyHandle,
					   &keyslot))
			return TCSERR(TCS_E_KM_LOADFAILED);

		return TCSP_LoadKeyByBlob_Internal(hContext, parentTCSKeyHandle,
						   blobSize, keyBlob,
						   &pLoadKeyInfo->authData,
						   phKeyTCSI, &keyslot);
	}

	/* if KeyUUID is already loaded, increment the ref count and return */
	if (mc_get_handles_by_uuid(KeyUUID, phKeyTCSI, &keyslot) == TSS_SUCCESS) {
		if (keyslot) {
			if (ctx_mark_key_loaded(hContext, *phKeyTCSI)) {
				LogError("Error marking key as loaded");
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
			return TSS_SUCCESS;
		}
	}
	/*********************************************************************
	 *	The first thing to do in this func is setup all the info and make sure
	 *		that we get it all from either the keyfile or the keyCache
	 *		also, it's important to return if the key is already loaded
	 ***********************************************************************/
	LogDebugFn("calling ps_get_key_by_uuid");
	if (ps_get_key_by_uuid(KeyUUID, keyBlob, &blobSize))
		return TCSERR(TSS_E_PS_KEY_NOTFOUND);
	/* convert UINT16 to UIN32 */
	keySize = blobSize;

	LogDebugFn("calling getParentUUIDByUUID");
	/*---	Get my parent's UUID.  Since My key is registered, my parent should be as well. */
	if ((result = getParentUUIDByUUID(KeyUUID, &parentUuid)))
		return TCSERR(TCS_E_KM_LOADFAILED);

	if ((result = TCSP_LoadKeyByUUID_Internal(hContext, &parentUuid,
						  pLoadKeyInfo, &parentTCSKeyHandle)))
		return result;

	LogDebugFn("calling TCSP_LoadKeyByBlob_Internal");
	/*******************************************************
	 * If no errors have happend up till now, then the parent is loaded and ready for use.
	 * The parent's TCS Handle should be in parentTCSKeyHandle.
	 ******************************************************/
	if ((result = TCSP_LoadKeyByBlob_Internal(hContext, parentTCSKeyHandle,
						  keySize, keyBlob,
						  NULL,
						  phKeyTCSI, &keyslot))) {
		LogDebugFn("TCSP_LoadKeyByBlob_Internal returned 0x%x", result);
		if (result == TCPA_E_AUTHFAIL && pLoadKeyInfo) {
			BYTE blob[1000];

			/* set up a load key info struct */
			memcpy(&pLoadKeyInfo->parentKeyUUID, &parentUuid, sizeof(TSS_UUID));
			memcpy(&pLoadKeyInfo->keyUUID, KeyUUID, sizeof(TSS_UUID));

			/* calculate the paramDigest */
			offset = 0;
			LoadBlob_UINT32(&offset, TPM_ORD_LoadKey, blob);
			LoadBlob(&offset, keySize, blob, keyBlob);
			if (Hash(TSS_HASH_SHA1, offset, blob,
				 (BYTE *)&pLoadKeyInfo->paramDigest.digest))
				result = TCSERR(TSS_E_INTERNAL_ERROR);

			result = TCSERR(TCS_E_KM_LOADFAILED);
		}
	}

	return result;
}

TSS_RESULT
TCSP_GetRegisteredKeyByPublicInfo_Internal(TCS_CONTEXT_HANDLE tcsContext,	/* in */
					   TCPA_ALGORITHM_ID algID,		/* in */
					   UINT32 ulPublicInfoLength,		/* in */
					   BYTE * rgbPublicInfo,		/* in */
					   UINT32 * keySize,			/* out */
					   BYTE ** keyBlob)			/* out */
{
	TCPA_STORE_PUBKEY pubKey;
	TSS_RESULT result = TCSERR(TSS_E_FAIL);

	if ((result = ctx_verify_context(tcsContext)))
		return result;

	if (algID == TCPA_ALG_RSA) {
		/*---	Convert Public info to a structure */
		pubKey.keyLength = ulPublicInfoLength;
		pubKey.key = malloc(pubKey.keyLength);
		if (pubKey.key == NULL) {
			LogError("malloc of %d bytes failed.", pubKey.keyLength);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		memcpy(pubKey.key, rgbPublicInfo, pubKey.keyLength);

		if ((result = ps_get_key_by_pub(&pubKey, keySize, keyBlob))) {
			LogDebug("Public key data not found in PS");
			free(pubKey.key);
			return TCSERR(TSS_E_PS_KEY_NOTFOUND);
		}
	}
	free(pubKey.key);

	return result;
}
