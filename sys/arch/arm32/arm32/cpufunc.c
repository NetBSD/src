/*	$NetBSD: cpufunc.c,v 1.1 1997/02/04 05:50:21 mark Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
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
 *	This product includes software developed by Causality Limited.
 * 4. The name of Causality Limited may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUSALITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpufuncs.c
 *
 * C functions for supporting CPU / MMU / TLB specific operations.
 *
 * Created      : 30/01/97
 */

#include <sys/types.h>
#include <machine/cpu.h>
#include <machine/cpufunc.h>

#ifdef CPU_ARM6
struct cpu_functions arm6_cpufuncs = {
	/* CPU functions */
	
	cpufunc_id,			/* id			 */

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* domain		*/
	arm67_setttb,			/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	arm67_tlb_flush,		/* tlb_flushID		*/
	arm67_tlb_purge,		/* tlb_flushID_SE	*/
	arm67_tlb_flush,		/* tlb_flushI		*/
	arm67_tlb_purge,		/* tlb_flushI_SE	*/
	arm67_tlb_flush,		/* tlb_flushD		*/
	arm67_tlb_purge,		/* tlb_flushD_SE	*/

	/* Cache functions */

	arm67_cache_flush,		/* cache_flushID	*/
	(void *)arm67_cache_flush,	/* cache_flushID_SE	*/
	arm67_cache_flush,		/* cache_flushI		*/
	(void *)arm67_cache_flush,	/* cache_flushI_SE	*/
	arm67_cache_flush,		/* cache_flushD		*/
	(void *)arm67_cache_flush,	/* cache_flushD_SE	*/

	cpufunc_nullop,			/* cache_cleanID	s*/
	(void *)cpufunc_nullop,		/* cache_cleanID_E	s*/
	cpufunc_nullop,			/* cache_cleanD		s*/
	(void *)cpufunc_nullop,		/* cache_cleanD_E	*/

	arm67_cache_flush,		/* cache_purgeID	s*/
	(void *)arm67_cache_flush,	/* cache_purgeID_E	s*/
	arm67_cache_flush,		/* cache_purgeD		s*/
	(void *)arm67_cache_flush,	/* cache_purgeD_E	s*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	cpufunc_nullop,			/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	cpufunc_nullop,			/* cache_syncI		*/
	(void *)cpufunc_nullop,		/* cache_cleanID_rng	*/
	(void *)cpufunc_nullop,		/* cache_cleanD_rng	*/
	(void *)arm67_cache_flush,	/* cache_purgeID_rng	*/
	(void *)arm67_cache_flush,	/* cache_purgeD_rng	*/
	(void *)cpufunc_nullop,		/* cache_syncI_rng	*/

	arm6_dataabt_fixup,		/* dataabt_fixup	*/
	arm6_prefetchabt_fixup,		/* prefetchabt_fixup	*/

	arm67_context_switch		/* context_switch	*/
};
#endif	/* CPU_ARM6 */

#ifdef CPU_ARM7
struct cpu_functions arm7_cpufuncs = {
	/* CPU functions */
	
	cpufunc_id,			/* id			 */

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* domain		*/
	arm67_setttb,			/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	arm67_tlb_flush,		/* tlb_flushID		*/
	arm67_tlb_purge,		/* tlb_flushID_SE	*/
	arm67_tlb_flush,		/* tlb_flushI		*/
	arm67_tlb_purge,		/* tlb_flushI_SE	*/
	arm67_tlb_flush,		/* tlb_flushD		*/
	arm67_tlb_purge,		/* tlb_flushD_SE	*/

	/* Cache functions */

	arm67_cache_flush,		/* cache_flushID	*/
	(void *)arm67_cache_flush,	/* cache_flushID_SE	*/
	arm67_cache_flush,		/* cache_flushI		*/
	(void *)arm67_cache_flush,	/* cache_flushI_SE	*/
	arm67_cache_flush,		/* cache_flushD		*/
	(void *)arm67_cache_flush,	/* cache_flushD_SE	*/

	cpufunc_nullop,			/* cache_cleanID	s*/
	(void *)cpufunc_nullop,		/* cache_cleanID_E	s*/
	cpufunc_nullop,			/* cache_cleanD		s*/
	(void *)cpufunc_nullop,		/* cache_cleanD_E	*/

	arm67_cache_flush,		/* cache_purgeID	s*/
	(void *)arm67_cache_flush,	/* cache_purgeID_E	s*/
	arm67_cache_flush,		/* cache_purgeD		s*/
	(void *)arm67_cache_flush,	/* cache_purgeD_E	s*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	cpufunc_nullop,			/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	cpufunc_nullop,			/* cache_syncI		*/
	(void *)cpufunc_nullop,		/* cache_cleanID_rng	*/
	(void *)cpufunc_nullop,		/* cache_cleanD_rng	*/
	(void *)arm67_cache_flush,	/* cache_purgeID_rng	*/
	(void *)arm67_cache_flush,	/* cache_purgeD_rng	*/
	(void *)cpufunc_nullop,		/* cache_syncI_rng	*/

	arm7_dataabt_fixup,		/* dataabt_fixup	*/
	arm7_prefetchabt_fixup,		/* prefetchabt_fixup	*/

	arm67_context_switch		/* context_switch	*/
};
#endif	/* CPU_ARM7 */

#ifdef CPU_SA110
struct cpu_functions sa110_cpufuncs = {
	/* CPU functions */
	
	cpufunc_id,			/* id			 */

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* domain		*/
	sa110_setttb,			/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	sa110_tlb_flushID,		/* tlb_flushID		*/
	(void *)sa110_tlb_flushID,	/* tlb_flushID_SE	*/
	sa110_tlb_flushI,		/* tlb_flushI		*/
	(void *)sa110_tlb_flushI,	/* tlb_flushI_SE	*/
	sa110_tlb_flushD,		/* tlb_flushD		*/
	sa110_tlb_flushD_SE,		/* tlb_flushD_SE	*/

	/* Cache functions */

	sa110_cache_flushID,		/* cache_flushID	*/
	(void *)sa110_cache_flushID,	/* cache_flushID_SE	*/
	sa110_cache_flushI,		/* cache_flushI		*/
	(void *)sa110_cache_flushI,	/* cache_flushI_SE	*/
	sa110_cache_flushD,		/* cache_flushD		*/
	sa110_cache_flushD_SE,		/* cache_flushD_SE	*/

	sa110_cache_cleanID,		/* cache_cleanID	s*/
	sa110_cache_cleanD_E,		/* cache_cleanID_E	s*/
	sa110_cache_cleanD,		/* cache_cleanD		s*/
	sa110_cache_cleanD_E,		/* cache_cleanD_E	*/

	sa110_cache_purgeID,		/* cache_purgeID	s*/
	sa110_cache_purgeID_E,		/* cache_purgeID_E	s*/
	sa110_cache_purgeD,		/* cache_purgeD		s*/
	sa110_cache_purgeD_E,		/* cache_purgeD_E	s*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	sa110_drain_writebuf,		/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	sa110_cache_syncI,		/* cache_syncI		*/
	sa110_cache_cleanID_rng,	/* cache_cleanID_rng	*/
	sa110_cache_cleanD_rng,		/* cache_cleanD_rng	*/
	sa110_cache_purgeID_rng,	/* cache_purgeID_rng	*/
	sa110_cache_purgeD_rng,		/* cache_purgeD_rng	*/
	sa110_cache_syncI_rng,		/* cache_syncI_rng	*/

	sa110_dataabt_fixup,		/* dataabt_fixup	*/
	sa110_prefetchabt_fixup,	/* prefetchabt_fixup	*/

	sa110_context_switch		/* context_switch	*/

};          
#endif	/* CPU_SA110 */

struct cpu_functions cpufuncs;
u_int cputype;

/*
 * Cannot panic here as we may not have a console yet ...
 */

int
set_cpufuncs()
{
	cputype = cpufunc_id();
	cputype &= CPU_ID_CPU_MASK;

	switch (cputype) {
#ifdef CPU_ARM6
	case ID_ARM610:
		cpufuncs = arm6_cpufuncs;
		break;
#endif	/* CPU_ARM6 */
#ifdef CPU_ARM7
	case ID_ARM700:
	case ID_ARM710:
		cpufuncs = arm7_cpufuncs;
		break;
#endif	/* CPU_ARM7 */
#ifdef CPU_ARM8
	case ID_ARM810:
		/*
		 * Bzzzz. And the answer was ...
		 */
/*		panic("ARM8 architectures are not yet supported\n");*/
		return(ARCHITECTURE_NOT_SUPPORTED);
		break;
#endif	/* CPU_ARM8 */
#ifdef CPU_SA110
	case ID_SA110:
		cpufuncs = sa110_cpufuncs;
		break;
#endif	/* CPU_SA110 */
	default:
		/*
		 * Bzzzz. And the answer was ...
		 */
/*		panic("No support for this CPU type (%03x) in kernel\n", id >> 4);*/
		return(ARCHITECTURE_NOT_PRESENT);
		break;
	}
	return(0);
}

/*
 * Fixup routines for data and prefetch aborts.
 *
 * Several compile time symbols are used
 *
 * DEBUG_FAULT_CORRECTION - Print debugging information during the
 * correction of registers after a fault.
 * CPU_LATE_ABORT - Architectures supporting both early and late aborts
 * should use late aborts (only affects ARM6)
 */

#if defined(CPU_ARM6) || defined(CPU_ARM7)
extern int pmap_debug_level;
#endif

#ifdef CPU_ARM6
int
arm6_dataabt_fixup(arg)
	void *arg;
{
	trapframe_t *frame = arg;
	u_int fault_pc;
	u_int fault_instruction;
	int saved_lr = 0;

	if ((frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {

		/* Ok an abort in SVC mode */

		/*
		 * Copy the SVC r14 into the usr r14 - The usr r14 is garbage
		 * as the fault happened in svc mode but we need it in the
		 * usr slot so we can treat the registers as an array of ints
		 * during fixing.
		 * NOTE: This PC is in the position but writeback is not
		 * allowed on r15.
		 * Doing it like this is more efficient than trapping this
		 * case in all possible locations in the following fixup code.
		 */

		saved_lr = frame->tf_usr_lr;
		frame->tf_usr_lr = frame->tf_svc_lr;

		/*
		 * Note the trapframe does not have the SVC r13 so a fault
		 * from an instruction with writeback to r13 in SVC mode is
		 * not allowed. This should not happen as the kstack is
		 * always valid.
		 */
	}

	/* Get fault address and status from the CPU */

	fault_pc = frame->tf_pc;
	fault_instruction = *((volatile unsigned int *)fault_pc);

	/* Decode the fault instruction and fix the registers as needed */

	/* Was is a swap instruction ? */

	if ((fault_instruction & 0x0fb00ff0) == 0x01000090) {
#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >= 0)
			disassemble(fault_pc);
#endif	/* DEBUG_FAULT_CORRECTION */
	} else if ((fault_instruction & 0x0c000000) == 0x04000000) {

	/* Was is a ldr/str instruction */

#ifdef CPU_LATE_ABORT

		/* This is for late abort only */

		int base;
		int offset;
		int *registers = &frame->tf_r0;
#endif	/* CPU_LATE_ABORT */

#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >= 0)
			disassemble(fault_pc);
#endif	/* DEBUG_FAULT_CORRECTION */
		
#ifdef CPU_LATE_ABORT

/* This is for late abort only */

	if ((fault_instruction & (1 << 24)) == 0
	    || (fault_instruction & (1 << 21)) != 0) {
		base = (fault_instruction >> 16) & 0x0f;
		if (base == 13 && (frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {
			disassemble(fault_pc);
			panic("Abort handler cannot fix this :-(\n");
		}
		if (base == 15) {
			disassemble(fault_pc);
			panic("Abort handler cannot fix this :-(\n");
		}
#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >=0)
			printf("late abt fix: r%d=%08x ", base, registers[base]);
#endif	/* DEBUG_FAULT_CORRECTION */
		if ((fault_instruction & (1 << 25)) == 0) {
			/* Immediate offset - easy */                  
			offset = fault_instruction & 0xfff;
			if ((fault_instruction & (1 << 23)))
				offset = -offset;
			registers[base] += offset;
#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >=0)
				printf("imm=%08x ", offset);
#endif	/* DEBUG_FAULT_CORRECTION */
		} else {
			int shift;

			offset = fault_instruction & 0x0f;
			if (offset == base) {
				disassemble(fault_pc);
				panic("Abort handler cannot fix this :-(\n");
			}
                
/* Register offset - hard we have to cope with shifts ! */
			offset = registers[offset];

			if ((fault_instruction & (1 << 4)) == 0)
				shift = (fault_instruction >> 7) & 0x1f;
			else {
				if ((fault_instruction & (1 << 7)) != 0) {
					disassemble(fault_pc);
					panic("Abort handler cannot fix this :-(\n");
				}
				shift = ((fault_instruction >> 8) & 0xf);
				if (base == shift) {
					disassemble(fault_pc);
					panic("Abort handler cannot fix this :-(\n");
				}
#ifdef DEBUG_FAULT_CORRECTION
				if (pmap_debug_level >=0)
					printf("shift reg=%d ", shift);
#endif	/* DEBUG_FAULT_CORRECTION */
				shift = registers[shift];
			}
#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >=0)
				printf("shift=%08x ", shift);
#endif	/* DEBUG_FAULT_CORRECTION */
			switch (((fault_instruction >> 5) & 0x3)) {
			case 0 : /* Logical left */
				offset = (int)(((u_int)offset) << shift);
				break;
			case 1 : /* Logical Right */
				if (shift == 0) shift = 32;
				offset = (int)(((u_int)offset) >> shift);
				break;
			case 2 : /* Arithmetic Right */
				if (shift == 0) shift = 32;
				offset = (int)(((int)offset) >> shift);
				break;
			case 3 : /* Rotate right */
				disassemble(fault_pc);
				panic("Abort handler cannot fix this yet :-(\n");
				break;
			}

#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >=0)
				printf("abt: fixed LDR/STR with register offset\n");
#endif	/* DEBUG_FAULT_CORRECTION */               
			if ((fault_instruction & (1 << 23)))
				offset = -offset;
#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >=0)
				printf("offset=%08x ", offset);
#endif	/* DEBUG_FAULT_CORRECTION */
			registers[base] += offset;
		}
#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >=0)
			printf("r%d=%08x\n", base, registers[base]);
#endif	/* DEBUG_FAULT_CORRECTION */
	}
#endif	/* CPU_LATE_ABORT */
	} else if ((fault_instruction & 0x0e000000) == 0x08000000) {
		int base;
		int loop;
		int count;
		int *registers = &frame->tf_r0;
        
#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >= 0) {
			printf("LDM/STM\n");
			disassemble(fault_pc);
		}
#endif	/* DEBUG_FAULT_CORRECTION */
		if (fault_instruction & (1 << 21)) {
#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >= 0)
				printf("This instruction must be corrected\n");
#endif	/* DEBUG_FAULT_CORRECTION */
			base = (fault_instruction >> 16) & 0x0f;
			if (base == 15) {
				disassemble(fault_pc);
				panic("Abort handler cannot fix this :-(\n");
			}
			/* Count registers transferred */
			count = 0;
			for (loop = 0; loop < 16; ++loop) {
				if (fault_instruction & (1<<loop))
					++count;
			}
#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >= 0) {
				printf("%d registers used\n", count);
				printf("Corrected r%d by %d bytes ", base, count * 4);
			}
#endif	/* DEBUG_FAULT_CORRECTION */
			if (fault_instruction & (1 << 23)) {
#ifdef DEBUG_FAULT_CORRECTION
				if (pmap_debug_level >= 0)
					printf("down\n");
#endif	/* DEBUG_FAULT_CORRECTION */
				registers[base] -= count * 4;
			} else {
#ifdef DEBUG_FAULT_CORRECTION
				if (pmap_debug_level >= 0)
					printf("up\n");
#endif	/* DEBUG_FAULT_CORRECTION */
				registers[base] += count * 4;
			}
		}
	} else if ((fault_instruction & 0x0e000000) == 0x0c000000) {
		int base;
		int offset;
		int *registers = &frame->tf_r0;
	
/* REGISTER CORRECTION IS REQUIRED FOR THESE INSTRUCTIONS */

#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >= 0)
			disassemble(fault_pc);
#endif	/* DEBUG_FAULT_CORRECTION */

/* Only need to fix registers if write back is turned on */

		if ((fault_instruction & (1 << 21)) != 0) {
			base = (fault_instruction >> 16) & 0x0f;
			if (base == 13 && (frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {
				disassemble(fault_pc);
				panic("Abort handler cannot fix this :-(\n");
			}
			if (base == 15) {
				disassemble(fault_pc);
				panic("Abort handler cannot fix this :-(\n");
			}

			offset = (fault_instruction & 0xff) << 2;
			if (pmap_debug_level >= 0)
				printf("r%d=%08x\n", base, registers[base]);
			if ((fault_instruction & (1 << 23)) != 0)
				offset = -offset;
			registers[base] += offset;
			if (pmap_debug_level >= 0)
				printf("r%d=%08x\n", base, registers[base]);
		}
	} else if ((fault_instruction & 0x0e000000) == 0x0c000000) {
		disassemble(fault_pc);
		panic("How did this happen ...\nWe have faulted on a non data transfer instruction");
	}

	if ((frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {

		/* Ok an abort in SVC mode */

		/*
		 * Copy the SVC r14 into the usr r14 - The usr r14 is garbage
		 * as the fault happened in svc mode but we need it in the
		 * usr slot so we can treat the registers as an array of ints
		 * during fixing.
		 * NOTE: This PC is in the position but writeback is not
		 * allowed on r15.
		 * Doing it like this is more efficient than trapping this
		 * case in all possible locations in the prior fixup code.
		 */

		frame->tf_svc_lr = frame->tf_usr_lr;
		frame->tf_usr_lr = saved_lr;

		/*
		 * Note the trapframe does not have the SVC r13 so a fault
		 * from an instruction with writeback to r13 in SVC mode is
		 * not allowed. This should not happen as the kstack is
		 * always valid.
		 */
	}

	return(0);
}


int
arm6_prefetchabt_fixup(arg)
	void *arg;
{
	return(0);
}
#endif	/* CPU_ARM6 */

#ifdef CPU_ARM7
int
arm7_dataabt_fixup(arg)
	void *arg;
{
	trapframe_t *frame = arg;
	u_int fault_pc;
	u_int fault_instruction;
	int saved_lr = 0;

	if ((frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {

		/* Ok an abort in SVC mode */

		/*
		 * Copy the SVC r14 into the usr r14 - The usr r14 is garbage
		 * as the fault happened in svc mode but we need it in the
		 * usr slot so we can treat the registers as an array of ints
		 * during fixing.
		 * NOTE: This PC is in the position but writeback is not
		 * allowed on r15.
		 * Doing it like this is more efficient than trapping this
		 * case in all possible locations in the following fixup code.
		 */

		saved_lr = frame->tf_usr_lr;
		frame->tf_usr_lr = frame->tf_svc_lr;

		/*
		 * Note the trapframe does not have the SVC r13 so a fault
		 * from an instruction with writeback to r13 in SVC mode is
		 * not allowed. This should not happen as the kstack is
		 * always valid.
		 */
	}

	/* Get fault address and status from the CPU */

	fault_pc = frame->tf_pc;
	fault_instruction = *((volatile unsigned int *)fault_pc);

	/* Decode the fault instruction and fix the registers as needed */

	/* Was is a swap instruction ? */

	if ((fault_instruction & 0x0fb00ff0) == 0x01000090) {
#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >= 0)
			disassemble(fault_pc);
#endif	/* DEBUG_FAULT_CORRECTION */
	} else if ((fault_instruction & 0x0c000000) == 0x04000000) {

		/* Was is a ldr/str instruction */
		/* This is for late abort only */

		int base;
		int offset;
		int *registers = &frame->tf_r0;

#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >= 0)
			disassemble(fault_pc);
#endif	/* DEBUG_FAULT_CORRECTION */
		
	/* This is for late abort only */

	if ((fault_instruction & (1 << 24)) == 0
	    || (fault_instruction & (1 << 21)) != 0) {
		base = (fault_instruction >> 16) & 0x0f;
		if (base == 13 && (frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {
			disassemble(fault_pc);
			panic("Abort handler cannot fix this :-(\n");
		}
		if (base == 15) {
			disassemble(fault_pc);
			panic("Abort handler cannot fix this :-(\n");
		}
#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >=0)
			printf("late abt fix: r%d=%08x ", base, registers[base]);
#endif	/* DEBUG_FAULT_CORRECTION */
		if ((fault_instruction & (1 << 25)) == 0) {
			/* Immediate offset - easy */                  
			offset = fault_instruction & 0xfff;
			if ((fault_instruction & (1 << 23)))
				offset = -offset;
			registers[base] += offset;
#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >=0)
				printf("imm=%08x ", offset);
#endif	/* DEBUG_FAULT_CORRECTION */
		} else {
			int shift;

			offset = fault_instruction & 0x0f;
			if (offset == base) {
				disassemble(fault_pc);
				panic("Abort handler cannot fix this :-(\n");
			}
                
/* Register offset - hard we have to cope with shifts ! */
			offset = registers[offset];

			if ((fault_instruction & (1 << 4)) == 0)
				shift = (fault_instruction >> 7) & 0x1f;
			else {
				if ((fault_instruction & (1 << 7)) != 0) {
					disassemble(fault_pc);
					panic("Abort handler cannot fix this :-(\n");
				}
				shift = ((fault_instruction >> 8) & 0xf);
				if (base == shift) {
					disassemble(fault_pc);
					panic("Abort handler cannot fix this :-(\n");
				}
#ifdef DEBUG_FAULT_CORRECTION
				if (pmap_debug_level >=0)
					printf("shift reg=%d ", shift);
#endif	/* DEBUG_FAULT_CORRECTION */
				shift = registers[shift];
			}
#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >=0)
				printf("shift=%08x ", shift);
#endif	/* DEBUG_FAULT_CORRECTION */
			switch (((fault_instruction >> 5) & 0x3)) {
			case 0 : /* Logical left */
				offset = (int)(((u_int)offset) << shift);
				break;
			case 1 : /* Logical Right */
				if (shift == 0) shift = 32;
				offset = (int)(((u_int)offset) >> shift);
				break;
			case 2 : /* Arithmetic Right */
				if (shift == 0) shift = 32;
				offset = (int)(((int)offset) >> shift);
				break;
			case 3 : /* Rotate right */
				disassemble(fault_pc);
				panic("Abort handler cannot fix this yet :-(\n");
				break;
			}

#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >=0)
				printf("abt: fixed LDR/STR with register offset\n");
#endif	/* DEBUG_FAULT_CORRECTION */               
			if ((fault_instruction & (1 << 23)))
				offset = -offset;
#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >=0)
				printf("offset=%08x ", offset);
#endif	/* DEBUG_FAULT_CORRECTION */
			registers[base] += offset;
		}
#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >=0)
			printf("r%d=%08x\n", base, registers[base]);
#endif	/* DEBUG_FAULT_CORRECTION */
	}
	} else if ((fault_instruction & 0x0e000000) == 0x08000000) {
		int base;
		int loop;
		int count;
		int *registers = &frame->tf_r0;
        
#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >= 0) {
			printf("LDM/STM\n");
			disassemble(fault_pc);
		}
#endif	/* DEBUG_FAULT_CORRECTION */
		if (fault_instruction & (1 << 21)) {
#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >= 0)
				printf("This instruction must be corrected\n");
#endif	/* DEBUG_FAULT_CORRECTION */
			base = (fault_instruction >> 16) & 0x0f;
			if (base == 15) {
				disassemble(fault_pc);
				panic("Abort handler cannot fix this :-(\n");
			}
			/* Count registers transferred */
			count = 0;
			for (loop = 0; loop < 16; ++loop) {
				if (fault_instruction & (1<<loop))
					++count;
			}
#ifdef DEBUG_FAULT_CORRECTION
			if (pmap_debug_level >= 0) {
				printf("%d registers used\n", count);
				printf("Corrected r%d by %d bytes ", base, count * 4);
			}
#endif	/* DEBUG_FAULT_CORRECTION */
			if (fault_instruction & (1 << 23)) {
#ifdef DEBUG_FAULT_CORRECTION
				if (pmap_debug_level >= 0)
					printf("down\n");
#endif	/* DEBUG_FAULT_CORRECTION */
				registers[base] -= count * 4;
			} else {
#ifdef DEBUG_FAULT_CORRECTION
				if (pmap_debug_level >= 0)
					printf("up\n");
#endif	/* DEBUG_FAULT_CORRECTION */
				registers[base] += count * 4;
			}
		}
	} else if ((fault_instruction & 0x0e000000) == 0x0c000000) {
		int base;
		int offset;
		int *registers = &frame->tf_r0;
	
/* REGISTER CORRECTION IS REQUIRED FOR THESE INSTRUCTIONS */

#ifdef DEBUG_FAULT_CORRECTION
		if (pmap_debug_level >= 0)
			disassemble(fault_pc);
#endif	/* DEBUG_FAULT_CORRECTION */

/* Only need to fix registers if write back is turned on */

		if ((fault_instruction & (1 << 21)) != 0) {
			base = (fault_instruction >> 16) & 0x0f;
			if (base == 13 && (frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {
				disassemble(fault_pc);
				panic("Abort handler cannot fix this :-(\n");
			}
			if (base == 15) {
				disassemble(fault_pc);
				panic("Abort handler cannot fix this :-(\n");
			}

			offset = (fault_instruction & 0xff) << 2;
			if (pmap_debug_level >= 0)
				printf("r%d=%08x\n", base, registers[base]);
			if ((fault_instruction & (1 << 23)) != 0)
				offset = -offset;
			registers[base] += offset;
			if (pmap_debug_level >= 0)
				printf("r%d=%08x\n", base, registers[base]);
		}
	} else if ((fault_instruction & 0x0e000000) == 0x0c000000) {
		disassemble(fault_pc);
		panic("How did this happen ...\nWe have faulted on a non data transfer instruction");
	}

	if ((frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {

		/* Ok an abort in SVC mode */

		/*
		 * Copy the SVC r14 into the usr r14 - The usr r14 is garbage
		 * as the fault happened in svc mode but we need it in the
		 * usr slot so we can treat the registers as an array of ints
		 * during fixing.
		 * NOTE: This PC is in the position but writeback is not
		 * allowed on r15.
		 * Doing it like this is more efficient than trapping this
		 * case in all possible locations in the prior fixup code.
		 */

		frame->tf_svc_lr = frame->tf_usr_lr;
		frame->tf_usr_lr = saved_lr;

		/*
		 * Note the trapframe does not have the SVC r13 so a fault
		 * from an instruction with writeback to r13 in SVC mode is
		 * not allowed. This should not happen as the kstack is
		 * always valid.
		 */
	}

	return(0);
}


int
arm7_prefetchabt_fixup(arg)
	void *arg;
{
	return(0);
}
#endif	/* CPU_ARM7 */

#ifdef CPU_SA110
int
sa110_dataabt_fixup(arg)
	void *arg;
{
	return(0);
}

int
sa110_prefetchabt_fixup(arg)
	void *arg;
{
	return(0);
}
#endif	/* CPU_SA110 */
