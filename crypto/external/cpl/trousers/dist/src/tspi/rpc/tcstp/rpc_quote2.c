
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
RPC_Quote2_TP(struct host_table_entry *hte,
	      TCS_KEY_HANDLE keyHandle,	/* in */
	      TCPA_NONCE *antiReplay,	/* in */
	      UINT32 pcrDataSizeIn,	/* in */
	      BYTE * pcrDataIn,	/* in */
	      TSS_BOOL addVersion, /* in */
	      TPM_AUTH * privAuth,	/* in, out */
	      UINT32 * pcrDataSizeOut,	/* out */
	      BYTE ** pcrDataOut,	/* out */
	      UINT32* versionInfoSize, /* out */
	      BYTE** versionInfo, /* out */
	      UINT32 * sigSize,	/* out */
	      BYTE ** sig)	/* out */
{
	TSS_RESULT result;
	int i;

	initData(&hte->comm, 7);

	hte->comm.hdr.u.ordinal = TCSD_ORD_QUOTE2;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);


	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &keyHandle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_NONCE, 2, antiReplay, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, &pcrDataSizeIn, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 4, pcrDataIn, pcrDataSizeIn, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_BOOL, 5, &addVersion, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	if (privAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 6, privAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	/* Takes and sets the output data */
	if (result == TSS_SUCCESS) {
		i = 0;
		if (privAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, privAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pcrDataSizeOut, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*pcrDataOut = (BYTE *) malloc(*pcrDataSizeOut);
		if (*pcrDataOut == NULL) {
			LogError("malloc of %u bytes failed.", *pcrDataSizeOut);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *pcrDataOut, *pcrDataSizeOut, &hte->comm)) {
			free(*pcrDataOut);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		/* Retrieves the versionInfo Parameters */
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, versionInfoSize, 0, &hte->comm)) {
			free(*pcrDataOut);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (*versionInfoSize >0){
			*versionInfo = (BYTE *) malloc(*versionInfoSize);
			if (*versionInfo == NULL) {
				LogError("malloc of %u bytes failed.", *versionInfoSize);
				free(*pcrDataOut);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
			if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *versionInfo, *versionInfoSize,
				    &hte->comm)) {
				free(*pcrDataOut);
				free(*versionInfo);
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}

		if (getData(TCSD_PACKET_TYPE_UINT32, i++, sigSize, 0, &hte->comm)) {
			free(*pcrDataOut);
			if (addVersion)
				free(*versionInfo);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*sig = (BYTE *) malloc(*sigSize);
		if (*sig == NULL) {
			LogError("malloc of %u bytes failed.", *sigSize);
			free(*pcrDataOut);
			if (addVersion)
				free(*versionInfo);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *sig, *sigSize, &hte->comm)) {
			free(*pcrDataOut);
			if (addVersion)
				free(*versionInfo);
			free(*sig);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

done:
	return result;
}
