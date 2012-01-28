
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
RPC_ChangeAuth_TP(struct host_table_entry *hte,
			       TCS_KEY_HANDLE parentHandle,	/* in */
			       TCPA_PROTOCOL_ID protocolID,	/* in */
			       TCPA_ENCAUTH *newAuth,	/* in */
			       TCPA_ENTITY_TYPE entityType,	/* in */
			       UINT32 encDataSize,	/* in */
			       BYTE * encData,	/* in */
			       TPM_AUTH * ownerAuth,	/* in, out */
			       TPM_AUTH * entityAuth,	/* in, out */
			       UINT32 * outDataSize,	/* out */
			       BYTE ** outData)	/* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 9);
	hte->comm.hdr.u.ordinal = TCSD_ORD_CHANGEAUTH;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &parentHandle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT16, 2, &protocolID, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_ENCAUTH, 3, newAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT16, 4, &entityType, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 5, &encDataSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 6, encData, encDataSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 7, ownerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 8, entityAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_AUTH, 1, entityAuth, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, 2, outDataSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*outData = (BYTE *) malloc(*outDataSize);
		if (*outData == NULL) {
			LogError("malloc of %u bytes failed.", *outDataSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 3, *outData, *outDataSize, &hte->comm)) {
			free(*outData);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_ChangeAuthOwner_TP(struct host_table_entry *hte,
				    TCPA_PROTOCOL_ID protocolID,	/* in */
				    TCPA_ENCAUTH *newAuth,	/* in */
				    TCPA_ENTITY_TYPE entityType,	/* in */
				    TPM_AUTH * ownerAuth	/* in, out */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 5);
	hte->comm.hdr.u.ordinal = TCSD_ORD_CHANGEAUTHOWNER;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT16, 1, &protocolID, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_ENCAUTH, 2, newAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT16, 3, &entityType, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 4, ownerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (hte->comm.hdr.u.result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

TSS_RESULT
RPC_ChangeAuthAsymStart_TP(struct host_table_entry *hte,
					TCS_KEY_HANDLE idHandle,	/* in */
					TCPA_NONCE antiReplay,	/* in */
					UINT32 KeySizeIn,	/* in */
					BYTE * KeyDataIn,	/* in */
					TPM_AUTH * pAuth,	/* in, out */
					UINT32 * KeySizeOut,	/* out */
					BYTE ** KeyDataOut,	/* out */
					UINT32 * CertifyInfoSize,	/* out */
					BYTE ** CertifyInfo,	/* out */
					UINT32 * sigSize,	/* out */
					BYTE ** sig,	/* out */
					TCS_KEY_HANDLE * ephHandle	/* out */
    ) {
	return TSPERR(TSS_E_NOTIMPL);
}

TSS_RESULT
RPC_ChangeAuthAsymFinish_TP(struct host_table_entry *hte,
					 TCS_KEY_HANDLE parentHandle,	/* in */
					 TCS_KEY_HANDLE ephHandle,	/* in */
					 TCPA_ENTITY_TYPE entityType,	/* in */
					 TCPA_HMAC newAuthLink,	/* in */
					 UINT32 newAuthSize,	/* in */
					 BYTE * encNewAuth,	/* in */
					 UINT32 encDataSizeIn,	/* in */
					 BYTE * encDataIn,	/* in */
					 TPM_AUTH * ownerAuth,	/* in, out */
					 UINT32 * encDataSizeOut,	/* out */
					 BYTE ** encDataOut,	/* out */
					 TCPA_SALT_NONCE * saltNonce,	/* out */
					 TCPA_DIGEST * changeProof	/* out */
    ) {
	return TSPERR(TSS_E_NOTIMPL);
}
