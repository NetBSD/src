/*	$NetBSD: debug.c,v 1.1.1.1.6.2 2014/05/22 11:40:58 yamt Exp $	*/

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

