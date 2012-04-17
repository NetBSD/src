
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
obj_hash_add(TSS_HCONTEXT tspContext, UINT32 type, TSS_HOBJECT *phObject)
{
	TSS_RESULT result;
	struct tr_hash_obj *hash = calloc(1, sizeof(struct tr_hash_obj));

	if (hash == NULL) {
		LogError("malloc of %zd bytes failed.",
				sizeof(struct tr_hash_obj));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	if ((type == TSS_HASH_SHA1) ||
	    (type == TSS_HASH_DEFAULT)) {
		hash->type = TSS_HASH_SHA1;
		hash->hashSize = 20;
	} else if (type == TSS_HASH_OTHER) {
		hash->type = TSS_HASH_OTHER;
		hash->hashSize = 0;
	}

	if ((result = obj_list_add(&hash_list, tspContext, 0, hash, phObject))) {
		free(hash);
		return result;
	}

	return TSS_SUCCESS;
}

TSS_BOOL
obj_is_hash(TSS_HOBJECT hObject)
{
	TSS_BOOL answer = FALSE;

	if ((obj_list_get_obj(&hash_list, hObject))) {
		answer = TRUE;
		obj_list_put(&hash_list);
	}

	return answer;
}

TSS_RESULT
obj_hash_get_tsp_context(TSS_HHASH hHash, TSS_HCONTEXT *tspContext)
{
	struct tsp_object *obj;

	if ((obj = obj_list_get_obj(&hash_list, hHash)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	*tspContext = obj->tspContext;

	obj_list_put(&hash_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_hash_set_value(TSS_HHASH hHash, UINT32 size, BYTE *value)
{
	struct tsp_object *obj;
	struct tr_hash_obj *hash;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&hash_list, hHash)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	hash = (struct tr_hash_obj *)obj->data;

	if (hash->type != TSS_HASH_OTHER &&
	    size != TCPA_SHA1_160_HASH_LEN) {
		result = TSPERR(TSS_E_HASH_INVALID_LENGTH);
		goto done;
	}

	free(hash->hashData);

	if ((hash->hashData = calloc(1, size)) == NULL) {
		LogError("malloc of %d bytes failed.", size);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	hash->hashSize = size;
	memcpy(hash->hashData, value, size);

done:
	obj_list_put(&hash_list);

	return result;
}

TSS_RESULT
obj_hash_get_value(TSS_HHASH hHash, UINT32 *size, BYTE **value)
{
	struct tsp_object *obj;
	struct tr_hash_obj *hash;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&hash_list, hHash)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	hash = (struct tr_hash_obj *)obj->data;

	if (hash->hashData == NULL) {
		result = TSPERR(TSS_E_HASH_NO_DATA);
		goto done;
	}

	if ((*value = calloc_tspi(obj->tspContext, hash->hashSize)) == NULL) {
		LogError("malloc of %d bytes failed.", hash->hashSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	*size = hash->hashSize;
	memcpy(*value, hash->hashData, *size);

done:
	obj_list_put(&hash_list);

	return result;
}

TSS_RESULT
obj_hash_update_value(TSS_HHASH hHash, UINT32 size, BYTE *data)
{
	struct tsp_object *obj;
	struct tr_hash_obj *hash;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&hash_list, hHash)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	hash = (struct tr_hash_obj *)obj->data;

	if (hash->type != TSS_HASH_SHA1 &&
	    hash->type != TSS_HASH_DEFAULT) {
		result = TSPERR(TSS_E_FAIL);
		goto done;
	}

	if (hash->hashUpdateBuffer == NULL) {
		hash->hashUpdateBuffer = calloc(1, size);
		if (hash->hashUpdateBuffer == NULL) {
			LogError("malloc of %u bytes failed.", size);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
	} else {
		hash->hashUpdateBuffer = realloc(hash->hashUpdateBuffer,
				size + hash->hashUpdateSize);

		if (hash->hashUpdateBuffer == NULL) {
			LogError("malloc of %u bytes failed.", size + hash->hashUpdateSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
	}

	memcpy(&hash->hashUpdateBuffer[hash->hashUpdateSize], data, size);
	hash->hashUpdateSize += size;

	if (hash->hashData == NULL) {
		hash->hashData = calloc(1, TCPA_SHA1_160_HASH_LEN);
		if (hash->hashData == NULL) {
			LogError("malloc of %d bytes failed.", TCPA_SHA1_160_HASH_LEN);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
	}

	result = Trspi_Hash(TSS_HASH_SHA1, hash->hashUpdateSize, hash->hashUpdateBuffer,
			    hash->hashData);

done:
	obj_list_put(&hash_list);

	return result;
}

void
__tspi_hash_free(void *data)
{
	struct tr_hash_obj *hash = (struct tr_hash_obj *)data;

	free(hash->hashData);
	free(hash->hashUpdateBuffer);
	free(hash);
}

/*
 * remove hash object hObject from the list
 */
TSS_RESULT
obj_hash_remove(TSS_HOBJECT hObject, TSS_HCONTEXT tspContext)
{
	TSS_RESULT result;

	if ((result = obj_list_remove(&hash_list, &__tspi_hash_free, hObject, tspContext)))
		return result;

	return TSS_SUCCESS;
}

