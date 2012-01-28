
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_context.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"

MUTEX_DECLARE_EXTERN(tcs_ctx_lock);

/* runs through the list of all keys loaded by context c and decrements
 * their ref count by 1, then free's their structures.
 */
void
ctx_ref_count_keys(struct tcs_context *c)
{
	struct keys_loaded *cur, *prev;

	if (c == NULL)
		return;

	cur = prev = c->keys;

	while (cur != NULL) {
		key_mgr_dec_ref_count(cur->key_handle);
		cur = cur->next;
		free(prev);
		prev = cur;
	}
}
/* Traverse loaded keys list and if matching key handle is found return TRUE else return FALSE
 */
TSS_BOOL
ctx_has_key_loaded(TCS_CONTEXT_HANDLE ctx_handle, TCS_KEY_HANDLE key_handle)
{
	struct tcs_context *c;
	struct keys_loaded *k = NULL;

	MUTEX_LOCK(tcs_ctx_lock);

	c = get_context(ctx_handle);
	if (c == NULL) {
		MUTEX_UNLOCK(tcs_ctx_lock);
		return FALSE;
	}
	k = c->keys;
	while (k != NULL) {
		if (k->key_handle == key_handle) {
			MUTEX_UNLOCK(tcs_ctx_lock);
			return TRUE;
		}
		k = k->next;
	}

	MUTEX_UNLOCK(tcs_ctx_lock);
	return FALSE;
}

/* Traverse loaded keys list and if matching key handle is found remove it */
TSS_RESULT
ctx_remove_key_loaded(TCS_CONTEXT_HANDLE ctx_handle, TCS_KEY_HANDLE key_handle)
{
	struct tcs_context *c;
	struct keys_loaded *cur, *prev;

	MUTEX_LOCK(tcs_ctx_lock);

	c = get_context(ctx_handle);
	if (c == NULL) {
		MUTEX_UNLOCK(tcs_ctx_lock);
		return TCSERR(TCS_E_INVALID_CONTEXTHANDLE);
	}

	for (prev = cur = c->keys; cur; prev = cur, cur = cur->next) {
		if (cur->key_handle == key_handle) {
			if (prev == c->keys)
				c->keys = cur->next;
			else
				prev->next = cur->next;

			free(cur);
			MUTEX_UNLOCK(tcs_ctx_lock);
			return TCS_SUCCESS;
		}
	}

	MUTEX_UNLOCK(tcs_ctx_lock);
	return TCSERR(TCS_E_INVALID_KEY);
}

/* make a new entry in the per-context list of loaded keys. If the list already
 * contains a pointer to the key in memory, just return success.
 */
TSS_RESULT
ctx_mark_key_loaded(TCS_CONTEXT_HANDLE ctx_handle, TCS_KEY_HANDLE key_handle)
{
	struct tcs_context *c;
	struct keys_loaded *k = NULL, *new;
	TSS_RESULT result;

	MUTEX_LOCK(tcs_ctx_lock);

	c = get_context(ctx_handle);

	if (c != NULL) {
		k = c->keys;
		while (k != NULL) {
			if (k->key_handle == key_handle) {
				/* we've previously created a pointer to key_handle in the global
				 * list of loaded keys and incremented that key's reference count,
				 * so there's no need to do anything.
				 */
				result = TSS_SUCCESS;
				break;
			}

			k = k->next;
		}
	} else {
		MUTEX_UNLOCK(tcs_ctx_lock);
		return TCSERR(TSS_E_FAIL);
	}

	/* if we have no record of this key being loaded by this context, create a new
	 * entry and increment the key's reference count in the global list.
	 */
	if (k == NULL) {
		new = calloc(1, sizeof(struct keys_loaded));
		if (new == NULL) {
			LogError("malloc of %zd bytes failed.", sizeof(struct keys_loaded));
			MUTEX_UNLOCK(tcs_ctx_lock);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		new->key_handle = key_handle;
		new->next = c->keys;
		c->keys = new;
		result = key_mgr_inc_ref_count(new->key_handle);
	}

	MUTEX_UNLOCK(tcs_ctx_lock);

	return result;
}

