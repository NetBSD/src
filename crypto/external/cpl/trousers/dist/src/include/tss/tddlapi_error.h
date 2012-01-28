/*++

TDDL error return codes for the TPM Device Driver Library Interface (TDDLI)
 
--*/

#ifndef __TDDLAPI_ERROR_H__
#define __TDDLAPI_ERROR_H__


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


// no macros are used below intentionally 
// for a better error code recognition by the reader

// note that the values of TPM_E_BASE and TSS_E_BASE, TSS_W_BASE and TSS_I_BASE
// have to be adjusted for a platform other than Windows

//
// TPM specific error codes (layer nibble set to TPM layer TSS_LAYER_TPM)
//


#endif // __TDDLAPI_ERROR_H__

