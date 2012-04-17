
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
RPC_LoadKeyByBlob_TP(struct host_table_entry *hte,
		      TCS_KEY_HANDLE hUnwrappingKey,	/* in */
		      UINT32 cWrappedKeyBlobSize,	/* in */
		      BYTE * rgbWrappedKeyBlob,	/* in */
		      TPM_AUTH * pAuth,	/* in, out */
		      TCS_KEY_HANDLE * phKeyTCSI,	/* out */
		      TCS_KEY_HANDLE * phKeyHMAC)	/* out */
{
	TSS_RESULT result;
	int i;

	initData(&hte->comm, 5);
	hte->comm.hdr.u.ordinal = TCSD_ORD_LOADKEYBYBLOB;
	LogDebugFn("IN: TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hUnwrappingKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &cWrappedKeyBlobSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, rgbWrappedKeyBlob, cWrappedKeyBlobSize, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);

	if (pAuth != NULL) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, pAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (pAuth != NULL) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &hte->comm))
				result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, phKeyTCSI, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, phKeyHMAC, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);

		LogDebugFn("OUT: TCS key handle: 0x%x, TPM key slot: 0x%x", *phKeyTCSI,
			   *phKeyHMAC);
	}

	return result;
}

TSS_RESULT
RPC_EvictKey_TP(struct host_table_entry *hte,
		 TCS_KEY_HANDLE hKey)	/* in */
{
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_EVICTKEY;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_CreateWrapKey_TP(struct host_table_entry *hte,
		      TCS_KEY_HANDLE hWrappingKey,	/* in */
		      TCPA_ENCAUTH *KeyUsageAuth,	/* in */
		      TCPA_ENCAUTH *KeyMigrationAuth,	/* in */
		      UINT32 keyInfoSize,	/* in */
		      BYTE * keyInfo,	/* in */
		      UINT32 * keyDataSize,	/* out */
		      BYTE ** keyData,	/* out */
		      TPM_AUTH * pAuth)	/* in, out */
{
	TSS_RESULT result;

	initData(&hte->comm, 7);
	hte->comm.hdr.u.ordinal = TCSD_ORD_CREATEWRAPKEY;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hWrappingKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_ENCAUTH, 2, KeyUsageAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_ENCAUTH, 3, KeyMigrationAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 4, &keyInfoSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 5, keyInfo, keyInfoSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (pAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 6, pAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, keyDataSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*keyData = (BYTE *) malloc(*keyDataSize);
		if (*keyData == NULL) {
			LogError("malloc of %u bytes failed.", *keyDataSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 1, *keyData, *keyDataSize, &hte->comm)) {
			free(*keyData);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (pAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, 2, pAuth, 0, &hte->comm)) {
				free(*keyData);
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_GetPubKey_TP(struct host_table_entry *hte,
		  TCS_KEY_HANDLE hKey,	/* in */
		  TPM_AUTH * pAuth,	/* in, out */
		  UINT32 * pcPubKeySize,	/* out */
		  BYTE ** prgbPubKey)	/* out */
{
	TSS_RESULT result;
	int i;

	initData(&hte->comm, 3);
	hte->comm.hdr.u.ordinal = TCSD_ORD_GETPUBKEY;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (pAuth != NULL) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 2, pAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	i = 0;
	if (result == TSS_SUCCESS) {
		if (pAuth != NULL) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pcPubKeySize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*prgbPubKey = (BYTE *) malloc(*pcPubKeySize);
		if (*prgbPubKey == NULL) {
			LogError("malloc of %u bytes failed.", *pcPubKeySize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *prgbPubKey, *pcPubKeySize, &hte->comm)) {
			free(*prgbPubKey);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_TerminateHandle_TP(struct host_table_entry *hte,
			TCS_AUTHHANDLE handle)	/* in */
{
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_TERMINATEHANDLE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &handle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_OwnerReadInternalPub_TP(struct host_table_entry *hte,
			     TCS_KEY_HANDLE hKey,	/* in */
			     TPM_AUTH * pOwnerAuth,	/* in, out */
			     UINT32 * punPubKeySize,	/* out */
			     BYTE ** ppbPubKeyData)	/* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 3);
	hte->comm.hdr.u.ordinal = TCSD_ORD_OWNERREADINTERNALPUB;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (pOwnerAuth != NULL) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 2, pOwnerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, pOwnerAuth, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_UINT32, 1, punPubKeySize, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);

		*ppbPubKeyData = (BYTE *) malloc(*punPubKeySize);
		if (*ppbPubKeyData == NULL) {
			LogError("malloc of %u bytes failed.", *punPubKeySize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 2, *ppbPubKeyData, *punPubKeySize, 
			    &hte->comm)) {
			free(*ppbPubKeyData);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	return result;
}

/* TSS 1.2-only interfaces */
#ifdef TSS_BUILD_TSS12
TSS_RESULT
RPC_KeyControlOwner_TP(struct host_table_entry *hte,		// in
			TCS_KEY_HANDLE     hKey,		// in
			UINT32             ulPublicInfoLength,	// in
			BYTE*              rgbPublicInfo,	// in
			UINT32             attribName,		// in
			TSS_BOOL           attribValue,		// in
			TPM_AUTH*          pOwnerAuth,		// in, out
			TSS_UUID*          pUuidData)		// out

{
	TSS_RESULT result;

	initData(&hte->comm, 7);
	hte->comm.hdr.u.ordinal = TCSD_ORD_KEYCONTROLOWNER;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &ulPublicInfoLength, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, rgbPublicInfo, ulPublicInfoLength, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 4, &attribName, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_BOOL, 5, &attribValue, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (pOwnerAuth != NULL) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 6, pOwnerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, pOwnerAuth, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_UUID, 1, pUuidData, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}
#endif
