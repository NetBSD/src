/*++

Basic defines for TSS error return codes
 
--*/

#ifndef __TSS_ERROR_BASICS_H__
#define __TSS_ERROR_BASICS_H__


//
// definitions for the various TSS-SW layers
//
#ifndef TSS_LAYER_TPM
#define TSS_LAYER_TPM   0x0000L     // definition for TPM layer
#endif // TSS_LAYER_TPM

#define TSS_LAYER_TDDL   0x1000L     // definition for TDDL layer
#define TSS_LAYER_TCS   0x2000L     // definition for TCS layer

#ifndef TSS_LAYER_TSP
#define TSS_LAYER_TSP   0x3000L     // definition for TSP layer
#endif // TSS_LAYER_TSP


//
// definitions for the start points of layer specific error codes
//
#ifndef TSS_COMMON_OFFSET
#define TSS_COMMON_OFFSET   0x000L     
#endif // TSS_COMMON_OFFSET

#define TSS_TDDL_OFFSET    0x080L     
#define TSS_TCSI_OFFSET    0x0C0L     

#ifndef TSS_TSPI_OFFSET
#define TSS_TSPI_OFFSET    0x100L     
#endif // TSS_TSPI_OFFSET

#ifndef TSS_VENDOR_OFFSET
#define TSS_VENDOR_OFFSET   0x800L       
#endif // TSS_VENDOR_OFFSET
 
// do not exceed TSS_MAX_ERROR for vendor specific code values:               
#ifndef TSS_MAX_ERROR              
#define TSS_MAX_ERROR    0xFFFL 
#endif // TSS_MAX_ERROR


/* Macros for the construction and interpretation of error codes */
#define TPM_ERROR(code)        (code)
#define TDDL_ERROR(code)       ((code) ? (TSS_LAYER_TDDL | (code)) : (code))
#define TCS_ERROR(code)        ((code) ? (TSS_LAYER_TCS | (code)) : (code))
#define TSP_ERROR(code)        ((code) ? (TSS_LAYER_TSP | (code)) : (code))
#define ERROR_LAYER(error)     ((error) & 0xf000)
#define ERROR_CODE(error)      ((error) & 0x0fff)

#endif // __TSS_ERROR_BASICS_H__

