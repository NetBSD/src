/*++

There are platform dependent and general defines.

--*/

#ifndef TSS_PLATFORM_H
#define TSS_PLATFORM_H


/* The default implementation is to use stdint.h, a part of the C99 standard.
 * Systems that don't support this are handled on a case-by-case basis.
 */

#if !defined(WIN32)
#include <stdint.h>
   typedef uint8_t            BYTE;
   typedef int8_t             TSS_BOOL;
   typedef uint16_t           UINT16;
   typedef uint32_t           UINT32;
   typedef uint64_t           UINT64;

   typedef uint16_t           TSS_UNICODE;
   typedef void*              PVOID;

#elif defined(WIN32)
#include <basetsd.h>
   typedef  unsigned char    BYTE;  
   typedef  signed char      TSS_BOOL;
#ifndef _BASETSD_H_
   // basetsd.h provides definitions of UINT16, UINT32 and UINT64.
   typedef  unsigned short   UINT16;
   typedef  unsigned long    UINT32;
   typedef  unsigned __int64 UINT64;
#endif
   typedef  unsigned short   TSS_UNICODE;
   typedef  void*            PVOID;
#endif


/* Include this so that applications that use names as defined in the
 * 1.1 TSS specification can still compile
 */
#include <tss/compat11b.h>

#endif // TSS_PLATFORM_H
