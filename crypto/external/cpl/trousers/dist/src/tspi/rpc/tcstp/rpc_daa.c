
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
RPC_DaaJoin_TP(struct host_table_entry *hte,
		TPM_HANDLE handle, // in
		BYTE stage, // in
		UINT32 inputSize0, // in
		BYTE* inputData0, // in
		UINT32 inputSize1, // in
		BYTE* inputData1, // in
		TPM_AUTH* ownerAuth, // in/out
		UINT32* outputSize, // out
		BYTE** outputData) // out
{
	TSS_RESULT result;
	UINT32 i;

	LogDebugFn("stage=%d", stage);
	initData(&hte->comm, 8);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DAAJOIN;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);
	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &handle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_BYTE, 2, &stage, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, &inputSize0, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	LogDebugFn("inputSize0=<network>=%d <host>=%d", inputSize0, inputSize0);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 4, inputData0, inputSize0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 5, &inputSize1, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	LogDebugFn("inputSize1=<network>=%d <host>=%d", inputSize1, inputSize1);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 6, inputData1, inputSize1, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if( ownerAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 7, ownerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);
	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		LogDebugFn("getData outputSize");

		if( ownerAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, ownerAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, outputSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*outputData = (BYTE *) malloc(*outputSize);
		if (*outputData == NULL) {
			LogError("malloc of %u bytes failed.", *outputSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		LogDebugFn("getData outputData (outputSize=%u)", *outputSize);
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *outputData, *outputSize, &hte->comm)) {
			free(*outputData);
			*outputData = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}

done:
	LogDebugFn("result=%u", result);
	return result;
}

TSS_RESULT
RPC_DaaSign_TP(struct host_table_entry *hte,
		TPM_HANDLE handle, // in
		BYTE stage, // in
		UINT32 inputSize0, // in
		BYTE* inputData0, // in
		UINT32 inputSize1, // in
		BYTE* inputData1, // in
		TPM_AUTH* ownerAuth, // in/out
		UINT32* outputSize, // out
		BYTE** outputData) // out
{
	TSS_RESULT result;
	UINT32 i;

	LogDebugFn("stage=%d", stage);
	initData(&hte->comm, 8);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DAASIGN;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);
	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &handle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_BYTE, 2, &stage, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, &inputSize0, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	LogDebugFn("inputSize0=<network>=%d <host>=%d", inputSize0, inputSize0);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 4, inputData0, inputSize0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 5, &inputSize1, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	LogDebugFn("inputSize1=<network>=%d <host>=%d", inputSize1, inputSize1);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 6, inputData1, inputSize1, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if( ownerAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 7, ownerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	LogDebugFn("sendTCSDPacket: 0x%x", (int)hte);
	result = sendTCSDPacket(hte);
	//
	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;
	//
	if (result == TSS_SUCCESS) {
		i = 0;
		LogDebugFn("getData outputSize");

		if( ownerAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, ownerAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, outputSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*outputData = (BYTE *) malloc(*outputSize);
		if (*outputData == NULL) {
			LogError("malloc of %u bytes failed.", *outputSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		LogDebugFn("getData outputData (outputSize=%d)", *outputSize);
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *outputData, *outputSize, &hte->comm)) {
			free(*outputData);
			*outputData = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}

done:
	LogDebugFn("result=%u", result);
	return result;
}

