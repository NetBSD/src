/*	$NetBSD: exit.c,v 1.1.1.1.2.2 2018/09/06 06:56:39 pgoyette Exp $	*/

#include "lib.h"

VOID
Exit(
    IN EFI_STATUS   ExitStatus,
    IN UINTN        ExitDataSize,
    IN CHAR16       *ExitData OPTIONAL
    )
{
    uefi_call_wrapper(BS->Exit,
            4,
            LibImageHandle,
            ExitStatus,
            ExitDataSize,
            ExitData);

    // Uh oh, Exit() returned?!
    for (;;) { }
}
