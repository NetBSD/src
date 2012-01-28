
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
RPC_GetRegisteredKeyByPublicInfo_TP(struct host_table_entry *hte,
				     TCPA_ALGORITHM_ID algID,	/* in */
				     UINT32 ulPublicInfoLength,	/* in */
				     BYTE * rgbPublicInfo,	/* in */
				     UINT32 * keySize,		/* out */
				     BYTE ** keyBlob)		/* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 4);
	hte->comm.hdr.u.ordinal = TCSD_ORD_GETREGISTEREDKEYBYPUBLICINFO;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &algID, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &ulPublicInfoLength, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, rgbPublicInfo, ulPublicInfoLength, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, keySize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*keyBlob = (BYTE *) malloc(*keySize);
		if (*keyBlob == NULL) {
			LogError("malloc of %u bytes failed.", *keySize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 1, *keyBlob, *keySize, &hte->comm)) {
			free(*keyBlob);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_RegisterKey_TP(struct host_table_entry *hte,
			       TSS_UUID WrappingKeyUUID,	/* in */
			       TSS_UUID KeyUUID,	/* in */
			       UINT32 cKeySize,	/* in */
			       BYTE * rgbKey,	/* in */
			       UINT32 cVendorData,	/* in */
			       BYTE * gbVendorData	/* in */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 7);
	hte->comm.hdr.u.ordinal = TCSD_ORD_REGISTERKEY;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UUID, 1, &WrappingKeyUUID, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UUID, 2, &KeyUUID, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, &cKeySize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 4, rgbKey, cKeySize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 5, &cVendorData, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 6, gbVendorData, cVendorData, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_UnregisterKey_TP(struct host_table_entry *hte,
				  TSS_UUID KeyUUID	/* in */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_UNREGISTERKEY;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UUID, 1, &KeyUUID, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_EnumRegisteredKeys_TP(struct host_table_entry *hte,
				      TSS_UUID * pKeyUUID,	/* in */
				      UINT32 * pcKeyHierarchySize,	/* out */
				      TSS_KM_KEYINFO ** ppKeyHierarchy	/* out */
    ) {
	TSS_RESULT result;
	int i, j;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_ENUMREGISTEREDKEYS;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (pKeyUUID != NULL) {
		if (setData(TCSD_PACKET_TYPE_UUID, 1, pKeyUUID, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pcKeyHierarchySize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		if (*pcKeyHierarchySize > 0) {
			*ppKeyHierarchy = malloc((*pcKeyHierarchySize) * sizeof(TSS_KM_KEYINFO));
			if (*ppKeyHierarchy == NULL) {
				LogError("malloc of %zu bytes failed.", (*pcKeyHierarchySize) *
					 sizeof(TSS_KM_KEYINFO));
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
			for (j = 0; (UINT32)j < *pcKeyHierarchySize; j++) {
				if (getData(TCSD_PACKET_TYPE_KM_KEYINFO, i++,
					    &((*ppKeyHierarchy)[j]), 0, &hte->comm)) {
					free(*ppKeyHierarchy);
					result = TSPERR(TSS_E_INTERNAL_ERROR);
					goto done;
				}
			}
		} else {
			*ppKeyHierarchy = NULL;
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_EnumRegisteredKeys2_TP(struct host_table_entry *hte,
				      TSS_UUID * pKeyUUID,	/* in */
				      UINT32 * pcKeyHierarchySize,	/* out */
				      TSS_KM_KEYINFO2 ** ppKeyHierarchy	/* out */
					  )
{
	TSS_RESULT result;
	int i, j;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_ENUMREGISTEREDKEYS2;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (pKeyUUID != NULL) {
		if (setData(TCSD_PACKET_TYPE_UUID, 1, pKeyUUID, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pcKeyHierarchySize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		if (*pcKeyHierarchySize > 0) {
			*ppKeyHierarchy = malloc((*pcKeyHierarchySize) * sizeof(TSS_KM_KEYINFO2));
			if (*ppKeyHierarchy == NULL) {
				LogError("malloc of %zu bytes failed.", (*pcKeyHierarchySize) *
					 sizeof(TSS_KM_KEYINFO2));
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
			for (j = 0; (UINT32)j < *pcKeyHierarchySize; j++) {
				if (getData(TCSD_PACKET_TYPE_KM_KEYINFO2, i++,
					    &((*ppKeyHierarchy)[j]), 0, &hte->comm)) {
					free(*ppKeyHierarchy);
					result = TSPERR(TSS_E_INTERNAL_ERROR);
					goto done;
				}
			}
		} else {
			*ppKeyHierarchy = NULL;
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_GetRegisteredKey_TP(struct host_table_entry *hte,
				    TSS_UUID KeyUUID,	/* in */
				    TSS_KM_KEYINFO ** ppKeyInfo	/* out */
    ) {
	return TSPERR(TSS_E_NOTIMPL);
}

TSS_RESULT
RPC_GetRegisteredKeyBlob_TP(struct host_table_entry *hte,
					TSS_UUID KeyUUID,	/* in */
					UINT32 * pcKeySize,	/* out */
					BYTE ** prgbKey	/* out */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_GETREGISTEREDKEYBLOB;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UUID, 1, &KeyUUID, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, pcKeySize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*prgbKey = malloc(*pcKeySize);
		if (*prgbKey == NULL) {
			LogError("malloc of %u bytes failed.", *pcKeySize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 1, *prgbKey, *pcKeySize, &hte->comm)) {
			free(*prgbKey);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

done:
	return result;

}

TSS_RESULT
RPC_LoadKeyByUUID_TP(struct host_table_entry *hte,
				  TSS_UUID KeyUUID,	/* in */
				  TCS_LOADKEY_INFO * pLoadKeyInfo,	/* in, out */
				  TCS_KEY_HANDLE * phKeyTCSI	/* out */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 3);
	hte->comm.hdr.u.ordinal = TCSD_ORD_LOADKEYBYUUID;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UUID, 1, &KeyUUID, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (pLoadKeyInfo != NULL) {
		if (setData(TCSD_PACKET_TYPE_LOADKEY_INFO, 2, pLoadKeyInfo, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, phKeyTCSI, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);

		LogDebugFn("TCS key handle: 0x%x", *phKeyTCSI);
	} else if (pLoadKeyInfo && (result == (TCS_E_KM_LOADFAILED | TSS_LAYER_TCS))) {
		if (getData(TCSD_PACKET_TYPE_LOADKEY_INFO, 0, pLoadKeyInfo, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

void
LoadBlob_LOADKEY_INFO(UINT64 *offset, BYTE *blob, TCS_LOADKEY_INFO *info)
{
	Trspi_LoadBlob_UUID(offset, blob, info->keyUUID);
	Trspi_LoadBlob_UUID(offset, blob, info->parentKeyUUID);
	Trspi_LoadBlob(offset, TCPA_DIGEST_SIZE, blob, info->paramDigest.digest);
	Trspi_LoadBlob_UINT32(offset, info->authData.AuthHandle, blob);
	Trspi_LoadBlob(offset, TCPA_NONCE_SIZE, blob, (BYTE *)&info->authData.NonceOdd.nonce);
	Trspi_LoadBlob(offset, TCPA_NONCE_SIZE, blob, (BYTE *)&info->authData.NonceEven.nonce);
	Trspi_LoadBlob_BOOL(offset, info->authData.fContinueAuthSession, blob);
	Trspi_LoadBlob(offset, TCPA_DIGEST_SIZE, blob, (BYTE *)&info->authData.HMAC);
}

void
UnloadBlob_LOADKEY_INFO(UINT64 *offset, BYTE *blob, TCS_LOADKEY_INFO *info)
{
	Trspi_UnloadBlob_UUID(offset, blob, &info->keyUUID);
	Trspi_UnloadBlob_UUID(offset, blob, &info->parentKeyUUID);
	Trspi_UnloadBlob(offset, TCPA_DIGEST_SIZE, blob, (BYTE *)&info->paramDigest.digest);
	Trspi_UnloadBlob_UINT32(offset, &info->authData.AuthHandle, blob);
	Trspi_UnloadBlob(offset, TCPA_NONCE_SIZE, blob, (BYTE *)&info->authData.NonceOdd.nonce);
	Trspi_UnloadBlob(offset, TCPA_NONCE_SIZE, blob, (BYTE *)&info->authData.NonceEven.nonce);
	Trspi_UnloadBlob_BOOL(offset, &info->authData.fContinueAuthSession, blob);
	Trspi_UnloadBlob(offset, TCPA_DIGEST_SIZE, blob, (BYTE *)&info->authData.HMAC);
}

