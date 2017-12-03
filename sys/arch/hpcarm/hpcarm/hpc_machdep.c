/*	$NetBSD: hpc_machdep.c,v 1.101.2.2 2017/12/03 11:36:14 jdolecek Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *      This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Machine dependent functions for kernel setup.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpc_machdep.c,v 1.101.2.2 2017/12/03 11:36:14 jdolecek Exp $");

#include "opt_cputypes.h"
#include "opt_kloader.h"
#ifndef KLOADER_KERNEL_PATH
#define KLOADER_KERNEL_PATH	"/netbsd"
#endif

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/boot_flag.h>
#include <sys/mount.h>
#include <sys/pmf.h>
#include <sys/reboot.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>

#include <arm/locore.h>

#include <machine/bootconfig.h>
#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/pmap.h>
#ifdef KLOADER
#include <machine/kloader.h>
#endif

#include <dev/cons.h>
#include <dev/hpc/apm/apmvar.h>

BootConfig bootconfig;		/* Boot config storage */
#ifdef KLOADER
struct kloader_bootinfo kbootinfo;
static char kernel_path[] = KLOADER_KERNEL_PATH;
#endif
struct bootinfo *bootinfo, bootinfo_storage;
char booted_kernel_storage[80];
extern char *booted_kernel;

paddr_t physical_start;
paddr_t physical_freestart;
paddr_t physical_freeend;
paddr_t physical_end;

#ifndef PMAP_STATIC_L1S
int max_processes = 64;			/* Default number */
#endif /* !PMAP_STATIC_L1S */

char *boot_args = NULL;
char boot_file[16];

paddr_t msgbufphys;

/* Prototypes */
void dumpsys(void);

/* Mode dependent sleep function holder */
void (*__sleep_func)(void *);
void *__sleep_ctx;

void (*__cpu_reset)(void) __dead = cpu_reset;

u_int initarm(int, char **, struct bootinfo *);
#if defined(CPU_SA1100) || defined(CPU_SA1110)
u_int init_sa11x0(int, char **, struct bootinfo *);
#endif
#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
u_int init_pxa2x0(int, char **, struct bootinfo *);
#endif

#ifdef BOOT_DUMP
void	dumppages(char *, int);
#endif

/*
 * Reboots the system.
 *
 * Deal with any syncing, unmounting, dumping and shutdown hooks,
 * then reset the CPU.
 */
void
cpu_reboot(int howto, char *bootstr)
{

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast.
	 */
	if (cold) {
		doshutdownhooks();
		pmf_system_shutdown(boothowto);
		printf("Halted while still in the ICE age.\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		__cpu_reset();
		/* NOTREACHED */
	}

	/* Reset the sleep function. */
	__sleep_func = NULL;
	__sleep_ctx = NULL;

	/* Disable console buffering. */
	cnpollc(1);

#ifdef KLOADER
	if ((howto & RB_HALT) == 0) {
		if (howto & RB_STRING) {
			kloader_reboot_setup(bootstr);
		} else {
			kloader_reboot_setup(kernel_path);
		}
	}
#endif

	/*
	 * If RB_NOSYNC was not specified sync the discs.
	 * Note: Unless cold is set to 1 here, syslogd will die during
	 * the unmount.  It looks like syslogd is getting woken up only
	 * to find that it cannot page part of the binary in as the
	 * file system has been unmounted.
	 */
	if (!(howto & RB_NOSYNC))
		bootsync();

	/* Say NO to interrupts. */
	(void)splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

	/* Run any shutdown hooks. */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	/* Make sure IRQs are disabled. */
	IRQdisable;

	if (howto & RB_HALT) {
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
#ifdef KLOADER
	} else {
		kloader_reboot();
		/* NOTREACHED */
#endif
	}

	printf("rebooting...\n");
	__cpu_reset();
	/* NOTREACHED */
}

void
machine_sleep(void)
{

	if (__sleep_func != NULL)
		__sleep_func(__sleep_ctx);
}

void
machine_standby(void)
{
}

/*
 * Initial entry point on startup. This gets called before main() is
 * entered.
 * It should be responsible for setting up everything that must be
 * in place when main is called.
 * This includes:
 *   Taking a copy of the boot configuration structure.
 */
u_int
initarm(int argc, char **argv, struct bootinfo *bi)
{

	__sleep_func = NULL;
	__sleep_ctx = NULL;

	/* parse kernel args */
	booted_kernel = booted_kernel_storage;
	boothowto = 0;
	boot_file[0] = '\0';
	if (argc > 0 && argv != NULL) {
		strncpy(booted_kernel_storage, argv[0],
		    sizeof(booted_kernel_storage));
		for (int i = 1; i < argc; i++) {
			char *cp = argv[i];

			switch (*cp) {
			case 'b':
				/* boot device: -b=sd0 etc. */
				cp = cp + 2;
				if (strcmp(cp, MOUNT_NFS) == 0)
					rootfstype = MOUNT_NFS;
				else
					strncpy(boot_file, cp,
					    sizeof(boot_file));
				break;

			default:
				BOOT_FLAG(*cp, boothowto);
				break;
			}
		}
	}

	/* copy bootinfo into known kernel space */
	if (bi != NULL)
		bootinfo_storage = *bi;
	bootinfo = &bootinfo_storage;

#ifdef BOOTINFO_FB_WIDTH
	bootinfo->fb_line_bytes = BOOTINFO_FB_LINE_BYTES;
	bootinfo->fb_width = BOOTINFO_FB_WIDTH;
	bootinfo->fb_height = BOOTINFO_FB_HEIGHT;
	bootinfo->fb_type = BOOTINFO_FB_TYPE;
#endif

	if (bootinfo->magic == BOOTINFO_MAGIC) {
		platid.dw.dw0 = bootinfo->platid_cpu;
		platid.dw.dw1 = bootinfo->platid_machine;

#ifndef RTC_OFFSET
		/*
		 * rtc_offset from bootinfo.timezone set by hpcboot.exe
		 */
		if (rtc_offset == 0 &&
		    (bootinfo->timezone > (-12 * 60) &&
		     bootinfo->timezone <= (12 * 60)))
			rtc_offset = bootinfo->timezone;
#endif
	}

#ifdef KLOADER
	/* copy boot parameter for kloader */
	kloader_bootinfo_set(&kbootinfo, argc, argv, bi, false);
#endif

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions.
	 */
	set_cpufuncs();
	IRQdisable;

#if defined(CPU_SA1100) || defined(CPU_SA1110)
	return init_sa11x0(argc, argv, bi);
#elif defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
	return init_pxa2x0(argc, argv, bi);
#else
#error	No CPU support
#endif
}

#ifdef BOOT_DUMP
static void
dumppages(char *start, int nbytes)
{
	char *p = start;
	char *p1;
	int i;

	for (i = nbytes; i > 0; i -= 16, p += 16) {
		for (p1 = p + 15; p != p1; p1--) {
			if (*p1)
				break;
		}
		if (!*p1)
			continue;
		printf("%08x %02x %02x %02x %02x %02x %02x %02x %02x"
		    " %02x %02x %02x %02x %02x %02x %02x %02x\n",
		    (unsigned int)p,
		    p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
		    p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
	}
}
#endif
