/*	$NetBSD: exit.c,v 1.1.1.1.6.2 2019/06/10 22:08:36 christos Exp $	*/

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
