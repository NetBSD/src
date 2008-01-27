/*	$NetBSD: start.S,v 1.1.82.1 2008/01/27 13:08:41 chris Exp $	*/

/*
 * Copyright (c) 2002 Reinoud Zandijk
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <machine/asm.h>
#include <arm/armreg.h>
#include <riscoscalls.h>


/* ----------------------------------------------------------------------- */
ENTRY(relocate_code)
	/*
		- r0 pointer to configuration structure
		- r1 pointer to physical restart point
		- r2 pointer to relocation table (P)
		- r3 pointer to physical new L1 page address (P)
		- r4 kernel entry point
	*/

	/* save registers / move args up in register bank later */
	/* r8-r12 becomes r0-r4 */
	stmfd	sp!, {r0-r4}
	ldmfd	sp!, {r8-r12}

	/*
	 * determine processor architecture version. This is nessisary for the
	 * correct coprocessor instruction.
	 */
	mrc	15, 0, r0, c0, c0, 0					/* read CPU id in r0			*/
	mov	r3, r0							/* store in r3				*/

	/* assume its ARMv4 instruction set									*/
	mov	r14, #1

	/* check ARM6. It needs a special mask									*/
	mov	r0, #0x0000ff00
	mov	r1, #0x00000600						/* check for 0xxxxx06xx => ARM6		*/
	and	r2, r3, r0
	cmp	r2, r1
	moveq	r14, #0							/* mark v3				*/

	/* newer ARM's need a different mask									*/
	mov	r0, #0x0000f000
	
	/* check for ARM7 and derivatives like the ARM 7500 and ARM 7500FE					*/
	mov	r1, #0x00007000						/* check for 0xxxxx7xxx => ARM 7	*/
	and	r2, r3, r0
	cmp	r2, r1
	moveq	r14, #0							/* mark v3				*/

	/* switch off MMU, IDcache and WB and branch to physical code space					*/
	cmp	r14, #0
	mrcne	15, 0, r0, c1, c0, 0					/* read processor control register if v4*/
	bic	r0, r0, #0x3f						/* clear only known bits		*/
	moveq	r0, #0							/* for v3 just set to zero		*/
	orr	r0, r0, #CPU_CONTROL_LABT_ENABLE | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_32BP_ENABLE
	mov	r13, r0							/* save this control value in r13	*/
	cmp	r14, #0
	mcr	15, 0, r0, c1, c0, 0					/* write control register!		*/
/*1*/	mcrne	15, 0, r1, c7, c5, 0					/* write zero in ARMv4 MMU disable	*/
/*2*/	mov	pc, r9							/* branch to physical address		*/	
	
relocate_code_physical_restart:
	/* we are running in physical flat 1:1 space now */
	mov	r5, r10							/* r5 = is start of relocation table	*/
	ldr	r6, [r5], #4						/* r4 = number of relocated pages	*/
loop_relocate_pages:
	ldr	r2, [r5], #4						/* r2 = from address			*/
	ldr	r3, [r5], #4						/* r3 = to address			*/
	ldr	r7, [r5], #4						/* r7 = number of bytes to travel	*/
	mov	r1, #0							/* r1 = offset in page			*/
	/* its slow ... we dont know anything about alignment here 						*/
loop_one_page:
	ldrb	r0, [r2, r1]
	strb	r0, [r3, r1]
	add	r1, r1, #1
	cmp	r1, r7							/* all bytes copied?			*/
	bne	loop_one_page
	subs	r6, r6, #1
	bne	loop_relocate_pages

	/* OK! all is relocated... now switch over to the new L1 pages						*/

	/* flush ID cache											*/
	mov	r0, #0
	cmp	r14, #0
	mcreq	15, 0, r0, c7, c0, 0					/* flush v3 ID cache			*/
	mcrne	15, 0, r0, c7, c7, 0					/* flush v4 ID cache			*/

	/* drain write buffer (v4)										*/
	mov	r0, #0
	cmp	r14, #0
	mcrne	15, 0, r0, c7, c10, 4					/* drain WB (v4)			*/

	/* flush TLB												*/
	mcr	15, 0, r0, c5, c0, 0					/* flush TLB for v3 and v4		*/

	/* set new TLB address											*/
	mov	r0, r11
	mcr	15, 0, r0, c2, c0, 0					/* write TLB address			*/

	/* Switch on MMU, IDCache and WB and keep on running on flat translated memory				*/
	orr	r0, r13, #CPU_CONTROL_LABT_ENABLE | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_32BP_ENABLE
	orr	r0, r0,  #CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_DC_ENABLE   | CPU_CONTROL_MMU_ENABLE
	mcr	15, 0, r0, c1, c0, 0					/* write register !!!			*/
	mov	r0, r0							/* flat					*/
	mov	r0, r0							/* flat					*/
	/* not flat anymore but we just continue								*/

	/* call the kernel!											*/
	mov	r0, r8							/* saved configuration structure	*/
	mov	pc, r12							/* entry point ..... bye bye!		*/

relocate_code_end:
	b	relocate_code_end

/* ----------------------------------------------------------------------- */


/* we are not expected to ever return from here */
ENTRY(start_kernel)
	/*
	entry conditions :
		- on RISC OS page tables in usr26 mode on virtual space
		- r0 relocation code page (V)
		- r1 relocation pv offset
		- r2 configuration structure
		- r3 relocation table (P)
		- r4 L1 page descriptor (P)
		- r5 kernel entry point
	*/
	mov	ip, sp
	stmfd	sp!, {r4-r9, fp, ip, lr, pc}
	sub	fp, ip, #4

	/* get stuff out of the calling frame */
	ldr	r4, [ip, #0]
	ldr	r5, [ip, #4]

	/* relocate the relocation routine to the given page */
	adr	r6, relocate_code
	ldr	r7, Lnbpp
	mov	r8, r0
relocate_code_loop:
	ldr	r9, [r6], #4
	str	r9, [r8], #4
	subs	r7, r7, #4
	bne	relocate_code_loop

	/* we messed up the data cache : lets read a 64 or 128 kb <-- GROSS */
	mov	r7, #128*1024
	mov	r6, #0x8000						/* start of RISCOS application area	*/
flush_ID_cache_try:
	ldr	r9, [r6], #4
	subs	r7, r7, #4
	bne	flush_ID_cache_try

	/* enter sub26 mode */
	swi	OS_EnterOS

	/* go to sup32 mode with ICQ and FIQ disabled */
	mrs	r6, cpsr
	bic	r6, r6, #PSR_MODE					/* clear processor mode			*/
	orr	r6, r6, #(I32_bit | F32_bit)				/* disable ICQ + FIQ			*/
	orr	r6, r6, #PSR_SVC32_MODE					/* go to 32 bit supervisor mode		*/
	msr	cpsr, r6
	mov	r0, r0							/* nops ... just in case		*/
	mov	r0, r0

	/* set up info */
	mov	r9, r0							/* save relocated page address		*/
	mov	r7, #relocate_code_physical_restart - relocate_code	/* get offset				*/
	add	r1, r0, r1						/* get physical address			*/
	add	r1, r1, r7						/* add offset				*/
	mov	r0, r2							/* put configuration structure in r0	*/
	mov	r2, r3							/* relocation table			*/
	mov	r3, r4							/* L1 page discriptor			*/
	mov	r4, r5							/* kernel entry point			*/

	mov	pc, r9							/* jump to page addr == relocate_code	*/

emergency_exit:
	ldmdb	fp, {r4-r9, fp, sp, pc}

Lnbpp:
	.word	nbpp
Lvideomem_start_ro:
	.word	videomem_start_ro

