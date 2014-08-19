/*	$NetBSD: efilibplat.h,v 1.1.1.1.10.2 2014/08/20 00:04:23 tls Exp $	*/

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

   
