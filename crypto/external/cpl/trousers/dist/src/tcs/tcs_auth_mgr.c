
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "tcsd_wrap.h"
#include "tcsd.h"
#include "auth_mgr.h"
#include "req_mgr.h"


MUTEX_DECLARE_EXTERN(tcsp_lock);

/* Note: The after taking the auth_mgr_lock in any of the functions below, the
 * mem_cache_lock cannot be taken without risking a deadlock. So, the auth_mgr
 * functions must be "self-contained" wrt locking */

/* no locking done in init since its called by only a single thread */
TSS_RESULT
auth_mgr_init()
{
	memset(&auth_mgr, 0, sizeof(struct _auth_mgr));

	auth_mgr.max_auth_sessions = tpm_metrics.num_auths;

	auth_mgr.overflow = calloc(TSS_DEFAULT_OVERFLOW_AUTHS, sizeof(COND_VAR *));
	if (auth_mgr.overflow == NULL) {
		LogError("malloc of %zd bytes failed",
			 (TSS_DEFAULT_OVERFLOW_AUTHS * sizeof(COND_VAR *)));
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	auth_mgr.overflow_size = TSS_DEFAULT_OVERFLOW_AUTHS;

	auth_mgr.auth_mapper = calloc(TSS_DEFAULT_AUTH_TABLE_SIZE, sizeof(struct auth_map));
	if (auth_mgr.auth_mapper == NULL) {
		LogError("malloc of %zd bytes failed",
			 (TSS_DEFAULT_AUTH_TABLE_SIZE * sizeof(struct auth_map)));
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	auth_mgr.auth_mapper_size = TSS_DEFAULT_AUTH_TABLE_SIZE;

	return TSS_SUCCESS;
}

TSS_RESULT
auth_mgr_final()
{
	UINT32 i;

	/* wake up any sleeping threads, so they can be joined */
	for (i = 0; i < auth_mgr.overflow_size; i++) {
		if (auth_mgr.overflow[i] != NULL)
			COND_SIGNAL(auth_mgr.overflow[i]);
	}

	free(auth_mgr.overflow);
	free(auth_mgr.auth_mapper);

	return TSS_SUCCESS;
}

TSS_RESULT
auth_mgr_save_ctx(TCS_CONTEXT_HANDLE hContext)
{
       TSS_RESULT result = TSS_SUCCESS;
	UINT32 i;

	for (i = 0; i < auth_mgr.auth_mapper_size; i++) {
		if (auth_mgr.auth_mapper[i].full == TRUE &&
		    auth_mgr.auth_mapper[i].swap == NULL &&
		    auth_mgr.auth_mapper[i].tcs_ctx != hContext) {
			LogDebug("Calling TPM_SaveAuthContext for TCS CTX %x. Swapping out: TCS %x "
				 "TPM %x", hContext, auth_mgr.auth_mapper[i].tcs_ctx,
				 auth_mgr.auth_mapper[i].tpm_handle);

			if ((result = TPM_SaveAuthContext(auth_mgr.auth_mapper[i].tpm_handle,
							  &auth_mgr.auth_mapper[i].swap_size,
							  &auth_mgr.auth_mapper[i].swap))) {
				LogDebug("TPM_SaveAuthContext failed: 0x%x", result);
				return result;
			}
                       break;
		}
	}

       return result;
}

/* if there's a TCS context waiting to get auth, wake it up or swap it in */
void
auth_mgr_swap_in()
{
	if (auth_mgr.overflow[auth_mgr.of_tail] != NULL) {
		LogDebug("waking up thread %lddd, auth slot has opened", THREAD_ID);
		/* wake up the next sleeping thread in order and increment tail */
		COND_SIGNAL(auth_mgr.overflow[auth_mgr.of_tail]);
		auth_mgr.overflow[auth_mgr.of_tail] = NULL;
		auth_mgr.of_tail = (auth_mgr.of_tail + 1) % auth_mgr.overflow_size;
	} else {
		/* else nobody needs to be swapped in, so continue */
		LogDebug("no threads need to be signaled.");
	}
}

/* we need to swap out an auth context or add a waiting context to the overflow queue */
TSS_RESULT
auth_mgr_swap_out(TCS_CONTEXT_HANDLE hContext)
{
	COND_VAR *cond;

	/* If the TPM can do swapping and it succeeds, return, else cond wait below */
	if (tpm_metrics.authctx_swap && !auth_mgr_save_ctx(hContext))
		return TSS_SUCCESS;

	if ((cond = ctx_get_cond_var(hContext)) == NULL) {
		LogError("Auth swap variable not found for TCS context 0x%x", hContext);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	/* Test whether we are the last awake thread.  If we are, we can't go to sleep
	 * since then there'd be no worker thread to wake the others up. This situation
	 * can arise when we're on a busy system who's TPM doesn't support auth ctx
	 * swapping.
	 */
	if (auth_mgr.sleeping_threads == (tcsd_options.num_threads - 1)) {
		LogError("auth mgr failing: too many threads already waiting");
		LogTPMERR(TCPA_E_RESOURCES, __FILE__, __LINE__);
		return TCPA_E_RESOURCES;
	}

	if (auth_mgr.overflow[auth_mgr.of_head] == NULL) {
		auth_mgr.overflow[auth_mgr.of_head] = cond;
		auth_mgr.of_head = (auth_mgr.of_head + 1) % auth_mgr.overflow_size;
		/* go to sleep */
		LogDebug("thread %lddd going to sleep until auth slot opens", THREAD_ID);
		auth_mgr.sleeping_threads++;
		COND_WAIT(cond, &tcsp_lock);
		auth_mgr.sleeping_threads--;
	} else if (auth_mgr.overflow_size + TSS_DEFAULT_OVERFLOW_AUTHS < UINT_MAX) {
		COND_VAR **tmp = auth_mgr.overflow;

		LogDebugFn("Table of sleeping threads is full (%hu), growing to %hu entries",
			   auth_mgr.sleeping_threads,
			   auth_mgr.overflow_size + TSS_DEFAULT_OVERFLOW_AUTHS);

		auth_mgr.overflow = calloc(auth_mgr.overflow_size + TSS_DEFAULT_OVERFLOW_AUTHS,
					   sizeof(COND_VAR *));
		if (auth_mgr.overflow == NULL) {
			LogDebugFn("malloc of %zd bytes failed",
				   (auth_mgr.overflow_size + TSS_DEFAULT_OVERFLOW_AUTHS) *
				   sizeof(COND_VAR *));
			auth_mgr.overflow = tmp;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		auth_mgr.overflow_size += TSS_DEFAULT_OVERFLOW_AUTHS;

		LogDebugFn("Success.");
		memcpy(auth_mgr.overflow, tmp, auth_mgr.sleeping_threads * sizeof(COND_VAR *));
		free(tmp);

		/* XXX This could temporarily wake threads up out of order */
		auth_mgr.of_head = auth_mgr.sleeping_threads;
		auth_mgr.of_tail = 0;
		auth_mgr.overflow[auth_mgr.of_head] = cond;
		auth_mgr.of_head = (auth_mgr.of_head + 1) % auth_mgr.overflow_size;
		LogDebug("thread %lddd going to sleep until auth slot opens", THREAD_ID);
		auth_mgr.sleeping_threads++;
		COND_WAIT(cond, &tcsp_lock);
		auth_mgr.sleeping_threads--;
	} else {
		LogError("Auth table overflow is full (%u entries), some will fail.",
			 auth_mgr.overflow_size);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	return TSS_SUCCESS;
}

/* close all auth contexts associated with this TCS_CONTEXT_HANDLE */
TSS_RESULT
auth_mgr_close_context(TCS_CONTEXT_HANDLE tcs_handle)
{
	UINT32 i;
	TSS_RESULT result;

	for (i = 0; i < auth_mgr.auth_mapper_size; i++) {
		if (auth_mgr.auth_mapper[i].full == TRUE &&
		    auth_mgr.auth_mapper[i].tcs_ctx == tcs_handle) {
			if (auth_mgr.auth_mapper[i].swap) {
				/* This context is swapped out of the TPM, so we can just free the
				 * blob */
				free(auth_mgr.auth_mapper[i].swap);
				auth_mgr.auth_mapper[i].swap = NULL;
				auth_mgr.auth_mapper[i].swap_size = 0;
			} else {
				result = TCSP_FlushSpecific_Common(auth_mgr.auth_mapper[i].tpm_handle,
								   TPM_RT_AUTH);

				/* Ok, probably dealing with a 1.1 TPM */
				if (result == TPM_E_BAD_ORDINAL)
                                       result = internal_TerminateHandle(
                                           auth_mgr.auth_mapper[i].tpm_handle);

				if (result == TCPA_E_INVALID_AUTHHANDLE) {
					LogDebug("Tried to close an invalid auth handle: %x",
						 auth_mgr.auth_mapper[i].tpm_handle);
				} else if (result != TCPA_SUCCESS) {
					LogDebug("TPM_TerminateHandle returned %d", result);
				}
			}
                       /* clear the slot */
			auth_mgr.open_auth_sessions--;
			auth_mgr.auth_mapper[i].full = FALSE;
                       auth_mgr.auth_mapper[i].tpm_handle = 0;
                       auth_mgr.auth_mapper[i].tcs_ctx = 0;
			LogDebug("released auth for TCS %x TPM %x", tcs_handle,
                               auth_mgr.auth_mapper[i].tpm_handle);

			auth_mgr_swap_in();
		}
	}

	return TSS_SUCCESS;
}

void
auth_mgr_release_auth(TPM_AUTH *auth0, TPM_AUTH *auth1, TCS_CONTEXT_HANDLE tcs_handle)
{
	if (auth0)
		auth_mgr_release_auth_handle(auth0->AuthHandle, tcs_handle,
					     auth0->fContinueAuthSession);

	if (auth1)
		auth_mgr_release_auth_handle(auth1->AuthHandle, tcs_handle,
					     auth1->fContinueAuthSession);
}

/* unload the auth ctx associated with this auth handle */
TSS_RESULT
auth_mgr_release_auth_handle(TCS_AUTHHANDLE tpm_auth_handle, TCS_CONTEXT_HANDLE tcs_handle,
			     TSS_BOOL cont)
{
	UINT32 i;
	TSS_RESULT result = TSS_SUCCESS;

	for (i = 0; i < auth_mgr.auth_mapper_size; i++) {
		if (auth_mgr.auth_mapper[i].full == TRUE &&
		    auth_mgr.auth_mapper[i].tpm_handle == tpm_auth_handle &&
		    auth_mgr.auth_mapper[i].tcs_ctx == tcs_handle) {
			if (!cont) {
                               /*
                                * This function should not be necessary, but
                                * if the main operation resulted in an error,
                                * the TPM may still hold the auth handle
                                * and it must be freed.  Most of the time
                                * this call will result in TPM_E_INVALID_AUTHHANDLE
                                * error which can be ignored.
                                */
                               result = TCSP_FlushSpecific_Common(
                                   auth_mgr.auth_mapper[i].tpm_handle,
                                   TPM_RT_AUTH);

				/* Ok, probably dealing with a 1.1 TPM */
				if (result == TPM_E_BAD_ORDINAL)
                                       result = internal_TerminateHandle(
                                           auth_mgr.auth_mapper[i].tpm_handle);

				if (result == TCPA_E_INVALID_AUTHHANDLE) {
					LogDebug("Tried to close an invalid auth handle: %x",
						 auth_mgr.auth_mapper[i].tpm_handle);
				} else if (result != TCPA_SUCCESS) {
					LogDebug("TPM_TerminateHandle returned %d", result);
				}

                               if (result == TPM_SUCCESS) {
                                       LogDebug("released auth for TCS %x TPM %x",
                                                auth_mgr.auth_mapper[i].tcs_ctx, tpm_auth_handle);
                               }
                               /*
                                * Mark it as released, the "cont" flag indicates
                                * that it is no longer needed.
                                */
                               auth_mgr.open_auth_sessions--;
                               auth_mgr.auth_mapper[i].full = FALSE;
                               auth_mgr.auth_mapper[i].tpm_handle = 0;
                               auth_mgr.auth_mapper[i].tcs_ctx = 0;
                               auth_mgr_swap_in();
			}
                       /* If the cont flag is TRUE, we have to keep the handle */
		}
	}

	return result;
}

TSS_RESULT
auth_mgr_check(TCS_CONTEXT_HANDLE tcsContext, TPM_AUTHHANDLE *tpm_auth_handle)
{
	UINT32 i;
	TSS_RESULT result = TSS_SUCCESS;

	for (i = 0; i < auth_mgr.auth_mapper_size; i++) {
		if (auth_mgr.auth_mapper[i].full == TRUE &&
		    auth_mgr.auth_mapper[i].tpm_handle == *tpm_auth_handle &&
		    auth_mgr.auth_mapper[i].tcs_ctx == tcsContext) {
			/* We have a record of this session, now swap it into the TPM if need be. */
			if (auth_mgr.auth_mapper[i].swap) {
				LogDebugFn("TPM_LoadAuthContext for TCS %x TPM %x", tcsContext,
					   auth_mgr.auth_mapper[i].tpm_handle);

				result = TPM_LoadAuthContext(auth_mgr.auth_mapper[i].swap_size,
							     auth_mgr.auth_mapper[i].swap,
							     tpm_auth_handle);
				if (result == TSS_SUCCESS) {
					free(auth_mgr.auth_mapper[i].swap);
					auth_mgr.auth_mapper[i].swap = NULL;
					auth_mgr.auth_mapper[i].swap_size = 0;

					LogDebugFn("TPM_LoadAuthContext succeeded. Old TPM: %x, New"
						   " TPM: %x", auth_mgr.auth_mapper[i].tpm_handle,
						   *tpm_auth_handle);

					auth_mgr.auth_mapper[i].tpm_handle = *tpm_auth_handle;
				} else if (result == TPM_E_RESOURCES) {
					if ((result = auth_mgr_swap_out(tcsContext))) {
						LogDebugFn("TPM_LoadAuthContext failed with TPM_E_R"
							   "ESOURCES and swapping out failed, retur"
							   "ning error");
						return result;
					}

					LogDebugFn("Retrying TPM_LoadAuthContext after swap"
						   " out...");
					result =
					      TPM_LoadAuthContext(auth_mgr.auth_mapper[i].swap_size,
								  auth_mgr.auth_mapper[i].swap,
								  tpm_auth_handle);
					free(auth_mgr.auth_mapper[i].swap);
					auth_mgr.auth_mapper[i].swap = NULL;
					auth_mgr.auth_mapper[i].swap_size = 0;
				} else {
					LogDebug("TPM_LoadAuthContext failed: 0x%x.", result);
				}
			}

			return result;
		}
	}

	LogDebugFn("Can't find auth for TCS handle %x, should be %x", tcsContext, *tpm_auth_handle);
	return TCSERR(TSS_E_INTERNAL_ERROR);
}

TSS_RESULT
auth_mgr_add(TCS_CONTEXT_HANDLE tcsContext, TCS_AUTHHANDLE tpm_auth_handle)
{
	struct auth_map *tmp = auth_mgr.auth_mapper;
	UINT32 i;

	for (i = 0; i < auth_mgr.auth_mapper_size; i++) {
		if (auth_mgr.auth_mapper[i].full == FALSE) {
			auth_mgr.auth_mapper[i].tpm_handle = tpm_auth_handle;
			auth_mgr.auth_mapper[i].tcs_ctx = tcsContext;
			auth_mgr.auth_mapper[i].full = TRUE;
			auth_mgr.open_auth_sessions++;
			LogDebug("added auth for TCS %x TPM %x", tcsContext, tpm_auth_handle);

			return TSS_SUCCESS;
		}
	}


	LogDebugFn("Thread %ld growing the auth table to %u entries", THREAD_ID,
		   auth_mgr.auth_mapper_size + TSS_DEFAULT_AUTH_TABLE_SIZE);

	auth_mgr.auth_mapper = calloc(auth_mgr.auth_mapper_size +
				      TSS_DEFAULT_AUTH_TABLE_SIZE, sizeof(struct auth_map));
	if (auth_mgr.auth_mapper == NULL) {
		auth_mgr.auth_mapper = tmp;
		return TCSERR(TSS_E_OUTOFMEMORY);
	} else {
		memcpy(auth_mgr.auth_mapper, tmp,
				auth_mgr.auth_mapper_size * sizeof(struct auth_map));
		auth_mgr.auth_mapper_size += TSS_DEFAULT_AUTH_TABLE_SIZE;
		LogDebugFn("Success.");
		return TSS_SUCCESS;
	}
}

/* A thread wants a new OIAP or OSAP session with the TPM. Returning TRUE indicates that we should
 * allow it to open the session, FALSE to indicate that the request should be queued or have
 * another thread's session swapped out to make room for it.
 */
TSS_BOOL
auth_mgr_req_new(TCS_CONTEXT_HANDLE hContext)
{
	UINT32 i, opened = 0;

	for (i = 0; i < auth_mgr.auth_mapper_size; i++) {
		if (auth_mgr.auth_mapper[i].full == TRUE &&
		    auth_mgr.auth_mapper[i].tcs_ctx  == hContext) {
			opened++;
		}
	}

	/* If this TSP has already opened its max open auth handles, deny another open */
	if (opened >= MAX(2, (UINT32)auth_mgr.max_auth_sessions/2)) {
		LogDebug("Max opened auth handles already opened.");
		return FALSE;
	}

	/* if we have one opened already and there's a slot available, ok */
	if (opened && ((auth_mgr.max_auth_sessions - auth_mgr.open_auth_sessions) >= 1))
		return TRUE;

	/* we don't already have one open and there are at least 2 slots left */
	if ((auth_mgr.max_auth_sessions - auth_mgr.open_auth_sessions) >= 2)
		return TRUE;

	LogDebug("Request for new auth handle denied by TCS. (%d opened sessions)", opened);

	return FALSE;
}

TSS_RESULT
auth_mgr_oiap(TCS_CONTEXT_HANDLE hContext,	/* in */
	      TCS_AUTHHANDLE *authHandle,	/* out */
	      TCPA_NONCE *nonce0)		/* out */
{
	TSS_RESULT result;

	/* are the maximum number of auth sessions open? */
	if (auth_mgr_req_new(hContext) == FALSE) {
		if ((result = auth_mgr_swap_out(hContext)))
			goto done;
	}

	if ((result = TCSP_OIAP_Internal(hContext, authHandle, nonce0)))
		goto done;

	/* success, add an entry to the table */
	result = auth_mgr_add(hContext, *authHandle);
done:
	return result;
}

TSS_RESULT
auth_mgr_osap(TCS_CONTEXT_HANDLE hContext,	/* in */
	      TCPA_ENTITY_TYPE entityType,	/* in */
	      UINT32 entityValue,		/* in */
	      TCPA_NONCE nonceOddOSAP,		/* in */
	      TCS_AUTHHANDLE *authHandle,	/* out */
	      TCPA_NONCE *nonceEven,		/* out */
	      TCPA_NONCE *nonceEvenOSAP)	/* out */
{
	TSS_RESULT result;
	UINT32 newEntValue = 0;

	/* if ET is not KEYHANDLE or KEY, newEntValue is a don't care */
	if (entityType == TCPA_ET_KEYHANDLE || entityType == TCPA_ET_KEY) {
		if (ensureKeyIsLoaded(hContext, entityValue, &newEntValue))
			return TCSERR(TSS_E_FAIL);
	} else {
		newEntValue = entityValue;
	}

	/* are the maximum number of auth sessions open? */
	if (auth_mgr_req_new(hContext) == FALSE) {
		if ((result = auth_mgr_swap_out(hContext)))
			goto done;
	}

	if ((result = TCSP_OSAP_Internal(hContext, entityType, newEntValue, nonceOddOSAP,
					 authHandle, nonceEven, nonceEvenOSAP)))
		goto done;

	/* success, add an entry to the table */
	result = auth_mgr_add(hContext, *authHandle);
done:
	return result;
}

/* This function is only called directly from the TSP, we use other internal routines to free
 * our handles. */
TSS_RESULT
TCSP_TerminateHandle_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			      TCS_AUTHHANDLE handle)	/* in */
{
	TSS_RESULT result;

	LogDebug("Entering TCSI_TerminateHandle");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = auth_mgr_check(hContext, &handle)))
		return result;

	result = auth_mgr_release_auth_handle(handle, hContext, TRUE);

	LogResult("Terminate Handle", result);
	return result;
}

TSS_RESULT
TPM_SaveAuthContext(TPM_AUTHHANDLE handle, UINT32 *size, BYTE **blob)
{
	UINT64 offset;
	UINT32 trash, bsize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	offset = 10;
	LoadBlob_UINT32(&offset, handle, txBlob);
	LoadBlob_Header(TPM_TAG_RQU_COMMAND, offset, TPM_ORD_SaveAuthContext, txBlob);

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &trash);

	LogDebug("total packet size received from TPM: %u", trash);

	if (!result) {
		offset = 10;
		UnloadBlob_UINT32(&offset, &bsize, txBlob);

		LogDebug("Reported blob size from TPM: %u", bsize);

		*blob = malloc(bsize);
		if (*blob == NULL) {
			LogError("malloc of %u bytes failed.", bsize);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset, bsize, txBlob, *blob);
		*size = bsize;
	}

	return result;
}

TSS_RESULT
TPM_LoadAuthContext(UINT32 size, BYTE *blob, TPM_AUTHHANDLE *handle)
{
	UINT64 offset;
	UINT32 trash;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Loading %u byte auth blob back into TPM", size);

	offset = 10;
	LoadBlob_UINT32(&offset, size, txBlob);
	LoadBlob(&offset, size, txBlob, blob);
	LoadBlob_Header(TPM_TAG_RQU_COMMAND, offset, TPM_ORD_LoadAuthContext, txBlob);

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &trash);

	if (!result) {
		offset = 10;
		UnloadBlob_UINT32(&offset, handle, txBlob);
	}

	return result;
}
