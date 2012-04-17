/*++

TSS error return codes 
 
--*/

#ifndef __TSS_ERROR_H__
#define __TSS_ERROR_H__

#include <tss/platform.h>

//
// error coding scheme for a Microsoft Windows platform -
// refer to the TSS Specification Parts
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------+-----------------------+
//  |Lev|C|R|     Facility          | Layer |         Code          |
//  +---+-+-+-----------------------+-------+-----------------------+
//  | Platform specific coding      | TSS error coding system       |
//  +---+-+-+-----------------------+-------+-----------------------+
//
//      Lev - is the Level code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag  (must actually be set)
//
//      R - is a reserved bit    (unused)
//
//      Facility - is the facility code: TCPA: proposal 0x028
//
//      Code - is the facility's status code
//

//
// definitions for the code level information
//
#define TSS_LEVEL_SUCCESS  0x00     // code level success 
#define TSS_LEVEL_INFO     0x40000000L    // code level information
#define TSS_LEVEL_WARNING  0x80000000L    // code level warning
#define TSS_LEVEL_ERROR    0xC0000000L    // code level error

//
// some defines for the platform specific information
//
#define FACILITY_TSS            0x28L     // facility number for TCPA return codes
#define FACILITY_TSS_CODEPOS   (FACILITY_TSS << 16)  // shift the facility info to the code 
                                                                        // position

#define TSS_CUSTOM_CODEFLAG     0x20000000L    // bit position for the custom flag in 
                                                                        // return code

//
//
// TSS error return codes
//
//
#ifndef TSS_E_BASE
#define TSS_E_BASE    0x00000000L
#endif // TSS_E_BASE
#ifndef TSS_W_BASE
#define TSS_W_BASE    0x00000000L
#endif // TSS_W_BASE
#ifndef TSS_I_BASE
#define TSS_I_BASE    0x00000000L
#endif // TSS_I_BASE

//
// basic error return codes common to all TSS Service Provider Interface methods
// and returned by all TSS SW stack components
//

//
// MessageId: TSS_SUCCESS
//
// MessageText:
//
//  Successful completion of the operation.
//
#define TSS_SUCCESS     (UINT32)(0x00000000L)

//
// MessageId: TSS_E_FAIL
//
// MessageText:
//
//  An internal error has been detected, but the source is unknown.
//
#define TSS_E_FAIL     (UINT32)(TSS_E_BASE + 0x002L)

//
// MessageId: TSS_E_BAD_PARAMETER
//
// MessageText:
//
// One or more parameter is bad.
//
#define TSS_E_BAD_PARAMETER    (UINT32)(TSS_E_BASE + 0x003L)

//
// MessageId: TSS_E_INTERNAL_ERROR
//
// MessageText:
//
//  An internal SW error has been detected.
//
#define TSS_E_INTERNAL_ERROR    (UINT32)(TSS_E_BASE + 0x004L)

//
// MessageId: TSS_E_OUTOFMEMORY
//
// MessageText:
//
// Ran out of memory.
//
#define TSS_E_OUTOFMEMORY    (UINT32)(TSS_E_BASE + 0x005L)

//
// MessageId: TSS_E_NOTIMPL
//
// MessageText:
//
// Not implemented.
//
#define TSS_E_NOTIMPL     (UINT32)(TSS_E_BASE + 0x006L)

//
// MessageId: TSS_E_KEY_ALREADY_REGISTERED
//
// MessageText:
//
//  Key is already registered
//
#define TSS_E_KEY_ALREADY_REGISTERED  (UINT32)(TSS_E_BASE + 0x008L)


//
// MessageId: TSS_E_TPM_UNEXPECTED
//
// MessageText:
//
//  An unexpected TPM error has occurred.
//
#define TSS_E_TPM_UNEXPECTED    (UINT32)(TSS_E_BASE + 0x010L)

//
// MessageId: TSS_E_COMM_FAILURE
//
// MessageText:
//
//  A communications error with the TPM has been detected.
//
#define TSS_E_COMM_FAILURE    (UINT32)(TSS_E_BASE + 0x011L)

//
// MessageId: TSS_E_TIMEOUT
//
// MessageText:
//
//  The operation has timed out.
//
#define TSS_E_TIMEOUT     (UINT32)(TSS_E_BASE + 0x012L)

//
// MessageId: TSS_E_TPM_UNSUPPORTED_FEATURE
//
// MessageText:
//
//  The TPM does not support the requested feature.
//
#define TSS_E_TPM_UNSUPPORTED_FEATURE  (UINT32)(TSS_E_BASE + 0x014L)

//
// MessageId: TSS_E_CANCELED
//
// MessageText:
//
//  The action was canceled by request.
//
#define TSS_E_CANCELED     (UINT32)(TSS_E_BASE + 0x016L)

//
// MessageId: TSS_E_PS_KEY_NOTFOUND
//
// MessageText:
//
// The key cannot be found in the persistent storage database.
//
#define TSS_E_PS_KEY_NOTFOUND    (UINT32)(TSS_E_BASE + 0x020L)
//
// MessageId: TSS_E_PS_KEY_EXISTS
//
// MessageText:
//
// The key already exists in the persistent storage database.
//
#define TSS_E_PS_KEY_EXISTS            (UINT32)(TSS_E_BASE + 0x021L)

//
// MessageId: TSS_E_PS_BAD_KEY_STATE
//
// MessageText:
//
// The key data set not valid in the persistent storage database.
//
#define TSS_E_PS_BAD_KEY_STATE         (UINT32)(TSS_E_BASE + 0x022L)


//
// error codes returned by specific TSS Service Provider Interface methods
// offset TSS_TSPI_OFFSET
//

//
// MessageId: TSS_E_INVALID_OBJECT_TYPE
//
// MessageText:
//
// Object type not valid for this operation.
//
#define TSS_E_INVALID_OBJECT_TYPE   (UINT32)(TSS_E_BASE + 0x101L)

//
// MessageId: TSS_E_NO_CONNECTION
//
// MessageText:
//
// Core Service connection doesn't exist.
//
#define TSS_E_NO_CONNECTION    (UINT32)(TSS_E_BASE + 0x102L)
 
//
// MessageId: TSS_E_CONNECTION_FAILED
//
// MessageText:
//
// Core Service connection failed.
//
#define TSS_E_CONNECTION_FAILED   (UINT32)(TSS_E_BASE + 0x103L)

//
// MessageId: TSS_E_CONNECTION_BROKEN
//
// MessageText:
//
// Communication with Core Service failed.
//
#define TSS_E_CONNECTION_BROKEN   (UINT32)(TSS_E_BASE + 0x104L)

//
// MessageId: TSS_E_HASH_INVALID_ALG
//
// MessageText:
//
// Invalid hash algorithm.
//
#define TSS_E_HASH_INVALID_ALG   (UINT32)(TSS_E_BASE + 0x105L)

//
// MessageId: TSS_E_HASH_INVALID_LENGTH
//
// MessageText:
//
// Hash length is inconsistent with hash algorithm.
//
#define TSS_E_HASH_INVALID_LENGTH   (UINT32)(TSS_E_BASE + 0x106L)

//
// MessageId: TSS_E_HASH_NO_DATA
//
// MessageText:
//
// Hash object has no internal hash value.
//
#define TSS_E_HASH_NO_DATA    (UINT32)(TSS_E_BASE + 0x107L)


//
// MessageId: TSS_E_INVALID_ATTRIB_FLAG
//
// MessageText:
//
// Flag value for attrib-functions inconsistent.
//
#define TSS_E_INVALID_ATTRIB_FLAG   (UINT32)(TSS_E_BASE + 0x109L)

//
// MessageId: TSS_E_INVALID_ATTRIB_SUBFLAG
//
// MessageText:
//
// Subflag value for attrib-functions inconsistent.
//
#define TSS_E_INVALID_ATTRIB_SUBFLAG  (UINT32)(TSS_E_BASE + 0x10AL)

//
// MessageId: TSS_E_INVALID_ATTRIB_DATA
//
// MessageText:
//
// Data for attrib-functions invalid.
//
#define TSS_E_INVALID_ATTRIB_DATA   (UINT32)(TSS_E_BASE + 0x10BL)

//
// MessageId: TSS_E_INVALID_OBJECT_INITFLAG
//
// MessageText:
//
// Wrong flag information for object creation.
// 
// The alternate spelling is supported to be compatible with a typo
// in the 1.1b header files.
//
#define TSS_E_INVALID_OBJECT_INIT_FLAG  (UINT32)(TSS_E_BASE + 0x10CL)
#define TSS_E_INVALID_OBJECT_INITFLAG   TSS_E_INVALID_OBJECT_INIT_FLAG
 
//
// MessageId: TSS_E_NO_PCRS_SET
//
// MessageText:
//
// No PCR register are selected or set.
//
#define TSS_E_NO_PCRS_SET    (UINT32)(TSS_E_BASE + 0x10DL)

//
// MessageId: TSS_E_KEY_NOT_LOADED
//
// MessageText:
//
// The addressed key is currently not loaded.
//
#define TSS_E_KEY_NOT_LOADED    (UINT32)(TSS_E_BASE + 0x10EL)
 
//
// MessageId: TSS_E_KEY_NOT_SET
//
// MessageText:
//
// No key information is currently available.
//
#define TSS_E_KEY_NOT_SET    (UINT32)(TSS_E_BASE + 0x10FL)
      
//
// MessageId: TSS_E_VALIDATION_FAILED
//
// MessageText:
//
// Internal validation of data failed.
//
#define TSS_E_VALIDATION_FAILED   (UINT32)(TSS_E_BASE + 0x110L)

//
// MessageId: TSS_E_TSP_AUTHREQUIRED
//
// MessageText:
//
// Authorization is required.
//
#define TSS_E_TSP_AUTHREQUIRED   (UINT32)(TSS_E_BASE + 0x111L)

//
// MessageId: TSS_E_TSP_AUTH2REQUIRED
//
// MessageText:
//
// Multiple authorization is required.
//
#define TSS_E_TSP_AUTH2REQUIRED   (UINT32)(TSS_E_BASE + 0x112L)

//
// MessageId: TSS_E_TSP_AUTHFAIL
//
// MessageText:
//
// Authorization failed.
//
#define TSS_E_TSP_AUTHFAIL    (UINT32)(TSS_E_BASE + 0x113L)

//
// MessageId: TSS_E_TSP_AUTH2FAIL
//
// MessageText:
//
// Multiple authorization failed.
//
#define TSS_E_TSP_AUTH2FAIL    (UINT32)(TSS_E_BASE + 0x114L)
 
//
// MessageId: TSS_E_KEY_NO_MIGRATION_POLICY
//
// MessageText:
//
// There's no migration policy object set for the addressed key.
//
#define TSS_E_KEY_NO_MIGRATION_POLICY  (UINT32)(TSS_E_BASE + 0x115L) 

//
// MessageId: TSS_E_POLICY_NO_SECRET
//
// MessageText:
//
// No secret information is currently available for the addressed policy object.
//
#define TSS_E_POLICY_NO_SECRET   (UINT32)(TSS_E_BASE + 0x116L)

//
// MessageId: TSS_E_INVALID_OBJ_ACCESS
//
// MessageText:
//
// The operation failed due to an invalid object status.
//
#define TSS_E_INVALID_OBJ_ACCESS   (UINT32)(TSS_E_BASE + 0x117L)

//
// MessageId: TSS_E_INVALID_ENCSCHEME
//
// MessageText:
//
// 
//
#define TSS_E_INVALID_ENCSCHEME   (UINT32)(TSS_E_BASE + 0x118L)


//
// MessageId: TSS_E_INVALID_SIGSCHEME
//
// MessageText:
//
// 
//
#define TSS_E_INVALID_SIGSCHEME   (UINT32)(TSS_E_BASE + 0x119L)

//
// MessageId: TSS_E_ENC_INVALID_LENGTH
//
// MessageText:
//
// 
//
#define TSS_E_ENC_INVALID_LENGTH   (UINT32)(TSS_E_BASE + 0x120L)


//
// MessageId: TSS_E_ENC_NO_DATA
//
// MessageText:
//
// 
//
#define TSS_E_ENC_NO_DATA    (UINT32)(TSS_E_BASE + 0x121L)

//
// MessageId: TSS_E_ENC_INVALID_TYPE
//
// MessageText:
//
// 
//
#define TSS_E_ENC_INVALID_TYPE   (UINT32)(TSS_E_BASE + 0x122L)


//
// MessageId: TSS_E_INVALID_KEYUSAGE
//
// MessageText:
//
// 
//
#define TSS_E_INVALID_KEYUSAGE   (UINT32)(TSS_E_BASE + 0x123L)

//
// MessageId: TSS_E_VERIFICATION_FAILED
//
// MessageText:
//
// 
//
#define TSS_E_VERIFICATION_FAILED   (UINT32)(TSS_E_BASE + 0x124L)

//
// MessageId: TSS_E_HASH_NO_IDENTIFIER
//
// MessageText:
//
// Hash algorithm identifier not set.
//
#define TSS_E_HASH_NO_IDENTIFIER   (UINT32)(TSS_E_BASE + 0x125L)

//
// MessageId: TSS_E_INVALID_HANDLE
//
// MessageText:
//
//  An invalid handle
//
#define TSS_E_INVALID_HANDLE    (UINT32)(TSS_E_BASE + 0x126L)

//
// MessageId: TSS_E_SILENT_CONTEXT
//
// MessageText:
//
//  A silent context requires user input
//
#define TSS_E_SILENT_CONTEXT           (UINT32)(TSS_E_BASE + 0x127L)

//
// MessageId: TSS_E_EK_CHECKSUM
//
// MessageText:
//
// TSP is instructed to verify the EK checksum and it does not verify.
//
#define TSS_E_EK_CHECKSUM             (UINT32)(TSS_E_BASE + 0x128L)


//
// MessageId: TSS_E_DELGATION_NOTSET
//
// MessageText:
//
// The Policy object does not have a delegation blob set.
//
#define TSS_E_DELEGATION_NOTSET      (UINT32)(TSS_E_BASE + 0x129L)

//
// MessageId: TSS_E_DELFAMILY_NOTFOUND
//
// MessageText:
//
// The specified delegation family was not found
//
#define TSS_E_DELFAMILY_NOTFOUND       (UINT32)(TSS_E_BASE + 0x130L)

//
// MessageId: TSS_E_DELFAMILY_ROWEXISTS
//
// MessageText:
//
// The specified delegation family table row is already in use and
// the command flags does not allow the TSS to overwrite the existing
// entry.
//
#define TSS_E_DELFAMILY_ROWEXISTS    (UINT32)(TSS_E_BASE + 0x131L)

//
// MessageId: TSS_E_VERSION_MISMATCH
//
// MessageText:
//
// The specified delegation family table row is already in use and
// the command flags does not allow the TSS to overwrite the existing
// entry.
//
#define TSS_E_VERSION_MISMATCH       (UINT32)(TSS_E_BASE + 0x132L)

//
//  MessageId: TSS_E_DAA_AR_DECRYPTION_ERROR
//
//  Decryption of the encrypted pseudonym has failed, due to
//  either a wrong secret key or a wrong decryption condition.
//
#define TSS_E_DAA_AR_DECRYPTION_ERROR             (UINT32)(TSS_E_BASE + 0x133L)

//
//  MessageId: TSS_E_DAA_AUTHENTICATION_ERROR
//
//  The TPM could not be authenticated by the DAA Issuer.
//
#define TSS_E_DAA_AUTHENTICATION_ERROR            (UINT32)(TSS_E_BASE + 0x134L)

//
//  MessageId: TSS_E_DAA_CHALLENGE_RESPONSE_ERROR
//
//  DAA Challenge response error.
//
#define TSS_E_DAA_CHALLENGE_RESPONSE_ERROR        (UINT32)(TSS_E_BASE + 0x135L)

//
//  MessageId: TSS_E_DAA_CREDENTIAL_PROOF_ERROR
//
//  Verification of the credential TSS_DAA_CRED_ISSUER issued by
//  the DAA Issuer has failed.
//
#define TSS_E_DAA_CREDENTIAL_PROOF_ERROR          (UINT32)(TSS_E_BASE + 0x136L)

//
//  MessageId: TSS_E_DAA_CREDENTIAL_REQUEST_PROOF_ERROR
//
//  Verification of the platform's credential request
//  TSS_DAA_CREDENTIAL_REQUEST has failed.
//
#define TSS_E_DAA_CREDENTIAL_REQUEST_PROOF_ERROR  (UINT32)(TSS_E_BASE + 0x137L)

//
//  MessageId: TSS_E_DAA_ISSUER_KEY_ERROR
//
//  DAA Issuer's authentication key chain could not be verified or
//  is not correct.
//
#define TSS_E_DAA_ISSUER_KEY_ERROR                (UINT32)(TSS_E_BASE + 0x138L)

//
//  MessageId: TSS_E_DAA_PSEUDONYM_ERROR
//
//  While verifying the pseudonym of the TPM, the private key of the
//  TPM was found on the rogue list.
//
#define TSS_E_DAA_PSEUDONYM_ERROR                 (UINT32)(TSS_E_BASE + 0x139L)

//
//  MessageId: TSS_E_INVALID_RESOURCE
//
//  Pointer to memory wrong.
//
#define TSS_E_INVALID_RESOURCE                    (UINT32)(TSS_E_BASE + 0x13AL)

//
//  MessageId: TSS_E_NV_AREA_EXIST
//
//  The NV area referenced already exists
//
#define TSS_E_NV_AREA_EXIST                       (UINT32)(TSS_E_BASE + 0x13BL)

//
//  MessageId: TSS_E_NV_AREA_NOT_EXIST
//
//  The NV area referenced doesn't exist
//
#define TSS_E_NV_AREA_NOT_EXIST                   (UINT32)(TSS_E_BASE + 0x13CL)

//
//  MessageId: TSS_E_TSP_TRANS_AUTHFAIL
//
//  The transport session authorization failed
//
#define TSS_E_TSP_TRANS_AUTHFAIL                  (UINT32)(TSS_E_BASE + 0x13DL)

//
//  MessageId: TSS_E_TSP_TRANS_AUTHREQUIRED
//
//  Authorization for transport is required
//
#define TSS_E_TSP_TRANS_AUTHREQUIRED              (UINT32)(TSS_E_BASE + 0x13EL)

//
//  MessageId: TSS_E_TSP_TRANS_NOT_EXCLUSIVE
//
//  A command was executed outside of an exclusive transport session.
//
#define TSS_E_TSP_TRANS_NOTEXCLUSIVE              (UINT32)(TSS_E_BASE + 0x13FL)

//
//  MessageId: TSS_E_TSP_TRANS_FAIL
//
//  Generic transport protection error.
//
#define TSS_E_TSP_TRANS_FAIL                     (UINT32)(TSS_E_BASE + 0x140L)

//
//  MessageId: TSS_E_TSP_TRANS_NO_PUBKEY
//
//  A command could not be executed through a logged transport session
//  because the command used a key and the key's public key is not
//  known to the TSP.
//
#define TSS_E_TSP_TRANS_NO_PUBKEY                (UINT32)(TSS_E_BASE + 0x141L)

//
//  MessageId: TSS_E_NO_ACTIVE_COUNTER
//
//  The TPM active counter has not been set yet.
//
#define TSS_E_NO_ACTIVE_COUNTER                  (UINT32)(TSS_E_BASE + 0x142L)


#endif // __TSS_ERROR_H__
