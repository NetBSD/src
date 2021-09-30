/*	$NetBSD: pause.c,v 1.1.1.1 2021/09/30 18:50:09 jmcneill Exp $	*/

#include "lib.h"

VOID
Pause(
    VOID
)
// Pause until any key is pressed
{
    EFI_INPUT_KEY Key;
    EFI_STATUS    Status EFI_UNUSED;

    WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
    Status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &Key);
    ASSERT(!EFI_ERROR(Status));
}
