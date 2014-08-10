/*	$NetBSD: efistdarg.h,v 1.1.1.1.2.1 2014/08/10 06:55:40 tls Exp $	*/

#ifndef _EFISTDARG_H_
#define _EFISTDARG_H_

/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    devpath.h

Abstract:

    Defines for parsing the EFI Device Path structures



Revision History

--*/
#ifdef __GNUC__
#ifdef __NetBSD__
#include <sys/stdarg.h>
#else
#include "stdarg.h"
#endif
#else
#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(UINTN) - 1) & ~(sizeof(UINTN) - 1) )

typedef CHAR8 * va_list;

#define va_start(ap,v)  ( ap = (va_list)&v + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)  ( ap = (va_list)0 )
#endif

#endif  /* _INC_STDARG */
