
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
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
#include "tcsd_wrap.h"
#include "tcsd.h"


unsigned long nextContextHandle = 0xA0000000;
struct tcs_context *tcs_context_table = NULL;

MUTEX_DECLARE_INIT(tcs_ctx_lock);

TCS_CONTEXT_HANDLE getNextHandle();
struct tcs_context *create_tcs_context();
struct tcs_context *get_context(TCS_CONTEXT_HANDLE);
struct tcs_context *get_previous_context(TCS_CONTEXT_HANDLE);

TSS_BOOL initContextHandle = 1;

TCS_CONTEXT_HANDLE
getNextHandle()
{
	UINT32 tempRand;
	time_t currentTime;

	if (initContextHandle) {
		currentTime = time(NULL);
		srand(currentTime);
		tempRand = rand();
		tempRand = tempRand << 16;
		tempRand &= 0x00FF0000;
		nextContextHandle |= tempRand;
		initContextHandle = 0;
	}
	currentTime = time(NULL);
	srand(currentTime + 1);
	tempRand = rand();
	tempRand = tempRand << 8;
	tempRand &= 0x0000FF00;
	if (nextContextHandle == 0)
		return getNextHandle();
	else
		return ((nextContextHandle++) | tempRand);
}

struct tcs_context *
create_tcs_context()
{
	struct tcs_context *ret = (struct tcs_context *)calloc(1, sizeof(struct tcs_context));

	if (ret != NULL) {
		ret->handle = getNextHandle();
		COND_INIT(ret->cond);
	}
	return ret;
}

struct tcs_context *
get_context(TCS_CONTEXT_HANDLE handle)
{
	struct tcs_context *index;
	index = tcs_context_table;
	while (index) {
		if (index->handle == handle)
			break;
		index = index->next;
	}

	return index;
}

struct tcs_context *
get_previous_context(TCS_CONTEXT_HANDLE handle)
{
	struct tcs_context *index;
	index = tcs_context_table;
	while (index) {
		if (index->next) {
			if (index->next->handle == handle)
				return index;
		}
		index = index->next;
	}

	return 0;
}

void
destroy_context(TCS_CONTEXT_HANDLE handle)
{
	struct tcs_context *toKill;
	struct tcs_context *previous;

	MUTEX_LOCK(tcs_ctx_lock);

	toKill = get_context(handle);
	previous = get_previous_context(handle);

	if (!previous && tcs_context_table->handle == handle) {
		/* this means that toKill is the first one */
		tcs_context_table = tcs_context_table->next;
	} else if (previous && toKill) {
		/* both are found */
		previous->next = toKill->next;
	} else {
		MUTEX_UNLOCK(tcs_ctx_lock);
		return;
	}

	MUTEX_UNLOCK(tcs_ctx_lock);

	CTX_ref_count_keys(toKill);

#ifdef TSS_BUILD_TRANSPORT
	/* Free existing transport session if necessary */
	if (toKill->transHandle)
		TCSP_FlushSpecific_Common(toKill->transHandle, TPM_RT_TRANS);
#endif

	free(toKill);
}

TCS_CONTEXT_HANDLE
make_context()
{
	struct tcs_context *index;

	MUTEX_LOCK(tcs_ctx_lock);

	index = tcs_context_table;

	if (!index) {
		tcs_context_table = create_tcs_context();
		if (tcs_context_table == NULL) {
			LogError("Malloc Failure.");
			MUTEX_UNLOCK(tcs_ctx_lock);
			return 0;
		}
		index = tcs_context_table;
	} else {
		while (index->next) {
			index = index->next;
		}
		index->next = create_tcs_context();
		if (index->next == NULL) {
			LogError("Malloc Failure.");
			MUTEX_UNLOCK(tcs_ctx_lock);
			return 0;
		}
		index = index->next;
	}

	MUTEX_UNLOCK(tcs_ctx_lock);

	return index->handle;
}


TSS_RESULT
ctx_verify_context(TCS_CONTEXT_HANDLE tcsContext)
{
	struct tcs_context *c;

	if (tcsContext == InternalContext) {
		LogDebug("Success: %x is an Internal Context", tcsContext);
		return TSS_SUCCESS;
	}

	MUTEX_LOCK(tcs_ctx_lock);

	c = get_context(tcsContext);

	MUTEX_UNLOCK(tcs_ctx_lock);

	if (c == NULL) {
		LogDebug("Fail: Context %x not found", tcsContext);
		return TCSERR(TCS_E_INVALID_CONTEXTHANDLE);
	}

	return TSS_SUCCESS;
}


COND_VAR *
ctx_get_cond_var(TCS_CONTEXT_HANDLE tcs_handle)
{
	struct tcs_context *c;
	COND_VAR *ret = NULL;

	MUTEX_LOCK(tcs_ctx_lock);

	c = get_context(tcs_handle);

	if (c != NULL)
		ret = &c->cond;

	MUTEX_UNLOCK(tcs_ctx_lock);

	return ret;
}

/* the only transport flag at the TCS level is whether the session is exclusive or not. If the app
 * is requesting an exclusive transport session, check that no other exclusive sessions exist and
 * if not, flag this context as being the one. If so, return internal error. */
TSS_RESULT
ctx_req_exclusive_transport(TCS_CONTEXT_HANDLE tcsContext)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tcs_context *tmp, *self = NULL;

	/* If the daemon is configured to ignore apps that want an exclusive transport, just
	 * return */
	if (!tcsd_options.exclusive_transport)
		return result;

	MUTEX_LOCK(tcs_ctx_lock);

	tmp = tcs_context_table;
	while (tmp) {
		if (tmp->flags & TSS_CONTEXT_FLAG_TRANSPORT_EXCLUSIVE) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		if (tmp->handle == tcsContext)
			self = tmp;

		tmp = tmp->next;
	}

	if (self)
		self->flags |= TSS_CONTEXT_FLAG_TRANSPORT_EXCLUSIVE;
	else
		result = TCSERR(TCS_E_INVALID_CONTEXTHANDLE);
done:
	MUTEX_UNLOCK(tcs_ctx_lock);

	return result;
}

TSS_RESULT
ctx_set_transport_enabled(TCS_CONTEXT_HANDLE tcsContext, UINT32 hTransHandle)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tcs_context *tmp, *self = NULL;

	MUTEX_LOCK(tcs_ctx_lock);

	tmp = tcs_context_table;
	while (tmp) {
		if (tmp->handle == tcsContext) {
			self = tmp;
			break;
		}

		tmp = tmp->next;
	}

	if (self) {
		self->flags |= TSS_CONTEXT_FLAG_TRANSPORT_ENABLED;
		self->transHandle = hTransHandle;
	} else
		result = TCSERR(TCS_E_INVALID_CONTEXTHANDLE);

	MUTEX_UNLOCK(tcs_ctx_lock);

	return result;
}

TSS_RESULT
ctx_set_transport_disabled(TCS_CONTEXT_HANDLE tcsContext, TCS_HANDLE *transHandle)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tcs_context *tmp, *self = NULL;

	MUTEX_LOCK(tcs_ctx_lock);

	tmp = tcs_context_table;
	while (tmp) {
		if (tmp->handle == tcsContext) {
			self = tmp;
			break;
		}

		tmp = tmp->next;
	}

	if (self) {
		if (!transHandle || *transHandle == self->transHandle) {
			self->transHandle = 0;
			self->flags &= ~TSS_CONTEXT_FLAG_TRANSPORT_ENABLED;
		}
	} else
		result = TCSERR(TCS_E_INVALID_CONTEXTHANDLE);

	MUTEX_UNLOCK(tcs_ctx_lock);

	return result;
}
