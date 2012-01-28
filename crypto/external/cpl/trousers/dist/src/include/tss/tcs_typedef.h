/*++

Global typedefs for TSS Core Service
 
*/

#ifndef __TCS_TYPEDEF_H__
#define __TCS_TYPEDEF_H__

#include <tss/tss_structs.h>
#include <tss/tpm.h>

typedef UINT32  TCS_AUTHHANDLE;
typedef UINT32  TCS_CONTEXT_HANDLE;
typedef UINT32  TCS_KEY_HANDLE;
typedef UINT32  TCS_HANDLE;


// Substitution definitions for TCS-IDL
typedef TPM_ENCAUTH                TCG_ENCAUTH;
typedef TPM_NONCE                  TCG_NONCE;
typedef TPM_ENTITY_TYPE            TCG_ENTITY_TYPE;
typedef TPM_PCRINDEX               TCG_PCRINDEX;
typedef TPM_DIGEST                 TCG_DIGEST;
typedef TPM_PCRVALUE               TCG_PCRVALUE;
typedef TPM_DIRVALUE               TCG_DIRVALUE;
typedef TPM_DIRINDEX               TCG_DIRINDEX;



#endif // __TCS_TYPEDEF_H__

