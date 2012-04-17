
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
#include <inttypes.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
Tspi_TPM_DirWrite(TSS_HTPM hTPM,		/* in */
		  UINT32 ulDirIndex,		/* in */
		  UINT32 ulDirDataLength,	/* in */
		  BYTE * rgbDirData)		/* in */
{
	TSS_HCONTEXT tspContext;
	TCPA_RESULT result;
	TPM_AUTH auth;
	TCPA_DIGEST hashDigest;
	TSS_HPOLICY hPolicy;
	TCPA_DIRVALUE dirValue = { { 0 } };
	Trspi_HashCtx hashCtx;

	if (rgbDirData == NULL && ulDirDataLength != 0)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (ulDirDataLength > (UINT32)sizeof(TCPA_DIRVALUE))
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hPolicy)))
		return result;

	memcpy((BYTE *)&dirValue, rgbDirData, ulDirDataLength);

	/* hash to be used for the OIAP calc */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_DirWriteAuth);
	result |= Trspi_Hash_UINT32(&hashCtx, ulDirIndex);
	result |= Trspi_HashUpdate(&hashCtx, (UINT32)sizeof(TCPA_DIRVALUE), (BYTE *)&dirValue);
	if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
		return result;

	/* hashDigest now has the hash result */
	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_DirWriteAuth, hPolicy, FALSE,
					      &hashDigest, &auth)))
		return result;

	if ((result = TCS_API(tspContext)->DirWriteAuth(tspContext, ulDirIndex, &dirValue, &auth)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_DirWriteAuth);
	if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
		return result;

	return obj_policy_validate_auth_oiap(hPolicy, &hashDigest, &auth);
}

TSS_RESULT
Tspi_TPM_DirRead(TSS_HTPM hTPM,			/* in */
		 UINT32 ulDirIndex,		/* in */
		 UINT32 * pulDirDataLength,	/* out */
		 BYTE ** prgbDirData)		/* out */
{
	TCPA_DIRVALUE dirValue;
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;

	if (pulDirDataLength == NULL || prgbDirData == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = TCS_API(tspContext)->DirRead(tspContext, ulDirIndex, &dirValue)))
		return result;

	*pulDirDataLength = 20;
	*prgbDirData = calloc_tspi(tspContext, *pulDirDataLength);
	if (*prgbDirData == NULL) {
		LogError("malloc of %d bytes failed.", *pulDirDataLength);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	memcpy(*prgbDirData, dirValue.digest, *pulDirDataLength);
	return TSS_SUCCESS;
}
