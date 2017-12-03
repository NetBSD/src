/*	$NetBSD: efiboot.h,v 1.5.8.2 2017/12/03 11:36:18 jdolecek Exp $	*/

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

#include <efi.h>
#include <efilib.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "libi386.h"

#include "efiboot_machdep.h"

extern EFI_GUID GraphicsOutputProtocol;

/* boot.c */
void boot(void);
void clearit(void);
void print_banner(void);

/* efiboot.c */
extern EFI_HANDLE IH;
extern EFI_DEVICE_PATH *efi_bootdp;
extern EFI_LOADED_IMAGE *efi_li;
extern uintptr_t efi_main_sp;
extern physaddr_t efi_loadaddr, efi_kernel_start;
extern u_long efi_kernel_size;
extern bool efi_cleanuped;
void efi_cleanup(void);

/* eficons.c */
int cninit(void);
void consinit(int, int, int);
void command_text(char *);
void command_gop(char *);

/* efidisk.c */
void efi_disk_probe(void);

/* efimemory.c */
void efi_memory_probe(void);
void efi_memory_show_map(bool);
EFI_MEMORY_DESCRIPTOR *efi_memory_get_map(UINTN *, UINTN *, UINTN *, UINT32 *,
    bool);

/* panic.c */
__dead VOID Panic(IN CHAR16 *, ...);
