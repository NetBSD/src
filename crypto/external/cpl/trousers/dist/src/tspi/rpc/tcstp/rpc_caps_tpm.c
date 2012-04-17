
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
RPC_GetTPMCapability_TP(struct host_table_entry *hte,
		      TCPA_CAPABILITY_AREA capArea,	/* in */
		      UINT32 subCapSize,		/* in */
		      BYTE * subCap,			/* in */
		      UINT32 * respSize,		/* out */
		      BYTE ** resp)			/* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 4);
	hte->comm.hdr.u.ordinal = TCSD_ORD_GETCAPABILITY;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &capArea, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &subCapSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, subCap, subCapSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, respSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*resp = (BYTE *) malloc(*respSize);
		if (*resp == NULL) {
			LogError("malloc of %u bytes failed.", *respSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 1, *resp, *respSize, &hte->comm)) {
			free(*resp);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_GetCapabilitySigned_TP(struct host_table_entry *hte,
					TCS_KEY_HANDLE keyHandle,	/* in */
					TCPA_NONCE antiReplay,	/* in */
					TCPA_CAPABILITY_AREA capArea,	/* in */
					UINT32 subCapSize,	/* in */
					BYTE * subCap,	/* in */
					TPM_AUTH * privAuth,	/* in, out */
					TCPA_VERSION * Version,	/* out */
					UINT32 * respSize,	/* out */
					BYTE ** resp,	/* out */
					UINT32 * sigSize,	/* out */
					BYTE ** sig)	/* out */
{
	return TSPERR(TSS_E_NOTIMPL);
}

TSS_RESULT
RPC_GetCapabilityOwner_TP(struct host_table_entry *hte,
				       TPM_AUTH * pOwnerAuth,	/* out */
				       TCPA_VERSION * pVersion,	/* out */
				       UINT32 * pNonVolatileFlags,	/* out */
				       UINT32 * pVolatileFlags)	/* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_GETCAPABILITYOWNER;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 1, pOwnerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_VERSION, 0, pVersion, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_UINT32, 1, pNonVolatileFlags, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_UINT32, 2, pVolatileFlags, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_AUTH, 3, pOwnerAuth, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

TSS_RESULT
RPC_SetCapability_TP(struct host_table_entry *hte,
		      TCPA_CAPABILITY_AREA capArea,	/* in */
		      UINT32 subCapSize,	/* in */
		      BYTE * subCap,	/* in */
		      UINT32 valueSize,	/* in */
		      BYTE * value,	/* in */
		      TPM_AUTH * pOwnerAuth)	/* in, out */
{
	TSS_RESULT result;

	initData(&hte->comm, 7);
	hte->comm.hdr.u.ordinal = TCSD_ORD_SETCAPABILITY;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &capArea, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &subCapSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, subCap, subCapSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 4, &valueSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 5, value, valueSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (pOwnerAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 6, pOwnerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, pOwnerAuth, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}
