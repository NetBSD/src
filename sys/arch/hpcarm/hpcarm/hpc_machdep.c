/*	$NetBSD: hpc_machdep.c,v 1.99 2010/11/14 03:17:50 uebayasi Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: hpc_machdep.c,v 1.99 2010/11/14 03:17:50 uebayasi Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/pmf.h>

#include <uvm/uvm.h>

#include <arm/cpufunc.h>

#include <machine/bootconfig.h>
#include <machine/bootinfo.h>
#include <machine/pmap.h>

#include <dev/cons.h>
#include <dev/hpc/apm/apmvar.h>

BootConfig bootconfig;		/* Boot config storage */
struct bootinfo *bootinfo, bootinfo_storage;
char booted_kernel_storage[80];
char *booted_kernel = booted_kernel_storage;

paddr_t physical_start;
paddr_t physical_freestart;
paddr_t physical_freeend;
paddr_t physical_end;

#ifndef PMAP_STATIC_L1S
int max_processes = 64;			/* Default number */
#endif /* !PMAP_STATIC_L1S */

/* Physical and virtual addresses for some global pages */
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
pv_addr_t kernelstack;

char *boot_args = NULL;
char boot_file[16];

vaddr_t msgbufphys;

/* Prototypes */
void dumpsys(void);

/* Mode dependent sleep function holder */
void (*__sleep_func)(void *);
void *__sleep_ctx;

void (*__cpu_reset)(void) = cpu_reset;

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
