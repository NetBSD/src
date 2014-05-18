/*	$NetBSD: efilibplat.h,v 1.1.1.1.4.2 2014/05/18 17:46:03 rmind Exp $	*/

/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    efilibplat.h

Abstract:

    EFI to compile bindings




Revision History

--*/

VOID
InitializeLibPlatform (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

   
