/*	$NetBSD: debug.c,v 1.1.1.1.34.1 2018/09/06 06:56:39 pgoyette Exp $	*/

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
    IN CONST CHAR8    *FileName,
    IN INTN           LineNo,
    IN CONST CHAR8    *Description
    )
{
    DbgPrint (D_ERROR, (CHAR8 *)"%EASSERT FAILED: %a(%d): %a%N\n", FileName, LineNo, Description);

    BREAKPOINT();
    return 0;
}

