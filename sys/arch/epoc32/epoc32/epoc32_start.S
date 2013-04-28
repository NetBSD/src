/*	$NetBSD: epoc32_start.S,v 1.1 2013/04/28 12:11:26 kiyohara Exp $	*/
/*
 * Copyright (c) 2012, 2013 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <machine/bootinfo.h>
#include <arm/armreg.h>
#include "assym.h"

/*
 * Kernel start routine for EPOC32
 * this code is excuted at the very first after the kernel is loaded.
 */
	.text

	.global _C_LABEL(epoc32_start)
_C_LABEL(epoc32_start):

	/* copy bootinfo */
	adr	r1, bootinfo
	mov	r2, #BTINFO_MAX_SIZE
	sub	r2, r2, #4
0:
	ldr	r3, [r0, r2]
	str	r3, [r1, r2]
	subs	r2, r2, #4
	bge	0b

	/*
	 * Build page table from scratch.
	 * Our boot-loader(e32boot) disables D-cache already.
	 */
	adr	r0, mmu_init_table
	b	2f

1:
	str	r3, [r2]
	add	r2, r2, #4
	add	r3, r3, #L1_S_SIZE
	adds	r1, r1, #-1
	bhi	1b
2:
	ldmia	r0!, {r1, r2}		/* # of sections, VA */
	ldr	r3, startup_pagetable	/* pagetable */
	add	r2, r2, r3
	ldr	r3, [r0], #4		/* # of PA|attr */
	cmp	r1, #0
	bne	1b

	/*
	 * Check physical memory segments. We require least 4MB for startup.
	 */
	adr	r0, bootinfo
next_btinfo:
	ldmia	r0, {r1, r2}
	cmp	r2, #BTINFO_NONE
	beq	99f			/* XXXX: Oops. no memory info? */
	cmp	r2, #BTINFO_MEMORY
	beq	btinfo_memory
	add	r0, r0, r1
	b	next_btinfo

btinfo_memory_index:
	.space	4

btinfo_memory:
	mov	r3, #0
	str	r3, btinfo_memory_index

	ldr	r3, [r0, #12]		/* Load size */
	cmp	r3, #L1_S_SIZE
	blt	map_l1c
map_l1s:
	/* Make L1 descriptor */
	ldr	r3, [r0, #8]		/* Load phys address */
	ldr	r2, btinfo_memory_index
	add	r3, r2, r3
	ldr	r2, startup_pagetable_l1s_attr
	orr	r2, r2, r3

	ldr	r1, startup_pagetable	/* pagetable */
	ldr	r3, startup_space
	add	r3, r3, #KERNEL_BASE
	mov	r3, r3, lsr #(L1_S_SHIFT - 2)
	add	r1, r3, r1
	str	r2, [r1]

	ldr	r2, btinfo_memory_index
	add	r2, r2, #L1_S_SIZE
	str	r2, btinfo_memory_index
	ldr	r1, startup_space
	add	r1, r1, #L1_S_SIZE
	str	r1, startup_space
	cmp	r1, #0x00400000		/* 4MB */
	bge	99f
	ldr	r1, [r0, #12]
	cmp	r1, r2
	bgt	map_l1s
	ldr	r1, [r0]		/* Load size of btinfo_memory */
	add	r0, r0, r1
	b	next_btinfo

map_l1c:
	mov	r1, #1
	mov	r2, r1, lsl #L1_S_SHIFT
	sub	r2, r2, #1
	ldr	r3, startup_space
	ands	r2, r3, r2
	bne	map_l2s

	/* Make L1 descriptor */
	ldr	r2, startup_pagetable
	add	r2, r2, #L1_TABLE_SIZE	/* L2 table */
	mov	r3, r3, lsr #(12 - 2)
	add	r2, r2, r3
	ldr	r3, startup_pagetable_l1c_attr
	orr	r2, r2, r3

	ldr	r1, startup_pagetable
	ldr	r3, startup_space
	add	r3, r3, #KERNEL_BASE
	mov	r3, r3, lsr #(20 - 2)
	add	r1, r1, r3
	str	r2, [r1]

map_l2s:
	ldr	r1, [r0, #8]		/* Load phys address */
	ldr	r2, btinfo_memory_index
	add	r1, r1, r2
	ldr	r2, startup_pagetable_l2s_attr
	orr	r2, r1, r2

	ldr	r1, startup_space
	mov	r1, r1, lsr #(L2_S_SHIFT - 2)
	ldr	r3, startup_pagetable
	add	r3, r3, #L1_TABLE_SIZE	/* L2 table */
	add	r1, r3, r1
	str	r2, [r1]

	ldr	r2, btinfo_memory_index
	add	r2, r2, #L2_S_SIZE
	str	r2, btinfo_memory_index
	ldr	r1, startup_space
	add	r1, r1, #L2_S_SIZE
	str	r1, startup_space
	cmp	r1, #0x00400000		/* 4MB */
	bge	99f
	ldr	r1, [r0, #12]
	cmp	r1, r2
	bgt	map_l2s
	ldr	r1, [r0]		/* Load size of btinfo_memory */
	add	r0, r0, r1
	b	next_btinfo

99:

	/*
	 * Set the Domain Access register.  Very important!
	 * startup_pagetable puts to domain 0 now.
	 */
#define KERNEL_DOMAIN(x)	((x) << (PMAP_DOMAIN_KERNEL << 1))
	mov	r0, #(KERNEL_DOMAIN(DOMAIN_CLIENT) | DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0

	/*
	 * Enable MMU and etc...
	 */
	ldr	r0, startup_pagetable
	mcr	p15, 0, r0, c2, c0, 0	/* Set TTB */

	mcr	p15, 0, r1, c0, c0, 0	/* Get ProcID */
	and	r2, r1, #CPU_ID_IMPLEMENTOR_MASK
	cmp	r2, #CPU_ID_DEC
	beq	cpu_id_dec
	ands	r2, r1, #CPU_ID_7ARCH_MASK
	bne	cpu_id_7arch_v4t
cpu_id_7arch_v3:	/* 7 */
	mcr	p15, 0, r0, c5, c0, 0	/* Flash TLB */
	b	enable_mmu

cpu_id_7arch_v4t:	/* 7TDMI */
cpu_id_dec:		/* SA1100 */
	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0	/* Flash TLB */
	nop

enable_mmu:
	ldr	r0, startup_control
	mcr	p15, 0, r0, c1, c0, 0
	nop
	nop

	/* Jump to kernel code in TRUE VA */
	adr	r0, Lstart
	ldr	pc, [r0]

Lstart:
	.word	start

	.globl	_C_LABEL(startup_pagetable)
startup_pagetable:
#define STARTUP_PAGETABLE_ADDR		0xc0000000	/* aligned 16kByte */
	.word	STARTUP_PAGETABLE_ADDR
startup_pagetable_l1s_attr:
	.word	L1_S_PROTO_generic | L1_S_C | L1_S_AP_KRW
startup_pagetable_l1c_attr:
	.word	L1_C_PROTO_generic
startup_pagetable_l2s_attr:
	.word	L2_S_PROTO_generic | L2_C | L2_AP_KRW
startup_space:
	.word	0x00000000
startup_control:
	.word	CPU_CONTROL_MMU_ENABLE  | \
		CPU_CONTROL_DC_ENABLE   | \
		CPU_CONTROL_WBUF_ENABLE | \
		CPU_CONTROL_32BP_ENABLE | \
		CPU_CONTROL_32BD_ENABLE | \
		CPU_CONTROL_IC_ENABLE

	.globl	_C_LABEL(bootinfo)
bootinfo:
	.space	BTINFO_MAX_SIZE

#define MMU_INIT(va, pa, n_sec, attr)	  \
	.word	n_sec			; \
	.word	(va) >> (L1_S_SHIFT - 2); \
	.word	(pa) | (attr)		;

mmu_init_table:
	/* fill all table VA==PA */
	MMU_INIT(0x00000000, 0x00000000,
	    1 << (32 - L1_S_SHIFT), L1_S_PROTO | L1_S_AP_KRW)
	MMU_INIT(0, 0, 0, 0)		/* end of table */
