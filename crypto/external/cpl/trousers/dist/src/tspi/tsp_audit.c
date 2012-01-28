
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
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
__tspi_audit_set_ordinal_audit_status(TSS_HTPM hTpm,
				TSS_FLAG flag,
				TSS_FLAG subFlag,
				UINT32 ulOrdinal)
{
	TSS_BOOL bAuditState;
	TSS_HCONTEXT tspContext;
	TSS_HPOLICY hPolicy;
	TPM_AUTH ownerAuth;
	Trspi_HashCtx hashCtx;
	TCPA_DIGEST digest;
	TSS_RESULT result = TSS_SUCCESS;

	if (flag != TSS_TSPATTRIB_TPM_ORDINAL_AUDIT_STATUS)
		return TSPERR(TSS_E_BAD_PARAMETER);

	switch (subFlag) {
		case TPM_CAP_PROP_TPM_SET_ORDINAL_AUDIT:
			bAuditState = TRUE;
			break;

		case TPM_CAP_PROP_TPM_CLEAR_ORDINAL_AUDIT:
			bAuditState = FALSE;
			break;

		default:
			return TSPERR(TSS_E_BAD_PARAMETER);
	}

	if ((result = obj_tpm_get_tsp_context(hTpm, &tspContext)))
		return result;

	if ((result = obj_tpm_get_policy(hTpm, TSS_POLICY_USAGE, &hPolicy)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_SetOrdinalAuditStatus);
	result |= Trspi_Hash_UINT32(&hashCtx, ulOrdinal);
	result |= Trspi_Hash_BOOL(&hashCtx, bAuditState);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if ((result = secret_PerformAuth_OIAP(hTpm, TPM_ORD_SetOrdinalAuditStatus,
					      hPolicy, FALSE, &digest, &ownerAuth)))
		return result;

	if ((result = TCS_API(tspContext)->SetOrdinalAuditStatus(tspContext, &ownerAuth, ulOrdinal,
								 bAuditState)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_SetOrdinalAuditStatus);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	return obj_policy_validate_auth_oiap(hPolicy, &digest, &ownerAuth);
}

#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_SetOrdinalAuditStatus(TSS_HCONTEXT tspContext,        /* in */
				TPM_AUTH *ownerAuth,          /* in/out */
				UINT32 ulOrdinal,             /* in */
				TSS_BOOL bAuditState) /* in */
{
        TSS_RESULT result;
        UINT32 handlesLen = 0;
        UINT64 offset;
        BYTE data[sizeof(UINT32) + sizeof(TSS_BOOL)];


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, ulOrdinal, data);
	Trspi_LoadBlob_BOOL(&offset, bAuditState, data);

	result = obj_context_transport_execute(tspContext, TPM_ORD_SetOrdinalAuditStatus,
					       sizeof(data), data, NULL, &handlesLen, NULL,
					       ownerAuth, NULL, NULL, NULL);
	return result;
}

TSS_RESULT
Transport_GetAuditDigest(TSS_HCONTEXT tspContext,       /* in */
			 UINT32 startOrdinal,         /* in */
			 TPM_DIGEST *auditDigest,             /* out */
			 UINT32 *counterValueSize,            /* out */
			 BYTE **counterValue,         /* out */
			 TSS_BOOL *more,                      /* out */
			 UINT32 *ordSize,                     /* out */
			 UINT32 **ordList)                    /* out */
{
        TSS_RESULT result;
        UINT32 handlesLen = 0, decLen;
        BYTE *dec = NULL;
        UINT64 offset;
        BYTE data[sizeof(UINT32)];


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, startOrdinal, data);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_GetAuditDigest,
						    sizeof(data), data, NULL, &handlesLen, NULL,
						    NULL, NULL, &decLen, &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_COUNTER_VALUE(&offset, dec, NULL);

	*counterValueSize = (UINT32)offset;
	if ((*counterValue = malloc(*counterValueSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *counterValueSize);
		*counterValueSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	offset = 0;
	Trspi_UnloadBlob(&offset, *counterValueSize, dec, *counterValue);

	Trspi_UnloadBlob_DIGEST(&offset, dec, auditDigest);
	Trspi_UnloadBlob_BOOL(&offset, more, dec);

	Trspi_UnloadBlob_UINT32(&offset, ordSize, dec);

	if ((*ordList = malloc(*ordSize)) == NULL) {
		free(dec);
		free(*counterValue);
		*counterValue = NULL;
		*counterValueSize = 0;
		LogError("malloc of %u bytes failed", *ordSize);
		*ordSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	Trspi_UnloadBlob(&offset, *ordSize, dec, *(BYTE **)ordList);
	*ordSize /= sizeof(UINT32);

	return TSS_SUCCESS;
}

TSS_RESULT
Transport_GetAuditDigestSigned(TSS_HCONTEXT tspContext,       /* in */
			       TCS_KEY_HANDLE keyHandle,      /* in */
			       TSS_BOOL closeAudit,           /* in */
			       TPM_NONCE *antiReplay,         /* in */
			       TPM_AUTH *privAuth,            /* in/out */
			       UINT32 *counterValueSize,      /* out */
			       BYTE **counterValue,           /* out */
			       TPM_DIGEST *auditDigest,       /* out */
			       TPM_DIGEST *ordinalDigest,     /* out */
			       UINT32 *sigSize,               /* out */
			       BYTE **sig)                    /* out */
{
        TSS_RESULT result;
        UINT32 handlesLen, decLen;
        TCS_HANDLE *handles, handle;
        BYTE *dec = NULL;
        TPM_DIGEST pubKeyHash;
        Trspi_HashCtx hashCtx;
        UINT64 offset;
        BYTE data[sizeof(TSS_BOOL) + sizeof(TPM_NONCE)];


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(keyHandle, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = keyHandle;
	handles = &handle;

	offset = 0;
	Trspi_LoadBlob_BOOL(&offset, closeAudit, data);
	Trspi_LoadBlob_NONCE(&offset, data, antiReplay);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_GetAuditDigestSigned,
						    sizeof(data), data, &pubKeyHash, &handlesLen,
						    &handles, privAuth, NULL, &decLen, &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_COUNTER_VALUE(&offset, dec, NULL);

	*counterValueSize = (UINT32)offset;
	if ((*counterValue = malloc(*counterValueSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *counterValueSize);
		*counterValueSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	offset = 0;
	Trspi_UnloadBlob(&offset, *counterValueSize, dec, *counterValue);

	Trspi_UnloadBlob_DIGEST(&offset, dec, auditDigest);
	Trspi_UnloadBlob_DIGEST(&offset, dec, ordinalDigest);

	Trspi_UnloadBlob_UINT32(&offset, sigSize, dec);

	if ((*sig = malloc(*sigSize)) == NULL) {
		free(dec);
		free(*counterValue);
		*counterValue = NULL;
		*counterValueSize = 0;
		LogError("malloc of %u bytes failed", *sigSize);
		*counterValueSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *sigSize, dec, *sig);

	return TSS_SUCCESS;
}
#endif
