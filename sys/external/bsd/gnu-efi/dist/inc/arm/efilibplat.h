/*	$NetBSD: efilibplat.h,v 1.1.1.1.6.2 2019/06/10 22:08:35 christos Exp $	*/

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

