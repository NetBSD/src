/*	$NetBSD: boot.c,v 1.12 2018/10/29 05:15:21 mrg Exp $	*/

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
#include "efifdt.h"
#include "efiacpi.h"
#include "efienv.h"

#include <sys/bootblock.h>
#include <sys/boot_flag.h>
#include <machine/limits.h>

#include <loadfile.h>

extern const char bootprog_name[], bootprog_rev[], bootprog_kernrev[];

extern char twiddle_toggle;

static const char * const names[] = {
	"netbsd", "netbsd.gz",
	"onetbsd", "onetbsd.gz",
	"netbsd.old", "netbsd.old.gz",
};

#define NUMNAMES	__arraycount(names)

static char default_device[32];
static char initrd_path[255];
static char dtb_path[255];
static char bootfile[255];

#define	DEFTIMEOUT	5
#define DEFFILENAME	names[0]

int	set_bootfile(const char *);

void	command_boot(char *);
void	command_dev(char *);
void	command_dtb(char *);
void	command_initrd(char *);
void	command_ls(char *);
void	command_printenv(char *);
void	command_setenv(char *);
void	command_clearenv(char *);
void	command_resetenv(char *);
void	command_reset(char *);
void	command_version(char *);
void	command_quit(char *);

const struct boot_command commands[] = {
	{ "boot",	command_boot,		"boot [dev:][filename] [args]\n     (ex. \"hd0a:\\netbsd.old -s\"" },
	{ "dev",	command_dev,		"dev" },
	{ "dtb",	command_dtb,		"dtb [dev:][filename]" },
	{ "initrd",	command_initrd,		"initrd [dev:][filename]" },
	{ "ls",		command_ls,		"ls [hdNn:/path]" },
	{ "printenv",	command_printenv,	"printenv [key]" },
	{ "setenv",	command_setenv,		"setenv <key> <value>" },
	{ "clearenv",	command_clearenv,	"clearenv <key>" },
	{ "resetenv",	command_resetenv,	"resetenv" },
	{ "reboot",	command_reset,		"reboot|reset" },
	{ "reset",	command_reset,		NULL },
	{ "version",	command_version,	"version" },
	{ "help",	command_help,		"help|?" },
	{ "?",		command_help,		NULL },
	{ "quit",	command_quit,		"quit" },
	{ NULL,		NULL },
};

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

	exec_netbsd(kernel, bootargs);
}

void
command_dev(char *arg)
{
	if (arg && *arg) {
		set_default_device(arg);
	} else {
		efi_block_show();
		efi_net_show();
		efi_pxe_show();
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
command_ls(char *arg)
{
	ls(arg);
}

void
command_printenv(char *arg)
{
	char *val;

	if (arg && *arg) {
		val = efi_env_get(arg);
		if (val) {
			printf("\"%s\" = \"%s\"\n", arg, val);
			FreePool(val);
		}
	} else {
		efi_env_print();
	}
}

void
command_setenv(char *arg)
{
	char *spc;

	spc = strchr(arg, ' ');
	if (spc == NULL || spc[1] == '\0') {
		command_help("");
		return;
	}

	*spc = '\0';
	efi_env_set(arg, spc + 1);
}

void
command_clearenv(char *arg)
{
	if (*arg == '\0') {
		command_help("");
		return;
	}
	efi_env_clear(arg);
}

void
command_resetenv(char *arg)
{
	efi_env_reset();
}

void
command_version(char *arg)
{
	char *ufirmware;
	int rv;

	printf("EFI version: %d.%02d\n",
	    ST->Hdr.Revision >> 16, ST->Hdr.Revision & 0xffff);
	ufirmware = NULL;
	rv = ucs2_to_utf8(ST->FirmwareVendor, &ufirmware);
	if (rv == 0) {
		printf("EFI Firmware: %s (rev %d.%02d)\n", ufirmware,
		    ST->FirmwareRevision >> 16,
		    ST->FirmwareRevision & 0xffff);
		FreePool(ufirmware);
	}

	efi_fdt_show();
	efi_acpi_show();
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
set_bootfile(const char *arg)
{
	if (strlen(arg) + 1 > sizeof(bootfile))
		return ERANGE;
	strcpy(bootfile, arg);
	return 0;
}

void
print_banner(void)
{
	printf("\n\n"
	    ">> %s, Revision %s (from NetBSD %s)\n",
	    bootprog_name, bootprog_rev, bootprog_kernrev);
}

static void
read_env(void)
{
	char *s;

	s = efi_env_get("fdtfile");
	if (s) {
#ifdef EFIBOOT_DEBUG
		printf(">> Setting DTB path to '%s' from environment\n", s);
#endif
		set_dtb_path(s);
		FreePool(s);
	}

	s = efi_env_get("initrd");
	if (s) {
#ifdef EFIBOOT_DEBUG
		printf(">> Setting initrd path to '%s' from environment\n", s);
#endif
		set_initrd_path(s);
		FreePool(s);
	}

	s = efi_env_get("bootfile");
	if (s) {
#ifdef EFIBOOT_DEBUG
		printf(">> Setting bootfile path to '%s' from environment\n", s);
#endif
		set_bootfile(s);
		FreePool(s);
	}

	s = efi_env_get("rootdev");
	if (s) {
#ifdef EFIBOOT_DEBUG
		printf(">> Setting default device to '%s' from environment\n", s);
#endif
		set_default_device(s);
		FreePool(s);
	}
}

void
boot(void)
{
	int currname, c;

	read_env();
	print_banner();
	printf("Press return to boot now, any other key for boot prompt\n");

	if (bootfile[0] != '\0')
		currname = -1;
	else
		currname = 0;

	for (; currname < (int)NUMNAMES; currname++) {
		if (currname >= 0)
			set_bootfile(names[currname]);
		printf("booting %s - starting in ", bootfile);

		c = awaitkey(DEFTIMEOUT, 1);
		if (c != '\r' && c != '\n' && c != '\0')
			bootprompt(); /* does not return */

		exec_netbsd(bootfile, "");
	}

	bootprompt();	/* does not return */
}
