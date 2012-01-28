
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
common_Seal_TP(UINT32 sealOrdinal,
			 struct host_table_entry *hte,
			 TCS_KEY_HANDLE keyHandle,	/* in */
			 TCPA_ENCAUTH *encAuth,	/* in */
			 UINT32 pcrInfoSize,	/* in */
			 BYTE * PcrInfo,	/* in */
			 UINT32 inDataSize,	/* in */
			 BYTE * inData,	/* in */
			 TPM_AUTH * pubAuth,	/* in, out */
			 UINT32 * SealedDataSize,	/* out */
			 BYTE ** SealedData	/* out */
    ) {
	TSS_RESULT result;
	int i = 0;

	initData(&hte->comm, 8);
	hte->comm.hdr.u.ordinal = sealOrdinal;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, i++, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, i++, &keyHandle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_ENCAUTH, i++, encAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, i++, &pcrInfoSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (pcrInfoSize > 0) {
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, PcrInfo, pcrInfoSize, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	if (setData(TCSD_PACKET_TYPE_UINT32, i++, &inDataSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (inDataSize > 0) {
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, inData, inDataSize, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if (setData(TCSD_PACKET_TYPE_AUTH, i, pubAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (hte->comm.hdr.u.result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, pubAuth, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		if (getData(TCSD_PACKET_TYPE_UINT32, 1, SealedDataSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*SealedData = (BYTE *) malloc(*SealedDataSize);
		if (*SealedData == NULL) {
			LogError("malloc of %u bytes failed.", *SealedDataSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 2, *SealedData, *SealedDataSize, &hte->comm)) {
			free(*SealedData);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_Seal_TP(struct host_table_entry *hte,
			 TCS_KEY_HANDLE keyHandle,	/* in */
			 TCPA_ENCAUTH *encAuth,	/* in */
			 UINT32 pcrInfoSize,	/* in */
			 BYTE * PcrInfo,	/* in */
			 UINT32 inDataSize,	/* in */
			 BYTE * inData,	/* in */
			 TPM_AUTH * pubAuth,	/* in, out */
			 UINT32 * SealedDataSize,	/* out */
			 BYTE ** SealedData	/* out */
    ) {
	return common_Seal_TP(TCSD_ORD_SEAL, hte, keyHandle, encAuth, pcrInfoSize, PcrInfo,
			      inDataSize, inData, pubAuth, SealedDataSize, SealedData);
}

#ifdef TSS_BUILD_SEALX
TSS_RESULT
RPC_Sealx_TP(struct host_table_entry *hte,
			 TCS_KEY_HANDLE keyHandle,	/* in */
			 TCPA_ENCAUTH *encAuth,	/* in */
			 UINT32 pcrInfoSize,	/* in */
			 BYTE * PcrInfo,	/* in */
			 UINT32 inDataSize,	/* in */
			 BYTE * inData,	/* in */
			 TPM_AUTH * pubAuth,	/* in, out */
			 UINT32 * SealedDataSize,	/* out */
			 BYTE ** SealedData	/* out */
    ) {
	return common_Seal_TP(TCSD_ORD_SEALX, hte, keyHandle, encAuth, pcrInfoSize, PcrInfo,
			      inDataSize, inData, pubAuth, SealedDataSize, SealedData);
}
#endif

TSS_RESULT
RPC_Unseal_TP(struct host_table_entry *hte,
			   TCS_KEY_HANDLE parentHandle,	/* in */
			   UINT32 SealedDataSize,	/* in */
			   BYTE * SealedData,	/* in */
			   TPM_AUTH * parentAuth,	/* in, out */
			   TPM_AUTH * dataAuth,	/* in, out */
			   UINT32 * DataSize,	/* out */
			   BYTE ** Data	/* out */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 6);
	hte->comm.hdr.u.ordinal = TCSD_ORD_UNSEAL;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &parentHandle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &SealedDataSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, SealedData, SealedDataSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (parentAuth != NULL) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, parentAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if (setData(TCSD_PACKET_TYPE_AUTH, 5, dataAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (parentAuth != NULL) {
			if (getData(TCSD_PACKET_TYPE_AUTH, 0, parentAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}

		if (getData(TCSD_PACKET_TYPE_AUTH, 1, dataAuth, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		if (getData(TCSD_PACKET_TYPE_UINT32, 2, DataSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*Data = (BYTE *) malloc(*DataSize);
		if (*Data == NULL) {
			LogError("malloc of %u bytes failed.", *DataSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 3, *Data, *DataSize, &hte->comm)) {
			free(*Data);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

done:
	return result;
}
