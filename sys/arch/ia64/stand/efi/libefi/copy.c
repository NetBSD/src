/*	$NetBSD: copy.c,v 1.4.40.1 2016/10/05 20:55:29 skrll Exp $	*/

/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
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

/* __FBSDID("$FreeBSD: src/sys/boot/efi/libefi/copy.c,v 1.5 2003/04/03 21:36:29 obrien Exp $");
 */

#include <efi.h>
#include <efilib.h>
#include <lib/libsa/stand.h>

#include <machine/efilib.h>

int
efi_copyin(void *src, vaddr_t va, size_t len)
{

	memcpy((void *)efimd_va2pa(va), src, len);
	return (len);
}

int
efi_copyout(vaddr_t va, void *dst, size_t len)
{

	memcpy(dst, (void *)efimd_va2pa(va), len);
	return (len);
}

int
efi_readin(int fd, vaddr_t va, size_t len)
{

	return (read(fd, (void *)efimd_va2pa(va), len));
}
