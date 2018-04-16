/*	$NetBSD: efiboot.h,v 1.5.10.3 2018/04/16 01:59:54 pgoyette Exp $	*/

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
extern enum efi_boot_device_type {
	BOOT_DEVICE_TYPE_HD,
	BOOT_DEVICE_TYPE_CD,
	BOOT_DEVICE_TYPE_NET
} efi_bootdp_type;
extern EFI_LOADED_IMAGE *efi_li;
extern uintptr_t efi_main_sp;
extern physaddr_t efi_loadaddr, efi_kernel_start;
extern u_long efi_kernel_size;
extern bool efi_cleanuped;
void efi_cleanup(void);

/* efichar.c */
size_t ucs2len(const CHAR16 *);
int ucs2_to_utf8(const CHAR16 *, char **);
int utf8_to_ucs2(const char *, CHAR16 **, size_t *);

/* eficons.c */
int cninit(void);
void consinit(int, int, int);
void efi_cons_show(void);
void command_text(char *);
void command_gop(char *);

/* efidev.c */
int efi_device_path_depth(EFI_DEVICE_PATH *dp, int);
int efi_device_path_ncmp(EFI_DEVICE_PATH *, EFI_DEVICE_PATH *, int);

/* efidisk.c */
void efi_disk_probe(void);
void efi_disk_show(void);

/* efimemory.c */
void efi_memory_probe(void);
void efi_memory_show_map(bool);
EFI_MEMORY_DESCRIPTOR *efi_memory_get_map(UINTN *, UINTN *, UINTN *, UINT32 *,
    bool);

/* efinet.c */
void efi_net_probe(void);
void efi_net_show(void);
int efi_net_get_booted_interface_unit(void);

/* efipxe.c */
void efi_pxe_probe(void);
void efi_pxe_show(void);
bool efi_pxe_match_booted_interface(const EFI_MAC_ADDRESS *, UINT32);

/* panic.c */
__dead VOID Panic(IN CHAR16 *, ...);
