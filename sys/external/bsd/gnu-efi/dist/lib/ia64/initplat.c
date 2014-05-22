/*	$NetBSD: initplat.c,v 1.1.1.1.6.2 2014/05/22 11:40:58 yamt Exp $	*/

/*++

Copyright (c) 1999  Intel Corporation
    
Module Name:

    initplat.c

Abstract:

    Functions to make SAL and PAL proc calls

Revision History

--*/
#include "lib.h"

//#include "palproc.h"

VOID
InitializeLibPlatform (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )

{
    PLABEL  SalPlabel;
    UINT64  PalEntry;

    LibInitSalAndPalProc (&SalPlabel, &PalEntry);
}
