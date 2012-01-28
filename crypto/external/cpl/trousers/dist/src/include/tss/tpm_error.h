/*
 * The TPM error codes extracted from the TPM main specification
 * version 1.2 revision 85.
 */

#ifndef __TPM_ERROR_H__
#define __TPM_ERROR_H__


#ifndef TPM_E_BASE
#define TPM_E_BASE        ((UINT32)0)
#endif

#ifndef TPM_E_NON_FATAL
#define TPM_E_NON_FATAL   ((UINT32)0x00000800)
#endif


// Successful completion of the TPM operation.
#define TPM_SUCCESS    TPM_E_BASE

//
// MessageId: TPM_E_AUTHFAIL
//
// MessageText:
//
// Authentication failed
//
#define TPM_E_AUTHFAIL ((UINT32)(TPM_E_BASE + 0x00000001))

//
// MessageId: TPM_E_BADINDEX
//
// MessageText:
//
// The index to a PCR, DIR or other register is incorrect
//
#define TPM_E_BADINDEX ((UINT32)(TPM_E_BASE + 0x00000002))

//
// MessageId: TPM_E_BAD_PARAMETER
//
// MessageText:
//
// One or more parameter is bad
//
#define TPM_E_BAD_PARAMETER ((UINT32)(TPM_E_BASE + 0x00000003))

//
// MessageId: TPM_E_AUDITFAILURE
//
// MessageText:
//
// An operation completed successfully but the auditing of that
// operation failed.
//
#define TPM_E_AUDITFAILURE ((UINT32)(TPM_E_BASE + 0x00000004))

//
// MessageId: TPM_E_CLEAR_DISABLED
//
// MessageText:
//
// The clear disable flag is set and all clear operations now require
// physical access
//
#define TPM_E_CLEAR_DISABLED ((UINT32)(TPM_E_BASE + 0x00000005))

//
// MessageId: TPM_E_DEACTIVATED
//
// MessageText:
//
// The TPM is deactivated
//
#define TPM_E_DEACTIVATED ((UINT32)(TPM_E_BASE + 0x00000006))

//
// MessageId: TPM_E_DISABLED
//
// MessageText:
//
// The TPM is disabled
//
#define TPM_E_DISABLED ((UINT32)(TPM_E_BASE + 0x00000007))

//
// MessageId: TPM_E_DISABLED_CMD
//
// MessageText:
//
// The target command has been disabled
//
#define TPM_E_DISABLED_CMD ((UINT32)(TPM_E_BASE + 0x00000008))

//
// MessageId: TPM_E_FAIL
//
// MessageText:
//
// The operation failed
//
#define TPM_E_FAIL ((UINT32)(TPM_E_BASE + 0x00000009))

//
// MessageId: TPM_E_BAD_ORDINAL
//
// MessageText:
//
// The ordinal was unknown or inconsistent
//
#define TPM_E_BAD_ORDINAL ((UINT32)(TPM_E_BASE + 0x0000000a))

//
// MessageId: TPM_E_INSTALL_DISABLED
//
// MessageText:
//
// The ability to install an owner is disabled
//
#define TPM_E_INSTALL_DISABLED ((UINT32)(TPM_E_BASE + 0x0000000b))

//
// MessageId: TPM_E_INVALID_KEYHANDLE
//
// MessageText:
//
// The key handle can not be interpreted
//
#define TPM_E_INVALID_KEYHANDLE ((UINT32)(TPM_E_BASE + 0x0000000c))

//
// MessageId: TPM_E_KEYNOTFOUND
//
// MessageText:
//
// The key handle points to an invalid key
//
#define TPM_E_KEYNOTFOUND ((UINT32)(TPM_E_BASE + 0x0000000d))

//
// MessageId: TPM_E_INAPPROPRIATE_ENC
//
// MessageText:
//
// Unacceptable encryption scheme
//
#define TPM_E_INAPPROPRIATE_ENC ((UINT32)(TPM_E_BASE + 0x0000000e))

//
// MessageId: TPM_E_MIGRATEFAIL
//
// MessageText:
//
// Migration authorization failed
//
#define TPM_E_MIGRATEFAIL ((UINT32)(TPM_E_BASE + 0x0000000f))

//
// MessageId: TPM_E_INVALID_PCR_INFO
//
// MessageText:
//
// PCR information could not be interpreted
//
#define TPM_E_INVALID_PCR_INFO ((UINT32)(TPM_E_BASE + 0x00000010))

//
// MessageId: TPM_E_NOSPACE
//
// MessageText:
//
// No room to load key.
//
#define TPM_E_NOSPACE ((UINT32)(TPM_E_BASE + 0x00000011))

//
// MessageId: TPM_E_NOSRK
//
// MessageText:
//
// There is no SRK set
//
#define TPM_E_NOSRK ((UINT32)(TPM_E_BASE + 0x00000012))

//
// MessageId: TPM_E_NOTSEALED_BLOB
//
// MessageText:
//
// An encrypted blob is invalid or was not created by this TPM
//
#define TPM_E_NOTSEALED_BLOB ((UINT32)(TPM_E_BASE + 0x00000013))

//
// MessageId: TPM_E_OWNER_SET
//
// MessageText:
//
// There is already an Owner 
//
#define TPM_E_OWNER_SET ((UINT32)(TPM_E_BASE + 0x00000014))

//
// MessageId: TPM_E_RESOURCES
//
// MessageText:
//
// The TPM has insufficient internal resources to perform the
// requested action.
//
#define TPM_E_RESOURCES ((UINT32)(TPM_E_BASE + 0x00000015))

//
// MessageId: TPM_E_SHORTRANDOM
//
// MessageText:
//
// A random string was too short
//
#define TPM_E_SHORTRANDOM ((UINT32)(TPM_E_BASE + 0x00000016))

//
// MessageId: TPM_E_SIZE
//
// MessageText:
//
// The TPM does not have the space to perform the operation.
//
#define TPM_E_SIZE ((UINT32)(TPM_E_BASE + 0x00000017))

//
// MessageId: TPM_E_WRONGPCRVAL
//
// MessageText:
//
// The named PCR value does not match the current PCR value.
//
#define TPM_E_WRONGPCRVAL ((UINT32)(TPM_E_BASE + 0x00000018))

//
// MessageId: TPM_E_BAD_PARAM_SIZE 
//
// MessageText:
//
// The paramSize argument to the command has the incorrect value 
//
#define TPM_E_BAD_PARAM_SIZE ((UINT32)(TPM_E_BASE + 0x00000019))

//
// MessageId: TPM_E_SHA_THREAD
//
// MessageText:
//
// There is no existing SHA-1 thread.
//
#define TPM_E_SHA_THREAD ((UINT32)(TPM_E_BASE + 0x0000001a))

//
// MessageId: TPM_E_SHA_ERROR
//
// MessageText:
//
// The calculation is unable to proceed because the existing SHA-1
// thread has already encountered an error.
//
#define TPM_E_SHA_ERROR ((UINT32)(TPM_E_BASE + 0x0000001b))

//
// MessageId: TPM_E_FAILEDSELFTEST
//
// MessageText:
//
// Self-test has failed and the TPM has shutdown.
//
#define TPM_E_FAILEDSELFTEST ((UINT32)(TPM_E_BASE + 0x0000001c))

//
// MessageId: TPM_E_AUTH2FAIL
//
// MessageText:
//
// The authorization for the second key in a 2 key function failed
// authorization
//
#define TPM_E_AUTH2FAIL ((UINT32)(TPM_E_BASE + 0x0000001d))

//
// MessageId: TPM_E_BADTAG
//
// MessageText:
//
// The tag value sent to for a command is invalid
//
#define TPM_E_BADTAG ((UINT32)(TPM_E_BASE + 0x0000001e))

//
// MessageId: TPM_E_IOERROR
//
// MessageText:
//
// An IO error occurred transmitting information to the TPM
//
#define TPM_E_IOERROR ((UINT32)(TPM_E_BASE + 0x0000001f))

//
// MessageId: TPM_E_ENCRYPT_ERROR
//
// MessageText:
//
// The encryption process had a problem.
//
#define TPM_E_ENCRYPT_ERROR ((UINT32)(TPM_E_BASE + 0x00000020))

//
// MessageId: TPM_E_DECRYPT_ERROR
//
// MessageText:
//
// The decryption process did not complete.
//
#define TPM_E_DECRYPT_ERROR ((UINT32)(TPM_E_BASE + 0x00000021))

//
// MessageId: TPM_E_INVALID_AUTHHANDLE
//
// MessageText:
//
// An invalid handle was used.
//
#define TPM_E_INVALID_AUTHHANDLE ((UINT32)(TPM_E_BASE + 0x00000022))

//
// MessageId: TPM_E_NO_ENDORSEMENT
//
// MessageText:
//
// The TPM does not a EK installed
//
#define TPM_E_NO_ENDORSEMENT ((UINT32)(TPM_E_BASE + 0x00000023))

//
// MessageId: TPM_E_INVALID_KEYUSAGE
//
// MessageText:
//
// The usage of a key is not allowed
//
#define TPM_E_INVALID_KEYUSAGE ((UINT32)(TPM_E_BASE + 0x00000024))

//
// MessageId: TPM_E_WRONG_ENTITYTYPE
//
// MessageText:
//
// The submitted entity type is not allowed
//
#define TPM_E_WRONG_ENTITYTYPE ((UINT32)(TPM_E_BASE + 0x00000025))

//
// MessageId: TPM_E_INVALID_POSTINIT
//
// MessageText:
//
// The command was received in the wrong sequence relative to TPM_Init
// and a subsequent TPM_Startup
//
#define TPM_E_INVALID_POSTINIT ((UINT32)(TPM_E_BASE + 0x00000026))

//
// MessageId: TPM_E_INAPPROPRIATE_SIG
//
// MessageText:
//
// Signed data cannot include additional DER information
//
#define TPM_E_INAPPROPRIATE_SIG ((UINT32)(TPM_E_BASE + 0x00000027))

//
// MessageId: TPM_E_BAD_KEY_PROPERTY
//
// MessageText:
//
// The key properties in TPM_KEY_PARMs are not supported by this TPM
//
#define TPM_E_BAD_KEY_PROPERTY ((UINT32)(TPM_E_BASE + 0x00000028))

//
// MessageId: TPM_E_BAD_MIGRATION
//
// MessageText:
//
// The migration properties of this key are incorrect.
//
#define TPM_E_BAD_MIGRATION ((UINT32)(TPM_E_BASE + 0x00000029))

//
// MessageId: TPM_E_BAD_SCHEME
//
// MessageText:
//
// The signature or encryption scheme for this key is incorrect or not
// permitted in this situation.
//
#define TPM_E_BAD_SCHEME ((UINT32)(TPM_E_BASE + 0x0000002a))

//
// MessageId: TPM_E_BAD_DATASIZE
//
// MessageText:
//
// The size of the data (or blob) parameter is bad or inconsistent
// with the referenced key
//
#define TPM_E_BAD_DATASIZE ((UINT32)(TPM_E_BASE + 0x0000002b))

//
// MessageId: TPM_E_BAD_MODE
//
// MessageText:
//
// A mode parameter is bad, such as capArea or subCapArea for
// TPM_GetCapability, physicalPresence parameter for
// TPM_PhysicalPresence, or migrationType for TPM_CreateMigrationBlob.
//
#define TPM_E_BAD_MODE ((UINT32)(TPM_E_BASE + 0x0000002c))

//
// MessageId: TPM_E_BAD_PRESENCE
//
// MessageText:
//
// Either the physicalPresence or physicalPresenceLock bits have the
// wrong value
//
#define TPM_E_BAD_PRESENCE ((UINT32)(TPM_E_BASE + 0x0000002d))

//
// MessageId: TPM_E_BAD_VERSION
//
// MessageText:
//
// The TPM cannot perform this version of the capability
//
#define TPM_E_BAD_VERSION ((UINT32)(TPM_E_BASE + 0x0000002e))

//
// MessageId: TPM_E_NO_WRAP_TRANSPORT
//
// MessageText:
//
// The TPM does not allow for wrapped transport sessions
//
#define TPM_E_NO_WRAP_TRANSPORT ((UINT32)(TPM_E_BASE + 0x0000002f))

//
// MessageId: TPM_E_AUDITFAIL_UNSUCCESSFUL
//
// MessageText:
//
// TPM audit construction failed and the underlying command was
// returning a failure code also
//
#define TPM_E_AUDITFAIL_UNSUCCESSFUL ((UINT32)(TPM_E_BASE + 0x00000030))

//
// MessageId: TPM_E_AUDITFAIL_SUCCESSFUL
//
// MessageText:
//
// TPM audit construction failed and the underlying command was
// returning success
//
#define TPM_E_AUDITFAIL_SUCCESSFUL ((UINT32)(TPM_E_BASE + 0x00000031))

//
// MessageId: TPM_E_NOTRESETABLE
//
// MessageText:
//
// Attempt to reset a PCR register that does not have the resettable
// attribute
//
#define TPM_E_NOTRESETABLE ((UINT32)(TPM_E_BASE + 0x00000032))

//
// MessageId: TPM_E_NOTLOCAL
//
// MessageText:
//
// Attempt to reset a PCR register that requires locality and locality
// modifier not part of command transport
//
#define TPM_E_NOTLOCAL ((UINT32)(TPM_E_BASE + 0x00000033))

//
// MessageId: TPM_E_BAD_TYPE
//
// MessageText:
//
// Make identity blob not properly typed
//
#define TPM_E_BAD_TYPE ((UINT32)(TPM_E_BASE + 0x00000034))

//
// MessageId: TPM_E_INVALID_RESOURCE
//
// MessageText:
//
// When saving context identified resource type does not match actual
// resource
//
#define TPM_E_INVALID_RESOURCE ((UINT32)(TPM_E_BASE + 0x00000035))

//
// MessageId: TPM_E_NOTFIPS
//
// MessageText:
//
// The TPM is attempting to execute a command only available when in
// FIPS mode
//
#define TPM_E_NOTFIPS ((UINT32)(TPM_E_BASE + 0x00000036))

//
// MessageId: TPM_E_INVALID_FAMILY
//
// MessageText:
//
// The command is attempting to use an invalid family ID
//
#define TPM_E_INVALID_FAMILY ((UINT32)(TPM_E_BASE + 0x00000037))

//
// MessageId: TPM_E_NO_NV_PERMISSION
//
// MessageText:
//
// The permission to manipulate the NV storage is not available
//
#define TPM_E_NO_NV_PERMISSION ((UINT32)(TPM_E_BASE + 0x00000038))

//
// MessageId: TPM_E_REQUIRES_SIGN
//
// MessageText:
//
// The operation requires a signed command
//
#define TPM_E_REQUIRES_SIGN ((UINT32)(TPM_E_BASE + 0x00000039))

//
// MessageId: TPM_E_KEY_NOTSUPPORTED
//
// MessageText:
//
// Wrong operation to load an NV key
//
#define TPM_E_KEY_NOTSUPPORTED ((UINT32)(TPM_E_BASE + 0x0000003a))

//
// MessageId: TPM_E_AUTH_CONFLICT
//
// MessageText:
//
// NV_LoadKey blob requires both owner and blob authorization
//
#define TPM_E_AUTH_CONFLICT ((UINT32)(TPM_E_BASE + 0x0000003b))

//
// MessageId: TPM_E_AREA_LOCKED
//
// MessageText:
//
// The NV area is locked and not writable
//
#define TPM_E_AREA_LOCKED ((UINT32)(TPM_E_BASE + 0x0000003c))

//
// MessageId: TPM_E_BAD_LOCALITY
//
// MessageText:
//
// The locality is incorrect for the attempted operation
//
#define TPM_E_BAD_LOCALITY ((UINT32)(TPM_E_BASE + 0x0000003d))

//
// MessageId: TPM_E_READ_ONLY
//
// MessageText:
//
// The NV area is read only and can't be written to
//
#define TPM_E_READ_ONLY ((UINT32)(TPM_E_BASE + 0x0000003e))

//
// MessageId: TPM_E_PER_NOWRITE
//
// MessageText:
//
// There is no protection on the write to the NV area
//
#define TPM_E_PER_NOWRITE ((UINT32)(TPM_E_BASE + 0x0000003f))

//
// MessageId: TPM_E_FAMILYCOUNT
//
// MessageText:
//
// The family count value does not match
//
#define TPM_E_FAMILYCOUNT ((UINT32)(TPM_E_BASE + 0x00000040))

//
// MessageId: TPM_E_WRITE_LOCKED
//
// MessageText:
//
// The NV area has already been written to
//
#define TPM_E_WRITE_LOCKED ((UINT32)(TPM_E_BASE + 0x00000041))

//
// MessageId: TPM_E_BAD_ATTRIBUTES
//
// MessageText:
//
// The NV area attributes conflict
//
#define TPM_E_BAD_ATTRIBUTES ((UINT32)(TPM_E_BASE + 0x00000042))

//
// MessageId: TPM_E_INVALID_STRUCTURE
//
// MessageText:
//
// The structure tag and version are invalid or inconsistent
//
#define TPM_E_INVALID_STRUCTURE ((UINT32)(TPM_E_BASE + 0x00000043))

//
// MessageId: TPM_E_KEY_OWNER_CONTROL
//
// MessageText:
//
// The key is under control of the TPM Owner and can only be evicted
// by the TPM Owner.
//
#define TPM_E_KEY_OWNER_CONTROL ((UINT32)(TPM_E_BASE + 0x00000044))

//
// MessageId: TPM_E_BAD_COUNTER
//
// MessageText:
//
// The counter handle is incorrect
//
#define TPM_E_BAD_COUNTER ((UINT32)(TPM_E_BASE + 0x00000045))

//
// MessageId: TPM_E_NOT_FULLWRITE
//
// MessageText:
//
// The write is not a complete write of the area
//
#define TPM_E_NOT_FULLWRITE ((UINT32)(TPM_E_BASE + 0x00000046))

//
// MessageId: TPM_E_CONTEXT_GAP
//
// MessageText:
//
// The gap between saved context counts is too large
//
#define TPM_E_CONTEXT_GAP ((UINT32)(TPM_E_BASE + 0x00000047))

//
// MessageId: TPM_E_MAXNVWRITES
//
// MessageText:
//
// The maximum number of NV writes without an owner has been exceeded
//
#define TPM_E_MAXNVWRITES ((UINT32)(TPM_E_BASE + 0x00000048))

//
// MessageId: TPM_E_NOOPERATOR
//
// MessageText:
//
// No operator AuthData value is set
//
#define TPM_E_NOOPERATOR ((UINT32)(TPM_E_BASE + 0x00000049))

//
// MessageId: TPM_E_RESOURCEMISSING
//
// MessageText:
//
// The resource pointed to by context is not loaded
//
#define TPM_E_RESOURCEMISSING ((UINT32)(TPM_E_BASE + 0x0000004a))

//
// MessageId: TPM_E_DELEGATE_LOCK
//
// MessageText:
//
// The delegate administration is locked
//
#define TPM_E_DELEGATE_LOCK ((UINT32)(TPM_E_BASE + 0x0000004b))

//
// MessageId: TPM_E_DELEGATE_FAMILY
//
// MessageText:
//
// Attempt to manage a family other then the delegated family
//
#define TPM_E_DELEGATE_FAMILY ((UINT32)(TPM_E_BASE + 0x0000004c))

//
// MessageId: TPM_E_DELEGATE_ADMIN
//
// MessageText:
//
// Delegation table management not enabled
//
#define TPM_E_DELEGATE_ADMIN ((UINT32)(TPM_E_BASE + 0x0000004d))

//
// MessageId: TPM_E_TRANSPORT_NOTEXCLUSIVE
//
// MessageText:
//
// There was a command executed outside of an exclusive transport session
//
#define TPM_E_TRANSPORT_NOTEXCLUSIVE ((UINT32)(TPM_E_BASE + 0x0000004e))

//
// MessageId: TPM_E_OWNER_CONTROL
//
// MessageText:
//
// Attempt to context save a owner evict controlled key
//
#define TPM_E_OWNER_CONTROL ((UINT32)(TPM_E_BASE + 0x0000004f))

//
// MessageId: TPM_E_DAA_RESOURCES
//
// MessageText:
//
// The DAA command has no resources available to execute the command
//
#define TPM_E_DAA_RESOURCES ((UINT32)(TPM_E_BASE + 0x00000050))

//
// MessageId: TPM_E_DAA_INPUT_DATA0
//
// MessageText:
//
// The consistency check on DAA parameter inputData0 has failed.
//
#define TPM_E_DAA_INPUT_DATA0 ((UINT32)(TPM_E_BASE + 0x00000051))

//
// MessageId: TPM_E_DAA_INPUT_DATA1
//
// MessageText:
//
// The consistency check on DAA parameter inputData1 has failed.
//
#define TPM_E_DAA_INPUT_DATA1 ((UINT32)(TPM_E_BASE + 0x00000052))

//
// MessageId: TPM_E_DAA_ISSUER_SETTINGS
//
// MessageText:
//
// The consistency check on DAA_issuerSettings has failed.
//
#define TPM_E_DAA_ISSUER_SETTINGS ((UINT32)(TPM_E_BASE + 0x00000053))

//
// MessageId: TPM_E_DAA_TPM_SETTINGS
//
// MessageText:
//
// The consistency check on DAA_tpmSpecific has failed.
//
#define TPM_E_DAA_TPM_SETTINGS ((UINT32)(TPM_E_BASE + 0x00000054))

//
// MessageId: TPM_E_DAA_STAGE
//
// MessageText:
//
// The atomic process indicated by the submitted DAA command is not
// the expected process.
//
#define TPM_E_DAA_STAGE ((UINT32)(TPM_E_BASE + 0x00000055))

//
// MessageId: TPM_E_DAA_ISSUER_VALIDITY
//
// MessageText:
//
// The issuer's validity check has detected an inconsistency
//
#define TPM_E_DAA_ISSUER_VALIDITY ((UINT32)(TPM_E_BASE + 0x00000056))

//
// MessageId: TPM_E_DAA_WRONG_W
//
// MessageText:
//
// The consistency check on w has failed.
//
#define TPM_E_DAA_WRONG_W ((UINT32)(TPM_E_BASE + 0x00000057))

//
// MessageId: TPM_E_BAD_HANDLE
//
// MessageText:
//
// The handle is incorrect
//
#define TPM_E_BAD_HANDLE ((UINT32)(TPM_E_BASE + 0x00000058))

//
// MessageId: TPM_E_BAD_DELEGATE
//
// MessageText:
//
// Delegation is not correct
//
#define TPM_E_BAD_DELEGATE ((UINT32)(TPM_E_BASE + 0x00000059))

//
// MessageId: TPM_E_BADCONTEXT
//
// MessageText:
//
// The context blob is invalid
//
#define TPM_E_BADCONTEXT ((UINT32)(TPM_E_BASE + 0x0000005a))

//
// MessageId: TPM_E_TOOMANYCONTEXTS
//
// MessageText:
//
// Too many contexts held by the TPM
//
#define TPM_E_TOOMANYCONTEXTS ((UINT32)(TPM_E_BASE + 0x0000005b))

//
// MessageId: TPM_E_MA_TICKET_SIGNATURE
//
// MessageText:
//
// Migration authority signature validation failure
//
#define TPM_E_MA_TICKET_SIGNATURE ((UINT32)(TPM_E_BASE + 0x0000005c))

//
// MessageId: TPM_E_MA_DESTINATION
//
// MessageText:
//
// Migration destination not authenticated
//
#define TPM_E_MA_DESTINATION ((UINT32)(TPM_E_BASE + 0x0000005d))

//
// MessageId: TPM_E_MA_SOURCE
//
// MessageText:
//
// Migration source incorrect
//
#define TPM_E_MA_SOURCE ((UINT32)(TPM_E_BASE + 0x0000005e))

//
// MessageId: TPM_E_MA_AUTHORITY
//
// MessageText:
//
// Incorrect migration authority
//
#define TPM_E_MA_AUTHORITY ((UINT32)(TPM_E_BASE + 0x0000005f))

//
// MessageId: TPM_E_PERMANENTEK
//
// MessageText:
//
// Attempt to revoke the EK and the EK is not revocable
//
#define TPM_E_PERMANENTEK ((UINT32)(TPM_E_BASE + 0x00000061))

//
// MessageId: TPM_E_BAD_SIGNATURE
//
// MessageText:
//
// Bad signature of CMK ticket
//
#define TPM_E_BAD_SIGNATURE ((UINT32)(TPM_E_BASE + 0x00000062))

//
// MessageId: TPM_E_NOCONTEXTSPACE
//
// MessageText:
//
// There is no room in the context list for additional contexts
//
#define TPM_E_NOCONTEXTSPACE  ((UINT32)(TPM_E_BASE + 0x00000063))


//
// MessageId: TPM_E_RETRY
//
// MessageText:
//
// The TPM is too busy to respond to the command immediately, but the
// command could be resubmitted at a later time. The TPM MAY return
// TPM_Retry for any command at any time.
//
#define TPM_E_RETRY ((UINT32)(TPM_E_BASE + TPM_E_NON_FATAL))

//
// MessageId: TPM_E_NEEDS_SELFTEST
//
// MessageText:
//
// SelfTestFull has not been run
//
#define TPM_E_NEEDS_SELFTEST ((UINT32)(TPM_E_BASE + TPM_E_NON_FATAL + 1))

//
// MessageId: TPM_E_DOING_SELFTEST
//
// MessageText:
//
// The TPM is currently executing a full selftest
//
#define TPM_E_DOING_SELFTEST ((UINT32)(TPM_E_BASE + TPM_E_NON_FATAL + 2))

//
// MessageId: TPM_E_DEFEND_LOCK_RUNNING
//
// MessageText:
//
// The TPM is defending against dictionary attacks and is in some
// time-out period.
//
#define TPM_E_DEFEND_LOCK_RUNNING ((UINT32)(TPM_E_BASE + TPM_E_NON_FATAL + 3))

#endif /* __TPM_ERROR_H__ */
