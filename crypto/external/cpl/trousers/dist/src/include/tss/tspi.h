#if !defined(_TSPI_H_)
#define _TSPI_H_

#include <tss/tss_defines.h>
#include <tss/tss_typedef.h>
#include <tss/tss_structs.h>
#include <tss/tss_error.h>
#include <tss/tss_error_basics.h>

#if !defined( TSPICALL )
  #if !defined(WIN32) || defined (TSP_STATIC)
    // Linux, or a Win32 static library
    #define TSPICALL extern TSS_RESULT
  #elif defined (TSPDLL_EXPORTS)
    // Win32 DLL build
    #define TSPICALL extern __declspec(dllexport) TSS_RESULT
  #else
    // Win32 DLL import
    #define TSPICALL extern __declspec(dllimport) TSS_RESULT
  #endif
#endif /* TSPICALL */

#if defined ( __cplusplus )
extern "C" {
#endif /* __cplusplus */


// Class-independent ASN.1 conversion functions
TSPICALL Tspi_EncodeDER_TssBlob
(
    UINT32              rawBlobSize,                   // in
    BYTE*               rawBlob,                       // in
    UINT32              blobType,                      // in
    UINT32*             derBlobSize,                   // in, out
    BYTE*               derBlob                        // out
);

TSPICALL Tspi_DecodeBER_TssBlob
(
    UINT32              berBlobSize,                   // in
    BYTE*               berBlob,                       // in
    UINT32*             blobType,                      // out
    UINT32*             rawBlobSize,                   // in, out
    BYTE*               rawBlob                        // out
);



// Common Methods
TSPICALL Tspi_SetAttribUint32
(
    TSS_HOBJECT         hObject,                       // in
    TSS_FLAG            attribFlag,                    // in
    TSS_FLAG            subFlag,                       // in
    UINT32              ulAttrib                       // in
);

TSPICALL Tspi_GetAttribUint32
(
    TSS_HOBJECT         hObject,                       // in
    TSS_FLAG            attribFlag,                    // in
    TSS_FLAG            subFlag,                       // in
    UINT32*             pulAttrib                      // out
);

TSPICALL Tspi_SetAttribData
(
    TSS_HOBJECT         hObject,                       // in
    TSS_FLAG            attribFlag,                    // in
    TSS_FLAG            subFlag,                       // in
    UINT32              ulAttribDataSize,              // in
    BYTE*               rgbAttribData                  // in
);

TSPICALL Tspi_GetAttribData
(
    TSS_HOBJECT         hObject,                       // in
    TSS_FLAG            attribFlag,                    // in
    TSS_FLAG            subFlag,                       // in
    UINT32*             pulAttribDataSize,             // out
    BYTE**              prgbAttribData                 // out
);

TSPICALL Tspi_ChangeAuth
(
    TSS_HOBJECT         hObjectToChange,               // in
    TSS_HOBJECT         hParentObject,                 // in
    TSS_HPOLICY         hNewPolicy                     // in
);

TSPICALL Tspi_ChangeAuthAsym
(
    TSS_HOBJECT         hObjectToChange,               // in
    TSS_HOBJECT         hParentObject,                 // in
    TSS_HKEY            hIdentKey,                     // in
    TSS_HPOLICY         hNewPolicy                     // in
);

TSPICALL Tspi_GetPolicyObject
(
    TSS_HOBJECT         hObject,                       // in
    TSS_FLAG            policyType,                    // in
    TSS_HPOLICY*        phPolicy                       // out
);



// Tspi_Context Class Definitions
TSPICALL Tspi_Context_Create
(
    TSS_HCONTEXT*       phContext                      // out
);

TSPICALL Tspi_Context_Close
(
    TSS_HCONTEXT        hContext                       // in
);

TSPICALL Tspi_Context_Connect
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_UNICODE*        wszDestination                 // in
);

TSPICALL Tspi_Context_FreeMemory
(
    TSS_HCONTEXT        hContext,                      // in
    BYTE*               rgbMemory                      // in
);

TSPICALL Tspi_Context_GetDefaultPolicy
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_HPOLICY*        phPolicy                       // out
);

TSPICALL Tspi_Context_CreateObject
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_FLAG            objectType,                    // in
    TSS_FLAG            initFlags,                     // in
    TSS_HOBJECT*        phObject                       // out
);

TSPICALL Tspi_Context_CloseObject
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_HOBJECT         hObject                        // in
);

TSPICALL Tspi_Context_GetCapability
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_FLAG            capArea,                       // in
    UINT32              ulSubCapLength,                // in
    BYTE*               rgbSubCap,                     // in
    UINT32*             pulRespDataLength,             // out
    BYTE**              prgbRespData                   // out
);

TSPICALL Tspi_Context_GetTpmObject
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_HTPM*           phTPM                          // out
);

TSPICALL Tspi_Context_SetTransEncryptionKey
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_HKEY            hKey                           // in
);

TSPICALL Tspi_Context_CloseSignTransport
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_HKEY            hSigningKey,                   // in
    TSS_VALIDATION*     pValidationData                // in, out
);

TSPICALL Tspi_Context_LoadKeyByBlob
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_HKEY            hUnwrappingKey,                // in
    UINT32              ulBlobLength,                  // in
    BYTE*               rgbBlobData,                   // in
    TSS_HKEY*           phKey                          // out
);

TSPICALL Tspi_Context_LoadKeyByUUID
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_FLAG            persistentStorageType,         // in
    TSS_UUID            uuidData,                      // in
    TSS_HKEY*           phKey                          // out
);

TSPICALL Tspi_Context_RegisterKey
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_HKEY            hKey,                          // in
    TSS_FLAG            persistentStorageType,         // in
    TSS_UUID            uuidKey,                       // in
    TSS_FLAG            persistentStorageTypeParent,   // in
    TSS_UUID            uuidParentKey                  // in
);

TSPICALL Tspi_Context_UnregisterKey
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_FLAG            persistentStorageType,         // in
    TSS_UUID            uuidKey,                       // in
    TSS_HKEY*           phkey                          // out
);

TSPICALL Tspi_Context_GetKeyByUUID
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_FLAG            persistentStorageType,         // in
    TSS_UUID            uuidData,                      // in
    TSS_HKEY*           phKey                          // out
);

TSPICALL Tspi_Context_GetKeyByPublicInfo
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_FLAG            persistentStorageType,         // in
    TSS_ALGORITHM_ID    algID,                         // in
    UINT32              ulPublicInfoLength,            // in
    BYTE*               rgbPublicInfo,                 // in
    TSS_HKEY*           phKey                          // out
);

TSPICALL Tspi_Context_GetRegisteredKeysByUUID
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_FLAG            persistentStorageType,         // in
    TSS_UUID*           pUuidData,                     // in
    UINT32*             pulKeyHierarchySize,           // out
    TSS_KM_KEYINFO**    ppKeyHierarchy                 // out
);

TSPICALL Tspi_Context_GetRegisteredKeysByUUID2
(
    TSS_HCONTEXT        hContext,                      // in
    TSS_FLAG            persistentStorageType,         // in
    TSS_UUID*           pUuidData,                     // in
    UINT32*             pulKeyHierarchySize,           // out
    TSS_KM_KEYINFO2**   ppKeyHierarchy                 // out
);


// Policy class definitions
TSPICALL Tspi_Policy_SetSecret
(
    TSS_HPOLICY         hPolicy,                       // in
    TSS_FLAG            secretMode,                    // in
    UINT32              ulSecretLength,                // in
    BYTE*               rgbSecret                      // in
);

TSPICALL Tspi_Policy_FlushSecret
(
    TSS_HPOLICY         hPolicy                        // in
);

TSPICALL Tspi_Policy_AssignToObject
(
    TSS_HPOLICY         hPolicy,                       // in
    TSS_HOBJECT         hObject                        // in
);



// TPM Class Definitions
TSPICALL Tspi_TPM_KeyControlOwner
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hKey,                          // in
    UINT32              attribName,                    // in
    TSS_BOOL            attribValue,                   // in
    TSS_UUID*           pUuidData                      // out
);

TSPICALL Tspi_TPM_CreateEndorsementKey
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hKey,                          // in
    TSS_VALIDATION*     pValidationData                // in, out
);

TSPICALL Tspi_TPM_CreateRevocableEndorsementKey
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hKey,                          // in
    TSS_VALIDATION*     pValidationData,               // in, out
    UINT32*             pulEkResetDataLength,          // in, out
    BYTE**              rgbEkResetData                 // in, out
);

TSPICALL Tspi_TPM_RevokeEndorsementKey
(
    TSS_HTPM            hTPM,                          // in
    UINT32              ulEkResetDataLength,           // in
    BYTE*               rgbEkResetData                 // in
);

TSPICALL Tspi_TPM_GetPubEndorsementKey
(
    TSS_HTPM            hTPM,                          // in
    TSS_BOOL            fOwnerAuthorized,              // in
    TSS_VALIDATION*     pValidationData,               // in, out
    TSS_HKEY*           phEndorsementPubKey            // out
);

TSPICALL Tspi_TPM_OwnerGetSRKPubKey
(
    TSS_HTPM            hTPM,                          // in
    UINT32*             pulPubKeyLength,               // out
    BYTE**              prgbPubKey                     // out
);

TSPICALL Tspi_TPM_TakeOwnership
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hKeySRK,                       // in
    TSS_HKEY            hEndorsementPubKey             // in
);

TSPICALL Tspi_TPM_ClearOwner
(
    TSS_HTPM            hTPM,                          // in
    TSS_BOOL            fForcedClear                   // in
);

TSPICALL Tspi_TPM_CollateIdentityRequest
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hKeySRK,                       // in
    TSS_HKEY            hCAPubKey,                     // in
    UINT32              ulIdentityLabelLength,         // in
    BYTE*               rgbIdentityLabelData,          // in
    TSS_HKEY            hIdentityKey,                  // in
    TSS_ALGORITHM_ID    algID,                         // in
    UINT32*             pulTCPAIdentityReqLength,      // out
    BYTE**              prgbTCPAIdentityReq            // out
);

TSPICALL Tspi_TPM_ActivateIdentity
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hIdentKey,                     // in
    UINT32              ulAsymCAContentsBlobLength,    // in
    BYTE*               rgbAsymCAContentsBlob,         // in
    UINT32              ulSymCAAttestationBlobLength,  // in
    BYTE*               rgbSymCAAttestationBlob,       // in
    UINT32*             pulCredentialLength,           // out
    BYTE**              prgbCredential                 // out
);

TSPICALL Tspi_TPM_CreateMaintenanceArchive
(
    TSS_HTPM            hTPM,                          // in
    TSS_BOOL            fGenerateRndNumber,            // in
    UINT32*             pulRndNumberLength,            // out
    BYTE**              prgbRndNumber,                 // out
    UINT32*             pulArchiveDataLength,          // out
    BYTE**              prgbArchiveData                // out
);

TSPICALL Tspi_TPM_KillMaintenanceFeature
(
    TSS_HTPM            hTPM                           // in
);

TSPICALL Tspi_TPM_LoadMaintenancePubKey
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hMaintenanceKey,               // in
    TSS_VALIDATION*     pValidationData                // in, out
);

TSPICALL Tspi_TPM_CheckMaintenancePubKey
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hMaintenanceKey,               // in
    TSS_VALIDATION*     pValidationData                // in, out
);

TSPICALL Tspi_TPM_SetOperatorAuth
(
    TSS_HTPM            hTPM,                          // in
    TSS_HPOLICY         hOperatorPolicy                // in
);

TSPICALL Tspi_TPM_SetStatus
(
    TSS_HTPM            hTPM,                          // in
    TSS_FLAG            statusFlag,                    // in
    TSS_BOOL            fTpmState                      // in
);

TSPICALL Tspi_TPM_GetStatus
(
    TSS_HTPM            hTPM,                          // in
    TSS_FLAG            statusFlag,                    // in
    TSS_BOOL*           pfTpmState                     // out
);

TSPICALL Tspi_TPM_GetCapability
(
    TSS_HTPM            hTPM,                          // in
    TSS_FLAG            capArea,                       // in
    UINT32              ulSubCapLength,                // in
    BYTE*               rgbSubCap,                     // in
    UINT32*             pulRespDataLength,             // out
    BYTE**              prgbRespData                   // out
);

TSPICALL Tspi_TPM_GetCapabilitySigned
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hKey,                          // in
    TSS_FLAG            capArea,                       // in
    UINT32              ulSubCapLength,                // in
    BYTE*               rgbSubCap,                     // in
    TSS_VALIDATION*     pValidationData,               // in, out
    UINT32*             pulRespDataLength,             // out
    BYTE**              prgbRespData                   // out
);

TSPICALL Tspi_TPM_SelfTestFull
(
    TSS_HTPM            hTPM                           // in
);

TSPICALL Tspi_TPM_CertifySelfTest
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hKey,                          // in
    TSS_VALIDATION*     pValidationData                // in, out
);

TSPICALL Tspi_TPM_GetTestResult
(
    TSS_HTPM            hTPM,                          // in
    UINT32*             pulTestResultLength,           // out
    BYTE**              prgbTestResult                 // out
);

TSPICALL Tspi_TPM_GetRandom
(
    TSS_HTPM            hTPM,                          // in
    UINT32              ulRandomDataLength,            // in
    BYTE**              prgbRandomData                 // out
);

TSPICALL Tspi_TPM_StirRandom
(
    TSS_HTPM            hTPM,                          // in
    UINT32              ulEntropyDataLength,           // in
    BYTE*               rgbEntropyData                 // in
);

TSPICALL Tspi_TPM_GetEvent
(
    TSS_HTPM            hTPM,                          // in
    UINT32              ulPcrIndex,                    // in
    UINT32              ulEventNumber,                 // in
    TSS_PCR_EVENT*      pPcrEvent                      // out
);

TSPICALL Tspi_TPM_GetEvents
(
    TSS_HTPM            hTPM,                          // in
    UINT32              ulPcrIndex,                    // in
    UINT32              ulStartNumber,                 // in
    UINT32*             pulEventNumber,                // in, out
    TSS_PCR_EVENT**     prgPcrEvents                   // out
);

TSPICALL Tspi_TPM_GetEventLog
(
    TSS_HTPM            hTPM,                          // in
    UINT32*             pulEventNumber,                // out
    TSS_PCR_EVENT**     prgPcrEvents                   // out
);

TSPICALL Tspi_TPM_Quote
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hIdentKey,                     // in
    TSS_HPCRS           hPcrComposite,                 // in
    TSS_VALIDATION*     pValidationData                // in, out
);

TSPICALL Tspi_TPM_Quote2
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hIdentKey,                     // in
    TSS_BOOL            fAddVersion,                   // in
    TSS_HPCRS           hPcrComposite,                 // in
    TSS_VALIDATION*     pValidationData,               // in, out
    UINT32*             versionInfoSize,               // out
    BYTE**              versionInfo                    // out
);

TSPICALL Tspi_TPM_PcrExtend
(
    TSS_HTPM            hTPM,                          // in
    UINT32              ulPcrIndex,                    // in
    UINT32              ulPcrDataLength,               // in
    BYTE*               pbPcrData,                     // in
    TSS_PCR_EVENT*      pPcrEvent,                     // in
    UINT32*             pulPcrValueLength,             // out
    BYTE**              prgbPcrValue                   // out
);

TSPICALL Tspi_TPM_PcrRead
(
    TSS_HTPM            hTPM,                          // in
    UINT32              ulPcrIndex,                    // in
    UINT32*             pulPcrValueLength,             // out
    BYTE**              prgbPcrValue                   // out
);

TSPICALL Tspi_TPM_PcrReset
(
    TSS_HTPM            hTPM,                          // in
    TSS_HPCRS           hPcrComposite                  // in
);

TSPICALL Tspi_TPM_AuthorizeMigrationTicket
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hMigrationKey,                 // in
    TSS_MIGRATE_SCHEME  migrationScheme,               // in
    UINT32*             pulMigTicketLength,            // out
    BYTE**              prgbMigTicket                  // out
);

TSPICALL Tspi_TPM_CMKSetRestrictions
(
    TSS_HTPM            hTPM,                          // in
    TSS_CMK_DELEGATE    CmkDelegate                    // in
);

TSPICALL Tspi_TPM_CMKApproveMA
(
    TSS_HTPM            hTPM,                          // in
    TSS_HMIGDATA        hMaAuthData                    // in
);

TSPICALL Tspi_TPM_CMKCreateTicket
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hVerifyKey,                    // in
    TSS_HMIGDATA        hSigData                       // in
);

TSPICALL Tspi_TPM_ReadCounter
(
    TSS_HTPM            hTPM,                          // in
    UINT32*             counterValue                   // out
);

TSPICALL Tspi_TPM_ReadCurrentTicks
(
    TSS_HTPM            hTPM,                          // in
    TPM_CURRENT_TICKS*  tickCount                      // out
);

TSPICALL Tspi_TPM_DirWrite
(
    TSS_HTPM            hTPM,                          // in
    UINT32              ulDirIndex,                    // in
    UINT32              ulDirDataLength,               // in
    BYTE*               rgbDirData                     // in
);

TSPICALL Tspi_TPM_DirRead
(
    TSS_HTPM            hTPM,                          // in
    UINT32              ulDirIndex,                    // in
    UINT32*             pulDirDataLength,              // out
    BYTE**              prgbDirData                    // out
);

TSPICALL Tspi_TPM_Delegate_AddFamily
(
    TSS_HTPM            hTPM,                          // in, must not be NULL
    BYTE                bLabel,                        // in
    TSS_HDELFAMILY*     phFamily                       // out
);

TSPICALL Tspi_TPM_Delegate_GetFamily
(
    TSS_HTPM            hTPM,                          // in, must not NULL
    UINT32              ulFamilyID,                    // in
    TSS_HDELFAMILY*     phFamily                       // out
);

TSPICALL Tspi_TPM_Delegate_InvalidateFamily
(
    TSS_HTPM            hTPM,                          // in, must not be NULL
    TSS_HDELFAMILY      hFamily                        // in
);

TSPICALL Tspi_TPM_Delegate_CreateDelegation
(
    TSS_HOBJECT         hObject,                       // in
    BYTE                bLabel,                        // in
    UINT32              ulFlags,                       // in
    TSS_HPCRS           hPcr,                          // in, may be NULL
    TSS_HDELFAMILY      hFamily,                       // in
    TSS_HPOLICY         hDelegation                    // in, out
);

TSPICALL Tspi_TPM_Delegate_CacheOwnerDelegation
(
    TSS_HTPM            hTPM,                          // in, must not be NULL
    TSS_HPOLICY         hDelegation,                   // in, out
    UINT32              ulIndex,                       // in
    UINT32              ulFlags                        // in
);

TSPICALL Tspi_TPM_Delegate_UpdateVerificationCount
(
    TSS_HTPM            hTPM,                          // in
    TSS_HPOLICY         hDelegation                    // in, out
);

TSPICALL Tspi_TPM_Delegate_VerifyDelegation
(
    TSS_HPOLICY         hDelegation                    // in, out
);

TSPICALL Tspi_TPM_Delegate_ReadTables
(
    TSS_HCONTEXT                  hContext,                      // in
    UINT32*                       pulFamilyTableSize,            // out
    TSS_FAMILY_TABLE_ENTRY**      ppFamilyTable,                 // out
    UINT32*                       pulDelegateTableSize,          // out
    TSS_DELEGATION_TABLE_ENTRY**  ppDelegateTable                // out
);

TSPICALL Tspi_TPM_DAA_JoinInit
(
    TSS_HTPM                      hTPM,                          // in
    TSS_HDAA_ISSUER_KEY           hIssuerKey,                    // in
    UINT32                        daaCounter,                    // in
    UINT32                        issuerAuthPKsLength,           // in
    TSS_HKEY*                     issuerAuthPKs,                 // in
    UINT32                        issuerAuthPKSignaturesLength,  // in
    UINT32                        issuerAuthPKSignaturesLength2, // in
    BYTE**                        issuerAuthPKSignatures,        // in
    UINT32*                       capitalUprimeLength,           // out
    BYTE**                        capitalUprime,                 // out
    TSS_DAA_IDENTITY_PROOF**      identityProof,                 // out
    UINT32*                       joinSessionLength,             // out
    BYTE**                        joinSession                    // out
);

TSPICALL Tspi_TPM_DAA_JoinCreateDaaPubKey
(
    TSS_HTPM                      hTPM,                          // in
    TSS_HDAA_CREDENTIAL           hDAACredential,                // in
    UINT32                        authenticationChallengeLength, // in
    BYTE*                         authenticationChallenge,       // in
    UINT32                        nonceIssuerLength,             // in
    BYTE*                         nonceIssuer,                   // in
    UINT32                        attributesPlatformLength,      // in
    UINT32                        attributesPlatformLength2,     // in
    BYTE**                        attributesPlatform,            // in
    UINT32                        joinSessionLength,             // in
    BYTE*                         joinSession,                   // in
    TSS_DAA_CREDENTIAL_REQUEST**  credentialRequest              // out
);

TSPICALL Tspi_TPM_DAA_JoinStoreCredential
(
    TSS_HTPM                      hTPM,                          // in
    TSS_HDAA_CREDENTIAL           hDAACredential,                // in
    TSS_DAA_CRED_ISSUER*          credIssuer,                    // in
    UINT32                        joinSessionLength,             // in
    BYTE*                         joinSession                    // in
);

TSPICALL Tspi_TPM_DAA_Sign
(
    TSS_HTPM                      hTPM,                          // in
    TSS_HDAA_CREDENTIAL           hDAACredential,                // in
    TSS_HDAA_ARA_KEY              hARAKey,                       // in
    TSS_DAA_SELECTED_ATTRIB*      revealAttributes,              // in
    UINT32                        verifierNonceLength,           // in
    BYTE*                         verifierNonce,                 // in
    UINT32                        verifierBaseNameLength,        // in
    BYTE*                         verifierBaseName,              // in
    TSS_HOBJECT                   signData,                      // in
    TSS_DAA_SIGNATURE**           daaSignature                   // out
);

TSPICALL Tspi_TPM_GetAuditDigest
(
    TSS_HTPM            hTPM,                          // in
    TSS_HKEY            hKey,                          // in
    TSS_BOOL            closeAudit,                    // in
    UINT32*             pulAuditDigestSize,            // out
    BYTE**              prgbAuditDigest,               // out
    TPM_COUNTER_VALUE*  pCounterValue,                 // out
    TSS_VALIDATION*     pValidationData,               // out
    UINT32*             ordSize,                       // out
    UINT32**            ordList                        // out
);



// PcrComposite Class Definitions
TSPICALL Tspi_PcrComposite_SelectPcrIndex
(
    TSS_HPCRS           hPcrComposite,                 // in
    UINT32              ulPcrIndex                     // in
);

TSPICALL Tspi_PcrComposite_SelectPcrIndexEx
(
    TSS_HPCRS           hPcrComposite,                 // in
    UINT32              ulPcrIndex,                    // in
    UINT32              direction                      // in
);

TSPICALL Tspi_PcrComposite_SetPcrValue
(
    TSS_HPCRS           hPcrComposite,                 // in
    UINT32              ulPcrIndex,                    // in
    UINT32              ulPcrValueLength,              // in
    BYTE*               rgbPcrValue                    // in
);

TSPICALL Tspi_PcrComposite_GetPcrValue
(
    TSS_HPCRS           hPcrComposite,                 // in
    UINT32              ulPcrIndex,                    // in
    UINT32*             pulPcrValueLength,             // out
    BYTE**              prgbPcrValue                   // out
);

TSPICALL Tspi_PcrComposite_SetPcrLocality
(
    TSS_HPCRS           hPcrComposite,                 // in
    UINT32              LocalityValue                  // in
);

TSPICALL Tspi_PcrComposite_GetPcrLocality
(
    TSS_HPCRS           hPcrComposite,                 // in
    UINT32*             pLocalityValue                 // out
);

TSPICALL Tspi_PcrComposite_GetCompositeHash
(
    TSS_HPCRS           hPcrComposite,                 // in
    UINT32*             pLen,                          // in
    BYTE**              ppbHashData                    // out
);



// Key Class Definition
TSPICALL Tspi_Key_LoadKey
(
    TSS_HKEY            hKey,                          // in
    TSS_HKEY            hUnwrappingKey                 // in
);

TSPICALL Tspi_Key_UnloadKey
(
    TSS_HKEY            hKey                           // in
);

TSPICALL Tspi_Key_GetPubKey
(
    TSS_HKEY            hKey,                          // in
    UINT32*             pulPubKeyLength,               // out
    BYTE**              prgbPubKey                     // out
);

TSPICALL Tspi_Key_CertifyKey
(
    TSS_HKEY            hKey,                          // in
    TSS_HKEY            hCertifyingKey,                // in
    TSS_VALIDATION*     pValidationData                // in, out
);

TSPICALL Tspi_Key_CreateKey
(
    TSS_HKEY            hKey,                          // in
    TSS_HKEY            hWrappingKey,                  // in
    TSS_HPCRS           hPcrComposite                  // in, may be NULL
);

TSPICALL Tspi_Key_WrapKey
(
    TSS_HKEY            hKey,                          // in
    TSS_HKEY            hWrappingKey,                  // in
    TSS_HPCRS           hPcrComposite                  // in, may be NULL
);

TSPICALL Tspi_Key_CreateMigrationBlob
(
    TSS_HKEY            hKeyToMigrate,                 // in
    TSS_HKEY            hParentKey,                    // in
    UINT32              ulMigTicketLength,             // in
    BYTE*               rgbMigTicket,                  // in
    UINT32*             pulRandomLength,               // out
    BYTE**              prgbRandom,                    // out
    UINT32*             pulMigrationBlobLength,        // out
    BYTE**              prgbMigrationBlob              // out
);

TSPICALL Tspi_Key_ConvertMigrationBlob
(
    TSS_HKEY            hKeyToMigrate,                 // in
    TSS_HKEY            hParentKey,                    // in
    UINT32              ulRandomLength,                // in
    BYTE*               rgbRandom,                     // in
    UINT32              ulMigrationBlobLength,         // in
    BYTE*               rgbMigrationBlob               // in
);

TSPICALL Tspi_Key_MigrateKey
(
    TSS_HKEY            hMaKey,                        // in
    TSS_HKEY            hPublicKey,                    // in
    TSS_HKEY            hMigData                       // in
);

TSPICALL Tspi_Key_CMKCreateBlob
(
    TSS_HKEY            hKeyToMigrate,                 // in
    TSS_HKEY            hParentKey,                    // in
    TSS_HMIGDATA        hMigrationData,                // in
    UINT32*             pulRandomLength,               // out
    BYTE**              prgbRandom                     // out
);

TSPICALL Tspi_Key_CMKConvertMigration
(
    TSS_HKEY            hKeyToMigrate,                 // in
    TSS_HKEY            hParentKey,                    // in
    TSS_HMIGDATA        hMigrationData,                // in
    UINT32              ulRandomLength,                // in
    BYTE*               rgbRandom                      // in
);



// Hash Class Definition
TSPICALL Tspi_Hash_Sign
(
    TSS_HHASH           hHash,                         // in
    TSS_HKEY            hKey,                          // in
    UINT32*             pulSignatureLength,            // out
    BYTE**              prgbSignature                  // out
);

TSPICALL Tspi_Hash_VerifySignature
(
    TSS_HHASH           hHash,                         // in
    TSS_HKEY            hKey,                          // in
    UINT32              ulSignatureLength,             // in
    BYTE*               rgbSignature                   // in
);

TSPICALL Tspi_Hash_SetHashValue
(
    TSS_HHASH           hHash,                         // in
    UINT32              ulHashValueLength,             // in
    BYTE*               rgbHashValue                   // in
);

TSPICALL Tspi_Hash_GetHashValue
(
    TSS_HHASH           hHash,                         // in
    UINT32*             pulHashValueLength,            // out
    BYTE**              prgbHashValue                  // out
);

TSPICALL Tspi_Hash_UpdateHashValue
(
    TSS_HHASH           hHash,                         // in
    UINT32              ulDataLength,                  // in
    BYTE*               rgbData                        // in
);

TSPICALL Tspi_Hash_TickStampBlob
(
    TSS_HHASH           hHash,                         // in
    TSS_HKEY            hIdentKey,                     // in
    TSS_VALIDATION*     pValidationData                // in
);



// EncData Class Definition
TSPICALL Tspi_Data_Bind
(
    TSS_HENCDATA        hEncData,                      // in
    TSS_HKEY            hEncKey,                       // in
    UINT32              ulDataLength,                  // in
    BYTE*               rgbDataToBind                  // in
);

TSPICALL Tspi_Data_Unbind
(
    TSS_HENCDATA        hEncData,                      // in
    TSS_HKEY            hKey,                          // in
    UINT32*             pulUnboundDataLength,          // out
    BYTE**              prgbUnboundData                // out
);

TSPICALL Tspi_Data_Seal
(
    TSS_HENCDATA        hEncData,                      // in
    TSS_HKEY            hEncKey,                       // in
    UINT32              ulDataLength,                  // in
    BYTE*               rgbDataToSeal,                 // in
    TSS_HPCRS           hPcrComposite                  // in
);

TSPICALL Tspi_Data_Unseal
(
    TSS_HENCDATA        hEncData,                      // in
    TSS_HKEY            hKey,                          // in
    UINT32*             pulUnsealedDataLength,         // out
    BYTE**              prgbUnsealedData               // out
);



// NV Class Definition
TSPICALL Tspi_NV_DefineSpace
(
    TSS_HNVSTORE        hNVStore,                      // in
    TSS_HPCRS           hReadPcrComposite,             // in, may be NULL
    TSS_HPCRS           hWritePcrComposite             // in, may be NULL
);

TSPICALL Tspi_NV_ReleaseSpace
(
    TSS_HNVSTORE        hNVStore                       // in
);

TSPICALL Tspi_NV_WriteValue
(
    TSS_HNVSTORE        hNVStore,                      // in
    UINT32              offset,                        // in
    UINT32              ulDataLength,                  // in
    BYTE*               rgbDataToWrite                 // in
);

TSPICALL Tspi_NV_ReadValue
(
    TSS_HNVSTORE        hNVStore,                      // in
    UINT32              offset,                        // in
    UINT32*             ulDataLength,                  // in, out
    BYTE**              rgbDataRead                    // out
);


// DAA Utility functions (optional, do not require a TPM or TCS)
TSPICALL Tspi_DAA_IssuerKeyVerify
(
    TSS_HDAA_CREDENTIAL           hDAACredential,                // in
    TSS_HDAA_ISSUER_KEY           hIssuerKey,                    // in
    TSS_BOOL*                     isCorrect                      // out
);

TSPICALL Tspi_DAA_Issuer_GenerateKey
(
    TSS_HDAA_ISSUER_KEY           hIssuerKey,                    // in
    UINT32                        issuerBaseNameLength,          // in
    BYTE*                         issuerBaseName                 // in
);

TSPICALL Tspi_DAA_Issuer_InitCredential
(
    TSS_HDAA_ISSUER_KEY           hIssuerKey,                    // in
    TSS_HKEY                      issuerAuthPK,                  // in
    TSS_DAA_IDENTITY_PROOF*       identityProof,                 // in
    UINT32                        capitalUprimeLength,           // in
    BYTE*                         capitalUprime,                 // in
    UINT32                        daaCounter,                    // in
    UINT32*                       nonceIssuerLength,             // out
    BYTE**                        nonceIssuer,                   // out
    UINT32*                       authenticationChallengeLength, // out
    BYTE**                        authenticationChallenge,       // out
    UINT32*                       joinSessionLength,             // out
    BYTE**                        joinSession                    // out
);

TSPICALL Tspi_DAA_Issuer_IssueCredential
(
    TSS_HDAA_ISSUER_KEY           hIssuerKey,                    // in
    TSS_DAA_CREDENTIAL_REQUEST*   credentialRequest,             // in
    UINT32                        issuerJoinSessionLength,       // in
    BYTE*                         issuerJoinSession,             // in
    TSS_DAA_CRED_ISSUER**         credIssuer                     // out
);

TSPICALL Tspi_DAA_Verifier_Init
(
    TSS_HDAA_CREDENTIAL           hDAACredential,                // in
    UINT32*                       nonceVerifierLength,           // out
    BYTE**                        nonceVerifier,                 // out
    UINT32*                       baseNameLength,                // out
    BYTE**                        baseName                       // out
);

TSPICALL Tspi_DAA_VerifySignature
(
    TSS_HDAA_CREDENTIAL           hDAACredential,                // in
    TSS_HDAA_ISSUER_KEY           hIssuerKey,                    // in
    TSS_HDAA_ARA_KEY              hARAKey,                       // in
    TSS_HHASH                     hARACondition,                 // in
    UINT32                        attributesLength,              // in
    UINT32                        attributesLength2,             // in
    BYTE**                        attributes,                    // in
    UINT32                        verifierNonceLength,           // in
    BYTE*                         verifierNonce,                 // in
    UINT32                        verifierBaseNameLength,        // in
    BYTE*                         verifierBaseName,              // in
    TSS_HOBJECT                   signData,                      // in
    TSS_DAA_SIGNATURE*            daaSignature,                  // in
    TSS_BOOL*                     isCorrect                      // out
);

TSPICALL Tspi_DAA_ARA_GenerateKey
(
    TSS_HDAA_ISSUER_KEY           hIssuerKey,                    // in
    TSS_HDAA_ARA_KEY              hARAKey                        // in
);

TSPICALL Tspi_DAA_ARA_RevokeAnonymity
(
    TSS_HDAA_ARA_KEY              hARAKey,                       // in
    TSS_HHASH                     hARACondition,                 // in
    TSS_HDAA_ISSUER_KEY           hIssuerKey,                    // in
    TSS_DAA_PSEUDONYM_ENCRYPTED*  encryptedPseudonym,            // in
    TSS_DAA_PSEUDONYM_PLAIN**     pseudonym                      // out
);



// Callback typedefs
typedef TSS_RESULT (*Tspicb_CallbackHMACAuth)
(
    PVOID            lpAppData,          // in
    TSS_HOBJECT      hAuthorizedObject,  // in
    TSS_BOOL         ReturnOrVerify,     // in
    UINT32           ulPendingFunction,  // in
    TSS_BOOL         ContinueUse,        // in
    UINT32           ulSizeNonces,       // in
    BYTE*            rgbNonceEven,       // in
    BYTE*            rgbNonceOdd,        // in
    BYTE*            rgbNonceEvenOSAP,   // in
    BYTE*            rgbNonceOddOSAP,    // in
    UINT32           ulSizeDigestHmac,   // in
    BYTE*            rgbParamDigest,     // in
    BYTE*            rgbHmacData         // in, out
);

typedef TSS_RESULT (*Tspicb_CallbackXorEnc)
(
   PVOID            lpAppData,            // in
   TSS_HOBJECT      hOSAPObject,          // in
   TSS_HOBJECT      hObject,              // in
   TSS_FLAG         PurposeSecret,        // in
   UINT32           ulSizeNonces,         // in
   BYTE*            rgbNonceEven,         // in
   BYTE*            rgbNonceOdd,          // in
   BYTE*            rgbNonceEvenOSAP,     // in
   BYTE*            rgbNonceOddOSAP,      // in
   UINT32           ulSizeEncAuth,        // in
   BYTE*            rgbEncAuthUsage,      // out
   BYTE*            rgbEncAuthMigration   // out
);

typedef TSS_RESULT (*Tspicb_CallbackTakeOwnership)
(
   PVOID            lpAppData,         // in
   TSS_HOBJECT      hObject,           // in
   TSS_HKEY         hObjectPubKey,     // in
   UINT32           ulSizeEncAuth,     // in
   BYTE*            rgbEncAuth         // out
);

typedef TSS_RESULT (*Tspicb_CallbackSealxMask)
(
    PVOID            lpAppData,        // in
    TSS_HKEY         hKey,             // in
    TSS_HENCDATA     hEncData,         // in
    TSS_ALGORITHM_ID algID,            // in
    UINT32           ulSizeNonces,     // in
    BYTE*            rgbNonceEven,     // in
    BYTE*            rgbNonceOdd,      // in
    BYTE*            rgbNonceEvenOSAP, // in
    BYTE*            rgbNonceOddOSAP,  // in
    UINT32           ulDataLength,     // in
    BYTE*            rgbDataToMask,    // in
    BYTE*            rgbMaskedData     // out
);

typedef TSS_RESULT (*Tspicb_CallbackChangeAuthAsym)
(
   PVOID            lpAppData,        // in
   TSS_HOBJECT      hObject,          // in
   TSS_HKEY         hObjectPubKey,    // in
   UINT32           ulSizeEncAuth,    // in
   UINT32           ulSizeAuthLink,   // in
   BYTE*            rgbEncAuth,       // out
   BYTE*            rgbAuthLink       // out
);

typedef TSS_RESULT (*Tspicb_CollateIdentity)
(
   PVOID            lpAppData,                      // in
   UINT32           ulTCPAPlainIdentityProofLength, // in
   BYTE*            rgbTCPAPlainIdentityProof,      // in
   TSS_ALGORITHM_ID algID,                          // in
   UINT32           ulSessionKeyLength,             // out
   BYTE*            rgbSessionKey,                  // out
   UINT32*          pulTCPAIdentityProofLength,     // out
   BYTE*            rgbTCPAIdentityProof            // out
);


typedef TSS_RESULT (*Tspicb_ActivateIdentity)
(
   PVOID            lpAppData,                    // in
   UINT32           ulSessionKeyLength,           // in
   BYTE*            rgbSessionKey,                // in
   UINT32           ulSymCAAttestationBlobLength, // in
   BYTE*            rgbSymCAAttestationBlob,      // in
   UINT32*          pulCredentialLength,          // out
   BYTE*            rgbCredential                 // out
);


typedef TSS_RESULT (*Tspicb_DAA_Sign)
(
    PVOID                        lpAppData,                 // in
    TSS_HDAA_ISSUER_KEY          daaPublicKey,              // in
    UINT32                       gammasLength,              // in
    BYTE**                       gammas,                    // in
    UINT32                       attributesLength,          // in
    BYTE**                       attributes,                // in
    UINT32                       randomAttributesLength,    // in
    BYTE**                       randomAttributes,          // in
    UINT32                       attributeCommitmentsLength,// in
    TSS_DAA_ATTRIB_COMMIT*       attributeCommitments,      // in
    TSS_DAA_ATTRIB_COMMIT*       attributeCommitmentsProof, // in
    TSS_DAA_PSEUDONYM_PLAIN*     pseudonym,                 // in
    TSS_DAA_PSEUDONYM_PLAIN*     pseudonymTilde,            // in
    TSS_DAA_PSEUDONYM_ENCRYPTED* pseudonymEncrypted,        // in
    TSS_DAA_PSEUDONYM_ENCRYPTED* pseudonymEncProof,         // in
    TSS_DAA_SIGN_CALLBACK**      additionalProof            // out
);

typedef TSS_RESULT (*Tspicb_DAA_VerifySignature)
(
    PVOID                        lpAppData,                 // in
    UINT32                       challengeLength,           // in
    BYTE*                        challenge,                 // in
    TSS_DAA_SIGN_CALLBACK*       additionalProof,           // in
    TSS_HDAA_ISSUER_KEY          daaPublicKey,              // in
    UINT32                       gammasLength,              // in
    BYTE**                       gammas,                    // in
    UINT32                       sAttributesLength,         // in
    BYTE**                       sAttributes,               // in
    UINT32                       attributeCommitmentsLength,// in
    TSS_DAA_ATTRIB_COMMIT*       attributeCommitments,      // in
    TSS_DAA_ATTRIB_COMMIT*       attributeCommitmentsProof, // in
    UINT32                       zetaLength,                // in
    BYTE*                        zeta,                      // in
    UINT32                       sFLength,                  // in
    BYTE*                        sF,                        // in
    TSS_DAA_PSEUDONYM*           pseudonym,                 // in
    TSS_DAA_PSEUDONYM*           pseudonymProof,            // in
    TSS_BOOL*                    isCorrect                  // out
);


#if defined ( __cplusplus )
}
#endif /* __cplusplus */


#endif /* _TSPI_H_ */
