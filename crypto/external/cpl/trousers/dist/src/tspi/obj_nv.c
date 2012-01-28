/*
 * The Initial Developer of the Original Code is Intel Corporation.
 * Portions created by Intel Corporation are Copyright (C) 2007 Intel Corporation.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 *
 * trousers - An open source TCG Software Stack
 *
 * Author: james.xu@intel.com Rossey.liu@intel.com
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"

TSS_RESULT
obj_nvstore_add(TSS_HCONTEXT tspContext, TSS_HOBJECT *phObject)
{
	TSS_RESULT result;
	struct tr_nvstore_obj *nvstore = calloc(1, sizeof(struct tr_nvstore_obj));

	if (nvstore == NULL) {
		LogError("malloc of %zd bytes failed.",
				sizeof(struct tr_nvstore_obj));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	if ((result = obj_list_add(&nvstore_list, tspContext, 0, nvstore, phObject))) {
		free(nvstore);
		return result;
	}

	return TSS_SUCCESS;
}

TSS_BOOL
obj_is_nvstore(TSS_HOBJECT hObject)
{
	TSS_BOOL answer = FALSE;

	if ((obj_list_get_obj(&nvstore_list, hObject))) {
		answer = TRUE;
		obj_list_put(&nvstore_list);
	}

	return answer;
}

void
nvstore_free(void *data)
{
	struct tr_nvstore_obj *nvstore = (struct tr_nvstore_obj *)data;

	free(nvstore);
}

TSS_RESULT
obj_nvstore_remove(TSS_HOBJECT hObject, TSS_HCONTEXT tspContext)
{
	TSS_RESULT result;

	if ((result = obj_list_remove(&nvstore_list, &nvstore_free, hObject, tspContext)))
		return result;

	return TSS_SUCCESS;
}

TSS_RESULT
obj_nvstore_get_tsp_context(TSS_HNVSTORE hNvstore, TSS_HCONTEXT * tspContext)
{
	struct tsp_object *obj;

	if ((obj = obj_list_get_obj(&nvstore_list, hNvstore)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	*tspContext = obj->tspContext;

	obj_list_put(&nvstore_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_nvstore_set_index(TSS_HNVSTORE hNvstore, UINT32 index)
{
	struct tsp_object *obj;
	struct tr_nvstore_obj *nvstore;
	TSS_RESULT result=TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&nvstore_list, hNvstore)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	nvstore = (struct tr_nvstore_obj *)obj->data;

	nvstore->nvIndex = index;

	obj_list_put(&nvstore_list);

	return result;
}

TSS_RESULT
obj_nvstore_get_index(TSS_HNVSTORE hNvstore, UINT32 * index)
{
	struct tsp_object *obj;
	struct tr_nvstore_obj *nvstore;
	TSS_RESULT result=TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&nvstore_list, hNvstore)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	nvstore = (struct tr_nvstore_obj *)obj->data;

	*index = nvstore->nvIndex;

	obj_list_put(&nvstore_list);

	return result;
}

TSS_RESULT
obj_nvstore_set_datasize(TSS_HNVSTORE hNvstore, UINT32 datasize)
{
	struct tsp_object *obj;
	struct tr_nvstore_obj *nvstore;
	TSS_RESULT result=TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&nvstore_list, hNvstore)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	nvstore = (struct tr_nvstore_obj *)obj->data;

	nvstore->dataSize= datasize;

	obj_list_put(&nvstore_list);

	return result;
}

TSS_RESULT
obj_nvstore_get_datasize(TSS_HNVSTORE hNvstore, UINT32 * datasize)
{
	struct tsp_object *obj;
	struct tr_nvstore_obj *nvstore;
	TSS_RESULT result=TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&nvstore_list, hNvstore)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	nvstore = (struct tr_nvstore_obj *)obj->data;

	*datasize = nvstore->dataSize;

	obj_list_put(&nvstore_list);

	return result;
}

TSS_RESULT
obj_nvstore_set_permission(TSS_HNVSTORE hNvstore, UINT32 permission)
{
	struct tsp_object *obj;
	struct tr_nvstore_obj *nvstore;
	TSS_RESULT result=TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&nvstore_list, hNvstore)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	nvstore = (struct tr_nvstore_obj *)obj->data;

	nvstore->permission.attributes= permission;

	obj_list_put(&nvstore_list);

	return result;
}

TSS_RESULT
obj_nvstore_get_permission_from_tpm(TSS_HNVSTORE hNvstore, UINT32 * permission)
{
	BYTE nv_data_public[MAX_PUBLIC_DATA_SIZE] = {0};
	UINT32 data_public_size = MAX_PUBLIC_DATA_SIZE;
	UINT32 offset;
	UINT16 pcrread_sizeOfSelect;
	UINT16 pcrwrite_sizeOfSelect;
	TPM_NV_ATTRIBUTES nv_attributes_value;
	TSS_HCONTEXT tspContext;
	TSS_RESULT result;

	if((result = obj_nvstore_get_datapublic(hNvstore, &data_public_size, nv_data_public)))
		return result;

	if ((result = obj_nvstore_get_tsp_context(hNvstore, &tspContext)))
		return result;

	offset = 0;
	offset = sizeof(TPM_STRUCTURE_TAG)+ sizeof(TPM_NV_INDEX);
	pcrread_sizeOfSelect = Decode_UINT16(nv_data_public + offset);

	offset = offset + sizeof(UINT16) + pcrread_sizeOfSelect
			+ sizeof(TPM_LOCALITY_SELECTION)
			+ sizeof(TPM_COMPOSITE_HASH);
	pcrwrite_sizeOfSelect = Decode_UINT16(nv_data_public + offset);

	offset = offset + sizeof(UINT16) + pcrwrite_sizeOfSelect
			+ sizeof(TPM_LOCALITY_SELECTION)
			+ sizeof(TPM_COMPOSITE_HASH);

	nv_attributes_value.attributes = Decode_UINT32(nv_data_public
						       + offset + sizeof(TPM_STRUCTURE_TAG));
	*permission = nv_attributes_value.attributes;

	return result;
}

TSS_RESULT
obj_nvstore_get_permission(TSS_HNVSTORE hNvstore, UINT32 * permission)
{
	struct tsp_object *obj;
	struct tr_nvstore_obj *nvstore;
	TSS_RESULT result=TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&nvstore_list, hNvstore)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	nvstore = (struct tr_nvstore_obj *)obj->data;

	*permission = nvstore->permission.attributes;

	obj_list_put(&nvstore_list);

	return result;
}

TSS_RESULT
obj_nvstore_set_policy(TSS_HNVSTORE hNvstore, TSS_HPOLICY hPolicy)
{
	struct tsp_object *obj;
	struct tr_nvstore_obj *nvstore;
	UINT32 policyType;
	TSS_RESULT result = TSS_SUCCESS;

	if ((result = obj_policy_get_type(hPolicy, &policyType)))
		return result;

	if ((obj = obj_list_get_obj(&nvstore_list, hNvstore)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	nvstore = (struct tr_nvstore_obj *)obj->data;

	switch (policyType) {
		case TSS_POLICY_USAGE:
			nvstore->policy = hPolicy;
			break;
		default:
			result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	obj_list_put(&nvstore_list);

	return result;
}

TSS_RESULT
obj_nvstore_get_policy(TSS_HNVSTORE hNvstore, UINT32 policyType, TSS_HPOLICY *phPolicy)
{
	struct tsp_object *obj;
	struct tr_nvstore_obj *nvstore;
	TSS_RESULT result=TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&nvstore_list, hNvstore)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	nvstore = (struct tr_nvstore_obj *)obj->data;

	switch (policyType) {
		case TSS_POLICY_USAGE:
			*phPolicy = nvstore->policy;
			break;
		default:
			result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	obj_list_put(&nvstore_list);

	return result;
}

TSS_RESULT
obj_nvstore_get_datapublic(TSS_HNVSTORE hNvstore, UINT32 *size, BYTE *nv_data_public)
{
	struct tsp_object *obj;
	TSS_HCONTEXT  hContext;
	TSS_HTPM hTpm;
	TSS_RESULT result;
	struct tr_nvstore_obj *nvstore;
	UINT32 uiResultLen;
	BYTE *pResult;
	UINT32 i;
	TPM_BOOL defined_index = FALSE;

	if ((obj = obj_list_get_obj(&nvstore_list, hNvstore)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	hContext = obj->tspContext;
	nvstore = (struct tr_nvstore_obj *)obj->data;

	if ((result = obj_tpm_get(hContext, &hTpm)))
		goto out;

	if ((result = Tspi_TPM_GetCapability(hTpm, TSS_TPMCAP_NV_LIST, 0,
				 NULL, &uiResultLen, &pResult))) {
		goto out;
	}

	for (i = 0; i < uiResultLen/sizeof(UINT32); i++) {
		if (nvstore->nvIndex == Decode_UINT32(pResult + i * sizeof(UINT32))) {
			defined_index = TRUE;
			break;
		}
	}

	free_tspi(hContext, pResult);

	if (!defined_index) {
		result = TSPERR(TPM_E_BADINDEX);
		goto out;
	}

	if ((result = Tspi_TPM_GetCapability(hTpm, TSS_TPMCAP_NV_INDEX,
					     sizeof(UINT32), (BYTE *)(&(nvstore->nvIndex)),
					     &uiResultLen, &pResult))) {
		LogDebug("get the index capability error");
		goto out;
	}

	if (uiResultLen > *size) {
		free_tspi(hContext, pResult);
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto out;
	}
	*size = uiResultLen;
	memcpy(nv_data_public, pResult, uiResultLen);
	free_tspi(hContext, pResult);

out:
	obj_list_put(&nvstore_list);
	return result;
}

TSS_RESULT
obj_nvstore_get_readdigestatrelease(TSS_HNVSTORE hNvstore, UINT32 *size, BYTE **data)
{
	BYTE nv_data_public[MAX_PUBLIC_DATA_SIZE];
	UINT32 data_public_size = MAX_PUBLIC_DATA_SIZE;
	UINT32 offset;
	UINT16 pcrread_sizeOfSelect;
	TSS_HCONTEXT tspContext;
	TSS_RESULT result;

	if((result = obj_nvstore_get_datapublic(hNvstore, &data_public_size, nv_data_public)))
		return result;

	if ((result = obj_nvstore_get_tsp_context(hNvstore, &tspContext)))
		return result;

	*size = sizeof(TPM_COMPOSITE_HASH);
	*data = calloc_tspi(tspContext, *size);
	if (*data == NULL) {
		LogError("malloc of %u bytes failed.", *size);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		return result;
	}

	offset = sizeof(TPM_STRUCTURE_TAG)+ sizeof(TPM_NV_INDEX);
	pcrread_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrread_sizeOfSelect + sizeof(TPM_LOCALITY_SELECTION);
	memcpy(*data, nv_data_public + offset, sizeof(TPM_COMPOSITE_HASH));

	return result;
}

TSS_RESULT
obj_nvstore_get_readpcrselection(TSS_HNVSTORE hNvstore, UINT32 *size, BYTE **data)
{
	BYTE nv_data_public[MAX_PUBLIC_DATA_SIZE];
	UINT32 data_public_size = MAX_PUBLIC_DATA_SIZE;
	UINT32 offset;
	UINT16 pcrread_sizeOfSelect;
	TSS_HCONTEXT tspContext;
	TSS_RESULT result;

	if((result = obj_nvstore_get_datapublic(hNvstore, &data_public_size, nv_data_public)))
		return result;

	if ((result = obj_nvstore_get_tsp_context(hNvstore, &tspContext)))
		return result;

	offset = sizeof(TPM_STRUCTURE_TAG)+ sizeof(TPM_NV_INDEX);
	pcrread_sizeOfSelect = Decode_UINT16(nv_data_public + offset);

	*size = sizeof(UINT16) + pcrread_sizeOfSelect;
	*data = calloc_tspi(tspContext, *size);
	if (*data == NULL) {
		LogError("malloc of %u bytes failed.", *size);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		return result;
	}

	memcpy(*data, nv_data_public + offset, *size);

	return result;
}

TSS_RESULT
obj_nvstore_get_writedigestatrelease(TSS_HNVSTORE hNvstore, UINT32 *size, BYTE **data)
{
	BYTE nv_data_public[MAX_PUBLIC_DATA_SIZE];
	UINT32 data_public_size = MAX_PUBLIC_DATA_SIZE;
	UINT32 offset;
	UINT16 pcrread_sizeOfSelect;
	UINT16 pcrwrite_sizeOfSelect;
	TSS_HCONTEXT tspContext;
	TSS_RESULT result;

	if ((result = obj_nvstore_get_datapublic(hNvstore, &data_public_size, nv_data_public)))
		return result;

	if ((result = obj_nvstore_get_tsp_context(hNvstore, &tspContext)))
		return result;

	*size = sizeof(TPM_COMPOSITE_HASH);
	*data = calloc_tspi(tspContext, *size);
	if (*data == NULL) {
		LogError("malloc of %u bytes failed.", *size);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		return result;
	}

	offset = sizeof(TPM_STRUCTURE_TAG)+ sizeof(TPM_NV_INDEX);
	pcrread_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrread_sizeOfSelect
			+ sizeof(TPM_LOCALITY_SELECTION)
			+ sizeof(TPM_COMPOSITE_HASH);

	pcrwrite_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrread_sizeOfSelect + sizeof(TPM_LOCALITY_SELECTION);
	memcpy(*data, nv_data_public + offset, sizeof(TPM_COMPOSITE_HASH));

	return result;
}

TSS_RESULT
obj_nvstore_get_writepcrselection(TSS_HNVSTORE hNvstore, UINT32 *size, BYTE **data)
{
	BYTE nv_data_public[MAX_PUBLIC_DATA_SIZE];
	UINT32 data_public_size = MAX_PUBLIC_DATA_SIZE;
	UINT32 offset;
	UINT16 pcrread_sizeOfSelect;
	UINT16 pcrwrite_sizeOfSelect;
	TSS_HCONTEXT tspContext;
	TSS_RESULT result;

	if((result = obj_nvstore_get_datapublic(hNvstore, &data_public_size, nv_data_public)))
		return result;

	if ((result = obj_nvstore_get_tsp_context(hNvstore, &tspContext)))
		return result;

	offset = sizeof(TPM_STRUCTURE_TAG)+ sizeof(TPM_NV_INDEX);
	pcrread_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrread_sizeOfSelect
			+ sizeof(TPM_LOCALITY_SELECTION)
			+ sizeof(TPM_COMPOSITE_HASH);
	pcrwrite_sizeOfSelect = Decode_UINT16(nv_data_public + offset);

	*size = sizeof(UINT16) + pcrwrite_sizeOfSelect;
	*data = calloc_tspi(tspContext, *size);
	if (*data == NULL) {
		LogError("malloc of %u bytes failed.", *size);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		return result;
	}

	memcpy(*data, nv_data_public + offset, *size);
	return result;
}

TSS_RESULT
obj_nvstore_get_state_readstclear(TSS_HNVSTORE hNvstore, UINT32 * readstclear)
{
	BYTE nv_data_public[MAX_PUBLIC_DATA_SIZE];
	UINT32 data_public_size = MAX_PUBLIC_DATA_SIZE;
	UINT32 offset;
	UINT16 pcrread_sizeOfSelect;
	UINT16 pcrwrite_sizeOfSelect;
	TPM_BOOL value;
	TSS_RESULT result;

	if ((result = obj_nvstore_get_datapublic(hNvstore, &data_public_size, nv_data_public)))
		return result;

	offset = sizeof(TPM_STRUCTURE_TAG)+ sizeof(TPM_NV_INDEX);
	pcrread_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrread_sizeOfSelect
			+ sizeof(TPM_LOCALITY_SELECTION)
			+ sizeof(TPM_COMPOSITE_HASH);

	pcrwrite_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrwrite_sizeOfSelect
			+ sizeof(TPM_LOCALITY_SELECTION)
			+ sizeof(TPM_COMPOSITE_HASH)
			+ sizeof(TPM_NV_ATTRIBUTES);

	value = *((TPM_BOOL *)(nv_data_public + offset));

	*readstclear = value;

	return result;
}

TSS_RESULT
obj_nvstore_get_state_writedefine(TSS_HNVSTORE hNvstore, UINT32 * writedefine)
{
	BYTE nv_data_public[MAX_PUBLIC_DATA_SIZE];
	UINT32 data_public_size = MAX_PUBLIC_DATA_SIZE;
	UINT32 offset;
	UINT16 pcrread_sizeOfSelect;
	UINT16 pcrwrite_sizeOfSelect;
	TPM_BOOL value;
	TSS_RESULT result;

	if ((result = obj_nvstore_get_datapublic(hNvstore, &data_public_size, nv_data_public)))
		return result;

	offset = sizeof(TPM_STRUCTURE_TAG)+ sizeof(TPM_NV_INDEX);
	pcrread_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrread_sizeOfSelect
			+ sizeof(TPM_LOCALITY_SELECTION)
			+ sizeof(TPM_COMPOSITE_HASH);

	pcrwrite_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrwrite_sizeOfSelect
			+ sizeof(TPM_LOCALITY_SELECTION)
			+ sizeof(TPM_COMPOSITE_HASH)
			+ sizeof(TPM_NV_ATTRIBUTES)
			+ sizeof(TPM_BOOL)
			+ sizeof(TPM_BOOL);

	value = *((TPM_BOOL *)(nv_data_public + offset));

	*writedefine = value;

	return result;
}

TSS_RESULT
obj_nvstore_get_state_writestclear(TSS_HNVSTORE hNvstore, UINT32 * writestclear)
{
	BYTE nv_data_public[MAX_PUBLIC_DATA_SIZE];
	UINT32 data_public_size = MAX_PUBLIC_DATA_SIZE;
	UINT32 offset;
	UINT16 pcrread_sizeOfSelect;
	UINT16 pcrwrite_sizeOfSelect;
	TPM_BOOL value;
	TSS_RESULT result;

	if ((result = obj_nvstore_get_datapublic(hNvstore, &data_public_size, nv_data_public)))
		return result;

	offset = sizeof(TPM_STRUCTURE_TAG)+ sizeof(TPM_NV_INDEX);
	pcrread_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrread_sizeOfSelect
			+ sizeof(TPM_LOCALITY_SELECTION)
			+ sizeof(TPM_COMPOSITE_HASH);

	pcrwrite_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrwrite_sizeOfSelect
			+ sizeof(TPM_LOCALITY_SELECTION)
			+ sizeof(TPM_COMPOSITE_HASH)
			+ sizeof(TPM_NV_ATTRIBUTES)
			+ sizeof(TPM_BOOL);

	value = *((TPM_BOOL *)(nv_data_public + offset));
	*writestclear = value;

	return result;
}

TSS_RESULT
obj_nvstore_get_readlocalityatrelease(TSS_HNVSTORE hNvstore, UINT32 * readlocalityatrelease)
{
	BYTE nv_data_public[MAX_PUBLIC_DATA_SIZE];
	UINT32 data_public_size = MAX_PUBLIC_DATA_SIZE;
	UINT32 offset;
	UINT16 pcrread_sizeOfSelect;
	TPM_LOCALITY_SELECTION locality_value;
	TSS_RESULT result;

	if ((result = obj_nvstore_get_datapublic(hNvstore, &data_public_size, nv_data_public)))
		return result;

	offset = sizeof(TPM_STRUCTURE_TAG)+ sizeof(TPM_NV_INDEX);
	pcrread_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrread_sizeOfSelect;
	locality_value = *((TPM_LOCALITY_SELECTION *)(nv_data_public + offset));
	*readlocalityatrelease = locality_value;

	return result;
}

TSS_RESULT
obj_nvstore_get_writelocalityatrelease(TSS_HNVSTORE hNvstore, UINT32 * writelocalityatrelease)
{
	BYTE nv_data_public[MAX_PUBLIC_DATA_SIZE];
	UINT32 data_public_size = MAX_PUBLIC_DATA_SIZE;
	UINT32 offset;
	UINT16 pcrread_sizeOfSelect;
	UINT16 pcrwrite_sizeOfSelect;
	TPM_LOCALITY_SELECTION locality_value;
	TSS_RESULT result;

	if ((result = obj_nvstore_get_datapublic(hNvstore, &data_public_size, nv_data_public)))
		return result;

	offset = sizeof(TPM_STRUCTURE_TAG)+ sizeof(TPM_NV_INDEX);
	pcrread_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrread_sizeOfSelect
			+ sizeof(TPM_LOCALITY_SELECTION)
			+ sizeof(TPM_COMPOSITE_HASH);
	pcrwrite_sizeOfSelect = Decode_UINT16(nv_data_public + offset);
	offset = offset + sizeof(UINT16) + pcrwrite_sizeOfSelect;

	locality_value = *((TPM_LOCALITY_SELECTION *)(nv_data_public + offset));
	*writelocalityatrelease = locality_value;

	return result;
}

TSS_RESULT
obj_nvstore_create_pcrshortinfo(TSS_HNVSTORE hNvstore,
				TSS_HPCRS hPcrComposite,
				UINT32 *size,
				BYTE **data)
{
	struct tsp_object *obj;
	BYTE pdata[MAX_PUBLIC_DATA_SIZE];
	UINT32 dataLen;
	UINT64 offset;
	TSS_HCONTEXT tspContext;
	TSS_RESULT result = TSS_SUCCESS;
	BYTE*  ppbHashData;
	UINT32 i;
	BYTE  digAtRelease[TPM_SHA1_160_HASH_LEN] = { 0, };
	UINT32 tmp_locAtRelease;
	TPM_LOCALITY_SELECTION locAtRelease = TPM_LOC_ZERO | TPM_LOC_ONE
					| TPM_LOC_TWO | TPM_LOC_THREE| TPM_LOC_FOUR;
	BYTE tmp_pcr_select[3] = {0, 0, 0};
	TCPA_PCR_SELECTION pcrSelect = { 3, tmp_pcr_select};

	if ((obj = obj_list_get_obj(&nvstore_list, hNvstore)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	tspContext = obj->tspContext;

	if (hPcrComposite) {
		if ((result = obj_pcrs_get_selection(hPcrComposite, &dataLen, pdata))) {
			LogDebug("get_selection error from hReadPcrComposite");
			goto out;
		}

		/* the index should not >= 24, so the sizeofselect should not be >3*/
		if (dataLen - sizeof(UINT16) > 3) {
			result = TSPERR(TSS_E_BAD_PARAMETER);
			goto out;
		}
		offset = 0;
		Trspi_UnloadBlob_PCR_SELECTION(&offset, pdata, &pcrSelect);

		if (pcrSelect.sizeOfSelect != 0) {
			if ((result = obj_pcrs_get_digest_at_release(hPcrComposite,
								     &dataLen, &ppbHashData))) {
				LogDebug("get_composite error from hReadPcrComposite");
				goto out;
			}
			memcpy(digAtRelease, ppbHashData, dataLen);
			free_tspi(tspContext, ppbHashData);
		} else {
			pcrSelect.sizeOfSelect = 3;
			pcrSelect.pcrSelect = tmp_pcr_select;
		}

		if (pcrSelect.sizeOfSelect < 3) {
			for (i = 0; i < pcrSelect.sizeOfSelect; i++) {
				tmp_pcr_select[i] = pcrSelect.pcrSelect[i];
			}
			pcrSelect.sizeOfSelect = 3;
			pcrSelect.pcrSelect = tmp_pcr_select;
		}

		if ((result = obj_pcrs_get_locality(hPcrComposite, &tmp_locAtRelease)))
			goto out;
		locAtRelease = (TPM_LOCALITY_SELECTION)(tmp_locAtRelease & TSS_LOCALITY_MASK);
	}

	*size = sizeof(UINT16) + 3 + sizeof(TPM_LOCALITY_SELECTION) + TPM_SHA1_160_HASH_LEN;
	*data = calloc_tspi(tspContext, *size);
	if (*data == NULL) {
		LogError("malloc of %u bytes failed.", *size);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto out;
	}

	offset = 0;
	Trspi_LoadBlob_PCR_SELECTION(&offset, *data, &pcrSelect);
	Trspi_LoadBlob_BYTE(&offset, locAtRelease, *data);
	Trspi_LoadBlob(&offset, TPM_SHA1_160_HASH_LEN, *data, digAtRelease);

out:
	obj_list_put(&nvstore_list);
	return result;
}


