/*	$NetBSD: boot.c,v 1.11 2005/12/24 20:07:31 perry Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>
#include <sys/reboot.h>
#include <sys/boot_flag.h>
#include <machine/bootinfo.h>
#include <machine/cpu.h>
#include <machine/residual.h>
#include <powerpc/spr.h>

#include "boot.h"

char *names[] = {
	"in()",
#if 0
	"fd(0,0,0)netbsd", "fd(0,0,0)netbsd.gz",
	"fd(0,0,0)netbsd.old", "fd(0,0,0)netbsd.old.gz",
	"fd(0,0,0)onetbsd", "fd(0,0,0)onetbsd.gz"
#endif
};
#define	NUMNAMES (sizeof (names) / sizeof (names[0]))

#define	NAMELEN	128
char namebuf[NAMELEN];
char nametmp[NAMELEN];

char bootinfo[BOOTINFO_MAXSIZE];
struct btinfo_residual btinfo_residual;
struct btinfo_console btinfo_console;
struct btinfo_clock btinfo_clock;

RESIDUAL residual;

extern u_long ns_per_tick;
extern char bootprog_name[], bootprog_rev[], bootprog_maker[], bootprog_date[];

void boot __P((void *, u_long));
static void exec_kernel __P((char *));

void
boot(resp, loadaddr)
	void *resp;
	u_long loadaddr;
{
	extern char _end[], _edata[];
	int n = 0;
	int addr, speed;
	unsigned int cpuvers;
	char *name, *cnname, *p;

	/* Clear all of BSS */
	memset(_edata, 0, _end - _edata);

	/*
	 * console init
	 */
	cnname = cninit(&addr, &speed);

	/* make bootinfo */
	/*
	 * residual data
	 */
	btinfo_residual.common.next = sizeof(btinfo_residual);
	btinfo_residual.common.type = BTINFO_RESIDUAL;
	if (resp) {
		memcpy(&residual, resp, sizeof(residual));
		btinfo_residual.addr = (void *)&residual;
	} else {
		printf("Warning: no residual data.\n");
		btinfo_residual.addr = 0;
	}

	/*
	 * console
	 */
	btinfo_console.common.next = sizeof(btinfo_console);
	btinfo_console.common.type = BTINFO_CONSOLE;
	strcpy(btinfo_console.devname, cnname);
	btinfo_console.addr = addr;
	btinfo_console.speed = speed;

	/*
	 * clock
	 */
	__asm volatile ("mfpvr %0" : "=r"(cpuvers));
	cpuvers >>= 16;
	btinfo_clock.common.next = 0;
	btinfo_clock.common.type = BTINFO_CLOCK;
	if (cpuvers == MPC601) {
		btinfo_clock.ticks_per_sec = 1000000000;
	} else {
		btinfo_clock.ticks_per_sec = resp ?
		    residual.VitalProductData.ProcessorBusHz/4 : TICKS_PER_SEC;
	}
	ns_per_tick = 1000000000 / btinfo_clock.ticks_per_sec;

	p = bootinfo;
        memcpy(p, (void *)&btinfo_residual, sizeof(btinfo_residual));
        p += sizeof(btinfo_residual);
        memcpy(p, (void *)&btinfo_console, sizeof(btinfo_console));
        p += sizeof(btinfo_console);
        memcpy(p, (void *)&btinfo_clock, sizeof(btinfo_clock));

	/*
	 * load kernel if attached
	 */
	init_in(loadaddr);

	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	for (;;) {
		name = names[n++];
		if (n >= NUMNAMES)
			n = 0;

		exec_kernel(name);
	}
}

/*
 * Exec kernel
 */
static void
exec_kernel(name)
	char *name;
{
	int howto = 0;
	char c, *ptr;
	u_long marks[MARK_MAX];
#ifdef DBMONITOR
	int go_monitor;
	extern int db_monitor __P((void));
#endif /* DBMONITOR */

ret:
	printf("\nBoot: ");
	memset(namebuf, 0, sizeof (namebuf));
	(void)tgets(namebuf);

	ptr = namebuf;
#ifdef DBMONITOR
	go_monitor = 0;
	if (*ptr == '!') {
		if (*(++ptr) == NULL) {
			db_monitor();
			printf("\n");
			goto ret;
		} else {
			go_monitor++;
		}
	}
#endif /* DBMONITOR */
	while ((c = *ptr)) {
		while (c == ' ')
			c = *++ptr;
		if (!c)
			goto next;
		if (c == '-') {
			while ((c = *++ptr) && c != ' ')
				BOOT_FLAG(c, howto);
		} else {
			name = ptr;
			while ((c = *++ptr) && c != ' ');
			if (c)
				*ptr++ = 0;
		}
	}

next:
	printf("Loading %s", name);
	if (howto)
		printf(" (howto 0x%x)", howto);
	printf("\n");

	marks[MARK_START] = 0;
	if (loadfile(name, marks, LOAD_ALL) == 0) {
#ifdef DBMONITOR
		if (go_monitor) {
			db_monitor();
			printf("\n");
		}
#endif /* DBMONITOR */

		printf("start=0x%lx\n\n", marks[MARK_ENTRY]);
		delay(1000);
		__syncicache((void *)marks[MARK_ENTRY],
			(u_int)marks[MARK_SYM] - (u_int)marks[MARK_ENTRY]);

		run((void *)marks[MARK_SYM],
		    (void *)marks[MARK_END],
		    (void *)howto,
		    (void *)bootinfo,
		    (void *)marks[MARK_ENTRY]);
	}
}
