/*++

TPM Device Driver Library error return codes 
 
--*/

#ifndef __TDDL_ERROR_H__
#define __TDDL_ERROR_H__

#include <tss/tss_error_basics.h>
#include <tss/tss_error.h>


#ifndef TSS_E_BASE
#define TSS_E_BASE      0x00000000L
#endif // TSS_E_BASE


//
// specific error codes returned by the TPM device driver library
// offset TSS_TDDL_OFFSET
//
#define TDDL_E_FAIL 		TSS_E_FAIL
#define TDDL_E_TIMEOUT 		TSS_E_TIMEOUT

// The connection was already established.
#define TDDL_E_ALREADY_OPENED   (UINT32)(TSS_E_BASE + 0x081L)

// The device was not connected.
#define TDDL_E_ALREADY_CLOSED   (UINT32)(TSS_E_BASE + 0x082L)

// The receive buffer is too small.
#define TDDL_E_INSUFFICIENT_BUFFER  (UINT32)(TSS_E_BASE + 0x083L)

// The command has already completed.
#define TDDL_E_COMMAND_COMPLETED  (UINT32)(TSS_E_BASE + 0x084L)

// TPM aborted processing of command.
#define TDDL_E_COMMAND_ABORTED  (UINT32)(TSS_E_BASE + 0x085L)

//  The request could not be performed because of an I/O device error.
#define TDDL_E_IOERROR    (UINT32)(TSS_E_BASE + 0x087L)

// Unsupported TAG is requested
#define TDDL_E_BADTAG    (UINT32)(TSS_E_BASE + 0x088L)

// the requested TPM component was not found
#define TDDL_E_COMPONENT_NOT_FOUND  (UINT32)(TSS_E_BASE + 0x089L)

#endif // __TDDL_ERROR_H__

