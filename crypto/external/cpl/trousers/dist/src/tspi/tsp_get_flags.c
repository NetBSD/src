
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
get_tpm_flags(TSS_HCONTEXT tspContext, TSS_HTPM hTPM, UINT32 *volFlags, UINT32 *nonVolFlags)
{
	TCPA_DIGEST digest;
	TPM_AUTH auth;
	TCPA_VERSION version;
	TSS_RESULT result;
	TSS_HPOLICY hPolicy;
	Trspi_HashCtx hashCtx;

	if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hPolicy)))
		return result;

	/* do an owner authorized get capability call */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_GetCapabilityOwner);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_GetCapabilityOwner, hPolicy, FALSE,
					      &digest, &auth)))
		return result;

	if ((result = TCS_API(tspContext)->GetCapabilityOwner(tspContext, &auth, &version,
							      nonVolFlags, volFlags)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_GetCapabilityOwner);
	result |= Trspi_Hash_VERSION(&hashCtx, (TSS_VERSION *)&version);
	result |= Trspi_Hash_UINT32(&hashCtx, *nonVolFlags);
	result |= Trspi_Hash_UINT32(&hashCtx, *volFlags);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	return obj_policy_validate_auth_oiap(hPolicy, &digest, &auth);
}
