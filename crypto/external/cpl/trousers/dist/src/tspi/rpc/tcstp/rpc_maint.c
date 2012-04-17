
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
RPC_CreateMaintenanceArchive_TP(struct host_table_entry *hte,
					     TSS_BOOL generateRandom,	/* in */
					     TPM_AUTH * ownerAuth,	/* in, out */
					     UINT32 * randomSize,	/* out */
					     BYTE ** random,	/* out */
					     UINT32 * archiveSize,	/* out */
					     BYTE ** archive	/* out */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 3);
	hte->comm.hdr.u.ordinal = TCSD_ORD_CREATEMAINTENANCEARCHIVE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_BOOL, 1, &generateRandom, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 2, ownerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_UINT32, 1, randomSize, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);

		if (*randomSize > 0) {
			*random = malloc(*randomSize);
			if (*random == NULL) {
				LogError("malloc of %u bytes failed.", *randomSize);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}

			if (getData(TCSD_PACKET_TYPE_PBYTE, 2, *random, *randomSize, &hte->comm)) {
				free(*random);
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		} else {
			*random = NULL;
		}

		/* Assume all elements are in the list, even when *randomSize == 0. */
		if (getData(TCSD_PACKET_TYPE_UINT32, 3, archiveSize, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);

		if (*archiveSize > 0) {
			*archive = malloc(*archiveSize);
			if (*archive == NULL) {
				free(*random);
				LogError("malloc of %u bytes failed.", *archiveSize);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}

			if (getData(TCSD_PACKET_TYPE_PBYTE, 4, *archive, *archiveSize, &hte->comm)) {
				free(*random);
				free(*archive);
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		} else {
			*archive = NULL;
		}
	}
done:
	return result;
}

TSS_RESULT
RPC_LoadMaintenanceArchive_TP(struct host_table_entry *hte,
					   UINT32 dataInSize,	/* in */
					   BYTE * dataIn,	/* in */
					   TPM_AUTH * ownerAuth,	/* in, out */
					   UINT32 * dataOutSize,	/* out */
					   BYTE ** dataOut	/* out */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 4);
	hte->comm.hdr.u.ordinal = TCSD_ORD_LOADMAINTENANCEARCHIVE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &dataInSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 2, &dataIn, dataInSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 3, ownerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_UINT32, 1, dataOutSize, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);

		if (*dataOutSize > 0) {
			*dataOut = malloc(*dataOutSize);
			if (*dataOut == NULL) {
				LogError("malloc of %u bytes failed.", *dataOutSize);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}

			if (getData(TCSD_PACKET_TYPE_PBYTE, 2, *dataOut, *dataOutSize, &hte->comm)) {
				free(*dataOut);
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		} else {
			*dataOut = NULL;
		}
	}
done:
	return result;
}

TSS_RESULT
RPC_KillMaintenanceFeature_TP(struct host_table_entry *hte,
					   TPM_AUTH * ownerAuth	/* in , out */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_KILLMAINTENANCEFEATURE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 1, ownerAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_AUTH, 0, ownerAuth, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

TSS_RESULT
RPC_LoadManuMaintPub_TP(struct host_table_entry *hte,
				     TCPA_NONCE antiReplay,	/* in */
				     UINT32 PubKeySize,	/* in */
				     BYTE * PubKey,	/* in */
				     TCPA_DIGEST * checksum	/* out */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 4);
	hte->comm.hdr.u.ordinal = TCSD_ORD_LOADMANUFACTURERMAINTENANCEPUB;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_NONCE, 1, &antiReplay, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &PubKeySize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, PubKey, PubKeySize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_DIGEST, 0, checksum, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

TSS_RESULT
RPC_ReadManuMaintPub_TP(struct host_table_entry *hte,
				     TCPA_NONCE antiReplay,	/* in */
				     TCPA_DIGEST * checksum	/* out */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_READMANUFACTURERMAINTENANCEPUB;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_NONCE, 1, &antiReplay, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_DIGEST, 0, checksum, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}
