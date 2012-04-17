
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
RPC_CertifyKey_TP(struct host_table_entry *hte,
			       TCS_KEY_HANDLE certHandle,	/* in */
			       TCS_KEY_HANDLE keyHandle,	/* in */
			       TPM_NONCE * antiReplay,	/* in */
			       TPM_AUTH * certAuth,	/* in, out */
			       TPM_AUTH * keyAuth,	/* in, out */
			       UINT32 * CertifyInfoSize,	/* out */
			       BYTE ** CertifyInfo,	/* out */
			       UINT32 * outDataSize,	/* out */
			       BYTE ** outData	/* out */
    ) {
	TSS_RESULT result;
	TPM_AUTH null_auth;
	int i;

	initData(&hte->comm, 6);
	memset(&null_auth, 0, sizeof(TPM_AUTH));

	hte->comm.hdr.u.ordinal = TCSD_ORD_CERTIFYKEY;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &certHandle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &keyHandle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_NONCE, 3, antiReplay, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (certAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, certAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, &null_auth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	if (keyAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 5, keyAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		if (setData(TCSD_PACKET_TYPE_AUTH, 5, &null_auth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (certAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, certAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (keyAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, keyAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, CertifyInfoSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*CertifyInfo = (BYTE *) malloc(*CertifyInfoSize);
		if (*CertifyInfo == NULL) {
			LogError("malloc of %u bytes failed.", *CertifyInfoSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *CertifyInfo, *CertifyInfoSize, &hte->comm)) {
			free(*CertifyInfo);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, outDataSize, 0, &hte->comm)) {
			free(*CertifyInfo);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*outData = (BYTE *) malloc(*outDataSize);
		if (*outData == NULL) {
			LogError("malloc of %u bytes failed.", *outDataSize);
			free(*CertifyInfo);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *outData, *outDataSize, &hte->comm)) {
			free(*CertifyInfo);
			free(*outData);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}

done:
	return result;
}
