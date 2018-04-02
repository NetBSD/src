/*	$NetBSD: efidelay.c,v 1.1.12.1 2018/04/02 08:50:33 martin Exp $	*/

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

void
delay(int us)
{
	EFI_STATUS status;
	CHAR16 errmsg[128];
	char *uerrmsg;
	int rv;

	status = uefi_call_wrapper(BS->Stall, 1, us);
	if (EFI_ERROR(status)) {
		StatusToString(errmsg, status);
		uerrmsg = NULL;
		rv = ucs2_to_utf8(errmsg, &uerrmsg);
		if (rv)
			uerrmsg = "";
		panic("couldn't delay %d us: %s(%" PRIxMAX ")\n", us, uerrmsg,
		    (uintmax_t)status);
		if (rv == 0)
			FreePool(uerrmsg);
	}
}

void
wait_sec(int sec)
{
	while (sec-- > 0)
		delay(1000 * 1000);
}
