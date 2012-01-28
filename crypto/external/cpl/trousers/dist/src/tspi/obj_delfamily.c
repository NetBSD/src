
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
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
#include "tsp_delegate.h"

void
delfamily_free(void *data)
{
	struct tr_delfamily_obj *delfamily = (struct tr_delfamily_obj *)data;

	free(delfamily);
}

TSS_BOOL
obj_is_delfamily(TSS_HOBJECT hObject)
{
	TSS_BOOL answer = FALSE;

	if ((obj_list_get_obj(&delfamily_list, hObject))) {
		answer = TRUE;
		obj_list_put(&delfamily_list);
	}

	return answer;
}

TSS_RESULT
obj_delfamily_add(TSS_HCONTEXT hContext, TSS_HOBJECT *phObject)
{
	TSS_RESULT result;
	struct tr_delfamily_obj *delfamily = calloc(1, sizeof(struct tr_delfamily_obj));

	if (delfamily == NULL) {
		LogError("malloc of %zd bytes failed.",
				sizeof(struct tr_delfamily_obj));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	if ((result = obj_list_add(&delfamily_list, hContext, 0, delfamily, phObject))) {
		free(delfamily);
		return result;
	}

	return TSS_SUCCESS;
}

TSS_RESULT
obj_delfamily_remove(TSS_HDELFAMILY hFamily, TSS_HOBJECT hObject)
{
	TSS_HCONTEXT hContext;
	TSS_RESULT result;

	if (obj_is_tpm(hObject)) {
		if ((result = obj_tpm_get_tsp_context((TSS_HTPM)hObject, &hContext)))
			return result;
	} else
		hContext = (TSS_HCONTEXT)hObject;

	if ((result = obj_list_remove(&delfamily_list, &delfamily_free, hFamily, hContext)))
		return result;

	return TSS_SUCCESS;
}

void
obj_delfamily_find_by_familyid(TSS_HOBJECT hObject, UINT32 familyID, TSS_HDELFAMILY *hFamily)
{
	TSS_HCONTEXT hContext;
	struct tsp_object *obj, *prev = NULL;
	struct obj_list *list = &delfamily_list;
	struct tr_delfamily_obj *delfamily;

	pthread_mutex_lock(&list->lock);

	*hFamily = NULL_HDELFAMILY;

	if (obj_is_tpm(hObject)) {
		if (obj_tpm_get_tsp_context((TSS_HTPM)hObject, &hContext))
			return;
	} else
		hContext = (TSS_HCONTEXT)hObject;

	for (obj = list->head; obj; prev = obj, obj = obj->next) {
		if (obj->tspContext != hContext)
			continue;

		delfamily = (struct tr_delfamily_obj *)obj->data;
		if (delfamily->familyID == familyID) {
			*hFamily = obj->handle;
			break;
		}
	}

	pthread_mutex_unlock(&list->lock);
}

TSS_RESULT
obj_delfamily_get_tsp_context(TSS_HDELFAMILY hFamily, TSS_HCONTEXT *hContext)
{
	struct tsp_object *obj;

	if ((obj = obj_list_get_obj(&delfamily_list, hFamily)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	*hContext = obj->tspContext;

	obj_list_put(&delfamily_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_delfamily_set_locked(TSS_HDELFAMILY hFamily, TSS_BOOL state, TSS_BOOL setInTpm)
{
	struct tsp_object *obj;
	struct tr_delfamily_obj *delfamily;
	TSS_HTPM hTpm;
	UINT32 opDataSize;
	BYTE opData[8];
	UINT32 outDataSize;
	BYTE *outData = NULL;
	UINT64 offset;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&delfamily_list, hFamily)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	delfamily = (struct tr_delfamily_obj *)obj->data;

	if (setInTpm) {
		if ((result = obj_tpm_get(obj->tspContext, &hTpm)))
			goto done;

		offset = 0;
		Trspi_LoadBlob_BOOL(&offset, state, opData);
		opDataSize = offset;
		if ((result = do_delegate_manage(hTpm, delfamily->familyID, TPM_FAMILY_ADMIN,
				opDataSize, opData, &outDataSize, &outData)))
			goto done;
	}

	if (state)
		delfamily->stateFlags |= TSS_DELFAMILY_FLAGS_STATE_LOCKED;
	else
		delfamily->stateFlags &= ~TSS_DELFAMILY_FLAGS_STATE_LOCKED;

done:
	obj_list_put(&delfamily_list);

	free(outData);

	return result;
}

TSS_RESULT
obj_delfamily_get_locked(TSS_HDELFAMILY hFamily, TSS_BOOL *state)
{
	struct tsp_object *obj;
	struct tr_delfamily_obj *delfamily;

	if ((obj = obj_list_get_obj(&delfamily_list, hFamily)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	delfamily = (struct tr_delfamily_obj *)obj->data;

	*state = (delfamily->stateFlags & TSS_DELFAMILY_FLAGS_STATE_LOCKED) ? TRUE : FALSE;

	obj_list_put(&delfamily_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_delfamily_set_enabled(TSS_HDELFAMILY hFamily, TSS_BOOL state, TSS_BOOL setInTpm)
{
	struct tsp_object *obj;
	struct tr_delfamily_obj *delfamily;
	TSS_HTPM hTpm;
	UINT32 opDataSize;
	BYTE opData[8];
	UINT32 outDataSize;
	BYTE *outData = NULL;
	UINT64 offset;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&delfamily_list, hFamily)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	delfamily = (struct tr_delfamily_obj *)obj->data;

	if (setInTpm) {
		if ((result = obj_tpm_get(obj->tspContext, &hTpm)))
			goto done;

		offset = 0;
		Trspi_LoadBlob_BOOL(&offset, state, opData);
		opDataSize = offset;
		if ((result = do_delegate_manage(hTpm, delfamily->familyID, TPM_FAMILY_ENABLE,
				opDataSize, opData, &outDataSize, &outData)))
			goto done;
	}

	if (state)
		delfamily->stateFlags |= TSS_DELFAMILY_FLAGS_STATE_ENABLED;
	else
		delfamily->stateFlags &= ~TSS_DELFAMILY_FLAGS_STATE_ENABLED;

done:
	obj_list_put(&delfamily_list);

	free(outData);

	return result;
}

TSS_RESULT
obj_delfamily_get_enabled(TSS_HDELFAMILY hFamily, TSS_BOOL *state)
{
	struct tsp_object *obj;
	struct tr_delfamily_obj *delfamily;

	if ((obj = obj_list_get_obj(&delfamily_list, hFamily)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	delfamily = (struct tr_delfamily_obj *)obj->data;

	*state = (delfamily->stateFlags & TSS_DELFAMILY_FLAGS_STATE_ENABLED) ? TRUE : FALSE;

	obj_list_put(&delfamily_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_delfamily_set_vercount(TSS_HDELFAMILY hFamily, UINT32 verCount)
{
	struct tsp_object *obj;
	struct tr_delfamily_obj *delfamily;

	if ((obj = obj_list_get_obj(&delfamily_list, hFamily)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	delfamily = (struct tr_delfamily_obj *)obj->data;

	delfamily->verCount = verCount;

	obj_list_put(&delfamily_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_delfamily_get_vercount(TSS_HDELFAMILY hFamily, UINT32 *verCount)
{
	struct tsp_object *obj;
	struct tr_delfamily_obj *delfamily;

	if ((obj = obj_list_get_obj(&delfamily_list, hFamily)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	delfamily = (struct tr_delfamily_obj *)obj->data;

	*verCount = delfamily->verCount;

	obj_list_put(&delfamily_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_delfamily_set_familyid(TSS_HDELFAMILY hFamily, UINT32 familyID)
{
	struct tsp_object *obj;
	struct tr_delfamily_obj *delfamily;

	if ((obj = obj_list_get_obj(&delfamily_list, hFamily)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	delfamily = (struct tr_delfamily_obj *)obj->data;

	delfamily->familyID = familyID;

	obj_list_put(&delfamily_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_delfamily_get_familyid(TSS_HDELFAMILY hFamily, UINT32 *familyID)
{
	struct tsp_object *obj;
	struct tr_delfamily_obj *delfamily;

	if ((obj = obj_list_get_obj(&delfamily_list, hFamily)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	delfamily = (struct tr_delfamily_obj *)obj->data;

	*familyID = delfamily->familyID;

	obj_list_put(&delfamily_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_delfamily_set_label(TSS_HDELFAMILY hFamily, BYTE label)
{
	struct tsp_object *obj;
	struct tr_delfamily_obj *delfamily;

	if ((obj = obj_list_get_obj(&delfamily_list, hFamily)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	delfamily = (struct tr_delfamily_obj *)obj->data;

	delfamily->label = label;

	obj_list_put(&delfamily_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_delfamily_get_label(TSS_HDELFAMILY hFamily, BYTE *label)
{
	struct tsp_object *obj;
	struct tr_delfamily_obj *delfamily;

	if ((obj = obj_list_get_obj(&delfamily_list, hFamily)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	delfamily = (struct tr_delfamily_obj *)obj->data;

	*label = delfamily->label;

	obj_list_put(&delfamily_list);

	return TSS_SUCCESS;
}

