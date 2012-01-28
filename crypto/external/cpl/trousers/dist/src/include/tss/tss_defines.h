/*++
 
Global defines for TSS.

--*/

#ifndef __TSS_DEFINES_H__
#define __TSS_DEFINES_H__

#include <tss/platform.h>
#include <tss/tpm.h>


//////////////////////////////////////////////////////////////////////////
// Object types:
//////////////////////////////////////////////////////////////////////////

//
// definition of the object types that can be created via CreateObject
//
#define   TSS_OBJECT_TYPE_POLICY    (0x01)      // Policy object
#define   TSS_OBJECT_TYPE_RSAKEY    (0x02)      // RSA-Key object
#define   TSS_OBJECT_TYPE_ENCDATA   (0x03)      // Encrypted data object
#define   TSS_OBJECT_TYPE_PCRS      (0x04)      // PCR composite object
#define   TSS_OBJECT_TYPE_HASH      (0x05)      // Hash object
#define   TSS_OBJECT_TYPE_DELFAMILY (0x06)      // Delegation Family object
#define   TSS_OBJECT_TYPE_NV        (0x07)      // NV object
#define   TSS_OBJECT_TYPE_MIGDATA   (0x08)      // CMK Migration data object
#define   TSS_OBJECT_TYPE_DAA_CERTIFICATE (0x09) // DAA credential
#define   TSS_OBJECT_TYPE_DAA_ISSUER_KEY  (0x0a) // DAA cred. issuer keypair
#define   TSS_OBJECT_TYPE_DAA_ARA_KEY     (0x0b) // DAA anonymity revocation
                                                 // authority keypair


//////////////////////////////////////////////////////////////////////////
// CreateObject: Flags
//////////////////////////////////////////////////////////////////////////


//************************************
// Flags for creating RSAKEY object: *
//************************************

//
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//                                                              |x x|Auth
//                                                            |x|    Volatility
//                                                          |x|      Migration
//                                                  |x x x x|        Type
//                                          |x x x x|                Size
//                                      |x x|                        CMK
//                                |x x x|                            Version
//              |0 0 0 0 0 0 0 0 0|                                  Reserved
//  |x x x x x x|                                                    Fixed Type
//

//  Authorization:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//
//   Never                                                      |0 0|
//   Always                                                     |0 1|
//   Private key always                                         |1 0|
//
#define   TSS_KEY_NO_AUTHORIZATION            (0x00000000) // no auth needed
                                                           // for this key
#define   TSS_KEY_AUTHORIZATION               (0x00000001) // key needs auth
                                                           // for all ops
#define   TSS_KEY_AUTHORIZATION_PRIV_USE_ONLY (0x00000002) // key needs auth
                                                           // for privkey ops,
                                                           // noauth for pubkey

//
//  Volatility
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//
//   Non Volatile                                             |0|
//   Volatile                                                 |1|
//
#define    TSS_KEY_NON_VOLATILE      (0x00000000)   // Key is non-volatile
#define    TSS_KEY_VOLATILE          (0x00000004)   // Key is volatile

//
//  Migration
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//
//   Non Migratable                                         |0|
//   Migratable                                             |1|
//
#define   TSS_KEY_NOT_MIGRATABLE     (0x00000000)   // key is not migratable
#define   TSS_KEY_MIGRATABLE         (0x00000008)   // key is migratable

//
//  Usage
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//
//   Default (Legacy)                               |0 0 0 0|
//   Signing                                        |0 0 0 1|
//   Storage                                        |0 0 1 0|
//   Identity                                       |0 0 1 1|
//   AuthChange                                     |0 1 0 0|
//   Bind                                           |0 1 0 1|
//   Legacy                                         |0 1 1 0|
//
#define   TSS_KEY_TYPE_DEFAULT    (0x00000000)   // indicate a default key
                                                 // (Legacy-Key)
#define   TSS_KEY_TYPE_SIGNING    (0x00000010)   // indicate a signing key
#define   TSS_KEY_TYPE_STORAGE    (0x00000020)   // used as storage key
#define   TSS_KEY_TYPE_IDENTITY   (0x00000030)   // indicate an idendity key
#define   TSS_KEY_TYPE_AUTHCHANGE (0x00000040)   // indicate an ephemeral key
#define   TSS_KEY_TYPE_BIND       (0x00000050)   // indicate a key for TPM_Bind
#define   TSS_KEY_TYPE_LEGACY     (0x00000060)   // indicate a key that can
                                                 // perform signing and binding
#define   TSS_KEY_TYPE_MIGRATE    (0x00000070)   // indicate a key that can
                                                 // act as a CMK MA
#define   TSS_KEY_TYPE_BITMASK    (0x000000F0)   // mask to extract key type

//
//  Key size
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//
// DEFAULT                                  |0 0 0 0|
//   512                                    |0 0 0 1|
//  1024                                    |0 0 1 0|
//  2048                                    |0 0 1 1|
//  4096                                    |0 1 0 0|
//  8192                                    |0 1 0 1|
// 16384                                    |0 1 1 0|
//
#define TSS_KEY_SIZE_DEFAULT (UINT32)(0x00000000) // indicate tpm-specific size
#define TSS_KEY_SIZE_512     (UINT32)(0x00000100) // indicate a 512-bit key
#define TSS_KEY_SIZE_1024    (UINT32)(0x00000200) // indicate a 1024-bit key
#define TSS_KEY_SIZE_2048    (UINT32)(0x00000300) // indicate a 2048-bit key
#define TSS_KEY_SIZE_4096    (UINT32)(0x00000400) // indicate a 4096-bit key
#define TSS_KEY_SIZE_8192    (UINT32)(0x00000500) // indicate a 8192-bit key
#define TSS_KEY_SIZE_16384   (UINT32)(0x00000600) // indicate a 16384-bit key
#define TSS_KEY_SIZE_BITMASK (UINT32)(0x00000F00) // mask to extract key size

//
//  Certified Migratability
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//
// DEFAULT                              |0 0|
// Not Certified Migratable             |0 0|
// Certified Migratable                 |0 1|
//
#define TSS_KEY_NOT_CERTIFIED_MIGRATABLE (UINT32)(0x00000000)
#define TSS_KEY_CERTIFIED_MIGRATABLE     (UINT32)(0x00001000)

//
//  Specification version
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//
// Context default                |0 0 0|
// TPM_KEY 1.1b key               |0 0 1|
// TPM_KEY12 1.2 key              |0 1 0|
//
#define TSS_KEY_STRUCT_DEFAULT            (UINT32)(0x00000000)
#define TSS_KEY_STRUCT_KEY                (UINT32)(0x00004000)
#define TSS_KEY_STRUCT_KEY12              (UINT32)(0x00008000)
#define TSS_KEY_STRUCT_BITMASK            (UINT32)(0x0001C000)


//
//  fixed KeyTypes (templates)
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//
//  |0 0 0 0 0 0|                             Empty Key
//  |0 0 0 0 0 1|                             Storage Root Key
//
#define   TSS_KEY_EMPTY_KEY (0x00000000) // no TPM key template
                                         // (empty TSP key object)
#define   TSS_KEY_TSP_SRK   (0x04000000) // use a TPM SRK template
                                         // (TSP key object for SRK)
#define   TSS_KEY_TEMPLATE_BITMASK (0xFC000000) // bitmask to extract key
                                                // template


//*************************************
// Flags for creating ENCDATA object: *
//*************************************

//
//  Type
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//
//   Seal                                                     |0 0 1|
//   Bind                                                     |0 1 0|
//   Legacy                                                   |0 1 1|
//
//   ENCDATA Reserved:
//  |x x x x x x x x x x x x x x x x x x x x x x x x x x x x x|
//
#define   TSS_ENCDATA_SEAL     (0x00000001)   // data for seal operation
#define   TSS_ENCDATA_BIND     (0x00000002)   // data for bind operation
#define   TSS_ENCDATA_LEGACY   (0x00000003)   // data for legacy bind operation


//**********************************
// Flags for creating HASH object: *
//**********************************

//
//  Algorithm
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//
//   DEFAULT               
//  |0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0|
//   SHA1
//  |0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1|
//   OTHER
//  |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
//
#define   TSS_HASH_DEFAULT    (0x00000000)   // Default hash algorithm
#define   TSS_HASH_SHA1       (0x00000001)   // SHA-1 with 20 bytes
#define   TSS_HASH_OTHER      (0xFFFFFFFF)   // Not-specified hash algorithm


//************************************
// Flags for creating POLICY object: *
//************************************

//
//  Type
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//
//   Usage                                                    |0 0 1|
//   Migration                                                |0 1 0|
//   Operator                                                 |0 1 1|
//
//   POLICY Reserved:
//  |x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x|

#define   TSS_POLICY_USAGE         (0x00000001)   // usage policy object
#define   TSS_POLICY_MIGRATION     (0x00000002)   // migration policy object
#define   TSS_POLICY_OPERATOR      (0x00000003)   // migration policy object


//******************************************
// Flags for creating PCRComposite object: *
//******************************************

//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
//                                                              |x x| Struct
//  |x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x|     Reserved
//

//  PCRComposite Version:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   ---------------------------------------------------------------
// TPM_PCR_DEFAULT                                            |0 0 0|
// TPM_PCR_INFO                                               |0 0 1|
// TPM_PCR_INFO_LONG                                          |0 1 0|
// TPM_PCR_INFO_SHORT                                         |0 1 1|
//

#define   TSS_PCRS_STRUCT_DEFAULT    (0x00000000) // depends on context
#define   TSS_PCRS_STRUCT_INFO       (0x00000001) // TPM_PCR_INFO
#define   TSS_PCRS_STRUCT_INFO_LONG  (0x00000002) // TPM_PCR_INFO_LONG
#define   TSS_PCRS_STRUCT_INFO_SHORT (0x00000003) // TPM_PCR_INFO_SHORT



//////////////////////////////////////////////////////////////////////////
// Attribute Flags, Subflags, and Values
//////////////////////////////////////////////////////////////////////////


//******************
// Context object: *
//******************

//
// Attributes
//
#define TSS_TSPATTRIB_CONTEXT_SILENT_MODE        (0x00000001)
                                                    // dialog display control
#define TSS_TSPATTRIB_CONTEXT_MACHINE_NAME       (0x00000002)
                                                    // remote machine name
#define TSS_TSPATTRIB_CONTEXT_VERSION_MODE       (0x00000003)
                                                    // context version
#define TSS_TSPATTRIB_CONTEXT_TRANSPORT          (0x00000004)
                                                    // transport control
#define TSS_TSPATTRIB_CONTEXT_CONNECTION_VERSION (0x00000005)
                                                    // connection version
#define TSS_TSPATTRIB_SECRET_HASH_MODE           (0x00000006)
                                                    // flag indicating whether
                                                    // NUL is included in the
                                                    // hash of the password
//
// SubFlags for Flag TSS_TSPATTRIB_CONTEXT_TRANSPORT
//
#define   TSS_TSPATTRIB_CONTEXTTRANS_CONTROL   (0x00000008)
#define   TSS_TSPATTRIB_CONTEXTTRANS_MODE      (0x00000010)

//
// Values for the TSS_TSPATTRIB_CONTEXT_SILENT_MODE attribute
//
#define   TSS_TSPATTRIB_CONTEXT_NOT_SILENT (0x00000000) // TSP dialogs enabled
#define   TSS_TSPATTRIB_CONTEXT_SILENT     (0x00000001) // TSP dialogs disabled

//
// Values for the TSS_TSPATTRIB_CONTEXT_VERSION_MODE attribute
//
#define   TSS_TSPATTRIB_CONTEXT_VERSION_AUTO (0x00000001)
#define   TSS_TSPATTRIB_CONTEXT_VERSION_V1_1 (0x00000002)
#define   TSS_TSPATTRIB_CONTEXT_VERSION_V1_2 (0x00000003)

//
// Values for the subflag TSS_TSPATTRIB_CONTEXT_TRANS_CONTROL
//
#define   TSS_TSPATTRIB_DISABLE_TRANSPORT      (0x00000016)
#define   TSS_TSPATTRIB_ENABLE_TRANSPORT       (0x00000032)

//
// Values for the subflag TSS_TSPATTRIB_CONTEXT_TRANS_MODE
//
#define   TSS_TSPATTRIB_TRANSPORT_NO_DEFAULT_ENCRYPTION (0x00000000)
#define   TSS_TSPATTRIB_TRANSPORT_DEFAULT_ENCRYPTION    (0x00000001)
#define   TSS_TSPATTRIB_TRANSPORT_AUTHENTIC_CHANNEL     (0x00000002)
#define   TSS_TSPATTRIB_TRANSPORT_EXCLUSIVE             (0x00000004)
#define   TSS_TSPATTRIB_TRANSPORT_STATIC_AUTH           (0x00000008)

//
// Values for the TSS_TSPATTRIB_CONTEXT_CONNECTION_VERSION attribute
//
#define TSS_CONNECTION_VERSION_1_1                      (0x00000001)
#define TSS_CONNECTION_VERSION_1_2                      (0x00000002)


//
// Subflags of TSS_TSPATTRIB_SECRET_HASH_MODE
//
#define TSS_TSPATTRIB_SECRET_HASH_MODE_POPUP     (0x00000001)

//
// Values for TSS_TSPATTRIB_SECRET_HASH_MODE_POPUP subflag
//
#define TSS_TSPATTRIB_HASH_MODE_NOT_NULL         (0x00000000)
#define TSS_TSPATTRIB_HASH_MODE_NULL             (0x00000001)


// *************
// TPM object: *
// *************

//
// Attributes:
//
#define TSS_TSPATTRIB_TPM_CALLBACK_COLLATEIDENTITY  0x00000001
#define TSS_TSPATTRIB_TPM_CALLBACK_ACTIVATEIDENTITY 0x00000002
#define TSS_TSPATTRIB_TPM_ORDINAL_AUDIT_STATUS      0x00000003
#define TSS_TSPATTRIB_TPM_CREDENTIAL                0x00001000

//
// Subflags for TSS_TSPATTRIB_TPM_ORDINAL_AUDIT_STATUS
//
#define TPM_CAP_PROP_TPM_CLEAR_ORDINAL_AUDIT        0x00000000
#define TPM_CAP_PROP_TPM_SET_ORDINAL_AUDIT          0x00000001

//
// Subflags for TSS_TSPATTRIB_TPM_CREDENTIAL
//
#define TSS_TPMATTRIB_EKCERT                        0x00000001
#define TSS_TPMATTRIB_TPM_CC                        0x00000002
#define TSS_TPMATTRIB_PLATFORMCERT                  0x00000003
#define TSS_TPMATTRIB_PLATFORM_CC                   0x00000004


//*****************
// Policy object: *
//*****************

//
// Attributes
//
#define TSS_TSPATTRIB_POLICY_CALLBACK_HMAC           (0x00000080)
                                        // enable/disable callback function

#define TSS_TSPATTRIB_POLICY_CALLBACK_XOR_ENC        (0x00000100)
                                        // enable/disable callback function

#define TSS_TSPATTRIB_POLICY_CALLBACK_TAKEOWNERSHIP  (0x00000180)
                                        // enable/disable callback function

#define TSS_TSPATTRIB_POLICY_CALLBACK_CHANGEAUTHASYM (0x00000200)
                                        // enable/disable callback function

#define TSS_TSPATTRIB_POLICY_SECRET_LIFETIME         (0x00000280)
                                        // set lifetime mode for policy secret

#define TSS_TSPATTRIB_POLICY_POPUPSTRING             (0x00000300)
                                        // set a NULL terminated UNICODE string
                                        // which is displayed in the TSP policy
                                        // popup dialog
#define TSS_TSPATTRIB_POLICY_CALLBACK_SEALX_MASK     (0x00000380)
                                        // enable/disable callback function
#if 0
/* This attribute flag is defined earlier with the context attributes.
 * It is valid for both context and policy objects. It is copied
 * here as a reminder to avoid collisions.
 */
#define TSS_TSPATTRIB_SECRET_HASH_MODE               (0x00000006)
                                                    // flag indicating whether
                                                    // NUL is included in the
                                                    // hash of the password
#endif


#define TSS_TSPATTRIB_POLICY_DELEGATION_INFO         (0x00000001)
#define TSS_TSPATTRIB_POLICY_DELEGATION_PCR          (0x00000002)

//
// SubFlags for Flag TSS_TSPATTRIB_POLICY_SECRET_LIFETIME
//
#define TSS_SECRET_LIFETIME_ALWAYS  (0x00000001) // secret will not be
                                                 // invalidated
#define TSS_SECRET_LIFETIME_COUNTER (0x00000002) // secret lifetime
                                                 // controlled by counter
#define TSS_SECRET_LIFETIME_TIMER   (0x00000003) // secret lifetime
                                                 // controlled by time
#define TSS_TSPATTRIB_POLSECRET_LIFETIME_ALWAYS  TSS_SECRET_LIFETIME_ALWAYS
#define TSS_TSPATTRIB_POLSECRET_LIFETIME_COUNTER TSS_SECRET_LIFETIME_COUNTER
#define TSS_TSPATTRIB_POLSECRET_LIFETIME_TIMER   TSS_SECRET_LIFETIME_TIMER

// Alternate names misspelled in the 1.1 TSS spec.
#define TSS_TSPATTRIB_POLICYSECRET_LIFETIME_ALWAYS  TSS_SECRET_LIFETIME_ALWAYS
#define TSS_TSPATTRIB_POLICYSECRET_LIFETIME_COUNTER TSS_SECRET_LIFETIME_COUNTER
#define TSS_TSPATTRIB_POLICYSECRET_LIFETIME_TIMER   TSS_SECRET_LIFETIME_TIMER

//
// Subflags of TSS_TSPATTRIB_POLICY_DELEGATION_INFO
//
#define TSS_TSPATTRIB_POLDEL_TYPE                (0x00000001)
#define TSS_TSPATTRIB_POLDEL_INDEX               (0x00000002)
#define TSS_TSPATTRIB_POLDEL_PER1                (0x00000003)
#define TSS_TSPATTRIB_POLDEL_PER2                (0x00000004)
#define TSS_TSPATTRIB_POLDEL_LABEL               (0x00000005)
#define TSS_TSPATTRIB_POLDEL_FAMILYID            (0x00000006)
#define TSS_TSPATTRIB_POLDEL_VERCOUNT            (0x00000007)
#define TSS_TSPATTRIB_POLDEL_OWNERBLOB           (0x00000008)
#define TSS_TSPATTRIB_POLDEL_KEYBLOB             (0x00000009)

//
// Subflags of TSS_TSPATTRIB_POLICY_DELEGATION_PCR
//
#define TSS_TSPATTRIB_POLDELPCR_LOCALITY         (0x00000001)
#define TSS_TSPATTRIB_POLDELPCR_DIGESTATRELEASE  (0x00000002)
#define TSS_TSPATTRIB_POLDELPCR_SELECTION        (0x00000003)

//
// Values for the Policy TSS_TSPATTRIB_POLDEL_TYPE attribute
//
#define TSS_DELEGATIONTYPE_NONE                  (0x00000001)
#define TSS_DELEGATIONTYPE_OWNER                 (0x00000002)
#define TSS_DELEGATIONTYPE_KEY                   (0x00000003)



//
//  Flags used for the 'mode' parameter in Tspi_Policy_SetSecret()
//
#define TSS_SECRET_MODE_NONE     (0x00000800) // No authorization will be
                                              // processed
#define TSS_SECRET_MODE_SHA1     (0x00001000) // Secret string will not be
                                              // touched by TSP 
#define TSS_SECRET_MODE_PLAIN    (0x00001800) // Secret string will be hashed
                                              // using SHA1
#define TSS_SECRET_MODE_POPUP    (0x00002000) // TSS SP will ask for a secret
#define TSS_SECRET_MODE_CALLBACK (0x00002800) // Application has to provide a
                                              // call back function



//******************
// EncData object: *
//******************

//
// Attributes
//
#define TSS_TSPATTRIB_ENCDATA_BLOB     (0x00000008)
#define TSS_TSPATTRIB_ENCDATA_PCR      (0x00000010)
#define TSS_TSPATTRIB_ENCDATA_PCR_LONG (0x00000018)
#define TSS_TSPATTRIB_ENCDATA_SEAL     (0x00000020)

//
// SubFlags for Flag TSS_TSPATTRIB_ENCDATA_BLOB
//
#define TSS_TSPATTRIB_ENCDATABLOB_BLOB   (0x00000001)   // encrypted data blob

//
// SubFlags for Flag TSS_TSPATTRIB_ENCDATA_PCR
//
#define TSS_TSPATTRIB_ENCDATAPCR_DIGEST_ATCREATION       (0x00000002)
#define TSS_TSPATTRIB_ENCDATAPCR_DIGEST_ATRELEASE        (0x00000003)
#define TSS_TSPATTRIB_ENCDATAPCR_SELECTION               (0x00000004)
// support typo from 1.1 headers
#define TSS_TSPATTRIB_ENCDATAPCR_DIGEST_RELEASE \
                          TSS_TSPATTRIB_ENCDATAPCR_DIGEST_ATRELEASE

#define TSS_TSPATTRIB_ENCDATAPCRLONG_LOCALITY_ATCREATION (0x00000005)
#define TSS_TSPATTRIB_ENCDATAPCRLONG_LOCALITY_ATRELEASE  (0x00000006)
#define TSS_TSPATTRIB_ENCDATAPCRLONG_CREATION_SELECTION  (0x00000007)
#define TSS_TSPATTRIB_ENCDATAPCRLONG_RELEASE_SELECTION   (0x00000008)
#define TSS_TSPATTRIB_ENCDATAPCRLONG_DIGEST_ATCREATION   (0x00000009)
#define TSS_TSPATTRIB_ENCDATAPCRLONG_DIGEST_ATRELEASE    (0x0000000A)


//
// Attribute subflags TSS_TSPATTRIB_ENCDATA_SEAL
//
#define TSS_TSPATTRIB_ENCDATASEAL_PROTECT_MODE           (0x00000001)

//
// Attribute values for 
//    TSS_TSPATTRIB_ENCDATA_SEAL/TSS_TSPATTRIB_ENCDATASEAL_PROTECT_MODE
//
#define  TSS_TSPATTRIB_ENCDATASEAL_NOPROTECT             (0x00000000)
#define  TSS_TSPATTRIB_ENCDATASEAL_PROTECT               (0x00000001)

// Accounting for typos in original header files
#define  TSS_TSPATTRIB_ENCDATASEAL_NO_PROTECT                                \
                                           TSS_TSPATTRIB_ENCDATASEAL_NOPROTECT

//*************
// NV object: *
//*************

//
// Attributes
//
#define TSS_TSPATTRIB_NV_INDEX                     (0x00000001)
#define TSS_TSPATTRIB_NV_PERMISSIONS               (0x00000002)
#define TSS_TSPATTRIB_NV_STATE                     (0x00000003)
#define TSS_TSPATTRIB_NV_DATASIZE                  (0x00000004)
#define TSS_TSPATTRIB_NV_PCR                       (0x00000005)

#define TSS_TSPATTRIB_NVSTATE_READSTCLEAR          (0x00100000)
#define TSS_TSPATTRIB_NVSTATE_WRITESTCLEAR         (0x00200000)
#define TSS_TSPATTRIB_NVSTATE_WRITEDEFINE          (0x00300000)

#define TSS_TSPATTRIB_NVPCR_READPCRSELECTION       (0x01000000)
#define TSS_TSPATTRIB_NVPCR_READDIGESTATRELEASE    (0x02000000)
#define TSS_TSPATTRIB_NVPCR_READLOCALITYATRELEASE  (0x03000000)
#define TSS_TSPATTRIB_NVPCR_WRITEPCRSELECTION      (0x04000000)
#define TSS_TSPATTRIB_NVPCR_WRITEDIGESTATRELEASE   (0x05000000)
#define TSS_TSPATTRIB_NVPCR_WRITELOCALITYATRELEASE (0x06000000)

/* NV index flags
 *
 * From the TPM spec, Part 2, Section 19.1.
 *
 *        3                   2                   1
 *      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |T|P|U|D| resvd |   Purview     |          Index                |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#define TSS_NV_TPM                (0x80000000) // TPM mfr reserved bit
#define TSS_NV_PLATFORM           (0x40000000) // Platform mfr reserved bit
#define TSS_NV_USER               (0x20000000) // User reserved bit
#define TSS_NV_DEFINED            (0x10000000) // "Defined permanently" flag
#define TSS_NV_MASK_TPM           (0x80000000) // mask to extract 'T'
#define TSS_NV_MASK_PLATFORM      (0x40000000) // mask to extract 'P'
#define TSS_NV_MASK_USER          (0x20000000) // mask to extract 'U'
#define TSS_NV_MASK_DEFINED       (0x10000000) // mask to extract 'D'
#define TSS_NV_MASK_RESERVED      (0x0f000000) // mask to extract reserved bits
#define TSS_NV_MASK_PURVIEW       (0x00ff0000) // mask to extract purview byte
#define TSS_NV_MASK_INDEX         (0x0000ffff) // mask to extract index byte

// This is the index of the NV storage area where the number of sessions
// per locality is stored.
#define TSS_NV_INDEX_SESSIONS     (0x00011101)


//******************
// MigData object: *
//******************

//
// Attributes
//
#define TSS_MIGATTRIB_MIGRATIONBLOB                    (0x00000010)
#define TSS_MIGATTRIB_MIGRATIONTICKET                  (0x00000020)
#define TSS_MIGATTRIB_AUTHORITY_DATA                   (0x00000030)
#define TSS_MIGATTRIB_MIG_AUTH_DATA                    (0x00000040)
#define TSS_MIGATTRIB_TICKET_DATA                      (0x00000050)
#define TSS_MIGATTRIB_PAYLOAD_TYPE                     (0x00000060)

//
// Attribute subflags TSS_MIGATTRIB_MIGRATIONBLOB
//
#define TSS_MIGATTRIB_MIGRATION_XOR_BLOB               (0x00000101)
#define TSS_MIGATTRIB_MIGRATION_REWRAPPED_BLOB         (0x00000102)
#define TSS_MIGATTRIB_MIG_MSALIST_PUBKEY_BLOB          (0x00000103)
#define TSS_MIGATTRIB_MIG_AUTHORITY_PUBKEY_BLOB        (0x00000104)
#define TSS_MIGATTRIB_MIG_DESTINATION_PUBKEY_BLOB      (0x00000105)
#define TSS_MIGATTRIB_MIG_SOURCE_PUBKEY_BLOB           (0x00000106)
#define TSS_MIGATTRIB_MIG_REWRAPPED_BLOB               TSS_MIGATTRIB_MIGRATION_REWRAPPED_BLOB
#define TSS_MIGATTRIB_MIG_XOR_BLOB                     TSS_MIGATTRIB_MIGRATION_XOR_BLOB

//
// Attribute subflags TSS_MIGATTRIB_MIGRATIONTICKET
//
// none

//
// Attribute subflags TSS_MIGATTRIB_AUTHORITY_DATA
//
#define TSS_MIGATTRIB_AUTHORITY_DIGEST                 (0x00000301)
#define TSS_MIGATTRIB_AUTHORITY_APPROVAL_HMAC          (0x00000302)
#define TSS_MIGATTRIB_AUTHORITY_MSALIST                (0x00000303)

//
// Attribute subflags TSS_MIGATTRIB_MIG_AUTH_DATA
//
#define TSS_MIGATTRIB_MIG_AUTH_AUTHORITY_DIGEST        (0x00000401)
#define TSS_MIGATTRIB_MIG_AUTH_DESTINATION_DIGEST      (0x00000402)
#define TSS_MIGATTRIB_MIG_AUTH_SOURCE_DIGEST           (0x00000403)

//
// Attribute subflags TSS_MIGATTRIB_TICKET_DATA
//
#define TSS_MIGATTRIB_TICKET_SIG_DIGEST                (0x00000501)
#define TSS_MIGATTRIB_TICKET_SIG_VALUE                 (0x00000502)
#define TSS_MIGATTRIB_TICKET_SIG_TICKET                (0x00000503)
#define TSS_MIGATTRIB_TICKET_RESTRICT_TICKET           (0x00000504)

//
// Attribute subflags TSS_MIGATTRIB_PAYLOAD_TYPE
//
#define TSS_MIGATTRIB_PT_MIGRATE_RESTRICTED            (0x00000601)
#define TSS_MIGATTRIB_PT_MIGRATE_EXTERNAL              (0x00000602)




//***************
// Hash object: *
//***************

//
// Attributes
//
#define TSS_TSPATTRIB_HASH_IDENTIFIER (0x00001000) // Hash algorithm identifier
#define TSS_TSPATTRIB_ALG_IDENTIFIER  (0x00002000) // ASN.1 alg identifier



//***************
// PCRs object: *
//***************

//
// Attributes
//
#define TSS_TSPATTRIB_PCRS_INFO  (0x00000001) // info 

//
// Subflags for TSS_TSPATTRIB_PCRS_INFO flag
//
#define TSS_TSPATTRIB_PCRSINFO_PCRSTRUCT (0x00000001) // type of pcr struct
                                                      // TSS_PCRS_STRUCT_TYPE_XX

//****************************
// Delegation Family object: *
//****************************

//
// Attributes
//
#define TSS_TSPATTRIB_DELFAMILY_STATE            (0x00000001)
#define TSS_TSPATTRIB_DELFAMILY_INFO             (0x00000002)

// DELFAMILY_STATE sub-attributes
#define TSS_TSPATTRIB_DELFAMILYSTATE_LOCKED      (0x00000001)
#define TSS_TSPATTRIB_DELFAMILYSTATE_ENABLED     (0x00000002)

// DELFAMILY_INFO sub-attributes
#define TSS_TSPATTRIB_DELFAMILYINFO_LABEL        (0x00000003)
#define TSS_TSPATTRIB_DELFAMILYINFO_VERCOUNT     (0x00000004)
#define TSS_TSPATTRIB_DELFAMILYINFO_FAMILYID     (0x00000005)

// Bitmasks for the 'ulFlags' argument to Tspi_TPM_Delegate_CreateDelegation.
// Only one bit used for now.
#define TSS_DELEGATE_INCREMENTVERIFICATIONCOUNT               ((UINT32)1)

// Bitmasks for the 'ulFlags' argument to
// Tspi_TPM_Delegate_CacheOwnerDelegation. Only 1 bit is used for now.
#define TSS_DELEGATE_CACHEOWNERDELEGATION_OVERWRITEEXISTING   ((UINT32)1)



//*************************
// DAA Credential Object: *
//*************************

//
// Attribute flags
//
#define TSS_TSPATTRIB_DAACRED_COMMIT                   (0x00000001)
#define TSS_TSPATTRIB_DAACRED_ATTRIB_GAMMAS            (0x00000002)
#define TSS_TSPATTRIB_DAACRED_CREDENTIAL_BLOB          (0x00000003)
#define TSS_TSPATTRIB_DAACRED_CALLBACK_SIGN            (0x00000004)
#define TSS_TSPATTRIB_DAACRED_CALLBACK_VERIFYSIGNATURE (0x00000005)

//
// Subflags for TSS_TSPATTRIB_DAACRED_COMMIT
// 
#define TSS_TSPATTRIB_DAACOMMIT_NUMBER              (0x00000001)
#define TSS_TSPATTRIB_DAACOMMIT_SELECTION           (0x00000002)
#define TSS_TSPATTRIB_DAACOMMIT_COMMITMENTS         (0x00000003)

//
// Subflags for TSS_TSPATTRIB_DAACRED_ATTRIB_GAMMAS
// 
#define TSS_TSPATTRIB_DAAATTRIBGAMMAS_BLOB          (0xffffffff)



//*************************
// DAA Issuer Key Object: *
//*************************

//
// Attribute flags
//
#define TSS_TSPATTRIB_DAAISSUERKEY_BLOB              (0x00000001)
#define TSS_TSPATTRIB_DAAISSUERKEY_PUBKEY            (0x00000002)

//
// Subflags for TSS_TSPATTRIB_DAAISSUERKEY_BLOB
// 
#define TSS_TSPATTRIB_DAAISSUERKEYBLOB_PUBLIC_KEY     (0x00000001)
#define TSS_TSPATTRIB_DAAISSUERKEYBLOB_SECRET_KEY     (0x00000002)
#define TSS_TSPATTRIB_DAAISSUERKEYBLOB_KEYBLOB        (0x00000003)
#define TSS_TSPATTRIB_DAAISSUERKEYBLOB_PROOF          (0x00000004)

//
// Subflags for TSS_TSPATTRIB_DAAISSUERKEY_PUBKEY
// 
#define TSS_TSPATTRIB_DAAISSUERKEYPUBKEY_NUM_ATTRIBS          (0x00000001)
#define TSS_TSPATTRIB_DAAISSUERKEYPUBKEY_NUM_PLATFORM_ATTRIBS (0x00000002)
#define TSS_TSPATTRIB_DAAISSUERKEYPUBKEY_NUM_ISSUER_ATTRIBS   (0x00000003)



//***************************************
// DAA Anonymity Revocation Key Object: *
//***************************************

//
// Attribute flags
//
#define TSS_TSPATTRIB_DAAARAKEY_BLOB                 (0x00000001)

//
// Subflags for TSS_TSPATTRIB_DAAARAKEY_BLOB
// 
#define TSS_TSPATTRIB_DAAARAKEYBLOB_PUBLIC_KEY     (0x00000001)
#define TSS_TSPATTRIB_DAAARAKEYBLOB_SECRET_KEY     (0x00000002)
#define TSS_TSPATTRIB_DAAARAKEYBLOB_KEYBLOB        (0x00000003)



//
// Structure payload flags for TSS_DAA_PSEUDONYM,
// (TSS_DAA_PSEUDONYM.payloadFlag)
//
#define TSS_FLAG_DAA_PSEUDONYM_PLAIN                 (0x00000000)
#define TSS_FLAG_DAA_PSEUDONYM_ENCRYPTED             (0x00000001)


//**************
// Key Object: *
//**************

//
// Attribute flags
//
#define TSS_TSPATTRIB_KEY_BLOB       (0x00000040) // key info as blob data
#define TSS_TSPATTRIB_KEY_INFO       (0x00000080) // keyparam info as blob data
#define TSS_TSPATTRIB_KEY_UUID       (0x000000C0) // key UUID info as blob data
#define TSS_TSPATTRIB_KEY_PCR        (0x00000100) // composite digest value for
                                                  // the key
#define TSS_TSPATTRIB_RSAKEY_INFO    (0x00000140) // public key info
#define TSS_TSPATTRIB_KEY_REGISTER   (0x00000180) // register location
#define TSS_TSPATTRIB_KEY_PCR_LONG   (0x000001c0) // PCR_INFO_LONG for the key
#define TSS_TSPATTRIB_KEY_CONTROLBIT (0x00000200) // key control flags
#define TSS_TSPATTRIB_KEY_CMKINFO    (0x00000400) // CMK info

//
// SubFlags for Flag TSS_TSPATTRIB_KEY_BLOB
//
#define TSS_TSPATTRIB_KEYBLOB_BLOB        (0x00000008) // key info using the
                                                       // key blob
#define TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY  (0x00000010) // public key info
                                                       // using the blob
#define TSS_TSPATTRIB_KEYBLOB_PRIVATE_KEY (0x00000028) // encrypted private key
                                                       // blob

//
// SubFlags for Flag TSS_TSPATTRIB_KEY_INFO
//
#define TSS_TSPATTRIB_KEYINFO_SIZE          (0x00000080) // key size in bits
#define TSS_TSPATTRIB_KEYINFO_USAGE         (0x00000100) // key usage info
#define TSS_TSPATTRIB_KEYINFO_KEYFLAGS      (0x00000180) // key flags   
#define TSS_TSPATTRIB_KEYINFO_AUTHUSAGE     (0x00000200) // key auth usage info
#define TSS_TSPATTRIB_KEYINFO_ALGORITHM     (0x00000280) // key algorithm ID
#define TSS_TSPATTRIB_KEYINFO_SIGSCHEME     (0x00000300) // key sig scheme
#define TSS_TSPATTRIB_KEYINFO_ENCSCHEME     (0x00000380) // key enc scheme   
#define TSS_TSPATTRIB_KEYINFO_MIGRATABLE    (0x00000400) // if true then key is
                                                         // migratable
#define TSS_TSPATTRIB_KEYINFO_REDIRECTED    (0x00000480) // key is redirected
#define TSS_TSPATTRIB_KEYINFO_VOLATILE      (0x00000500) // if true key is
                                                         // volatile
#define TSS_TSPATTRIB_KEYINFO_AUTHDATAUSAGE (0x00000580) // if true auth is
                                                         // required
#define TSS_TSPATTRIB_KEYINFO_VERSION       (0x00000600) // version info as TSS
                                                         // version struct
#define TSS_TSPATTRIB_KEYINFO_CMK           (0x00000680) // if true then key
                                                         // is certified
                                                         // migratable
#define TSS_TSPATTRIB_KEYINFO_KEYSTRUCT     (0x00000700) // type of key struct
                                                         // used for this key
                                                         // (TPM_KEY or 
                                                         // TPM_KEY12)
#define TSS_TSPATTRIB_KEYCONTROL_OWNEREVICT (0x00000780) // Get current status
							 // of owner evict flag

//
// SubFlags for Flag TSS_TSPATTRIB_RSAKEY_INFO
//
#define TSS_TSPATTRIB_KEYINFO_RSA_EXPONENT  (0x00001000)
#define TSS_TSPATTRIB_KEYINFO_RSA_MODULUS   (0x00002000)
#define TSS_TSPATTRIB_KEYINFO_RSA_KEYSIZE   (0x00003000)
#define TSS_TSPATTRIB_KEYINFO_RSA_PRIMES    (0x00004000)

//
// SubFlags for Flag TSS_TSPATTRIB_KEY_PCR
//
#define TSS_TSPATTRIB_KEYPCR_DIGEST_ATCREATION  (0x00008000)
#define TSS_TSPATTRIB_KEYPCR_DIGEST_ATRELEASE   (0x00010000)
#define TSS_TSPATTRIB_KEYPCR_SELECTION          (0x00018000)

//
// SubFlags for TSS_TSPATTRIB_KEY_REGISTER
//
#define TSS_TSPATTRIB_KEYREGISTER_USER    (0x02000000)
#define TSS_TSPATTRIB_KEYREGISTER_SYSTEM  (0x04000000)
#define TSS_TSPATTRIB_KEYREGISTER_NO      (0x06000000)

//
// SubFlags for Flag TSS_TSPATTRIB_KEY_PCR_LONG
//
#define TSS_TSPATTRIB_KEYPCRLONG_LOCALITY_ATCREATION (0x00040000) /* UINT32 */
#define TSS_TSPATTRIB_KEYPCRLONG_LOCALITY_ATRELEASE  (0x00080000) /* UINT32 */
#define TSS_TSPATTRIB_KEYPCRLONG_CREATION_SELECTION  (0x000C0000) /* DATA */
#define TSS_TSPATTRIB_KEYPCRLONG_RELEASE_SELECTION   (0x00100000) /* DATA */
#define TSS_TSPATTRIB_KEYPCRLONG_DIGEST_ATCREATION   (0x00140000) /* DATA */
#define TSS_TSPATTRIB_KEYPCRLONG_DIGEST_ATRELEASE    (0x00180000) /* DATA */

//
// SubFlags for Flag TSS_TSPATTRIB_KEY_CMKINFO
//
#define TSS_TSPATTRIB_KEYINFO_CMK_MA_APPROVAL  (0x00000010)
#define TSS_TSPATTRIB_KEYINFO_CMK_MA_DIGEST    (0x00000020)


//
// Attribute Values
//

//
// key size definitions
//
#define TSS_KEY_SIZEVAL_512BIT      (0x0200)
#define TSS_KEY_SIZEVAL_1024BIT     (0x0400)
#define TSS_KEY_SIZEVAL_2048BIT     (0x0800)
#define TSS_KEY_SIZEVAL_4096BIT     (0x1000)
#define TSS_KEY_SIZEVAL_8192BIT     (0x2000)
#define TSS_KEY_SIZEVAL_16384BIT    (0x4000)

//
// key usage definitions
// Values intentionally moved away from corresponding TPM values to avoid
// possible misuse
//
#define TSS_KEYUSAGE_BIND           (0x00)   
#define TSS_KEYUSAGE_IDENTITY       (0x01)   
#define TSS_KEYUSAGE_LEGACY         (0x02)   
#define TSS_KEYUSAGE_SIGN           (0x03)   
#define TSS_KEYUSAGE_STORAGE        (0x04)
#define TSS_KEYUSAGE_AUTHCHANGE     (0x05)
#define TSS_KEYUSAGE_MIGRATE        (0x06)

//
// key flag definitions
//
#define TSS_KEYFLAG_REDIRECTION          (0x00000001)
#define TSS_KEYFLAG_MIGRATABLE           (0x00000002)
#define TSS_KEYFLAG_VOLATILEKEY          (0x00000004)
#define TSS_KEYFLAG_CERTIFIED_MIGRATABLE (0x00000008)

//
//  algorithm ID definitions
//
//  This table defines the algo id's
//  Values intentionally moved away from corresponding TPM values to avoid
//  possible misuse
//
#define   TSS_ALG_RSA               (0x20)
#define   TSS_ALG_DES               (0x21)
#define   TSS_ALG_3DES              (0x22)
#define   TSS_ALG_SHA               (0x23)
#define   TSS_ALG_HMAC              (0x24)
#define   TSS_ALG_AES128            (0x25)
#define   TSS_ALG_AES192            (0x26)
#define   TSS_ALG_AES256            (0x27)
#define   TSS_ALG_XOR               (0x28)
#define   TSS_ALG_MGF1              (0x29)

#define   TSS_ALG_AES               TSS_ALG_AES128

// Special values for 
//   Tspi_Context_GetCapability(TSS_TSPCAP_ALG)
//   Tspi_Context_GetCapability(TSS_TCSCAP_ALG)
#define   TSS_ALG_DEFAULT           (0xfe)
#define   TSS_ALG_DEFAULT_SIZE      (0xff)


//
// key signature scheme definitions
//
#define TSS_SS_NONE                 (0x10)
#define TSS_SS_RSASSAPKCS1V15_SHA1  (0x11)
#define TSS_SS_RSASSAPKCS1V15_DER   (0x12)
#define	TSS_SS_RSASSAPKCS1V15_INFO  (0x13)

//
// key encryption scheme definitions
//
#define TSS_ES_NONE                 (0x10)
#define TSS_ES_RSAESPKCSV15         (0x11)
#define TSS_ES_RSAESOAEP_SHA1_MGF1  (0x12)
#define TSS_ES_SYM_CNT              (0x13)
#define TSS_ES_SYM_OFB              (0x14)
#define TSS_ES_SYM_CBC_PKCS5PAD     (0x15)


//
// persistent storage registration definitions
//
#define TSS_PS_TYPE_USER   (1) // Key is registered persistantly in the user
                               // storage database.
#define TSS_PS_TYPE_SYSTEM (2) // Key is registered persistantly in the system
                               // storage database.

//
// migration scheme definitions
// Values intentionally moved away from corresponding TPM values to avoid
// possible misuse
//
#define TSS_MS_MIGRATE                   (0x20)
#define TSS_MS_REWRAP                    (0x21)
#define TSS_MS_MAINT                     (0x22)
#define TSS_MS_RESTRICT_MIGRATE          (0x23)
#define TSS_MS_RESTRICT_APPROVE_DOUBLE   (0x24)
#define TSS_MS_RESTRICT_MIGRATE_EXTERNAL (0x25)

//
// TPM key authorization
// Values intentionally moved away from corresponding TPM values to avoid
// possible misuse
//
#define TSS_KEYAUTH_AUTH_NEVER         (0x10)
#define TSS_KEYAUTH_AUTH_ALWAYS        (0x11)
#define TSS_KEYAUTH_AUTH_PRIV_USE_ONLY (0x12)


//
// Flags for TPM status information (GetStatus and SetStatus)
//
#define TSS_TPMSTATUS_DISABLEOWNERCLEAR      (0x00000001) // persistent flag
#define TSS_TPMSTATUS_DISABLEFORCECLEAR      (0x00000002) // volatile flag
#define TSS_TPMSTATUS_DISABLED               (0x00000003) // persistent flag
#define TSS_TPMSTATUS_DEACTIVATED            (0x00000004) // volatile flag
#define TSS_TPMSTATUS_OWNERSETDISABLE        (0x00000005) // persistent flag
                                                          // for SetStatus
                                                          // (disable flag) 
#define TSS_TPMSTATUS_SETOWNERINSTALL        (0x00000006) // persistent flag
                                                          // (ownership flag)
#define TSS_TPMSTATUS_DISABLEPUBEKREAD       (0x00000007) // persistent flag
#define TSS_TPMSTATUS_ALLOWMAINTENANCE       (0x00000008) // persistent flag
#define TSS_TPMSTATUS_PHYSPRES_LIFETIMELOCK  (0x00000009) // persistent flag
#define TSS_TPMSTATUS_PHYSPRES_HWENABLE      (0x0000000A) // persistent flag
#define TSS_TPMSTATUS_PHYSPRES_CMDENABLE     (0x0000000B) // persistent flag
#define TSS_TPMSTATUS_PHYSPRES_LOCK          (0x0000000C) // volatile flag
#define TSS_TPMSTATUS_PHYSPRESENCE           (0x0000000D) // volatile flag
#define TSS_TPMSTATUS_PHYSICALDISABLE        (0x0000000E) // persistent flag
                                                          // (SetStatus
                                                          //  disable flag)
#define TSS_TPMSTATUS_CEKP_USED              (0x0000000F) // persistent flag
#define TSS_TPMSTATUS_PHYSICALSETDEACTIVATED (0x00000010) // persistent flag
                                                          // (deactivated flag)
#define TSS_TPMSTATUS_SETTEMPDEACTIVATED     (0x00000011) // volatile flag
                                                          // (deactivated flag)
#define TSS_TPMSTATUS_POSTINITIALISE         (0x00000012) // volatile flag
#define TSS_TPMSTATUS_TPMPOST                (0x00000013) // persistent flag
#define TSS_TPMSTATUS_TPMPOSTLOCK            (0x00000014) // persistent flag
#define TSS_TPMSTATUS_DISABLEPUBSRKREAD      (0x00000016) // persistent flag
#define TSS_TPMSTATUS_MAINTENANCEUSED        (0x00000017) // persistent flag
#define TSS_TPMSTATUS_OPERATORINSTALLED      (0x00000018) // persistent flag
#define TSS_TPMSTATUS_OPERATOR_INSTALLED     (TSS_TPMSTATUS_OPERATORINSTALLED)
#define TSS_TPMSTATUS_FIPS                   (0x00000019) // persistent flag
#define TSS_TPMSTATUS_ENABLEREVOKEEK         (0x0000001A) // persistent flag
#define TSS_TPMSTATUS_ENABLE_REVOKEEK        (TSS_TPMSTATUS_ENABLEREVOKEEK)
#define TSS_TPMSTATUS_NV_LOCK                (0x0000001B) // persistent flag
#define TSS_TPMSTATUS_TPM_ESTABLISHED        (0x0000001C) // persistent flag
#define TSS_TPMSTATUS_RESETLOCK              (0x0000001D) // volatile flag
#define TSS_TPMSTATUS_DISABLE_FULL_DA_LOGIC_INFO (0x0000001D) //persistent flag


//
// Capability flag definitions
//
// TPM capabilities            
//
#define TSS_TPMCAP_ORD                   (0x10)
#define TSS_TPMCAP_ALG                   (0x11)
#define TSS_TPMCAP_FLAG                  (0x12)
#define TSS_TPMCAP_PROPERTY              (0x13)
#define TSS_TPMCAP_VERSION               (0x14)
#define TSS_TPMCAP_VERSION_VAL           (0x15)
#define TSS_TPMCAP_NV_LIST               (0x16)
#define TSS_TPMCAP_NV_INDEX              (0x17)
#define TSS_TPMCAP_MFR                   (0x18)
#define TSS_TPMCAP_SYM_MODE              (0x19)
#define TSS_TPMCAP_HANDLE                (0x1a)
#define TSS_TPMCAP_TRANS_ES              (0x1b)
#define TSS_TPMCAP_AUTH_ENCRYPT          (0x1c)  
#define TSS_TPMCAP_SET_PERM_FLAGS        (0x1d)  // cf. TPM_SET_PERM_FLAGS
#define TSS_TPMCAP_SET_VENDOR            (0x1e)  // cf. TPM_SET_VENDOR
#define TSS_TPMCAP_DA_LOGIC              (0x1f)

//
// Sub-Capability Flags for TSS_TPMCAP_PROPERTY
//
#define TSS_TPMCAP_PROP_PCR                 (0x10)
#define TSS_TPMCAP_PROP_DIR                 (0x11)
#define TSS_TPMCAP_PROP_MANUFACTURER        (0x12)
#define TSS_TPMCAP_PROP_SLOTS               (0x13)
#define TSS_TPMCAP_PROP_KEYS                TSS_TPMCAP_PROP_SLOTS
#define TSS_TPMCAP_PROP_FAMILYROWS          (0x14)
#define TSS_TPMCAP_PROP_DELEGATEROWS        (0x15)
#define TSS_TPMCAP_PROP_OWNER               (0x16)
#define TSS_TPMCAP_PROP_MAXKEYS             (0x18)
#define TSS_TPMCAP_PROP_AUTHSESSIONS        (0x19)
#define TSS_TPMCAP_PROP_MAXAUTHSESSIONS     (0x1a)
#define TSS_TPMCAP_PROP_TRANSESSIONS        (0x1b)
#define TSS_TPMCAP_PROP_MAXTRANSESSIONS     (0x1c)
#define TSS_TPMCAP_PROP_SESSIONS            (0x1d)
#define TSS_TPMCAP_PROP_MAXSESSIONS         (0x1e)
#define TSS_TPMCAP_PROP_CONTEXTS            (0x1f)
#define TSS_TPMCAP_PROP_MAXCONTEXTS         (0x20)
#define TSS_TPMCAP_PROP_DAASESSIONS         (0x21)
#define TSS_TPMCAP_PROP_MAXDAASESSIONS      (0x22)
#define TSS_TPMCAP_PROP_DAA_INTERRUPT       (0x23)
#define TSS_TPMCAP_PROP_COUNTERS            (0x24)
#define TSS_TPMCAP_PROP_MAXCOUNTERS         (0x25)
#define TSS_TPMCAP_PROP_ACTIVECOUNTER       (0x26)
#define TSS_TPMCAP_PROP_MIN_COUNTER         (0x27)
#define TSS_TPMCAP_PROP_TISTIMEOUTS         (0x28)
#define TSS_TPMCAP_PROP_STARTUPEFFECTS      (0x29)
#define TSS_TPMCAP_PROP_MAXCONTEXTCOUNTDIST (0x2a)
#define TSS_TPMCAP_PROP_CMKRESTRICTION      (0x2b)
#define TSS_TPMCAP_PROP_DURATION            (0x2c)
#define TSS_TPMCAP_PROP_MAXNVAVAILABLE      (0x2d)
#define TSS_TPMCAP_PROP_INPUTBUFFERSIZE     (0x2e)
#define TSS_TPMCAP_PROP_REVISION            (0x2f)
#define TSS_TPMCAP_PROP_LOCALITIES_AVAIL    (0x32)

//
// Resource type flags
// Sub-Capability Flags for TSS_TPMCAP_HANDLE
//
#define TSS_RT_KEY                     ((UINT32)0x00000010)
#define TSS_RT_AUTH                    ((UINT32)0x00000020)
#define TSS_RT_TRANS                   ((UINT32)0x00000030)
#define TSS_RT_COUNTER                 ((UINT32)0x00000040)


//
// TSS Core Service Capabilities   
//
#define TSS_TCSCAP_ALG                   (0x00000001)
#define TSS_TCSCAP_VERSION               (0x00000002)
#define TSS_TCSCAP_CACHING               (0x00000003)
#define TSS_TCSCAP_PERSSTORAGE           (0x00000004)
#define TSS_TCSCAP_MANUFACTURER          (0x00000005)
#define TSS_TCSCAP_PLATFORM_CLASS        (0x00000006)
#define TSS_TCSCAP_TRANSPORT             (0x00000007)
#define TSS_TCSCAP_PLATFORM_INFO         (0x00000008)

//
// Sub-Capability Flags TSS-CoreService-Capabilities
//
#define TSS_TCSCAP_PROP_KEYCACHE         (0x00000100)
#define TSS_TCSCAP_PROP_AUTHCACHE        (0x00000101)
#define TSS_TCSCAP_PROP_MANUFACTURER_STR (0x00000102)
#define TSS_TCSCAP_PROP_MANUFACTURER_ID  (0x00000103)
#define TSS_TCSCAP_PLATFORM_VERSION      (0x00001100)
#define TSS_TCSCAP_PLATFORM_TYPE         (0x00001101)
#define TSS_TCSCAP_TRANS_EXCLUSIVE       (0x00002100)
#define TSS_TCSCAP_PROP_HOST_PLATFORM    (0x00003001)
#define TSS_TCSCAP_PROP_ALL_PLATFORMS    (0x00003002)

//
// TSS Service Provider Capabilities      
//
#define TSS_TSPCAP_ALG                   (0x00000010)
#define TSS_TSPCAP_VERSION               (0x00000011)
#define TSS_TSPCAP_PERSSTORAGE           (0x00000012)
#define TSS_TSPCAP_MANUFACTURER          (0x00000013)
#define TSS_TSPCAP_RETURNVALUE_INFO      (0x00000015)
#define TSS_TSPCAP_PLATFORM_INFO         (0x00000016)

// Sub-Capability Flags for TSS_TSPCAP_MANUFACTURER
//
#define TSS_TSPCAP_PROP_MANUFACTURER_STR (0x00000102)
#define TSS_TSPCAP_PROP_MANUFACTURER_ID  (0x00000103)

// Sub-Capability Flags for TSS_TSPCAP_PLATFORM_INFO
//
#define TSS_TSPCAP_PLATFORM_TYPE         (0x00000201)
#define TSS_TSPCAP_PLATFORM_VERSION      (0x00000202)



// Sub-Capability Flags for TSS_TSPCAP_RETURNVALUE_INFO
//
#define TSS_TSPCAP_PROP_RETURNVALUE_INFO (0x00000201)

//
// Event type definitions
//
#define TSS_EV_CODE_CERT                 (0x00000001)
#define TSS_EV_CODE_NOCERT               (0x00000002)
#define TSS_EV_XML_CONFIG                (0x00000003)
#define TSS_EV_NO_ACTION                 (0x00000004)
#define TSS_EV_SEPARATOR                 (0x00000005)
#define TSS_EV_ACTION                    (0x00000006)
#define TSS_EV_PLATFORM_SPECIFIC         (0x00000007)


//
// TSP random number limits
//
#define TSS_TSPCAP_RANDOMLIMIT     (0x00001000)   // Errata: Missing from spec

//
// UUIDs
//
// Errata: This are not in the spec
#define TSS_UUID_SRK  {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 1}} // Storage root key
#define TSS_UUID_SK   {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 2}} // System key
#define TSS_UUID_RK   {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 3}} // roaming key
#define TSS_UUID_CRK  {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 8}} // CMK roaming key
#define TSS_UUID_USK1 {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 4}} // user storage key 1
#define TSS_UUID_USK2 {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 5}} // user storage key 2
#define TSS_UUID_USK3 {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 6}} // user storage key 3
#define TSS_UUID_USK4 {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 7}} // user storage key 4
#define TSS_UUID_USK5 {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 9}} // user storage key 5
#define TSS_UUID_USK6 {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 10}}// user storage key 6

// macro to derive UUIDs for keys whose "OwnerEvict" key is set.
#define TSS_UUID_OWNEREVICT(i) {0, 0, 0, 0, 0, {0, 0, 0, 0, 1, (i)}}


//
// TPM well-known secret
//
#define TSS_WELL_KNOWN_SECRET \
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}


// Values for the "direction" parameters in the Tspi_PcrComposite_XX functions.
#define TSS_PCRS_DIRECTION_CREATION                        ((UINT32)1)
#define TSS_PCRS_DIRECTION_RELEASE                         ((UINT32)2)


//
// TSS blob version definition for ASN.1 blobs
//
#define TSS_BLOB_STRUCT_VERSION                              0x01

//
// TSS blob type definitions for ASN.1 blobs
//
#define TSS_BLOB_TYPE_KEY                                    0x01
#define TSS_BLOB_TYPE_PUBKEY                                 0x02
#define TSS_BLOB_TYPE_MIGKEY                                 0x03
#define TSS_BLOB_TYPE_SEALEDDATA                             0x04
#define TSS_BLOB_TYPE_BOUNDDATA                              0x05
#define TSS_BLOB_TYPE_MIGTICKET                              0x06
#define TSS_BLOB_TYPE_PRIVATEKEY                             0x07
#define TSS_BLOB_TYPE_PRIVATEKEY_MOD1                        0x08
#define TSS_BLOB_TYPE_RANDOM_XOR                             0x09
#define TSS_BLOB_TYPE_CERTIFY_INFO                           0x0A
#define TSS_BLOB_TYPE_KEY_1_2                                0x0B
#define TSS_BLOB_TYPE_CERTIFY_INFO_2                         0x0C
#define TSS_BLOB_TYPE_CMK_MIG_KEY                            0x0D
#define TSS_BLOB_TYPE_CMK_BYTE_STREAM                        0x0E



//
// Values for TPM_CMK_DELEGATE bitmasks
// For now these are exactly the same values as the corresponding
// TPM_CMK_DELEGATE_* bitmasks.
//
#define TSS_CMK_DELEGATE_SIGNING       (((UINT32)1)<<31)
#define TSS_CMK_DELEGATE_STORAGE       (((UINT32)1)<<30)
#define TSS_CMK_DELEGATE_BIND          (((UINT32)1)<<29)
#define TSS_CMK_DELEGATE_LEGACY        (((UINT32)1)<<28)
#define TSS_CMK_DELEGATE_MIGRATE       (((UINT32)1)<<27)


//
// Constants for DAA
//
#define TSS_DAA_LENGTH_N                256             // Length of the RSA Modulus (2048 bits)
#define TSS_DAA_LENGTH_F                13              // Length of the f_i's (information encoded into the certificate, 104 bits)
#define TSS_DAA_LENGTH_E                46              // Length of the e's (exponents, part of certificate, 386 bits)
#define TSS_DAA_LENGTH_E_PRIME          15              // Length of the interval the e's are chosen from (120 bits)
#define TSS_DAA_LENGTH_V                317             // Length of the v's (random value, part of certificate, 2536 bits)
#define TSS_DAA_LENGTH_SAFETY           10              // Length of the security parameter controlling the statistical zero-knowledge property (80 bits)
#define TSS_DAA_LENGTH_HASH     TPM_SHA1_160_HASH_LEN   // Length of the output of the hash function SHA-1 used for the Fiat-Shamir heuristic(160 bits)
#define TSS_DAA_LENGTH_S                128             // Length of the split large exponent for easier computations on the TPM (1024 bits)
#define TSS_DAA_LENGTH_GAMMA            204             // Length of the modulus 'Gamma' (1632 bits)
#define TSS_DAA_LENGTH_RHO              26              // Length of the order 'rho' of the sub group of Z*_Gamma that is used for roggue tagging (208 bits)
#define TSS_DAA_LENGTH_MFG1_GAMMA       214             // Length of the output of MGF1 in conjunction with the modulus Gamma (1712 bits)
#define TSS_DAA_LENGTH_MGF1_AR          25              // Length of the output of MGF1 used for anonymity revocation (200 bits)


#endif // __TSS_DEFINES_H__
