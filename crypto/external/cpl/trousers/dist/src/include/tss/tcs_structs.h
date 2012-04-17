/*++

TSS Core Service structures

*/

#ifndef __TCS_STRUCT_H__
#define __TCS_STRUCT_H__

#include <tss/tpm.h>
#include <tss/tss_structs.h>
#include <tss/tcs_typedef.h>

typedef struct tdTCS_AUTH
{
    TCS_AUTHHANDLE  AuthHandle;
    TPM_NONCE       NonceOdd;   // system  
    TPM_NONCE       NonceEven;   // TPM   
    TSS_BOOL        fContinueAuthSession;
    TPM_AUTHDATA    HMAC;
} TCS_AUTH;

// This is kept for legacy compatibility
typedef TCS_AUTH    TPM_AUTH;


typedef struct tdTCS_LOADKEY_INFO
{
    TSS_UUID   keyUUID;
    TSS_UUID   parentKeyUUID;
    TPM_DIGEST  paramDigest; // SHA1 digest of the TPM_LoadKey
                             // Command input parameters
                             // As defined in TPM Main Specification
    TPM_AUTH   authData;     // Data regarding a valid auth
                             // Session including the
                             // HMAC digest
} TCS_LOADKEY_INFO;

#endif // __TCS_STRUCT_H__

