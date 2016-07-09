/*	$NetBSD: boot.c,v 1.31.4.1 2016/07/09 20:24:56 skrll Exp $	*/

/*
 * Copyright (c) 1997, 1999 Eduardo E. Horvath.  All rights reserved.
 * Copyright (c) 1997 Jason R. Thorpe.  All rights reserved.
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
 *
 * ELF support derived from NetBSD/alpha's boot loader, written
 * by Christopher G. Demetriou.
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
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * First try for the boot code
 *
 * Input syntax is:
 *	[promdev[{:|,}partition]]/[filename] [flags]
 */

#include <lib/libsa/stand.h>
#include <lib/libsa/bootcfg.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <sys/boot_flag.h>

#include <machine/cpu.h>
#include <machine/promlib.h>
#include <machine/bootinfo.h>
#include <sparc/stand/common/isfloppy.h>

#include "boot.h"
#include "ofdev.h"
#include "openfirm.h"


#define COMPAT_BOOT(marks)	(marks[MARK_START] == marks[MARK_ENTRY])


typedef void (*entry_t)(long o0, long bootargs, long bootsize, long o3,
                        long ofw);

/*
 * Boot device is derived from ROM provided information, or if there is none,
 * this list is used in sequence, to find a kernel.
 */
const char *kernelnames[] = {
	"netbsd",
	"netbsd.gz",
	"netbsd.old",
	"netbsd.old.gz",
	"onetbsd",
	"onetbsd.gz",
	"vmunix ",
#ifdef notyet
	"netbsd.pl ",
	"netbsd.pl.gz ",
	"netbsd.el ",
	"netbsd.el.gz ",
#endif
	NULL
};

char bootdev[PROM_MAX_PATH];
bool root_fs_quickseekable = true;	/* unset for tftp boots */
static bool bootinfo_pass_bootdev = false;

int debug  = 0;
int compatmode = 0;
extern char twiddle_toggle;

#if 0
static void
prom2boot(char *dev)
{
	char *cp, *lp = 0;
	int handle;
	char devtype[16];
	
	for (cp = dev; *cp; cp++)
		if (*cp == ':')
			lp = cp;
	if (!lp)
		lp = cp;
	*lp = 0;
}
#endif

static int
bootoptions(const char *ap, char *loaddev, char *kernel, char *options)
{
	int v = 0;
	const char *start1 = NULL, *end1 = NULL, *start2 = NULL, *end2 = NULL;
	const char *path;
	char partition, *pp;

	*kernel  = '\0';
	*options = '\0';

	if (ap == NULL) {
		return (0);
	}

	while (*ap == ' ') {
		ap++;
	}

	if (*ap != '-') {
		start1 = ap;
		while (*ap != '\0' && *ap != ' ') {
			ap++;
		}
		end1 = ap;

		while (*ap == ' ') {
			ap++;
		}

		if (*ap != '-') {
			start2 = ap;
			while (*ap != '\0' && *ap != ' ') {
				ap++;
			}
			end2 = ap;
			while (*ap != '\0' && *ap == ' ') {
				ap++;
			}
		}
	}
	if (end2 == start2) {
		start2 = end2 = NULL;
	}
	if (end1 == start1) {
		start1 = end1 = NULL;
	}

	if (start1 == NULL) {
		/* only options */
	} else if (start2 == NULL) {
		memcpy(kernel, start1, (end1 - start1));
		kernel[end1 - start1] = '\0';
		path = filename(kernel, &partition);
		if (path == NULL) {
			strcpy(loaddev, kernel);
			kernel[0] = '\0';
		} else if (path != kernel) {
			/* copy device part */
			memcpy(loaddev, kernel, path-kernel);
			loaddev[path-kernel] = '\0';
			if (partition) {
				pp = loaddev + strlen(loaddev);
				pp[0] = ':';
				pp[1] = partition;
				pp[2] = '\0';
			}
			/* and kernel path */
			strcpy(kernel, path);
		}
	} else {
		memcpy(loaddev, start1, (end1-start1));
		loaddev[end1-start1] = '\0';
		memcpy(kernel, start2, (end2 - start2));
		kernel[end2 - start2] = '\0';
	}

	twiddle_toggle = 1;
	strcpy(options, ap);
	while (*ap != '\0' && *ap != ' ' && *ap != '\t' && *ap != '\n') {
		BOOT_FLAG(*ap, v);
		switch(*ap++) {
		case 'D':
			debug = 2;
			break;
		case 'C':
			compatmode = 1;
			break;
		case 'T':
			twiddle_toggle = 1 - twiddle_toggle;
			break;
		default:
			break;
		}
	}

	if (((v & RB_KDB) != 0) && (debug == 0)) {
		debug = 1;
	}

	DPRINTF(("bootoptions: device='%s', kernel='%s', options='%s'\n",
	    loaddev, kernel, options));
	return (v);
}

/*
 * The older (those relying on ofwboot v1.8 and earlier) kernels can't handle
 * ksyms information unless it resides in a dedicated memory allocated from
 * PROM and aligned on NBPG boundary. This is because the kernels calculate
 * their ends on their own, they use address of 'end[]' reference which follows
 * text segment. Ok, allocate some memory from PROM and copy symbol information
 * over there.
 */
static void
ksyms_copyout(void **ssym, void **esym)
{
	uint8_t *addr;
	int kssize = (int)(long)((char *)*esym - (char *)*ssym + 1);

	DPRINTF(("ksyms_copyout(): ssym = %p, esym = %p, kssize = %d\n",
				*ssym, *esym, kssize));

	if ( (addr = OF_claim(0, kssize, NBPG)) == (void *)-1) {
		panic("ksyms_copyout(): no space for symbol table");
	}

	memcpy(addr, *ssym, kssize);
	*ssym = addr;
	*esym = addr + kssize - 1;

	DPRINTF(("ksyms_copyout(): ssym = %p, esym = %p\n", *ssym, *esym));
}

/*
 * Prepare boot information and jump directly to the kernel.
 */
static void
jump_to_kernel(u_long *marks, char *kernel, char *args, void *ofw,
	int boothowto)
{
	int l, machine_tag;
	long newargs[4];
	void *ssym, *esym;
	vaddr_t bootinfo;
	struct btinfo_symtab bi_sym;
	struct btinfo_kernend bi_kend;
	struct btinfo_boothowto bi_howto;
	char *cp;
	char bootline[PROM_MAX_PATH * 2];

	/* Compose kernel boot line. */
	strncpy(bootline, kernel, sizeof(bootline));
	cp = bootline + strlen(bootline);
	if (*args) {
		*cp++ = ' ';
		strncpy(bootline, args, sizeof(bootline) - (cp - bootline));
	}
	*cp = 0; args = bootline;

	/* Record symbol information in the bootinfo. */
	bootinfo = bi_init(marks[MARK_END]);
	bi_sym.nsym = marks[MARK_NSYM];
	bi_sym.ssym = marks[MARK_SYM];
	bi_sym.esym = marks[MARK_END];
	bi_add(&bi_sym, BTINFO_SYMTAB, sizeof(bi_sym));
	bi_kend.addr= bootinfo + BOOTINFO_SIZE;
	bi_add(&bi_kend, BTINFO_KERNEND, sizeof(bi_kend));
	bi_howto.boothowto = boothowto;
	bi_add(&bi_howto, BTINFO_BOOTHOWTO, sizeof(bi_howto));
	if (bootinfo_pass_bootdev) {
		struct {
			struct btinfo_common common;
			char name[256];
		} info;
		
		strcpy(info.name, bootdev);
		bi_add(&info, BTINFO_BOOTDEV, strlen(bootdev)
			+sizeof(struct btinfo_bootdev));
	}

	sparc64_finalize_tlb(marks[MARK_DATA]);
	sparc64_bi_add();

	ssym  = (void*)(long)marks[MARK_SYM];
	esym  = (void*)(long)marks[MARK_END];

	DPRINTF(("jump_to_kernel(): ssym = %p, esym = %p\n", ssym, esym));

	/* Adjust ksyms pointers, if needed. */
	if (COMPAT_BOOT(marks) || compatmode) {
		ksyms_copyout(&ssym, &esym);
	}

	freeall();
	/*
	 * When we come in args consists of a pointer to the boot
	 * string.  We need to fix it so it takes into account
	 * other params such as romp.
	 */

	/*
	 * Stash pointer to end of symbol table after the argument
	 * strings.
	 */
	l = strlen(args) + 1;
	memcpy(args + l, &esym, sizeof(esym));
	l += sizeof(esym);

	/*
	 * Tell the kernel we're an OpenFirmware system.
	 */
	machine_tag = SPARC_MACHINE_OPENFIRMWARE;
	memcpy(args + l, &machine_tag, sizeof(machine_tag));
	l += sizeof(machine_tag);

	/* 
	 * Since we don't need the boot string (we can get it from /chosen)
	 * we won't pass it in.  Just pass in esym and magic #
	 */
	newargs[0] = SPARC_MACHINE_OPENFIRMWARE;
	newargs[1] = (long)esym;
	newargs[2] = (long)ssym;
	newargs[3] = (long)(void*)bootinfo;
	args = (char *)newargs;
	l = sizeof(newargs);

	/* if -D is set then pause in the PROM. */
	if (debug > 1) callrom();

	/*
	 * Jump directly to the kernel. Solaris kernel and Sun PROM
	 * flash updates expect ROMP vector in %o0, so we do. Format
	 * of other parameters and their order reflect OF_chain()
	 * symantics since this is what older NetBSD kernels rely on.
	 * (see sparc64/include/bootinfo.h for specification).
	 */
	DPRINTF(("jump_to_kernel(%lx, %lx, %lx, %lx, %lx) @ %p\n", (long)ofw,
				(long)args, (long)l, (long)ofw, (long)ofw,
				(void*)marks[MARK_ENTRY]));
	(*(entry_t)marks[MARK_ENTRY])((long)ofw, (long)args, (long)l, (long)ofw,
				      (long)ofw);
	printf("Returned from kernel entry point!\n");
}

static void
start_kernel(char *kernel, char *bootline, void *ofw, int isfloppy,
	int boothowto)
{
	int fd;
	u_long marks[MARK_MAX];
	int flags = LOAD_ALL;
	if (isfloppy)
		flags &= ~LOAD_BACKWARDS;

	/*
	 * First, load headers using default allocator and check whether kernel
	 * entry address matches kernel text load address. If yes, this is the
	 * old kernel designed for ofwboot v1.8 and therefore it must be mapped
	 * by PROM. Otherwise, map the kernel with 4MB permanent pages.
	 */
	loadfile_set_allocator(LOADFILE_NOP_ALLOCATOR);
	if ( (fd = loadfile(kernel, marks, LOAD_HDR|COUNT_TEXT)) != -1) {
		if (COMPAT_BOOT(marks) || compatmode) {
			(void)printf("[c] ");
			loadfile_set_allocator(LOADFILE_OFW_ALLOCATOR);
		} else {
			loadfile_set_allocator(LOADFILE_MMU_ALLOCATOR);
		}
		(void)printf("Loading %s: ", kernel);

		if (fdloadfile(fd, marks, flags) != -1) {
			close(fd);
			jump_to_kernel(marks, kernel, bootline, ofw, boothowto);
		}
	}
	(void)printf("Failed to load '%s'.\n", kernel);
}

static void
help(void)
{
	printf( "enter a special command\n"
		"  halt\n"
		"  exit\n"
		"    to return to OpenFirmware\n"
		"  ?\n"
		"  help\n"
		"    to display this message\n"
		"or a boot specification:\n"
		"  [device] [kernel] [options]\n"
		"\n"
		"for example:\n"
		"  disk:a netbsd -s\n");
}

static void
do_config_command(const char *cmd, char *arg)
{
	DPRINTF(("do_config_command: %s\n", cmd));
	if (strcmp(cmd, "bootpartition") == 0) {
		char *c;

		DPRINTF(("switching boot partition to %s from %s\n",
		    arg, bootdev));
		c = strrchr(bootdev, ':');
		if (!c) return;
		if (c[1] == 0) return;
		if (strlen(arg) > strlen(c)) return;
		strcpy(c, arg);
		DPRINTF(("new boot device: %s\n", bootdev));
		bootinfo_pass_bootdev = true;
	}
}

static void
check_boot_config(void)
{
	if (!root_fs_quickseekable)
		return;

	perform_bootcfg(BOOTCFG_FILENAME, &do_config_command, 32768);
}

void
main(void *ofw)
{
	int boothowto, i = 0, isfloppy, kboothowto;

	char kernel[PROM_MAX_PATH];
	char bootline[PROM_MAX_PATH];

	/* Initialize OpenFirmware */
	romp = ofw;
	prom_init();

	printf("\r>> %s, Revision %s\n", bootprog_name, bootprog_rev);

	/* Figure boot arguments */
	strncpy(bootdev, prom_getbootpath(), sizeof(bootdev) - 1);
	kboothowto = boothowto =
	    bootoptions(prom_getbootargs(), bootdev, kernel, bootline);
	isfloppy = bootdev_isfloppy(bootdev);

	for (;; *kernel = '\0') {
		if (boothowto & RB_ASKNAME) {
			char cmdline[PROM_MAX_PATH];

			printf("Boot: ");
			kgets(cmdline, sizeof(cmdline));

			if (!strcmp(cmdline,"exit") ||
			    !strcmp(cmdline,"halt")) {
				prom_halt();
			} else if (!strcmp(cmdline, "?") ||
				   !strcmp(cmdline, "help")) {
				help();
				continue;
			}

			boothowto  = bootoptions(cmdline, bootdev, kernel,
			    bootline);
			boothowto |= RB_ASKNAME;
			i = 0;
		}

		if (*kernel == '\0') {
			if (kernelnames[i] == NULL) {
				boothowto |= RB_ASKNAME;
				continue;
			}
			strncpy(kernel, kernelnames[i++], PROM_MAX_PATH);
		} else if (i == 0) {
			/*
			 * Kernel name was passed via command line -- ask user
			 * again if requested image fails to boot.
			 */
			boothowto |= RB_ASKNAME;
		}

		check_boot_config();
		start_kernel(kernel, bootline, ofw, isfloppy, kboothowto);

		/*
		 * Try next name from kernel name list if not in askname mode,
		 * enter askname on reaching list's end.
		 */
		if ((boothowto & RB_ASKNAME) == 0 && (kernelnames[i] != NULL)) {
			printf(": trying %s...\n", kernelnames[i]);
		} else {
			printf("\n");
			boothowto |= RB_ASKNAME;
		}
	}

	(void)printf("Boot failed! Exiting to the Firmware.\n");
	prom_halt();
}
