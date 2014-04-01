/*	$NetBSD: initplat.c,v 1.1.1.1 2014/04/01 16:16:07 jakllsch Exp $	*/

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
