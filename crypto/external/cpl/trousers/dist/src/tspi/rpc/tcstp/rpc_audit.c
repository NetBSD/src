
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
#include <assert.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "tsplog.h"
#include "hosttable.h"
#include "tcsd_wrap.h"
#include "rpc_tcstp_tsp.h"


TSS_RESULT
RPC_SetOrdinalAuditStatus_TP(struct host_table_entry *hte,
			     TPM_AUTH *ownerAuth,	/* in/out */
			     UINT32 ulOrdinal,		/* in */
			     TSS_BOOL bAuditState)	/* in */
{
	TSS_RESULT result;

	initData(&hte->comm, 4);
	hte->comm.hdr.u.ordinal = TCSD_ORD_SETORDINALAUDITSTATUS;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &ulOrdinal, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_BOOL, 2, &bAuditState, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 3, ownerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

TSS_RESULT
RPC_GetAuditDigest_TP(struct host_table_entry *hte,
		      UINT32 startOrdinal,		/* in */
		      TPM_DIGEST *auditDigest,		/* out */
		      UINT32 *counterValueSize,	/* out */
		      BYTE **counterValue,		/* out */
		      TSS_BOOL *more,			/* out */
		      UINT32 *ordSize,			/* out */
		      UINT32 **ordList)		/* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_GETAUDITDIGEST;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &startOrdinal, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_DIGEST, 0, auditDigest, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, 1, counterValueSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*counterValue = (BYTE *)malloc(*counterValueSize);
		if (*counterValue == NULL) {
			LogError("malloc of %u bytes failed.", *counterValueSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 2, *counterValue, *counterValueSize, &hte->comm)) {
			free(*counterValue);
			*counterValue = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_BOOL, 3, more, 0, &hte->comm)) {
			free(*counterValue);
			*counterValue = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, 4, ordSize, 0, &hte->comm)) {
			free(*counterValue);
			*counterValue = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*ordList = (UINT32 *)malloc(*ordSize * sizeof(UINT32));
		if (*ordList == NULL) {
			LogError("malloc of %u bytes failed.", *ordSize);
			free(*counterValue);
			*counterValue = NULL;
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 5, *ordList, *ordSize * sizeof(UINT32), &hte->comm)) {
			free(*counterValue);
			*counterValue = NULL;
			free(*ordList);
			*ordList = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_GetAuditDigestSigned_TP(struct host_table_entry *hte,
			    TCS_KEY_HANDLE keyHandle,		/* in */
			    TSS_BOOL closeAudit,		/* in */
			    TPM_NONCE *antiReplay,		/* in */
			    TPM_AUTH *privAuth,		/* in/out */
			    UINT32 *counterValueSize,		/* out */
			    BYTE **counterValue,		/* out */
			    TPM_DIGEST *auditDigest,		/* out */
			    TPM_DIGEST *ordinalDigest,		/* out */
			    UINT32 *sigSize,			/* out */
			    BYTE **sig)			/* out */
{
	TSS_RESULT result;
	TPM_AUTH null_auth;
	int i;

	initData(&hte->comm, 5);
	hte->comm.hdr.u.ordinal = TCSD_ORD_GETAUDITDIGESTSIGNED;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	memset(&null_auth, 0, sizeof(TPM_AUTH));

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &keyHandle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_BOOL, 2, &closeAudit, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_NONCE, 3, antiReplay, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (privAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, privAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	else {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, &null_auth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (privAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, privAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, counterValueSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*counterValue = (BYTE *)malloc(*counterValueSize);
		if (*counterValue == NULL) {
			LogError("malloc of %u bytes failed.", *counterValueSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *counterValue, *counterValueSize, &hte->comm)) {
			free(*counterValue);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_DIGEST, i++, auditDigest, 0, &hte->comm)) {
			free(*counterValue);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_DIGEST, i++, ordinalDigest, 0, &hte->comm)) {
			free(*counterValue);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, sigSize, 0, &hte->comm)) {
			free(*counterValue);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*sig = (BYTE *)malloc(*sigSize);
		if (*sig == NULL) {
			LogError("malloc of %u bytes failed.", *sigSize);
			free(*counterValue);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *sig, *sigSize, &hte->comm)) {
			free(*counterValue);
			free(*sig);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}

done:
	return result;
}
