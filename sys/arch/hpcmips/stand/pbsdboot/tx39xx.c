/* $NetBSD: tx39xx.c,v 1.5 2000/01/16 23:30:13 uch Exp $ */

/*-
 * Copyright (c) 1999 Shin Takemura, UCHIYAMA Yasushi
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 */

#include <pbsdboot.h>

extern void tx39xx_asm_code();
extern void tx39xx_asm_code_end();
void tx39xx_asm_code_holder __P((void));

#define TX39_SYSADDR_CONFIG_REG		0x10C00000
#define TX39_SYSADDR_CONFIG_REG_LEN	0x00200000
typedef int tx_chipset_tag_t;
u_int32_t __tx39conf_addr;

u_int32_t
tx_conf_read(t, reg)
	tx_chipset_tag_t t;
	int reg;
{
	return *((u_int32_t*)(__tx39conf_addr + reg));
}

void
tx_conf_write(t, reg, val)
	tx_chipset_tag_t t;
	int reg;
	u_int32_t val;
{
	u_int32_t addr = (u_int32_t)t;
	*((u_int32_t*)(__tx39conf_addr +reg)) = val;
}

void
tx39xx_init(SYSTEM_INFO *info)
{
	/* 4KByte page */
	system_info.si_pagesize = info->dwPageSize;
	/* DRAM Bank 0/1 physical addr range */
	system_info.si_dramstart = 0x04000000;
	system_info.si_drammaxsize = 0x04000000;
	/* Pointer for bootstrap code */
	system_info.si_asmcode = (unsigned char*)tx39xx_asm_code;
	system_info.si_asmcodelen = (unsigned char*)tx39xx_asm_code_end
		- system_info.si_asmcode;
	system_info.si_boot = mips_boot;
	system_info.si_intrvec = 0x80;

	__tx39conf_addr = (int)VirtualAlloc(0, TX39_SYSADDR_CONFIG_REG_LEN, MEM_RESERVE, 
			    PAGE_NOACCESS);
	if (!VirtualCopy((LPVOID)__tx39conf_addr, 
			 (LPVOID)(TX39_SYSADDR_CONFIG_REG >> 8), 
			 TX39_SYSADDR_CONFIG_REG_LEN,
			 PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL)) {
		msg_printf(MSG_ERROR, whoami, 
			   TEXT("Mapping TX39 configuration register failed.\n"));
	}
}

void
tx39xx_asm_code_holder()
{
	/*
	 * void
	 * startprog(register struct map_s *map)
	 * {
	 *   register unsigned char *addr;
	 *   register unsigned char *p;
	 *   register int i;
	 * 
	 *   addr = map->base;
	 *   i = 0;
	 *   while (p = map->leaf[i / map->leafsize][i % map->leafsize]) {
	 *     register unsigned char *pe = p + map->pagesize;
	 *     while (p < pe) {
	 *       *addr++ = *p++;
	 *     }
	 *     i++;
	 *   }
	 * }
	 *
	 *  register assignment:
	 *    struct map_s *map		a0
	 *    unsigned char *addr	a1
	 *    unsigned char *p		a2
	 *    unsigned char *pe		a3
	 *    int i			t0
	 *
	 * struct map_s {
	 *   caddr_t entry;	+0
	 *   caddr_t base;	+4
	 *   int pagesize;	+8
	 *   int leafsize;	+12
	 *   int nleaves;	+16
	 *   caddr_t arg0;	+20
	 *   caddr_t arg1;  +24
	 *   caddr_t arg2;	+28
	 *   caddr_t arg3;	+32
	 *   caddr_t *leaf[32];	+36
	 *
	 */
	__asm(
		".set noreorder;"
		".globl tx39xx_asm_code;"
		"tx39xx_asm_code:"
		"lui	a0, 0x0000;"
		"ori	a0, 0x0000;"

		/* Disable interrupt */
		"nop;"
		"mtc0	zero, $12;"
		"nop;"

		/*
		 * Copy kernel to bootaddr
		 */
		/* addr = map->base;	*/
		"lw	a1, 4(a0);"

		/*   i = 0;		*/
		"ori	t0, zero, 0;"

	" loop_start:"

		/*   while (p = map->leaf[i / map->leafsize][i % map->leafsize]) { */
		/*   t1 = map->leafsize */
		"lw	t1, 12(a0);"

		/*   lo = i / map->leafsize, hi = i % map->leafsize */
		"addu	t3, zero, t0;"
		"div	t3, t1;"
		/*   t2 = map->leaf */
		"addiu	t2, a0, 36;"
		/*   t3 = i / map->leafsize */
		"nop;"
		"mflo	t3;"
		/*   t2 = map->leaf[i / map->leafsize] */
		"sll	t3, t3, 2;"
		"addu	t2, t2, t3;"
		"lw	t2, 0(t2);"
		/*   t3 = i % map->leafsize */
		"mfhi	t3;"

		/*   p = map->leaf[i / map->leafsize][i % map->leafsize] */
		"sll	t3, t3, 2;"
		"addu	t2, t2, t3;"
		"lw	a2, 0(t2);"

		/*		if (p == NULL) {	*/
		/*			break;			*/
		/*		}					*/
		"beq	a2, zero, loop_end;"
		"nop;"

		/*     register unsigned char *pe = p + map->pagesize; */
		"lw	t1, 8(a0);"
		"add	a3, a2, t1;"

		/*		while (p < pe) {		*/
	"loop_start2:"
		"sltu        t1, a2, a3;"
		"beq         zero,t1,loop_end2;"
		"nop;"

		/*			*addr++ = *p++;	*/
		"lw	t1, 0(a2);"
		"sw	t1, 0(a1);"
		"addi	a2, a2, 4;"
		"addi	a1, a1, 4;"

		/*		}	*/
		"beq	zero, zero, loop_start2;"
		"nop;"

		/*     i++;	*/
	"loop_end2:"
		"addi	t0, t0, 1;"
		"beq	zero, zero, loop_start;"
		"nop;"

	"loop_end:"
		"move t3, a0;"
	);

	/* 
	 * Flush cache
	 */
	__asm(
		"li	t1, 16384;"
		"li	t2, 8192;"

		/* Disable I-cache */
		"li	t5, ~0x00000020;"
		"mfc0	t6, $3;"
		"and	t5, t5, t6;"
		"nop;"
		"mtc0	t5, $3;"
		/* Stop streaming */
		"beq	zero, zero, 1f;"
		"nop;"
	"1:"
		/* Flush I-cache */
		"li	t0, 0x80000000;"
		"addu	t1, t0, t1;"
		"subu	t1, t1, 128;"
	"2:"
		".word 0xbd000000;"
		".word 0xbd000010;"
		".word 0xbd000020;"
		".word 0xbd000030;"
		".word 0xbd000040;"
		".word 0xbd000050;"
		".word 0xbd000060;"
		".word 0xbd000070;"
		"bne	t0, t1, 2b;"
		"addu	t0, t0, 128;"

		/* Flush D-cache */
		"li	t0, 0x80000000;"
		"addu	t1, t0, t2;"

	"3:"
		"lw	t2, 0(t0);"
		"bne	t1, t0, 3b;"
		"addiu	t0, t0, 4;"

		/* Enable I-cache */
		"nop;"
		"mtc0	t6, $3;"
		"nop;"
	);
	/* 
	 *  Jump to kernel entry 
	 */
	__asm(

		"lw	t0, 0(t3);"	/* entry addr */
		"lw	a1, 24(t3);"	/* arg1 */
		"lw	a2, 28(t3);"	/* arg2 */
		"lw	a3, 32(t3);"	/* arg3 */
		"lw	a0, 20(t3);"	/* arg0 */
		"jr	t0;"
		"nop;"

		".globl tx39xx_asm_code_end;"
		"tx39xx_asm_code_end: nop;"
		".set reorder;	"
	);
}
