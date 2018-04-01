/*	$NetBSD: fbt.c,v 1.23.2.1 2018/04/01 23:06:51 pgoyette Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 * Portions Copyright 2006-2008 John Birrell jb@freebsd.org
 * Portions Copyright 2010 Darran Hunt darran@NetBSD.org
 *
 * $FreeBSD: src/sys/cddl/dev/fbt/fbt.c,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $
 *
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpuvar.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/ksyms.h>
#include <sys/cpu.h>
#include <sys/kthread.h>
#include <sys/syslimits.h>
#include <sys/linker.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/selinfo.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <sys/unistd.h>

#include <machine/cpu.h>
#if defined(__i386__) || defined(__amd64__)
#include <machine/cpufunc.h>
#include <machine/specialreg.h>
#if 0
#include <x86/cpuvar.h>
#endif
#include <x86/cputypes.h>
#elif __arm__
#include <machine/trap.h>
#include <arm/cpufunc.h>
#include <arm/armreg.h>
#include <arm/frame.h>
#endif

#include <sys/exec_elf.h>

#include <sys/dtrace.h>
#include <sys/dtrace_bsd.h>
#include <sys/kern_ctf.h>
#include <sys/dtrace_impl.h>

mod_ctf_t *modptr;

MALLOC_DEFINE(M_FBT, "fbt", "Function Boundary Tracing");

#if defined(__i386__) || defined(__amd64__)
#define	FBT_PUSHL_EBP		0x55
#define	FBT_MOVL_ESP_EBP0_V0	0x8b
#define	FBT_MOVL_ESP_EBP1_V0	0xec
#define	FBT_MOVL_ESP_EBP0_V1	0x89
#define	FBT_MOVL_ESP_EBP1_V1	0xe5
#define	FBT_REX_RSP_RBP		0x48

#define	FBT_POPL_EBP		0x5d
#define	FBT_RET			0xc3
#define	FBT_RET_IMM16		0xc2
#define	FBT_LEAVE		0xc9
#endif

#ifdef __amd64__
#define	FBT_PATCHVAL		0xcc
#elif defined(__i386__)
#define	FBT_PATCHVAL		0xf0

#elif defined(__arm__)
#define	FBT_PATCHVAL		DTRACE_BREAKPOINT

/* entry and return */
#define	FBT_BX_LR_P(insn)	(((insn) & ~INSN_COND_MASK) == 0x012fff1e)
#define	FBT_B_LABEL_P(insn)	(((insn) & 0xff000000) == 0xea000000)
/* entry */
#define	FBT_MOV_IP_SP_P(insn)	((insn) == 0xe1a0c00d)
/* index=1, add=1, wback=0 */
#define	FBT_LDR_IMM_P(insn)	(((insn) & 0xfff00000) == 0xe5900000)
#define	FBT_MOVW_P(insn)	(((insn) & 0xfff00000) == 0xe3000000)
#define	FBT_MOV_IMM_P(insn)	(((insn) & 0xffff0000) == 0xe3a00000)
#define	FBT_CMP_IMM_P(insn)	(((insn) & 0xfff00000) == 0xe3500000)
#define	FBT_PUSH_P(insn)	(((insn) & 0xffff0000) == 0xe92d0000)
/* return */
/* cond=always, writeback=no, rn=sp and register_list includes pc */
#define	FBT_LDM_P(insn)	(((insn) & 0x0fff8000) == 0x089d8000)
#define	FBT_LDMIB_P(insn)	(((insn) & 0x0fff8000) == 0x099d8000)
#define	FBT_MOV_PC_LR_P(insn)	(((insn) & ~INSN_COND_MASK) == 0x01a0f00e)
/* cond=always, writeback=no, rn=sp and register_list includes lr, but not pc */
#define	FBT_LDM_LR_P(insn)	(((insn) & 0xffffc000) == 0xe89d4000)
#define	FBT_LDMIB_LR_P(insn)	(((insn) & 0xffffc000) == 0xe99d4000)

/* rval = insn | invop_id (overwriting cond with invop ID) */
#define	BUILD_RVAL(insn, id)	(((insn) & ~INSN_COND_MASK) | __SHIFTIN((id), INSN_COND_MASK))
/* encode cond in the first byte */
#define	PATCHVAL_ENCODE_COND(insn)	(FBT_PATCHVAL | __SHIFTOUT((insn), INSN_COND_MASK))

#else
#error "architecture not supported"
#endif

static dev_type_open(fbt_open);
static int	fbt_unload(void);
static void	fbt_getargdesc(void *, dtrace_id_t, void *, dtrace_argdesc_t *);
static void	fbt_provide_module(void *, dtrace_modctl_t *);
static void	fbt_destroy(void *, dtrace_id_t, void *);
static int	fbt_enable(void *, dtrace_id_t, void *);
static void	fbt_disable(void *, dtrace_id_t, void *);
static void	fbt_load(void);
static void	fbt_suspend(void *, dtrace_id_t, void *);
static void	fbt_resume(void *, dtrace_id_t, void *);

#define	FBT_ENTRY	"entry"
#define	FBT_RETURN	"return"
#define	FBT_ADDR2NDX(addr)	((((uintptr_t)(addr)) >> 4) & fbt_probetab_mask)
#define	FBT_PROBETAB_SIZE	0x8000		/* 32k entries -- 128K total */

static const struct cdevsw fbt_cdevsw = {
	.d_open		= fbt_open,
	.d_close	= noclose,
	.d_read		= noread,
	.d_write	= nowrite,
	.d_ioctl	= noioctl,
	.d_stop		= nostop,
	.d_tty		= notty,
	.d_poll		= nopoll,
	.d_mmap		= nommap,
	.d_kqfilter	= nokqfilter,
	.d_discard	= nodiscard,
	.d_flag		= D_OTHER
};

static dtrace_pattr_t fbt_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
};

static dtrace_pops_t fbt_pops = {
	NULL,
	fbt_provide_module,
	fbt_enable,
	fbt_disable,
	fbt_suspend,
	fbt_resume,
	fbt_getargdesc,
	NULL,
	NULL,
	fbt_destroy
};

typedef struct fbt_probe {
	struct fbt_probe *fbtp_hashnext;
#if defined(__i386__) || defined(__amd64__)
	uint8_t		*fbtp_patchpoint;
	int8_t		fbtp_rval;
	uint8_t		fbtp_patchval;
	uint8_t		fbtp_savedval;
#elif __arm__
	uint32_t	*fbtp_patchpoint;
	int32_t		fbtp_rval;
	uint32_t	fbtp_patchval;
	uint32_t	fbtp_savedval;
#endif
	uintptr_t	fbtp_roffset;
	dtrace_id_t	fbtp_id;
	const char	*fbtp_name;
	dtrace_modctl_t	*fbtp_ctl;
	int		fbtp_loadcnt;
	int		fbtp_primary;
	int		fbtp_invop_cnt;
	int		fbtp_symindx;
	struct fbt_probe *fbtp_next;
} fbt_probe_t;

#ifdef notyet
static struct cdev		*fbt_cdev;
static int			fbt_verbose = 0;
#endif
static dtrace_provider_id_t	fbt_id;
static fbt_probe_t		**fbt_probetab;
static int			fbt_probetab_size;
static int			fbt_probetab_mask;

#ifdef __arm__
extern void (* dtrace_emulation_jump_addr)(int, struct trapframe *);

static uint32_t
expand_imm(uint32_t imm12)
{
	uint32_t unrot = imm12 & 0xff;
	int amount = 2 * (imm12 >> 8);

	if (amount)
		return (unrot >> amount) | (unrot << (32 - amount));
	else
		return unrot;
}

static uint32_t
add_with_carry(uint32_t x, uint32_t y, int carry_in,
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
fbt_emulate(int _op, struct trapframe *frame)
{
	uint32_t op = _op;

	switch (op >> 28) {
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
	case DTRACE_INVOP_LDMIB: {
		/* ldmib sp, {..., pc} */
		uint32_t register_list = (op & 0xffff);
		uint32_t *sp = (uint32_t *)(intptr_t)frame->tf_svc_sp;
		uint32_t *regs = &frame->tf_r0;
		int i;

		/* IDMIB */
		if ((op >> 28) == 5)
			sp++;

		for (i=0; i <= 12; i++) {
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
		KDASSERT(rn == 15 || rn =< 12);
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
		uint32_t imm = expand_imm(op & 0xfff);
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
		uint32_t imm = expand_imm(op & 0xfff);
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
		result = add_with_carry(regs[rn], ~imm, 1, &carry, &overflow);
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
	case DTRACE_INVOP_B_LABEL: {
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
	/* FIXME: push will overwrite trapframe... */
	case DTRACE_INVOP_PUSH: {
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

		for (i=0; i < 16; i++) {
			if (register_list & (1 << i))
				count++;
		}
		sp -= count;

		for (i=0; i <= 12; i++) {
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
		KDASSERTMSG(0, "op=%u\n", op >> 28);
	}
}
#endif

static void
fbt_doubletrap(void)
{
	fbt_probe_t *fbt;
	int i;

	for (i = 0; i < fbt_probetab_size; i++) {
		fbt = fbt_probetab[i];

		for (; fbt != NULL; fbt = fbt->fbtp_next)
			*fbt->fbtp_patchpoint = fbt->fbtp_savedval;
	}
}


static int
fbt_invop(uintptr_t addr, struct trapframe *frame, uintptr_t rval)
{
	solaris_cpu_t *cpu;
	uintptr_t *stack;
	uintptr_t arg0, arg1, arg2, arg3, arg4;
	fbt_probe_t *fbt;

#ifdef __amd64__
	stack = (uintptr_t *)frame->tf_rsp;
#endif
#ifdef __i386__
	/* Skip hardware-saved registers. */
	stack = (uintptr_t *)&frame->tf_esp;
#endif
#ifdef __arm__
	stack = (uintptr_t *)frame->tf_svc_sp;
#endif

	cpu = &solaris_cpu[cpu_number()];
	fbt = fbt_probetab[FBT_ADDR2NDX(addr)];
	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
		if ((uintptr_t)fbt->fbtp_patchpoint == addr) {
			fbt->fbtp_invop_cnt++;
			if (fbt->fbtp_roffset == 0) {
#ifdef __amd64__
				/* fbt->fbtp_rval == DTRACE_INVOP_PUSHQ_RBP */
				DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
				cpu->cpu_dtrace_caller = stack[0];
				DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT |
				    CPU_DTRACE_BADADDR);

				arg0 = frame->tf_rdi;
				arg1 = frame->tf_rsi;
				arg2 = frame->tf_rdx;
				arg3 = frame->tf_rcx;
				arg4 = frame->tf_r8;
#else
				int i = 0;

				/*
				 * When accessing the arguments on the stack,
				 * we must protect against accessing beyond
				 * the stack.  We can safely set NOFAULT here
				 * -- we know that interrupts are already
				 * disabled.
				 */
				DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
				cpu->cpu_dtrace_caller = stack[i++];
				arg0 = stack[i++];
				arg1 = stack[i++];
				arg2 = stack[i++];
				arg3 = stack[i++];
				arg4 = stack[i++];
				DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT |
				    CPU_DTRACE_BADADDR);
#endif

				dtrace_probe(fbt->fbtp_id, arg0, arg1,
				    arg2, arg3, arg4);

				cpu->cpu_dtrace_caller = 0;
			} else {
#ifdef __amd64__
				/*
				 * On amd64, we instrument the ret, not the
				 * leave.  We therefore need to set the caller
				 * to ensure that the top frame of a stack()
				 * action is correct.
				 */
				DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
				cpu->cpu_dtrace_caller = stack[0];
				DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT |
				    CPU_DTRACE_BADADDR);
#endif

				dtrace_probe(fbt->fbtp_id, fbt->fbtp_roffset,
				    rval, 0, 0, 0);
				cpu->cpu_dtrace_caller = 0;
			}

			return (fbt->fbtp_rval);
		}
	}

	return (0);
}

#if defined(__i386__) || defined(__amd64__)
static int
fbt_provide_module_cb(const char *name, int symindx, void *value,
	uint32_t symsize, int type, void *opaque)
{
	fbt_probe_t *fbt, *retfbt;
	u_int8_t *instr, *limit;
	dtrace_modctl_t *mod = opaque;
	const char *modname = mod->mod_info->mi_name;
	int j;
	int size;

	/* got a function? */
	if (ELF_ST_TYPE(type) != STT_FUNC) {
	    return 0;
	}

	if (strncmp(name, "dtrace_", 7) == 0 &&
	    strncmp(name, "dtrace_safe_", 12) != 0) {
		/*
		 * Anything beginning with "dtrace_" may be called
		 * from probe context unless it explicitly indicates
		 * that it won't be called from probe context by
		 * using the prefix "dtrace_safe_".
		 */
		return (0);
	}

	if (name[0] == '_' && name[1] == '_')
		return (0);

	/*
	 * Exclude some more symbols which can be called from probe context.
	 */
	if (strcmp(name, "x86_curcpu") == 0 /* CPU */
	    || strcmp(name, "x86_curlwp") == 0 /* curproc, curlwp, curthread */
	    || strcmp(name, "cpu_index") == 0 /* cpu_number, curcpu_id */
	    || strncmp(name, "db_", 3) == 0 /* debugger */
	    || strncmp(name, "ddb_", 4) == 0 /* debugger */
	    || strncmp(name, "kdb_", 4) == 0 /* debugger */
	    || strncmp(name, "lockdebug_", 10) == 0 /* lockdebug XXX for now */
	    || strncmp(name, "kauth_", 5) == 0 /* CRED XXX for now */
	    ) {
		return 0;
	}

	instr = (u_int8_t *) value;
	limit = (u_int8_t *) value + symsize;

#ifdef __amd64__
	while (instr < limit) {
		if (*instr == FBT_PUSHL_EBP)
			break;

		if ((size = dtrace_instr_size(instr)) <= 0)
			break;

		instr += size;
	}

	if (instr >= limit || *instr != FBT_PUSHL_EBP) {
		/*
		 * We either don't save the frame pointer in this
		 * function, or we ran into some disassembly
		 * screw-up.  Either way, we bail.
		 */
		return (0);
	}
#else
	if (instr[0] != FBT_PUSHL_EBP) {
		return (0);
	}

	if (!(instr[1] == FBT_MOVL_ESP_EBP0_V0 &&
	    instr[2] == FBT_MOVL_ESP_EBP1_V0) &&
	    !(instr[1] == FBT_MOVL_ESP_EBP0_V1 &&
	    instr[2] == FBT_MOVL_ESP_EBP1_V1)) {
		return (0);
	}
#endif
	fbt = malloc(sizeof (fbt_probe_t), M_FBT, M_WAITOK | M_ZERO);
	fbt->fbtp_name = name;
	fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
	    name, FBT_ENTRY, 3, fbt);
	fbt->fbtp_patchpoint = instr;
	fbt->fbtp_ctl = mod;
	/* fbt->fbtp_loadcnt = lf->loadcnt; */
	fbt->fbtp_rval = DTRACE_INVOP_PUSHL_EBP;
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_patchval = FBT_PATCHVAL;
	fbt->fbtp_symindx = symindx;

	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;
	mod->mod_fbtentries++;

	retfbt = NULL;

	while (instr < limit) {
		if (instr >= limit)
			return (0);

		/*
		 * If this disassembly fails, then we've likely walked off into
		 * a jump table or some other unsuitable area.  Bail out of the
		 * disassembly now.
		 */
		if ((size = dtrace_instr_size(instr)) <= 0)
			return (0);

#ifdef __amd64__
		/*
		 * We only instrument "ret" on amd64 -- we don't yet instrument
		 * ret imm16, largely because the compiler doesn't seem to
		 * (yet) emit them in the kernel...
		 */
		if (*instr != FBT_RET) {
			instr += size;
			continue;
		}
#else
		if (!(size == 1 &&
		    (*instr == FBT_POPL_EBP || *instr == FBT_LEAVE) &&
		    (*(instr + 1) == FBT_RET ||
		    *(instr + 1) == FBT_RET_IMM16))) {
			instr += size;
			continue;
		}
#endif

		/*
		 * We (desperately) want to avoid erroneously instrumenting a
		 * jump table, especially given that our markers are pretty
		 * short:  two bytes on x86, and just one byte on amd64.  To
		 * determine if we're looking at a true instruction sequence
		 * or an inline jump table that happens to contain the same
		 * byte sequences, we resort to some heuristic sleeze:  we
		 * treat this instruction as being contained within a pointer,
		 * and see if that pointer points to within the body of the
		 * function.  If it does, we refuse to instrument it.
		 */
		for (j = 0; j < sizeof (uintptr_t); j++) {
			caddr_t check = (caddr_t) instr - j;
			uint8_t *ptr;

			if (check < (caddr_t)value)
				break;

			if (check + sizeof (caddr_t) > (caddr_t)limit)
				continue;

			ptr = *(uint8_t **)check;

			if (ptr >= (uint8_t *) value && ptr < limit) {
				instr += size;
				continue;
			}
		}

		/*
		 * We have a winner!
		 */
		fbt = malloc(sizeof (fbt_probe_t), M_FBT, M_WAITOK | M_ZERO);
		fbt->fbtp_name = name;

		if (retfbt == NULL) {
			fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
			    name, FBT_RETURN, 3, fbt);
		} else {
			retfbt->fbtp_next = fbt;
			fbt->fbtp_id = retfbt->fbtp_id;
		}

		retfbt = fbt;
		fbt->fbtp_patchpoint = instr;
		fbt->fbtp_ctl = mod;
		/* fbt->fbtp_loadcnt = lf->loadcnt; */
		fbt->fbtp_symindx = symindx;

#ifndef __amd64__
		if (*instr == FBT_POPL_EBP) {
			fbt->fbtp_rval = DTRACE_INVOP_POPL_EBP;
		} else {
			ASSERT(*instr == FBT_LEAVE);
			fbt->fbtp_rval = DTRACE_INVOP_LEAVE;
		}
		fbt->fbtp_roffset =
		    (uintptr_t)(instr - (uint8_t *) value) + 1;

#else
		ASSERT(*instr == FBT_RET);
		fbt->fbtp_rval = DTRACE_INVOP_RET;
		fbt->fbtp_roffset =
		    (uintptr_t)(instr - (uint8_t *) value);
#endif

		fbt->fbtp_savedval = *instr;
		fbt->fbtp_patchval = FBT_PATCHVAL;
		fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
		fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

		mod->mod_fbtentries++;

		instr += size;
	}

	return 0;
}

#elif defined(__arm__)

static int
fbt_provide_module_cb(const char *name, int symindx, void *value,
	uint32_t symsize, int type, void *opaque)
{
	fbt_probe_t *fbt, *retfbt;
	uint32_t *instr, *limit;
	bool was_ldm_lr = false;
	dtrace_modctl_t *mod = opaque;
	const char *modname = mod->mod_info->mi_name;
	int size;

	/* got a function? */
	if (ELF_ST_TYPE(type) != STT_FUNC) {
	    return 0;
	}

	if (strncmp(name, "dtrace_", 7) == 0 &&
	    strncmp(name, "dtrace_safe_", 12) != 0) {
		/*
		 * Anything beginning with "dtrace_" may be called
		 * from probe context unless it explicitly indicates
		 * that it won't be called from probe context by
		 * using the prefix "dtrace_safe_".
		 */
		return (0);
	}

	if (name[0] == '_' && name[1] == '_')
		return (0);

	/*
	 * Exclude some more symbols which can be called from probe context.
	 */
	if (strncmp(name, "db_", 3) == 0 /* debugger */
	    || strncmp(name, "ddb_", 4) == 0 /* debugger */
	    || strncmp(name, "kdb_", 4) == 0 /* debugger */
	    || strncmp(name, "lockdebug_", 10) == 0 /* lockdebug XXX for now */
	    || strncmp(name, "kauth_", 5) == 0 /* CRED XXX for now */
	    /* Sensitive functions on ARM */
	    || strncmp(name, "_spl", 4) == 0
	    || strcmp(name, "binuptime") == 0
	    || strcmp(name, "dosoftints") == 0
	    || strcmp(name, "fbt_emulate") == 0
	    || strcmp(name, "nanouptime") == 0
	    || strcmp(name, "undefinedinstruction") == 0
	    || strncmp(name, "dmt_", 4) == 0 /* omap */
	    || strncmp(name, "mvsoctmr_", 9) == 0 /* marvell */
	    ) {
		return 0;
	}

	instr = (uint32_t *) value;
	limit = (uint32_t *)((uintptr_t)value + symsize);

	if (!FBT_MOV_IP_SP_P(*instr)
	    && !FBT_BX_LR_P(*instr)
	    && !FBT_MOVW_P(*instr)
	    && !FBT_MOV_IMM_P(*instr)
	    && !FBT_B_LABEL_P(*instr)
	    && !FBT_LDR_IMM_P(*instr)
	    && !FBT_CMP_IMM_P(*instr)
	    /* && !FBT_PUSH_P(*instr) */
	    ) {
		return 0;
	}

	fbt = malloc(sizeof (fbt_probe_t), M_FBT, M_WAITOK | M_ZERO);
	fbt->fbtp_name = name;
	fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
	    name, FBT_ENTRY, 3, fbt);
	fbt->fbtp_patchpoint = instr;
	fbt->fbtp_ctl = mod;
	/* fbt->fbtp_loadcnt = lf->loadcnt; */
	if (FBT_MOV_IP_SP_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_MOV_IP_SP);
	else if (FBT_LDR_IMM_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_LDR_IMM);
	else if (FBT_MOVW_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_MOVW);
	else if (FBT_MOV_IMM_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_MOV_IMM);
	else if (FBT_CMP_IMM_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_CMP_IMM);
	else if (FBT_BX_LR_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_BX_LR);
	else if (FBT_PUSH_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_PUSH);
	else if (FBT_B_LABEL_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_B_LABEL);

	fbt->fbtp_patchval = PATCHVAL_ENCODE_COND(*instr);
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_symindx = symindx;

	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;
	mod->mod_fbtentries++;

	retfbt = NULL;

	while (instr < limit) {
		if (instr >= limit)
			return (0);

		size = 1;

		if (!FBT_BX_LR_P(*instr)
		    && !FBT_MOV_PC_LR_P(*instr)
		    && !FBT_LDM_P(*instr)
		    && !FBT_LDMIB_P(*instr)
		    && !(was_ldm_lr && FBT_B_LABEL_P(*instr))
		    ) {
			if (FBT_LDM_LR_P(*instr) || FBT_LDMIB_LR_P(*instr))
				was_ldm_lr = true;
			else
				was_ldm_lr = false;
			instr += size;
			continue;
		}

		/*
		 * We have a winner!
		 */
		fbt = malloc(sizeof (fbt_probe_t), M_FBT, M_WAITOK | M_ZERO);
		fbt->fbtp_name = name;

		if (retfbt == NULL) {
			fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
			    name, FBT_RETURN, 3, fbt);
		} else {
			retfbt->fbtp_next = fbt;
			fbt->fbtp_id = retfbt->fbtp_id;
		}

		retfbt = fbt;
		fbt->fbtp_patchpoint = instr;
		fbt->fbtp_ctl = mod;
		/* fbt->fbtp_loadcnt = lf->loadcnt; */
		fbt->fbtp_symindx = symindx;

		if (FBT_BX_LR_P(*instr))
			fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_BX_LR);
		else if (FBT_MOV_PC_LR_P(*instr))
			fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_MOV_PC_LR);
		else if (FBT_LDM_P(*instr))
			fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_LDM);
		else if (FBT_LDMIB_P(*instr))
			fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_LDMIB);
		else if (FBT_B_LABEL_P(*instr))
			fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_B_LABEL);

		fbt->fbtp_roffset = (uintptr_t)(instr - (uint32_t *) value);
		fbt->fbtp_patchval = PATCHVAL_ENCODE_COND(*instr);

		fbt->fbtp_savedval = *instr;
		fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
		fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

		mod->mod_fbtentries++;

		instr += size;
		was_ldm_lr = false;
	}

	return 0;
}
#else
#error "architecture not supported"
#endif


static void
fbt_provide_module(void *arg, dtrace_modctl_t *mod)
{
	char modname[MAXPATHLEN];
	int i;
	size_t len;

	strlcpy(modname, mod->mod_info->mi_name, sizeof(modname));
	len = strlen(modname);
	if (len > 5 && strcmp(modname + len - 3, ".kmod") == 0)
		modname[len - 4] = '\0';

	/*
	 * Employees of dtrace and their families are ineligible.  Void
	 * where prohibited.
	 */
	if (strcmp(modname, "dtrace") == 0)
		return;

	/*
	 * The cyclic timer subsystem can be built as a module and DTrace
	 * depends on that, so it is ineligible too.
	 */
	if (strcmp(modname, "cyclic") == 0)
		return;

	/*
	 * To register with DTrace, a module must list 'dtrace' as a
	 * dependency in order for the kernel linker to resolve
	 * symbols like dtrace_register(). All modules with such a
	 * dependency are ineligible for FBT tracing.
	 */
	for (i = 0; i < mod->mod_nrequired; i++) {
		if (strncmp((*mod->mod_required)[i]->mod_info->mi_name,
			    "dtrace", 6) == 0)
			return;
	}

	if (mod->mod_fbtentries) {
		/*
		 * This module has some FBT entries allocated; we're afraid
		 * to screw with it.
		 */
		return;
	}

	/*
	 * List the functions in the module and the symbol values.
	 */
	ksyms_mod_foreach(modname, fbt_provide_module_cb, mod);
}

static void
fbt_destroy(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg, *next, *hash, *last;
	dtrace_modctl_t *ctl;
	int ndx;

	do {
		ctl = fbt->fbtp_ctl;

		ctl->mod_fbtentries--;

		/*
		 * Now we need to remove this probe from the fbt_probetab.
		 */
		ndx = FBT_ADDR2NDX(fbt->fbtp_patchpoint);
		last = NULL;
		hash = fbt_probetab[ndx];

		while (hash != fbt) {
			ASSERT(hash != NULL);
			last = hash;
			hash = hash->fbtp_hashnext;
		}

		if (last != NULL) {
			last->fbtp_hashnext = fbt->fbtp_hashnext;
		} else {
			fbt_probetab[ndx] = fbt->fbtp_hashnext;
		}

		next = fbt->fbtp_next;
		free(fbt, M_FBT);

		fbt = next;
	} while (fbt != NULL);
}

#if defined(__i386__) || defined(__amd64__)

static int
fbt_enable(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
#if 0
	dtrace_modctl_t *ctl = fbt->fbtp_ctl;
#endif
	u_long psl;
	u_long cr0;


#if 0	/* XXX TBD */
	ctl->nenabled++;

	/*
	 * Now check that our modctl has the expected load count.  If it
	 * doesn't, this module must have been unloaded and reloaded -- and
	 * we're not going to touch it.
	 */
	if (ctl->loadcnt != fbt->fbtp_loadcnt) {
		if (fbt_verbose) {
			printf("fbt is failing for probe %s "
			    "(module %s reloaded)",
			    fbt->fbtp_name, ctl->filename);
		}

		return;
	}
#endif

	/* Disable interrupts. */
	psl = x86_read_psl();
	x86_disable_intr();

	/* Disable write protection in supervisor mode. */
	cr0 = rcr0();
	lcr0(cr0 & ~CR0_WP);

	for (; fbt != NULL; fbt = fbt->fbtp_next) {
		*fbt->fbtp_patchpoint = fbt->fbtp_patchval;
	}

	/* Write back and invalidate cache, flush pipelines. */
	wbinvd();
	x86_flush();
	x86_write_psl(psl);

	/* Re-enable write protection. */
	lcr0(cr0);

	return 0;
}

static void
fbt_disable(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
#if 0
	dtrace_modctl_t *ctl = fbt->fbtp_ctl;
#endif
	u_long psl;
	u_long cr0;

#if 0	/* XXX TBD */
	ASSERT(ctl->nenabled > 0);
	ctl->nenabled--;

	if ((ctl->loadcnt != fbt->fbtp_loadcnt))
		return;
#endif
	/* Disable interrupts. */
	psl = x86_read_psl();
	x86_disable_intr();

	/* Disable write protection in supervisor mode. */
	cr0 = rcr0();
	lcr0(cr0 & ~CR0_WP);

	for (; fbt != NULL; fbt = fbt->fbtp_next)
		*fbt->fbtp_patchpoint = fbt->fbtp_savedval;

	/* Write back and invalidate cache, flush pipelines. */
	wbinvd();
	x86_flush();
	x86_write_psl(psl);

	/* Re-enable write protection. */
	lcr0(cr0);
}

static void
fbt_suspend(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
#if 0
	dtrace_modctl_t *ctl = fbt->fbtp_ctl;
#endif
	u_long psl;
	u_long cr0;

#if 0	/* XXX TBD */
	ASSERT(ctl->nenabled > 0);

	if ((ctl->loadcnt != fbt->fbtp_loadcnt))
		return;
#endif

	/* Disable interrupts. */
	psl = x86_read_psl();
	x86_disable_intr();

	/* Disable write protection in supervisor mode. */
	cr0 = rcr0();
	lcr0(cr0 & ~CR0_WP);

	for (; fbt != NULL; fbt = fbt->fbtp_next)
		*fbt->fbtp_patchpoint = fbt->fbtp_savedval;

	/* Write back and invalidate cache, flush pipelines. */
	wbinvd();
	x86_flush();
	x86_write_psl(psl);

	/* Re-enable write protection. */
	lcr0(cr0);
}

static void
fbt_resume(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
#if 0
	dtrace_modctl_t *ctl = fbt->fbtp_ctl;
#endif
	u_long psl;
	u_long cr0;

#if 0	/* XXX TBD */
	ASSERT(ctl->nenabled > 0);

	if ((ctl->loadcnt != fbt->fbtp_loadcnt))
		return;
#endif
	/* Disable interrupts. */
	psl = x86_read_psl();
	x86_disable_intr();

	/* Disable write protection in supervisor mode. */
	cr0 = rcr0();
	lcr0(cr0 & ~CR0_WP);

	for (; fbt != NULL; fbt = fbt->fbtp_next)
		*fbt->fbtp_patchpoint = fbt->fbtp_patchval;

	/* Write back and invalidate cache, flush pipelines. */
	wbinvd();
	x86_flush();
	x86_write_psl(psl);

	/* Re-enable write protection. */
	lcr0(cr0);
}

#elif defined(__arm__)

static int
fbt_enable(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
#if 0
	dtrace_modctl_t *ctl = fbt->fbtp_ctl;
#endif
	dtrace_icookie_t c;


#if 0	/* XXX TBD */
	ctl->nenabled++;

	/*
	 * Now check that our modctl has the expected load count.  If it
	 * doesn't, this module must have been unloaded and reloaded -- and
	 * we're not going to touch it.
	 */
	if (ctl->loadcnt != fbt->fbtp_loadcnt) {
		if (fbt_verbose) {
			printf("fbt is failing for probe %s "
			    "(module %s reloaded)",
			    fbt->fbtp_name, ctl->filename);
		}

		return;
	}
#endif

	c = dtrace_interrupt_disable();

	for (fbt = parg; fbt != NULL; fbt = fbt->fbtp_next) {
		*fbt->fbtp_patchpoint = fbt->fbtp_patchval;
		cpu_idcache_wbinv_range((vaddr_t)fbt->fbtp_patchpoint, 4);
	}

	dtrace_interrupt_enable(c);

	return 0;
}

static void
fbt_disable(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
#if 0
	dtrace_modctl_t *ctl = fbt->fbtp_ctl;
#endif
	dtrace_icookie_t c;

#if 0	/* XXX TBD */
	ASSERT(ctl->nenabled > 0);
	ctl->nenabled--;

	if ((ctl->loadcnt != fbt->fbtp_loadcnt))
		return;
#endif

	c = dtrace_interrupt_disable();

	for (; fbt != NULL; fbt = fbt->fbtp_next) {
		*fbt->fbtp_patchpoint = fbt->fbtp_savedval;
		cpu_idcache_wbinv_range((vaddr_t)fbt->fbtp_patchpoint, 4);
	}

	dtrace_interrupt_enable(c);
}

static void
fbt_suspend(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
#if 0
	dtrace_modctl_t *ctl = fbt->fbtp_ctl;
#endif
	dtrace_icookie_t c;

#if 0	/* XXX TBD */
	ASSERT(ctl->nenabled > 0);

	if ((ctl->loadcnt != fbt->fbtp_loadcnt))
		return;
#endif

	c = dtrace_interrupt_disable();

	for (; fbt != NULL; fbt = fbt->fbtp_next) {
		*fbt->fbtp_patchpoint = fbt->fbtp_savedval;
		cpu_idcache_wbinv_range((vaddr_t)fbt->fbtp_patchpoint, 4);
	}

	dtrace_interrupt_enable(c);
}

static void
fbt_resume(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
#if 0
	dtrace_modctl_t *ctl = fbt->fbtp_ctl;
#endif
	dtrace_icookie_t c;

#if 0	/* XXX TBD */
	ASSERT(ctl->nenabled > 0);

	if ((ctl->loadcnt != fbt->fbtp_loadcnt))
		return;
#endif

	c = dtrace_interrupt_disable();

	for (; fbt != NULL; fbt = fbt->fbtp_next) {
		*fbt->fbtp_patchpoint = fbt->fbtp_patchval;
		cpu_idcache_wbinv_range((vaddr_t)fbt->fbtp_patchpoint, 4);
	}

	dtrace_interrupt_enable(c);
}

#else
#error "architecture not supported"
#endif

static int
fbt_ctfoff_init(dtrace_modctl_t *mod, mod_ctf_t *mc)
{
	const Elf_Sym *symp = mc->symtab;
	const ctf_header_t *hp = (const ctf_header_t *) mc->ctftab;
	const uint8_t *ctfdata = mc->ctftab + sizeof(ctf_header_t);
	int i;
	uint32_t *ctfoff;
	uint32_t objtoff = hp->cth_objtoff;
	uint32_t funcoff = hp->cth_funcoff;
	ushort_t info;
	ushort_t vlen;
	int nsyms = (mc->nmap != NULL) ? mc->nmapsize : mc->nsym;

	/* Sanity check. */
	if (hp->cth_magic != CTF_MAGIC) {
		printf("Bad magic value in CTF data of '%s'\n",
			mod->mod_info->mi_name);
		return (EINVAL);
	}

	if (mc->symtab == NULL) {
		printf("No symbol table in '%s'\n",
			mod->mod_info->mi_name);
		return (EINVAL);
	}

	if ((ctfoff = malloc(sizeof(uint32_t) * nsyms, M_FBT, M_WAITOK)) == NULL)
		return (ENOMEM);

	mc->ctfoffp = ctfoff;

	for (i = 0; i < nsyms; i++, ctfoff++, symp++) {
	   	if (mc->nmap != NULL) {
			if (mc->nmap[i] == 0) {
				printf("%s.%d: Error! Got zero nmap!\n",
					__func__, __LINE__);
				continue;
			}

			/* CTF expects the pre-sorted symbol ordering, 
			 * so map it from that to the current sorted
			 * and trimmed symbol table.
			 * ctfoff[new-ind] = oldind symbol info.
			 */

			/* map old index to new symbol table */
			symp = &mc->symtab[mc->nmap[i] - 1];

			/* map old index to new ctfoff index */
			ctfoff = &mc->ctfoffp[mc->nmap[i]-1];
		}

		if (symp->st_name == 0 || symp->st_shndx == SHN_UNDEF) {
			*ctfoff = 0xffffffff;
			continue;
		}

		switch (ELF_ST_TYPE(symp->st_info)) {
		case STT_OBJECT:
			if (objtoff >= hp->cth_funcoff ||
                            (symp->st_shndx == SHN_ABS && symp->st_value == 0)) {
				*ctfoff = 0xffffffff;
                                break;
                        }

                        *ctfoff = objtoff;
                        objtoff += sizeof (ushort_t);
			break;

		case STT_FUNC:
			if (funcoff >= hp->cth_typeoff) {
				*ctfoff = 0xffffffff;
				break;
			}

			*ctfoff = funcoff;

			info = *((const ushort_t *)(ctfdata + funcoff));
			vlen = CTF_INFO_VLEN(info);

			/*
			 * If we encounter a zero pad at the end, just skip it.
			 * Otherwise skip over the function and its return type
			 * (+2) and the argument list (vlen).
			 */
			if (CTF_INFO_KIND(info) == CTF_K_UNKNOWN && vlen == 0)
				funcoff += sizeof (ushort_t); /* skip pad */
			else
				funcoff += sizeof (ushort_t) * (vlen + 2);
			break;

		default:
			*ctfoff = 0xffffffff;
			break;
		}
	}

	return (0);
}

static ssize_t
fbt_get_ctt_size(uint8_t xversion, const ctf_type_t *tp, ssize_t *sizep,
    ssize_t *incrementp)
{
	ssize_t size, increment;

	if (xversion > CTF_VERSION_1 &&
	    tp->ctt_size == CTF_LSIZE_SENT) {
		size = CTF_TYPE_LSIZE(tp);
		increment = sizeof (ctf_type_t);
	} else {
		size = tp->ctt_size;
		increment = sizeof (ctf_stype_t);
	}

	if (sizep)
		*sizep = size;
	if (incrementp)
		*incrementp = increment;

	return (size);
}

static int
fbt_typoff_init(mod_ctf_t *mc)
{
	const ctf_header_t *hp = (const ctf_header_t *) mc->ctftab;
	const ctf_type_t *tbuf;
	const ctf_type_t *tend;
	const ctf_type_t *tp;
	const uint8_t *ctfdata = mc->ctftab + sizeof(ctf_header_t);
	int ctf_typemax = 0;
	uint32_t *xp;
	ulong_t pop[CTF_K_MAX + 1] = { 0 };

	/* Sanity check. */
	if (hp->cth_magic != CTF_MAGIC)
		return (EINVAL);

	tbuf = (const ctf_type_t *) (ctfdata + hp->cth_typeoff);
	tend = (const ctf_type_t *) (ctfdata + hp->cth_stroff);

	int child = hp->cth_parname != 0;

	/*
	 * We make two passes through the entire type section.  In this first
	 * pass, we count the number of each type and the total number of types.
	 */
	for (tp = tbuf; tp < tend; ctf_typemax++) {
		ushort_t kind = CTF_INFO_KIND(tp->ctt_info);
		ulong_t vlen = CTF_INFO_VLEN(tp->ctt_info);
		ssize_t size, increment;

		size_t vbytes;
		uint_t n;

		(void) fbt_get_ctt_size(hp->cth_version, tp, &size, &increment);

		switch (kind) {
		case CTF_K_INTEGER:
		case CTF_K_FLOAT:
			vbytes = sizeof (uint_t);
			break;
		case CTF_K_ARRAY:
			vbytes = sizeof (ctf_array_t);
			break;
		case CTF_K_FUNCTION:
			vbytes = sizeof (ushort_t) * (vlen + (vlen & 1));
			break;
		case CTF_K_STRUCT:
		case CTF_K_UNION:
			if (size < CTF_LSTRUCT_THRESH) {
				ctf_member_t *mp = (ctf_member_t *)
				    ((uintptr_t)tp + increment);

				vbytes = sizeof (ctf_member_t) * vlen;
				for (n = vlen; n != 0; n--, mp++)
					child |= CTF_TYPE_ISCHILD(mp->ctm_type);
			} else {
				ctf_lmember_t *lmp = (ctf_lmember_t *)
				    ((uintptr_t)tp + increment);

				vbytes = sizeof (ctf_lmember_t) * vlen;
				for (n = vlen; n != 0; n--, lmp++)
					child |=
					    CTF_TYPE_ISCHILD(lmp->ctlm_type);
			}
			break;
		case CTF_K_ENUM:
			vbytes = sizeof (ctf_enum_t) * vlen;
			break;
		case CTF_K_FORWARD:
			/*
			 * For forward declarations, ctt_type is the CTF_K_*
			 * kind for the tag, so bump that population count too.
			 * If ctt_type is unknown, treat the tag as a struct.
			 */
			if (tp->ctt_type == CTF_K_UNKNOWN ||
			    tp->ctt_type >= CTF_K_MAX)
				pop[CTF_K_STRUCT]++;
			else
				pop[tp->ctt_type]++;
			/*FALLTHRU*/
		case CTF_K_UNKNOWN:
			vbytes = 0;
			break;
		case CTF_K_POINTER:
		case CTF_K_TYPEDEF:
		case CTF_K_VOLATILE:
		case CTF_K_CONST:
		case CTF_K_RESTRICT:
			child |= CTF_TYPE_ISCHILD(tp->ctt_type);
			vbytes = 0;
			break;
		default:
			printf("%s(%d): detected invalid CTF kind -- %u\n", __func__, __LINE__, kind);
			return (EIO);
		}
		tp = (ctf_type_t *)((uintptr_t)tp + increment + vbytes);
		pop[kind]++;
	}

	mc->typlen = ctf_typemax;

	if ((xp = malloc(sizeof(uint32_t) * ctf_typemax, M_FBT, M_ZERO | M_WAITOK)) == NULL)
		return (ENOMEM);

	mc->typoffp = xp;

	/* type id 0 is used as a sentinel value */
	*xp++ = 0;

	/*
	 * In the second pass, fill in the type offset.
	 */
	for (tp = tbuf; tp < tend; xp++) {
		ushort_t kind = CTF_INFO_KIND(tp->ctt_info);
		ulong_t vlen = CTF_INFO_VLEN(tp->ctt_info);
		ssize_t size, increment;

		size_t vbytes;
		uint_t n;

		(void) fbt_get_ctt_size(hp->cth_version, tp, &size, &increment);

		switch (kind) {
		case CTF_K_INTEGER:
		case CTF_K_FLOAT:
			vbytes = sizeof (uint_t);
			break;
		case CTF_K_ARRAY:
			vbytes = sizeof (ctf_array_t);
			break;
		case CTF_K_FUNCTION:
			vbytes = sizeof (ushort_t) * (vlen + (vlen & 1));
			break;
		case CTF_K_STRUCT:
		case CTF_K_UNION:
			if (size < CTF_LSTRUCT_THRESH) {
				ctf_member_t *mp = (ctf_member_t *)
				    ((uintptr_t)tp + increment);

				vbytes = sizeof (ctf_member_t) * vlen;
				for (n = vlen; n != 0; n--, mp++)
					child |= CTF_TYPE_ISCHILD(mp->ctm_type);
			} else {
				ctf_lmember_t *lmp = (ctf_lmember_t *)
				    ((uintptr_t)tp + increment);

				vbytes = sizeof (ctf_lmember_t) * vlen;
				for (n = vlen; n != 0; n--, lmp++)
					child |=
					    CTF_TYPE_ISCHILD(lmp->ctlm_type);
			}
			break;
		case CTF_K_ENUM:
			vbytes = sizeof (ctf_enum_t) * vlen;
			break;
		case CTF_K_FORWARD:
		case CTF_K_UNKNOWN:
			vbytes = 0;
			break;
		case CTF_K_POINTER:
		case CTF_K_TYPEDEF:
		case CTF_K_VOLATILE:
		case CTF_K_CONST:
		case CTF_K_RESTRICT:
			vbytes = 0;
			break;
		default:
			printf("%s(%d): detected invalid CTF kind -- %u\n", __func__, __LINE__, kind);
			return (EIO);
		}
		*xp = (uint32_t)((uintptr_t) tp - (uintptr_t) ctfdata);
		tp = (ctf_type_t *)((uintptr_t)tp + increment + vbytes);
	}

	return (0);
}

/*
 * CTF Declaration Stack
 *
 * In order to implement ctf_type_name(), we must convert a type graph back
 * into a C type declaration.  Unfortunately, a type graph represents a storage
 * class ordering of the type whereas a type declaration must obey the C rules
 * for operator precedence, and the two orderings are frequently in conflict.
 * For example, consider these CTF type graphs and their C declarations:
 *
 * CTF_K_POINTER -> CTF_K_FUNCTION -> CTF_K_INTEGER  : int (*)()
 * CTF_K_POINTER -> CTF_K_ARRAY -> CTF_K_INTEGER     : int (*)[]
 *
 * In each case, parentheses are used to raise operator * to higher lexical
 * precedence, so the string form of the C declaration cannot be constructed by
 * walking the type graph links and forming the string from left to right.
 *
 * The functions in this file build a set of stacks from the type graph nodes
 * corresponding to the C operator precedence levels in the appropriate order.
 * The code in ctf_type_name() can then iterate over the levels and nodes in
 * lexical precedence order and construct the final C declaration string.
 */
typedef struct ctf_list {
	struct ctf_list *l_prev; /* previous pointer or tail pointer */
	struct ctf_list *l_next; /* next pointer or head pointer */
} ctf_list_t;

#define	ctf_list_prev(elem)	((void *)(((ctf_list_t *)(elem))->l_prev))
#define	ctf_list_next(elem)	((void *)(((ctf_list_t *)(elem))->l_next))

typedef enum {
	CTF_PREC_BASE,
	CTF_PREC_POINTER,
	CTF_PREC_ARRAY,
	CTF_PREC_FUNCTION,
	CTF_PREC_MAX
} ctf_decl_prec_t;

typedef struct ctf_decl_node {
	ctf_list_t cd_list;			/* linked list pointers */
	ctf_id_t cd_type;			/* type identifier */
	uint_t cd_kind;				/* type kind */
	uint_t cd_n;				/* type dimension if array */
} ctf_decl_node_t;

typedef struct ctf_decl {
	ctf_list_t cd_nodes[CTF_PREC_MAX];	/* declaration node stacks */
	int cd_order[CTF_PREC_MAX];		/* storage order of decls */
	ctf_decl_prec_t cd_qualp;		/* qualifier precision */
	ctf_decl_prec_t cd_ordp;		/* ordered precision */
	char *cd_buf;				/* buffer for output */
	char *cd_ptr;				/* buffer location */
	char *cd_end;				/* buffer limit */
	size_t cd_len;				/* buffer space required */
	int cd_err;				/* saved error value */
} ctf_decl_t;

/*
 * Simple doubly-linked list append routine.  This implementation assumes that
 * each list element contains an embedded ctf_list_t as the first member.
 * An additional ctf_list_t is used to store the head (l_next) and tail
 * (l_prev) pointers.  The current head and tail list elements have their
 * previous and next pointers set to NULL, respectively.
 */
static void
ctf_list_append(ctf_list_t *lp, void *new)
{
	ctf_list_t *p = lp->l_prev;	/* p = tail list element */
	ctf_list_t *q = new;		/* q = new list element */

	lp->l_prev = q;
	q->l_prev = p;
	q->l_next = NULL;

	if (p != NULL)
		p->l_next = q;
	else
		lp->l_next = q;
}

/*
 * Prepend the specified existing element to the given ctf_list_t.  The
 * existing pointer should be pointing at a struct with embedded ctf_list_t.
 */
static void
ctf_list_prepend(ctf_list_t *lp, void *new)
{
	ctf_list_t *p = new;		/* p = new list element */
	ctf_list_t *q = lp->l_next;	/* q = head list element */

	lp->l_next = p;
	p->l_prev = NULL;
	p->l_next = q;

	if (q != NULL)
		q->l_prev = p;
	else
		lp->l_prev = p;
}

static void
ctf_decl_init(ctf_decl_t *cd, char *buf, size_t len)
{
	int i;

	bzero(cd, sizeof (ctf_decl_t));

	for (i = CTF_PREC_BASE; i < CTF_PREC_MAX; i++)
		cd->cd_order[i] = CTF_PREC_BASE - 1;

	cd->cd_qualp = CTF_PREC_BASE;
	cd->cd_ordp = CTF_PREC_BASE;

	cd->cd_buf = buf;
	cd->cd_ptr = buf;
	cd->cd_end = buf + len;
}

static void
ctf_decl_fini(ctf_decl_t *cd)
{
	ctf_decl_node_t *cdp, *ndp;
	int i;

	for (i = CTF_PREC_BASE; i < CTF_PREC_MAX; i++) {
		for (cdp = ctf_list_next(&cd->cd_nodes[i]);
		    cdp != NULL; cdp = ndp) {
			ndp = ctf_list_next(cdp);
			free(cdp, M_FBT);
		}
	}
}

static const ctf_type_t *
ctf_lookup_by_id(mod_ctf_t *mc, ctf_id_t type)
{
	const ctf_type_t *tp;
	uint32_t offset;
	uint32_t *typoff = mc->typoffp;

	if (type >= mc->typlen) {
		printf("%s(%d): type %d exceeds max %ld\n",__func__,__LINE__,(int) type,mc->typlen);
		return(NULL);
	}

	/* Check if the type isn't cross-referenced. */
	if ((offset = typoff[type]) == 0) {
		printf("%s(%d): type %d isn't cross referenced\n",__func__,__LINE__, (int) type);
		return(NULL);
	}

	tp = (const ctf_type_t *)(mc->ctftab + offset + sizeof(ctf_header_t));

	return (tp);
}

static void
fbt_array_info(mod_ctf_t *mc, ctf_id_t type, ctf_arinfo_t *arp)
{
	const ctf_header_t *hp = (const ctf_header_t *) mc->ctftab;
	const ctf_type_t *tp;
	const ctf_array_t *ap;
	ssize_t increment;

	bzero(arp, sizeof(*arp));

	if ((tp = ctf_lookup_by_id(mc, type)) == NULL)
		return;

	if (CTF_INFO_KIND(tp->ctt_info) != CTF_K_ARRAY)
		return;

	(void) fbt_get_ctt_size(hp->cth_version, tp, NULL, &increment);

	ap = (const ctf_array_t *)((uintptr_t)tp + increment);
	arp->ctr_contents = ap->cta_contents;
	arp->ctr_index = ap->cta_index;
	arp->ctr_nelems = ap->cta_nelems;
}

static const char *
ctf_strptr(mod_ctf_t *mc, int name)
{
	const ctf_header_t *hp = (const ctf_header_t *) mc->ctftab;;
	const char *strp = "";

	if (name < 0 || name >= hp->cth_strlen)
		return(strp);

	strp = (const char *)(mc->ctftab + hp->cth_stroff + name + sizeof(ctf_header_t));

	return (strp);
}

static void
ctf_decl_push(ctf_decl_t *cd, mod_ctf_t *mc, ctf_id_t type)
{
	ctf_decl_node_t *cdp;
	ctf_decl_prec_t prec;
	uint_t kind, n = 1;
	int is_qual = 0;

	const ctf_type_t *tp;
	ctf_arinfo_t ar;

	if ((tp = ctf_lookup_by_id(mc, type)) == NULL) {
		cd->cd_err = ENOENT;
		return;
	}

	switch (kind = CTF_INFO_KIND(tp->ctt_info)) {
	case CTF_K_ARRAY:
		fbt_array_info(mc, type, &ar);
		ctf_decl_push(cd, mc, ar.ctr_contents);
		n = ar.ctr_nelems;
		prec = CTF_PREC_ARRAY;
		break;

	case CTF_K_TYPEDEF:
		if (ctf_strptr(mc, tp->ctt_name)[0] == '\0') {
			ctf_decl_push(cd, mc, tp->ctt_type);
			return;
		}
		prec = CTF_PREC_BASE;
		break;

	case CTF_K_FUNCTION:
		ctf_decl_push(cd, mc, tp->ctt_type);
		prec = CTF_PREC_FUNCTION;
		break;

	case CTF_K_POINTER:
		ctf_decl_push(cd, mc, tp->ctt_type);
		prec = CTF_PREC_POINTER;
		break;

	case CTF_K_VOLATILE:
	case CTF_K_CONST:
	case CTF_K_RESTRICT:
		ctf_decl_push(cd, mc, tp->ctt_type);
		prec = cd->cd_qualp;
		is_qual++;
		break;

	default:
		prec = CTF_PREC_BASE;
	}

	if ((cdp = malloc(sizeof (ctf_decl_node_t), M_FBT, M_WAITOK)) == NULL) {
		cd->cd_err = EAGAIN;
		return;
	}

	cdp->cd_type = type;
	cdp->cd_kind = kind;
	cdp->cd_n = n;

	if (ctf_list_next(&cd->cd_nodes[prec]) == NULL)
		cd->cd_order[prec] = cd->cd_ordp++;

	/*
	 * Reset cd_qualp to the highest precedence level that we've seen so
	 * far that can be qualified (CTF_PREC_BASE or CTF_PREC_POINTER).
	 */
	if (prec > cd->cd_qualp && prec < CTF_PREC_ARRAY)
		cd->cd_qualp = prec;

	/*
	 * C array declarators are ordered inside out so prepend them.  Also by
	 * convention qualifiers of base types precede the type specifier (e.g.
	 * const int vs. int const) even though the two forms are equivalent.
	 */
	if (kind == CTF_K_ARRAY || (is_qual && prec == CTF_PREC_BASE))
		ctf_list_prepend(&cd->cd_nodes[prec], cdp);
	else
		ctf_list_append(&cd->cd_nodes[prec], cdp);
}

static void
ctf_decl_sprintf(ctf_decl_t *cd, const char *format, ...)
{
	size_t len = (size_t)(cd->cd_end - cd->cd_ptr);
	va_list ap;
	size_t n;

	va_start(ap, format);
	n = vsnprintf(cd->cd_ptr, len, format, ap);
	va_end(ap);

	cd->cd_ptr += MIN(n, len);
	cd->cd_len += n;
}

static ssize_t
fbt_type_name(mod_ctf_t *mc, ctf_id_t type, char *buf, size_t len)
{
	ctf_decl_t cd;
	ctf_decl_node_t *cdp;
	ctf_decl_prec_t prec, lp, rp;
	int ptr, arr;
	uint_t k;

	if (mc == NULL && type == CTF_ERR)
		return (-1); /* simplify caller code by permitting CTF_ERR */

	ctf_decl_init(&cd, buf, len);
	ctf_decl_push(&cd, mc, type);

	if (cd.cd_err != 0) {
		ctf_decl_fini(&cd);
		return (-1);
	}

	/*
	 * If the type graph's order conflicts with lexical precedence order
	 * for pointers or arrays, then we need to surround the declarations at
	 * the corresponding lexical precedence with parentheses.  This can
	 * result in either a parenthesized pointer (*) as in int (*)() or
	 * int (*)[], or in a parenthesized pointer and array as in int (*[])().
	 */
	ptr = cd.cd_order[CTF_PREC_POINTER] > CTF_PREC_POINTER;
	arr = cd.cd_order[CTF_PREC_ARRAY] > CTF_PREC_ARRAY;

	rp = arr ? CTF_PREC_ARRAY : ptr ? CTF_PREC_POINTER : -1;
	lp = ptr ? CTF_PREC_POINTER : arr ? CTF_PREC_ARRAY : -1;

	k = CTF_K_POINTER; /* avoid leading whitespace (see below) */

	for (prec = CTF_PREC_BASE; prec < CTF_PREC_MAX; prec++) {
		for (cdp = ctf_list_next(&cd.cd_nodes[prec]);
		    cdp != NULL; cdp = ctf_list_next(cdp)) {

			const ctf_type_t *tp =
			    ctf_lookup_by_id(mc, cdp->cd_type);
			const char *name = ctf_strptr(mc, tp->ctt_name);

			if (k != CTF_K_POINTER && k != CTF_K_ARRAY)
				ctf_decl_sprintf(&cd, " ");

			if (lp == prec) {
				ctf_decl_sprintf(&cd, "(");
				lp = -1;
			}

			switch (cdp->cd_kind) {
			case CTF_K_INTEGER:
			case CTF_K_FLOAT:
			case CTF_K_TYPEDEF:
				ctf_decl_sprintf(&cd, "%s", name);
				break;
			case CTF_K_POINTER:
				ctf_decl_sprintf(&cd, "*");
				break;
			case CTF_K_ARRAY:
				ctf_decl_sprintf(&cd, "[%u]", cdp->cd_n);
				break;
			case CTF_K_FUNCTION:
				ctf_decl_sprintf(&cd, "()");
				break;
			case CTF_K_STRUCT:
			case CTF_K_FORWARD:
				ctf_decl_sprintf(&cd, "struct %s", name);
				break;
			case CTF_K_UNION:
				ctf_decl_sprintf(&cd, "union %s", name);
				break;
			case CTF_K_ENUM:
				ctf_decl_sprintf(&cd, "enum %s", name);
				break;
			case CTF_K_VOLATILE:
				ctf_decl_sprintf(&cd, "volatile");
				break;
			case CTF_K_CONST:
				ctf_decl_sprintf(&cd, "const");
				break;
			case CTF_K_RESTRICT:
				ctf_decl_sprintf(&cd, "restrict");
				break;
			}

			k = cdp->cd_kind;
		}

		if (rp == prec)
			ctf_decl_sprintf(&cd, ")");
	}

	ctf_decl_fini(&cd);
	return (cd.cd_len);
}

static void
fbt_getargdesc(void *arg __unused, dtrace_id_t id __unused, void *parg, dtrace_argdesc_t *desc)
{
	const ushort_t *dp;
	fbt_probe_t *fbt = parg;
	mod_ctf_t mc;
	dtrace_modctl_t *ctl = fbt->fbtp_ctl;
	int ndx = desc->dtargd_ndx;
	int symindx = fbt->fbtp_symindx;
	uint32_t *ctfoff;
	uint32_t offset;
	ushort_t info, kind, n;
	int nsyms;

	desc->dtargd_ndx = DTRACE_ARGNONE;

	/* Get a pointer to the CTF data and it's length. */
	if (mod_ctf_get(ctl, &mc) != 0) {
		static int report=0;
		if (report < 1) {
		    report++;
		    printf("FBT: Error no CTF section found in module \"%s\"\n",
			    ctl->mod_info->mi_name);
		}
		/* No CTF data? Something wrong? *shrug* */
		return;
	}

	nsyms = (mc.nmap != NULL) ? mc.nmapsize : mc.nsym;

	/* Check if this module hasn't been initialised yet. */
	if (mc.ctfoffp == NULL) {
		/*
		 * Initialise the CTF object and function symindx to
		 * byte offset array.
		 */
		if (fbt_ctfoff_init(ctl, &mc) != 0) {
			return;
		}

		/* Initialise the CTF type to byte offset array. */
		if (fbt_typoff_init(&mc) != 0) {
			return;
		}
	}

	ctfoff = mc.ctfoffp;

	if (ctfoff == NULL || mc.typoffp == NULL) {
		return;
	}

	/* Check if the symbol index is out of range. */
	if (symindx >= nsyms)
		return;

	/* Check if the symbol isn't cross-referenced. */
	if ((offset = ctfoff[symindx]) == 0xffffffff)
		return;

	dp = (const ushort_t *)(mc.ctftab + offset + sizeof(ctf_header_t));

	info = *dp++;
	kind = CTF_INFO_KIND(info);
	n = CTF_INFO_VLEN(info);

	if (kind == CTF_K_UNKNOWN && n == 0) {
		printf("%s(%d): Unknown function %s!\n",__func__,__LINE__,
		    fbt->fbtp_name);
		return;
	}

	if (kind != CTF_K_FUNCTION) {
		printf("%s(%d): Expected a function %s!\n",__func__,__LINE__,
		    fbt->fbtp_name);
		return;
	}

	/* Check if the requested argument doesn't exist. */
	if (ndx >= n)
		return;

	/* Skip the return type and arguments up to the one requested. */
	dp += ndx + 1;

	if (fbt_type_name(&mc, *dp, desc->dtargd_native, sizeof(desc->dtargd_native)) > 0) {
		desc->dtargd_ndx = ndx;
	}

	return;
}

static void
fbt_load(void)
{
	/* Default the probe table size if not specified. */
	if (fbt_probetab_size == 0)
		fbt_probetab_size = FBT_PROBETAB_SIZE;

	/* Choose the hash mask for the probe table. */
	fbt_probetab_mask = fbt_probetab_size - 1;

	/* Allocate memory for the probe table. */
	fbt_probetab =
	    malloc(fbt_probetab_size * sizeof (fbt_probe_t *), M_FBT, M_WAITOK | M_ZERO);

	dtrace_doubletrap_func = fbt_doubletrap;
	dtrace_invop_add(fbt_invop);
#ifdef __arm__
	dtrace_emulation_jump_addr = fbt_emulate;
#endif

	if (dtrace_register("fbt", &fbt_attr, DTRACE_PRIV_USER,
	    NULL, &fbt_pops, NULL, &fbt_id) != 0)
		return;
}


static int
fbt_unload(void)
{
	int error = 0;

#ifdef __arm__
	dtrace_emulation_jump_addr = NULL;
#endif
	/* De-register the invalid opcode handler. */
	dtrace_invop_remove(fbt_invop);

	dtrace_doubletrap_func = NULL;

	/* De-register this DTrace provider. */
	if ((error = dtrace_unregister(fbt_id)) != 0)
		return (error);

	/* Free the probe table. */
	free(fbt_probetab, M_FBT);
	fbt_probetab = NULL;
	fbt_probetab_mask = 0;

	return (error);
}


static int
dtrace_fbt_modcmd(modcmd_t cmd, void *data)
{
	int bmajor = -1, cmajor = -1;
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		fbt_load();
		return devsw_attach("fbt", NULL, &bmajor,
		    &fbt_cdevsw, &cmajor);
	case MODULE_CMD_FINI:
		error = fbt_unload();
		if (error != 0)
			return error;
		return devsw_detach(NULL, &fbt_cdevsw);
	case MODULE_CMD_AUTOUNLOAD:
		return EBUSY;
	default:
		return ENOTTY;
	}
}

static int
fbt_open(dev_t dev, int flags, int mode, struct lwp *l)
{
	return (0);
}

MODULE(MODULE_CLASS_MISC, dtrace_fbt, "dtrace,zlib");
