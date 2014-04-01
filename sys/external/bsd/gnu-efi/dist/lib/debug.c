/*	$NetBSD: debug.c,v 1.1.1.1 2014/04/01 16:16:06 jakllsch Exp $	*/

/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    debug.c

Abstract:

    Debug library functions



Revision History

--*/

#include "lib.h"



//
// Declare runtime functions
//

//
//
//

INTN
DbgAssert (
    IN CHAR8    *FileName,
    IN INTN     LineNo,
    IN CHAR8    *Description
    )
{
    DbgPrint (D_ERROR, (CHAR8 *)"%EASSERT FAILED: %a(%d): %a%N\n", FileName, LineNo, Description);

    BREAKPOINT();
    return 0;
}

