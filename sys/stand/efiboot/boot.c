/*	$NetBSD: boot.c,v 1.43 2022/03/25 21:23:00 jmcneill Exp $	*/

/*-
 * Copyright (c) 2016 Kimihiro Nonaka <nonaka@netbsd.org>
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
#include "efiblock.h"
#include "efifile.h"
#include "efirng.h"
#include "module.h"
#include "bootmenu.h"

#ifdef EFIBOOT_FDT
#include "efifdt.h"
#include "overlay.h"
#endif

#ifdef EFIBOOT_ACPI
#include "efiacpi.h"
#endif

#include <sys/bootblock.h>
#include <sys/boot_flag.h>
#include <machine/limits.h>

#include <loadfile.h>
#include <bootcfg.h>

extern const char bootprog_name[], bootprog_rev[], bootprog_kernrev[];

extern char twiddle_toggle;

static const char * const names[] = {
	"netbsd", "netbsd.gz",
	"onetbsd", "onetbsd.gz",
	"netbsd.old", "netbsd.old.gz",
};

#define NUMNAMES	__arraycount(names)

static const char *efi_memory_type[] = {
        [EfiReservedMemoryType]         = "Reserved Memory Type",
        [EfiLoaderCode]                 = "Loader Code",
        [EfiLoaderData]                 = "Loader Data",
        [EfiBootServicesCode]           = "Boot Services Code",
        [EfiBootServicesData]           = "Boot Services Data",
        [EfiRuntimeServicesCode]        = "Runtime Services Code",
        [EfiRuntimeServicesData]        = "Runtime Services Data",
        [EfiConventionalMemory]         = "Conventional Memory",
        [EfiUnusableMemory]             = "Unusable Memory",
        [EfiACPIReclaimMemory]          = "ACPI Reclaim Memory",
        [EfiACPIMemoryNVS]              = "ACPI Memory NVS",
        [EfiMemoryMappedIO]             = "MMIO",
        [EfiMemoryMappedIOPortSpace]    = "MMIO (Port Space)",
        [EfiPalCode]                    = "Pal Code",
        [EfiPersistentMemory]           = "Persistent Memory",
};

static char default_device[32];
static int default_fstype = FS_UNUSED;
static char initrd_path[255];
static char dtb_path[255];
static char netbsd_path[255];
static char netbsd_args[255];
static char rndseed_path[255];

#define DEFFILENAME	names[0]

int	set_bootfile(const char *);
int	set_bootargs(const char *);

#ifdef EFIBOOT_ACPI
void	command_acpi(char *);
#endif
void	command_boot(char *);
void	command_dev(char *);
void	command_initrd(char *);
void	command_rndseed(char *);
#ifdef EFIBOOT_FDT
void	command_dtb(char *);
void	command_dtoverlay(char *);
void	command_dtoverlays(char *);
#endif
void	command_modules(char *);
void	command_load(char *);
void	command_unload(char *);
void	command_ls(char *);
void	command_gop(char *);
void	command_mem(char *);
void	command_menu(char *);
void	command_reset(char *);
void	command_setup(char *);
void	command_userconf(char *);
void	command_version(char *);
void	command_quit(char *);

const struct boot_command commands[] = {
#ifdef EFIBOOT_ACPI
	{ "acpi",	command_acpi,		"acpi [{on|off}]" },
#endif
	{ "boot",	command_boot,		"boot [dev:][filename] [args]\n     (ex. \"hd0a:\\netbsd.old -s\"" },
	{ "dev",	command_dev,		"dev" },
#ifdef EFIBOOT_FDT
	{ "dtb",	command_dtb,		"dtb [dev:][filename]" },
	{ "dtoverlay",	command_dtoverlay,	"dtoverlay [dev:][filename]" },
	{ "dtoverlays",	command_dtoverlays,	"dtoverlays [{on|off|reset}]" },
#endif
	{ "initrd",	command_initrd,		"initrd [dev:][filename]" },
	{ "fs",		command_initrd,		NULL },
	{ "rndseed",	command_rndseed,	"rndseed [dev:][filename]" },
	{ "modules",	command_modules,	"modules [{on|off|reset}]" },
	{ "load",	command_load,		"load <module_name>" },
	{ "unload",	command_unload,		"unload <module_name>" },
	{ "ls",		command_ls,		"ls [hdNn:/path]" },
	{ "gop",	command_gop,		"gop [mode]" },
	{ "mem",	command_mem,		"mem" },
	{ "menu",	command_menu,		"menu" },
	{ "reboot",	command_reset,		"reboot|reset" },
	{ "reset",	command_reset,		NULL },
	{ "setup",	command_setup,		"setup" },
	{ "userconf",	command_userconf,	"userconf <command>" },
	{ "version",	command_version,	"version" },
	{ "ver",	command_version,	NULL },
	{ "help",	command_help,		"help|?" },
	{ "?",		command_help,		NULL },
	{ "quit",	command_quit,		"quit" },
	{ NULL,		NULL },
};

static int
bootcfg_path(char *pathbuf, size_t pathbuflen)
{

	/*
	 * Fallback to default_device
	 * - for ISO9660 (efi_file_path() succeeds but does not work correctly)
	 * - or whenever efi_file_path() fails (due to broken firmware)
	 */
	if (default_fstype == FS_ISO9660 || efi_bootdp == NULL ||
	    efi_file_path(efi_bootdp, BOOTCFG_FILENAME, pathbuf, pathbuflen))
		snprintf(pathbuf, pathbuflen, "%s:%s", default_device,
		    BOOTCFG_FILENAME);

	return 0;
}

void
command_help(char *arg)
{
	int n;

	printf("commands are:\n");
	for (n = 0; commands[n].c_name; n++) {
		if (commands[n].c_help)
			printf("%s\n", commands[n].c_help);
	}
}

#ifdef EFIBOOT_ACPI
void
command_acpi(char *arg)
{
	if (arg && *arg) {
		if (strcmp(arg, "on") == 0)
			efi_acpi_enable(1);
		else if (strcmp(arg, "off") == 0)
			efi_acpi_enable(0);
		else {
			command_help("");
			return;
		}
	} else {
		printf("ACPI support is %sabled\n",
		    efi_acpi_enabled() ? "en" : "dis");
	}
}
#endif

void
command_boot(char *arg)
{
	char *fname = arg;
	const char *kernel = *fname ? fname : bootfile;
	char *bootargs = gettrailer(arg);

	if (!kernel || !*kernel)
		kernel = DEFFILENAME;

	if (!*bootargs)
		bootargs = netbsd_args;

	efi_block_set_readahead(true);
	exec_netbsd(kernel, bootargs);
	efi_block_set_readahead(false);
}

void
command_dev(char *arg)
{
	if (arg && *arg) {
		set_default_device(arg);
	} else {
		efi_block_show();
		efi_net_show();
	}

	if (strlen(default_device) > 0) {
		printf("\n");
		printf("default: %s\n", default_device);
	}
}

void
command_initrd(char *arg)
{
	set_initrd_path(arg);
}

void
command_rndseed(char *arg)
{
	set_rndseed_path(arg);
}

#ifdef EFIBOOT_FDT
void
command_dtb(char *arg)
{
	set_dtb_path(arg);
}

void
command_dtoverlays(char *arg)
{
	if (arg && *arg) {
		if (strcmp(arg, "on") == 0)
			dtoverlay_enable(1);
		else if (strcmp(arg, "off") == 0)
			dtoverlay_enable(0);
		else if (strcmp(arg, "reset") == 0)
			dtoverlay_remove_all();
		else {
			command_help("");
			return;
		}
	} else {
		printf("Device Tree overlays are %sabled\n",
		    dtoverlay_enabled ? "en" : "dis");
	}
}

void
command_dtoverlay(char *arg)
{
	if (!arg || !*arg) {
		command_help("");
		return;
	}

	dtoverlay_add(arg);
}
#endif

void
command_modules(char *arg)
{
	if (arg && *arg) {
		if (strcmp(arg, "on") == 0)
			module_enable(1);
		else if (strcmp(arg, "off") == 0)
			module_enable(0);
		else if (strcmp(arg, "reset") == 0)
			module_remove_all();
		else {
			command_help("");
			return;
		}
	} else {
		printf("modules are %sabled\n", module_enabled ? "en" : "dis");
	}
}

void
command_load(char *arg)
{
	if (!arg || !*arg) {
		command_help("");
		return;
	}

	module_add(arg);
}

void
command_unload(char *arg)
{
	if (!arg || !*arg) {
		command_help("");
		return;
	}

	module_remove(arg);
}

void
command_ls(char *arg)
{
	ls(arg);
}

void
command_gop(char *arg)
{
	UINT32 mode;

	if (!arg || !*arg) {
		efi_gop_dump();
		return;
	}

	mode = atoi(arg);
	efi_gop_setmode(mode);
}

void
command_mem(char *arg)
{
	EFI_MEMORY_DESCRIPTOR *md, *memmap;
	UINTN nentries, mapkey, descsize;
	UINT32 descver;
	int n;

	printf("Type                    Start             End               Attributes\n");
	printf("----------------------  ----------------  ----------------  ----------------\n");
	memmap = LibMemoryMap(&nentries, &mapkey, &descsize, &descver);
	for (n = 0, md = memmap; n < nentries; n++, md = NextMemoryDescriptor(md, descsize)) {
		const char *mem_type = "<unknown>";
		if (md->Type < __arraycount(efi_memory_type))
			mem_type = efi_memory_type[md->Type];

		printf("%-22s  %016" PRIx64 "  %016" PRIx64 "  %016" PRIx64 "\n",
		    mem_type, md->PhysicalStart, md->PhysicalStart + (md->NumberOfPages * EFI_PAGE_SIZE) - 1,
		    md->Attribute);
	}
}

void
command_menu(char *arg)
{
	if (bootcfg_info.nummenu == 0) {
		printf("No menu defined in boot.cfg\n");
		return;
	}

	doboottypemenu();	/* Does not return */
}

void
command_version(char *arg)
{
	char pathbuf[80];
	char *ufirmware;
	const UINT64 *osindsup;
	int rv;

	printf("Version: %s (%s)\n", bootprog_rev, bootprog_kernrev);
	printf("EFI: %d.%02d\n",
	    ST->Hdr.Revision >> 16, ST->Hdr.Revision & 0xffff);
	ufirmware = NULL;
	rv = ucs2_to_utf8(ST->FirmwareVendor, &ufirmware);
	if (rv == 0) {
		printf("Firmware: %s (rev 0x%x)\n", ufirmware,
		    ST->FirmwareRevision);
		FreePool(ufirmware);
	}
	if (bootcfg_path(pathbuf, sizeof(pathbuf)) == 0) {
		printf("Config path: %s\n", pathbuf);
	}

	osindsup = LibGetVariable(L"OsIndicationsSupported", &EfiGlobalVariable);
	if (osindsup != NULL) {
		printf("UEFI OS indications supported: 0x%" PRIx64 "\n", *osindsup);
	}

#ifdef EFIBOOT_FDT
	efi_fdt_show();
#endif
#ifdef EFIBOOT_ACPI
	efi_acpi_show();
#endif
	efi_rng_show();
	efi_md_show();
	efi_gop_show();
}

void
command_quit(char *arg)
{
	efi_exit();
}

void
command_reset(char *arg)
{
	efi_reboot();
}

void
command_setup(char *arg)
{
	EFI_STATUS status;
	const UINT64 *osindsup;
	UINT64 osind;

	osindsup = LibGetVariable(L"OsIndicationsSupported", &EfiGlobalVariable);
	if (osindsup == NULL || (*osindsup & EFI_OS_INDICATIONS_BOOT_TO_FW_UI) == 0) {
		printf("Not supported by firmware\n");
		return;
	}

	osind = EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
	status = LibSetNVVariable(L"OsIndications", &EfiGlobalVariable, sizeof(osind), &osind);
	if (EFI_ERROR(status)) {
		printf("Failed to set OsIndications variable: %lu\n", (u_long)status);
		return;
	}

	efi_reboot();
}

void
command_userconf(char *arg)
{
	userconf_add(arg);
}

int
set_default_device(const char *arg)
{
	if (strlen(arg) + 1 > sizeof(default_device))
		return ERANGE;
	strcpy(default_device, arg);
	return 0;
}

char *
get_default_device(void)
{
	return default_device;
}

void
set_default_fstype(int fstype)
{
	default_fstype = fstype;
}

int
get_default_fstype(void)
{
	return default_fstype;
}

int
set_initrd_path(const char *arg)
{
	if (strlen(arg) + 1 > sizeof(initrd_path))
		return ERANGE;
	strcpy(initrd_path, arg);
	return 0;
}

char *
get_initrd_path(void)
{
	return initrd_path;
}

int
set_dtb_path(const char *arg)
{
	if (strlen(arg) + 1 > sizeof(dtb_path))
		return ERANGE;
	strcpy(dtb_path, arg);
	return 0;
}

char *
get_dtb_path(void)
{
	return dtb_path;
}

int
set_rndseed_path(const char *arg)
{
	if (strlen(arg) + 1 > sizeof(rndseed_path))
		return ERANGE;
	strcpy(rndseed_path, arg);
	return 0;
}

char *
get_rndseed_path(void)
{
	return rndseed_path;
}

int
set_bootfile(const char *arg)
{
	if (strlen(arg) + 1 > sizeof(netbsd_path))
		return ERANGE;
	strcpy(netbsd_path, arg);
	return 0;
}

int
set_bootargs(const char *arg)
{
	if (strlen(arg) + 1 > sizeof(netbsd_args))
		return ERANGE;
	strcpy(netbsd_args, arg);
	return 0;
}

void
boot(void)
{
	char pathbuf[80];
	int currname, c;

	if (bootcfg_path(pathbuf, sizeof(pathbuf)) == 0) {
		twiddle_toggle = 1;
		parsebootconf(pathbuf);
	}

	if (bootcfg_info.clear)
		uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	print_bootcfg_banner(bootprog_name, bootprog_rev);

	/* Display menu if configured */
	if (bootcfg_info.nummenu > 0) {
		doboottypemenu();	/* No return */
	}

	printf("Press return to boot now, any other key for boot prompt\n");

	if (netbsd_path[0] != '\0')
		currname = -1;
	else
		currname = 0;

	for (; currname < (int)NUMNAMES; currname++) {
		if (currname >= 0)
			set_bootfile(names[currname]);
		printf("booting %s%s%s - starting in ", netbsd_path,
		    netbsd_args[0] != '\0' ? " " : "", netbsd_args);

		c = awaitkey(bootcfg_info.timeout, 1);
		if (c != '\r' && c != '\n' && c != '\0')
			bootprompt(); /* does not return */

		efi_block_set_readahead(true);
		exec_netbsd(netbsd_path, netbsd_args);
		efi_block_set_readahead(false);
	}

	bootprompt();	/* does not return */
}
