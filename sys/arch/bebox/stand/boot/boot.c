/*	$NetBSD: boot.c,v 1.8 1999/06/24 01:10:31 sakamoto Exp $	*/

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

#include <sys/exec_elf.h>
#include <sys/reboot.h>
#include <machine/cpu.h>
#include <machine/param.h>
#include <bebox/include/bootinfo.h>
#include "boot.h"

char *name;
char *names[] = {
	"in()",
	"fd(0,1,0)netbsd", "fd(0,1,0)netbsd.gz",
	"fd(0,1,0)netbsd.old", "fd(0,1,0)netbsd.old.gz",
	"fd(0,1,0)onetbsd", "fd(0,1,0)onetbsd.gz"
};
#define NUMNAMES (sizeof (names) / sizeof (names[0]))

#define NAMELEN	128
char namebuf[NAMELEN];
char nametmp[NAMELEN];
int args;
void *startsym, *endsym, *bootinfo;
struct btinfo_memory btinfo_memory;
struct btinfo_console btinfo_console;
struct btinfo_clock btinfo_clock;
extern int errno;

void
main()
{
	int fd, n = 0;
	int addr, speed;
	char *cnname;
	void *p;
	void start_CPU1();
	extern int CPU1_alive;
	extern char bootprog_name[], bootprog_rev[],
		bootprog_maker[], bootprog_date[];
	extern char *cninit();

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
	bcopy((void *)&btinfo_memory, p, sizeof (btinfo_memory));
	p += sizeof (btinfo_memory);
	bcopy((void *)&btinfo_console, p, sizeof (btinfo_console));
	p += sizeof (btinfo_console);
	bcopy((void *)&btinfo_clock, p, sizeof (btinfo_clock));

	/*
	 * attached kernel check
	 */
	init_in();

	runCPU1(start_CPU1);
	wait_for(&CPU1_alive);

	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);
	printf(">> Memory: %d k\n", btinfo_memory.memsize / 1024);

	for (;;) {
		args = 0;
		name = names[n++];
		if (n >= NUMNAMES)
			n = 0;

		getbootdev(&args);
		fd = open(name, 0);
		if (fd >= 0) {
			exec_kernel(fd, &args);
			close(fd);	/* exec failed */
		} else {
			printf("open error:%s\n",  strerror(errno));
		}
	}
}

/*
 * Get boot device, file name, flag
 */
getbootdev(howto)
	int *howto;
{
	char c, *ptr;
#ifdef DBMONITOR
	extern int load_flag;
#endif /* DBMONITOR */

#ifdef DBMONITOR
ret:
	if (load_flag)
		printf("Load: ");
	else
#endif /* DBMONITOR */
		printf("Boot: ");

	bzero(namebuf, sizeof (namebuf));
	if (tgets(namebuf) != -1) {
		ptr = namebuf;
#ifdef DBMONITOR
		if (*ptr == '!') {
			db_monitor();
			printf("\n");
			goto ret;
		}
#endif /* DBMONITOR */
		while (c = *ptr) {
			while (c == ' ')
				c = *++ptr;
			if (!c)
				return;
			if (c == '-')
				while ((c = *++ptr) && c != ' ') {
					if (c == 'a')
						*howto |= RB_ASKNAME;
					else if (c == 'b')
						*howto |= RB_HALT;
					else if (c == 'd')
						*howto |= RB_KDB;
					else if (c == 'r')
						*howto |= RB_DFLTROOT;
					else if (c == 's')
						*howto |= RB_SINGLE;
				}
			else {
				name = ptr;
				while ((c = *++ptr) && c != ' ');
				if (c)
					*ptr++ = 0;
			}
		}
	} else {
		putchar('\n');
	}
}

exec_kernel(fd, howto)
	int fd;
	int *howto;
{
	int i, first = 1;
	Elf32_Ehdr hdr;
	Elf32_Phdr phdr;
	Elf32_Shdr shdr;
	void *sp, *ssp;
#ifdef DBMONITOR
	extern int load_flag;
#endif /* DBMONITOR */

#ifdef DBMONITOR
	printf("%s %s ", (load_flag ? "Loading" : "Booting"), name);
#else /* DBMONITOR */
	printf("Booting %s ", name);
#endif /* DBMONITOR */

	if (read(fd, &hdr, ELF32_HDR_SIZE) != ELF32_HDR_SIZE) {
		printf("\nread error: %s\n", strerror(errno));
		return;
	}
	if (bcmp(hdr.e_ident, Elf32_e_ident, Elf32_e_siz)) {
		printf("\nnot ELF32 format\n");
		return;
	}
	if (hdr.e_machine != Elf_em_ppc) {
		printf("\nnot PowerPC exec binary\n");
		return;
	}
#if 0
	printf("e_entry= 0x%x\n", hdr.e_entry);
	printf("e_phoff= 0x%x\n", hdr.e_phoff);
	printf("e_shoff= 0x%x\n", hdr.e_shoff);
	printf("e_flags= 0x%x\n", hdr.e_flags);
	printf("e_ehsize= 0x%x\n", hdr.e_ehsize);
	printf("e_phentsize= 0x%x\n", hdr.e_phentsize);
	printf("e_phnum= 0x%x\n", hdr.e_phnum);
	printf("e_shentsize= 0x%x\n", hdr.e_shentsize);
	printf("e_shnum= 0x%x\n", hdr.e_shnum);
	printf("e_shstrndx= 0x%x\n", hdr.e_shstrndx);
#endif
	printf("@ 0x%x\n", hdr.e_entry);

	for (i = 0; i < hdr.e_phnum; i++) {
		lseek(fd, hdr.e_phoff + sizeof(phdr) * i, SEEK_SET);
		if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr)) {
			printf("\nread phdr error: %s\n", strerror(errno));
			return;
		}
		if (phdr.p_type != Elf_pt_load ||
		    (phdr.p_flags & (Elf_pf_w|Elf_pf_x)) == 0)
			continue;

		/* Read in segment. */
		printf("%s%d", first ? "" : "+", phdr.p_filesz);
		lseek(fd, phdr.p_offset, SEEK_SET);
		if (read(fd, (void *)phdr.p_vaddr, phdr.p_filesz)
		    != phdr.p_filesz) {
			printf("read text error: %s\n", strerror(errno));
			return;
		}
		__syncicache((void *)phdr.p_vaddr, phdr.p_filesz);

		/* Zero out bss. */
		if (phdr.p_filesz < phdr.p_memsz) {
			printf("+%d", phdr.p_memsz - phdr.p_filesz);
			bzero(phdr.p_vaddr + phdr.p_filesz,
			    phdr.p_memsz - phdr.p_filesz);
			sp = (void *)(phdr.p_vaddr + phdr.p_memsz);
			startsym = sp = (void *)ALIGN(sp);
		}
		first = 0;
	}

	bcopy(&hdr, startsym, ELF32_HDR_SIZE);
	((Elf32_Ehdr *)startsym)->e_shoff = ELF32_HDR_SIZE;
	sp += ELF32_HDR_SIZE;
	ssp = sp + sizeof(shdr) * hdr.e_shnum;
	first = 1;
	printf("+[");

	for (i = 0; i < hdr.e_shnum; i++) {
		lseek(fd, hdr.e_shoff + sizeof(shdr) * i, SEEK_SET);
		if (read(fd, &shdr, sizeof(shdr)) != sizeof(shdr)) {
			printf("\nread shdr error: %s\n", strerror(errno));
			return;
		}
		bcopy(&shdr, sp, sizeof(shdr));
		((Elf32_Shdr *)sp)->sh_offset = ssp - startsym;
		sp += sizeof(shdr);

		if (shdr.sh_type != Elf_sht_strtab &&
		    shdr.sh_type != Elf_sht_symtab)
			continue;

		/* Read in segment. */
		printf("%s%d", first ? "" : "+", shdr.sh_size);
		lseek(fd, shdr.sh_offset, SEEK_SET);
		if (read(fd, ssp, shdr.sh_size) != shdr.sh_size) {
			printf("read symbol error: %s\n", strerror(errno));
			return;
		}
		ssp += shdr.sh_size;
		ssp = (void *)ALIGN(ssp);
		first = 0;
	}
	endsym = ssp;
	printf("]\n");
	close(fd);

#ifdef DBMONITOR
	if (!load_flag) {
		*(volatile long *)0x0080 = (long)hdr.e_entry;
		run((void *)hdr.e_entry);
	}
#else /* DBMONITOR */
	*(volatile long *)0x0080 = (long)hdr.e_entry;
	run((void *)hdr.e_entry);
#endif /* DBMONITOR */
}
