
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
#include <syslog.h>
#include <string.h>
#include <netdb.h>
#if (defined (__OpenBSD__) || defined (__FreeBSD__) || defined(__NetBSD__))
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include <errno.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "tcsd_wrap.h"
#include "tcsd.h"
#include "rpc_tcstp_tcs.h"


/* Lock is not static because we need to reference it in the auth manager */
MUTEX_DECLARE_INIT(tcsp_lock);


void
LoadBlob_Auth_Special(UINT64 *offset, BYTE *blob, TPM_AUTH *auth)
{
	LoadBlob(offset, TCPA_SHA1BASED_NONCE_LEN, blob, auth->NonceEven.nonce);
	LoadBlob_BOOL(offset, auth->fContinueAuthSession, blob);
	LoadBlob(offset, TCPA_SHA1BASED_NONCE_LEN, blob, (BYTE *)&auth->HMAC);
}

void
UnloadBlob_Auth_Special(UINT64 *offset, BYTE *blob, TPM_AUTH *auth)
{
	UnloadBlob_UINT32(offset, &auth->AuthHandle, blob);
	UnloadBlob(offset, TCPA_SHA1BASED_NONCE_LEN, blob, auth->NonceOdd.nonce);
	UnloadBlob_BOOL(offset, &auth->fContinueAuthSession, blob);
	UnloadBlob(offset, TCPA_SHA1BASED_NONCE_LEN, blob, (BYTE *)&auth->HMAC);
}

int
recv_from_socket(int sock, void *buffer, int size)
{
        int recv_size = 0, recv_total = 0;

	while (recv_total < size) {
		errno = 0;
		if ((recv_size = recv(sock, buffer+recv_total, size-recv_total, 0)) <= 0) {
			if (recv_size < 0) {
				if (errno == EINTR)
					continue;
				LogError("Socket receive connection error: %s.", strerror(errno));
			} else {
				LogDebug("Socket connection closed.");
			}

			return -1;
		}
		recv_total += recv_size;
	}

	return recv_total;
}

int
send_to_socket(int sock, void *buffer, int size)
{
	int send_size = 0, send_total = 0;

	while (send_total < size) {
		if ((send_size = send(sock, buffer+send_total, size-send_total, 0)) < 0) {
			LogError("Socket send connection error: %s.", strerror(errno));
			return -1;
		}
		send_total += send_size;
	}

	return send_total;
}


void
initData(struct tcsd_comm_data *comm, int parm_count)
{
	/* min packet size should be the size of the header */
	memset(&comm->hdr, 0, sizeof(struct tcsd_packet_hdr));
	comm->hdr.packet_size = sizeof(struct tcsd_packet_hdr);
	if (parm_count > 0) {
		comm->hdr.type_offset = sizeof(struct tcsd_packet_hdr);
		comm->hdr.parm_offset = comm->hdr.type_offset +
			(sizeof(TCSD_PACKET_TYPE) * parm_count);
		comm->hdr.packet_size = comm->hdr.parm_offset;
	}

	memset(comm->buf, 0, comm->buf_size);
}

int
loadData(UINT64 *offset, TCSD_PACKET_TYPE data_type, void *data, int data_size, BYTE *blob)
{
	switch (data_type) {
		case TCSD_PACKET_TYPE_BYTE:
			LoadBlob_BYTE(offset, *((BYTE *) (data)), blob);
			break;
		case TCSD_PACKET_TYPE_BOOL:
			LoadBlob_BOOL(offset, *((TSS_BOOL *) (data)), blob);
			break;
		case TCSD_PACKET_TYPE_UINT16:
			LoadBlob_UINT16(offset, *((UINT16 *) (data)), blob);
			break;
		case TCSD_PACKET_TYPE_UINT32:
			LoadBlob_UINT32(offset, *((UINT32 *) (data)), blob);
			break;
		case TCSD_PACKET_TYPE_UINT64:
			LoadBlob_UINT64(offset, *((UINT64 *) (data)), blob);
			break;
		case TCSD_PACKET_TYPE_PBYTE:
			LoadBlob(offset, data_size, blob, data);
			break;
		case TCSD_PACKET_TYPE_NONCE:
			LoadBlob(offset, sizeof(TCPA_NONCE), blob, ((TCPA_NONCE *)data)->nonce);
			break;
		case TCSD_PACKET_TYPE_DIGEST:
			LoadBlob(offset, sizeof(TCPA_DIGEST), blob, ((TCPA_DIGEST *)data)->digest);
			break;
		case TCSD_PACKET_TYPE_AUTH:
			LoadBlob_Auth_Special(offset, blob, ((TPM_AUTH *)data));
			break;
#ifdef TSS_BUILD_PS
		case TCSD_PACKET_TYPE_UUID:
			LoadBlob_UUID(offset, blob, *((TSS_UUID *)data));
			break;
		case TCSD_PACKET_TYPE_KM_KEYINFO:
			LoadBlob_KM_KEYINFO(offset, blob, ((TSS_KM_KEYINFO *)data));
			break;
		case TCSD_PACKET_TYPE_KM_KEYINFO2:
			LoadBlob_KM_KEYINFO2(offset, blob, ((TSS_KM_KEYINFO2 *)data));
			break;
		case TCSD_PACKET_TYPE_LOADKEY_INFO:
			LoadBlob_LOADKEY_INFO(offset, blob, ((TCS_LOADKEY_INFO *)data));
			break;
#endif
		case TCSD_PACKET_TYPE_ENCAUTH:
			LoadBlob(offset, sizeof(TCPA_ENCAUTH), blob,
				 ((TCPA_ENCAUTH *)data)->authdata);
			break;
		case TCSD_PACKET_TYPE_VERSION:
			LoadBlob_VERSION(offset, blob, ((TPM_VERSION *)data));
			break;
#ifdef TSS_BUILD_PCR_EVENTS
		case TCSD_PACKET_TYPE_PCR_EVENT:
			LoadBlob_PCR_EVENT(offset, blob, ((TSS_PCR_EVENT *)data));
			break;
#endif
		case TCSD_PACKET_TYPE_SECRET:
			LoadBlob(offset, sizeof(TCPA_SECRET), blob,
				 ((TCPA_SECRET *)data)->authdata);
			break;
		default:
			LogError("TCSD packet type unknown! (0x%x)", data_type & 0xff);
			return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	return TSS_SUCCESS;
}


int
setData(TCSD_PACKET_TYPE dataType,
	int index,
	void *theData,
	int theDataSize,
	struct tcsd_comm_data *comm)
{
	UINT64 old_offset, offset;
	TSS_RESULT result;
	TCSD_PACKET_TYPE *type;

	/* Calculate the size of the area needed (use NULL for blob address) */
	offset = 0;
	if ((result = loadData(&offset, dataType, theData, theDataSize, NULL)) != TSS_SUCCESS)
		return result;
	if (((int)comm->hdr.packet_size + (int)offset) < 0) {
		LogError("Too much data to be transmitted!");
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (((int)comm->hdr.packet_size + (int)offset) > comm->buf_size) {
		/* reallocate the buffer */
		BYTE *buffer;
		int buffer_size = comm->hdr.packet_size + offset;

		LogDebug("Increasing communication buffer to %d bytes.", buffer_size);
		buffer = realloc(comm->buf, buffer_size);
		if (buffer == NULL) {
			LogError("realloc of %d bytes failed.", buffer_size);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		comm->buf_size = buffer_size;
		comm->buf = buffer;
	}

	offset = old_offset = comm->hdr.parm_offset + comm->hdr.parm_size;
	if ((result = loadData(&offset, dataType, theData, theDataSize, comm->buf)) != TSS_SUCCESS)
		return result;
	type = (TCSD_PACKET_TYPE *)(comm->buf + comm->hdr.type_offset) + index;
	*type = dataType;
	comm->hdr.type_size += sizeof(TCSD_PACKET_TYPE);
	comm->hdr.parm_size += (offset - old_offset);

	comm->hdr.packet_size = offset;
	comm->hdr.num_parms++;

	return TSS_SUCCESS;
}

UINT32
getData(TCSD_PACKET_TYPE dataType,
	int index,
	void *theData,
	int theDataSize,
	struct tcsd_comm_data *comm)
{
	UINT64 old_offset, offset;
	TCSD_PACKET_TYPE *type = (TCSD_PACKET_TYPE *)(comm->buf + comm->hdr.type_offset) + index;

	if ((UINT32)index >= comm->hdr.num_parms || dataType != *type) {
		LogDebug("Data type of TCS packet element %d doesn't match.", index);
		return TSS_TCP_RPC_BAD_PACKET_TYPE;
	}
	old_offset = offset = comm->hdr.parm_offset;
	switch (dataType) {
		case TCSD_PACKET_TYPE_BYTE:
			if (old_offset + sizeof(BYTE) > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob_BYTE(&offset, (BYTE *) (theData), comm->buf);
			break;
		case TCSD_PACKET_TYPE_BOOL:
			if (old_offset + sizeof(TSS_BOOL) > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob_BOOL(&offset, (TSS_BOOL *) (theData), comm->buf);
			break;
		case TCSD_PACKET_TYPE_UINT16:
			if (old_offset + sizeof(UINT16) > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob_UINT16(&offset, (UINT16 *) (theData), comm->buf);
			break;
		case TCSD_PACKET_TYPE_UINT32:
			if (old_offset + sizeof(UINT32) > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob_UINT32(&offset, (UINT32 *) (theData), comm->buf);
			break;
		case TCSD_PACKET_TYPE_PBYTE:
			if (old_offset + theDataSize > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob(&offset, theDataSize, comm->buf, theData);
			break;
		case TCSD_PACKET_TYPE_NONCE:
			if (old_offset + sizeof(TPM_NONCE) > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob(&offset, sizeof(TCPA_NONCE), comm->buf,
					((TCPA_NONCE *) (theData))->nonce);
			break;
		case TCSD_PACKET_TYPE_DIGEST:
			if (old_offset + sizeof(TPM_DIGEST) > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob(&offset, sizeof(TCPA_DIGEST), comm->buf,
					((TCPA_DIGEST *) (theData))->digest);
			break;
		case TCSD_PACKET_TYPE_AUTH:
			if ((old_offset + sizeof(TCS_AUTHHANDLE)
					+ sizeof(TPM_BOOL)
					+ (2 * sizeof(TPM_NONCE))) > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob_Auth_Special(&offset, comm->buf, ((TPM_AUTH *) theData));
			break;
		case TCSD_PACKET_TYPE_ENCAUTH:
			if (old_offset + sizeof(TPM_ENCAUTH) > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob(&offset, sizeof(TCPA_ENCAUTH), comm->buf,
					((TCPA_ENCAUTH *) theData)->authdata);
			break;
		case TCSD_PACKET_TYPE_VERSION:
			if (old_offset + sizeof(TPM_VERSION) > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob_VERSION(&offset, comm->buf, ((TPM_VERSION *) theData));
			break;
#ifdef TSS_BUILD_PS
		case TCSD_PACKET_TYPE_KM_KEYINFO:
			UnloadBlob_KM_KEYINFO(&old_offset, comm->buf, NULL);

			if (old_offset > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			old_offset = offset;
			UnloadBlob_KM_KEYINFO(&offset, comm->buf, ((TSS_KM_KEYINFO *)theData));
			break;
		case TCSD_PACKET_TYPE_LOADKEY_INFO:
			UnloadBlob_LOADKEY_INFO(&old_offset, comm->buf, NULL);

			if (old_offset > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			old_offset = offset;
			UnloadBlob_LOADKEY_INFO(&offset, comm->buf, ((TCS_LOADKEY_INFO *)theData));
			break;
		case TCSD_PACKET_TYPE_UUID:
			if (old_offset + sizeof(TSS_UUID) > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob_UUID(&offset, comm->buf, (TSS_UUID *) theData);
			break;
#endif
#ifdef TSS_BUILD_PCR_EVENTS
		case TCSD_PACKET_TYPE_PCR_EVENT:
		{
			TSS_RESULT result;

			(void)UnloadBlob_PCR_EVENT(&old_offset, comm->buf, NULL);

			if (old_offset > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			old_offset = offset;
			if ((result = UnloadBlob_PCR_EVENT(&offset, comm->buf,
							   ((TSS_PCR_EVENT *)theData))))
				return result;
			break;
		}
#endif
		case TCSD_PACKET_TYPE_SECRET:
			if (old_offset + sizeof(TPM_SECRET) > comm->hdr.packet_size)
				return TCSERR(TSS_E_INTERNAL_ERROR);

			UnloadBlob(&offset, sizeof(TCPA_SECRET), comm->buf,
					((TCPA_SECRET *) theData)->authdata);
			break;
		default:
			LogError("TCSD packet type unknown! (0x%x)", dataType & 0xff);
			return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	comm->hdr.parm_offset = offset;
	comm->hdr.parm_size -= (offset - old_offset);

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_Error(struct tcsd_thread_data *data)
{
	LogError("%s reached.", __FUNCTION__);

	initData(&data->comm, 0);

	data->comm.hdr.u.result = TCSERR(TSS_E_FAIL);

	return TSS_SUCCESS;

}

/* Dispatch */
typedef struct tdDispatchTable {
	TSS_RESULT (*Func) (struct tcsd_thread_data *);
	const char *name;
} DispatchTable;

DispatchTable tcs_func_table[TCSD_MAX_NUM_ORDS] = {
	{tcs_wrap_Error,"Error"},   /* 0 */
	{tcs_wrap_OpenContext,"OpenContext"},
	{tcs_wrap_CloseContext,"CloseContext"},
	{tcs_wrap_Error,"Error"},
	{tcs_wrap_TCSGetCapability,"TCSGetCapability"},
	{tcs_wrap_RegisterKey,"RegisterKey"}, /* 5 */
	{tcs_wrap_UnregisterKey,"UnregisterKey"},
	{tcs_wrap_EnumRegisteredKeys,"EnumRegisteredKeys"},
	{tcs_wrap_Error,"Error"},
	{tcs_wrap_GetRegisteredKeyBlob,"GetRegisteredKeyBlob"},
	{tcs_wrap_GetRegisteredKeyByPublicInfo,"GetRegisteredKeyByPublicInfo"}, /* 10 */
	{tcs_wrap_LoadKeyByBlob,"LoadKeyByBlob"},
	{tcs_wrap_LoadKeyByUUID,"LoadKeyByUUID"},
	{tcs_wrap_EvictKey,"EvictKey"},
	{tcs_wrap_CreateWrapKey,"CreateWrapKey"},
	{tcs_wrap_GetPubkey,"GetPubkey"}, /* 15 */
	{tcs_wrap_MakeIdentity,"MakeIdentity"},
	{tcs_wrap_LogPcrEvent,"LogPcrEvent"},
	{tcs_wrap_GetPcrEvent,"GetPcrEvent"},
	{tcs_wrap_GetPcrEventsByPcr,"GetPcrEventsByPcr"},
	{tcs_wrap_GetPcrEventLog,"GetPcrEventLog"}, /* 20 */
	{tcs_wrap_SetOwnerInstall,"SetOwnerInstall"},
	{tcs_wrap_TakeOwnership,"TakeOwnership"},
	{tcs_wrap_OIAP,"OIAP"},
	{tcs_wrap_OSAP,"OSAP"},
	{tcs_wrap_ChangeAuth,"ChangeAuth"}, /* 25 */
	{tcs_wrap_ChangeAuthOwner,"ChangeAuthOwner"},
	{tcs_wrap_Error,"Error"},
	{tcs_wrap_Error,"Error"},
	{tcs_wrap_TerminateHandle,"TerminateHandle"},
	{tcs_wrap_ActivateIdentity,"ActivateIdentity"}, /* 30 */
	{tcs_wrap_Extend,"Extend"},
	{tcs_wrap_PcrRead,"PcrRead"},
	{tcs_wrap_Quote,"Quote"},
	{tcs_wrap_DirWriteAuth,"DirWriteAuth"},
	{tcs_wrap_DirRead,"DirRead"}, /* 35 */
	{tcs_wrap_Seal,"Seal"},
	{tcs_wrap_UnSeal,"UnSeal"},
	{tcs_wrap_UnBind,"UnBind"},
	{tcs_wrap_CreateMigrationBlob,"CreateMigrationBlob"},
	{tcs_wrap_ConvertMigrationBlob,"ConvertMigrationBlob"}, /* 40 */
	{tcs_wrap_AuthorizeMigrationKey,"AuthorizeMigrationKey"},
	{tcs_wrap_CertifyKey,"CertifyKey"},
	{tcs_wrap_Sign,"Sign"},
	{tcs_wrap_GetRandom,"GetRandom"},
	{tcs_wrap_StirRandom,"StirRandom"}, /* 45 */
	{tcs_wrap_GetCapability,"GetCapability"},
	{tcs_wrap_Error,"Error"},
	{tcs_wrap_GetCapabilityOwner,"GetCapabilityOwner"},
	{tcs_wrap_CreateEndorsementKeyPair,"CreateEndorsementKeyPair"},
	{tcs_wrap_ReadPubek,"ReadPubek"}, /* 50 */
	{tcs_wrap_DisablePubekRead,"DisablePubekRead"},
	{tcs_wrap_OwnerReadPubek,"OwnerReadPubek"},
	{tcs_wrap_SelfTestFull,"SelfTestFull"},
	{tcs_wrap_CertifySelfTest,"CertifySelfTest"},
	{tcs_wrap_Error,"Error"}, /* 55 */
	{tcs_wrap_GetTestResult,"GetTestResult"},
	{tcs_wrap_OwnerSetDisable,"OwnerSetDisable"},
	{tcs_wrap_OwnerClear,"OwnerClear"},
	{tcs_wrap_DisableOwnerClear,"DisableOwnerClear"},
	{tcs_wrap_ForceClear,"ForceClear"}, /* 60 */
	{tcs_wrap_DisableForceClear,"DisableForceClear"},
	{tcs_wrap_PhysicalDisable,"PhysicalDisable"},
	{tcs_wrap_PhysicalEnable,"PhysicalEnable"},
	{tcs_wrap_PhysicalSetDeactivated,"PhysicalSetDeactivated"},
	{tcs_wrap_SetTempDeactivated,"SetTempDeactivated"}, /* 65 */
	{tcs_wrap_PhysicalPresence,"PhysicalPresence"},
	{tcs_wrap_Error,"Error"},
	{tcs_wrap_Error,"Error"},
	{tcs_wrap_CreateMaintenanceArchive,"CreateMaintenanceArchive"},
	{tcs_wrap_LoadMaintenanceArchive,"LoadMaintenanceArchive"}, /* 70 */
	{tcs_wrap_KillMaintenanceFeature,"KillMaintenanceFeature"},
	{tcs_wrap_LoadManuMaintPub,"LoadManuMaintPub"},
	{tcs_wrap_ReadManuMaintPub,"ReadManuMaintPub"},
	{tcs_wrap_DaaJoin,"DaaJoin"},
	{tcs_wrap_DaaSign,"DaaSign"}, /* 75 */
	{tcs_wrap_SetCapability,"SetCapability"},
	{tcs_wrap_ResetLockValue,"ResetLockValue"},
	{tcs_wrap_PcrReset,"PcrReset"},
	{tcs_wrap_ReadCounter,"ReadCounter"},
	{tcs_wrap_CreateCounter,"CreateCounter"}, /* 80 */
	{tcs_wrap_IncrementCounter,"IncrementCounter"},
	{tcs_wrap_ReleaseCounter,"ReleaseCounter"},
	{tcs_wrap_ReleaseCounterOwner,"ReleaseCounterOwner"},
	{tcs_wrap_ReadCurrentTicks,"ReadCurrentTicks"},
	{tcs_wrap_TickStampBlob,"TicksStampBlob"}, /* 85 */
	{tcs_wrap_GetCredential,"GetCredential"},
	{tcs_wrap_NV_DefineOrReleaseSpace,"NVDefineOrReleaseSpace"},
	{tcs_wrap_NV_WriteValue,"NVWriteValue"},
	{tcs_wrap_NV_WriteValueAuth,"NVWriteValueAuth"},
	{tcs_wrap_NV_ReadValue,"NVReadValue"}, /* 90 */
	{tcs_wrap_NV_ReadValueAuth,"NVReadValueAuth"},
	{tcs_wrap_EstablishTransport,"EstablishTransport"},
	{tcs_wrap_ExecuteTransport,"ExecuteTransport"},
	{tcs_wrap_ReleaseTransportSigned,"ReleaseTransportSigned"},
	{tcs_wrap_SetOrdinalAuditStatus,"SetOrdinalAuditStatus"}, /* 95 */
	{tcs_wrap_GetAuditDigest,"GetAuditDigest"},
	{tcs_wrap_GetAuditDigestSigned,"GetAuditDigestSigned"},
	{tcs_wrap_Sealx,"Sealx"},
	{tcs_wrap_SetOperatorAuth,"SetOperatorAuth"},
	{tcs_wrap_OwnerReadInternalPub,"OwnerReadInternalPub"}, /* 100 */
	{tcs_wrap_EnumRegisteredKeys2,"EnumRegisteredKeys2"},
	{tcs_wrap_SetTempDeactivated2,"SetTempDeactivated2"},
	{tcs_wrap_Delegate_Manage,"Delegate_Manage"},
	{tcs_wrap_Delegate_CreateKeyDelegation,"Delegate_CreateKeyDelegation"},
	{tcs_wrap_Delegate_CreateOwnerDelegation,"Delegate_CreateOwnerDelegation"}, /* 105 */
	{tcs_wrap_Delegate_LoadOwnerDelegation,"Delegate_LoadOwnerDelegation"},
	{tcs_wrap_Delegate_ReadTable,"Delegate_ReadTable"},
	{tcs_wrap_Delegate_UpdateVerificationCount,"Delegate_UpdateVerificationCount"},
	{tcs_wrap_Delegate_VerifyDelegation,"Delegate_VerifyDelegation"},
	{tcs_wrap_CreateRevocableEndorsementKeyPair,"CreateRevocableEndorsementKeyPair"}, /* 110 */
	{tcs_wrap_RevokeEndorsementKeyPair,"RevokeEndorsementKeyPair"},
	{tcs_wrap_Error,"Error - was MakeIdentity2"},
	{tcs_wrap_Quote2,"Quote2"},
	{tcs_wrap_CMK_SetRestrictions,"CMK_SetRestrictions"},
	{tcs_wrap_CMK_ApproveMA,"CMK_ApproveMA"}, /* 115 */
	{tcs_wrap_CMK_CreateKey,"CMK_CreateKey"},
	{tcs_wrap_CMK_CreateTicket,"CMK_CreateTicket"},
	{tcs_wrap_CMK_CreateBlob,"CMK_CreateBlob"},
	{tcs_wrap_CMK_ConvertMigration,"CMK_ConvertMigration"},
	{tcs_wrap_FlushSpecific,"FlushSpecific"}, /* 120 */
	{tcs_wrap_KeyControlOwner, "KeyControlOwner"},
	{tcs_wrap_DSAP, "DSAP"}
};

int
access_control(struct tcsd_thread_data *thread_data)
{
	int i = 0;
	struct hostent *local_hostent = NULL;
	static char *localhostname = NULL;
	static int localhostname_len = 0;

	if (!localhostname) {
		if ((local_hostent = gethostbyname("localhost")) == NULL) {
			LogError("Error resolving localhost: %s", hstrerror(h_errno));
			return 1;
		}

		LogDebugFn("Cached local hostent:");
		LogDebugFn("h_name: %s", local_hostent->h_name);
		for (i = 0; local_hostent->h_aliases[i]; i++) {
			LogDebugFn("h_aliases[%d]: %s", i, local_hostent->h_aliases[i]);
		}
		LogDebugFn("h_addrtype: %s",
			   (local_hostent->h_addrtype == AF_INET6 ? "AF_INET6" : "AF_INET"));

		localhostname_len = strlen(local_hostent->h_name);
		if ((localhostname = strdup(local_hostent->h_name)) == NULL) {
			LogError("malloc of %d bytes failed.", localhostname_len);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
	}

	/* if the request comes from localhost, or is in the accepted ops list,
	 * approve it */
	if (!strncmp(thread_data->hostname, localhostname,
		     MIN((size_t)localhostname_len, strlen(thread_data->hostname)))) {
		return 0;
	} else {
		while (tcsd_options.remote_ops[i]) {
			if ((UINT32)tcsd_options.remote_ops[i] == thread_data->comm.hdr.u.ordinal) {
				LogInfo("Accepted %s operation from %s",
					tcs_func_table[thread_data->comm.hdr.u.ordinal].name,
					thread_data->hostname);
				return 0;
			}
			i++;
		}
	}

	return 1;
}

TSS_RESULT
dispatchCommand(struct tcsd_thread_data *data)
{
	UINT64 offset;
	TSS_RESULT result;

	/* First, check the ordinal bounds */
	if (data->comm.hdr.u.ordinal >= TCSD_MAX_NUM_ORDS) {
		LogError("Illegal TCSD Ordinal");
		return TCSERR(TSS_E_FAIL);
	}

	LogDebug("Dispatching ordinal %u", data->comm.hdr.u.ordinal);
	/* We only need to check access_control if there are remote operations that are defined
	 * in the config file, which means we allow remote connections */
	if (tcsd_options.remote_ops[0] && access_control(data)) {
		LogWarn("Denied %s operation from %s",
			tcs_func_table[data->comm.hdr.u.ordinal].name, data->hostname);

		/* set platform header */
		memset(&data->comm.hdr, 0, sizeof(data->comm.hdr));
		data->comm.hdr.packet_size = sizeof(struct tcsd_packet_hdr);
		data->comm.hdr.u.result = TCSERR(TSS_E_FAIL);

		/* set the comm buffer */
		memset(data->comm.buf, 0, data->comm.buf_size);
		offset = 0;
		LoadBlob_UINT32(&offset, data->comm.hdr.packet_size, data->comm.buf);
		LoadBlob_UINT32(&offset, data->comm.hdr.u.result, data->comm.buf);

		return TSS_SUCCESS;
	}

	/* Now, dispatch */
	if ((result = tcs_func_table[data->comm.hdr.u.ordinal].Func(data)) == TSS_SUCCESS) {
		/* set the comm buffer */
		offset = 0;
		LoadBlob_UINT32(&offset, data->comm.hdr.packet_size, data->comm.buf);
		LoadBlob_UINT32(&offset, data->comm.hdr.u.result, data->comm.buf);
		LoadBlob_UINT32(&offset, data->comm.hdr.num_parms, data->comm.buf);
		LoadBlob_UINT32(&offset, data->comm.hdr.type_size, data->comm.buf);
		LoadBlob_UINT32(&offset, data->comm.hdr.type_offset, data->comm.buf);
		LoadBlob_UINT32(&offset, data->comm.hdr.parm_size, data->comm.buf);
		LoadBlob_UINT32(&offset, data->comm.hdr.parm_offset, data->comm.buf);
	}

	return result;

}

TSS_RESULT
getTCSDPacket(struct tcsd_thread_data *data)
{
        /* make sure the all the data is present */
	if (data->comm.hdr.num_parms > 0 &&
	    data->comm.hdr.packet_size !=
		(UINT32)(data->comm.hdr.parm_offset + data->comm.hdr.parm_size))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	/* dispatch the command to the TCS */
	return dispatchCommand(data);
}
