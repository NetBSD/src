
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */

#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "tcs_tsp.h"
#include "tspps.h"
#include "tsplog.h"
#include "obj.h"

/*
 * tsp_ps.c
 *
 * Functions used to query the user persistent storage file.
 *
 * Since other apps may be altering the file, all operations must be atomic WRT the file and no
 * cache will be kept, since another app could delete keys from the file out from under us.
 *
 * Atomicity is guaranteed for operations inbetween calls to get_file() and put_file().
 *
 * A PS file will have the lifetime of the TSP context. For instance, this code will store hKeyA
 * and hKeyB in the file "a":
 *
 * setenv("TSS_USER_PS_FILE=a");
 * Tspi_Context_Create(&hContext);
 * Tspi_Context_RegisterKey(hKeyA);
 * setenv("TSS_USER_PS_FILE=b");
 * Tspi_Context_RegisterKey(hKeyB);
 *
 * but this code will store hKeyA in file "a" and hKeyB in file "b":
 *
 * setenv("TSS_USER_PS_FILE=a");
 * Tspi_Context_Create(&hContext);
 * Tspi_Context_RegisterKey(hKeyA);
 * Tspi_Context_Close(hContext);
 *
 * setenv("TSS_USER_PS_FILE=b");
 * Tspi_Context_Create(&hContext);
 * Tspi_Context_RegisterKey(hKeyB);
 *
 */

TSS_RESULT
ps_get_registered_keys(TSS_UUID *uuid, TSS_UUID *tcs_uuid, UINT32 *size, TSS_KM_KEYINFO **keys)
{
	int fd;
	UINT32 result;

	if ((result = get_file(&fd)))
		return result;

	result = psfile_get_registered_keys(fd, uuid, tcs_uuid, size, keys);

	put_file(fd);

	return result;
}

TSS_RESULT
ps_get_registered_keys2(TSS_UUID *uuid, TSS_UUID *tcs_uuid, UINT32 *size, TSS_KM_KEYINFO2 **keys)
{
	int fd;
	UINT32 result;

	if ((result = get_file(&fd)))
		return result;

	/* Sets the proper TSS_KM_KEYINFO2 fields according to the UUID type */
	result = psfile_get_registered_keys2(fd, uuid, tcs_uuid, size, keys);

	put_file(fd);

	return result;
}

TSS_RESULT
ps_is_key_registered(TSS_UUID *uuid, TSS_BOOL *answer)
{
	int fd;
	TSS_RESULT result;

	if ((result = get_file(&fd)))
		return result;

	result = psfile_is_key_registered(fd, uuid, answer);

	put_file(fd);

	return result;
}

TSS_RESULT
ps_write_key(TSS_UUID *uuid, TSS_UUID *parent_uuid, UINT32 parent_ps, UINT32 blob_size, BYTE *blob)
{
	int fd;
	TSS_RESULT result;
	UINT16 short_blob_size = (UINT16)blob_size;

	if (blob_size > USHRT_MAX) {
		LogError("Blob data being written to disk is too large(%u bytes)!", blob_size);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if ((result = get_file(&fd)))
		return result;

	result = psfile_write_key(fd, uuid, parent_uuid, parent_ps, blob, short_blob_size);

	put_file(fd);
	return result;
}


TSS_RESULT
ps_remove_key(TSS_UUID *uuid)
{
	int fd;
	TSS_RESULT result;

	if ((result = get_file(&fd)))
		return result;

	result = psfile_remove_key(fd, uuid);

	put_file(fd);
	return result;
}

TSS_RESULT
ps_get_key_by_pub(TSS_HCONTEXT tspContext, UINT32 pub_size, BYTE *pub, TSS_HKEY *hKey)
{
	int fd;
	TSS_RESULT result = TSS_SUCCESS;
	BYTE key[4096];
	TSS_UUID uuid;

	if ((result = get_file(&fd)))
		return result;

	if ((result = psfile_get_key_by_pub(fd, &uuid, pub_size, pub, key))) {
		put_file(fd);
		return result;
	}

	put_file(fd);

	result = obj_rsakey_add_by_key(tspContext, &uuid, key, TSS_OBJ_FLAG_USER_PS, hKey);

	return result;
}

TSS_RESULT
ps_get_key_by_uuid(TSS_HCONTEXT tspContext, TSS_UUID *uuid, TSS_HKEY *hKey)
{
	int fd;
	TSS_RESULT result = TSS_SUCCESS;
	BYTE key[4096];

	if ((result = get_file(&fd)))
		return result;

	if ((result = psfile_get_key_by_uuid(fd, uuid, key))) {
		put_file(fd);
		return result;
	}

	put_file(fd);

	result = obj_rsakey_add_by_key(tspContext, uuid, key, TSS_OBJ_FLAG_USER_PS, hKey);

	return result;
}

TSS_RESULT
ps_get_parent_uuid_by_uuid(TSS_UUID *uuid, TSS_UUID *parent_uuid)
{
	int fd;
	TSS_RESULT result;

	if ((result = get_file(&fd)))
		return result;

	result = psfile_get_parent_uuid_by_uuid(fd, uuid, parent_uuid);

	put_file(fd);
	return result;
}

TSS_RESULT
ps_get_parent_ps_type_by_uuid(TSS_UUID *uuid, UINT32 *type)
{
	int fd;
	TSS_RESULT result;

	if ((result = get_file(&fd)))
		return result;

	result = psfile_get_parent_ps_type(fd, uuid, type);

	put_file(fd);

        return result;
}

TSS_RESULT
ps_close()
{
	TSS_RESULT result;
	int fd;

	if ((result = get_file(&fd)))
		return result;

	psfile_close(fd);

	/* No need to call put_file() here, the file is closed */

	return TSS_SUCCESS;
}

TSS_RESULT
merge_key_hierarchies(TSS_HCONTEXT tspContext, UINT32 tsp_size, TSS_KM_KEYINFO *tsp_hier,
		      UINT32 tcs_size, TSS_KM_KEYINFO *tcs_hier, UINT32 *merged_size,
		      TSS_KM_KEYINFO **merged_hier)
{
	UINT32 i, j;

	*merged_hier = malloc((tsp_size + tcs_size) * sizeof(TSS_KM_KEYINFO));
	if (*merged_hier == NULL) {
		LogError("malloc of %zu bytes failed.", (tsp_size + tcs_size) *
				sizeof(TSS_KM_KEYINFO));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	for (i = 0; i < tsp_size; i++)
		memcpy(&((*merged_hier)[i]), &tsp_hier[i], sizeof(TSS_KM_KEYINFO));

	for (j = 0; j < tcs_size; j++)
		memcpy(&((*merged_hier)[i + j]), &tcs_hier[j], sizeof(TSS_KM_KEYINFO));

	*merged_size = i + j;

	return TSS_SUCCESS;
}


TSS_RESULT
merge_key_hierarchies2(TSS_HCONTEXT tspContext, UINT32 tsp_size, TSS_KM_KEYINFO2 *tsp_hier,
		      UINT32 tcs_size, TSS_KM_KEYINFO2 *tcs_hier, UINT32 *merged_size,
		      TSS_KM_KEYINFO2 **merged_hier)
{
	UINT32 i, j;

	*merged_hier = malloc((tsp_size + tcs_size) * sizeof(TSS_KM_KEYINFO2));
	if (*merged_hier == NULL) {
		LogError("malloc of %zu bytes failed.", (tsp_size + tcs_size) *
				sizeof(TSS_KM_KEYINFO2));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	for (i = 0; i < tsp_size; i++)
		memcpy(&((*merged_hier)[i]), &tsp_hier[i], sizeof(TSS_KM_KEYINFO2));

	for (j = 0; j < tcs_size; j++)
		memcpy(&((*merged_hier)[i + j]), &tcs_hier[j], sizeof(TSS_KM_KEYINFO2));

	*merged_size = i + j;

	return TSS_SUCCESS;
}


#if 0
TSS_RESULT
load_from_system_ps(TSS_HCONTEXT tspContext, TSS_UUID *uuid, TSS_HKEY *phKey)
{
	TCS_KEY_HANDLE tcsKeyHandle;
	TCS_LOADKEY_INFO info;
	BYTE *keyBlob = NULL;

	memset(&info, 0, sizeof(TCS_LOADKEY_INFO));

	result = TCSP_LoadKeyByUUID(tspContext, uuidData, &info, &tcsKeyHandle);

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
			if (ps_get_key_by_uuid(tspContext, &info.parentKeyUUID, &keyHandle))
				return result;
		}

		if (obj_rsakey_get_policy(keyHandle, TSS_POLICY_USAGE, &hPolicy, NULL))
			return result;

		if (secret_PerformAuth_OIAP(keyHandle, TPM_ORD_LoadKey, hPolicy, &info.paramDigest,
					    &info.authData))
			return result;

		if ((result = TCSP_LoadKeyByUUID(tspContext, *uuid, &info, &tcsKeyHandle)))
			return result;
	} else if (result)
		return result;

	if ((result = TCS_GetRegisteredKeyBlob(tspContext, *uuid, &keyBlobSize, &keyBlob)))
		return result;

	if ((result = obj_rsakey_add_by_key(tspContext, uuid, keyBlob, TSS_OBJ_FLAG_SYSTEM_PS,
					    phKey))) {
		free(keyBlob);
		return result;
	}

	result = obj_rsakey_set_tcs_handle(*phKey, tcsKeyHandle);

	free(keyBlob);

	return result;
}
#endif

