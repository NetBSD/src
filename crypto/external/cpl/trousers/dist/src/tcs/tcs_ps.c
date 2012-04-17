
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
#include "trousers_types.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcsps.h"
#include "tcslog.h"
#include "tddl.h"
#include "req_mgr.h"
#include "tcsd_wrap.h"
#include "tcsd.h"


TSS_RESULT
get_vendor_data(struct key_disk_cache *d, UINT32 *size, BYTE **data)
{
	if (d->vendor_data_size == 0) {
		*size = 0;
		*data = NULL;

		return TSS_SUCCESS;
	}

	return ps_get_vendor_data(d, size, data);
}

TSS_RESULT
fill_key_info(struct key_disk_cache *d, struct key_mem_cache *m, TSS_KM_KEYINFO *key_info)
{
	BYTE tmp_blob[2048];
	UINT16 tmp_blob_size = 2048;
	TSS_KEY tmp_key;
	UINT64 offset;
	TSS_RESULT result;

	if (m == NULL) {
		key_info->fIsLoaded = FALSE;

		/* read key from disk */
		if ((result = ps_get_key_by_cache_entry(d, (BYTE *)&tmp_blob, &tmp_blob_size)))
			return result;

		offset = 0;
		/* XXX add a real context handle here */
		if ((result = UnloadBlob_TSS_KEY(&offset, tmp_blob, &tmp_key)))
			return result;

		if (tmp_key.hdr.key12.tag == TPM_TAG_KEY12) {
			key_info->versionInfo.bMajor = TSS_SPEC_MAJOR;
			key_info->versionInfo.bMinor = TSS_SPEC_MINOR;
			key_info->versionInfo.bRevMajor = 0;
			key_info->versionInfo.bRevMajor = 0;
		} else
			memcpy(&key_info->versionInfo, &tmp_key.hdr.key11.ver, sizeof(TSS_VERSION));
		memcpy(&key_info->bAuthDataUsage, &tmp_key.authDataUsage,
		       sizeof(TCPA_AUTH_DATA_USAGE));
		destroy_key_refs(&tmp_key);
	} else {
		if (m->tpm_handle == NULL_TPM_HANDLE)
			key_info->fIsLoaded = FALSE;
		else
			key_info->fIsLoaded = TRUE;

		if (m->blob->hdr.key12.tag == TPM_TAG_KEY12) {
			key_info->versionInfo.bMajor = TSS_SPEC_MAJOR;
			key_info->versionInfo.bMinor = TSS_SPEC_MINOR;
			key_info->versionInfo.bRevMajor = 0;
			key_info->versionInfo.bRevMajor = 0;
		} else
			memcpy(&key_info->versionInfo, &m->blob->hdr.key11.ver, sizeof(TSS_VERSION));
		memcpy(&key_info->bAuthDataUsage, &m->blob->authDataUsage,
		       sizeof(TCPA_AUTH_DATA_USAGE));
	}

	memcpy(&key_info->keyUUID, &d->uuid, sizeof(TSS_UUID));
	memcpy(&key_info->parentKeyUUID, &d->parent_uuid, sizeof(TSS_UUID));

	return get_vendor_data(d, &key_info->ulVendorDataLength, &key_info->rgbVendorData);
}

TSS_RESULT
fill_key_info2(struct key_disk_cache *d, struct key_mem_cache *m, TSS_KM_KEYINFO2 *key_info)
{
	BYTE tmp_blob[2048];
	UINT16 tmp_blob_size = 2048;
	TSS_KEY tmp_key;
	UINT64 offset;
	TSS_RESULT result;

	if (m == NULL) {
		key_info->fIsLoaded = FALSE;

		/* read key from disk */
		if ((result = ps_get_key_by_cache_entry(d, (BYTE *)&tmp_blob, &tmp_blob_size)))
			return result;

		offset = 0;
		/* XXX add a real context handle here */
		if ((result = UnloadBlob_TSS_KEY(&offset, tmp_blob, &tmp_key)))
			return result;

		if (tmp_key.hdr.key12.tag == TPM_TAG_KEY12) {
			key_info->versionInfo.bMajor = TSS_SPEC_MAJOR;
			key_info->versionInfo.bMinor = TSS_SPEC_MINOR;
			key_info->versionInfo.bRevMajor = 0;
			key_info->versionInfo.bRevMajor = 0;
		} else
			memcpy(&key_info->versionInfo, &tmp_key.hdr.key11.ver, sizeof(TSS_VERSION));
		memcpy(&key_info->bAuthDataUsage, &tmp_key.authDataUsage,
		       sizeof(TCPA_AUTH_DATA_USAGE));
		destroy_key_refs(&tmp_key);
	} else {
		if (m->tpm_handle == NULL_TPM_HANDLE)
			key_info->fIsLoaded = FALSE;
		else
			key_info->fIsLoaded = TRUE;

		if (m->blob->hdr.key12.tag == TPM_TAG_KEY12) {
			key_info->versionInfo.bMajor = TSS_SPEC_MAJOR;
			key_info->versionInfo.bMinor = TSS_SPEC_MINOR;
			key_info->versionInfo.bRevMajor = 0;
			key_info->versionInfo.bRevMajor = 0;
		} else
			memcpy(&key_info->versionInfo, &m->blob->hdr.key11.ver, sizeof(TSS_VERSION));
		memcpy(&key_info->bAuthDataUsage, &m->blob->authDataUsage,
		       sizeof(TCPA_AUTH_DATA_USAGE));
	}

	memcpy(&key_info->keyUUID, &d->uuid, sizeof(TSS_UUID));
	memcpy(&key_info->parentKeyUUID, &d->parent_uuid, sizeof(TSS_UUID));

	/* Fill the two new TSS_KM_KEYINFO2 fields here */
	key_info->persistentStorageTypeParent = d->flags & CACHE_FLAG_PARENT_PS_SYSTEM ?
						TSS_PS_TYPE_SYSTEM : TSS_PS_TYPE_USER;
	key_info->persistentStorageType = TSS_PS_TYPE_SYSTEM;

	return get_vendor_data(d, &key_info->ulVendorDataLength, &key_info->rgbVendorData);
}

TSS_RESULT
key_mgr_load_by_uuid(TCS_CONTEXT_HANDLE hContext,
		     TSS_UUID *uuid,
		     TCS_LOADKEY_INFO *pInfo,
		     TCS_KEY_HANDLE *phKeyTCSI)
{
	TSS_RESULT result;

	MUTEX_LOCK(mem_cache_lock);

	result = TCSP_LoadKeyByUUID_Internal(hContext, uuid, pInfo, phKeyTCSI);

	LogDebug("Key %s loaded by UUID w/ TCS handle: 0x%x",
		result ? "NOT" : "successfully", result ? 0 : *phKeyTCSI);

	MUTEX_UNLOCK(mem_cache_lock);

	return result;
}

