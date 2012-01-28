
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */


#include <stdlib.h>
#include <stdio.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"


TSS_RESULT
TCS_OpenContext_Internal(TCS_CONTEXT_HANDLE * hContext)	/* out  */
{
	*hContext = make_context();
	if (*hContext == 0)
		return TCSERR(TSS_E_OUTOFMEMORY);

	return TSS_SUCCESS;
}

TSS_RESULT
TCS_CloseContext_Internal(TCS_CONTEXT_HANDLE hContext)	/* in */
{
	TSS_RESULT result;

	LogDebug("Closing context %.8X", hContext);

	if ((result = ctx_verify_context(hContext)))
		return result;

	destroy_context(hContext);

	/* close all auth handles associated with hContext */
	auth_mgr_close_context(hContext);

	KEY_MGR_ref_count();

	LogDebug("Context %.8X closed", hContext);
	return TSS_SUCCESS;
}

TSS_RESULT
TCS_FreeMemory_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			BYTE *pMemory)			/* in */
{
	free(pMemory);

	return TSS_SUCCESS;
}
