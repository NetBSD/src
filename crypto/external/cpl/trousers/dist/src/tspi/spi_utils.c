
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_UUID NULL_UUID = { 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0, 0 } };

TSS_VERSION VERSION_1_1 = { 1, 1, 0, 0 };

struct tcs_api_table tcs_normal_api = {
#ifdef TSS_BUILD_KEY
	.LoadKeyByBlob = RPC_LoadKeyByBlob,
	.EvictKey = RPC_EvictKey,
	.CreateWrapKey = RPC_CreateWrapKey,
	.GetPubKey = RPC_GetPubKey,
#ifdef TSS_BUILD_TSS12
	.OwnerReadInternalPub = RPC_OwnerReadInternalPub,
#endif
#ifdef TSS_BUILD_CERTIFY
	.CertifyKey = RPC_CertifyKey,
#endif
#endif
#ifdef TSS_BUILD_OWN
	.OwnerClear = RPC_OwnerClear,
	.ForceClear = RPC_ForceClear,
#endif
#ifdef TSS_BUILD_AUTH
	.TerminateHandle = RPC_TerminateHandle,
	.OIAP = RPC_OIAP,
	.OSAP = RPC_OSAP,
#endif
#ifdef TSS_BUILD_CHANGEAUTH
	.ChangeAuth = RPC_ChangeAuth,
	.ChangeAuthOwner = RPC_ChangeAuthOwner,
	.ChangeAuthAsymStart = RPC_ChangeAuthAsymStart,
	.ChangeAuthAsymFinish = RPC_ChangeAuthAsymFinish,
#endif
#ifdef TSS_BUILD_AIK
	.ActivateTPMIdentity = RPC_ActivateTPMIdentity,
#endif
#ifdef TSS_BUILD_PCR_EXTEND
	.Extend = RPC_Extend,
	.PcrRead = RPC_PcrRead,
	.PcrReset = RPC_PcrReset,
#endif
#ifdef TSS_BUILD_QUOTE
	.Quote = RPC_Quote,
#endif
#ifdef TSS_BUILD_QUOTE2
	.Quote2 = RPC_Quote2,
#endif
#ifdef TSS_BUILD_DIR
	.DirWriteAuth = RPC_DirWriteAuth,
	.DirRead = RPC_DirRead,
#endif
#ifdef TSS_BUILD_SEAL
	.Seal = RPC_Seal,
	.Unseal = RPC_Unseal,
#ifdef TSS_BUILD_SEALX
	.Sealx = RPC_Sealx,
#endif
#endif
#ifdef TSS_BUILD_BIND
	.UnBind = RPC_UnBind,
#endif
#ifdef TSS_BUILD_MIGRATION
	.CreateMigrationBlob = RPC_CreateMigrationBlob,
	.ConvertMigrationBlob = RPC_ConvertMigrationBlob,
	.AuthorizeMigrationKey = RPC_AuthorizeMigrationKey,
#endif
#ifdef TSS_BUILD_SIGN
	.Sign = RPC_Sign,
#endif
#ifdef TSS_BUILD_RANDOM
	.GetRandom = RPC_GetRandom,
	.StirRandom = RPC_StirRandom,
#endif
#ifdef TSS_BUILD_CAPS_TPM
	.GetTPMCapability = RPC_GetTPMCapability,
	.SetCapability = RPC_SetCapability,
	.GetCapabilityOwner = RPC_GetCapabilityOwner,
#endif
#ifdef TSS_BUILD_EK
	.CreateEndorsementKeyPair = RPC_CreateEndorsementKeyPair,
	.ReadPubek = RPC_ReadPubek,
	.OwnerReadPubek = RPC_OwnerReadPubek,
#endif
#ifdef TSS_BUILD_SELFTEST
	.SelfTestFull = RPC_SelfTestFull,
	.CertifySelfTest = RPC_CertifySelfTest,
	.GetTestResult = RPC_GetTestResult,
#endif
#ifdef TSS_BUILD_ADMIN
	.SetOwnerInstall = RPC_SetOwnerInstall,
	.DisablePubekRead = RPC_DisablePubekRead,
	.OwnerSetDisable = RPC_OwnerSetDisable,
	.DisableOwnerClear = RPC_DisableOwnerClear,
	.DisableForceClear = RPC_DisableForceClear,
	.PhysicalDisable = RPC_PhysicalDisable,
	.PhysicalEnable = RPC_PhysicalEnable,
	.PhysicalSetDeactivated = RPC_PhysicalSetDeactivated,
	.PhysicalPresence = RPC_PhysicalPresence,
	.SetTempDeactivated = RPC_SetTempDeactivated,
#ifdef TSS_BUILD_TSS12
	.SetTempDeactivated2 = RPC_SetTempDeactivated2,
	.ResetLockValue = RPC_ResetLockValue,
#endif
#endif
#ifdef TSS_BUILD_MAINT
	.CreateMaintenanceArchive = RPC_CreateMaintenanceArchive,
	.LoadMaintenanceArchive = RPC_LoadMaintenanceArchive,
	.KillMaintenanceFeature = RPC_KillMaintenanceFeature,
	.LoadManuMaintPub = RPC_LoadManuMaintPub,
	.ReadManuMaintPub = RPC_ReadManuMaintPub,
#endif
#ifdef TSS_BUILD_DAA
	.DaaJoin = RPC_DaaJoin,
	.DaaSign = RPC_DaaSign,
#endif
#ifdef TSS_BUILD_COUNTER
	.ReadCounter = RPC_ReadCounter,
	.CreateCounter = RPC_CreateCounter,
	.IncrementCounter = RPC_IncrementCounter,
	.ReleaseCounter = RPC_ReleaseCounter,
	.ReleaseCounterOwner = RPC_ReleaseCounterOwner,
#endif
#ifdef TSS_BUILD_TICK
	.ReadCurrentTicks = RPC_ReadCurrentTicks,
	.TickStampBlob = RPC_TickStampBlob,
#endif
#ifdef TSS_BUILD_NV
	.NV_DefineOrReleaseSpace = RPC_NV_DefineOrReleaseSpace,
	.NV_WriteValue = RPC_NV_WriteValue,
	.NV_WriteValueAuth = RPC_NV_WriteValueAuth,
	.NV_ReadValue = RPC_NV_ReadValue,
	.NV_ReadValueAuth = RPC_NV_ReadValueAuth,
#endif
#ifdef TSS_BUILD_AUDIT
	.SetOrdinalAuditStatus = RPC_SetOrdinalAuditStatus,
	.GetAuditDigest = RPC_GetAuditDigest,
	.GetAuditDigestSigned = RPC_GetAuditDigestSigned,
#endif
#ifdef TSS_BUILD_TSS12
	.SetOperatorAuth = RPC_SetOperatorAuth,
	.FlushSpecific = RPC_FlushSpecific,
#endif
#ifdef TSS_BUILD_DELEGATION
	.Delegate_Manage = RPC_Delegate_Manage,
	.Delegate_CreateKeyDelegation = RPC_Delegate_CreateKeyDelegation,
	.Delegate_CreateOwnerDelegation = RPC_Delegate_CreateOwnerDelegation,
	.Delegate_LoadOwnerDelegation = RPC_Delegate_LoadOwnerDelegation,
	.Delegate_ReadTable = RPC_Delegate_ReadTable,
	.Delegate_UpdateVerificationCount = RPC_Delegate_UpdateVerificationCount,
	.Delegate_VerifyDelegation = RPC_Delegate_VerifyDelegation,
	.DSAP = RPC_DSAP,
#endif
	.FieldUpgrade = RPC_FieldUpgrade,
	.SetRedirection = RPC_SetRedirection,
};

#ifdef TSS_BUILD_TRANSPORT
struct tcs_api_table tcs_transport_api = {
#ifdef TSS_BUILD_KEY
	.LoadKeyByBlob = Transport_LoadKeyByBlob,
	.EvictKey = Transport_EvictKey,
	.CreateWrapKey = Transport_CreateWrapKey,
	.GetPubKey = Transport_GetPubKey,
#ifdef TSS_BUILD_TSS12
	.OwnerReadInternalPub = Transport_OwnerReadInternalPub,
#endif
#ifdef TSS_BUILD_CERTIFY
	.CertifyKey = Transport_CertifyKey,
#endif
#endif
#ifdef TSS_BUILD_OWN
	.OwnerClear = Transport_OwnerClear,
	.ForceClear = Transport_ForceClear,
#endif
#ifdef TSS_BUILD_AUTH
	.OIAP = Transport_OIAP,
	.OSAP = Transport_OSAP,
	.TerminateHandle = Transport_TerminateHandle,
#endif
#ifdef TSS_BUILD_CHANGEAUTH
	.ChangeAuth = Transport_ChangeAuth,
	.ChangeAuthOwner = Transport_ChangeAuthOwner,
	.ChangeAuthAsymStart = RPC_ChangeAuthAsymStart,
	.ChangeAuthAsymFinish = RPC_ChangeAuthAsymFinish,
#endif
#ifdef TSS_BUILD_AIK
	.ActivateTPMIdentity = Transport_ActivateTPMIdentity,
#endif
#ifdef TSS_BUILD_PCR_EXTEND
	.Extend = Transport_Extend,
	.PcrRead = Transport_PcrRead,
	.PcrReset = Transport_PcrReset,
#endif
#ifdef TSS_BUILD_QUOTE
	.Quote = Transport_Quote,
#endif
#ifdef TSS_BUILD_QUOTE2
	.Quote2 = Transport_Quote2,
#endif
#ifdef TSS_BUILD_DIR
	.DirWriteAuth = Transport_DirWriteAuth,
	.DirRead = Transport_DirRead,
#endif
#ifdef TSS_BUILD_SEAL
	.Seal = Transport_Seal,
	.Sealx = Transport_Sealx,
	.Unseal = Transport_Unseal,
#endif
#ifdef TSS_BUILD_BIND
	.UnBind = Transport_UnBind,
#endif
#ifdef TSS_BUILD_MIGRATION
	.CreateMigrationBlob = Transport_CreateMigrationBlob,
	.ConvertMigrationBlob = Transport_ConvertMigrationBlob,
	.AuthorizeMigrationKey = Transport_AuthorizeMigrationKey,
#endif
#ifdef TSS_BUILD_SIGN
	.Sign = Transport_Sign,
#endif
#ifdef TSS_BUILD_RANDOM
	.GetRandom = Transport_GetRandom,
	.StirRandom = Transport_StirRandom,
#endif
#ifdef TSS_BUILD_CAPS_TPM
	.GetTPMCapability = Transport_GetTPMCapability,
	.SetCapability = Transport_SetCapability,
	.GetCapabilityOwner = Transport_GetCapabilityOwner,
#endif
#ifdef TSS_BUILD_EK
	.ReadPubek = RPC_ReadPubek,
	.OwnerReadPubek = RPC_OwnerReadPubek,
#endif
#ifdef TSS_BUILD_SELFTEST
	.SelfTestFull = Transport_SelfTestFull,
	.CertifySelfTest = Transport_CertifySelfTest,
	.GetTestResult = Transport_GetTestResult,
#endif
#ifdef TSS_BUILD_ADMIN
	.SetOwnerInstall = Transport_SetOwnerInstall,
	.DisablePubekRead = Transport_DisablePubekRead,
	.OwnerSetDisable = Transport_OwnerSetDisable,
	.ResetLockValue = Transport_ResetLockValue,
	.DisableOwnerClear = Transport_DisableOwnerClear,
	.DisableForceClear = Transport_DisableForceClear,
	.PhysicalDisable = Transport_PhysicalDisable,
	.PhysicalEnable = Transport_PhysicalEnable,
	.PhysicalSetDeactivated = Transport_PhysicalSetDeactivated,
	.PhysicalPresence = Transport_PhysicalPresence,
	.SetTempDeactivated = Transport_SetTempDeactivated,
	.SetTempDeactivated2 = Transport_SetTempDeactivated2,
#endif
#ifdef TSS_BUILD_MAINT
	.CreateMaintenanceArchive = Transport_CreateMaintenanceArchive,
	.LoadMaintenanceArchive = Transport_LoadMaintenanceArchive,
	.KillMaintenanceFeature = Transport_KillMaintenanceFeature,
	.LoadManuMaintPub = Transport_LoadManuMaintPub,
	.ReadManuMaintPub = Transport_ReadManuMaintPub,
#endif
#ifdef TSS_BUILD_DAA
	.DaaJoin = RPC_DaaJoin,
	.DaaSign = RPC_DaaSign,
#endif
#ifdef TSS_BUILD_COUNTER
	.ReadCounter = Transport_ReadCounter,
	.CreateCounter = RPC_CreateCounter,
	.IncrementCounter = RPC_IncrementCounter,
	.ReleaseCounter = RPC_ReleaseCounter,
	.ReleaseCounterOwner = RPC_ReleaseCounterOwner,
#endif
#ifdef TSS_BUILD_TICK
	.ReadCurrentTicks = Transport_ReadCurrentTicks,
	.TickStampBlob = Transport_TickStampBlob,
#endif
#ifdef TSS_BUILD_NV
	.NV_DefineOrReleaseSpace = Transport_NV_DefineOrReleaseSpace,
	.NV_WriteValue = Transport_NV_WriteValue,
	.NV_WriteValueAuth = Transport_NV_WriteValueAuth,
	.NV_ReadValue = Transport_NV_ReadValue,
	.NV_ReadValueAuth = Transport_NV_ReadValueAuth,
#endif
#ifdef TSS_BUILD_AUDIT
	.SetOrdinalAuditStatus = Transport_SetOrdinalAuditStatus,
	.GetAuditDigest = Transport_GetAuditDigest,
	.GetAuditDigestSigned = Transport_GetAuditDigestSigned,
#endif
#ifdef TSS_BUILD_TSS12
	.SetOperatorAuth = Transport_SetOperatorAuth,
	.FlushSpecific = Transport_FlushSpecific,
#endif
#ifdef TSS_BUILD_DELEGATION
	.Delegate_Manage = Transport_Delegate_Manage,
	.Delegate_CreateKeyDelegation = Transport_Delegate_CreateKeyDelegation,
	.Delegate_CreateOwnerDelegation = Transport_Delegate_CreateOwnerDelegation,
	.Delegate_LoadOwnerDelegation = Transport_Delegate_LoadOwnerDelegation,
	.Delegate_ReadTable = Transport_Delegate_ReadTable,
	.Delegate_UpdateVerificationCount = Transport_Delegate_UpdateVerificationCount,
	.Delegate_VerifyDelegation = Transport_Delegate_VerifyDelegation,
	.DSAP = Transport_DSAP,
#endif
	.FieldUpgrade = RPC_FieldUpgrade,
	.SetRedirection = RPC_SetRedirection,
};
#endif

UINT16
Decode_UINT16(BYTE * in)
{
	UINT16 temp = 0;
	temp = (in[1] & 0xFF);
	temp |= (in[0] << 8);
	return temp;
}

void
UINT32ToArray(UINT32 i, BYTE * out)
{
	out[0] = (BYTE) ((i >> 24) & 0xFF);
	out[1] = (BYTE) ((i >> 16) & 0xFF);
	out[2] = (BYTE) ((i >> 8) & 0xFF);
	out[3] = (BYTE) i & 0xFF;
}

void
UINT64ToArray(UINT64 i, BYTE *out)
{
	out[0] = (BYTE) ((i >> 56) & 0xFF);
	out[1] = (BYTE) ((i >> 48) & 0xFF);
	out[2] = (BYTE) ((i >> 40) & 0xFF);
	out[3] = (BYTE) ((i >> 32) & 0xFF);
	out[4] = (BYTE) ((i >> 24) & 0xFF);
	out[5] = (BYTE) ((i >> 16) & 0xFF);
	out[6] = (BYTE) ((i >> 8) & 0xFF);
	out[7] = (BYTE) i & 0xFF;
}

void
UINT16ToArray(UINT16 i, BYTE * out)
{
	out[0] = ((i >> 8) & 0xFF);
	out[1] = i & 0xFF;
}

UINT64
Decode_UINT64(BYTE *y)
{
	UINT64 x = 0;

	x = y[0];
	x = ((x << 8) | (y[1] & 0xFF));
	x = ((x << 8) | (y[2] & 0xFF));
	x = ((x << 8) | (y[3] & 0xFF));
	x = ((x << 8) | (y[4] & 0xFF));
	x = ((x << 8) | (y[5] & 0xFF));
	x = ((x << 8) | (y[6] & 0xFF));
	x = ((x << 8) | (y[7] & 0xFF));

	return x;
}

UINT32
Decode_UINT32(BYTE * y)
{
	UINT32 x = 0;

	x = y[0];
	x = ((x << 8) | (y[1] & 0xFF));
	x = ((x << 8) | (y[2] & 0xFF));
	x = ((x << 8) | (y[3] & 0xFF));

	return x;
}

UINT32
get_pcr_event_size(TSS_PCR_EVENT *e)
{
	return (sizeof(TSS_PCR_EVENT) + e->ulEventLength + e->ulPcrValueLength);
}

void
LoadBlob_AUTH(UINT64 *offset, BYTE *blob, TPM_AUTH *auth)
{
	Trspi_LoadBlob_UINT32(offset, auth->AuthHandle, blob);
	Trspi_LoadBlob(offset, 20, blob, auth->NonceOdd.nonce);
	Trspi_LoadBlob_BOOL(offset, auth->fContinueAuthSession, blob);
	Trspi_LoadBlob(offset, 20, blob, (BYTE *)&auth->HMAC);
}

void
UnloadBlob_AUTH(UINT64 *offset, BYTE *blob, TPM_AUTH *auth)
{
	Trspi_UnloadBlob(offset, 20, blob, auth->NonceEven.nonce);
	Trspi_UnloadBlob_BOOL(offset, &auth->fContinueAuthSession, blob);
	Trspi_UnloadBlob(offset, 20, blob, (BYTE *)&auth->HMAC);
}

/* If alloc is true, we allocate a new buffer for the bytes and set *data to that.
 * If alloc is false, data is really a BYTE*, so write the bytes directly to that buffer */
TSS_RESULT
get_local_random(TSS_HCONTEXT tspContext, TSS_BOOL alloc, UINT32 size, BYTE **data)
{
	FILE *f = NULL;
	BYTE *buf = NULL;

	f = fopen(TSS_LOCAL_RANDOM_DEVICE, "r");
	if (f == NULL) {
		LogError("open of %s failed: %s", TSS_LOCAL_RANDOM_DEVICE, strerror(errno));
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if (alloc) {
		buf = calloc_tspi(tspContext, size);
		if (buf == NULL) {
			LogError("malloc of %u bytes failed", size);
			fclose(f);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
	} else
		buf = (BYTE *)data;

	if (fread(buf, size, 1, f) == 0) {
		LogError("fread of %s failed: %s", TSS_LOCAL_RANDOM_DEVICE, strerror(errno));
		fclose(f);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if (alloc)
		*data = buf;
	fclose(f);

	return TSS_SUCCESS;
}
