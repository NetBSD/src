#ifndef TCS_H
#define TCS_H
#include <tss/platform.h>
#include <tss/tss_structs.h>
#include <tss/tcs_typedef.h>
#include <tss/tcs_defines.h>
#include <tss/tcs_structs.h>
#include <tss/tcs_error.h>
#include <tss/tpm.h>

#if defined __cplusplus
extern "C" {
#endif 

extern TSS_RESULT Tcsi_OpenContext
(
    TCS_CONTEXT_HANDLE*   hContext                     // out
);
extern TSS_RESULT Tcsi_CloseContext
(
    TCS_CONTEXT_HANDLE    hContext                     // in
);
extern TSS_RESULT Tcsi_FreeMemory
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    BYTE*                 pMemory                      // in
);
extern TSS_RESULT Tcsi_GetCapability
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_CAPABILITY_AREA   capArea,                     // in
    UINT32                subCapSize,                  // in
    BYTE*                 subCap,                      // in
    UINT32*               respSize,                    // out
    BYTE**                resp                         // out
);
extern TSS_RESULT Tcsi_RegisterKey
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_UUID              WrappingKeyUUID,             // in
    TSS_UUID              KeyUUID,                     // in
    UINT32                cKeySize,                    // in
    BYTE*                 rgbKey,                      // in
    UINT32                cVendorDataSize,             // in
    BYTE*                 gbVendorData                 // in
);
extern TSS_RESULT Tcsip_UnregisterKey
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_UUID              KeyUUID                      // in
);
extern TSS_RESULT Tcsip_KeyControlOwner
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        hKey,                        // in
    UINT32                ulPubKeyLength,              // in
    BYTE*                 prgbPubKey,                  // in
    UINT32                attribName,                  // in
    TSS_BOOL              attribValue,                 // in
    TPM_AUTH*             pOwnerAuth,                  // in, out
    TSS_UUID*             pUuidData                    // out
);
extern TSS_RESULT Tcsi_EnumRegisteredKeys
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_UUID*             pKeyUUID,                    // in
    UINT32*               pcKeyHierarchySize,          // out
    TSS_KM_KEYINFO**      ppKeyHierarchy               // out
);
extern TSS_RESULT Tcsi_GetRegisteredKey
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_UUID              KeyUUID,                     // in
    TSS_KM_KEYINFO**      ppKeyInfo                    // out
);
extern TSS_RESULT Tcsi_GetRegisteredKeyBlob
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_UUID              KeyUUID,                     // in
    UINT32*               pcKeySize,                   // out
    BYTE**                prgbKey                      // out
);
extern TSS_RESULT Tcsip_GetRegisteredKeyByPublicInfo
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_ALGORITHM_ID      algID,                       // in
    UINT32                ulPublicInfoLength,          // in
    BYTE*                 rgbPublicInfo,               // in
    UINT32*               keySize,                     // out
    BYTE**                keyBlob                      // out
);
extern TSS_RESULT Tcsip_LoadKeyByBlob
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        hUnwrappingKey,              // in
    UINT32                cWrappedKeyBlobSize,         // in
    BYTE*                 rgbWrappedKeyBlob,           // in
    TPM_AUTH*             pAuth,                       // in, out
    TCS_KEY_HANDLE*       phKeyTCSI,                   // out
    TCS_KEY_HANDLE*       phKeyHMAC                    // out
);
extern TSS_RESULT Tcsip_LoadKeyByUUID
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_UUID              KeyUUID,                     // in
    TCS_LOADKEY_INFO*     pLoadKeyInfo,                // in, out
    TCS_KEY_HANDLE*       phKeyTCSI                    // out
);
extern TSS_RESULT Tcsip_EvictKey
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        hKey                         // in
);
extern TSS_RESULT Tcsip_CreateWrapKey
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        hWrappingKey,                // in
    TPM_ENCAUTH           KeyUsageAuth,                // in
    TPM_ENCAUTH           KeyMigrationAuth,            // in
    UINT32                keyInfoSize,                 // in
    BYTE*                 keyInfo,                     // in
    TPM_AUTH*             pAuth,                       // in, out
    UINT32*               keyDataSize,                 // out
    BYTE**                keyData                      // out
);
extern TSS_RESULT Tcsip_GetPubKey
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        hKey,                        // in
    TPM_AUTH*             pAuth,                       // in, out
    UINT32*               pcPubKeySize,                // out
    BYTE**                prgbPubKey                   // out
);
extern TSS_RESULT Tcsip_MakeIdentity
(
    TCS_CONTEXT_HANDLE    hContext,                              // in
    TPM_ENCAUTH           identityAuth,                          // in
    TPM_CHOSENID_HASH     IDLabel_PrivCAHash,                    // in
    UINT32                idIdentityKeyInfoSize,                 // in
    BYTE*                 idIdentityKeyInfo,                     // in
    TPM_AUTH*             pSrkAuth,                              // in, out
    TPM_AUTH*             pOwnerAuth,                            // in, out
    UINT32*               idIdentityKeySize,                     // out
    BYTE**                idIdentityKey,                         // out
    UINT32*               pcIdentityBindingSize,                 // out
    BYTE**                prgbIdentityBinding,                   // out
    UINT32*               pcEndorsementCredentialSize,           // out
    BYTE**                prgbEndorsementCredential,             // out
    UINT32*               pcPlatformCredentialSize,              // out
    BYTE**                prgbPlatformCredential,                // out
    UINT32*               pcConformanceCredentialSize,           // out
    BYTE**                prgbConformanceCredential              // out
);
extern TSS_RESULT Tcsip_MakeIdentity2
(
    TCS_CONTEXT_HANDLE    hContext,                              // in
    TPM_ENCAUTH           identityAuth,                          // in
    TPM_CHOSENID_HASH     IDLabel_PrivCAHash,                    // in
    UINT32                idIdentityKeyInfoSize,                 // in
    BYTE*                 idIdentityKeyInfo,                     // in
    TPM_AUTH*             pSrkAuth,                              // in, out
    TPM_AUTH*             pOwnerAuth,                            // in, out
    UINT32*               idIdentityKeySize,                     // out
    BYTE**                idIdentityKey,                         // out
    UINT32*               pcIdentityBindingSize,                 // out
    BYTE**                prgbIdentityBinding                    // out
);
extern TSS_RESULT Tcsi_LogPcrEvent
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_PCR_EVENT         Event,                       // in
    UINT32*               pNumber                      // out
);
extern TSS_RESULT Tcsi_GetPcrEvent
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                PcrIndex,                    // in
    UINT32*               pNumber,                     // in, out
    TSS_PCR_EVENT**       ppEvent                      // out
);
extern TSS_RESULT Tcsi_GetPcrEventsByPcr
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                PcrIndex,                    // in
    UINT32                FirstEvent,                  // in
    UINT32*               pEventCount,                 // in, out
    TSS_PCR_EVENT**       ppEvents                     // out
);
extern TSS_RESULT Tcsi_GetPcrEventLog
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32*               pEventCount,                 // out
    TSS_PCR_EVENT**       ppEvents                     // out
);
extern TSS_RESULT Tcsip_SetOwnerInstall
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_BOOL              state                        // in
);
extern TSS_RESULT Tcsip_TakeOwnership
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT16                protocolID,                  // in
    UINT32                encOwnerAuthSize,            // in
    BYTE*                 encOwnerAuth,                // in
    UINT32                encSrkAuthSize,              // in
    BYTE*                 encSrkAuth,                  // in
    UINT32                srkKeyInfoSize,              // in
    BYTE*                 srkKeyInfo,                  // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               srkKeyDataSize,              // out
    BYTE**                srkKeyData                   // out
);
extern TSS_RESULT Tcsip_SetOperatorAuth
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_SECRET            operatorAuth                 // in
);
extern TSS_RESULT Tcsip_OIAP
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_AUTHHANDLE*       authHandle,                  // out
    TPM_NONCE*            nonce0                       // out
);
extern TSS_RESULT Tcsip_OSAP
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_ENTITY_TYPE       entityType,                  // in
    UINT32                entityValue,                 // in
    TPM_NONCE             nonceOddOSAP,                // in
    TCS_AUTHHANDLE*       authHandle,                  // out
    TPM_NONCE*            nonceEven,                   // out
    TPM_NONCE*            nonceEvenOSAP                // out
);
extern TSS_RESULT Tcsip_ChangeAuth
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        parentHandle,                // in
    TPM_PROTOCOL_ID       protocolID,                  // in
    TPM_ENCAUTH           newAuth,                     // in
    TPM_ENTITY_TYPE       entityType,                  // in
    UINT32                encDataSize,                 // in
    BYTE*                 encData,                     // in
    TPM_AUTH*             ownerAuth,                   // in, out
    TPM_AUTH*             entityAuth,                  // in, out
    UINT32*               outDataSize,                 // out
    BYTE**                outData                      // out
);
extern TSS_RESULT Tcsip_ChangeAuthOwner
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_PROTOCOL_ID       protocolID,                  // in
    TPM_ENCAUTH           newAuth,                     // in
    TPM_ENTITY_TYPE       entityType,                  // in
    TPM_AUTH*             ownerAuth                    // in, out
);
extern TSS_RESULT Tcsip_ChangeAuthAsymStart
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        idHandle,                    // in
    TPM_NONCE             antiReplay,                  // in
    UINT32                TempKeyInfoSize,             // in
    BYTE*                 TempKeyInfoData,             // in
    TPM_AUTH*             pAuth,                       // in, out
    UINT32*               TempKeySize,                 // out
    BYTE**                TempKeyData,                 // out
    UINT32*               CertifyInfoSize,             // out
    BYTE**                CertifyInfo,                 // out
    UINT32*               sigSize,                     // out
    BYTE**                sig,                         // out
    TCS_KEY_HANDLE*       ephHandle                    // out
);
extern TSS_RESULT Tcsip_ChangeAuthAsymFinish
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        parentHandle,                // in
    TCS_KEY_HANDLE        ephHandle,                   // in
    TPM_ENTITY_TYPE       entityType,                  // in
    TPM_HMAC              newAuthLink,                 // in
    UINT32                newAuthSize,                 // in
    BYTE*                 encNewAuth,                  // in
    UINT32                encDataSizeIn,               // in
    BYTE*                 encDataIn,                   // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               encDataSizeOut,              // out
    BYTE**                encDataOut,                  // out
    TPM_NONCE*            saltNonce,                   // out
    TPM_DIGEST*           changeProof                  // out
);
extern TSS_RESULT Tcsip_TerminateHandle
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_AUTHHANDLE        handle                       // in
);
extern TSS_RESULT Tcsip_ActivateTPMIdentity
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        idKey,                       // in
    UINT32                blobSize,                    // in
    BYTE*                 blob,                        // in
    TPM_AUTH*             idKeyAuth,                   // in, out
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               SymmetricKeySize,            // out
    BYTE**                SymmetricKey                 // out
);
extern TSS_RESULT Tcsip_EstablishTransport
(
    TCS_CONTEXT_HANDLE            hContext,                      // in
    UINT32                        ulTransControlFlags,           // in
    TCS_KEY_HANDLE                hEncKey,                       // in
    UINT32                        ulTransSessionInfoSize,        // in
    BYTE*                         rgbTransSessionInfo,           // in
    UINT32                        ulSecretSize,                  // in
    BYTE*                         rgbSecret,                     // in
    TPM_AUTH*                     pEncKeyAuth,                   // in, out
    TPM_MODIFIER_INDICATOR*       pbLocality,                    // out
    TCS_HANDLE*                   hTransSession,                 // out
    UINT32*                       ulCurrentTicksSize,            // out
    BYTE**                        prgbCurrentTicks,              // out
    TPM_NONCE*                    pTransNonce                    // out
);
extern TSS_RESULT Tcsip_ExecuteTransport
(
    TCS_CONTEXT_HANDLE            hContext,                      // in
    TPM_COMMAND_CODE              unWrappedCommandOrdinal,       // in
    UINT32                        ulWrappedCmdParamInSize,       // in
    BYTE*                         rgbWrappedCmdParamIn,          // in
    UINT32*                       pulHandleListSize,             // in, out
    TCS_HANDLE**                  rghHandles,                    // in, out
    TPM_AUTH*                     pWrappedCmdAuth1,              // in, out
    TPM_AUTH*                     pWrappedCmdAuth2,              // in, out
    TPM_AUTH*                     pTransAuth,                    // in, out
    UINT64*                       punCurrentTicks,               // out
    TPM_MODIFIER_INDICATOR*       pbLocality,                    // out
    TPM_RESULT*                   pulWrappedCmdReturnCode,       // out
    UINT32*                       ulWrappedCmdParamOutSize,      // out
    BYTE**                        rgbWrappedCmdParamOut          // out
);
extern TSS_RESULT Tcsip_ReleaseTransportSigned
(
    TCS_CONTEXT_HANDLE            hContext,            // in
    TCS_KEY_HANDLE                hSignatureKey,       // in
    TPM_NONCE                     AntiReplayNonce,     // in
    TPM_AUTH*                     pKeyAuth,            // in, out
    TPM_AUTH*                     pTransAuth,          // in, out
    TPM_MODIFIER_INDICATOR*       pbLocality,          // out
    UINT32*                       pulCurrentTicksSize, // out
    BYTE**                        prgbCurrentTicks,    // out
    UINT32*                       pulSignatureSize,    // out
    BYTE**                        prgbSignature        // out
);
extern TSS_RESULT Tcsip_Extend
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_PCRINDEX          pcrNum,                      // in
    TPM_DIGEST            inDigest,                    // in
    TPM_PCRVALUE*         outDigest                    // out
);
extern TSS_RESULT Tcsip_PcrRead
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_PCRINDEX          pcrNum,                      // in
    TPM_PCRVALUE*         outDigest                    // out
);
extern TSS_RESULT Tcsip_Quote
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    TPM_NONCE             antiReplay,                  // in
    UINT32                pcrTargetSize,               // in
    BYTE*                 pcrTarget,                   // in
    TPM_AUTH*             privAuth,                    // in, out
    UINT32*               pcrDataSize,                 // out
    BYTE**                pcrData,                     // out
    UINT32*               sigSize,                     // out
    BYTE**                sig                          // out
);
extern TSS_RESULT Tcsip_Quote2
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    TPM_NONCE             antiReplay,                  // in
    UINT32                pcrTargetSize,               // in
    BYTE*                 pcrTarget,                   // in
    TSS_BOOL              addVersion,                  // in
    TPM_AUTH*             privAuth,                    // in, out
    UINT32*               pcrDataSize,                 // out
    BYTE**                pcrData,                     // out
    UINT32*               versionInfoSize,             // out
    BYTE**                versionInfo,                 // out
    UINT32*               sigSize,                     // out
    BYTE**                sig                          // out
);
extern TSS_RESULT Tcsip_DirWriteAuth
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_DIRINDEX          dirIndex,                    // in
    TPM_DIRVALUE          newContents,                 // in
    TPM_AUTH*             ownerAuth                    // in, out
);
extern TSS_RESULT Tcsip_DirRead
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_DIRINDEX          dirIndex,                    // in
    TPM_DIRVALUE*         dirValue                     // out
);
extern TSS_RESULT Tcsip_Seal
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    TPM_ENCAUTH           encAuth,                     // in
    UINT32                pcrInfoSize,                 // in
    BYTE*                 PcrInfo,                     // in
    UINT32                inDataSize,                  // in
    BYTE*                 inData,                      // in
    TPM_AUTH*             pubAuth,                     // in, out
    UINT32*               SealedDataSize,              // out
    BYTE**                SealedData                   // out
);
extern TSS_RESULT Tcsip_Unseal
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    UINT32                SealedDataSize,              // in
    BYTE*                 SealedData,                  // in
    TPM_AUTH*             keyAuth,                     // in, out
    TPM_AUTH*             dataAuth,                    // in, out
    UINT32*               DataSize,                    // out
    BYTE**                Data                         // out
);
extern TSS_RESULT Tcsip_UnBind
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    UINT32                inDataSize,                  // in
    BYTE*                 inData,                      // in
    TPM_AUTH*             privAuth,                    // in, out
    UINT32*               outDataSize,                 // out
    BYTE**                outData                      // out
);
extern TSS_RESULT Tcsip_Sealx
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    TPM_ENCAUTH           encAuth,                     // in
    UINT32                pcrInfoSize,                 // in
    BYTE*                 PcrInfo,                     // in
    UINT32                inDataSize,                  // in
    BYTE*                 inData,                      // in
    TPM_AUTH*             pubAuth,                     // in, out
    UINT32*               SealedDataSize,              // out
    BYTE**                SealedData                   // out
);
extern TSS_RESULT Tcsip_LoadKey2ByBlob
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        hUnwrappingKey,              // in
    UINT32                cWrappedKeyBlobSize,         // in
    BYTE*                 rgbWrappedKeyBlob,           // in
    TPM_AUTH*             pAuth,                       // in, out
    TCS_KEY_HANDLE*       phKeyTCSI                    // out
);
extern TSS_RESULT Tcsip_CreateMigrationBlob
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        parentHandle,                // in
    TSS_MIGRATE_SCHEME    migrationType,               // in
    UINT32                MigrationKeyAuthSize,        // in
    BYTE*                 MigrationKeyAuth,            // in
    UINT32                encDataSize,                 // in
    BYTE*                 encData,                     // in
    TPM_AUTH*             parentAuth,                  // in, out
    TPM_AUTH*             entityAuth,                  // in, out
    UINT32*               randomSize,                  // out
    BYTE**                random,                      // out
    UINT32*               outDataSize,                 // out
    BYTE**                outData                      // out
);
extern TSS_RESULT Tcsip_ConvertMigrationBlob
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        parentHandle,                // in
    UINT32                inDataSize,                  // in
    BYTE*                 inData,                      // in
    UINT32                randomSize,                  // in
    BYTE*                 random,                      // in
    TPM_AUTH*             parentAuth,                  // in, out
    UINT32*               outDataSize,                 // out
    BYTE**                outData                      // out
);
extern TSS_RESULT Tcsip_AuthorizeMigrationKey
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_MIGRATE_SCHEME    migrateScheme,               // in
    UINT32                MigrationKeySize,            // in
    BYTE*                 MigrationKey,                // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               MigrationKeyAuthSize,        // out
    BYTE**                MigrationKeyAuth             // out
);
extern TSS_RESULT Tcsip_CertifyKey
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        certHandle,                  // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    TPM_NONCE             antiReplay,                  // in
    TPM_AUTH*             certAuth,                    // in, out
    TPM_AUTH*             keyAuth,                     // in, out
    UINT32*               CertifyInfoSize,             // out
    BYTE**                CertifyInfo,                 // out
    UINT32*               outDataSize,                 // out
    BYTE**                outData                      // out
);
extern TSS_RESULT Tcsip_CertifyKey2
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        certHandle,                  // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    TPM_DIGEST            MSAdigest,                   // in
    TPM_NONCE             antiReplay,                  // in
    TPM_AUTH*             certAuth,                    // in, out
    TPM_AUTH*             keyAuth,                     // in, out
    UINT32*               CertifyInfoSize,             // out
    BYTE**                CertifyInfo,                 // out
    UINT32*               outDataSize,                 // out
    BYTE**                outData                      // out
);
extern TSS_RESULT Tcsip_Sign
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    UINT32                areaToSignSize,              // in
    BYTE*                 areaToSign,                  // in
    TPM_AUTH*             privAuth,                    // in, out
    UINT32*               sigSize,                     // out
    BYTE**                sig                          // out
);
extern TSS_RESULT Tcsip_GetRandom
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32*               bytesRequested,              // in, out
    BYTE**                randomBytes                  // out
);
extern TSS_RESULT Tcsip_StirRandom
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                inDataSize,                  // in
    BYTE*                 inData                       // in
);
extern TSS_RESULT Tcsip_GetCapability
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_CAPABILITY_AREA   capArea,                     // in
    UINT32                subCapSize,                  // in
    BYTE*                 subCap,                      // in
    UINT32*               respSize,                    // out
    BYTE**                resp                         // out
);
extern TSS_RESULT Tcsip_GetCapabilitySigned
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    TPM_NONCE             antiReplay,                  // in
    TPM_CAPABILITY_AREA   capArea,                     // in
    UINT32                subCapSize,                  // in
    BYTE*                 subCap,                      // in
    TPM_AUTH*             privAuth,                    // in, out
    TPM_VERSION*          Version,                     // out
    UINT32*               respSize,                    // out
    BYTE**                resp,                        // out
    UINT32*               sigSize,                     // out
    BYTE**                sig                          // out
);
extern TSS_RESULT Tcsip_GetCapabilityOwner
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_AUTH*             pOwnerAuth,                  // in, out
    TPM_VERSION*          pVersion,                    // out
    UINT32*               pNonVolatileFlags,           // out
    UINT32*               pVolatileFlags               // out
);
extern TSS_RESULT Tcsip_CreateEndorsementKeyPair
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_NONCE             antiReplay,                  // in
    UINT32                endorsementKeyInfoSize,      // in
    BYTE*                 endorsementKeyInfo,          // in
    UINT32*               endorsementKeySize,          // out
    BYTE**                endorsementKey,              // out
    TPM_DIGEST*           checksum                     // out
);
extern TSS_RESULT Tcsip_ReadPubek
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_NONCE             antiReplay,                  // in
    UINT32*               pubEndorsementKeySize,       // out
    BYTE**                pubEndorsementKey,           // out
    TPM_DIGEST*           checksum                     // out
);
extern TSS_RESULT Tcsip_DisablePubekRead
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_AUTH*             ownerAuth                    // in, out
);
extern TSS_RESULT Tcsip_OwnerReadPubek
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               pubEndorsementKeySize,       // out
    BYTE**                pubEndorsementKey            // out
);
extern TSS_RESULT Tcsip_SelfTestFull
(
    TCS_CONTEXT_HANDLE    hContext                     // in
);
extern TSS_RESULT Tcsip_CertifySelfTest
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    TPM_NONCE             antiReplay,                  // in
    TPM_AUTH*             privAuth,                    // in, out
    UINT32*               sigSize,                     // out
    BYTE**                sig                          // out
);
extern TSS_RESULT Tcsip_ContinueSelfTest
(
    TCS_CONTEXT_HANDLE    hContext                     // in
);
extern TSS_RESULT Tcsip_GetTestResult
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32*               outDataSize,                 // out
    BYTE**                outData                      // out
);
extern TSS_RESULT Tcsip_OwnerSetDisable
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_BOOL              disableState,                // in
    TPM_AUTH*             ownerAuth                    // in, out
);
extern TSS_RESULT Tcsip_OwnerClear
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_AUTH*             ownerAuth                    // in, out
);
extern TSS_RESULT Tcsip_DisableOwnerClear
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_AUTH*             ownerAuth                    // in, out
);
extern TSS_RESULT Tcsip_ForceClear
(
    TCS_CONTEXT_HANDLE    hContext                     // in
);
extern TSS_RESULT Tcsip_DisableForceClear
(
    TCS_CONTEXT_HANDLE    hContext                     // in
);
extern TSS_RESULT Tcsip_PhysicalDisable
(
    TCS_CONTEXT_HANDLE    hContext                     // in
);
extern TSS_RESULT Tcsip_PhysicalEnable
(
    TCS_CONTEXT_HANDLE    hContext                     // in
);
extern TSS_RESULT Tcsip_PhysicalSetDeactivated
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_BOOL              state                        // in
);
extern TSS_RESULT Tcsip_SetTempDeactivated
(
    TCS_CONTEXT_HANDLE    hContext                     // in
);
extern TSS_RESULT Tcsip_SetTempDeactivated2
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_AUTH*             pOperatorAuth                // in, out
);
extern TSS_RESULT Tcsip_OwnerReadInternalPub
(
    TCS_CONTEXT_HANDLE   hContext,                     // in
    TCS_KEY_HANDLE       hKey,                         // in
    TPM_AUTH*            pOwnerAuth,                   // in, out
    UINT32*              punPubKeySize,                // out
    BYTE**               ppbPubKeyData                 // out
);
extern TSS_RESULT Tcsip_PhysicalPresence
(
    TCS_CONTEXT_HANDLE            hContext,            // in
    TPM_PHYSICAL_PRESENCE         fPhysicalPresence    // in
);
extern TSS_RESULT Tcsip_FieldUpgrade
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                dataInSize,                  // in
    BYTE*                 dataIn,                      // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               dataOutSize,                 // out
    BYTE**                dataOut                      // out
);
extern TSS_RESULT Tcsip_ResetLockValue
(
    TCS_CONTEXT_HANDLE            hContext,            // in
    TPM_AUTH*                     ownerAuth            // in, out
);
extern TSS_RESULT Tcsip_FlushSpecific
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_HANDLE            hResHandle,                  // in
    TPM_RESOURCE_TYPE     resourceType                 // in
);
extern TSS_RESULT Tcsip_SetRedirection
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    UINT32                c1,                          // in
    UINT32                c2,                          // in
    TPM_AUTH*             privAuth                     // in, out
);
extern TSS_RESULT Tcsip_DSAP
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_ENTITY_TYPE       entityType,                  // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    TPM_NONCE             nonceOddDSAP,                // in
    UINT32                entityValueSize,             // in
    BYTE*                 entityValue,                 // in
    TCS_AUTHHANDLE*       authHandle,                  // out
    TPM_NONCE*            nonceEven,                   // out
    TPM_NONCE*            nonceEvenDSAP                // out
);
extern TSS_RESULT Tcsip_Delegate_Manage
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_FAMILY_ID         familyID,                    // in
    TPM_FAMILY_OPERATION  opFlag,                      // in
    UINT32                opDataSize,                  // in
    BYTE*                 opData,                      // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               retDataSize,                 // out
    BYTE**                retData                      // out
);
extern TSS_RESULT Tcsip_Delegate_CreateKeyDelegation
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        hKey,                        // in
    UINT32                publicInfoSize,              // in
    BYTE*                 publicInfo,                  // in
    TPM_ENCAUTH           encDelAuth,                  // in
    TPM_AUTH*             keyAuth,                     // in, out
    UINT32*               blobSize,                    // out
    BYTE**                blob                         // out
);
extern TSS_RESULT Tcsip_Delegate_CreateOwnerDelegation
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_BOOL              increment,                   // in
    UINT32                publicInfoSize,              // in
    BYTE*                 publicInfo,                  // in
    TPM_ENCAUTH           encDelAuth,                  // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               blobSize,                    // out
    BYTE**                blob                         // out
);
extern TSS_RESULT Tcsip_Delegate_LoadOwnerDelegation
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_DELEGATE_INDEX    index,                       // in
    UINT32                blobSize,                    // in
    BYTE*                 blob,                        // in
    TPM_AUTH*             ownerAuth                    // in, out
);
extern TSS_RESULT Tcsip_Delegate_UpdateVerificationCount
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                inputSize,                   // in
    BYTE*                 input,                       // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               outputSize,                  // out
    BYTE**                output                       // out
);
extern TSS_RESULT Tcsip_Delegate_VerifyDelegation
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                delegateSize,                // in
    BYTE*                 delegate                     // in
);
extern TSS_RESULT Tcsip_Delegate_ReadTable
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32*               pulFamilyTableSize,          // out
    BYTE**                ppFamilyTable,               // out
    UINT32*               pulDelegateTableSize,        // out
    BYTE**                ppDelegateTable              // out
);
extern TSS_RESULT Tcsip_NV_DefineOrReleaseSpace
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                cPubInfoSize,                // in
    BYTE*                 pPubInfo,                    // in
    TPM_ENCAUTH           encAuth,                     // in
    TPM_AUTH*             pAuth                        // in, out
);
extern TSS_RESULT Tcsip_NV_WriteValue
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_NV_INDEX          hNVStore,                    // in
    UINT32                offset,                      // in
    UINT32                ulDataLength,                // in
    BYTE*                 rgbDataToWrite,              // in
    TPM_AUTH*             privAuth                     // in, out
);
extern TSS_RESULT Tcsip_NV_WriteValueAuth
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_NV_INDEX          hNVStore,                    // in
    UINT32                offset,                      // in
    UINT32                ulDataLength,                // in
    BYTE*                 rgbDataToWrite,              // in
    TPM_AUTH*             NVAuth                       // in, out
);
extern TSS_RESULT Tcsip_NV_ReadValue
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_NV_INDEX          hNVStore,                    // in
    UINT32                offset,                      // in
    UINT32*               pulDataLength,               // in, out
    TPM_AUTH*             privAuth,                    // in, out
    BYTE**                rgbDataRead                  // out
);
extern TSS_RESULT Tcsip_NV_ReadValueAuth
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_NV_INDEX          hNVStore,                    // in
    UINT32                offset,                      // in
    UINT32*               pulDataLength,               // in, out
    TPM_AUTH*             NVAuth,                      // in, out
    BYTE**                rgbDataRead                  // out
);
extern TSS_RESULT Tcsip_CreateMaintenanceArchive
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_BOOL              generateRandom,              // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               randomSize,                  // out
    BYTE**                random,                      // out
    UINT32*               archiveSize,                 // out
    BYTE**                archive                      // out
);
extern TSS_RESULT Tcsip_LoadMaintenanceArchive
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                dataInSize,                  // in
    BYTE*                 dataIn,                      // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               dataOutSize,                 // out
    BYTE**                dataOut                      // out
);
extern TSS_RESULT Tcsip_KillMaintenanceFeature
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_AUTH*             ownerAuth                    // in, out
);
extern TSS_RESULT Tcsip_LoadManuMaintPub
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_NONCE             antiReplay,                  // in
    UINT32                PubKeySize,                  // in
    BYTE*                 PubKey,                      // in
    TPM_DIGEST*           checksum                     // out
);
extern TSS_RESULT Tcsip_ReadManuMaintPub
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_NONCE             antiReplay,                  // in
    TPM_DIGEST*           checksum                     // out
);
extern TSS_RESULT Tcsip_CreateRevocableEndorsementKeyPair
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_NONCE             antiReplay,                  // in
    UINT32                endorsementKeyInfoSize,      // in
    BYTE*                 endorsementKeyInfo,          // in
    TSS_BOOL              GenResetAuth,                // in
    TPM_DIGEST*           EKResetAuth,                 // in, out
    UINT32*               endorsementKeySize,          // out
    BYTE**                endorsementKey,              // out
    TPM_DIGEST*           checksum                     // out
);
extern TSS_RESULT Tcsip_RevokeEndorsementKeyPair
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_DIGEST            EKResetAuth                  // in
);
extern TSS_RESULT Tcsip_PcrReset
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                pcrTargetSize,               // in
    BYTE*                 pcrTarget                    // in
);
extern TSS_RESULT Tcsip_ReadCounter
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_COUNTER_ID        idCounter,                   // in
    TPM_COUNTER_VALUE*    counterValue                 // out
);
extern TSS_RESULT Tcsip_CreateCounter
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                LabelSize,                   // in (=4)
    BYTE*                 pLabel,                      // in
    TPM_ENCAUTH           CounterAuth,                 // in
    TPM_AUTH*             pOwnerAuth,                  // in, out
    TSS_COUNTER_ID*       idCounter,                   // out
    TPM_COUNTER_VALUE*    counterValue                 // out
);
extern TSS_RESULT Tcsip_IncrementCounter
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_COUNTER_ID        idCounter,                   // in
    TPM_AUTH*             pCounterAuth,                // in, out
    TPM_COUNTER_VALUE*    counterValue                 // out
);
extern TSS_RESULT Tcsip_ReleaseCounter
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_COUNTER_ID        idCounter,                   // in
    TPM_AUTH*             pCounterAuth                 // in, out
);
extern TSS_RESULT Tcsip_ReleaseCounterOwner
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_COUNTER_ID        idCounter,                   // in
    TPM_AUTH*             pOwnerAuth                   // in, out
);
extern TSS_RESULT Tcsip_ReadCurrentTicks
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32*               pulCurrentTimeSize,          // out
    BYTE**                prgbCurrentTime              // out
);
extern TSS_RESULT Tcsip_TickStampBlob
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        hKey,                        // in
    TPM_NONCE             antiReplay,                  // in
    TPM_DIGEST            digestToStamp,               // in
    TPM_AUTH*             privAuth,                    // in, out
    UINT32*               pulSignatureLength,          // out
    BYTE**                prgbSignature,               // out
    UINT32*               pulTickCountSize,            // out
    BYTE**                prgbTickCount                // out
);
extern TSS_RESULT Tcsip_TPM_DAA_Join
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_HANDLE            handle,                      // in
    BYTE                  stage,                       // in
    UINT32                inputSize0,                  // in
    BYTE*                 inputData0,                  // in
    UINT32                inputSize1,                  // in
    BYTE*                 inputData1,                  // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               outputSize,                  // out
    BYTE**                outputData                   // out
);
extern TSS_RESULT Tcsip_TPM_DAA_Sign
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_HANDLE            handle,                      // in
    BYTE                  stage,                       // in
    UINT32                inputSize0,                  // in
    BYTE*                 inputData0,                  // in
    UINT32                inputSize1,                  // in
    BYTE*                 inputData1,                  // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               outputSize,                  // out
    BYTE**                outputData                   // out
);
extern TSS_RESULT Tcsip_MigrateKey
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        hMaKey,                      // in
    UINT32                PublicKeySize,               // in
    BYTE*                 PublicKey,                   // in
    UINT32                inDataSize,                  // in
    BYTE*                 inData,                      // in
    TPM_AUTH*             ownerAuth,                   // in, out
    UINT32*               outDataSize,                 // out
    BYTE**                outData                      // out
);
extern TSS_RESULT Tcsip_CMK_SetRestrictions
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TSS_CMK_DELEGATE      Restriction,                 // in
    TPM_AUTH*             ownerAuth                    // in, out
);
extern TSS_RESULT Tcsip_CMK_ApproveMA
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_DIGEST            migAuthorityDigest,          // in
    TPM_AUTH*             ownerAuth,                   // in, out
    TPM_HMAC*             HmacMigAuthDigest            // out
);
extern TSS_RESULT Tcsip_CMK_CreateKey
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        hWrappingKey,                // in
    TPM_ENCAUTH           KeyUsageAuth,                // in
    TPM_HMAC              MigAuthApproval,             // in
    TPM_DIGEST            MigAuthorityDigest,          // in
    UINT32*               keyDataSize,                 // in, out
    BYTE**                prgbKeyData,                 // in, out
    TPM_AUTH*             pAuth                        // in, out
);
extern TSS_RESULT Tcsip_CMK_CreateTicket
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                PublicVerifyKeySize,         // in
    BYTE*                 PublicVerifyKey,             // in
    TPM_DIGEST            SignedData,                  // in
    UINT32                SigValueSize,                // in
    BYTE*                 SigValue,                    // in
    TPM_AUTH*             pOwnerAuth,                  // in, out
    TPM_HMAC*             SigTicket                    // out
);
extern TSS_RESULT Tcsip_CMK_CreateBlob
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        parentHandle,                // in
    TSS_MIGRATE_SCHEME    migrationType,               // in
    UINT32                MigrationKeyAuthSize,        // in
    BYTE*                 MigrationKeyAuth,            // in
    TPM_DIGEST            PubSourceKeyDigest,          // in
    UINT32                msaListSize,                 // in
    BYTE*                 msaList,                     // in
    UINT32                restrictTicketSize,          // in
    BYTE*                 restrictTicket,              // in
    UINT32                sigTicketSize,               // in
    BYTE*                 sigTicket,                   // in
    UINT32                encDataSize,                 // in
    BYTE*                 encData,                     // in
    TPM_AUTH*             parentAuth,                  // in, out
    UINT32*               randomSize,                  // out
    BYTE**                random,                      // out
    UINT32*               outDataSize,                 // out
    BYTE**                outData                      // out
);
extern TSS_RESULT Tcsip_CMK_ConvertMigration
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        parentHandle,                // in
    TPM_CMK_AUTH          restrictTicket,              // in
    TPM_HMAC              sigTicket,                   // in
    UINT32                keyDataSize,                 // in
    BYTE*                 prgbKeyData,                 // in
    UINT32                msaListSize,                 // in
    BYTE*                 msaList,                     // in
    UINT32                randomSize,                  // in
    BYTE*                 random,                      // in
    TPM_AUTH*             parentAuth,                  // in, out
    UINT32*               outDataSize,                 // out
    BYTE**                outData                      // out
);
extern TSS_RESULT Tcsip_SetCapability
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TPM_CAPABILITY_AREA   capArea,                     // in
    UINT32                subCapSize,                  // in
    BYTE*                 subCap,                      // in
    UINT32                valueSize,                   // in
    BYTE*                 value,                       // in
    TPM_AUTH*             ownerAuth                    // in, out
);
extern TSS_RESULT Tcsip_GetAuditDigest
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                startOrdinal,                // in
    TPM_DIGEST*           auditDigest,                 // out
    UINT32*               counterValueSize,            // out
    BYTE**                counterValue,                // out
    TSS_BOOL*             more,                        // out
    UINT32*               ordSize,                     // out
    UINT32**              ordList                      // out
);
extern TSS_RESULT Tcsip_GetAuditDigestSigned
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    TCS_KEY_HANDLE        keyHandle,                   // in
    TSS_BOOL              closeAudit,                  // in
    TPM_NONCE             antiReplay,                  // in
    TPM_AUTH*             privAuth,                    // in, out
    UINT32*               counterValueSize,            // out
    BYTE**                counterValue,                // out
    TPM_DIGEST*           auditDigest,                 // out
    TPM_DIGEST*           ordinalDigest,               // out
    UINT32*               sigSize,                     // out
    BYTE**                sig                          // out
);
extern TSS_RESULT Tcsip_SetOrdinalAuditStatus
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                ordinalToAudit,              // in
    TSS_BOOL              auditState,                  // in
    TPM_AUTH*             ownerAuth                    // in, out
);
extern TSS_RESULT Tcsi_Admin_TSS_SessionsPerLocality
(
    TCS_CONTEXT_HANDLE    hContext,                    // in
    UINT32                ulLocality,                  // in
    UINT32                ulSessions,                  // in
    TPM_AUTH*             pOwnerAuth                   // in, out
);
extern TSS_RESULT Tcsi_GetCredential
(
    TCS_CONTEXT_HANDLE  hContext,               // in
    UINT32              ulCredentialType,       // in
    UINT32              ulCredentialAccessMode, // in
    UINT32*             pulCredentialSize,      // out
    BYTE**              prgbCredentialData      // out
);

#if defined __cplusplus
} // extern "C"
#endif

#endif /* TCS_H */
