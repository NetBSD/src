/*++
 
TSS Core Service error return codes 
 
--*/

#ifndef __TCS_ERROR_H__
#define __TCS_ERROR_H__


#ifndef TSS_E_BASE
#define TSS_E_BASE      0x00000000L
#endif // TSS_E_BASE

// The context handle supplied is invalid.
#define TCS_E_INVALID_CONTEXTHANDLE  (UINT32)(TSS_E_BASE + 0x0C1L) 

// The key handle supplied is invalid.
#define TCS_E_INVALID_KEYHANDLE  (UINT32)(TSS_E_BASE + 0x0C2L) 

// The authorization session handle supplied is invalid.
#define TCS_E_INVALID_AUTHHANDLE  (UINT32)(TSS_E_BASE + 0x0C3L)

// the auth session has been closed by the TPM
#define TCS_E_INVALID_AUTHSESSION  (UINT32)(TSS_E_BASE + 0x0C4L) 

// the key has been unloaded 
#define TCS_E_INVALID_KEY   (UINT32)(TSS_E_BASE + 0x0C5L) 

// Key addressed by the application key handle does not match the key addressed
// by the given UUID.
#define TCS_E_KEY_MISMATCH   (UINT32)(TSS_E_BASE + 0x0C8L)

// Key adressed by Key's UUID cannot be loaded because one of the required
// parent keys needs authorization.
#define TCS_E_KM_LOADFAILED   (UINT32)(TSS_E_BASE + 0x0CAL)

// The Key Cache Manager could not reload the key into the TPM.
#define TCS_E_KEY_CONTEXT_RELOAD  (UINT32)(TSS_E_BASE + 0x0CCL)

// Bad memory index
#define TCS_E_BAD_INDEX                           (UINT32)(TSS_E_BASE + 0x0CDL)


// These TCS_E_ macros are defined by name in the TSS spec, however
// they are defined to have the same values as the TSS_E_ equivalents.
#define TCS_SUCCESS                        TSS_SUCCESS
#define TCS_E_KEY_ALREADY_REGISTERED       TSS_E_KEY_ALREADY_REGISTERED
#define TCS_E_KEY_NOT_REGISTERED           TSS_E_KEY_NOT_REGISTERED
#define TCS_E_BAD_PARAMETER                TSS_E_BAD_PARAMETER
#define TCS_E_OUTOFMEMORY                  TSS_E_OUTOFMEMORY
#define TCS_E_SIZE                         TSS_E_SIZE
#define TCS_E_NOTIMPL                      TSS_E_NOTIMPL
#define TCS_E_INTERNAL_ERROR               TSS_E_INTERNAL_ERROR

#endif // __TCS_ERROR_H__
