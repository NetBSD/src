
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
RPC_TakeOwnership_TP(struct host_table_entry *hte,
		      UINT16 protocolID,	/* in */
		      UINT32 encOwnerAuthSize,	/* in */
		      BYTE * encOwnerAuth,	/* in */
		      UINT32 encSrkAuthSize,	/* in */
		      BYTE * encSrkAuth,	/* in */
		      UINT32 srkInfoSize,	/* in */
		      BYTE * srkInfo,	/* in */
		      TPM_AUTH * ownerAuth,	/* in, out */
		      UINT32 * srkKeySize,
		      BYTE ** srkKey)
{
	TSS_RESULT result;

	initData(&hte->comm, 9);
	hte->comm.hdr.u.ordinal = TCSD_ORD_TAKEOWNERSHIP;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT16, 1, &protocolID, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &encOwnerAuthSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, encOwnerAuth, encOwnerAuthSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 4, &encSrkAuthSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 5, encSrkAuth, encSrkAuthSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 6, &srkInfoSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 7, srkInfo, srkInfoSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 8, ownerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, 1, srkKeySize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*srkKey = (BYTE *) malloc(*srkKeySize);
		if (*srkKey == NULL) {
			LogError("malloc of %u bytes failed.", *srkKeySize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 2, *srkKey, *srkKeySize, &hte->comm)) {
			free(*srkKey);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

done:
	return result;
}


TSS_RESULT
RPC_OwnerClear_TP(struct host_table_entry *hte,
		   TPM_AUTH * ownerAuth)	/* in, out */
{
        TSS_RESULT result;

	initData(&hte->comm, 2);
        hte->comm.hdr.u.ordinal = TCSD_ORD_OWNERCLEAR;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

        if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
                return TSPERR(TSS_E_INTERNAL_ERROR);

        if (setData(TCSD_PACKET_TYPE_AUTH, 1, ownerAuth, 0, &hte->comm))
                return TSPERR(TSS_E_INTERNAL_ERROR);

        result = sendTCSDPacket(hte);

        if (result == TSS_SUCCESS)
                result = hte->comm.hdr.u.result;

        if (result == TSS_SUCCESS ){
                if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm))
                        result = TSPERR(TSS_E_INTERNAL_ERROR);
        }

        return result;
}
