
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
RPC_CMK_SetRestrictions_TP(struct host_table_entry *hte,
			   TSS_CMK_DELEGATE restriction,	/* in */
			   TPM_AUTH *ownerAuth)			/* in, out */
{
	TSS_RESULT result;

	initData(&hte->comm, 3);
	hte->comm.hdr.u.ordinal = TCSD_ORD_CMK_SETRESTRICTIONS;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &restriction, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 2, ownerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

TSS_RESULT
RPC_CMK_ApproveMA_TP(struct host_table_entry *hte,
		     TPM_DIGEST migAuthorityDigest,	/* in */
		     TPM_AUTH *ownerAuth,		/* in, out */
		     TPM_HMAC *migAuthorityApproval)	/* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 3);
	hte->comm.hdr.u.ordinal = TCSD_ORD_CMK_APPROVEMA;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_DIGEST, 1, &migAuthorityDigest, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 2, ownerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_DIGEST, 1, migAuthorityApproval, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

TSS_RESULT
RPC_CMK_CreateKey_TP(struct host_table_entry *hte,
		     TCS_KEY_HANDLE hWrappingKey,	/* in */
		     TPM_ENCAUTH *keyUsageAuth,		/* in */
		     TPM_HMAC *migAuthorityApproval,	/* in */
		     TPM_DIGEST *migAuthorityDigest,	/* in */
		     UINT32 *keyDataSize,		/* in, out */
		     BYTE **keyData,			/* in, out */
		     TPM_AUTH *pAuth)			/* in, out */
{
	TSS_RESULT result;

	initData(&hte->comm, 8);
	hte->comm.hdr.u.ordinal = TCSD_ORD_CMK_CREATEKEY;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hWrappingKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_ENCAUTH, 2, keyUsageAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_DIGEST, 3, migAuthorityApproval, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_DIGEST, 4, migAuthorityDigest, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 5, keyDataSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 6, *keyData, *keyDataSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (pAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 7, pAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		TPM_AUTH nullAuth;

		memset(&nullAuth, 0, sizeof(TPM_AUTH));
		if (setData(TCSD_PACKET_TYPE_AUTH, 7, &nullAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	free(*keyData);
	*keyData = NULL;

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, keyDataSize, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		*keyData = (BYTE *)malloc(*keyDataSize);
		if (*keyData == NULL) {
			LogError("malloc of %u bytes failed.", *keyDataSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 1, *keyData, *keyDataSize, &hte->comm)) {
			free(*keyData);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (pAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, 2, pAuth, 0, &hte->comm)) {
				free(*keyData);
				return TSPERR(TSS_E_INTERNAL_ERROR);
			}
		}
	}

	return result;
}

TSS_RESULT
RPC_CMK_CreateTicket_TP(struct host_table_entry *hte,
			UINT32 publicVerifyKeySize,	/* in */
			BYTE *publicVerifyKey,		/* in */
			TPM_DIGEST signedData,		/* in */
			UINT32 sigValueSize,		/* in */
			BYTE *sigValue,			/* in */
			TPM_AUTH *ownerAuth,		/* in, out */
			TPM_HMAC *sigTicket)		/* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 7);
	hte->comm.hdr.u.ordinal = TCSD_ORD_CMK_CREATETICKET;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &publicVerifyKeySize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 2, publicVerifyKey, publicVerifyKeySize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_DIGEST, 3, &signedData, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 4, &sigValueSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 5, sigValue, sigValueSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 6, ownerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_DIGEST, 1, sigTicket, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

TSS_RESULT
RPC_CMK_CreateBlob_TP(struct host_table_entry *hte,
		      TCS_KEY_HANDLE hParentKey,	/* in */
		      TSS_MIGRATE_SCHEME migrationType,	/* in */
		      UINT32 migKeyAuthSize,		/* in */
		      BYTE *migKeyAuth,			/* in */
		      TPM_DIGEST pubSourceKeyDigest,	/* in */
		      UINT32 msaListSize,		/* in */
		      BYTE *msaList,			/* in */
		      UINT32 restrictTicketSize,	/* in */
		      BYTE *restrictTicket,		/* in */
		      UINT32 sigTicketSize,		/* in */
		      BYTE *sigTicket,			/* in */
		      UINT32 encDataSize,		/* in */
		      BYTE *encData,			/* in */
		      TPM_AUTH *pAuth,			/* in, out */
		      UINT32 *randomSize,		/* out */
		      BYTE **random,			/* out */
		      UINT32 *outDataSize,		/* out */
		      BYTE **outData)			/* out */
{
	TSS_RESULT result;
	int i;

	initData(&hte->comm, 15);
	hte->comm.hdr.u.ordinal = TCSD_ORD_CMK_CREATEBLOB;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hParentKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT16, 2, &migrationType, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, &migKeyAuthSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 4, migKeyAuth, migKeyAuthSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_DIGEST, 5, &pubSourceKeyDigest, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 6, &msaListSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 7, msaList, msaListSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 8, &restrictTicketSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 9, restrictTicket, restrictTicketSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 10, &sigTicketSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 11, sigTicket, sigTicketSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 12, &encDataSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 13, encData, encDataSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (pAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 14, pAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		TPM_AUTH nullAuth;

		memset(&nullAuth, 0, sizeof(TPM_AUTH));
		if (setData(TCSD_PACKET_TYPE_AUTH, 14, &nullAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (pAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &hte->comm))
				return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, randomSize, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		*random = (BYTE *)malloc(*randomSize);
		if (*random == NULL) {
			LogError("malloc of %u bytes failed.", *randomSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *random, *randomSize, &hte->comm)) {
			free(*random);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, outDataSize, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		*outData = (BYTE *)malloc(*outDataSize);
		if (*outData == NULL) {
			LogError("malloc of %u bytes failed.", *outDataSize);
			free(*random);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *outData, *outDataSize, &hte->comm)) {
			free(*random);
			free(*outData);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	return result;
}

TSS_RESULT
RPC_CMK_ConvertMigration_TP(struct host_table_entry *hte,
			    TCS_KEY_HANDLE hParentHandle,	/* in */
			    TPM_CMK_AUTH restrictTicket,	/* in */
			    TPM_HMAC sigTicket,			/* in */
			    UINT32 keyDataSize,			/* in */
			    BYTE *keyData,			/* in */
			    UINT32 msaListSize,			/* in */
			    BYTE *msaList,			/* in */	
			    UINT32 randomSize,			/* in */
			    BYTE *random,			/* in */
			    TPM_AUTH *pAuth,			/* in, out */
			    UINT32 *outDataSize,		/* out */
			    BYTE **outData)			/* out */
{
	TSS_RESULT result;
	int i;

	initData(&hte->comm, 11);
	hte->comm.hdr.u.ordinal = TCSD_ORD_CMK_CONVERTMIGRATION;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hParentHandle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 2, &restrictTicket, sizeof(restrictTicket), &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_DIGEST, 3, &sigTicket, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 4, &keyDataSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 5, keyData, keyDataSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 6, &msaListSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 7, msaList, msaListSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 8, &randomSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 9, random, randomSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (pAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 10, pAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		TPM_AUTH nullAuth;

		memset(&nullAuth, 0, sizeof(TPM_AUTH));
		if (setData(TCSD_PACKET_TYPE_AUTH, 10, &nullAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (pAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &hte->comm))
				return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, outDataSize, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		*outData = (BYTE *)malloc(*outDataSize);
		if (*outData == NULL) {
			LogError("malloc of %u bytes failed.", *outDataSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *outData, *outDataSize, &hte->comm)) {
			free(*outData);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	return result;
}

