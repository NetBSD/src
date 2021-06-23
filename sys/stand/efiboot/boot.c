/*	$NetBSD: boot.c,v 1.34 2021/06/23 21:43:38 jmcneill Exp $	*/

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
#include "efifdt.h"
#include "efiacpi.h"
#include "efirng.h"
#include "module.h"
#include "overlay.h"
#include "bootmenu.h"

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

#define	DEFTIMEOUT	5
#define DEFFILENAME	names[0]

int	set_bootfile(const char *);
int	set_bootargs(const char *);

void	command_boot(char *);
void	command_dev(char *);
void	command_dtb(char *);
void	command_initrd(char *);
void	command_rndseed(char *);
void	command_dtoverlay(char *);
void	command_dtoverlays(char *);
void	command_modules(char *);
void	command_load(char *);
void	command_unload(char *);
void	command_ls(char *);
void	command_mem(char *);
void	command_menu(char *);
void	command_reset(char *);
void	command_version(char *);
void	command_quit(char *);

const struct boot_command commands[] = {
	{ "boot",	command_boot,		"boot [dev:][filename] [args]\n     (ex. \"hd0a:\\netbsd.old -s\"" },
	{ "dev",	command_dev,		"dev" },
	{ "dtb",	command_dtb,		"dtb [dev:][filename]" },
	{ "initrd",	command_initrd,		"initrd [dev:][filename]" },
	{ "rndseed",	command_rndseed,	"rndseed [dev:][filename]" },
	{ "dtoverlay",	command_dtoverlay,	"dtoverlay [dev:][filename]" },
	{ "dtoverlays",	command_dtoverlays,	"dtoverlays [{on|off|reset}]" },
	{ "modules",	command_modules,	"modules [{on|off|reset}]" },
	{ "load",	command_load,		"load <module_name>" },
	{ "unload",	command_unload,		"unload <module_name>" },
	{ "ls",		command_ls,		"ls [hdNn:/path]" },
	{ "mem",	command_mem,		"mem" },
	{ "menu",	command_menu,		"menu" },
	{ "reboot",	command_reset,		"reboot|reset" },
	{ "reset",	command_reset,		NULL },
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
command_dtb(char *arg)
{
	set_dtb_path(arg);
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

	efi_fdt_show();
	efi_acpi_show();
	efi_rng_show();
	efi_md_show();
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

static void
get_memory_info(uint64_t *ptotal)
{
	EFI_MEMORY_DESCRIPTOR *md, *memmap;
	UINTN nentries, mapkey, descsize;
	UINT32 descver;
	uint64_t totalpg = 0;
	int n;

	memmap = LibMemoryMap(&nentries, &mapkey, &descsize, &descver);
	for (n = 0, md = memmap; n < nentries; n++, md = NextMemoryDescriptor(md, descsize)) {
		if ((md->Attribute & EFI_MEMORY_WB) == 0) {
			continue;
		}
		totalpg += md->NumberOfPages;
	}

	*ptotal = totalpg * EFI_PAGE_SIZE;
}

static void
format_bytes(uint64_t val, uint64_t *pdiv, const char **punit)
{
	static const char *units[] = { "KB", "MB", "GB" };
	unsigned n;

	*punit = "bytes";
	*pdiv = 1;

	for (n = 0; n < __arraycount(units) && val >= 1024 * 10; n++) {
		*punit = units[n];
		*pdiv *= 1024;
		val /= 1024;
	}
}

void
print_banner(void)
{
	const char *total_unit;
	uint64_t total, total_div;

	get_memory_info(&total);
	format_bytes(total, &total_div, &total_unit);

	printf("  \\-__,------,___.\n");
	printf("   \\        __,---`  %s\n", bootprog_name);
	printf("    \\       `---,_.  Revision %s\n", bootprog_rev);
	printf("     \\-,_____,.---`  Memory: %" PRIu64 " %s\n",
	    total / total_div, total_unit);
	printf("      \\\n");
	printf("       \\\n");
	printf("        \\\n\n");
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

	print_banner();

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

		c = awaitkey(DEFTIMEOUT, 1);
		if (c != '\r' && c != '\n' && c != '\0')
			bootprompt(); /* does not return */

		efi_block_set_readahead(true);
		exec_netbsd(netbsd_path, netbsd_args);
		efi_block_set_readahead(false);
	}

	bootprompt();	/* does not return */
}
