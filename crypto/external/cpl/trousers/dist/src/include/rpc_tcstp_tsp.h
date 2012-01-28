
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _RPC_TCSTP_TSP_H_
#define _RPC_TCSTP_TSP_H_

#include "hosttable.h"
#include "rpc_tcstp.h"
#include "tcsd_wrap.h"
#include "tcsd.h"

int setData(TCSD_PACKET_TYPE,int,void *,int,struct tcsd_comm_data *);
UINT32 getData(TCSD_PACKET_TYPE,int,void *,int,struct tcsd_comm_data *);
void initData(struct tcsd_comm_data *, int);
TSS_RESULT sendTCSDPacket(struct host_table_entry *);
TSS_RESULT send_init(struct host_table_entry *);
TSS_RESULT tcs_sendit(struct host_table_entry *);
short get_port();

/* Context commands always included */
TSS_RESULT RPC_OpenContext_TP(struct host_table_entry *, UINT32 *, TCS_CONTEXT_HANDLE *);
TSS_RESULT RPC_CloseContext_TP(struct host_table_entry *);
TSS_RESULT RPC_FreeMemory_TP(struct host_table_entry *,BYTE *);

#ifdef TSS_BUILD_AUTH
TSS_RESULT RPC_OIAP_TP(struct host_table_entry *,TCS_AUTHHANDLE *,TCPA_NONCE *);
TSS_RESULT RPC_OSAP_TP(struct host_table_entry *,TCPA_ENTITY_TYPE,UINT32,TCPA_NONCE*,TCS_AUTHHANDLE *,TCPA_NONCE *,TCPA_NONCE *);
#else
#define RPC_OIAP_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_OSAP_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_PCR_EVENTS
TSS_RESULT RPC_LogPcrEvent_TP(struct host_table_entry *,TSS_PCR_EVENT,UINT32 *);
TSS_RESULT RPC_GetPcrEvent_TP(struct host_table_entry *,UINT32,UINT32 *,TSS_PCR_EVENT **);
TSS_RESULT RPC_GetPcrEventLog_TP(struct host_table_entry *,UINT32 *,TSS_PCR_EVENT **);
TSS_RESULT RPC_GetPcrEventsByPcr_TP(struct host_table_entry *,UINT32,UINT32,UINT32 *,TSS_PCR_EVENT **);
#else
#define RPC_LogPcrEvent_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetPcrEvent_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetPcrEventLog_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetPcrEventsByPcr_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_PS
TSS_RESULT RPC_GetRegisteredKeyByPublicInfo_TP(struct host_table_entry * tcsContext,TCPA_ALGORITHM_ID algID,UINT32,BYTE *,UINT32 *,BYTE **);
TSS_RESULT RPC_RegisterKey_TP(struct host_table_entry *,TSS_UUID,TSS_UUID,UINT32,BYTE *,UINT32,BYTE *);
TSS_RESULT RPC_UnregisterKey_TP(struct host_table_entry *,TSS_UUID);
TSS_RESULT RPC_EnumRegisteredKeys_TP(struct host_table_entry *,TSS_UUID *,UINT32 *,TSS_KM_KEYINFO **);
TSS_RESULT RPC_EnumRegisteredKeys2_TP(struct host_table_entry *,TSS_UUID *,UINT32 *,TSS_KM_KEYINFO2 **);
TSS_RESULT RPC_GetRegisteredKey_TP(struct host_table_entry *,TSS_UUID,TSS_KM_KEYINFO **);
TSS_RESULT RPC_GetRegisteredKeyBlob_TP(struct host_table_entry *,TSS_UUID,UINT32 *,BYTE **);
TSS_RESULT RPC_LoadKeyByUUID_TP(struct host_table_entry *,TSS_UUID,TCS_LOADKEY_INFO *,TCS_KEY_HANDLE *);
#else
#define RPC_GetRegisteredKeyByPublicInfo_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_RegisterKey_TP(...)				TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_UnregisterKey_TP(...)			TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_EnumRegisteredKeys_TP(...)			TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_EnumRegisteredKeys2_TP(...)			TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetRegisteredKey_TP(...)			TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetRegisteredKeyBlob_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_LoadKeyByUUID_TP(...)			TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_KEY
TSS_RESULT RPC_LoadKeyByBlob_TP(struct host_table_entry *,TCS_KEY_HANDLE,UINT32,BYTE *,TPM_AUTH *,TCS_KEY_HANDLE *,TCS_KEY_HANDLE *);
TSS_RESULT RPC_EvictKey_TP(struct host_table_entry *,TCS_KEY_HANDLE);
TSS_RESULT RPC_CreateWrapKey_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCPA_ENCAUTH *,TCPA_ENCAUTH *,UINT32,BYTE *,UINT32 *,BYTE **,TPM_AUTH *);
TSS_RESULT RPC_GetPubKey_TP(struct host_table_entry *,TCS_KEY_HANDLE,TPM_AUTH *,UINT32 *,BYTE **);
TSS_RESULT RPC_TerminateHandle_TP(struct host_table_entry *,TCS_AUTHHANDLE);
TSS_RESULT RPC_OwnerReadInternalPub_TP(struct host_table_entry *, TCS_KEY_HANDLE, TPM_AUTH *, UINT32 *, BYTE **);
#ifdef TSS_BUILD_TSS12
TSS_RESULT RPC_KeyControlOwner_TP(struct host_table_entry *, TCS_KEY_HANDLE, UINT32, BYTE *, UINT32, TSS_BOOL, TPM_AUTH *, TSS_UUID *);
#else
#define RPC_KeyControlOwner_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif
#else
#define RPC_LoadKeyByBlob_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_EvictKey_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_CreateWrapKey_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetPubKey_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_TerminateHandle_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_OwnerReadInternalPub_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_KeyControlOwner_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_AIK
TSS_RESULT RPC_MakeIdentity_TP(struct host_table_entry *,TCPA_ENCAUTH,TCPA_CHOSENID_HASH,UINT32,BYTE *,TPM_AUTH *,TPM_AUTH *,UINT32 *,BYTE **,UINT32 *,BYTE **,UINT32 *,BYTE **,UINT32 *,BYTE **,UINT32 *,BYTE **);
TSS_RESULT RPC_GetCredential_TP(struct host_table_entry *,UINT32 ,UINT32 ,UINT32 *,BYTE **);
TSS_RESULT RPC_ActivateTPMIdentity_TP(struct host_table_entry *,TCS_KEY_HANDLE,UINT32,BYTE *,TPM_AUTH *,TPM_AUTH *,UINT32 *,BYTE **);
#else
#define RPC_MakeIdentity_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetCredential_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ActivateTPMIdentity_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_ADMIN
TSS_RESULT RPC_SetOwnerInstall_TP(struct host_table_entry *,TSS_BOOL);
TSS_RESULT RPC_DisableOwnerClear_TP(struct host_table_entry *,TPM_AUTH *);
TSS_RESULT RPC_ForceClear_TP(struct host_table_entry * hContext);
TSS_RESULT RPC_DisableForceClear_TP(struct host_table_entry * hContext);
TSS_RESULT RPC_PhysicalDisable_TP(struct host_table_entry * hContext);
TSS_RESULT RPC_PhysicalEnable_TP(struct host_table_entry * hContext);
TSS_RESULT RPC_PhysicalSetDeactivated_TP(struct host_table_entry *,TSS_BOOL);
TSS_RESULT RPC_PhysicalPresence_TP(struct host_table_entry *,TCPA_PHYSICAL_PRESENCE);
TSS_RESULT RPC_SetTempDeactivated_TP(struct host_table_entry * hContext);
#ifdef TSS_BUILD_TSS12
TSS_RESULT RPC_SetTempDeactivated2_TP(struct host_table_entry *, TPM_AUTH *);
#else
#define RPC_SetTempDeactivated2_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif
TSS_RESULT RPC_FieldUpgrade_TP(struct host_table_entry *,UINT32,BYTE *,UINT32 *,BYTE **,TPM_AUTH *);
TSS_RESULT RPC_SetRedirection_TP(struct host_table_entry *,TCS_KEY_HANDLE,UINT32,UINT32,TPM_AUTH *);
TSS_RESULT RPC_OwnerSetDisable_TP(struct host_table_entry *,TSS_BOOL,TPM_AUTH *);
TSS_RESULT RPC_ResetLockValue_TP(struct host_table_entry *, TPM_AUTH *);
#else
#define RPC_SetOwnerInstall_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_DisableOwnerClear_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ForceClear_TP(...)			TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_DisableForceClear_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_PhysicalDisable_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_PhysicalEnable_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_PhysicalSetDeactivated_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_PhysicalPresence_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_SetTempDeactivated_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_SetTempDeactivated2_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_FieldUpgrade_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_SetRedirection_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_OwnerSetDisable_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ResetLockValue_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_OWN
TSS_RESULT RPC_TakeOwnership_TP(struct host_table_entry *,UINT16,UINT32,BYTE *,UINT32,BYTE *,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **);
TSS_RESULT RPC_OwnerClear_TP(struct host_table_entry *,TPM_AUTH *);
#else
#define RPC_TakeOwnership_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_OwnerClear_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_CHANGEAUTH
TSS_RESULT RPC_ChangeAuth_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCPA_PROTOCOL_ID,TCPA_ENCAUTH *,TCPA_ENTITY_TYPE,UINT32,BYTE *,TPM_AUTH *,TPM_AUTH *,UINT32 *,BYTE **);
TSS_RESULT RPC_ChangeAuthOwner_TP(struct host_table_entry *,TCPA_PROTOCOL_ID,TCPA_ENCAUTH *,TCPA_ENTITY_TYPE,TPM_AUTH *);
TSS_RESULT RPC_ChangeAuthAsymStart_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCPA_NONCE,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **,UINT32 *,BYTE **,UINT32 *,BYTE **,TCS_KEY_HANDLE *);
TSS_RESULT RPC_ChangeAuthAsymFinish_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCS_KEY_HANDLE,TCPA_ENTITY_TYPE,TCPA_HMAC,UINT32,BYTE *,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **,TCPA_SALT_NONCE *,TCPA_DIGEST *);
#else
#define RPC_ChangeAuth_TP(...)			TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ChangeAuthOwner_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ChangeAuthAsymStart_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ChangeAuthAsymFinish_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_PCR_EXTEND
TSS_RESULT RPC_Extend_TP(struct host_table_entry *,TCPA_PCRINDEX,TCPA_DIGEST,TCPA_PCRVALUE *);
TSS_RESULT RPC_PcrRead_TP(struct host_table_entry *,TCPA_PCRINDEX,TCPA_PCRVALUE *);
TSS_RESULT RPC_PcrReset_TP(struct host_table_entry *,UINT32,BYTE *);
#else
#define RPC_Extend_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_PcrRead_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_PcrReset_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_QUOTE
TSS_RESULT RPC_Quote_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCPA_NONCE *,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **,UINT32 *,BYTE **);
#else
#define RPC_Quote_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_QUOTE2
TSS_RESULT RPC_Quote2_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCPA_NONCE *,UINT32,BYTE *,TSS_BOOL,TPM_AUTH *,UINT32 *,BYTE **,UINT32 *,BYTE **,UINT32 *,BYTE **);
#else
#define RPC_Quote2_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_DIR
TSS_RESULT RPC_DirWriteAuth_TP(struct host_table_entry *,TCPA_DIRINDEX,TCPA_DIRVALUE *,TPM_AUTH *);
TSS_RESULT RPC_DirRead_TP(struct host_table_entry *,TCPA_DIRINDEX,TCPA_DIRVALUE *);
#else
#define RPC_DirWriteAuth_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_DirRead_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_SEAL
TSS_RESULT RPC_Seal_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCPA_ENCAUTH *,UINT32,BYTE *,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **);
TSS_RESULT RPC_Unseal_TP(struct host_table_entry *,TCS_KEY_HANDLE,UINT32,BYTE *,TPM_AUTH *,TPM_AUTH *,UINT32 *,BYTE **);
#else
#define RPC_Seal_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_Unseal_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_SEALX
TSS_RESULT RPC_Sealx_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCPA_ENCAUTH *,UINT32,BYTE *,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **);
#else
#define RPC_Sealx_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_BIND
TSS_RESULT RPC_UnBind_TP(struct host_table_entry *,TCS_KEY_HANDLE,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **);
#else
#define RPC_UnBind_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_MIGRATION
TSS_RESULT RPC_CreateMigrationBlob_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCPA_MIGRATE_SCHEME,UINT32,BYTE *,UINT32,BYTE *,TPM_AUTH *,TPM_AUTH *,UINT32 *,BYTE **,UINT32 *,BYTE **);
TSS_RESULT RPC_ConvertMigrationBlob_TP(struct host_table_entry *,TCS_KEY_HANDLE,UINT32,BYTE *,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **);
TSS_RESULT RPC_AuthorizeMigrationKey_TP(struct host_table_entry *,TCPA_MIGRATE_SCHEME,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **);
#else
#define RPC_CreateMigrationBlob_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ConvertMigrationBlob_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_AuthorizeMigrationKey_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_CERTIFY
TSS_RESULT RPC_CertifyKey_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCS_KEY_HANDLE,TPM_NONCE *,TPM_AUTH *,TPM_AUTH *,UINT32 *,BYTE **,UINT32 *,BYTE **);
#else
#define RPC_CertifyKey_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_SIGN
TSS_RESULT RPC_Sign_TP(struct host_table_entry *,TCS_KEY_HANDLE,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **);
#else
#define RPC_Sign_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_RANDOM
TSS_RESULT RPC_GetRandom_TP(struct host_table_entry *,UINT32,BYTE **);
TSS_RESULT RPC_StirRandom_TP(struct host_table_entry *,UINT32,BYTE *);
#else
#define RPC_GetRandom_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_StirRandom_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_CAPS_TPM
TSS_RESULT RPC_GetTPMCapability_TP(struct host_table_entry *,TCPA_CAPABILITY_AREA,UINT32,BYTE *,UINT32 *,BYTE **);
TSS_RESULT RPC_GetCapabilitySigned_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCPA_NONCE,TCPA_CAPABILITY_AREA,UINT32,BYTE *,TPM_AUTH *,TCPA_VERSION *,UINT32 *,BYTE **,UINT32 *,BYTE **);
TSS_RESULT RPC_GetCapabilityOwner_TP(struct host_table_entry *,TPM_AUTH *,TCPA_VERSION *,UINT32 *,UINT32 *);
TSS_RESULT RPC_SetCapability_TP(struct host_table_entry *,TCPA_CAPABILITY_AREA,UINT32,BYTE *,UINT32,BYTE *,TPM_AUTH *);
#else
#define RPC_GetTPMCapability_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetCapabilitySigned_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetCapabilityOwner_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_SetCapability_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_CAPS
TSS_RESULT RPC_GetCapability_TP(struct host_table_entry *,TCPA_CAPABILITY_AREA,UINT32,BYTE *,UINT32 *,BYTE **);
#else
#define RPC_GetCapability_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_EK
TSS_RESULT RPC_CreateEndorsementKeyPair_TP(struct host_table_entry *,TCPA_NONCE,UINT32,BYTE *,UINT32 *,BYTE **,TCPA_DIGEST *);
TSS_RESULT RPC_ReadPubek_TP(struct host_table_entry *,TCPA_NONCE,UINT32 *,BYTE **,TCPA_DIGEST *);
TSS_RESULT RPC_OwnerReadPubek_TP(struct host_table_entry *,TPM_AUTH *,UINT32 *,BYTE **);
TSS_RESULT RPC_DisablePubekRead_TP(struct host_table_entry *,TPM_AUTH *);
#ifdef TSS_BUILD_TSS12
TSS_RESULT RPC_CreateRevocableEndorsementKeyPair_TP(struct host_table_entry *,TPM_NONCE,UINT32,BYTE *,TSS_BOOL,TPM_DIGEST *,UINT32 *,BYTE **,TPM_DIGEST *);
TSS_RESULT RPC_RevokeEndorsementKeyPair_TP(struct host_table_entry *,TPM_DIGEST *);
#else
#define RPC_CreateRevocableEndorsementKeyPair_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_RevokeEndorsementKeyPair_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#endif
#else
#define RPC_DisablePubekRead_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_CreateEndorsementKeyPair_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ReadPubek_TP(...)			TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_OwnerReadPubek_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_CreateRevocableEndorsementKeyPair_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_RevokeEndorsementKeyPair_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_SELFTEST
TSS_RESULT RPC_SelfTestFull_TP(struct host_table_entry * hContext);
TSS_RESULT RPC_CertifySelfTest_TP(struct host_table_entry *,TCS_KEY_HANDLE,TCPA_NONCE,TPM_AUTH *,UINT32 *,BYTE **);
TSS_RESULT RPC_GetTestResult_TP(struct host_table_entry *,UINT32 *,BYTE **);
#else
#define RPC_SelfTestFull_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_CertifySelfTest_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetTestResult_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_MAINT
TSS_RESULT RPC_CreateMaintenanceArchive_TP(struct host_table_entry *,TSS_BOOL,TPM_AUTH *,UINT32 *,BYTE **,UINT32 *,BYTE **);
TSS_RESULT RPC_LoadMaintenanceArchive_TP(struct host_table_entry *,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **);
TSS_RESULT RPC_KillMaintenanceFeature_TP(struct host_table_entry *,TPM_AUTH *);
TSS_RESULT RPC_LoadManuMaintPub_TP(struct host_table_entry *,TCPA_NONCE,UINT32,BYTE *,TCPA_DIGEST *);
TSS_RESULT RPC_ReadManuMaintPub_TP(struct host_table_entry *,TCPA_NONCE,TCPA_DIGEST *);
#else
#define RPC_CreateMaintenanceArchive_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_LoadMaintenanceArchive_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_KillMaintenanceFeature_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_LoadManuMaintPub_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ReadManuMaintPub_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_DAA
TSS_RESULT RPC_DaaJoin_TP(struct host_table_entry *,TPM_HANDLE,BYTE,UINT32,BYTE *,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **);
TSS_RESULT RPC_DaaSign_TP(struct host_table_entry *,TPM_HANDLE,BYTE,UINT32,BYTE *,UINT32,BYTE *,TPM_AUTH *,UINT32 *,BYTE **);
#else
#define RPC_DaaJoin_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_DaaSign_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_COUNTER
TSS_RESULT RPC_ReadCounter_TP(struct host_table_entry *,TSS_COUNTER_ID,TPM_COUNTER_VALUE *);
TSS_RESULT RPC_CreateCounter_TP(struct host_table_entry *,UINT32,BYTE *,TPM_ENCAUTH,TPM_AUTH *,TSS_COUNTER_ID *,TPM_COUNTER_VALUE *);
TSS_RESULT RPC_IncrementCounter_TP(struct host_table_entry *,TSS_COUNTER_ID,TPM_AUTH *,TPM_COUNTER_VALUE *);
TSS_RESULT RPC_ReleaseCounter_TP(struct host_table_entry *,TSS_COUNTER_ID,TPM_AUTH *);
TSS_RESULT RPC_ReleaseCounterOwner_TP(struct host_table_entry *,TSS_COUNTER_ID,TPM_AUTH *);
#else
#define RPC_ReadCounter_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_CreateCounter_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_IncrementCounter_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ReleaseCounter_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ReleaseCounterOwner_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_TICK
TSS_RESULT RPC_ReadCurrentTicks_TP(struct host_table_entry *,UINT32 *,BYTE **);
TSS_RESULT RPC_TickStampBlob_TP(struct host_table_entry *,TCS_KEY_HANDLE,TPM_NONCE *,TPM_DIGEST *,TPM_AUTH *,UINT32 *,BYTE **,UINT32 *,BYTE **);
#else
#define RPC_ReadCurrentTicks_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_TickStampBlob_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT RPC_EstablishTransport_TP(struct host_table_entry *, UINT32, TCS_KEY_HANDLE, UINT32, BYTE*, UINT32, BYTE*, TPM_AUTH*, TPM_MODIFIER_INDICATOR*, TCS_HANDLE*, UINT32*, BYTE**, TPM_NONCE*);
TSS_RESULT RPC_ExecuteTransport_TP(struct host_table_entry *,TPM_COMMAND_CODE, UINT32, BYTE*, UINT32*, TCS_HANDLE**, TPM_AUTH*, TPM_AUTH*, TPM_AUTH*, UINT64*, TPM_MODIFIER_INDICATOR*, TPM_RESULT*, UINT32*, BYTE**);
TSS_RESULT RPC_ReleaseTransportSigned_TP(struct host_table_entry *, TCS_KEY_HANDLE, TPM_NONCE *, TPM_AUTH*, TPM_AUTH*, TPM_MODIFIER_INDICATOR*, UINT32*, BYTE**, UINT32*, BYTE**);
#else
#define RPC_EstablishTransport_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ExecuteTransport_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_ReleaseTransportSigned_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_NV
TSS_RESULT RPC_NV_DefineOrReleaseSpace_TP(struct host_table_entry *hte, UINT32, BYTE *, TCPA_ENCAUTH, TPM_AUTH *);
TSS_RESULT RPC_NV_WriteValue_TP(struct host_table_entry *hte, TSS_NV_INDEX, UINT32, UINT32, BYTE *, TPM_AUTH *);
TSS_RESULT RPC_NV_WriteValueAuth_TP(struct host_table_entry *hte, TSS_NV_INDEX, UINT32, UINT32, BYTE *, TPM_AUTH *);
TSS_RESULT RPC_NV_ReadValue_TP(struct host_table_entry *hte, TSS_NV_INDEX, UINT32, UINT32 *, TPM_AUTH *, BYTE **);
TSS_RESULT RPC_NV_ReadValueAuth_TP(struct host_table_entry *hte, TSS_NV_INDEX, UINT32, UINT32 *, TPM_AUTH *, BYTE **);
#else
#define RPC_NV_DefineOrReleaseSpace_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_NV_WriteValue_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_NV_WriteValueAuth_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_NV_ReadValue_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_NV_ReadValueAuth_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_AUDIT
TSS_RESULT RPC_SetOrdinalAuditStatus_TP(struct host_table_entry *hte, TPM_AUTH *, UINT32, TSS_BOOL);
TSS_RESULT RPC_GetAuditDigest_TP(struct host_table_entry *hte, UINT32, TPM_DIGEST *, UINT32 *, BYTE **, TSS_BOOL *, UINT32 *, UINT32 **);
TSS_RESULT RPC_GetAuditDigestSigned_TP(struct host_table_entry *hte, TCS_KEY_HANDLE, TSS_BOOL, TPM_NONCE *, TPM_AUTH *, UINT32 *, BYTE **, TPM_DIGEST *, TPM_DIGEST *, UINT32 *, BYTE **);
#else
#define RPC_SetOrdinalAuditStatus_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetAuditDigest_TP(...)      TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_GetAuditDigestSigned_TP(...)      TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_TSS12
TSS_RESULT RPC_SetOperatorAuth_TP(struct host_table_entry *hte, TCPA_SECRET *);
TSS_RESULT RPC_FlushSpecific_TP(struct host_table_entry *hte, TCS_HANDLE, TPM_RESOURCE_TYPE);
#else
#define RPC_SetOperatorAuth_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_FlushSpecific_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_DELEGATION
TSS_RESULT RPC_Delegate_Manage_TP(struct host_table_entry *hte, TPM_FAMILY_ID, TPM_FAMILY_OPERATION, UINT32, BYTE *, TPM_AUTH *, UINT32 *, BYTE **);
TSS_RESULT RPC_Delegate_CreateKeyDelegation_TP(struct host_table_entry *hte, TCS_KEY_HANDLE, UINT32, BYTE *, TPM_ENCAUTH *, TPM_AUTH *, UINT32 *, BYTE **);
TSS_RESULT RPC_Delegate_CreateOwnerDelegation_TP(struct host_table_entry *hte, TSS_BOOL, UINT32, BYTE *, TPM_ENCAUTH *, TPM_AUTH *, UINT32 *, BYTE **);
TSS_RESULT RPC_Delegate_LoadOwnerDelegation_TP(struct host_table_entry *hte, TPM_DELEGATE_INDEX, UINT32, BYTE *, TPM_AUTH *);
TSS_RESULT RPC_Delegate_ReadTable_TP(struct host_table_entry *hte, UINT32 *, BYTE **, UINT32 *, BYTE **);
TSS_RESULT RPC_Delegate_UpdateVerificationCount_TP(struct host_table_entry *hte, UINT32, BYTE *, TPM_AUTH *, UINT32 *, BYTE **);
TSS_RESULT RPC_Delegate_VerifyDelegation_TP(struct host_table_entry *hte, UINT32, BYTE *);
TSS_RESULT RPC_DSAP_TP(struct host_table_entry *hte, TPM_ENTITY_TYPE, TCS_KEY_HANDLE, TPM_NONCE *, UINT32, BYTE *, TCS_AUTHHANDLE *, TPM_NONCE *, TPM_NONCE *);
#else
#define RPC_Delegate_Manage_TP(...)			TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_Delegate_CreateKeyDelegation_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_Delegate_CreateOwnerDelegation_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_Delegate_LoadOwnerDelegation_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_Delegate_ReadTable_TP(...)			TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_Delegate_UpdateVerificationCount_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_Delegate_VerifyDelegation_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_DSAP_TP(...)				TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#ifdef TSS_BUILD_CMK
TSS_RESULT RPC_CMK_SetRestrictions_TP(struct host_table_entry *hte, TSS_CMK_DELEGATE, TPM_AUTH *);
TSS_RESULT RPC_CMK_ApproveMA_TP(struct host_table_entry *hte, TPM_DIGEST, TPM_AUTH *, TPM_HMAC *);
TSS_RESULT RPC_CMK_CreateKey_TP(struct host_table_entry *hte, TCS_KEY_HANDLE, TPM_ENCAUTH *, TPM_HMAC *, TPM_DIGEST *, UINT32 *, BYTE **, TPM_AUTH *);
TSS_RESULT RPC_CMK_CreateTicket_TP(struct host_table_entry *hte, UINT32, BYTE *, TPM_DIGEST, UINT32, BYTE *, TPM_AUTH *, TPM_HMAC *);
TSS_RESULT RPC_CMK_CreateBlob_TP(struct host_table_entry *hte, TCS_KEY_HANDLE, TSS_MIGRATE_SCHEME, UINT32, BYTE *, TPM_DIGEST, UINT32, BYTE *, UINT32, BYTE *, UINT32, BYTE *, UINT32, BYTE *, TPM_AUTH *, UINT32 *, BYTE **, UINT32 *, BYTE **);
TSS_RESULT RPC_CMK_ConvertMigration_TP(struct host_table_entry *hte, TCS_KEY_HANDLE, TPM_CMK_AUTH, TPM_HMAC, UINT32, BYTE *, UINT32, BYTE *, UINT32, BYTE *, TPM_AUTH *, UINT32 *, BYTE **);
#else
#define RPC_CMK_SetRestrictions_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_CMK_ApproveMA_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_CMK_CreateKey_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_CMK_CreateTicket_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_CMK_CreateBlob_TP(...)		TSPERR(TSS_E_INTERNAL_ERROR)
#define RPC_CMK_ConvertMigration_TP(...)	TSPERR(TSS_E_INTERNAL_ERROR)
#endif

#endif
