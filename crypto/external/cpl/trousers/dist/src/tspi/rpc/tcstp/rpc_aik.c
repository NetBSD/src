
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
RPC_MakeIdentity_TP(struct host_table_entry *hte,
		    TCPA_ENCAUTH identityAuth,	/* in */
		    TCPA_CHOSENID_HASH IDLabel_PrivCAHash,	/* in */
		    UINT32 idKeyInfoSize,	/* in */
		    BYTE * idKeyInfo,	/* in */
		    TPM_AUTH * pSrkAuth,	/* in, out */
		    TPM_AUTH * pOwnerAuth,	/* in, out */
		    UINT32 * idKeySize,	/* out */
		    BYTE ** idKey,	/* out */
		    UINT32 * pcIdentityBindingSize,	/* out */
		    BYTE ** prgbIdentityBinding,	/* out */
		    UINT32 * pcEndorsementCredentialSize,	/* out */
		    BYTE ** prgbEndorsementCredential,	/* out */
		    UINT32 * pcPlatformCredentialSize,	/* out */
		    BYTE ** prgbPlatformCredential,	/* out */
		    UINT32 * pcConformanceCredentialSize,	/* out */
		    BYTE ** prgbConformanceCredential)	/* out */
{
	TSS_RESULT result;
	int i;

	initData(&hte->comm, 7);
	hte->comm.hdr.u.ordinal = TCSD_ORD_MAKEIDENTITY;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_ENCAUTH, 1, &identityAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_DIGEST, 2, &IDLabel_PrivCAHash, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, &idKeyInfoSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 4, idKeyInfo, idKeyInfoSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	i = 5;
	if (pSrkAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, i++, pSrkAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	if (setData(TCSD_PACKET_TYPE_AUTH, i++, pOwnerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	i = 0;
	if (result == TSS_SUCCESS) {
		i = 0;
		if (pSrkAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, pSrkAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (getData(TCSD_PACKET_TYPE_AUTH, i++, pOwnerAuth, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, idKeySize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*idKey = (BYTE *) malloc(*idKeySize);
		if (*idKey == NULL) {
			LogError("malloc of %u bytes failed.", *idKeySize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *idKey, *idKeySize, &hte->comm)) {
			free(*idKey);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pcIdentityBindingSize, 0, &hte->comm)) {
			free(*idKey);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*prgbIdentityBinding = (BYTE *) malloc(*pcIdentityBindingSize);
		if (*prgbIdentityBinding == NULL) {
			LogError("malloc of %u bytes failed.", *pcIdentityBindingSize);
			free(*idKey);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *prgbIdentityBinding, *pcIdentityBindingSize, &hte->comm)) {
			free(*idKey);
			free(*prgbIdentityBinding);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pcEndorsementCredentialSize, 0, &hte->comm)) {
			free(*idKey);
			free(*prgbIdentityBinding);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*prgbEndorsementCredential = (BYTE *) malloc(*pcEndorsementCredentialSize);
		if (*prgbEndorsementCredential == NULL) {
			LogError("malloc of %u bytes failed.", *pcEndorsementCredentialSize);
			free(*idKey);
			free(*prgbIdentityBinding);
			*prgbIdentityBinding = NULL;
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *prgbEndorsementCredential, *pcEndorsementCredentialSize, &hte->comm)) {
			free(*idKey);
			free(*prgbIdentityBinding);
			*prgbIdentityBinding = NULL;
			free(*prgbEndorsementCredential);
			*prgbEndorsementCredential = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pcPlatformCredentialSize, 0, &hte->comm)) {
			free(*idKey);
			free(*prgbIdentityBinding);
			*prgbIdentityBinding = NULL;
			free(*prgbEndorsementCredential);
			*prgbEndorsementCredential = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*prgbPlatformCredential = (BYTE *) malloc(*pcPlatformCredentialSize);
		if (*prgbPlatformCredential == NULL) {
			LogError("malloc of %u bytes failed.", *pcPlatformCredentialSize);
			free(*idKey);
			free(*prgbIdentityBinding);
			*prgbIdentityBinding = NULL;
			free(*prgbEndorsementCredential);
			*prgbEndorsementCredential = NULL;
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *prgbPlatformCredential, *pcPlatformCredentialSize, &hte->comm)) {
			free(*idKey);
			free(*prgbIdentityBinding);
			*prgbIdentityBinding = NULL;
			free(*prgbEndorsementCredential);
			*prgbEndorsementCredential = NULL;
			free(*prgbPlatformCredential);
			*prgbPlatformCredential = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pcConformanceCredentialSize, 0, &hte->comm)) {
			free(*idKey);
			free(*prgbIdentityBinding);
			*prgbIdentityBinding = NULL;
			free(*prgbEndorsementCredential);
			*prgbEndorsementCredential = NULL;
			free(*prgbPlatformCredential);
			*prgbPlatformCredential = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*prgbConformanceCredential = (BYTE *) malloc(*pcConformanceCredentialSize);
		if (*prgbConformanceCredential == NULL) {
			LogError("malloc of %u bytes failed.", *pcConformanceCredentialSize);
			free(*idKey);
			free(*prgbIdentityBinding);
			*prgbIdentityBinding = NULL;
			free(*prgbEndorsementCredential);
			*prgbEndorsementCredential = NULL;
			free(*prgbPlatformCredential);
			*prgbPlatformCredential = NULL;
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *prgbConformanceCredential, *pcConformanceCredentialSize, &hte->comm)) {
			free(*idKey);
			free(*prgbIdentityBinding);
			*prgbIdentityBinding = NULL;
			free(*prgbEndorsementCredential);
			*prgbEndorsementCredential = NULL;
			free(*prgbPlatformCredential);
			*prgbPlatformCredential = NULL;
			free(*prgbConformanceCredential);
			*prgbConformanceCredential = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_GetCredential_TP(struct host_table_entry *hte,
		     UINT32 ulCredentialType,          /* in */
		     UINT32 ulCredentialAccessMode,    /* in */
		     UINT32 * pulCredentialSize,       /* out */
		     BYTE ** prgbCredentialData)       /* out */
{
	TSS_RESULT result;

	initData(&hte->comm, 3);
	hte->comm.hdr.u.ordinal = TCSD_ORD_GETCREDENTIAL;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &ulCredentialType, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &ulCredentialAccessMode, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, pulCredentialSize, 0, &hte->comm)) {
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}

		*prgbCredentialData = (BYTE *) malloc(*pulCredentialSize);
		if (*prgbCredentialData == NULL) {
			LogError("malloc of %u bytes failed.", *pulCredentialSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		if (getData(TCSD_PACKET_TYPE_PBYTE, 1, *prgbCredentialData,
			    *pulCredentialSize, &hte->comm)) {
			free(*prgbCredentialData);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
                }
	}
	return result;
}

TSS_RESULT
RPC_ActivateTPMIdentity_TP(struct host_table_entry *hte,
			   TCS_KEY_HANDLE idKey,	/* in */
			   UINT32 blobSize,	/* in */
			   BYTE * blob,	/* in */
			   TPM_AUTH * idKeyAuth,	/* in, out */
			   TPM_AUTH * ownerAuth,	/* in, out */
			   UINT32 * SymmetricKeySize,	/* out */
			   BYTE ** SymmetricKey)	/* out */
{
	TSS_RESULT result;
	int i = 0;

	initData(&hte->comm, 6);
	hte->comm.hdr.u.ordinal = TCSD_ORD_ACTIVATETPMIDENTITY;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, i++, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, i++, &idKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, i++, &blobSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, i++, blob, blobSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (idKeyAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, i++, idKeyAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	if (setData(TCSD_PACKET_TYPE_AUTH, i++, ownerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (idKeyAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, idKeyAuth, 0, &hte->comm))
				result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_AUTH, i++, ownerAuth, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, SymmetricKeySize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*SymmetricKey = malloc(*SymmetricKeySize);
		if (*SymmetricKey == NULL) {
			LogError("malloc of %u bytes failed.", *SymmetricKeySize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *SymmetricKey, *SymmetricKeySize, &hte->comm)) {
			free(*SymmetricKey);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}
done:
	return result;
}
