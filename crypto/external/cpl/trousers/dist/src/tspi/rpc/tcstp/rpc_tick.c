
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
#include <string.h>
#include <assert.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "hosttable.h"
#include "tcsd_wrap.h"
#include "obj.h"
#include "rpc_tcstp_tsp.h"


TSS_RESULT
RPC_ReadCurrentTicks_TP(struct host_table_entry* hte,
			 UINT32*                  pulCurrentTime,	/* out */
			 BYTE**                   prgbCurrentTime)	/* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 1);
	hte->comm.hdr.u.ordinal = TCSD_ORD_READCURRENTTICKS;
	LogDebugFn("IN: TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, pulCurrentTime, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*prgbCurrentTime = malloc(*pulCurrentTime);
		if (*prgbCurrentTime == NULL) {
			*pulCurrentTime = 0;
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}

		if (getData(TCSD_PACKET_TYPE_PBYTE, 1, *prgbCurrentTime, *pulCurrentTime,
			    &hte->comm)) {
			free(*prgbCurrentTime);
			*prgbCurrentTime = NULL;
			*pulCurrentTime = 0;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}
done:
	return result;
}

TSS_RESULT
RPC_TickStampBlob_TP(struct host_table_entry* hte,
		      TCS_KEY_HANDLE           hKey,			/* in */
		      TPM_NONCE*               antiReplay,		/* in */
		      TPM_DIGEST*              digestToStamp,		/* in */
		      TPM_AUTH*                privAuth,		/* in, out */
		      UINT32*                  pulSignatureLength,	/* out */
		      BYTE**                   prgbSignature,		/* out */
		      UINT32*                  pulTickCountLength,	/* out */
		      BYTE**                   prgbTickCount)		/* out */

{
	TSS_RESULT result;
	UINT32 i;

	initData(&hte->comm, 5);
	hte->comm.hdr.u.ordinal = TCSD_ORD_TICKSTAMPBLOB;
	LogDebugFn("IN: TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_NONCE, 2, antiReplay, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_DIGEST, 3, digestToStamp, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (privAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, privAuth, 0, &hte->comm))
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

		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pulSignatureLength, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*prgbSignature = malloc(*pulSignatureLength);
		if (*prgbSignature == NULL) {
			*pulSignatureLength = 0;
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}

		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *prgbSignature, *pulSignatureLength,
			    &hte->comm)) {
			free(*prgbSignature);
			*prgbSignature = NULL;
			*pulSignatureLength = 0;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pulTickCountLength, 0, &hte->comm)) {
			free(*prgbSignature);
			*prgbSignature = NULL;
			*pulSignatureLength = 0;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*prgbTickCount = malloc(*pulTickCountLength);
		if (*prgbTickCount == NULL) {
			free(*prgbSignature);
			*prgbSignature = NULL;
			*pulSignatureLength = 0;
			*pulTickCountLength = 0;
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}

		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *prgbTickCount, *pulTickCountLength,
			    &hte->comm)) {
			free(*prgbSignature);
			*prgbSignature = NULL;
			*pulSignatureLength = 0;
			free(*prgbTickCount);
			*prgbTickCount = NULL;
			*pulTickCountLength = 0;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}
done:
	return result;
}
