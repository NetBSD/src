/*
 * mpcsa_start.S
 * Copyright (c) 2007, S. Kantoluoto <sami.kantoluoto@iki.fi>
 * All rights reserved.
 *
 * Based on vx115_vep_start.S
 * Copyright (c) 2007, J. Sevy <jsevy@cs.drexel.edu>
 *
 * Based on g42xxeb_start.S
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/arm32/pte.h>
#include <arm/arm32/pmap.h>        /* for PMAP_DOMAIN_KERNEL */

#ifndef	FLASH_START
#define	FLASH_START	0x10020000
#endif

#ifndef SDRAM_START
#define SDRAM_START    0x20000000
#endif

#define	MODBASE	       0x20200000

/*
 * CPWAIT -- Canonical method to wait for CP15 update.
 * NOTE: Clobbers the specified temp reg.
 * copied from arm/arm/cpufunc_asm_xscale.S
 * XXX: better be in a common header file.
 */
#define    CPWAIT_BRANCH    \
    sub pc, pc, #4

#define    CPWAIT(tmp)                             				 \
    mrc p15, 0, tmp, c2, c0, 0  /* arbitrary read of CP15 */    ;\
    mov tmp, tmp        		/* wait for it to complete */   ;\
    CPWAIT_BRANCH            	/* branch to next insn */		
    
/*
 * Kernel start routine for MPCSA board.
 * This code is executed from Flash when the bootloader jumps to it.
 */
    .text

    .global _C_LABEL(mpcsa_start)
_C_LABEL(mpcsa_start):

    /* make sure we're in supervisor mode */
    mov	   r0,sp
    msr    CPSR_c,#I32_bit | F32_bit | PSR_SVC32_MODE
    mov	   sp,r0

    /* move code to RAM */
    ldr    r1, Lcopy_size
    adr    r0, _C_LABEL(mpcsa_start)
    add    r1, r1, #3
    mov    r1, r1, LSR #2         /* get size of code in words */
    mov    r2, #SDRAM_START
    add    r2, r2, #0x200000    /* code placed 2MB above kernel base */
    mov    r4, r2

    /* copy kernel to RAM */
5:  ldr    r3, [r0], #4
    subs   r1, r1, #1
    str    r3, [r2], #4
    bhi    5b

    /* jump to RAM */
    ldr    r0, Lstart_off
    add    pc, r4, r0


Lcopy_size:    .word _edata-_C_LABEL(mpcsa_start)
Lstart_off:    .word mpcsa_start_ram-_C_LABEL(mpcsa_start)

mpcsa_start_ram:    
    /*
     *  Kernel is loaded in SDRAM (0x20200000), and is expected to run
     *  in VA 0xc0200000.
     */

    /* build page table from scratch */
    ldr    r0, Lstartup_pagetable
    adr    r4, mmu_init_table
    b      3f

2:
    str    r3, [r0, r2]
    add    r2, r2, #4
    add    r3, r3, #(L1_S_SIZE)
    adds   r1, r1, #-1
    bhi    2b
3:    
    ldmia  r4!, {r1,r2,r3}   /* # of sections, PA|attr, VA */
    cmp    r1, #0
    bne    2b    


    mcr    p15, 0, r0, c2, c0, 0    /* Set TTB */
    mcr    p15, 0, r0, c8, c7, 0    /* Flush TLB */

    /* Set the Domain Access register.  Very important! */
    mov    r0, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
    mcr    p15, 0, r0, c3, c0, 0

    /* Enable MMU */
    mrc    p15, 0, r0, c1, c0, 0
    orr    r0, r0, #CPU_CONTROL_MMU_ENABLE
    mcr    p15, 0, r0, c1, c0, 0
    CPWAIT(r0)

    /* Jump to kernel code in TRUE VA */
    adr    r0, Lstart
    ldr    pc, [r0]

Lstart:
    .word    start


#define MMU_INIT(va,pa,n_sec,attr)  \
    .word    n_sec                 ;\
    .word    4*((va)>>L1_S_SHIFT)  ;\
    .word    (pa) | (attr)

#define STARTUP_PAGETABLE_ADDR  0x20100000 + 0x4000

Lstartup_pagetable:
	.word STARTUP_PAGETABLE_ADDR
	
mmu_init_table:    
    /* fill all table VA==PA */
    MMU_INIT(0x00000000, 0x00000000, 1<<(32-L1_S_SHIFT), L1_TYPE_S|L1_S_AP(AP_KRW))
    /* map in peripheral space */
    MMU_INIT(0xfff00000, 0xfff00000, 1, L1_TYPE_S|L1_S_AP(AP_KRW))
    /* map SDRAM VA==PA, WT cacheable */
    MMU_INIT(0x20100000, 0x20100000, 63, L1_TYPE_S|L1_S_C|L1_S_AP(AP_KRW))
    /* map VA 0xc0000000..0xc0efffff to PA 0x20100000..0x23ffffff */
    MMU_INIT(0xc0000000, 0x20000000, 63, L1_TYPE_S|L1_S_C|L1_S_AP(AP_KRW))
    .word 0    /* end of table */

	.align	7
jme_module_header:
	.word	0xe990510e		/* magic number			*/
	.string	"NetBSD/mpcsa\0\0\0"	/* module name			*/
	.short	0			/* build flags			*/
	.short	0			/* flags			*/
	.word	0			/* version			*/
	.word	0			/* compile / link timestamp	*/
	.word	MODBASE		/* pointer to init routine	*/
	.word	MODBASE		/* start address		*/
	.word	(_edata-(_C_LABEL(mpcsa_start)))			/* length of module		*/
	.word	(jme_module_header-_C_LABEL(mpcsa_start)+MODBASE)			/* back pointer to module struct*/
	.word	0			/* data pointer (ignored but SBZ)*/
	.word	0			/* data length (ignored but SBZ)*/
	.word	0x20200000		/* alternate address		*/
	.word	0			/* reserved, SBZ		*/

jme_kmodule_header:
	.word	0xe000301e		/* magic number			*/
	.string	"JME kernel\0\0\0\0\0"	/* module name			*/
	.short	0			/* build flags			*/
	.short	0			/* flags			*/
	.word	0			/* version			*/
	.word	0			/* compile / link timestamp	*/
	.word	MODBASE		/* pointer to init routine	*/
	.word	MODBASE		/* start address		*/
	.word	(_edata-(_C_LABEL(mpcsa_start)))			/* length of module		*/
	.word	(jme_kmodule_header-_C_LABEL(mpcsa_start)+MODBASE)	/* back pointer to module struct*/
	.word	0			/* data pointer (ignored but SBZ)*/
	.word	0			/* data length (ignored but SBZ)*/
	.word	0x20200000		/* alternate address		*/
	.word	0			/* reserved, SBZ		*/
