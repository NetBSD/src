/*	$NetBSD: panic.c,v 1.3.2.3 2017/08/28 17:51:41 skrll Exp $	*/

/*-
 * Copyright (c) 2016 Kimihiro Nonaka <nonaka@netbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "efiboot.h"

#include <efistdarg.h>

/* from sys/external/bsd/gnu-efi/dist/lib/print.c */
UINTN VPrint (IN CHAR16 *, va_list);

__dead VOID
Panic(
    IN CHAR16 *fmt,
    ...
    )
{
	va_list args;

	va_start(args, fmt);
	VPrint(fmt, args);
	Print(L"\n");
	va_end(args);
	reboot();
	/*NOTREACHED*/
	__unreachable();
}

__dead void
reboot(void)
{

	if (!efi_cleanuped)
		WaitForSingleEvent(ST->ConIn->WaitForKey, 0);

	uefi_call_wrapper(RT->ResetSystem, 4, EfiResetCold, EFI_SUCCESS,
	    0, NULL);
	for (;;)
		continue;
}

__dead void
_rtt(void)
{

	reboot();
}
