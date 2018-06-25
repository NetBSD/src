/*	$NetBSD: dtrace_subr.c,v 1.3.10.1 2018/06/25 07:25:14 pgoyette Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *
 * $FreeBSD: head/sys/cddl/dev/dtrace/arm/dtrace_subr.c 308457 2016-11-08 23:59:41Z bdrewery $
 *
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/xcall.h>
#include <sys/cpu.h>
#include <sys/cpuvar.h>
#include <sys/dtrace_impl.h>
#include <sys/dtrace_bsd.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/vmparam.h>
#include <uvm/uvm_pglist.h>
#include <uvm/uvm_prot.h>
#include <uvm/uvm_pmap.h>

#define FAULT_ALIGN	FAULT_ALIGN_0
extern uintptr_t 	kernelbase;
extern uintptr_t 	dtrace_in_probe_addr;
extern int		dtrace_in_probe;

void dtrace_gethrtime_init(void *arg);

#define	DELAYBRANCH(x)	((int)(x) < 0)

#define	BIT_PC		15
#define	BIT_LR		14
#define	BIT_SP		13

extern dtrace_id_t	dtrace_probeid_error;
extern int (*dtrace_invop_jump_addr)(struct trapframe *);
extern void dtrace_getnanotime(struct timespec *tsp);

int dtrace_invop(uintptr_t, struct trapframe *, uintptr_t);
void dtrace_invop_init(void);
void dtrace_invop_uninit(void);

typedef struct dtrace_invop_hdlr {
	int (*dtih_func)(uintptr_t, struct trapframe *, uintptr_t);
	struct dtrace_invop_hdlr *dtih_next;
} dtrace_invop_hdlr_t;

dtrace_invop_hdlr_t *dtrace_invop_hdlr;

int
dtrace_invop(uintptr_t addr, struct trapframe *frame, uintptr_t eax)
{
	dtrace_invop_hdlr_t *hdlr;
	int rval;

	for (hdlr = dtrace_invop_hdlr; hdlr != NULL; hdlr = hdlr->dtih_next)
		if ((rval = hdlr->dtih_func(addr, frame, eax)) != 0)
			return (rval);

	return (0);
}


void
dtrace_invop_add(int (*func)(uintptr_t, struct trapframe *, uintptr_t))
{
	dtrace_invop_hdlr_t *hdlr;

	hdlr = kmem_alloc(sizeof (dtrace_invop_hdlr_t), KM_SLEEP);
	hdlr->dtih_func = func;
	hdlr->dtih_next = dtrace_invop_hdlr;
	dtrace_invop_hdlr = hdlr;
}

void
dtrace_invop_remove(int (*func)(uintptr_t, struct trapframe *, uintptr_t))
{
	dtrace_invop_hdlr_t *hdlr = dtrace_invop_hdlr, *prev = NULL;

	for (;;) {
		if (hdlr == NULL)
			panic("attempt to remove non-existent invop handler");

		if (hdlr->dtih_func == func)
			break;

		prev = hdlr;
		hdlr = hdlr->dtih_next;
	}

	if (prev == NULL) {
		ASSERT(dtrace_invop_hdlr == hdlr);
		dtrace_invop_hdlr = hdlr->dtih_next;
	} else {
		ASSERT(dtrace_invop_hdlr != hdlr);
		prev->dtih_next = hdlr->dtih_next;
	}

	kmem_free(hdlr, sizeof (dtrace_invop_hdlr_t));
}

/*ARGSUSED*/
void
dtrace_toxic_ranges(void (*func)(uintptr_t base, uintptr_t limit))
{
	(*func)(0, kernelbase);
}

static void
xcall_func(void *arg0, void *arg1)
{
    	dtrace_xcall_t func = arg0;

    	(*func)(arg1);
}

void
dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg)
{
	uint64_t where;

	if (cpu == DTRACE_CPUALL) {
		where = xc_broadcast(0, xcall_func, func, arg);
	} else {
		struct cpu_info *cinfo = cpu_lookup(cpu);

		KASSERT(cinfo != NULL);
		where = xc_unicast(0, xcall_func, func, arg, cinfo);
	}
	xc_wait(where);

	/* XXX Q. Do we really need the other cpus to wait also? 
	 * (see solaris:xc_sync())
	 */
}

static void
dtrace_sync_func(void)
{
}

void
dtrace_sync(void)
{
	dtrace_xcall(DTRACE_CPUALL, (dtrace_xcall_t)dtrace_sync_func, NULL);
}

/*
 * DTrace needs a high resolution time function which can
 * be called from a probe context and guaranteed not to have
 * instrumented with probes itself.
 *
 * Returns nanoseconds since boot.
 */
uint64_t
dtrace_gethrtime(void)
{
	struct	timespec curtime;

	nanouptime(&curtime);

	return (curtime.tv_sec * 1000000000UL + curtime.tv_nsec);
}

uint64_t
dtrace_gethrestime(void)
{
	struct timespec current_time;

	dtrace_getnanotime(&current_time);

	return (current_time.tv_sec * 1000000000UL + current_time.tv_nsec);
}

/* Function to handle DTrace traps during probes. Not used on ARM yet */
int
dtrace_trap(struct trapframe *frame, u_int type)
{
	cpuid_t curcpu_id = cpu_number();	/* current cpu id */

	/*
	 * A trap can occur while DTrace executes a probe. Before
	 * executing the probe, DTrace blocks re-scheduling and sets
	 * a flag in its per-cpu flags to indicate that it doesn't
	 * want to fault. On returning from the probe, the no-fault
	 * flag is cleared and finally re-scheduling is enabled.
	 *
	 * Check if DTrace has enabled 'no-fault' mode:
	 *
	 */

	if ((cpu_core[curcpu_id].cpuc_dtrace_flags & CPU_DTRACE_NOFAULT) != 0) {
		/*
		 * There are only a couple of trap types that are expected.
		 * All the rest will be handled in the usual way.
		 */
		switch (type) {
		/* Page fault. */
		case FAULT_ALIGN:
			/* Flag a bad address. */
			cpu_core[curcpu_id].cpuc_dtrace_flags |= CPU_DTRACE_BADADDR;
			cpu_core[curcpu_id].cpuc_dtrace_illval = 0;

			/*
			 * Offset the instruction pointer to the instruction
			 * following the one causing the fault.
			 */
			frame->tf_pc += sizeof(int);
			return (1);
		default:
			/* Handle all other traps in the usual way. */
			break;
		}
	}

	/* Handle the trap in the usual way. */
	return (0);
}

void
dtrace_probe_error(dtrace_state_t *state, dtrace_epid_t epid, int which,
    int fault, int fltoffs, uintptr_t illval)
{

	dtrace_probe(dtrace_probeid_error, (uint64_t)(uintptr_t)state,
	    (uintptr_t)epid,
	    (uintptr_t)which, (uintptr_t)fault, (uintptr_t)fltoffs);
}

void
dtrace_gethrtime_init(void *arg)
{
	/* FIXME */
}

static uint32_t
dtrace_expand_imm(uint32_t imm12)
{
	uint32_t unrot = imm12 & 0xff;
	int amount = 2 * (imm12 >> 8);

	if (amount)
		return (unrot >> amount) | (unrot << (32 - amount));
	else
		return unrot;
}

static uint32_t
dtrace_add_with_carry(uint32_t x, uint32_t y, int carry_in,
	int *carry_out, int *overflow)
{
	uint32_t result;
	uint64_t unsigned_sum = x + y + (uint32_t)carry_in;
	int64_t signed_sum = (int32_t)x + (int32_t)y + (int32_t)carry_in;
	KASSERT(carry_in == 1);

	result = (uint32_t)(unsigned_sum & 0xffffffff);
	*carry_out = ((uint64_t)result == unsigned_sum) ? 1 : 0;
	*overflow = ((int64_t)result == signed_sum) ? 0 : 1;
	
	return result;
}

static void
dtrace_invop_emulate(int invop, struct trapframe *frame)
{
	uint32_t op = invop;
#if 1
	/* nbsd encoding */
	uint32_t code = op >> 28;
	uint32_t data = op;
#else
	/* fbsd encoding */
	uint32_t code = op & DTRACE_INVOP_MASK;
	uint32_t data = DTRACE_INVOP_DATA(invop);
#endif

	switch (code) {
	case DTRACE_INVOP_MOV_IP_SP:
		/* mov ip, sp */
		frame->tf_ip = frame->tf_svc_sp;
		frame->tf_pc += 4;
		break;
	case DTRACE_INVOP_BX_LR:
		/* bx lr */
		frame->tf_pc = frame->tf_svc_lr;
		break;
	case DTRACE_INVOP_MOV_PC_LR:
		/* mov pc, lr */
		frame->tf_pc = frame->tf_svc_lr;
		break;
	case DTRACE_INVOP_LDM:
		/* ldm sp, {..., pc} */
		/* FALLTHRU */
	case DTRACE_INVOP_POPM: {
		/* ldmib sp, {..., pc} */
		uint32_t register_list = (op & 0xffff);
		uint32_t *sp = (uint32_t *)(intptr_t)frame->tf_svc_sp;
		uint32_t *regs = &frame->tf_r0;
		int i;

		/* POPM */
		if (code == DTRACE_INVOP_POPM)
			sp++;

		for (i = 0; i <= 12; i++) {
			if (register_list & (1 << i))
				regs[i] = *sp++;
		}
		if (register_list & (1 << 13))
			frame->tf_svc_sp = *sp++;
		if (register_list & (1 << 14))
			frame->tf_svc_lr = *sp++;
		frame->tf_pc = *sp;
		break;
	}
	case DTRACE_INVOP_LDR_IMM: {
		/* ldr r?, [{pc,r?}, #?] */
		uint32_t rt = (op >> 12) & 0xf;
		uint32_t rn = (op >> 16) & 0xf;
		uint32_t imm = op & 0xfff;
		uint32_t *regs = &frame->tf_r0;
		KDASSERT(rt <= 12);
		KDASSERT(rn == 15 || rn <= 12);
		if (rn == 15)
			regs[rt] = *((uint32_t *)(intptr_t)(frame->tf_pc + 8 + imm));
		else
			regs[rt] = *((uint32_t *)(intptr_t)(regs[rn] + imm));
		frame->tf_pc += 4;
		break;
	}
	case DTRACE_INVOP_MOVW: {
		/* movw r?, #? */
		uint32_t rd = (op >> 12) & 0xf;
		uint32_t imm = (op & 0xfff) | ((op & 0xf0000) >> 4);
		uint32_t *regs = &frame->tf_r0;
		KDASSERT(rd <= 12);
		regs[rd] = imm;
		frame->tf_pc += 4;
		break;
	}
	case DTRACE_INVOP_MOV_IMM: {
		/* mov r?, #? */
		uint32_t rd = (op >> 12) & 0xf;
		uint32_t imm = dtrace_expand_imm(op & 0xfff);
		uint32_t *regs = &frame->tf_r0;
		KDASSERT(rd <= 12);
		regs[rd] = imm;
		frame->tf_pc += 4;
		break;
	}
	case DTRACE_INVOP_CMP_IMM: {
		/* cmp r?, #? */
		uint32_t rn = (op >> 16) & 0xf;
		uint32_t *regs = &frame->tf_r0;
		uint32_t imm = dtrace_expand_imm(op & 0xfff);
		uint32_t spsr = frame->tf_spsr;
		uint32_t result;
		int carry;
		int overflow;
		/*
		 * (result, carry, overflow) = AddWithCarry(R[n], NOT(imm32), ’1’);
		 * APSR.N = result<31>;
		 * APSR.Z = IsZeroBit(result);
		 * APSR.C = carry;
		 * APSR.V = overflow; 
		 */
		KDASSERT(rn <= 12);
		result = dtrace_add_with_carry(regs[rn], ~imm, 1, &carry, &overflow);
		if (result & 0x80000000)
			spsr |= PSR_N_bit;
		else
			spsr &= ~PSR_N_bit;
		if (result == 0)
			spsr |= PSR_Z_bit;
		else
			spsr &= ~PSR_Z_bit;
		if (carry)
			spsr |= PSR_C_bit;
		else
			spsr &= ~PSR_C_bit;
		if (overflow)
			spsr |= PSR_V_bit;
		else
			spsr &= ~PSR_V_bit;

#if 0
		aprint_normal("pc=%x Rn=%x imm=%x %c%c%c%c\n", frame->tf_pc, regs[rn], imm,
		    (spsr & PSR_N_bit) ? 'N' : 'n',
		    (spsr & PSR_Z_bit) ? 'Z' : 'z',
		    (spsr & PSR_C_bit) ? 'C' : 'c',
		    (spsr & PSR_V_bit) ? 'V' : 'v');
#endif
		frame->tf_spsr = spsr;
		frame->tf_pc += 4;
		break;
	}
	case DTRACE_INVOP_B: {
		/* b ??? */
		uint32_t imm = (op & 0x00ffffff) << 2;
		int32_t diff;
		/* SignExtend(imm26, 32) */
		if (imm & 0x02000000)
			imm |= 0xfc000000;
		diff = (int32_t)imm;
		frame->tf_pc += 8 + diff;
		break;
	}
	case DTRACE_INVOP_PUSHM: {
		/* push {...} */
		uint32_t register_list = (op & 0xffff);
		uint32_t *sp = (uint32_t *)(intptr_t)frame->tf_svc_sp;
		uint32_t *regs = &frame->tf_r0;
		int i;
		int count = 0;

#if 0
		if ((op & 0x0fff0fff) == 0x052d0004) {
			/* A2: str r4, [sp, #-4]! */
			*(sp - 1) = regs[4];
			frame->tf_pc += 4;
			break;
		}
#endif

		for (i = 0; i < 16; i++) {
			if (register_list & (1 << i))
				count++;
		}
		sp -= count;

		for (i = 0; i <= 12; i++) {
			if (register_list & (1 << i))
				*sp++ = regs[i];
		}
		if (register_list & (1 << 13))
			*sp++ = frame->tf_svc_sp;
		if (register_list & (1 << 14))
			*sp++ = frame->tf_svc_lr;
		if (register_list & (1 << 15))
			*sp = frame->tf_pc + 8;

		/* make sure the caches and memory are in sync */
		cpu_dcache_wbinv_range(frame->tf_svc_sp, count * 4);

		/* In case the current page tables have been modified ... */
		cpu_tlb_flushID();
		cpu_cpwait();

		frame->tf_svc_sp -= count * 4;
		frame->tf_pc += 4;

		break;
	}
	default:
		KDASSERTMSG(0, "invop 0x%08x code %u tf %p", invop, code, frame);
	}
}

static int
dtrace_invop_start(struct trapframe *frame)
{
#if 0
	register_t *r0, *sp;
	int data, invop, reg, update_sp;
#endif
	int invop;

	invop = dtrace_invop(frame->tf_pc, frame, frame->tf_r0);

	dtrace_invop_emulate(invop, frame);

#if 0
	switch (invop & DTRACE_INVOP_MASK) {
	case DTRACE_INVOP_PUSHM:
		sp = (register_t *)frame->tf_svc_sp;
		r0 = &frame->tf_r0;
		data = DTRACE_INVOP_DATA(invop);

		/*
		 * Store the pc, lr, and sp. These have their own
		 * entries in the struct.
		 */
		if (data & (1 << BIT_PC)) {
			sp--;
			*sp = frame->tf_pc;
		}
		if (data & (1 << BIT_LR)) {
			sp--;
			*sp = frame->tf_svc_lr;
		}
		if (data & (1 << BIT_SP)) {
			sp--;
			*sp = frame->tf_svc_sp;
		}

		/* Store the general registers */
		for (reg = 12; reg >= 0; reg--) {
			if (data & (1 << reg)) {
				sp--;
				*sp = r0[reg];
			}
		}

		/* Update the stack pointer and program counter to continue */
		frame->tf_svc_sp = (register_t)sp;
		frame->tf_pc += 4;
		break;
	case DTRACE_INVOP_POPM:
		sp = (register_t *)frame->tf_svc_sp;
		r0 = &frame->tf_r0;
		data = DTRACE_INVOP_DATA(invop);

		/* Read the general registers */
		for (reg = 0; reg <= 12; reg++) {
			if (data & (1 << reg)) {
				r0[reg] = *sp;
				sp++;
			}
		}

		/*
		 * Set the stack pointer. If we don't update it here we will
		 * need to update it at the end as the instruction would do
		 */
		update_sp = 1;
		if (data & (1 << BIT_SP)) {
			frame->tf_svc_sp = *sp;
			*sp++;
			update_sp = 0;
		}

		/* Update the link register, we need to use the correct copy */
		if (data & (1 << BIT_LR)) {
			frame->tf_svc_lr = *sp;
			*sp++;
		}
		/*
		 * And the program counter. If it's not in the list skip over
		 * it when we return so to not hit this again.
		 */
		if (data & (1 << BIT_PC)) {
			frame->tf_pc = *sp;
			*sp++;
		} else
			frame->tf_pc += 4;

		/* Update the stack pointer if we haven't already done so */
		if (update_sp)
			frame->tf_svc_sp = (register_t)sp;
		break;
	case DTRACE_INVOP_B:
		data = DTRACE_INVOP_DATA(invop) & 0x00ffffff;
		/* Sign extend the data */
		if ((data & (1 << 23)) != 0)
			data |= 0xff000000;
		/* The data is the number of 4-byte words to change the pc */
		data *= 4;
		data += 8;
		frame->tf_pc += data;
		break;

	default:
		return (-1);
		break;
	}
#endif

	return (0);
}

void dtrace_invop_init(void)
{
	dtrace_invop_jump_addr = dtrace_invop_start;
}

void dtrace_invop_uninit(void)
{
	dtrace_invop_jump_addr = 0;
}
