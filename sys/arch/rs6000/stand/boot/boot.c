/*	$NetBSD: boot.c,v 1.2 2007/12/17 19:54:32 garbled Exp $	*/

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
#include <machine/iplcb.h>
#include <powerpc/spr.h>

#include "boot.h"

char *names[] = {
	"in()",
};
#define	NUMNAMES (sizeof (names) / sizeof (names[0]))

#define	NAMELEN	128
char namebuf[NAMELEN];
char nametmp[NAMELEN];

unsigned char cregs[10];

char bootinfo[BOOTINFO_MAXSIZE];
struct btinfo_iplcb btinfo_iplcb;
struct btinfo_console btinfo_console;

/*struct ipl_cb iplcb;
struct ipl_directory ipldir;*/

extern u_long ns_per_tick;
extern char bootprog_name[], bootprog_rev[], bootprog_maker[], bootprog_date[];

void boot(void *, void *);
static void exec_kernel(char *);

#define	PSL_DR	(1<<4)
#define PSL_EE	0x00008000
#define BAT_BL_128K	0x00000000
#define BAT_BL_1M	0x0000001c
#define BAT_BL_2M	0x0000003c
#define BAT_W		0x00000040
#define BAT_I		0x00000020
#define BAT_M		0x00000010
#define BAT_G		0x00000008
#define BAT_X		0x00000004
#define BAT_Vs		0x00000002
#define BAT_Vu		0x00000001
#define BAT_PP_RW	0x00000002
#define BAT_RPN		(~0x1ffff)
#define BAT_EPI		(~0x1ffffL)
#define BAT_V		(BAT_Vs|BAT_Vu)
#define BAT_BL		0x00001ffc

#define BATU(va, len, v)	\
	(((va) & BAT_EPI) | ((len) & BAT_BL) | ((v) & BAT_V))
#define BATL(pa, wimg, pp)	\
	(((pa) & BAT_RPN) | (wimg) | (pp))


static void
setled(uint32_t val)
{
#ifdef POWER
	register_t savemsr, msr, savesr15;

	__asm volatile ("mfmsr %0" : "=r"(savemsr));
	msr = savemsr & ~PSL_DR;
	__asm volatile ("mtmsr %0" : : "r"(msr));

	__asm volatile ("mfsr %0,15;isync" : "=r"(savesr15));
	__asm volatile ("mtsr 15,%0" : : "r"(0x82040080));
	__asm volatile ("mtmsr %0" : : "r"(msr|PSL_DR));
	__asm volatile ("isync");
	*(uint32_t *)0xF0A00300 = val;
	__asm volatile ("mtmsr %0" : : "r"(savemsr));
	__asm volatile ("mtsr 15,%0;isync" : : "r"(savesr15));
#else
#ifdef NOTYET
	register_t savemsr, msr, batu, batl;

	/* turn on DR */
	__asm volatile ("mfmsr %0" : "=r"(savemsr));
	msr = savemsr|PSL_DR;
	__asm volatile ("mtmsr %0" : : "r"(msr));
	__asm volatile ("isync");

	/* set up a bat and map the whole NVRAM chunk */
	batl = BATL(0xFF600000, BAT_I|BAT_G, BAT_PP_RW);
	batu = BATU(0xFF600000, BAT_BL_1M, BAT_Vs);
	__asm volatile ("mtdbatl 1,%0; mtdbatu 1,%1;"
	    ::  "r"(batl), "r"(batu));
	__asm volatile ("isync");

	*(volatile uint32_t *)0xFF600300 = val;
	__asm volatile ("eieio");
	__asm volatile ("isync");

	/* put back to normal */
	__asm volatile ("mtmsr %0" : : "r"(savemsr));
	__asm volatile ("isync");
#endif /* NOTYET */
	
#endif
}


void
boot(void *iplcb_p, void *extiplcb_p)
{
	extern char _end[], _edata[];
	int n = 0;
	int addr, speed;
	/*unsigned int cpuvers;*/
	char *name, *cnname, *p;
	struct ipl_cb *iplcb_ptr;
	struct ipl_directory *dirp;
	struct ipl_info *infop;

	//setled(0x30100000); /* we have control */
	//for (;;);
	iplcb_ptr = (struct ipl_cb *)iplcb_p;
	dirp = &(iplcb_ptr->dir);

	/* Clear all of BSS */
	memset(_edata, 0, _end - _edata);

	/*
	 * console init
	 */
	//setled(0x30000000); /* attempting r14 setup */
	setup_iocc();
	//setled(0x31000000); /* attempting console init */
	cnname = cninit(&addr, &speed);
	printf("\n");
	//setled(0x31100000); /* we have the console */

	printf("IPLCB ptr = 0x%p\n", iplcb_p);

	infop = (struct ipl_info *)((char *)iplcb_p + dirp->iplinfo_off);
	printf("Machine model = 0x%x\n", infop->model);
	printf("RAM = 0x%x\n", infop->ram_size);

	//dump_iplcb(iplcb_p);

	/* make bootinfo */
	/*
	 * ipl control block
	 */
	btinfo_iplcb.common.next = sizeof(btinfo_iplcb);
	btinfo_iplcb.common.type = BTINFO_IPLCB;
	if (iplcb_ptr) {
		btinfo_iplcb.addr = (void *)iplcb_p;
	} else {
		printf("Warning: no IPL Control Block.\n");
		btinfo_iplcb.addr = 0;
	}

	/*
	 * console
	 */
	btinfo_console.common.next = sizeof(btinfo_console);
	btinfo_console.common.type = BTINFO_CONSOLE;
	strcpy(btinfo_console.devname, cnname);
	btinfo_console.addr = addr;
	btinfo_console.speed = speed;

	p = bootinfo;
        memcpy(p, (void *)&btinfo_iplcb, sizeof(btinfo_iplcb));
        p += sizeof(btinfo_iplcb);
        memcpy(p, (void *)&btinfo_console, sizeof(btinfo_console));
        p += sizeof(btinfo_console);

	/*
	 * load kernel if attached
	 */
	init_in(RELOC);

	setled(0x38000000); /* attempting boot */
	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	for (;;) {
		name = names[n++];
		if (n >= NUMNAMES)
			n = 0;
		setled(0x38100000 + (0x100000 * n));
		exec_kernel(name);
		setled(0x39900000); /* boot failed! */
	}
}

/*
 * Exec kernel
 */
static void
exec_kernel(char *name)
{
	int howto = 0;
	char c, *ptr;
	u_long marks[MARK_MAX];
#ifdef DBMONITOR
	int go_monitor;
	extern int db_monitor __P((void));

ret:
#endif /* DBMONITOR */
	printf("\nBoot: ");
	memset(namebuf, 0, sizeof (namebuf));
	if (tgets(namebuf) == -1)
		printf("\n");

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
		printf("About to run\n");
		run((void *)marks[MARK_SYM],
		    (void *)marks[MARK_END],
		    (void *)howto,
		    (void *)bootinfo,
		    (void *)marks[MARK_ENTRY]);
	}
}
