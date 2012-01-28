
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

void
tpm_free(void *data)
{
	struct tr_tpm_obj *tpm = (struct tr_tpm_obj *)data;

	free(tpm);
}

TSS_RESULT
obj_tpm_add(TSS_HCONTEXT tspContext, TSS_HOBJECT *phObject)
{
	TSS_RESULT result;
	struct tr_tpm_obj *tpm = calloc(1, sizeof(struct tr_tpm_obj));

	if (tpm == NULL) {
		LogError("malloc of %zd bytes failed.",
				sizeof(struct tr_tpm_obj));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	/* add usage policy */
	if ((result = obj_policy_add(tspContext, TSS_POLICY_USAGE,
					&tpm->policy))) {
		free(tpm);
		return result;
	}

	/* initialize the default ctr_id to inactive until we query the TPM */
	tpm->ctr_id = 0xffffffff;

	if ((result = obj_list_add(&tpm_list, tspContext, 0, tpm, phObject))) {
		free(tpm);
		return result;
	}

	return TSS_SUCCESS;
}

TSS_BOOL
obj_is_tpm(TSS_HOBJECT hObject)
{
	TSS_BOOL answer = FALSE;

	if ((obj_list_get_obj(&tpm_list, hObject))) {
		answer = TRUE;
		obj_list_put(&tpm_list);
	}

	return answer;
}

TSS_RESULT
obj_tpm_set_policy(TSS_HTPM hTpm, TSS_HPOLICY hPolicy)
{
	struct tsp_object *obj;
	struct tr_tpm_obj *tpm;
	UINT32 policyType;
	TSS_RESULT result = TSS_SUCCESS;

	if ((result = obj_policy_get_type(hPolicy, &policyType)))
		return result;

	if ((obj = obj_list_get_obj(&tpm_list, hTpm)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	tpm = (struct tr_tpm_obj *)obj->data;

	switch (policyType) {
		case TSS_POLICY_USAGE:
			tpm->policy = hPolicy;
			break;
#ifdef TSS_BUILD_TSS12
		case TSS_POLICY_OPERATOR:
			tpm->operatorPolicy = hPolicy;
			break;
#endif
		default:
			result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	obj_list_put(&tpm_list);

	return result;
}

TSS_RESULT
obj_tpm_get_policy(TSS_HTPM hTpm, UINT32 policyType, TSS_HPOLICY *phPolicy)
{
	struct tsp_object *obj;
	struct tr_tpm_obj *tpm;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&tpm_list, hTpm)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	tpm = (struct tr_tpm_obj *)obj->data;

	switch (policyType) {
		case TSS_POLICY_USAGE:
			*phPolicy = tpm->policy;
			break;
#ifdef TSS_BUILD_TSS12
		case TSS_POLICY_OPERATOR:
			*phPolicy = tpm->operatorPolicy;
			break;
#endif
		default:
			result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	obj_list_put(&tpm_list);

	return result;
}

TSS_RESULT
obj_tpm_get_tsp_context(TSS_HTPM hTpm, TSS_HCONTEXT *tspContext)
{
	struct tsp_object *obj;

	if ((obj = obj_list_get_obj(&tpm_list, hTpm)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	*tspContext = obj->tspContext;

	obj_list_put(&tpm_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_tpm_get(TSS_HCONTEXT tspContext, TSS_HTPM *phTpm)
{
	struct tsp_object *obj;

	if ((obj = obj_list_get_tspcontext(&tpm_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	*phTpm = obj->handle;

	obj_list_put(&tpm_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_tpm_get_cb11(TSS_HTPM hTpm, TSS_FLAG type, UINT32 *cb)
{
#ifndef __LP64__
	struct tsp_object *obj;
	struct tr_tpm_obj *tpm;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&tpm_list, hTpm)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	tpm = (struct tr_tpm_obj *)obj->data;

	switch (type) {
		case TSS_TSPATTRIB_TPM_CALLBACK_COLLATEIDENTITY:
			*cb = (UINT32)tpm->Tspicb_CollateIdentity;
			break;
		case TSS_TSPATTRIB_TPM_CALLBACK_ACTIVATEIDENTITY:
			*cb = (UINT32)tpm->Tspicb_ActivateIdentity;
			break;
		default:
			result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
			break;
	}

	obj_list_put(&tpm_list);

	return result;
#else
	return TSPERR(TSS_E_FAIL);
#endif
}

TSS_RESULT
obj_tpm_set_cb11(TSS_HTPM hTpm, TSS_FLAG type, TSS_FLAG app_data, UINT32 cb)
{
#ifndef __LP64__
	struct tsp_object *obj;
	struct tr_tpm_obj *tpm;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&tpm_list, hTpm)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	tpm = (struct tr_tpm_obj *)obj->data;

	switch (type) {
		case TSS_TSPATTRIB_TPM_CALLBACK_COLLATEIDENTITY:
			tpm->Tspicb_CollateIdentity = (PVOID)cb;
			tpm->collateAppData = (PVOID)app_data;
			break;
		case TSS_TSPATTRIB_TPM_CALLBACK_ACTIVATEIDENTITY:
			tpm->Tspicb_ActivateIdentity = (PVOID)cb;
			tpm->activateAppData = (PVOID)app_data;
			break;
		default:
			result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
			break;
	}

	obj_list_put(&tpm_list);

	return result;
#else
	return TSPERR(TSS_E_FAIL);
#endif
}

TSS_RESULT
obj_tpm_set_cred(TSS_HTPM hTpm, TSS_FLAG type, UINT32 CredSize, BYTE *CredData)
{
	struct tsp_object *obj;
	struct tr_tpm_obj *tpm;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&tpm_list, hTpm)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	tpm = (struct tr_tpm_obj *)obj->data;

	switch (type) {
		case TSS_TPMATTRIB_EKCERT:
			if ((tpm->EndorsementCred = malloc(CredSize)) == NULL) {
				LogError("malloc of %u bytes failed", CredSize);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
			memcpy(tpm->EndorsementCred, CredData, CredSize);
			tpm->EndorsementCredSize = CredSize;
			break;
		case TSS_TPMATTRIB_TPM_CC:
			if ((tpm->ConformanceCred = malloc(CredSize)) == NULL) {
				LogError("malloc of %u bytes failed", CredSize);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
			memcpy(tpm->ConformanceCred, CredData, CredSize);
			tpm->ConformanceCredSize = CredSize;
			break;
		case TSS_TPMATTRIB_PLATFORMCERT:
			if ((tpm->PlatformCred = malloc(CredSize)) == NULL) {
				LogError("malloc of %u bytes failed", CredSize);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
			memcpy(tpm->PlatformCred, CredData, CredSize);
			tpm->PlatformCredSize = CredSize;
			break;
		case TSS_TPMATTRIB_PLATFORM_CC:
			if ((tpm->PlatformConfCred = malloc(CredSize)) == NULL) {
				LogError("malloc of %u bytes failed", CredSize);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
			memcpy(tpm->PlatformConfCred, CredData, CredSize);
			tpm->PlatformConfCredSize = CredSize;
			break;
		default:
			result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
			break;
	}
done:
	obj_list_put(&tpm_list);

	return result;
}

TSS_RESULT
obj_tpm_get_cred(TSS_HTPM hTpm, TSS_FLAG type, UINT32 *CredSize, BYTE **CredData)
{
	struct tsp_object *obj;
	struct tr_tpm_obj *tpm;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&tpm_list, hTpm)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	tpm = (struct tr_tpm_obj *)obj->data;

	/* get the size of data we need to allocate */
	switch (type) {
		case TSS_TPMATTRIB_EKCERT:
			*CredSize = tpm->EndorsementCredSize;
			break;
		case TSS_TPMATTRIB_TPM_CC:
			*CredSize = tpm->ConformanceCredSize;
			break;
		case TSS_TPMATTRIB_PLATFORMCERT:
			*CredSize = tpm->PlatformCredSize;
			break;
		case TSS_TPMATTRIB_PLATFORM_CC:
			*CredSize = tpm->PlatformConfCredSize;
			break;
		default:
			LogError("Credential type is unknown");
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
	}

	if (*CredSize == 0) {
		*CredData = NULL;
		goto done;
	}

	if ((*CredData = calloc_tspi(obj->tspContext, *CredSize)) == NULL) {
		*CredSize = 0;
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}

	switch (type) {
		case TSS_TPMATTRIB_EKCERT:
			memcpy(*CredData, tpm->EndorsementCred, *CredSize);
			break;
		case TSS_TPMATTRIB_TPM_CC:
			memcpy(*CredData, tpm->ConformanceCred, *CredSize);
			break;
		case TSS_TPMATTRIB_PLATFORMCERT:
			memcpy(*CredData, tpm->PlatformCred, *CredSize);
			break;
		case TSS_TPMATTRIB_PLATFORM_CC:
			memcpy(*CredData, tpm->PlatformConfCred, *CredSize);
			break;
		default:
			result = TSPERR(TSS_E_BAD_PARAMETER);
			*CredSize = 0;
			free(*CredData);
			*CredData = NULL;
			break;
	}

done:
	obj_list_put(&tpm_list);
	return result;
}

TSS_RESULT
obj_tpm_set_cb12(TSS_HTPM hTpm, TSS_FLAG flag, BYTE *in)
{
	struct tsp_object *obj;
	struct tr_tpm_obj *tpm;
	TSS_RESULT result = TSS_SUCCESS;
	TSS_CALLBACK *cb = (TSS_CALLBACK *)in;

	if ((obj = obj_list_get_obj(&tpm_list, hTpm)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	tpm = (struct tr_tpm_obj *)obj->data;

	switch (flag) {
		case TSS_TSPATTRIB_TPM_CALLBACK_COLLATEIDENTITY:
			if (!cb) {
				tpm->Tspicb_CollateIdentity = NULL;
				break;
			}

			tpm->Tspicb_CollateIdentity = (TSS_RESULT (*)(PVOID,
				UINT32, BYTE *, TSS_ALGORITHM_ID, UINT32 *,
				BYTE *, UINT32 *, BYTE *))cb->callback;
			tpm->collateAppData = cb->appData;
			tpm->collateAlg = cb->alg;
			break;
		case TSS_TSPATTRIB_TPM_CALLBACK_ACTIVATEIDENTITY:
			if (!cb) {
				tpm->Tspicb_ActivateIdentity = NULL;
				break;
			}

			tpm->Tspicb_ActivateIdentity = (TSS_RESULT (*)(PVOID,
				UINT32, BYTE *, UINT32, BYTE *, UINT32 *,
				BYTE *))cb->callback;
			tpm->activateAppData = cb->appData;
			tpm->activateAlg = cb->alg;
			break;
		default:
			result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
			break;
	}

	obj_list_put(&tpm_list);

	return result;
}

TSS_RESULT
obj_tpm_get_cb12(TSS_HTPM hTpm, TSS_FLAG flag, UINT32 *size, BYTE **out)
{
	struct tsp_object *obj;
	struct tr_tpm_obj *tpm;
	TSS_RESULT result = TSS_SUCCESS;
	TSS_CALLBACK *cb;

	if ((obj = obj_list_get_obj(&tpm_list, hTpm)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	tpm = (struct tr_tpm_obj *)obj->data;

	if ((cb = calloc_tspi(obj->tspContext, sizeof(TSS_CALLBACK))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(TSS_CALLBACK));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}

	switch (flag) {
		case TSS_TSPATTRIB_TPM_CALLBACK_COLLATEIDENTITY:
			cb->callback = tpm->Tspicb_CollateIdentity;
			cb->appData = tpm->collateAppData;
			cb->alg = tpm->collateAlg;
			*size = sizeof(TSS_CALLBACK);
			*out = (BYTE *)cb;
			break;
		case TSS_TSPATTRIB_TPM_CALLBACK_ACTIVATEIDENTITY:
			cb->callback = tpm->Tspicb_ActivateIdentity;
			cb->appData = tpm->activateAppData;
			cb->alg = tpm->activateAlg;
			*size = sizeof(TSS_CALLBACK);
			*out = (BYTE *)cb;
			break;
		default:
			free_tspi(obj->tspContext, cb);
			result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
			break;
	}
done:
	obj_list_put(&tpm_list);

	return result;
}

void
obj_tpm_remove_policy_refs(TSS_HPOLICY hPolicy, TSS_HCONTEXT tspContext)
{
	struct tsp_object *obj, *prev = NULL;
	struct obj_list *list = &tpm_list;
	struct tr_tpm_obj *tpm;

	pthread_mutex_lock(&list->lock);

	for (obj = list->head; obj; prev = obj, obj = obj->next) {
		if (obj->tspContext != tspContext)
			continue;

		tpm = (struct tr_tpm_obj *)obj->data;
		if (tpm->policy == hPolicy)
			tpm->policy = NULL_HPOLICY;
#ifdef TSS_BUILD_TSS12
		if (tpm->operatorPolicy == hPolicy)
			tpm->operatorPolicy = NULL_HPOLICY;
#endif
	}

	pthread_mutex_unlock(&list->lock);
}

#ifdef TSS_BUILD_COUNTER
TSS_RESULT
obj_tpm_get_current_counter(TSS_HTPM hTPM, TSS_COUNTER_ID *ctr_id)
{
	struct tsp_object *obj;
	struct tr_tpm_obj *tpm;
	TSS_RESULT result = TSS_SUCCESS;
	UINT32 respLen, subCap = endian32(TPM_CAP_PROP_ACTIVE_COUNTER);
	BYTE *resp;

	if ((obj = obj_list_get_obj(&tpm_list, hTPM)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	tpm = (struct tr_tpm_obj *)obj->data;

	if (tpm->ctr_id != 0xffffffff) {
		*ctr_id = tpm->ctr_id;
		goto done;
	}

	/* No counter has yet been associated with the TPM object, so let the TPM object lock
	 * protect us here and get a counter ID */
	if ((result = TCS_API(obj->tspContext)->GetTPMCapability(obj->tspContext, TPM_CAP_PROPERTY,
								 sizeof(UINT32), (BYTE *)&subCap,
								 &respLen, &resp)))
		goto done;

	if (respLen != sizeof(UINT32)) {
		LogDebug("TPM GetCap response size isn't sizeof(UINT32)!");
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto done;
	}

	memcpy(&tpm->ctr_id, resp, respLen);
	free(resp);

	if (tpm->ctr_id == 0xffffffff) {
		result = TSPERR(TSS_E_NO_ACTIVE_COUNTER);
		goto done;
	}
	*ctr_id = tpm->ctr_id;
done:
	obj_list_put(&tpm_list);

	return result;
}
#endif

