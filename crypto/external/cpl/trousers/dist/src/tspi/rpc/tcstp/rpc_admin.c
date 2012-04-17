
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
RPC_SetOwnerInstall_TP(struct host_table_entry *hte,
		       TSS_BOOL state)	/* in */
{
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_SETOWNERINSTALL;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_BOOL, 1, &state, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_DisableOwnerClear_TP(struct host_table_entry *hte,
			 TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DISABLEOWNERCLEAR;
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

TSS_RESULT
RPC_ForceClear_TP(struct host_table_entry *hte)
{
	TSS_RESULT result;

	initData(&hte->comm, 1);
	hte->comm.hdr.u.ordinal = TCSD_ORD_FORCECLEAR;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_DisableForceClear_TP(struct host_table_entry *hte)
{
	TSS_RESULT result;

	initData(&hte->comm, 1);
	hte->comm.hdr.u.ordinal = TCSD_ORD_DISABLEFORCECLEAR;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_PhysicalDisable_TP(struct host_table_entry *hte)
{
	TSS_RESULT result;

	initData(&hte->comm, 1);
	hte->comm.hdr.u.ordinal = TCSD_ORD_PHYSICALDISABLE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_PhysicalEnable_TP(struct host_table_entry *hte)
{
	TSS_RESULT result;

	initData(&hte->comm, 1);
	hte->comm.hdr.u.ordinal = TCSD_ORD_PHYSICALENABLE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_OwnerSetDisable_TP(struct host_table_entry *hte,
		       TSS_BOOL disableState,     /*  in */
		       TPM_AUTH * ownerAuth)   /*  in, out */
{
	TSS_RESULT result;

	initData(&hte->comm, 3);
	hte->comm.hdr.u.ordinal = TCSD_ORD_OWNERSETDISABLE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_BOOL, 1, &disableState, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_AUTH, 2, ownerAuth, 0, &hte->comm))
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
RPC_PhysicalSetDeactivated_TP(struct host_table_entry *hte,
			      TSS_BOOL state)	/* in */
{
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_PHYSICALSETDEACTIVATED;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_BOOL, 1, &state, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_PhysicalPresence_TP(struct host_table_entry *hte,
			TCPA_PHYSICAL_PRESENCE fPhysicalPresence)	/* in */
{
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_PHYSICALPRESENCE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT16, 1, &fPhysicalPresence, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_SetTempDeactivated_TP(struct host_table_entry *hte)
{
	TSS_RESULT result;

	initData(&hte->comm, 1);
	hte->comm.hdr.u.ordinal = TCSD_ORD_SETTEMPDEACTIVATED;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT
RPC_SetTempDeactivated2_TP(struct host_table_entry *hte,
			   TPM_AUTH *operatorAuth)		/* in/out */
{
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_SETTEMPDEACTIVATED2;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (operatorAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 1, operatorAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		TPM_AUTH nullAuth;

		memset(&nullAuth, 0, sizeof(TPM_AUTH));
		if (setData(TCSD_PACKET_TYPE_AUTH, 1, &nullAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (operatorAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, 0, operatorAuth, 0, &hte->comm))
				result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	return result;
}
#endif

TSS_RESULT
RPC_FieldUpgrade_TP(struct host_table_entry *hte,
		    UINT32 dataInSize,	/* in */
		    BYTE * dataIn,	/* in */
		    UINT32 * dataOutSize,	/* out */
		    BYTE ** dataOut,	/* out */
		    TPM_AUTH * ownerAuth)	/* in, out */
{
	return TSPERR(TSS_E_NOTIMPL);
}

TSS_RESULT
RPC_SetRedirection_TP(struct host_table_entry *hte,
		      TCS_KEY_HANDLE keyHandle,	/* in */
		      UINT32 c1,	/* in */
		      UINT32 c2,	/* in */
		      TPM_AUTH * privAuth)	/* in, out */
{
	return TSPERR(TSS_E_NOTIMPL);

}

#ifdef TSS_BUILD_TSS12
TSS_RESULT
RPC_ResetLockValue_TP(struct host_table_entry *hte,
		      TPM_AUTH * ownerAuth)   /* in, out */
{
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_RESETLOCKVALUE;
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
RPC_FlushSpecific_TP(struct host_table_entry *hte,
		     TCS_HANDLE hResHandle, /* in */
		     TPM_RESOURCE_TYPE resourceType) /* in */
{
	TSS_RESULT result;

	initData(&hte->comm, 3);
	hte->comm.hdr.u.ordinal = TCSD_ORD_FLUSHSPECIFIC;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hResHandle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &resourceType, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}
#endif
