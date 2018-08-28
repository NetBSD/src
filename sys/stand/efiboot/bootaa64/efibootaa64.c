/*	$NetBSD: efibootaa64.c,v 1.1 2018/08/24 02:01:06 jmcneill Exp $	*/

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

#include "../efiboot.h"
#include "../efifdt.h"

#include <sys/bootblock.h>

#include <loadfile.h>

/* cache.S */
void aarch64_dcache_wbinv_range(vaddr_t, vsize_t);
void aarch64_icache_inv_all(void);

void
efi_boot_kernel(u_long marks[MARK_MAX])
{
	void (*kernel_entry)(register_t, register_t, register_t, register_t);
	u_long kernel_size;

	kernel_entry = (void *)marks[MARK_ENTRY];
	kernel_size = marks[MARK_END] - marks[MARK_START];

	aarch64_dcache_wbinv_range((u_long)kernel_entry, kernel_size);
	if (efi_fdt_size() > 0)
		aarch64_dcache_wbinv_range((u_long)efi_fdt_data(), efi_fdt_size());
	aarch64_icache_inv_all();

	kernel_entry((register_t)efi_fdt_data(), 0, 0, 0);
}
