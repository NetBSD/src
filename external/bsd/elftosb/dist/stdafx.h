#ifndef stdafx_h_
#define stdafx_h_

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

// Default to external release.
#ifndef SGTL_INTERNAL
    #define SGTL_INTERNAL 0
#endif

#include <iostream>
#include <stdexcept>

#if defined(WIN32)
//#include <tchar.h>
    
    // define this macro for use in VC++
    #if !defined(__LITTLE_ENDIAN__)
        #define __LITTLE_ENDIAN__ 1
    #endif // !defined(__LITTLE_ENDIAN__)
#endif // defined(WIN32)

#if defined(Linux)
// For Linux systems only, types.h only defines the signed
// integer types.  This is not professional code.
// Update: They are defined in the header files in the more recent version of redhat enterprise gcc.
#include <sys/types.h>
//typedef unsigned long uint32_t;
//typedef unsigned short uint16_t;
//typedef unsigned char uint8_t;

//#define TCHAR char
//#define _tmain main

    // give a default endian in case one is not defined on Linux (it should be, though)
    #if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
        #define __LITTLE_ENDIAN__ 1
    #endif // !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)

#endif // defined(Linux)


#if !defined(Linux)
// redefine missing typedefs from stdint.h or syst/types.h

typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef long int32_t;
typedef short int16_t;
typedef char int8_t;
#endif // !defined(Linux)

#if !defined(TRUE)
    #define TRUE 1
#endif // !defined(TRUE)

#if !defined(FALSE)
    #define FALSE 0
#endif // !defined(FALSE)

#endif // stdafx_h_
