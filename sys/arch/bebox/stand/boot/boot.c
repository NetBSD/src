/*	$NetBSD: boot.c,v 1.9 1999/06/28 01:20:43 sakamoto Exp $	*/

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
#include <stand.h>
#include <loadfile.h>
#include <sys/reboot.h>
#include <machine/bootinfo.h>
#include "boot.h"

char *names[] = {
	"in()",
	"fd(0,1,0)netbsd", "fd(0,1,0)netbsd.gz",
	"fd(0,1,0)netbsd.old", "fd(0,1,0)netbsd.old.gz",
	"fd(0,1,0)onetbsd", "fd(0,1,0)onetbsd.gz"
};
#define	NUMNAMES (sizeof (names) / sizeof (names[0]))

#define	NAMELEN	128
char namebuf[NAMELEN];
char nametmp[NAMELEN];

struct btinfo_memory btinfo_memory;
struct btinfo_console btinfo_console;
struct btinfo_clock btinfo_clock;

extern char bootprog_name[], bootprog_rev[], bootprog_maker[], bootprog_date[];

void exec_kernel __P((char *, void *));

void
main()
{
	int n = 0;
	int addr, speed;
	char *name, *cnname;
	void *p, *bootinfo;

	if (whichCPU() == 1)
		cpu1();
	resetCPU1();

	/*
	 * console init
	 */
	cnname = cninit(&addr, &speed);

	/*
	 * make bootinfo
	 */
	bootinfo = (void *)0x3030;

	/*
	 * memory
	 */
	btinfo_memory.common.next = sizeof (btinfo_memory);
	btinfo_memory.common.type = BTINFO_MEMORY;
	btinfo_memory.memsize = *(int *)0x3010;

	/*
	 * console
	 */
	btinfo_console.common.next = sizeof (btinfo_console);
	btinfo_console.common.type = BTINFO_CONSOLE;
	strcpy(btinfo_console.devname, cnname);
	btinfo_console.addr = addr;
	btinfo_console.speed = speed;

	/*
	 * clock
	 */
	btinfo_clock.common.next = 0;
	btinfo_clock.common.type = BTINFO_CLOCK;
	btinfo_clock.ticks_per_sec = TICKS_PER_SEC;

	p = bootinfo;
	memcpy(p, (void *)&btinfo_memory, sizeof (btinfo_memory));
	p += sizeof (btinfo_memory);
	memcpy(p, (void *)&btinfo_console, sizeof (btinfo_console));
	p += sizeof (btinfo_console);
	memcpy(p, (void *)&btinfo_clock, sizeof (btinfo_clock));

	/*
	 * attached kernel check
	 */
	init_in();

	runCPU1((void *)start_CPU1);
	wait_for(&CPU1_alive);

	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);
	printf(">> Memory: %d k\n", btinfo_memory.memsize / 1024);

	for (;;) {
		name = names[n++];
		if (n >= NUMNAMES)
			n = 0;

		exec_kernel(name, bootinfo);
	}
}

/*
 * Exec kernel
 */
void
exec_kernel(name, bootinfo)
	char *name;
	void *bootinfo;
{
	int howto = 0;
	char c, *ptr;
	u_long marks[MARK_MAX];
#ifdef DBMONITOR
	int go_monitor;
	extern int db_monitor __P((void));
#endif /* DBMONITOR */
	extern int tgets __P((char *buf));

ret:
	do {
		printf("\nBoot: ");

		memset(namebuf, 0, sizeof (namebuf));
	} while (tgets(namebuf) == -1);

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
			while ((c = *++ptr) && c != ' ') {
				if (c == 'a')
					howto |= RB_ASKNAME;
				else if (c == 'b')
					howto |= RB_HALT;
				else if (c == 'd')
					howto |= RB_KDB;
				else if (c == 'r')
					howto |= RB_DFLTROOT;
				else if (c == 's')
					howto |= RB_SINGLE;
			}
		} else {
			name = ptr;
			while ((c = *++ptr) && c != ' ');
			if (c)
				*ptr++ = 0;
		}
	}

next:
	printf("Loading %s\n", name);

	marks[MARK_START] = 0;
	if (loadfile(name, marks, LOAD_ALL) == 0) {
#ifdef DBMONITOR
		if (go_monitor) {
			db_monitor();
			printf("\n");
		}
#endif /* DBMONITOR */

		printf("start=0x%x\n\n", marks[MARK_ENTRY]);
		delay(1000);
		__syncicache((void *)marks[MARK_ENTRY],
			(u_int)marks[MARK_SYM] - (u_int)marks[MARK_ENTRY]);

		*(volatile u_long *)0x0080 = marks[MARK_ENTRY];
		run((void *)marks[MARK_SYM],
		    (void *)marks[MARK_END],
		    (void *)howto,
		    bootinfo,
		    (void *)marks[MARK_ENTRY]);
	}
}
