/*	$NetBSD: tpause.c,v 1.1.1.1.10.2 2014/08/20 00:04:23 tls Exp $	*/

#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	Print(L"Press `q' to quit, any other key to continue:\n");
	
}
