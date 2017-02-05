/*	$NetBSD: boot.c,v 1.2.2.2 2017/02/05 13:40:12 skrll Exp $	*/

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

#include <sys/bootblock.h>
#include <sys/boot_flag.h>

#include <lib/libsa/bootcfg.h>

#include <bootmod.h>
#include <bootmenu.h>
#include "devopen.h"

int errno;
int boot_biosdev;
daddr_t boot_biossector;

extern const char bootprog_name[], bootprog_rev[], bootprog_kernrev[];

extern struct x86_boot_params boot_params;
extern char twiddle_toggle;

static const char * const names[][2] = {
	{ "netbsd", "netbsd.gz" },
	{ "onetbsd", "onetbsd.gz" },
	{ "netbsd.old", "netbsd.old.gz" },
};

#define NUMNAMES	__arraycount(names)
#define DEFFILENAME	names[0][0]

#define	MAXDEVNAME	16

void	command_help(char *);
void	command_quit(char *);
void	command_boot(char *);
void	command_consdev(char *);
void	command_dev(char *);
void	command_devpath(char *);
void	command_efivar(char *);
void	command_gop(char *);
#if LIBSA_ENABLE_LS_OP
void	command_ls(char *);
#endif
void	command_memmap(char *);
#ifndef SMALL
void	command_menu(char *);
#endif
void	command_modules(char *);
void	command_multiboot(char *);
void	command_text(char *);
void	command_version(char *);

const struct bootblk_command commands[] = {
	{ "help",	command_help },
	{ "?",		command_help },
	{ "quit",	command_quit },
	{ "boot",	command_boot },
	{ "consdev",	command_consdev },
	{ "dev",	command_dev },
	{ "devpath",	command_devpath },
	{ "efivar",	command_efivar },
	{ "fs",		fs_add },
	{ "gop",	command_gop },
	{ "load",	module_add },
#if LIBSA_ENABLE_LS_OP
	{ "ls",		command_ls },
#endif
	{ "memmap",	command_memmap },
#ifndef SMALL
	{ "menu",	command_menu },
#endif
	{ "modules",	command_modules },
	{ "multiboot",	command_multiboot },
	{ "rndseed",	rnd_add },
	{ "splash",	splash_add },
	{ "text",	command_text },
	{ "userconf",	userconf_add },
	{ "version",	command_version },
	{ NULL,		NULL },
};

static char *default_devname;
static int default_unit, default_partition;
static const char *default_filename;

static char *sprint_bootsel(const char *);
static void bootit(const char *, int);

int
parsebootfile(const char *fname, char **fsname, char **devname, int *unit,
    int *partition, const char **file)
{
	const char *col;

	*fsname = "ufs";
	*devname = default_devname;
	*unit = default_unit;
	*partition = default_partition;
	*file = default_filename;

	if (fname == NULL)
		return 0;

	if ((col = strchr(fname, ':')) != NULL) {	/* device given */
		static char savedevname[MAXDEVNAME+1];
		int devlen;
		int u = 0, p = 0;
		int i = 0;

		devlen = col - fname;
		if (devlen > MAXDEVNAME)
			return EINVAL;

#define isvalidname(c) ((c) >= 'a' && (c) <= 'z')
		if (!isvalidname(fname[i]))
			return EINVAL;
		do {
			savedevname[i] = fname[i];
			i++;
		} while (isvalidname(fname[i]));
		savedevname[i] = '\0';

#define isnum(c) ((c) >= '0' && (c) <= '9')
		if (i < devlen) {
			if (!isnum(fname[i]))
				return EUNIT;
			do {
				u *= 10;
				u += fname[i++] - '0';
			} while (isnum(fname[i]));
		}

#define isvalidpart(c) ((c) >= 'a' && (c) <= 'z')
		if (i < devlen) {
			if (!isvalidpart(fname[i]))
				return EPART;
			p = fname[i++] - 'a';
		}

		if (i != devlen)
			return ENXIO;

		*devname = savedevname;
		*unit = u;
		*partition = p;
		fname = col + 1;
	}

	if (*fname)
		*file = fname;

	return 0;
}

static char *
sprint_bootsel(const char *filename)
{
	char *fsname, *devname;
	int unit, partition;
	const char *file;
	static char buf[80];

	if (parsebootfile(filename, &fsname, &devname, &unit,
			  &partition, &file) == 0) {
		snprintf(buf, sizeof(buf), "%s%d%c:%s", devname, unit,
		    'a' + partition, file);
		return buf;
	}
	return "(invalid)";
}

void
clearit(void)
{

	if (bootcfg_info.clear)
		clear_pc_screen();
}

static void
bootit(const char *filename, int howto)
{

	if (howto & AB_VERBOSE)
		printf("booting %s (howto 0x%x)\n", sprint_bootsel(filename),
		    howto);

	if (exec_netbsd(filename, 0, howto, 0, efi_cleanup) < 0)
		printf("boot: %s: %s\n", sprint_bootsel(filename),
		       strerror(errno));
	else
		printf("boot returned\n");
}

void
print_banner(void)
{
	int n;

	clearit();
	if (bootcfg_info.banner[0]) {
		for (n = 0; n < BOOTCFG_MAXBANNER && bootcfg_info.banner[n];
		    n++)
			printf("%s\n", bootcfg_info.banner[n]);
	} else
		command_version("short");
}

void
boot(void)
{
	int currname;
	int c;

	boot_modules_enabled = !(boot_params.bp_flags & X86_BP_FLAGS_NOMODULES);

	/* try to set default device to what BIOS tells us */
	bios2dev(boot_biosdev, boot_biossector, &default_devname, &default_unit,
	    &default_partition);

	/* if the user types "boot" without filename */
	default_filename = DEFFILENAME;

	if (!(boot_params.bp_flags & X86_BP_FLAGS_NOBOOTCONF)) {
		parsebootconf(BOOTCFG_FILENAME);
	} else {
		bootcfg_info.timeout = boot_params.bp_timeout;
	}

	/*
	 * If console set in boot.cfg, switch to it.
	 * This will print the banner, so we don't need to explicitly do it
	 */
	if (bootcfg_info.consdev)
		command_consdev(bootcfg_info.consdev);
	else
		print_banner();

	/* Display the menu, if applicable */
	twiddle_toggle = 0;
	if (bootcfg_info.nummenu > 0) {
		/* Does not return */
		doboottypemenu();
	}

	printf("Press return to boot now, any other key for boot menu\n");
	for (currname = 0; currname < NUMNAMES; currname++) {
		printf("booting %s - starting in ",
		       sprint_bootsel(names[currname][0]));

		c = awaitkey((bootcfg_info.timeout < 0) ? 0
		    : bootcfg_info.timeout, 1);
		if ((c != '\r') && (c != '\n') && (c != '\0')) {
		    if ((boot_params.bp_flags & X86_BP_FLAGS_PASSWORD) == 0) {
			/* do NOT ask for password */
			bootmenu(); /* does not return */
		    } else {
			/* DO ask for password */
			if (check_password((char *)boot_params.bp_password)) {
			    /* password ok */
			    printf("type \"?\" or \"help\" for help.\n");
			    bootmenu(); /* does not return */
			} else {
			    /* bad password */
			    printf("Wrong password.\n");
			    currname = 0;
			    continue;
			}
		    }
		}

		/*
		 * try pairs of names[] entries, foo and foo.gz
		 */
		/* don't print "booting..." again */
		bootit(names[currname][0], 0);
		/* since it failed, try compressed bootfile. */
		bootit(names[currname][1], AB_VERBOSE);
	}

	bootmenu();	/* does not return */
}

/* ARGSUSED */
void
command_help(char *arg)
{

	printf("commands are:\n"
	       "boot [xdNx:][filename] [-12acdqsvxz]\n"
	       "     (ex. \"hd0a:netbsd.old -s\"\n"
	       "dev [xd[N[x]]:]\n"
	       "consdev {pc|com[0123]|com[0123]kbd|auto}\n"
	       "devpath\n"
	       "efivar\n"
	       "gop [{modenum|list}]\n"
	       "load {path_to_module}\n"
#if LIBSA_ENABLE_LS_OP
	       "ls [path]\n"
#endif
	       "memmap [{sorted|unsorted}]\n"
#ifndef SMALL
	       "menu (reenters boot menu, if defined in boot.cfg)\n"
#endif
	       "modules {on|off|enabled|disabled}\n"
	       "multiboot [xdNx:][filename] [<args>]\n"
	       "rndseed {path_to_rndseed_file}\n"
	       "splash {path_to_image_file}\n"
	       "text [{modenum|list}]\n"
	       "userconf {command}\n"
	       "version\n"
	       "help|?\n"
	       "quit\n");
}

#if LIBSA_ENABLE_LS_OP
void
command_ls(char *arg)
{
	const char *save = default_filename;

	default_filename = "/";
	ls(arg);
	default_filename = save;
}
#endif

/* ARGSUSED */
void
command_quit(char *arg)
{

	printf("Exiting...\n");
	delay(1 * 1000 * 1000);
	reboot();
	/* Note: we shouldn't get to this point! */
	panic("Could not reboot!");
}

void
command_boot(char *arg)
{
	char *filename;
	int howto;

	if (!parseboot(arg, &filename, &howto))
		return;

	if (filename != NULL) {
		bootit(filename, howto);
	} else {
		int i;

		if (howto == 0)
			bootdefault();
		for (i = 0; i < NUMNAMES; i++) {
			bootit(names[i][0], howto);
			bootit(names[i][1], howto);
		}
	}
}

void
command_dev(char *arg)
{
	static char savedevname[MAXDEVNAME + 1];
	char *fsname, *devname;
	const char *file; /* dummy */

	if (*arg == '\0') {
		biosdisk_probe();
		printf("default %s%d%c\n", default_devname, default_unit,
		       'a' + default_partition);
		return;
	}

	if (strchr(arg, ':') == NULL ||
	    parsebootfile(arg, &fsname, &devname, &default_unit,
	      &default_partition, &file)) {
		command_help(NULL);
		return;
	}

	/* put to own static storage */
	strncpy(savedevname, devname, MAXDEVNAME + 1);
	default_devname = savedevname;
}

/* ARGSUSED */
void
command_consdev(char *arg)
{

	/* XXX not implemented yet */
}

#ifndef SMALL
/* ARGSUSED */
void
command_menu(char *arg)
{

	if (bootcfg_info.nummenu > 0) {
		/* Does not return */
		doboottypemenu();
	} else
		printf("No menu defined in boot.cfg\n");
}
#endif /* !SMALL */

void
command_modules(char *arg)
{

	if (strcmp(arg, "enabled") == 0 ||
	    strcmp(arg, "on") == 0)
		boot_modules_enabled = true;
	else if (strcmp(arg, "disabled") == 0 ||
	    strcmp(arg, "off") == 0)
		boot_modules_enabled = false;
	else
		printf("invalid flag, must be 'enabled' or 'disabled'.\n");
}

void
command_multiboot(char *arg)
{
	char *filename;

	filename = arg;
	if (exec_multiboot(filename, gettrailer(arg)) < 0)
		printf("multiboot: %s: %s\n", sprint_bootsel(filename),
		       strerror(errno));
	else
		printf("boot returned\n");
}

void
command_version(char *arg)
{

	if (strcmp(arg, "full") == 0) {
		printf("ImageBase: 0x%" PRIxPTR "\n",
		    (uintptr_t)efi_li->ImageBase);
		printf("Stack: 0x%" PRIxPTR "\n", efi_main_sp);
		printf("EFI version: %d.%02d\n",
		    ST->Hdr.Revision >> 16, ST->Hdr.Revision & 0xffff);
		Print(L"EFI Firmware: %s (rev %d.%02d)\n", ST->FirmwareVendor,
		    ST->FirmwareRevision >> 16, ST->FirmwareRevision & 0xffff);
	}

	printf("\n"
	    ">> %s, Revision %s (from NetBSD %s)\n"
	    ">> Memory: %d/%d k\n",
	    bootprog_name, bootprog_rev, bootprog_kernrev,
	    getbasemem(), getextmem());
}

void
command_memmap(char *arg)
{
	bool sorted = true;

	if (*arg == '\0' || strcmp(arg, "sorted") == 0)
		/* Already sorted is true. */;
	else if (strcmp(arg, "unsorted") == 0)
		sorted = false;
	else {
		printf("invalid flag, "
		    "must be 'sorted' or 'unsorted'.\n");
		return;
	}

	efi_memory_show_map(sorted);
}

void
command_devpath(char *arg)
{
	EFI_STATUS status;
	UINTN i, nhandles;
	EFI_HANDLE *handles;
	EFI_DEVICE_PATH *dp0, *dp;
	CHAR16 *path;
	UINTN cols, rows, row = 0;

	status = uefi_call_wrapper(ST->ConOut->QueryMode, 4, ST->ConOut,
	    ST->ConOut->Mode->Mode, &cols, &rows);
	if (EFI_ERROR(status) || rows <= 2)
		rows = 0;
	else
		rows -= 2;

	/*
	 * all devices.
	 */
	status = LibLocateHandle(ByProtocol, &DevicePathProtocol, NULL,
	    &nhandles, &handles);
	if (EFI_ERROR(status))
		return;

	for (i = 0; i < nhandles; i++) {
		status = uefi_call_wrapper(BS->HandleProtocol, 3, handles[i],
		    &DevicePathProtocol, (void **)&dp0);
		if (EFI_ERROR(status))
			break;

		Print(L"DevicePathType %d\n", DevicePathType(dp0));
		for (dp = dp0;
		     !IsDevicePathEnd(dp);
		     dp = NextDevicePathNode(dp)) {
			path = DevicePathToStr(dp);
			Print(L"%d:%d:%s\n", DevicePathType(dp), DevicePathSubType(dp), path);
			FreePool(path);

			if (++row >= rows) {
				row = 0;
				Print(L"Press Any Key to continue :");
				(void) awaitkey(-1, 0);
				Print(L"\n");
			}
		}
	}
}

void
command_efivar(char *arg)
{
	static const CHAR16 header[] =
	 L"GUID                                Variable Name        Value\n"
	 L"=================================== ==================== ========\n";
	EFI_STATUS status;
	UINTN sz = 64;
	CHAR16 *name = NULL, *tmp, *val;
	EFI_GUID vendor;
	UINTN cols, rows, row = 0;

	status = uefi_call_wrapper(ST->ConOut->QueryMode, 4, ST->ConOut,
	    ST->ConOut->Mode->Mode, &cols, &rows);
	if (EFI_ERROR(status) || rows <= 2)
		rows = 0;
	else
		rows -= 2;

	name = AllocatePool(sz);
	if (name == NULL) {
		Print(L"memory allocation failed: %ld bytes\n",
		    (UINT64)sz);
		return;
	}

	name[0] = 0;
	vendor = NullGuid;

	Print(L"%s", header);
	for (;;) {
		status = uefi_call_wrapper(RT->GetNextVariableName, 3,
		    &sz, name, &vendor);
		if (EFI_ERROR(status)) {
			if (status == EFI_NOT_FOUND)
				break;
			if (status != EFI_BUFFER_TOO_SMALL) {
				Print(L"GetNextVariableName failed: %r\n",
				    status);
				break;
			}

			tmp = AllocatePool(sz);
			if (tmp == NULL) {
				Print(L"memory allocation failed: %ld bytes\n",
				    (UINT64)sz);
				break;
			}
			FreePool(name);
			name = tmp;
		}

		val = LibGetVariable(name, &vendor);
		Print(L"%.-35g %.-20s %s\n", &vendor, name,
		    val ? val : L"(null)");
		FreePool(val);

		if (++row >= rows) {
			row = 0;
			Print(L"Press Any Key to continue :");
			(void) awaitkey(-1, 0);
			Print(L"\n");
		}
	}

	FreePool(name);
}
