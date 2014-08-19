/*	$NetBSD: initplat.c,v 1.1.1.1.10.2 2014/08/20 00:04:24 tls Exp $	*/

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
