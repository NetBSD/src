/*	$NetBSD: main.c,v 1.24.2.2 2010/11/09 06:03:38 uebayasi Exp $	*/

/*
 * Copyright (c) 1996
 * 	Matthias Drochner.  All rights reserved.
 * Copyright (c) 1996
 * 	Perry E. Metzger.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgements:
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 *	This product includes software developed for the NetBSD Project
 *	by Perry E. Metzger.
 * 4. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/types.h>
#include <sys/reboot.h>
#include <sys/bootblock.h>

#include <lib/libkern/libkern.h>

#include <lib/libsa/stand.h>

#include <libi386.h>
#include <bootmenu.h>
#include <bootmod.h>
#include "pxeboot.h"
#include "vbe.h"

extern struct x86_boot_params boot_params;

int errno;
int debug;

extern char	bootprog_name[], bootprog_rev[], bootprog_kernrev[];

int	main(void);

void	command_help(char *);
void	command_quit(char *);
void	command_boot(char *);
void	command_consdev(char *);
void	command_modules(char *);
void	command_multiboot(char *);

const struct bootblk_command commands[] = {
	{ "help",	command_help },
	{ "?",		command_help },
	{ "quit",	command_quit },
	{ "boot",	command_boot },
	{ "consdev",	command_consdev },
	{ "modules",    command_modules },
	{ "multiboot",  command_multiboot },
	{ "load",	module_add },
	{ "vesa",	command_vesa },
	{ NULL,		NULL },
};

static void
clearit(void)
{

	if (bootconf.clear)
		clear_pc_screen();
}

static void
alldone(void)
{
	pxe_fini();
	clearit();
}

static int 
bootit(const char *filename, int howto)
{
	if (exec_netbsd(filename, 0, howto, 0, alldone) < 0)
		printf("boot: %s\n", strerror(errno));
	else
		printf("boot returned\n");
	return (-1);
}

static void
print_banner(void)
{
	int base = getbasemem();
	int ext = getextmem();

	clearit();
	printf("\n"
	       ">> NetBSD/x86 PXE boot, Revision %s (from NetBSD %s)\n"
	       ">> Memory: %d/%d k\n",
	       bootprog_rev, bootprog_kernrev,
	       base, ext);
}

int
main(void)
{
	extern char twiddle_toggle;
        char c;

	twiddle_toggle = 1;	/* no twiddling until we're ready */

#ifdef SUPPORT_SERIAL
	initio(SUPPORT_SERIAL);
#else
	initio(CONSDEV_PC);
#endif
	gateA20();
	boot_modules_enabled = !(boot_params.bp_flags
				 & X86_BP_FLAGS_NOMODULES);

#ifndef SMALL
	if (!(boot_params.bp_flags & X86_BP_FLAGS_NOBOOTCONF))
		parsebootconf(BOOTCONF);

	/*
	 * If console set in boot.cfg, switch to it.
	 * This will print the banner, so we don't need to explicitly do it
	 */
	if (bootconf.consdev)
		command_consdev(bootconf.consdev);
	else 
		print_banner();

	/* Display the menu, if applicable */
	twiddle_toggle = 0;
	if (bootconf.nummenu > 0) {
		/* Does not return */
		doboottypemenu();
	}
#else
	twiddle_toggle = 0;
	print_banner();
#endif

	printf("Press return to boot now, any other key for boot menu\n");
	printf("booting netbsd - starting in ");

#ifdef SMALL
	c = awaitkey(boot_params.bp_timeout, 1);
#else
	c = awaitkey((bootconf.timeout < 0) ? 0 : bootconf.timeout, 1);
#endif
	if ((c != '\r') && (c != '\n') && (c != '\0') &&
	    ((boot_params.bp_flags & X86_BP_FLAGS_PASSWORD) == 0
	     || check_password((char *)boot_params.bp_password))) {
		printf("type \"?\" or \"help\" for help.\n");
		bootmenu(); /* does not return */
	}

	/*
	 * The file name provided here is just a default.  If the
	 * DHCP server provides a file name, we'll use that instead.
	 */
	bootit("netbsd", 0);

	/*
	 * If that fails, let the BIOS try the next boot device.
	 */
	return (1);
}

/* ARGSUSED */
void
command_help(char *arg)
{
	printf("commands are:\n"
	       "boot [filename] [-adsqv]\n"
	       "     (ex. \"netbsd.old -s\"\n"
	       "consdev {pc|com[0123]|com[0123]kbd|auto}\n"
	       "vesa {modenum|on|off|enabled|disabled|list}\n"
	       "multiboot [filename] [<args>]\n"
	       "modules {on|off|enabled|disabled}\n"
	       "load {path_to_module}\n"
	       "help|?\n"
	       "quit\n");
}

/* ARGSUSED */
void
command_quit(char *arg)
{

	printf("Exiting...\n");
	delay(1000000);
	reboot();
	/* Note: we shouldn't get to this point! */
	panic("Could not reboot!");
	exit(0);
}

void
command_boot(char *arg)
{
	char *filename;
	int howto;

	if (parseboot(arg, &filename, &howto))
		bootit(filename, howto);
}

static const struct cons_devs {
    const char	*name;
    u_int	tag;
} cons_devs[] = {
	{ "pc",		CONSDEV_PC },
	{ "com0",	CONSDEV_COM0 },
	{ "com1",	CONSDEV_COM1 },
	{ "com2",	CONSDEV_COM2 },
	{ "com3",	CONSDEV_COM3 },
	{ "com0kbd",	CONSDEV_COM0KBD },
	{ "com1kbd",	CONSDEV_COM1KBD },
	{ "com2kbd",	CONSDEV_COM2KBD },
	{ "com3kbd",	CONSDEV_COM3KBD },
	{ "auto",	CONSDEV_AUTO },
	{ 0, 0 } };

void
command_consdev(char *arg)
{
	const struct cons_devs *cdp;

	for (cdp = cons_devs; cdp->name; cdp++) {
		if (!strcmp(arg, cdp->name)) {
			initio(cdp->tag);
			print_banner();
			return;
		}
	}
	printf("invalid console device.\n");
}
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
		printf("multiboot: %s: %s\n", filename,
		  strerror(errno));
	else
		printf("boot returned\n");
}
