/*	$NetBSD: efiboot.h,v 1.7 2018/09/15 17:06:32 jmcneill Exp $	*/

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

#include <loadfile.h>

#include "efiboot_machdep.h"

struct boot_command {
	const char *c_name;
	void (*c_fn)(char *);
	const char *c_help;
};

/* conf.c */
extern struct fs_ops null_fs_ops;
extern struct fs_ops tftp_fs_ops;

/* boot.c */
void boot(void);
void clearit(void);
void print_banner(void);
extern const struct boot_command commands[];
void command_help(char *);
int set_default_device(char *);
char *get_default_device(void);
int set_initrd_path(char *);
char *get_initrd_path(void);
int set_dtb_path(char *);
char *get_dtb_path(void);
extern int howto;

/* console.c */
int ischar(void);

/* efiboot.c */
extern EFI_HANDLE IH;
extern EFI_DEVICE_PATH *efi_bootdp;
extern EFI_LOADED_IMAGE *efi_li;
void efi_cleanup(void);
void efi_exit(void);
void efi_delay(int);
void efi_reboot(void);

/* efichar.c */
size_t ucs2len(const CHAR16 *);
int ucs2_to_utf8(const CHAR16 *, char **);
int utf8_to_ucs2(const char *, CHAR16 **, size_t *);

/* efidev.c */
int efi_device_path_depth(EFI_DEVICE_PATH *dp, int);
int efi_device_path_ncmp(EFI_DEVICE_PATH *, EFI_DEVICE_PATH *, int);

/* efinet.c */
int efi_net_open(struct open_file *, ...);
void efi_net_probe(void);
void efi_net_show(void);
int efi_net_get_booted_interface_unit(void);
extern struct netif_driver efinetif;

/* efipxe.c */
void efi_pxe_probe(void);
void efi_pxe_show(void);
bool efi_pxe_match_booted_interface(const EFI_MAC_ADDRESS *, UINT32);

/* exec.c */
int exec_netbsd(const char *, const char *);

/* panic.c */
__dead VOID Panic(IN CHAR16 *, ...);
__dead void reboot(void);

/* prompt.c */
char *gettrailer(char *);
void docommand(char *);
char awaitkey(int, int);
__dead void bootprompt(void);
