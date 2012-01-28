/*++

TSS Core Service structures

*/

#ifndef __TCS_DEFINES_H__
#define __TCS_DEFINES_H__

#define TSS_TCSATTRIB_TRANSPORT_DEFAULT           ((UINT32)(0x00000000))
#define TSS_TCSATTRIB_TRANSPORT_EXCLUSIVE         ((UINT32)(0x00000001))


// Values for the ulCredentialType parameter to Tcsi_GetCredential
#define TSS_TCS_CREDENTIAL_EKCERT                 ((UINT32)0x00000001)
#define TSS_TCS_CREDENTIAL_TPM_CC                 ((UINT32)0x00000002)
#define TSS_TCS_CREDENTIAL_PLATFORMCERT           ((UINT32)0x00000003)


// Values for the ulCredentialAccessMode parameter to Tcsi_GetCredential
//  TSS_TCS_CERT_ACCESS_AUTO triggers the default behavior.
//  Values with TSS_TCS_CERT_VENDOR_SPECIFIC_BIT set trigger
//    vendor specific behavior.
#define TSS_TCS_CERT_ACCESS_AUTO                  ((UINT32)0x00000001)

#define TSS_TCS_CERT_VENDOR_SPECIFIC_BIT          ((UINT32)0x80000000)

#endif // __TCS_DEFINES_H__
