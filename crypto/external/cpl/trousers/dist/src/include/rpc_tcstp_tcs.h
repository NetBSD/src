
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _RPC_TCSTP_TCS_H_
#define _RPC_TCSTP_TCS_H_

#include "rpc_tcstp.h"

#define DECLARE_TCSTP_FUNC(x) \
	TSS_RESULT tcs_wrap_##x(struct tcsd_thread_data *)
/* Auth session, context and TPM caps support are always compiled in. TPM caps
 * are necessary so that the TCSD can know what type of TPM its talking to */
DECLARE_TCSTP_FUNC(OpenContext);
DECLARE_TCSTP_FUNC(CloseContext);
DECLARE_TCSTP_FUNC(OIAP);
DECLARE_TCSTP_FUNC(OSAP);
DECLARE_TCSTP_FUNC(GetCapability);
DECLARE_TCSTP_FUNC(GetCapabilityOwner);
DECLARE_TCSTP_FUNC(SetCapability);

#ifdef TSS_BUILD_RANDOM
DECLARE_TCSTP_FUNC(GetRandom);
DECLARE_TCSTP_FUNC(StirRandom);
#else
#define tcs_wrap_GetRandom	tcs_wrap_Error
#define tcs_wrap_StirRandom	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_EK
DECLARE_TCSTP_FUNC(CreateEndorsementKeyPair);
DECLARE_TCSTP_FUNC(ReadPubek);
DECLARE_TCSTP_FUNC(OwnerReadPubek);
DECLARE_TCSTP_FUNC(DisablePubekRead);
#ifdef TSS_BUILD_TSS12
DECLARE_TCSTP_FUNC(CreateRevocableEndorsementKeyPair);
DECLARE_TCSTP_FUNC(RevokeEndorsementKeyPair);
#else
#define tcs_wrap_CreateRevocableEndorsementKeyPair	tcs_wrap_Error
#define tcs_wrap_RevokeEndorsementKeyPair		tcs_wrap_Error
#endif
#else
#define tcs_wrap_CreateEndorsementKeyPair		tcs_wrap_Error
#define tcs_wrap_ReadPubek				tcs_wrap_Error
#define tcs_wrap_OwnerReadPubek				tcs_wrap_Error
#define tcs_wrap_DisablePubekRead			tcs_wrap_Error
#define tcs_wrap_CreateRevocableEndorsementKeyPair	tcs_wrap_Error
#define tcs_wrap_RevokeEndorsementKeyPair		tcs_wrap_Error
#endif

#ifdef TSS_BUILD_KEY
DECLARE_TCSTP_FUNC(EvictKey);
DECLARE_TCSTP_FUNC(GetPubkey);
DECLARE_TCSTP_FUNC(TerminateHandle);
DECLARE_TCSTP_FUNC(LoadKeyByBlob);
DECLARE_TCSTP_FUNC(CreateWrapKey);
#ifdef TSS_BUILD_TSS12
DECLARE_TCSTP_FUNC(KeyControlOwner);
DECLARE_TCSTP_FUNC(OwnerReadInternalPub);
#else
#define tcs_wrap_KeyControlOwner	tcs_wrap_Error
#define tcs_wrap_OwnerReadInternalPub	tcs_wrap_Error
#endif
#else
#define tcs_wrap_EvictKey		tcs_wrap_Error
#define tcs_wrap_GetPubkey		tcs_wrap_Error
#define tcs_wrap_TerminateHandle	tcs_wrap_Error
#define tcs_wrap_LoadKeyByBlob		tcs_wrap_Error
#define tcs_wrap_CreateWrapKey		tcs_wrap_Error
#define tcs_wrap_KeyControlOwner	tcs_wrap_Error

#endif

#ifdef TSS_BUILD_PCR_EXTEND
DECLARE_TCSTP_FUNC(Extend);
DECLARE_TCSTP_FUNC(PcrRead);
DECLARE_TCSTP_FUNC(PcrReset);
#else
#define tcs_wrap_Extend		tcs_wrap_Error
#define tcs_wrap_PcrRead	tcs_wrap_Error
#define tcs_wrap_PcrReset	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_CAPS
DECLARE_TCSTP_FUNC(TCSGetCapability);
#else
#define tcs_wrap_TCSGetCapability	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_OWN
DECLARE_TCSTP_FUNC(TakeOwnership);
DECLARE_TCSTP_FUNC(OwnerClear);
#else
#define tcs_wrap_TakeOwnership	tcs_wrap_Error
#define tcs_wrap_OwnerClear	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_PS
DECLARE_TCSTP_FUNC(RegisterKey);
DECLARE_TCSTP_FUNC(UnregisterKey);
DECLARE_TCSTP_FUNC(GetRegisteredKeyBlob);
DECLARE_TCSTP_FUNC(LoadKeyByUUID);
DECLARE_TCSTP_FUNC(GetRegisteredKeyByPublicInfo);
DECLARE_TCSTP_FUNC(EnumRegisteredKeys);
DECLARE_TCSTP_FUNC(EnumRegisteredKeys2);
#else
#define tcs_wrap_RegisterKey			tcs_wrap_Error
#define tcs_wrap_UnregisterKey			tcs_wrap_Error
#define tcs_wrap_GetRegisteredKeyBlob		tcs_wrap_Error
#define tcs_wrap_LoadKeyByUUID			tcs_wrap_Error
#define tcs_wrap_GetRegisteredKeyByPublicInfo	tcs_wrap_Error
#define tcs_wrap_EnumRegisteredKeys		tcs_wrap_Error
#define tcs_wrap_EnumRegisteredKeys2	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_SIGN
DECLARE_TCSTP_FUNC(Sign);
#else
#define tcs_wrap_Sign	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_DIR
DECLARE_TCSTP_FUNC(DirWriteAuth);
DECLARE_TCSTP_FUNC(DirRead);
#else
#define tcs_wrap_DirWriteAuth	tcs_wrap_Error
#define tcs_wrap_DirRead	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_SEAL
DECLARE_TCSTP_FUNC(Seal);
DECLARE_TCSTP_FUNC(UnSeal);
#else
#define tcs_wrap_Seal	tcs_wrap_Error
#define tcs_wrap_UnSeal	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_SEALX
DECLARE_TCSTP_FUNC(Sealx);
#else
#define tcs_wrap_Sealx	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_BIND
DECLARE_TCSTP_FUNC(UnBind);
#else
#define tcs_wrap_UnBind	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_CHANGEAUTH
DECLARE_TCSTP_FUNC(ChangeAuth);
DECLARE_TCSTP_FUNC(ChangeAuthOwner);
#else
#define tcs_wrap_ChangeAuth		tcs_wrap_Error
#define tcs_wrap_ChangeAuthOwner	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_QUOTE
DECLARE_TCSTP_FUNC(Quote);
#else
#define tcs_wrap_Quote	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_QUOTE2
DECLARE_TCSTP_FUNC(Quote2);
#else
#define tcs_wrap_Quote2		tcs_wrap_Error
#endif

#ifdef TSS_BUILD_PCR_EVENTS
DECLARE_TCSTP_FUNC(LogPcrEvent);
DECLARE_TCSTP_FUNC(GetPcrEvent);
DECLARE_TCSTP_FUNC(GetPcrEventsByPcr);
DECLARE_TCSTP_FUNC(GetPcrEventLog);
#else
#define tcs_wrap_LogPcrEvent		tcs_wrap_Error
#define tcs_wrap_GetPcrEvent		tcs_wrap_Error
#define tcs_wrap_GetPcrEventsByPcr	tcs_wrap_Error
#define tcs_wrap_GetPcrEventLog		tcs_wrap_Error
#endif

#ifdef TSS_BUILD_SELFTEST
DECLARE_TCSTP_FUNC(SelfTestFull);
DECLARE_TCSTP_FUNC(CertifySelfTest);
DECLARE_TCSTP_FUNC(GetTestResult);
#else
#define tcs_wrap_SelfTestFull		tcs_wrap_Error
#define tcs_wrap_CertifySelfTest	tcs_wrap_Error
#define tcs_wrap_GetTestResult		tcs_wrap_Error
#endif

#ifdef TSS_BUILD_ADMIN
DECLARE_TCSTP_FUNC(DisableOwnerClear);
DECLARE_TCSTP_FUNC(ForceClear);
DECLARE_TCSTP_FUNC(DisableForceClear);
DECLARE_TCSTP_FUNC(PhysicalEnable);
DECLARE_TCSTP_FUNC(PhysicalSetDeactivated);
DECLARE_TCSTP_FUNC(SetOwnerInstall);
DECLARE_TCSTP_FUNC(OwnerSetDisable);
DECLARE_TCSTP_FUNC(PhysicalDisable);
DECLARE_TCSTP_FUNC(PhysicalPresence);
DECLARE_TCSTP_FUNC(SetTempDeactivated);
#ifdef TSS_BUILD_TSS12
DECLARE_TCSTP_FUNC(SetTempDeactivated2);
DECLARE_TCSTP_FUNC(ResetLockValue);
#else
#define tcs_wrap_SetTempDeactivated2	tcs_wrap_Error
#define tcs_wrap_ResetLockValue		tcs_wrap_Error
#endif
#else
#define tcs_wrap_DisableOwnerClear	tcs_wrap_Error
#define tcs_wrap_ForceClear		tcs_wrap_Error
#define tcs_wrap_DisableForceClear	tcs_wrap_Error
#define tcs_wrap_PhysicalEnable		tcs_wrap_Error
#define tcs_wrap_PhysicalSetDeactivated	tcs_wrap_Error
#define tcs_wrap_SetOwnerInstall	tcs_wrap_Error
#define tcs_wrap_OwnerSetDisable	tcs_wrap_Error
#define tcs_wrap_PhysicalDisable	tcs_wrap_Error
#define tcs_wrap_PhysicalPresence	tcs_wrap_Error
#define tcs_wrap_SetTempDeactivated	tcs_wrap_Error
#define tcs_wrap_SetTempDeactivated2	tcs_wrap_Error
#define tcs_wrap_ResetLockValue		tcs_wrap_Error
#endif

#ifdef TSS_BUILD_CERTIFY
DECLARE_TCSTP_FUNC(CertifyKey);
#else
#define tcs_wrap_CertifyKey	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_AIK
DECLARE_TCSTP_FUNC(MakeIdentity);
DECLARE_TCSTP_FUNC(ActivateIdentity);
#ifdef TSS_BUILD_TSS12
DECLARE_TCSTP_FUNC(GetCredential);
#else
#define tcs_wrap_GetCredential		tcs_wrap_Error
#endif
#else
#define tcs_wrap_MakeIdentity		tcs_wrap_Error
#define tcs_wrap_ActivateIdentity	tcs_wrap_Error
#define tcs_wrap_GetCredential		tcs_wrap_Error
#endif

#ifdef TSS_BUILD_MIGRATION
DECLARE_TCSTP_FUNC(CreateMigrationBlob);
DECLARE_TCSTP_FUNC(ConvertMigrationBlob);
DECLARE_TCSTP_FUNC(AuthorizeMigrationKey);
#else
#define tcs_wrap_CreateMigrationBlob	tcs_wrap_Error
#define tcs_wrap_ConvertMigrationBlob	tcs_wrap_Error
#define tcs_wrap_AuthorizeMigrationKey	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_MAINT
DECLARE_TCSTP_FUNC(KillMaintenanceFeature);
DECLARE_TCSTP_FUNC(CreateMaintenanceArchive);
DECLARE_TCSTP_FUNC(LoadMaintenanceArchive);
DECLARE_TCSTP_FUNC(LoadManuMaintPub);
DECLARE_TCSTP_FUNC(ReadManuMaintPub);
#else
#define tcs_wrap_KillMaintenanceFeature		tcs_wrap_Error
#define tcs_wrap_CreateMaintenanceArchive	tcs_wrap_Error
#define tcs_wrap_LoadMaintenanceArchive		tcs_wrap_Error
#define tcs_wrap_LoadManuMaintPub		tcs_wrap_Error
#define tcs_wrap_ReadManuMaintPub		tcs_wrap_Error
#endif

#ifdef TSS_BUILD_DAA
DECLARE_TCSTP_FUNC(DaaJoin);
DECLARE_TCSTP_FUNC(DaaSign);
#else
#define tcs_wrap_DaaJoin	tcs_wrap_Error
#define tcs_wrap_DaaSign	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_NV
DECLARE_TCSTP_FUNC(NV_DefineOrReleaseSpace);
DECLARE_TCSTP_FUNC(NV_WriteValue);
DECLARE_TCSTP_FUNC(NV_WriteValueAuth);
DECLARE_TCSTP_FUNC(NV_ReadValue);
DECLARE_TCSTP_FUNC(NV_ReadValueAuth);
#else
#define tcs_wrap_NV_DefineOrReleaseSpace      tcs_wrap_Error
#define tcs_wrap_NV_WriteValue                tcs_wrap_Error
#define tcs_wrap_NV_WriteValueAuth            tcs_wrap_Error
#define tcs_wrap_NV_ReadValue                 tcs_wrap_Error
#define tcs_wrap_NV_ReadValueAuth             tcs_wrap_Error
#endif

#ifdef TSS_BUILD_COUNTER
DECLARE_TCSTP_FUNC(ReadCounter);
DECLARE_TCSTP_FUNC(CreateCounter);
DECLARE_TCSTP_FUNC(IncrementCounter);
DECLARE_TCSTP_FUNC(ReleaseCounter);
DECLARE_TCSTP_FUNC(ReleaseCounterOwner);
#else
#define tcs_wrap_ReadCounter		tcs_wrap_Error
#define tcs_wrap_CreateCounter		tcs_wrap_Error
#define tcs_wrap_IncrementCounter	tcs_wrap_Error
#define tcs_wrap_ReleaseCounter		tcs_wrap_Error
#define tcs_wrap_ReleaseCounterOwner	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_TICK
DECLARE_TCSTP_FUNC(ReadCurrentTicks);
DECLARE_TCSTP_FUNC(TickStampBlob);
#else
#define tcs_wrap_ReadCurrentTicks	tcs_wrap_Error
#define tcs_wrap_TickStampBlob		tcs_wrap_Error
#endif

#ifdef TSS_BUILD_TRANSPORT
DECLARE_TCSTP_FUNC(EstablishTransport);
DECLARE_TCSTP_FUNC(ExecuteTransport);
DECLARE_TCSTP_FUNC(ReleaseTransportSigned);
#else
#define tcs_wrap_EstablishTransport	tcs_wrap_Error
#define tcs_wrap_ExecuteTransport	tcs_wrap_Error
#define tcs_wrap_ReleaseTransportSigned	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_AUDIT
DECLARE_TCSTP_FUNC(SetOrdinalAuditStatus);
DECLARE_TCSTP_FUNC(GetAuditDigest);
DECLARE_TCSTP_FUNC(GetAuditDigestSigned);
#else
#define tcs_wrap_SetOrdinalAuditStatus	tcs_wrap_Error
#define tcs_wrap_GetAuditDigest		tcs_wrap_Error
#define tcs_wrap_GetAuditDigestSigned	tcs_wrap_Error
#endif

#ifdef TSS_BUILD_TSS12
DECLARE_TCSTP_FUNC(SetOperatorAuth);
DECLARE_TCSTP_FUNC(FlushSpecific);
#else
#define tcs_wrap_SetOperatorAuth	tcs_wrap_Error
#define tcs_wrap_FlushSpecific		tcs_wrap_Error
#endif

#ifdef TSS_BUILD_DELEGATION
DECLARE_TCSTP_FUNC(Delegate_Manage);
DECLARE_TCSTP_FUNC(Delegate_CreateKeyDelegation);
DECLARE_TCSTP_FUNC(Delegate_CreateOwnerDelegation);
DECLARE_TCSTP_FUNC(Delegate_LoadOwnerDelegation);
DECLARE_TCSTP_FUNC(Delegate_ReadTable);
DECLARE_TCSTP_FUNC(Delegate_UpdateVerificationCount);
DECLARE_TCSTP_FUNC(Delegate_VerifyDelegation);
DECLARE_TCSTP_FUNC(DSAP);
#else
#define tcs_wrap_Delegate_Manage			tcs_wrap_Error
#define tcs_wrap_Delegate_CreateKeyDelegation		tcs_wrap_Error
#define tcs_wrap_Delegate_CreateOwnerDelegation		tcs_wrap_Error
#define tcs_wrap_Delegate_LoadOwnerDelegation		tcs_wrap_Error
#define tcs_wrap_Delegate_ReadTable			tcs_wrap_Error
#define tcs_wrap_Delegate_UpdateVerificationCount	tcs_wrap_Error
#define tcs_wrap_Delegate_VerifyDelegation		tcs_wrap_Error
#define tcs_wrap_DSAP					tcs_wrap_Error
#endif

#ifdef TSS_BUILD_CMK
DECLARE_TCSTP_FUNC(CMK_SetRestrictions);
DECLARE_TCSTP_FUNC(CMK_ApproveMA);
DECLARE_TCSTP_FUNC(CMK_CreateKey);
DECLARE_TCSTP_FUNC(CMK_CreateTicket);
DECLARE_TCSTP_FUNC(CMK_CreateBlob);
DECLARE_TCSTP_FUNC(CMK_ConvertMigration);
#else
#define tcs_wrap_CMK_SetRestrictions	tcs_wrap_Error
#define tcs_wrap_CMK_ApproveMA		tcs_wrap_Error
#define tcs_wrap_CMK_CreateKey		tcs_wrap_Error
#define tcs_wrap_CMK_CreateTicket	tcs_wrap_Error
#define tcs_wrap_CMK_CreateBlob		tcs_wrap_Error
#define tcs_wrap_CMK_ConvertMigration	tcs_wrap_Error
#endif

DECLARE_TCSTP_FUNC(dispatchCommand);

void LoadBlob_Auth_Special(UINT64 *, BYTE *, TPM_AUTH *);
void UnloadBlob_Auth_Special(UINT64 *, BYTE *, TPM_AUTH *);
void LoadBlob_KM_KEYINFO(UINT64 *, BYTE *, TSS_KM_KEYINFO *);
void LoadBlob_KM_KEYINFO2(UINT64 *, BYTE *, TSS_KM_KEYINFO2 *);
void UnloadBlob_KM_KEYINFO(UINT64 *, BYTE *, TSS_KM_KEYINFO *);
void UnloadBlob_KM_KEYINFO2(UINT64 *, BYTE *, TSS_KM_KEYINFO2 *);
void LoadBlob_LOADKEY_INFO(UINT64 *, BYTE *, TCS_LOADKEY_INFO *);
void UnloadBlob_LOADKEY_INFO(UINT64 *, BYTE *, TCS_LOADKEY_INFO *);
void LoadBlob_PCR_EVENT(UINT64 *, BYTE *, TSS_PCR_EVENT *);
TSS_RESULT UnloadBlob_PCR_EVENT(UINT64 *, BYTE *, TSS_PCR_EVENT *);
int setData(TCSD_PACKET_TYPE, int, void *, int, struct tcsd_comm_data *);
UINT32 getData(TCSD_PACKET_TYPE, int, void *, int, struct tcsd_comm_data *);
void initData(struct tcsd_comm_data *, int);
int recv_from_socket(int, void *, int);
int send_to_socket(int, void *, int);
TSS_RESULT getTCSDPacket(struct tcsd_thread_data *);

MUTEX_DECLARE_EXTERN(tcsp_lock);

#endif


