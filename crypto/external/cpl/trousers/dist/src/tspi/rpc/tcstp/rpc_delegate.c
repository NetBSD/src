
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
RPC_Delegate_Manage_TP(struct host_table_entry *hte,
		       TPM_FAMILY_ID familyID,		/* in */
		       TPM_FAMILY_OPERATION opFlag,	/* in */
		       UINT32 opDataSize,		/* in */
		       BYTE *opData,			/* in */
		       TPM_AUTH *ownerAuth,		/* in/out */
		       UINT32 *retDataSize,		/* out */
		       BYTE **retData)			/* out */
{
	TSS_RESULT result;
	int i;

	initData(&hte->comm, 6);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DELEGATE_MANAGE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &familyID, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &opFlag, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, &opDataSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 4, opData, opDataSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (ownerAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 5, ownerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		TPM_AUTH nullAuth;

		memset(&nullAuth, 0, sizeof(TPM_AUTH));
		if (setData(TCSD_PACKET_TYPE_AUTH, 5, &nullAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (ownerAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, ownerAuth, 0, &hte->comm))
				return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, retDataSize, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		*retData = (BYTE *)malloc(*retDataSize);
		if (*retData == NULL) {
			LogError("malloc of %u bytes failed.", *retDataSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *retData, *retDataSize, &hte->comm)) {
			free(*retData);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	return result;
}

TSS_RESULT
RPC_Delegate_CreateKeyDelegation_TP(struct host_table_entry *hte,
				    TCS_KEY_HANDLE hKey,		/* in */
				    UINT32 publicInfoSize,		/* in */
				    BYTE *publicInfo,			/* in */
				    TPM_ENCAUTH *encDelAuth,		/* in */
				    TPM_AUTH *keyAuth,			/* in/out */
				    UINT32 *blobSize,			/* out */
				    BYTE **blob)			/* out */
{
	TSS_RESULT result;
	int i;

	initData(&hte->comm, 8);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DELEGATE_CREATEKEYDELEGATION;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &publicInfoSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, publicInfo, publicInfoSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_ENCAUTH, 4, encDelAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (keyAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 5, keyAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		TPM_AUTH nullAuth;

		memset(&nullAuth, 0, sizeof(TPM_AUTH));
		if (setData(TCSD_PACKET_TYPE_AUTH, 5, &nullAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (keyAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, keyAuth, 0, &hte->comm))
				return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, blobSize, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		*blob = (BYTE *)malloc(*blobSize);
		if (*blob == NULL) {
			LogError("malloc of %u bytes failed.", *blobSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *blob, *blobSize, &hte->comm)) {
			free(*blob);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	return result;
}

TSS_RESULT
RPC_Delegate_CreateOwnerDelegation_TP(struct host_table_entry *hte,
				      TSS_BOOL increment,		/* in */
				      UINT32 publicInfoSize,		/* in */
				      BYTE *publicInfo,		/* in */
				      TPM_ENCAUTH *encDelAuth,		/* in */
				      TPM_AUTH *ownerAuth,		/* in/out */
				      UINT32 *blobSize,		/* out */
				      BYTE **blob)			/* out */
{
	TSS_RESULT result;
	int i;

	initData(&hte->comm, 8);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DELEGATE_CREATEOWNERDELEGATION;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_BOOL, 1, &increment, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &publicInfoSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, publicInfo, publicInfoSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_ENCAUTH, 4, encDelAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (ownerAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 5, ownerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		TPM_AUTH nullAuth;

		memset(&nullAuth, 0, sizeof(TPM_AUTH));
		if (setData(TCSD_PACKET_TYPE_AUTH, 5, &nullAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (ownerAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, ownerAuth, 0, &hte->comm))
				return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, blobSize, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		*blob = (BYTE *)malloc(*blobSize);
		if (*blob == NULL) {
			LogError("malloc of %u bytes failed.", *blobSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *blob, *blobSize, &hte->comm)) {
			free(*blob);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	return result;
}

TSS_RESULT
RPC_Delegate_LoadOwnerDelegation_TP(struct host_table_entry *hte,
				    TPM_DELEGATE_INDEX index,		/* in */
				    UINT32 blobSize,			/* in */
				    BYTE *blob,			/* in */
				    TPM_AUTH *ownerAuth)		/* in/out */
{
	TSS_RESULT result;

	initData(&hte->comm, 5);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DELEGATE_LOADOWNERDELEGATION;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &index, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &blobSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, blob, blobSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (ownerAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, ownerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		TPM_AUTH nullAuth;

		memset(&nullAuth, 0, sizeof(TPM_AUTH));
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, &nullAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (ownerAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm))
				return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	return result;
}

TSS_RESULT
RPC_Delegate_ReadTable_TP(struct host_table_entry *hte,
			  UINT32 *familyTableSize,	/* out */
			  BYTE **familyTable,		/* out */
			  UINT32 *delegateTableSize,	/* out */
			  BYTE **delegateTable)	/* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 1);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DELEGATE_READTABLE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, familyTableSize, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		*familyTable = (BYTE *)malloc(*familyTableSize);
		if (*familyTable == NULL) {
			LogError("malloc of %u bytes failed.", *familyTableSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 1, *familyTable, *familyTableSize, &hte->comm)) {
			free(*familyTable);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}

		if (getData(TCSD_PACKET_TYPE_UINT32, 2, delegateTableSize, 0, &hte->comm)) {
			free(*familyTable);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		*delegateTable = (BYTE *)malloc(*delegateTableSize);
		if (*delegateTable == NULL) {
			free(*familyTable);
			LogError("malloc of %u bytes failed.", *delegateTableSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 3, *delegateTable, *delegateTableSize, &hte->comm)) {
			free(*familyTable);
			free(*delegateTable);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	return result;
}

TSS_RESULT
RPC_Delegate_UpdateVerificationCount_TP(struct host_table_entry *hte,
					UINT32 inputSize,		/* in */
					BYTE *input,			/* in */
					TPM_AUTH *ownerAuth,		/* in/out */
					UINT32 *outputSize,		/* out */
					BYTE **output)			/* out */
{
	TSS_RESULT result;
	int i;

	initData(&hte->comm, 4);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DELEGATE_UPDATEVERIFICATIONCOUNT;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &inputSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 2, input, inputSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (ownerAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 3, ownerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		TPM_AUTH nullAuth;

		memset(&nullAuth, 0, sizeof(TPM_AUTH));
		if (setData(TCSD_PACKET_TYPE_AUTH, 3, &nullAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (ownerAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, ownerAuth, 0, &hte->comm))
				return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, outputSize, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		*output = (BYTE *)malloc(*outputSize);
		if (*output == NULL) {
			LogError("malloc of %u bytes failed.", *outputSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *output, *outputSize, &hte->comm)) {
			free(*output);
			output = NULL;
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	return result;
}

TSS_RESULT
RPC_Delegate_VerifyDelegation_TP(struct host_table_entry *hte,
				 UINT32 delegateSize,		/* in */
				 BYTE *delegate)		/* in */
{
	TSS_RESULT result;

	initData(&hte->comm, 3);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DELEGATE_VERIFYDELEGATION;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &delegateSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 2, delegate, delegateSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_DSAP_TP(struct host_table_entry *hte,
	    TPM_ENTITY_TYPE entityType,	/* in */
	    TCS_KEY_HANDLE keyHandle,	/* in */
	    TPM_NONCE *nonceOddDSAP,	/* in */
	    UINT32 entityValueSize,	/* in */
	    BYTE * entityValue,		/* in */
	    TCS_AUTHHANDLE *authHandle,	/* out */
	    TPM_NONCE *nonceEven,	/* out */
	    TPM_NONCE *nonceEvenDSAP)	/* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 6);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DSAP;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT16, 1, &entityType, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &keyHandle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_NONCE, 3, nonceOddDSAP, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 4, &entityValueSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 5, entityValue, entityValueSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, authHandle, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_NONCE, 1, nonceEven, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_NONCE, 2, nonceEvenDSAP, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}
