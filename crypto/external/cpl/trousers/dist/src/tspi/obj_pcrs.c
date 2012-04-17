
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
obj_pcrs_add(TSS_HCONTEXT tspContext, UINT32 type, TSS_HOBJECT *phObject)
{
	TSS_RESULT result;
	UINT32 ver;
	struct tr_pcrs_obj *pcrs;

	if ((pcrs = calloc(1, sizeof(struct tr_pcrs_obj))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct tr_pcrs_obj));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	if (type == TSS_PCRS_STRUCT_DEFAULT) {
		if ((result = obj_context_get_connection_version(tspContext, &ver))) {
			free(pcrs);
			return result;
		}

		switch (ver) {
			case TSS_TSPATTRIB_CONTEXT_VERSION_V1_2:
				pcrs->type = TSS_PCRS_STRUCT_INFO_LONG;
				pcrs->info.infolong.localityAtRelease = TSS_LOCALITY_ALL;
				break;
			case TSS_TSPATTRIB_CONTEXT_VERSION_V1_1:
				/* fall through */
			default:
				pcrs->type = TSS_PCRS_STRUCT_INFO;
				break;
		}
	} else
		pcrs->type = type;

	if ((result = obj_list_add(&pcrs_list, tspContext, 0, pcrs, phObject))) {
		free(pcrs);
		return result;
	}

	return TSS_SUCCESS;
}

void
pcrs_free(void *data)
{
	struct tr_pcrs_obj *pcrs = (struct tr_pcrs_obj *)data;

	switch (pcrs->type) {
		case TSS_PCRS_STRUCT_INFO:
			free(pcrs->info.info11.pcrSelection.pcrSelect);
			break;
		case TSS_PCRS_STRUCT_INFO_SHORT:
			free(pcrs->info.infoshort.pcrSelection.pcrSelect);
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			free(pcrs->info.infolong.creationPCRSelection.pcrSelect);
			free(pcrs->info.infolong.releasePCRSelection.pcrSelect);
			break;
		default:
			LogDebugFn("Undefined type of PCRs object");
			break;
	}

	free(pcrs);
}

TSS_RESULT
obj_pcrs_remove(TSS_HOBJECT hObject, TSS_HCONTEXT tspContext)
{
	TSS_RESULT result;

	if ((result = obj_list_remove(&pcrs_list, &pcrs_free, hObject, tspContext)))
		return result;

	return TSS_SUCCESS;
}

TSS_BOOL
obj_is_pcrs(TSS_HOBJECT hObject)
{
	TSS_BOOL answer = FALSE;

	if ((obj_list_get_obj(&pcrs_list, hObject))) {
		answer = TRUE;
		obj_list_put(&pcrs_list);
	}

	return answer;
}

TSS_RESULT
obj_pcrs_get_tsp_context(TSS_HPCRS hPcrs, TSS_HCONTEXT *tspContext)
{
	struct tsp_object *obj;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	*tspContext = obj->tspContext;

	obj_list_put(&pcrs_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_pcrs_get_type(TSS_HPCRS hPcrs, UINT32 *type)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	*type = pcrs->type;

	obj_list_put(&pcrs_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_pcrs_get_selection(TSS_HPCRS hPcrs, UINT32 *size, BYTE *out)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	TPM_PCR_SELECTION *tmp;
	UINT64 offset = 0;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	switch (pcrs->type) {
		case TSS_PCRS_STRUCT_INFO:
			tmp = &pcrs->info.info11.pcrSelection;
			break;
		case TSS_PCRS_STRUCT_INFO_SHORT:
			tmp = &pcrs->info.infoshort.pcrSelection;
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			tmp = &pcrs->info.infolong.creationPCRSelection;
			break;
		default:
			LogDebugFn("Undefined type of PCRs object");
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
	}

	Trspi_LoadBlob_PCR_SELECTION(&offset, out, tmp);
	*size = offset;
done:
	obj_list_put(&pcrs_list);

	return result;
}

TSS_RESULT
obj_pcrs_set_values(TSS_HPCRS hPcrs, TPM_PCR_COMPOSITE *pcrComp)
{
	TSS_RESULT result = TSS_SUCCESS;
	TPM_PCR_SELECTION *select = &(pcrComp->select);
	UINT16 i, val_idx = 0;

	for (i = 0; i < select->sizeOfSelect * 8; i++) {
		if (select->pcrSelect[i / 8] & (1 << (i % 8))) {
			if ((result = obj_pcrs_set_value(hPcrs, i, TCPA_SHA1_160_HASH_LEN,
							 (BYTE *)&pcrComp->pcrValue[val_idx])))
				return result;

			val_idx++;
		}
	}

	return result;
}

TSS_RESULT
obj_pcrs_set_value(TSS_HPCRS hPcrs, UINT32 idx, UINT32 size, BYTE *value)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	TPM_PCR_SELECTION *select;
	TPM_COMPOSITE_HASH *compHash;
	UINT16 bytes_to_hold = (idx / 8) + 1;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	switch(pcrs->type) {
		case TSS_PCRS_STRUCT_INFO:
			bytes_to_hold = (bytes_to_hold < 2) ? 2 : bytes_to_hold;
			select = &pcrs->info.info11.pcrSelection;
			compHash = &pcrs->info.info11.digestAtRelease;
			break;
		case TSS_PCRS_STRUCT_INFO_SHORT:
			bytes_to_hold = (bytes_to_hold < 3) ? 3 : bytes_to_hold;
			select = &pcrs->info.infoshort.pcrSelection;
			compHash = &pcrs->info.infoshort.digestAtRelease;
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			bytes_to_hold = (bytes_to_hold < 3) ? 3 : bytes_to_hold;
			select = &pcrs->info.infolong.releasePCRSelection;
			compHash = &pcrs->info.infolong.digestAtRelease;
			break;
		default:
			LogDebugFn("Undefined type of PCRs object");
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
			break;
	}

	/* allocate the selection structure */
	if (select->pcrSelect == NULL) {
		if ((select->pcrSelect = malloc(bytes_to_hold)) == NULL) {
			LogError("malloc of %d bytes failed.", bytes_to_hold);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		select->sizeOfSelect = bytes_to_hold;
		memset(select->pcrSelect, 0, bytes_to_hold);

		/* allocate the pcr array */
		if ((pcrs->pcrs = malloc(bytes_to_hold * 8 *
					 TCPA_SHA1_160_HASH_LEN)) == NULL) {
			LogError("malloc of %d bytes failed.",
				bytes_to_hold * 8 * TCPA_SHA1_160_HASH_LEN);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
	} else if (select->sizeOfSelect < bytes_to_hold) {
		if ((select->pcrSelect = realloc(select->pcrSelect, bytes_to_hold)) == NULL) {
			LogError("malloc of %d bytes failed.", bytes_to_hold);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		/* set the newly allocated bytes to 0 */
		memset(&select->pcrSelect[select->sizeOfSelect], 0,
				bytes_to_hold - select->sizeOfSelect);
		select->sizeOfSelect = bytes_to_hold;

		/* realloc the pcrs array */
		if ((pcrs->pcrs = realloc(pcrs->pcrs, bytes_to_hold * 8 *
					  sizeof(TPM_PCRVALUE))) == NULL) {
			LogError("malloc of %d bytes failed.",
					bytes_to_hold * 8 * TCPA_SHA1_160_HASH_LEN);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
	}

	/* set the bit in the selection structure */
	select->pcrSelect[idx / 8] |= (1 << (idx % 8));

	/* set the value in the pcrs array */
	memcpy(&(pcrs->pcrs[idx]), value, size);

	result = pcrs_calc_composite(select, pcrs->pcrs, compHash);

done:
	obj_list_put(&pcrs_list);

	return result;
}

TSS_RESULT
obj_pcrs_get_value(TSS_HPCRS hPcrs, UINT32 idx, UINT32 *size, BYTE **value)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	TPM_PCR_SELECTION *select;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	switch(pcrs->type) {
		case TSS_PCRS_STRUCT_INFO:
			select = &pcrs->info.info11.pcrSelection;
			break;
		case TSS_PCRS_STRUCT_INFO_SHORT:
			select = &pcrs->info.infoshort.pcrSelection;
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			select = &pcrs->info.infolong.creationPCRSelection;
			break;
		default:
			LogDebugFn("Undefined type of PCRs object");
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
			break;
	}

	if (select->sizeOfSelect < (idx / 8) + 1) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}

	if ((*value = calloc_tspi(obj->tspContext, TCPA_SHA1_160_HASH_LEN)) == NULL) {
		LogError("malloc of %d bytes failed.", TCPA_SHA1_160_HASH_LEN);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}

	*size = TCPA_SHA1_160_HASH_LEN;
	memcpy(*value, &pcrs->pcrs[idx], TCPA_SHA1_160_HASH_LEN);

done:
	obj_list_put(&pcrs_list);

	return result;
}

TSS_RESULT
obj_pcrs_get_digest_at_release(TSS_HPCRS hPcrs, UINT32 *size, BYTE **out)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	BYTE *digest;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	switch(pcrs->type) {
		case TSS_PCRS_STRUCT_INFO:
			result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
			goto done;
		case TSS_PCRS_STRUCT_INFO_SHORT:
			digest = (BYTE *)&pcrs->info.infoshort.digestAtRelease;
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			digest = (BYTE *)&pcrs->info.infolong.digestAtRelease;
			break;
		default:
			LogDebugFn("Undefined type of PCRs object");
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
			break;
	}

	if ((*out = calloc_tspi(obj->tspContext, sizeof(TPM_COMPOSITE_HASH))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(TPM_COMPOSITE_HASH));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	memcpy(*out, digest, sizeof(TPM_COMPOSITE_HASH));
	*size = sizeof(TPM_COMPOSITE_HASH);

done:
	obj_list_put(&pcrs_list);

	return result;
}

TSS_RESULT
obj_pcrs_select_index(TSS_HPCRS hPcrs, UINT32 idx)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	TPM_PCR_SELECTION *select;
	UINT16 bytes_to_hold = (idx / 8) + 1;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	switch(pcrs->type) {
		case TSS_PCRS_STRUCT_INFO:
			bytes_to_hold = (bytes_to_hold < 2) ? 2 : bytes_to_hold;
			select = &pcrs->info.info11.pcrSelection;
			break;
		case TSS_PCRS_STRUCT_INFO_SHORT:
		case TSS_PCRS_STRUCT_INFO_LONG:
			result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
			goto done;
		default:
			LogDebugFn("Undefined type of PCRs object");
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
			break;
	}

	/* allocate the selection structure */
	if (select->pcrSelect == NULL) {
		if ((select->pcrSelect = malloc(bytes_to_hold)) == NULL) {
			LogError("malloc of %d bytes failed.", bytes_to_hold);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		select->sizeOfSelect = bytes_to_hold;
		memset(select->pcrSelect, 0, bytes_to_hold);

		/* alloc the pcrs array */
		if ((pcrs->pcrs = malloc(bytes_to_hold * 8 * TCPA_SHA1_160_HASH_LEN)) == NULL) {
			LogError("malloc of %d bytes failed.", bytes_to_hold * 8 *
				 TCPA_SHA1_160_HASH_LEN);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
	} else if (select->sizeOfSelect < bytes_to_hold) {
		if ((select->pcrSelect = realloc(select->pcrSelect, bytes_to_hold)) == NULL) {
			LogError("malloc of %d bytes failed.", bytes_to_hold);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		/* set the newly allocated bytes to 0 */
		memset(&select->pcrSelect[select->sizeOfSelect], 0,
		       bytes_to_hold - select->sizeOfSelect);
		select->sizeOfSelect = bytes_to_hold;

		/* realloc the pcrs array */
		if ((pcrs->pcrs = realloc(pcrs->pcrs,
					  bytes_to_hold * 8 * TCPA_SHA1_160_HASH_LEN)) == NULL) {
			LogError("malloc of %d bytes failed.", bytes_to_hold * 8 *
				 TCPA_SHA1_160_HASH_LEN);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
	}

	/* set the bit in the selection structure */
	select->pcrSelect[idx / 8] |= (1 << (idx % 8));

done:
	obj_list_put(&pcrs_list);

	return result;
}

TSS_RESULT
obj_pcrs_select_index_ex(TSS_HPCRS hPcrs, UINT32 dir, UINT32 idx)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	TPM_PCR_SELECTION *select;
	UINT16 bytes_to_hold = (idx / 8) + 1;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	switch(pcrs->type) {
		case TSS_PCRS_STRUCT_INFO:
			result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
			goto done;
		case TSS_PCRS_STRUCT_INFO_SHORT:
			if (dir == TSS_PCRS_DIRECTION_CREATION) {
				result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
				goto done;
			}
			bytes_to_hold = (bytes_to_hold < 3) ? 3 : bytes_to_hold;
			select = &pcrs->info.infoshort.pcrSelection;
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			bytes_to_hold = (bytes_to_hold < 3) ? 3 : bytes_to_hold;
			if (dir == TSS_PCRS_DIRECTION_CREATION)
				select = &pcrs->info.infolong.creationPCRSelection;
			else
				select = &pcrs->info.infolong.releasePCRSelection;
			break;
		default:
			LogDebugFn("Undefined type of PCRs object");
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
			break;
	}

	/* allocate the selection structure */
	if (select->pcrSelect == NULL) {
		if ((select->pcrSelect = malloc(bytes_to_hold)) == NULL) {
			LogError("malloc of %d bytes failed.", bytes_to_hold);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		select->sizeOfSelect = bytes_to_hold;
		memset(select->pcrSelect, 0, bytes_to_hold);

		/* alloc the pcrs array */
		if ((pcrs->pcrs = malloc(bytes_to_hold * 8 * TCPA_SHA1_160_HASH_LEN)) == NULL) {
			LogError("malloc of %d bytes failed.", bytes_to_hold * 8 *
				 TCPA_SHA1_160_HASH_LEN);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
	} else if (select->sizeOfSelect < bytes_to_hold) {
		if ((select->pcrSelect = realloc(select->pcrSelect, bytes_to_hold)) == NULL) {
			LogError("malloc of %d bytes failed.", bytes_to_hold);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		/* set the newly allocated bytes to 0 */
		memset(&select->pcrSelect[select->sizeOfSelect], 0,
		       bytes_to_hold - select->sizeOfSelect);
		select->sizeOfSelect = bytes_to_hold;

		/* realloc the pcrs array */
		if ((pcrs->pcrs = realloc(pcrs->pcrs,
					  bytes_to_hold * 8 * TCPA_SHA1_160_HASH_LEN)) == NULL) {
			LogError("malloc of %d bytes failed.", bytes_to_hold * 8 *
				 TCPA_SHA1_160_HASH_LEN);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
	}

	/* set the bit in the selection structure */
	select->pcrSelect[idx / 8] |= (1 << (idx % 8));

done:
	obj_list_put(&pcrs_list);

	return result;
}

TSS_RESULT
obj_pcrs_create_info_type(TSS_HPCRS hPcrs, UINT32 *type, UINT32 *size, BYTE **info)
{
	TSS_RESULT result;

	/* If type equals 0, then we create the structure
	   based on how the object was created */
	if (*type == 0) {
		struct tsp_object *obj;
		struct tr_pcrs_obj *pcrs;

		if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
			return TSPERR(TSS_E_INVALID_HANDLE);

		pcrs = (struct tr_pcrs_obj *)obj->data;
		*type = pcrs->type;

		obj_list_put(&pcrs_list);
	}

	switch (*type) {
	case TSS_PCRS_STRUCT_INFO:
		result = obj_pcrs_create_info(hPcrs, size, info);
		break;
	case TSS_PCRS_STRUCT_INFO_LONG:
		result = obj_pcrs_create_info_long(hPcrs, size, info);
		break;
	case TSS_PCRS_STRUCT_INFO_SHORT:
		result = obj_pcrs_create_info_short(hPcrs, size, info);
		break;
	default:
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

/* Create a PCR info struct based on the hPcrs object */
TSS_RESULT
obj_pcrs_create_info(TSS_HPCRS hPcrs, UINT32 *size, BYTE **info)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	TPM_PCR_INFO info11;
	UINT64 offset;
	UINT32 ret_size;
	BYTE *ret;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	/* Set everything that is not assigned to be all zeroes */
	memset(&info11, 0, sizeof(info11));

	switch (pcrs->type) {
		case TSS_PCRS_STRUCT_INFO:
			info11 = pcrs->info.info11;
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			info11.pcrSelection = pcrs->info.infolong.releasePCRSelection;
			info11.digestAtRelease = pcrs->info.infolong.digestAtRelease;
			break;
		case TSS_PCRS_STRUCT_INFO_SHORT:
			info11.pcrSelection = pcrs->info.infoshort.pcrSelection;
			info11.digestAtRelease = pcrs->info.infoshort.digestAtRelease;
			break;
		default:
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
	}

	offset = 0;
	Trspi_LoadBlob_PCR_INFO(&offset, NULL, &info11);
	ret_size = offset;

	if ((ret = calloc(1, ret_size)) == NULL) {
		result = TSPERR(TSS_E_OUTOFMEMORY);
		LogDebug("malloc of %u bytes failed.", ret_size);
		goto done;
	}

	offset = 0;
	Trspi_LoadBlob_PCR_INFO(&offset, ret, &info11);

	*info = ret;
	*size = ret_size;

done:
	obj_list_put(&pcrs_list);

	return result;
}

TSS_RESULT
obj_pcrs_create_info_long(TSS_HPCRS hPcrs, UINT32 *size, BYTE **info)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	TPM_PCR_INFO_LONG infolong;
	BYTE dummyBits[3] = { 0, 0, 0 };
	TPM_PCR_SELECTION dummySelection = { 3, dummyBits };
	UINT64 offset;
	UINT32 ret_size;
	BYTE *ret;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	/* Set everything that is not assigned to be all zeroes */
	memset(&infolong, 0, sizeof(infolong));

	infolong.tag = TPM_TAG_PCR_INFO_LONG;
	/* localityAtCreation and creationPCRSelection certainly do not need to be set here, but
	 * some chips such as Winbond do not ignore them on input, so we must give them dummy
	 * "good" values */
	infolong.localityAtCreation = TPM_LOC_ZERO;
	infolong.creationPCRSelection = dummySelection;
	switch (pcrs->type) {
		case TSS_PCRS_STRUCT_INFO:
			infolong.localityAtRelease = TSS_LOCALITY_ALL;
			infolong.releasePCRSelection = pcrs->info.info11.pcrSelection;
			infolong.digestAtRelease = pcrs->info.info11.digestAtRelease;
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			infolong.localityAtRelease = pcrs->info.infolong.localityAtRelease;
			infolong.releasePCRSelection = pcrs->info.infolong.releasePCRSelection;
			infolong.digestAtRelease = pcrs->info.infolong.digestAtRelease;
			break;
		case TSS_PCRS_STRUCT_INFO_SHORT:
			infolong.localityAtRelease = pcrs->info.infoshort.localityAtRelease;
			infolong.releasePCRSelection = pcrs->info.infoshort.pcrSelection;
			infolong.digestAtRelease = pcrs->info.infoshort.digestAtRelease;
			break;
		default:
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
	}

	offset = 0;
	Trspi_LoadBlob_PCR_INFO_LONG(&offset, NULL, &infolong);
	ret_size = offset;

	if ((ret = calloc(1, ret_size)) == NULL) {
		result = TSPERR(TSS_E_OUTOFMEMORY);
		LogDebug("malloc of %u bytes failed.", ret_size);
		goto done;
	}

	offset = 0;
	Trspi_LoadBlob_PCR_INFO_LONG(&offset, ret, &infolong);

	*info = ret;
	*size = ret_size;

done:
	obj_list_put(&pcrs_list);

	return result;
}

TSS_RESULT
obj_pcrs_create_info_short(TSS_HPCRS hPcrs, UINT32 *size, BYTE **info)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	TPM_PCR_INFO_SHORT infoshort;
	BYTE select[] = { 0, 0, 0 };
	UINT64 offset;
	UINT32 ret_size;
	BYTE *ret;

	/* Set everything that is not assigned to be all zeroes */
	memset(&infoshort, 0, sizeof(infoshort));

	if (hPcrs != NULL_HPCRS) {
		if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
			return TSPERR(TSS_E_INVALID_HANDLE);

		pcrs = (struct tr_pcrs_obj *)obj->data;

		switch (pcrs->type) {
			case TSS_PCRS_STRUCT_INFO:
				infoshort.pcrSelection = pcrs->info.info11.pcrSelection;
				infoshort.localityAtRelease = TSS_LOCALITY_ALL;
				infoshort.digestAtRelease = pcrs->info.info11.digestAtRelease;
				break;
			case TSS_PCRS_STRUCT_INFO_LONG:
				infoshort.pcrSelection = pcrs->info.infolong.releasePCRSelection;
				infoshort.localityAtRelease = pcrs->info.infolong.localityAtRelease;
				infoshort.digestAtRelease = pcrs->info.infolong.digestAtRelease;
				break;
			case TSS_PCRS_STRUCT_INFO_SHORT:
				infoshort = pcrs->info.infoshort;
				break;
			default:
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
		}
	} else {
		infoshort.pcrSelection.sizeOfSelect = sizeof(select);
		infoshort.pcrSelection.pcrSelect = select;
		infoshort.localityAtRelease = TSS_LOCALITY_ALL;
	}

	offset = 0;
	Trspi_LoadBlob_PCR_INFO_SHORT(&offset, NULL, &infoshort);
	ret_size = offset;

	if ((ret = calloc(1, ret_size)) == NULL) {
		result = TSPERR(TSS_E_OUTOFMEMORY);
		LogDebug("malloc of %u bytes failed.", ret_size);
		goto done;
	}

	offset = 0;
	Trspi_LoadBlob_PCR_INFO_SHORT(&offset, ret, &infoshort);

	*info = ret;
	*size = ret_size;

done:
	if (hPcrs != NULL_HPCRS)
		obj_list_put(&pcrs_list);

	return result;
}

TSS_RESULT
obj_pcrs_get_locality(TSS_HPCRS hPcrs, UINT32 *out)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	BYTE *locality;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	switch(pcrs->type) {
		case TSS_PCRS_STRUCT_INFO:
			result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
			goto done;
		case TSS_PCRS_STRUCT_INFO_SHORT:
			locality = &pcrs->info.infoshort.localityAtRelease;
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			locality = &pcrs->info.infolong.localityAtRelease;
			break;
		default:
			LogDebugFn("Undefined type of PCRs object");
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
	}

	*out = (UINT32)*locality;

done:
	obj_list_put(&pcrs_list);

	return result;
}

TSS_RESULT
obj_pcrs_set_locality(TSS_HPCRS hPcrs, UINT32 locality)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	BYTE *loc;

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	switch(pcrs->type) {
		case TSS_PCRS_STRUCT_INFO:
			result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
			goto done;
		case TSS_PCRS_STRUCT_INFO_SHORT:
			loc = &pcrs->info.infoshort.localityAtRelease;
			break;
		case TSS_PCRS_STRUCT_INFO_LONG:
			loc = &pcrs->info.infolong.localityAtRelease;
			break;
		default:
			LogDebugFn("Undefined type of PCRs object");
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
	}

	*loc = locality;
done:
	obj_list_put(&pcrs_list);

	return result;
}

TSS_RESULT
obj_pcrs_set_digest_at_release(TSS_HPCRS hPcrs, TPM_COMPOSITE_HASH digest)
{
	struct tsp_object *obj;
	struct tr_pcrs_obj *pcrs;
	TSS_RESULT result = TSS_SUCCESS;
	TPM_COMPOSITE_HASH *dig;

	LogDebugFn("######## Digest to be set on TSS object:");
	LogDebugData(TCPA_SHA1_160_HASH_LEN, digest.digest);

	if ((obj = obj_list_get_obj(&pcrs_list, hPcrs)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	pcrs = (struct tr_pcrs_obj *)obj->data;

	switch(pcrs->type) {
	case TSS_PCRS_STRUCT_INFO:
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
		goto done;
	case TSS_PCRS_STRUCT_INFO_SHORT:
		dig = &pcrs->info.infoshort.digestAtRelease;
		break;
	case TSS_PCRS_STRUCT_INFO_LONG:
		dig = &pcrs->info.infolong.digestAtRelease;
		break;
	default:
		LogDebugFn("Undefined type of PCRs object");
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto done;
	}

	/* Copy the digest information */
	memcpy(dig->digest,&digest.digest,TPM_SHA1_160_HASH_LEN);

	LogDebugFn("######## Digest SET on TSS object:");
	LogDebugData(TCPA_SHA1_160_HASH_LEN,pcrs->info.infoshort.digestAtRelease.digest);

done:
	obj_list_put(&pcrs_list);

	return result;
}

