
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2005, 2007
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
obj_encdata_add(TSS_HCONTEXT tspContext, UINT32 type, TSS_HOBJECT *phObject)
{
	TSS_RESULT result;
	struct tr_encdata_obj *encdata = calloc(1, sizeof(struct tr_encdata_obj));

	if (encdata == NULL) {
		LogError("malloc of %zd bytes failed.",
				sizeof(struct tr_encdata_obj));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	/* add usage policy */
	if ((result = obj_context_get_policy(tspContext, TSS_POLICY_USAGE,
					     &encdata->usagePolicy))) {
		free(encdata);
		return result;
	}

	encdata->type = type;

	if ((result = obj_list_add(&encdata_list, tspContext, 0, encdata, phObject))) {
		free(encdata);
		return result;
	}

	return TSS_SUCCESS;
}

TSS_BOOL
obj_is_encdata(TSS_HOBJECT hObject)
{
	TSS_BOOL answer = FALSE;

	if ((obj_list_get_obj(&encdata_list, hObject))) {
		answer = TRUE;
		obj_list_put(&encdata_list);
	}

	return answer;
}

TSS_RESULT
obj_encdata_get_tsp_context(TSS_HENCDATA hEncdata, TSS_HCONTEXT *tspContext)
{
	struct tsp_object *obj;

	if ((obj = obj_list_get_obj(&encdata_list, hEncdata)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	*tspContext = obj->tspContext;

	obj_list_put(&encdata_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_encdata_get_policy(TSS_HENCDATA hEncData, UINT32 policyType, TSS_HPOLICY *phPolicy)
{
	struct tsp_object *obj;
	struct tr_encdata_obj *encdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&encdata_list, hEncData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	encdata = (struct tr_encdata_obj *)obj->data;

	switch (policyType) {
		case TSS_POLICY_USAGE:
			*phPolicy = encdata->usagePolicy;
			break;
		default:
			result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	obj_list_put(&encdata_list);

	return result;
}

TSS_RESULT
obj_encdata_set_policy(TSS_HENCDATA hEncData, TSS_HPOLICY hPolicy)
{
	struct tsp_object *obj;
	struct tr_encdata_obj *encdata;
	UINT32 policyType;
	TSS_RESULT result = TSS_SUCCESS;

	if ((result = obj_policy_get_type(hPolicy, &policyType)))
		return result;

	if ((obj = obj_list_get_obj(&encdata_list, hEncData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	encdata = (struct tr_encdata_obj *)obj->data;

	switch (policyType) {
		case TSS_POLICY_USAGE:
			encdata->usagePolicy = hPolicy;
			break;
		default:
			result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	obj_list_put(&encdata_list);

	return result;
}

TSS_RESULT
obj_encdata_get_data(TSS_HENCDATA hEncData, UINT32 *size, BYTE **data)
{
	struct tsp_object *obj;
	struct tr_encdata_obj *encdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&encdata_list, hEncData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	encdata = (struct tr_encdata_obj *)obj->data;

	if (encdata->encryptedDataLength == 0) {
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
		goto done;
	} else {
		*data = calloc_tspi(obj->tspContext, encdata->encryptedDataLength);
		if (*data == NULL) {
			LogError("malloc of %d bytes failed.",
					encdata->encryptedDataLength);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		*size = encdata->encryptedDataLength;
		memcpy(*data, encdata->encryptedData, *size);
	}

done:
	obj_list_put(&encdata_list);

	return result;
}

TSS_RESULT
obj_encdata_get_pcr_digest(TSS_HENCDATA hEncData,
			   TSS_FLAG pcrInfoType,
			   TSS_FLAG dir,
			   UINT32 *size,
			   BYTE **data)
{
	struct tsp_object *obj;
	struct tr_encdata_obj *encdata;
	TSS_RESULT result = TSS_SUCCESS;
	TPM_DIGEST *digest;
	UINT64 offset;

	if ((obj = obj_list_get_obj(&encdata_list, hEncData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	encdata = (struct tr_encdata_obj *)obj->data;

	if (pcrInfoType != encdata->pcrInfoType) {
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
		goto done;
	}

	switch (pcrInfoType) {
		case TSS_PCRS_STRUCT_INFO:
			if (dir == TSS_TSPATTRIB_ENCDATAPCR_DIGEST_ATCREATION)
				digest = &encdata->pcrInfo.info11.digestAtCreation;
			else if (dir == TSS_TSPATTRIB_ENCDATAPCR_DIGEST_ATRELEASE)
				digest = &encdata->pcrInfo.info11.digestAtRelease;
			else {
				result = TSPERR(TSS_E_BAD_PARAMETER);
				goto done;
			}
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			if (dir == TSS_TSPATTRIB_ENCDATAPCRLONG_DIGEST_ATCREATION)
				digest = &encdata->pcrInfo.infolong.digestAtCreation;
			else if (dir == TSS_TSPATTRIB_ENCDATAPCRLONG_DIGEST_ATRELEASE)
				digest = &encdata->pcrInfo.infolong.digestAtRelease;
			else {
				result = TSPERR(TSS_E_BAD_PARAMETER);
				goto done;
			}
			break;
		default:
			result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
			goto done;
	}

	*size = sizeof(TPM_DIGEST);

	if ((*data = calloc_tspi(obj->tspContext, *size)) == NULL) {
		LogError("malloc of %u bytes failed.", *size);
		*size = 0;
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}

	offset = 0;
	Trspi_LoadBlob_DIGEST(&offset, *data, digest);
done:
	obj_list_put(&encdata_list);

	return result;
}

TSS_RESULT
obj_encdata_get_pcr_locality(TSS_HENCDATA hEncData, TSS_FLAG dir, UINT32 *locality)
{
	struct tsp_object *obj;
	struct tr_encdata_obj *encdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&encdata_list, hEncData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	encdata = (struct tr_encdata_obj *)obj->data;

	if (encdata->pcrInfoType == TSS_PCRS_STRUCT_INFO_LONG) {
		if (dir == TSS_TSPATTRIB_ENCDATAPCRLONG_LOCALITY_ATCREATION)
			*locality = encdata->pcrInfo.infolong.localityAtCreation;
		else if (dir == TSS_TSPATTRIB_ENCDATAPCRLONG_LOCALITY_ATRELEASE)
			*locality = encdata->pcrInfo.infolong.localityAtRelease;
		else
			result = TSPERR(TSS_E_BAD_PARAMETER);
	} else
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);

	obj_list_put(&encdata_list);

	return result;
}

TSS_RESULT
obj_encdata_get_pcr_selection(TSS_HENCDATA hEncData,
			      TSS_FLAG pcrInfoType,
			      TSS_FLAG dir,
			      UINT32 *size,
			      BYTE **data)
{
	struct tsp_object *obj;
	struct tr_encdata_obj *encdata;
	TSS_RESULT result = TSS_SUCCESS;
	TPM_PCR_SELECTION *selection = NULL;
	UINT64 offset;

	if ((obj = obj_list_get_obj(&encdata_list, hEncData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	encdata = (struct tr_encdata_obj *)obj->data;

	if (pcrInfoType != encdata->pcrInfoType) {
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
		goto done;
	}

	switch (pcrInfoType) {
		case TSS_PCRS_STRUCT_INFO:
			if (dir == TSS_TSPATTRIB_ENCDATAPCR_SELECTION)
				selection = &encdata->pcrInfo.info11.pcrSelection;
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			if (dir == TSS_TSPATTRIB_ENCDATAPCRLONG_CREATION_SELECTION)
				selection = &encdata->pcrInfo.infolong.creationPCRSelection;
			else if (dir == TSS_TSPATTRIB_ENCDATAPCRLONG_RELEASE_SELECTION)
				selection = &encdata->pcrInfo.infolong.releasePCRSelection;
			else {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
			break;
		default:
			result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
			goto done;
	}

	*size = sizeof(UINT16) + selection->sizeOfSelect;

	if ((*data = calloc_tspi(obj->tspContext, *size)) == NULL) {
		LogError("malloc of %u bytes failed.", *size);
		*size = 0;
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}

	offset = 0;
	Trspi_LoadBlob_PCR_SELECTION(&offset, *data, selection);
done:
	obj_list_put(&encdata_list);

	return result;
}

TSS_RESULT
obj_encdata_set_pcr_info(TSS_HENCDATA hEncData, UINT32 pcrInfoType, BYTE *info_blob)
{
	struct tsp_object *obj;
	struct tr_encdata_obj *encdata;
	TSS_RESULT result = TSS_SUCCESS;
	UINT64 offset = 0;

	if ((obj = obj_list_get_obj(&encdata_list, hEncData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	encdata = (struct tr_encdata_obj *)obj->data;

	switch (pcrInfoType) {
		case TSS_PCRS_STRUCT_INFO_LONG:
			result = Trspi_UnloadBlob_PCR_INFO_LONG(&offset, info_blob,
								&encdata->pcrInfo.infolong);
			break;
		case TSS_PCRS_STRUCT_INFO:
			result = Trspi_UnloadBlob_PCR_INFO(&offset, info_blob,
							   &encdata->pcrInfo.info11);
			break;
		default:
			result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
			goto done;
	}

	encdata->pcrInfoType = pcrInfoType;

	/* XXX are we using this anywhere? */
	obj->flags |= TSS_OBJ_FLAG_PCRS;
done:
	obj_list_put(&encdata_list);

	return result;
}

TSS_RESULT
obj_encdata_set_data(TSS_HENCDATA hEncData, UINT32 size, BYTE *data)
{
	struct tsp_object *obj;
	struct tr_encdata_obj *encdata;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&encdata_list, hEncData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	encdata = (struct tr_encdata_obj *)obj->data;

	free(encdata->encryptedData);
	encdata->encryptedData = NULL;
	encdata->encryptedDataLength = 0;

	if (size > 0) {
		if ((encdata->encryptedData = malloc(size)) == NULL) {
			LogError("malloc of %u bytes failed.", size);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		encdata->encryptedDataLength = size;
		memcpy(encdata->encryptedData, data, size);
	}

done:
	obj_list_put(&encdata_list);

	return result;
}

void
encdata_free(void *data)
{
	struct tr_encdata_obj *encdata = (struct tr_encdata_obj *)data;

	free(encdata->encryptedData);

	switch (encdata->pcrInfoType) {
		case TSS_PCRS_STRUCT_INFO:
			free(encdata->pcrInfo.info11.pcrSelection.pcrSelect);
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			free(encdata->pcrInfo.infolong.creationPCRSelection.pcrSelect);
			free(encdata->pcrInfo.infolong.releasePCRSelection.pcrSelect);
			break;
		default:
			/* no PCR data was set */
			break;
	}

	free(encdata);
}

/* remove an individual encdata object from the encdata list with handle
 * equal to hObject */
TSS_RESULT
obj_encdata_remove(TSS_HOBJECT hObject, TSS_HCONTEXT tspContext)
{
	TSS_RESULT result;

	if ((result = obj_list_remove(&encdata_list, &encdata_free, hObject, tspContext)))
		return result;

	return TSS_SUCCESS;
}

void
obj_encdata_remove_policy_refs(TSS_HPOLICY hPolicy, TSS_HCONTEXT tspContext)
{
	struct tsp_object *obj, *prev = NULL;
	struct obj_list *list = &encdata_list;
	struct tr_encdata_obj *encdata;

	pthread_mutex_lock(&list->lock);

	for (obj = list->head; obj; prev = obj, obj = obj->next) {
		if (obj->tspContext != tspContext)
			continue;

		encdata = (struct tr_encdata_obj *)obj->data;
		if (encdata->usagePolicy == hPolicy)
			encdata->usagePolicy = NULL_HPOLICY;
	}

	pthread_mutex_unlock(&list->lock);
}

#ifdef TSS_BUILD_SEALX
TSS_RESULT
obj_encdata_set_seal_protect_mode(TSS_HENCDATA hEncData, UINT32 protectMode)
{
	struct tsp_object *obj;
	struct tr_encdata_obj *encdata;

	if ((obj = obj_list_get_obj(&encdata_list, hEncData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	encdata = (struct tr_encdata_obj *)obj->data;

	encdata->protectMode = protectMode;

	obj_list_put(&encdata_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_encdata_get_seal_protect_mode(TSS_HENCDATA hEncData, UINT32 *protectMode)
{
	struct tsp_object *obj;
	struct tr_encdata_obj *encdata;

	if ((obj = obj_list_get_obj(&encdata_list, hEncData)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	encdata = (struct tr_encdata_obj *)obj->data;

	*protectMode = encdata->protectMode;

	obj_list_put(&encdata_list);

	return TSS_SUCCESS;
}
#endif

