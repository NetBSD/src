/* $NetBSD: xenload.c,v 1.1 2004/04/17 23:20:37 cl Exp $ */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
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
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xenload.c,v 1.1 2004/04/17 23:20:37 cl Exp $");

#include <sys/lock.h>

#include <lib/libsa/stand.h>

#include <machine/vmparam.h>

#include "xenload.h"

void prepare(void);

#define	__data __attribute__ ((section (".data")))

start_info_t __data *start_info;

static int
HYPERVISOR_console_write_pic(const char *str, int count) {
	int ret;

	__asm__ __volatile__ (
		"	movl	%2,%%ebx	;"
		TRAP_INSTR
		: "=a" (ret) : "0" (__HYPERVISOR_console_write), 
		"r" (str), "c" (count)
		: "ebx");

	return ret;
}

static int
HYPERVISOR_exit_pic(void) {
	int ret;

	__asm__ __volatile__ (
		"	movl	%2,%%ebx	;"
		TRAP_INSTR
		: "=a" (ret) : "0" (__HYPERVISOR_sched_op),
		"r" (SCHEDOP_exit) );

	return ret;
}

#define	MAXLINELEN	256

void
putchar(int c)
{
	static char __data linebuf[MAXLINELEN];
	static int __data linepos = 0;

	linebuf[linepos++] = c;
	if (c == 0 || c == '\n' || linepos == MAXLINELEN - 1) {
		linebuf[linepos] = 0;
		HYPERVISOR_console_write_pic(linebuf, linepos);
		linepos = 0;
	}
}

void
prepare(void)
{
	extern void *text_start;
	vaddr_t loadaddr, symaddr;
	unsigned long symlen;
	vaddr_t modaddr, relocaddr;

	modaddr = start_info->mod_start;

	if (modaddr == 0) {
		printf("xen12load failed: NetBSD kernel image missing\n");
		HYPERVISOR_exit_pic();
	}

	if (prepareelfimage(modaddr, &loadaddr, &symaddr, &symlen) != 0) {
		printf("xen12load failed: prepare elf image failed\n");
		HYPERVISOR_exit_pic();
	}

	if (loadaddr != (vaddr_t)&text_start) {
		printf("xen12load failed: load address 0x%x doesn't match "
		    "kernel load address 0x%x\n", &text_start, loadaddr);
		HYPERVISOR_exit_pic();
	}

	relocaddr = (symaddr + symlen + modaddr - loadaddr + PAGE_SIZE - 1) &
		~(PAGE_SIZE - 1);
	printf("NetBSD kernel image prepared at %p for %p.\n",
	    modaddr, loadaddr, symaddr);
#ifdef DEBUG
	printf("kernel symbols at %p, size 0x%x.\n", symaddr, symlen);
#endif
	printf("Using relocation loader at %p.\n", relocaddr);

	start_info->mod_start = symlen ? symaddr : 0;
	start_info->mod_len = symlen;
	do_reloc(relocaddr, modaddr);

	printf("xen12load failed: do_reloc returned\n");
	HYPERVISOR_exit_pic();
}
