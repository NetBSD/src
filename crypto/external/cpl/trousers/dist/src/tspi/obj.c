
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
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
#include "tsplog.h"
#include "obj.h"

UINT32 nextObjectHandle = 0xC0000000;

MUTEX_DECLARE_INIT(handle_lock);

TPM_LIST_DECLARE;
CONTEXT_LIST_DECLARE;
HASH_LIST_DECLARE;
PCRS_LIST_DECLARE;
POLICY_LIST_DECLARE;
RSAKEY_LIST_DECLARE;
ENCDATA_LIST_DECLARE;
DAACRED_LIST_DECLARE;
DAAARAKEY_LIST_DECLARE;
DAAISSUERKEY_LIST_DECLARE;
NVSTORE_LIST_DECLARE;
DELFAMILY_LIST_DECLARE;
MIGDATA_LIST_DECLARE;

void
list_init(struct obj_list *list)
{
	list->head = NULL;
	MUTEX_INIT(list->lock);
}

void
__tspi_obj_list_init()
{
	TPM_LIST_INIT();
	CONTEXT_LIST_INIT();
	HASH_LIST_INIT();
	PCRS_LIST_INIT();
	POLICY_LIST_INIT();
	RSAKEY_LIST_INIT();
	ENCDATA_LIST_INIT();
	DAACRED_LIST_INIT();
	DAAARAKEY_LIST_INIT();
	DAAISSUERKEY_LIST_INIT();
	NVSTORE_LIST_INIT();
	DELFAMILY_LIST_INIT();
	MIGDATA_LIST_INIT();
}

TSS_HOBJECT
obj_get_next_handle()
{
	MUTEX_LOCK(handle_lock);

	/* return any object handle except NULL_HOBJECT */
	do {
		nextObjectHandle++;
	} while (nextObjectHandle == NULL_HOBJECT);

	MUTEX_UNLOCK(handle_lock);

	return nextObjectHandle;
}

/* search through the provided list for an object with handle matching
 * @handle. If found, return a pointer to the object with the list
 * locked, else return NULL.  To release the lock, caller should
 * call obj_list_put() after manipulating the object.
 */
struct tsp_object *
obj_list_get_obj(struct obj_list *list, UINT32 handle)
{
	struct tsp_object *obj;

	MUTEX_LOCK(list->lock);

	for (obj = list->head; obj; obj = obj->next) {
		if (obj->handle == handle)
			break;
	}

	if (obj == NULL)
		MUTEX_UNLOCK(list->lock);

	return obj;
}

/* search through the provided list for an object with TSP context
 * matching @tspContext. If found, return a pointer to the object
 * with the list locked, else return NULL.  To release the lock,
 * caller should call obj_list_put() after manipulating the object.
 */
struct tsp_object *
obj_list_get_tspcontext(struct obj_list *list, UINT32 tspContext)
{
	struct tsp_object *obj;

	MUTEX_LOCK(list->lock);

	for (obj = list->head; obj; obj = obj->next) {
		if (obj->tspContext == tspContext)
			break;
	}

	return obj;
}

/* release a list whose handle was returned by obj_list_get_obj() */
void
obj_list_put(struct obj_list *list)
{
	MUTEX_UNLOCK(list->lock);
}

TSS_RESULT
obj_list_add(struct obj_list *list, UINT32 tsp_context, TSS_FLAG flags, void *data,
	     TSS_HOBJECT *phObject)
{
        struct tsp_object *new_obj, *tmp;

        new_obj = calloc(1, sizeof(struct tsp_object));
        if (new_obj == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct tsp_object));
                return TSPERR(TSS_E_OUTOFMEMORY);
        }

        new_obj->handle = obj_get_next_handle();
	new_obj->flags = flags;
        new_obj->data = data;

	if (list == &context_list)
		new_obj->tspContext = new_obj->handle;
	else
		new_obj->tspContext = tsp_context;

        MUTEX_LOCK(list->lock);

        if (list->head == NULL) {
                list->head = new_obj;
        } else {
                tmp = list->head;
                list->head = new_obj;
                new_obj->next = tmp;
        }

        MUTEX_UNLOCK(list->lock);

        *phObject = new_obj->handle;

        return TSS_SUCCESS;
}

TSS_RESULT
obj_list_remove(struct obj_list *list, void (*freeFcn)(void *), TSS_HOBJECT hObject, TSS_HCONTEXT tspContext)
{
	struct tsp_object *obj, *prev = NULL;

	MUTEX_LOCK(list->lock);

	for (obj = list->head; obj; prev = obj, obj = obj->next) {
		if (obj->handle == hObject) {
			/* validate tspContext */
			if (obj->tspContext != tspContext)
				break;

			(*freeFcn)(obj->data);

			if (prev)
				prev->next = obj->next;
			else
				list->head = obj->next;
			free(obj);

			MUTEX_UNLOCK(list->lock);
			return TSS_SUCCESS;
		}
	}

	MUTEX_UNLOCK(list->lock);

	return TSPERR(TSS_E_INVALID_HANDLE);
}

/* a generic routine for removing all members of a list who's tsp context
 * matches @tspContext */
void
obj_list_close(struct obj_list *list, void (*freeFcn)(void *), TSS_HCONTEXT tspContext)
{
	struct tsp_object *index;
	struct tsp_object *next = NULL;
	struct tsp_object *toKill;
	struct tsp_object *prev = NULL;

	MUTEX_LOCK(list->lock);

	for (index = list->head; index; ) {
		next = index->next;
		if (index->tspContext == tspContext) {
			toKill = index;
			if (prev == NULL) {
				list->head = toKill->next;
			} else {
				prev->next = toKill->next;
			}

			(*freeFcn)(toKill->data);
			free(toKill);

			index = next;
		} else {
			prev = index;
			index = next;
		}
	}

	MUTEX_UNLOCK(list->lock);
}

void
obj_close_context(TSS_HCONTEXT tspContext)
{
	TPM_LIST_CLOSE(tspContext);
	CONTEXT_LIST_CLOSE(tspContext);
	HASH_LIST_CLOSE(tspContext);
	PCRS_LIST_CLOSE(tspContext);
	POLICY_LIST_CLOSE(tspContext);
	RSAKEY_LIST_CLOSE(tspContext);
	ENCDATA_LIST_CLOSE(tspContext);
	DAACRED_LIST_CLOSE(tspContext);
	DAAARAKEY_LIST_CLOSE(tspContext);
	DAAISSUERKEY_LIST_CLOSE(tspContext);
	NVSTORE_LIST_CLOSE(tspContext);
	DELFAMILY_LIST_CLOSE(tspContext);
	MIGDATA_LIST_CLOSE(tspContext);
}

/* When a policy object is closed, all references to it must be removed. This function
 * calls the object specific routines for each working object type to remove all refs to the
 * policy */
void
obj_lists_remove_policy_refs(TSS_HPOLICY hPolicy, TSS_HCONTEXT tspContext)
{
	obj_rsakey_remove_policy_refs(hPolicy, tspContext);
	obj_encdata_remove_policy_refs(hPolicy, tspContext);
	obj_tpm_remove_policy_refs(hPolicy, tspContext);
}

/* search all key lists (right now only RSA keys exist) looking for a TCS key handle, when
 * found, return the hash of its TPM_STORE_PUBKEY structure */
TSS_RESULT
obj_tcskey_get_pubkeyhash(TCS_KEY_HANDLE hKey, BYTE *pubKeyHash)
{
	struct tsp_object *obj;
	struct obj_list *list = &rsakey_list;
	struct tr_rsakey_obj *rsakey = NULL;
	TSS_RESULT result = TSS_SUCCESS;
	Trspi_HashCtx hashCtx;

	MUTEX_LOCK(list->lock);

	for (obj = list->head; obj; obj = obj->next) {
		rsakey = (struct tr_rsakey_obj *)obj->data;
		if (rsakey->tcsHandle == hKey)
			break;
	}

	if (obj == NULL || rsakey == NULL) {
		MUTEX_UNLOCK(list->lock);
		return TSPERR(TSS_E_KEY_NOT_LOADED);
	}

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_STORE_PUBKEY(&hashCtx, &rsakey->key.pubKey);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash)))
		result = TSPERR(TSS_E_INTERNAL_ERROR);

	MUTEX_UNLOCK(list->lock);

	return result;
}
