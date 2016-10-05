/*	$NetBSD: efifpswa.c,v 1.4.30.1 2016/10/05 20:55:29 skrll Exp $	*/

/*-
 * Copyright (c) 2001 Peter Wemm <peter@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/boot/efi/libefi/efifpswa.c,v 1.2 2004/01/04 23:28:16 obrien Exp $"); */

#include <sys/param.h>
#include <sys/time.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include <efi.h>
#include <efilib.h>
#include <efifpswa.h>

#include <bootstrap.h>
#include "efiboot.h"

int
fpswa_init(u_int64_t *fpswa_interface)
{
	EFI_STATUS	status;
	static EFI_GUID	fpswaid = EFI_INTEL_FPSWA;
	UINTN		sz;
	EFI_HANDLE	fpswa_handle;
	FPSWA_INTERFACE *fpswa;

	*fpswa_interface = 0;
	sz = sizeof(EFI_HANDLE);
	status = BS->LocateHandle(ByProtocol, &fpswaid, 0, &sz, &fpswa_handle);
	if (EFI_ERROR(status))
		return ENOENT;

	status = BS->HandleProtocol(fpswa_handle, &fpswaid, (VOID **)&fpswa);
	if (EFI_ERROR(status))
		return ENOENT;
	*fpswa_interface = (u_int64_t)fpswa;
	return 0;
}
