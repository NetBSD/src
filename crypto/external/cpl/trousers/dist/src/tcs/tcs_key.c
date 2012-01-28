
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#include <string.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "req_mgr.h"
#include "tcs_tsp.h"
#include "tcslog.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"

struct key_mem_cache *key_mem_cache_head = NULL;

TSS_UUID NULL_UUID = { 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0, 0 } };


TSS_RESULT
add_cache_entry(TCS_CONTEXT_HANDLE hContext,
		BYTE*              blob,
		TCS_KEY_HANDLE     hParent,
		TPM_KEY_HANDLE     hSlot,
		TCS_KEY_HANDLE*    new)
{
	UINT64 offset;
	TSS_RESULT result;
	TCS_KEY_HANDLE tcsHandle;
	TSS_KEY key, *pKey;

	if (!blob) {
		pKey = NULL;
	} else {
		offset = 0;
		if ((result = UnloadBlob_TSS_KEY(&offset, blob, &key)))
			return result;

		if ((tcsHandle = mc_get_handle_by_pub(&key.pubKey, hParent)) == NULL_TCS_HANDLE) {
			pKey = &key;
		} else {
			mc_set_slot_by_handle(tcsHandle, hSlot);
			*new = tcsHandle;
			goto done;
		}
	}

	LogDebugFn("No existing key handle for this key, creating new one...");
	/* Get a new TCS Key Handle */
	tcsHandle = getNextTcsKeyHandle();
	LogDebugFn("calling mc_add_entry, TCS handle: 0x%x, TPM handle 0x%x", tcsHandle, hSlot);

	if ((result = mc_add_entry(tcsHandle, hSlot, pKey)))
		goto done;

	LogDebugFn("ctx_mark_key_loaded");
	if (ctx_mark_key_loaded(hContext, tcsHandle)) {
		LogError("Error marking key as loaded");
		result = TCSERR(TSS_E_INTERNAL_ERROR);
		goto done;
	}

	if ((result = mc_set_parent_by_handle(tcsHandle, hParent))) {
		LogError("mc_set_parent_by_handle failed.");
		goto done;
	}

	*new = tcsHandle;
done:
	if (blob)
		destroy_key_refs(&key);
	return result;
}

/* Check that the context has this key loaded and return the associated slot. Do not search PS if
 * the key is not found */
TSS_RESULT
get_slot_lite(TCS_CONTEXT_HANDLE hContext, TCS_KEY_HANDLE hKey, TPM_KEY_HANDLE *out)
{
	if (ctx_has_key_loaded(hContext, hKey)) {
		if ((*out = mc_get_slot_by_handle(hKey)) == NULL_TPM_HANDLE)
			return TCSERR(TCS_E_INVALID_KEY);

		return TSS_SUCCESS;
	}

	return TCSERR(TCS_E_INVALID_KEY);
}

/* XXX Can get_slot be merged with ensureKeyIsLoaded? */

/* Given a handle, get_slot searches the mem cache for a mapping to a TPM handle. If there is no
 * mapping, it looks up the pub key of the handle and attempts to load it by finding its pub key
 * in the persistent store. If that's not found, return error. */
TSS_RESULT
get_slot(TCS_CONTEXT_HANDLE hContext, TCS_KEY_HANDLE hKey, TPM_KEY_HANDLE *out)
{
	TSS_RESULT result = TSS_SUCCESS;
	TPM_STORE_PUBKEY *pub = NULL;
	TPM_KEY_HANDLE slot;

        LogDebugFn("calling mc_get_slot_by_handle");
        if ((slot = mc_get_slot_by_handle(hKey)) == NULL_TPM_HANDLE) {
                LogDebugFn("calling mc_get_pub_by_slot");
                if ((pub = mc_get_pub_by_slot(hKey)) == NULL)
                        return TCSERR(TCS_E_KM_LOADFAILED);

                LogDebugFn("calling LoadKeyShim");
                /* Otherwise, try to load it using the shim */
                result = LoadKeyShim(hContext, pub, NULL, &slot);
        }

	if (!result)
		*out = slot;

	return result;
}

/* load_key_init is the common entry point for all load key requests to the TCSD. These can come in
 * as straight load or load2 requests, or through a transport session.
 *
 * We'll always attempt to load the key if
 * A) It requires auth (load should fail if auth is bad, even when its already been loaded by
 *    another thread)
 * B) Its in a transport session (the key blob is encrypted)
 *
 * Otherwise if the key is already loaded by another thread and it doesn't require auth, then we
 * will just set *load_key to FALSE, telling the caller that there's no need to send anything to
 * the TPM.
 */
TSS_RESULT
load_key_init(TPM_COMMAND_CODE   ord,
	      TCS_CONTEXT_HANDLE hContext,
	      TCS_KEY_HANDLE     parent_handle,
	      UINT32             blob_size,
	      BYTE*              blob,
	      TSS_BOOL           encrypted,
	      TPM_AUTH*          auth,
	      TSS_BOOL*          load_key,
	      UINT64*            out_len,
	      BYTE*              out,
	      TCS_KEY_HANDLE*    handle,
	      TPM_KEY_HANDLE*    slot)
{
	TSS_RESULT result = TSS_SUCCESS;
	TSS_KEY key;
	UINT64 offset;
	TPM_KEY_HANDLE tpm_slot;
	TCS_KEY_HANDLE tcs_handle;
	TSS_BOOL canLoad;


	if (!encrypted) {
		offset = 0;
		memset(&key, 0, sizeof(TSS_KEY));
		if ((result = UnloadBlob_TSS_KEY(&offset, blob, &key)))
			return result;
	}

	if (!auth && !encrypted) {
		LogDebugFn("Checking if LoadKeyByBlob can be avoided by using existing key");

		if ((tcs_handle = mc_get_handle_by_pub(&key.pubKey, parent_handle))) {
			LogDebugFn("tcs key handle exists");

			tpm_slot = mc_get_slot_by_handle(tcs_handle);
			if (tpm_slot && (isKeyLoaded(tpm_slot) == TRUE)) {
				LogDebugFn("Don't need to reload this key.");
				*handle = tcs_handle;
				*slot = tpm_slot;
				*load_key = FALSE;
				goto done;
			}
		}
	}
	*load_key = TRUE;

	LogDebugFn("calling canILoadThisKey");
	if (!encrypted) {
		if ((result = canILoadThisKey(&(key.algorithmParms), &canLoad)))
			goto error;

		if (canLoad == FALSE) {
			LogDebugFn("calling evictFirstKey");
			/* Evict a key that isn't the parent */
			if ((result = evictFirstKey(parent_handle)))
				goto error;
		}
	}

error:
	if (!encrypted)
		destroy_key_refs(&key);
done:
	return result;
}

TSS_RESULT
load_key_final(TCS_CONTEXT_HANDLE hContext,
	       TCS_KEY_HANDLE     parent_handle,
	       TCS_KEY_HANDLE*    tcs_handle,
	       BYTE*              blob,
	       TPM_KEY_HANDLE     slot)
{
	if (*tcs_handle == NULL_TCS_HANDLE)
		return add_cache_entry(hContext, blob, parent_handle, slot, tcs_handle);
	else
		return mc_set_slot_by_handle(*tcs_handle, slot);
}

TSS_RESULT
canILoadThisKey(TCPA_KEY_PARMS *parms, TSS_BOOL *b)
{
	UINT16 subCapLength;
	UINT64 offset;
	BYTE subCap[100];
	TCPA_RESULT result;
	UINT32 respDataLength;
	BYTE *respData;

	offset = 0;
	LoadBlob_KEY_PARMS(&offset, subCap, parms);
	subCapLength = offset;

	if ((result = TCSP_GetCapability_Internal(InternalContext, TCPA_CAP_CHECK_LOADED,
						  subCapLength, subCap, &respDataLength,
						  &respData))) {
		*b = FALSE;
		LogDebugFn("NO");
		return result;
	}

	*b = respData[0];
	free(respData);
	LogDebugFn("%s", *b ? "YES" : "NO");

	return TSS_SUCCESS;
}

TCPA_RESULT
internal_EvictByKeySlot(TCPA_KEY_HANDLE slot)
{
	TCPA_RESULT result;
	UINT32 paramSize;
	UINT64 offset;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Evict Key");

#ifdef TSS_BUILD_TSS12
	if (TPM_VERSION_IS(1,2)) {
		LogDebugFn("Evicting key using FlushSpecific for TPM 1.2");

		return TCSP_FlushSpecific_Common(slot, TPM_RT_KEY);
	}
#endif

	offset = 10;
	LoadBlob_UINT32(&offset, slot, txBlob);
	LoadBlob_Header(TPM_TAG_RQU_COMMAND, offset, TPM_ORD_EvictKey, txBlob);

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);

	LogResult("Evict Key", result);
	return result;
}

TSS_RESULT
clearUnknownKeys(TCS_CONTEXT_HANDLE hContext, UINT32 *cleared)
{
	TSS_RESULT result = TSS_SUCCESS;
	TCPA_KEY_HANDLE_LIST keyList = { 0, NULL };
	int i;
	BYTE *respData = NULL;
	UINT32 respDataSize = 0, count = 0;
	TCPA_CAPABILITY_AREA capArea = -1;
	UINT64 offset = 0;
	TSS_BOOL found = FALSE;
	struct key_mem_cache *tmp;

	capArea = TCPA_CAP_KEY_HANDLE;

	if ((result = TCSP_GetCapability_Internal(hContext, capArea, 0, NULL, &respDataSize,
						  &respData)))
		return result;

	if ((result = UnloadBlob_KEY_HANDLE_LIST(&offset, respData, &keyList)))
		goto done;

#ifdef TSS_DEBUG
	LogDebug("Loaded TPM key handles:");
	for (i = 0; i < keyList.loaded; i++) {
		LogDebugFn("%d: %x", i, keyList.handle[i]);
	}

	LogDebug("Loaded TCSD key handles:");
	i=0;
	for (tmp = key_mem_cache_head; tmp; tmp = tmp->next) {
		LogDebugFn("%d: 0x%x -> 0x%x", i++, tmp->tpm_handle,
			    tmp->tcs_handle);
	}
#endif

	for (i = 0; i < keyList.loaded; i++) {
		/* as long as we're only called from evictFirstKey(), we don't
		 * need to lock here */
		for (tmp = key_mem_cache_head; tmp; tmp = tmp->next) {
			if (tmp->tpm_handle == keyList.handle[i]) {
				found = TRUE;
				break;
			}
		}
		if (found)
			found = FALSE;
		else {
			if ((result = internal_EvictByKeySlot(keyList.handle[i])))
				goto done;
			else
				count++;
		}
	}

	*cleared = count;
done:
	free(keyList.handle);
	free(respData);

	return TSS_SUCCESS;
}

#if 0
TCPA_RESULT
clearKeysFromChip(TCS_CONTEXT_HANDLE hContext)
{
	TCPA_RESULT result;
	TCPA_KEY_HANDLE_LIST keyList;
	UINT32 i;
	BYTE *respData = 0;
	UINT32 respDataSize = 0;
	TCPA_CAPABILITY_AREA capArea = -1;
	UINT64 offset = 0;

	capArea = TCPA_CAP_KEY_HANDLE;

	if ((result = TCSP_GetCapability_Internal(hContext, capArea, 0, NULL,
					&respDataSize, &respData)))
		return result;

	if ((result = UnloadBlob_KEY_HANDLE_LIST(&offset, respData, &keyList)))
		return result;
	for (i = 0; i < keyList.loaded; i++) {
		if (keyList.handle[i] == SRK_TPM_HANDLE ||	/*can't evict SRK */
		    keyList.handle[i] == EK_TPM_HANDLE)	/*can't evict EK */
			continue;
		if ((result = internal_EvictByKeySlot(keyList.handle[i])))
			return result;
	}
	return TSS_SUCCESS;
}
#endif

void
LoadBlob_KEY_PARMS(UINT64 *offset, BYTE *blob, TCPA_KEY_PARMS *keyInfo)
{
	LoadBlob_UINT32(offset, keyInfo->algorithmID, blob);
	LoadBlob_UINT16(offset, keyInfo->encScheme, blob);
	LoadBlob_UINT16(offset, keyInfo->sigScheme, blob);
	LoadBlob_UINT32(offset, keyInfo->parmSize, blob);
	LoadBlob(offset, keyInfo->parmSize, blob, keyInfo->parms);
}

TSS_RESULT
UnloadBlob_STORE_PUBKEY(UINT64 *offset, BYTE *blob, TCPA_STORE_PUBKEY *store)
{
	if (!store) {
		UINT32 keyLength;

		UnloadBlob_UINT32(offset, &keyLength, blob);

		if (keyLength > 0)
			UnloadBlob(offset, keyLength, blob, NULL);

		return TSS_SUCCESS;
	}

	UnloadBlob_UINT32(offset, &store->keyLength, blob);

	if (store->keyLength == 0) {
		store->key = NULL;
		LogWarn("Unloading a public key of size 0!");
	} else {
		store->key = (BYTE *)malloc(store->keyLength);
		if (store->key == NULL) {
			LogError("malloc of %u bytes failed.", store->keyLength);
			store->keyLength = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		UnloadBlob(offset, store->keyLength, blob, store->key);
	}

	return TSS_SUCCESS;
}

void
LoadBlob_STORE_PUBKEY(UINT64 *offset, BYTE * blob, TCPA_STORE_PUBKEY * store)
{
	LoadBlob_UINT32(offset, store->keyLength, blob);
	LoadBlob(offset, store->keyLength, blob, store->key);
}

TSS_RESULT
UnloadBlob_TSS_KEY(UINT64 *offset, BYTE *blob, TSS_KEY *key)
{
	TSS_RESULT rc;

	if (!key) {
		UINT32 size;

		/* TPM_KEY's ver and TPM_KEY12's tag/file are
		   the same size, so... */
		UnloadBlob_VERSION(offset, blob, NULL);
		UnloadBlob_UINT16(offset, NULL, blob);
		UnloadBlob_KEY_FLAGS(offset, blob, NULL);
		UnloadBlob_BOOL(offset, NULL, blob);
		if ((rc = UnloadBlob_KEY_PARMS(offset, blob, NULL)))
			return rc;
		UnloadBlob_UINT32(offset, &size, blob);

		if (size > 0)
			UnloadBlob(offset, size, blob, NULL);

		if ((rc = UnloadBlob_STORE_PUBKEY(offset, blob, NULL)))
			return rc;

		UnloadBlob_UINT32(offset, &size, blob);

		if (size > 0)
			UnloadBlob(offset, size, blob, NULL);

		return TSS_SUCCESS;
	}

	if (key->hdr.key12.tag == TPM_TAG_KEY12) {
		UnloadBlob_UINT16(offset, &key->hdr.key12.tag, blob);
		UnloadBlob_UINT16(offset, &key->hdr.key12.fill, blob);
	} else
		UnloadBlob_TCPA_VERSION(offset, blob, &key->hdr.key11.ver);
	UnloadBlob_UINT16(offset, &key->keyUsage, blob);
	UnloadBlob_KEY_FLAGS(offset, blob, &key->keyFlags);
	UnloadBlob_BOOL(offset, (TSS_BOOL *)&key->authDataUsage, blob);
	if ((rc = UnloadBlob_KEY_PARMS(offset, blob, &key->algorithmParms)))
		return rc;
	UnloadBlob_UINT32(offset, &key->PCRInfoSize, blob);

	if (key->PCRInfoSize == 0)
		key->PCRInfo = NULL;
	else {
		key->PCRInfo = malloc(key->PCRInfoSize);
		if (key->PCRInfo == NULL) {
			LogError("malloc of %u bytes failed.", key->PCRInfoSize);
			key->PCRInfoSize = 0;
			free(key->algorithmParms.parms);
			key->algorithmParms.parms = NULL;
			key->algorithmParms.parmSize = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(offset, key->PCRInfoSize, blob, key->PCRInfo);
	}

	if ((rc = UnloadBlob_STORE_PUBKEY(offset, blob, &key->pubKey))) {
		free(key->PCRInfo);
		key->PCRInfo = NULL;
		key->PCRInfoSize = 0;
		free(key->algorithmParms.parms);
		key->algorithmParms.parms = NULL;
		key->algorithmParms.parmSize = 0;
		return rc;
	}
	UnloadBlob_UINT32(offset, &key->encSize, blob);

	if (key->encSize == 0)
		key->encData = NULL;
	else {
		key->encData = (BYTE *)malloc(key->encSize);
		if (key->encData == NULL) {
			LogError("malloc of %d bytes failed.", key->encSize);
			key->encSize = 0;
			free(key->algorithmParms.parms);
			key->algorithmParms.parms = NULL;
			key->algorithmParms.parmSize = 0;
			free(key->PCRInfo);
			key->PCRInfo = NULL;
			key->PCRInfoSize = 0;
			free(key->pubKey.key);
			key->pubKey.key = NULL;
			key->pubKey.keyLength = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(offset, key->encSize, blob, key->encData);
	}

	return TSS_SUCCESS;
}

void
LoadBlob_TSS_KEY(UINT64 *offset, BYTE * blob, TSS_KEY * key)
{
	if (key->hdr.key12.tag == TPM_TAG_KEY12) {
		LoadBlob_UINT16(offset, key->hdr.key12.tag, blob);
		LoadBlob_UINT16(offset, key->hdr.key12.fill, blob);
	} else
		LoadBlob_TCPA_VERSION(offset, blob, &key->hdr.key11.ver);
	LoadBlob_UINT16(offset, key->keyUsage, blob);
	LoadBlob_KEY_FLAGS(offset, blob, &key->keyFlags);
	LoadBlob_BOOL(offset, key->authDataUsage, blob);
	LoadBlob_KEY_PARMS(offset, blob, &key->algorithmParms);
	LoadBlob_UINT32(offset, key->PCRInfoSize, blob);
	LoadBlob(offset, key->PCRInfoSize, blob, key->PCRInfo);
	LoadBlob_STORE_PUBKEY(offset, blob, &key->pubKey);
	LoadBlob_UINT32(offset, key->encSize, blob);
	LoadBlob(offset, key->encSize, blob, key->encData);
}

void
LoadBlob_PUBKEY(UINT64 *offset, BYTE * blob, TCPA_PUBKEY * key)
{
	LoadBlob_KEY_PARMS(offset, blob, &(key->algorithmParms));
	LoadBlob_STORE_PUBKEY(offset, blob, &(key->pubKey));
}

TSS_RESULT
UnloadBlob_PUBKEY(UINT64 *offset, BYTE *blob, TCPA_PUBKEY *key)
{
	TSS_RESULT rc;

	if (!key) {
		if ((rc = UnloadBlob_KEY_PARMS(offset, blob, NULL)))
			return rc;
		return UnloadBlob_STORE_PUBKEY(offset, blob, NULL);
	}

	if ((rc = UnloadBlob_KEY_PARMS(offset, blob, &key->algorithmParms)))
		return rc;
	if ((rc = UnloadBlob_STORE_PUBKEY(offset, blob, &key->pubKey))) {
		free(key->algorithmParms.parms);
		key->algorithmParms.parms = NULL;
		key->algorithmParms.parmSize = 0;
	}

	return rc;
}

void
LoadBlob_KEY_FLAGS(UINT64 *offset, BYTE * blob, TCPA_KEY_FLAGS * flags)
{
	LoadBlob_UINT32(offset, *flags, blob);
}

void
destroy_key_refs(TSS_KEY *key)
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
