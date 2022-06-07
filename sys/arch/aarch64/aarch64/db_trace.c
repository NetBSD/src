/* $NetBSD: db_trace.c,v 1.19 2022/06/07 23:55:25 ryo Exp $ */

/*
 * Copyright (c) 2017 Ryo Shimizu <ryo@nerv.org>
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
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.19 2022/06/07 23:55:25 ryo Exp $");

#include <sys/param.h>
#include <sys/bitops.h>
#include <sys/proc.h>

#include <aarch64/db_machdep.h>
#include <aarch64/machdep.h>
#include <aarch64/armreg.h>
#include <aarch64/vmparam.h>

#include <arm/cpufunc.h>

#include <uvm/uvm_extern.h>

#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_sym.h>
#include <ddb/db_proc.h>
#include <ddb/db_lwp.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>

#ifdef _KERNEL
extern char el0_trap[];
extern char el1_trap[];
#else
/* see also usr.sbin/crash/arch/aarch64.c */
extern vaddr_t el0_trap;
extern vaddr_t el1_trap;
#endif

#define MAXBACKTRACE	128	/* against infinite loop */


__CTASSERT(VM_MIN_ADDRESS == 0);
#define IN_USER_VM_ADDRESS(addr)	\
	((addr) < VM_MAX_ADDRESS)
#define IN_KERNEL_VM_ADDRESS(addr)	\
	((VM_MIN_KERNEL_ADDRESS <= (addr)) && ((addr) < VM_MAX_KERNEL_ADDRESS))

static void
pr_frame(struct trapframe *tf, void (*pr)(const char *, ...) __printflike(1, 2))
{
	struct trapframe tf_buf;

	db_read_bytes((db_addr_t)tf, sizeof(tf_buf), (char *)&tf_buf);

	if (tf_buf.tf_sp == 0) {
		(*pr)("---- switchframe %p (%zu bytes) ----\n",
		    tf, sizeof(*tf));
		dump_switchframe(tf, pr);
	} else {
#ifdef _KERNEL
		(*pr)("---- %s: trapframe %p (%zu bytes) ----\n",
		    (tf_buf.tf_esr == (uint64_t)-1) ? "Interrupt" :
		    eclass_trapname(__SHIFTOUT(tf_buf.tf_esr, ESR_EC)),
		    tf, sizeof(*tf));
#else
		(*pr)("---- trapframe %p (%zu bytes) ----\n", tf, sizeof(*tf));
#endif
		dump_trapframe(tf, pr);
	}
	(*pr)("------------------------"
	      "------------------------\n");
}

static bool __unused
is_lwp(void *p)
{
	lwp_t *lwp;

	for (lwp = db_lwp_first(); lwp != NULL; lwp = db_lwp_next(lwp)) {
		if (lwp == p)
			return true;
	}
	return false;
}

static vaddr_t
db_lwp_getuarea(lwp_t *l)
{
	void *laddr;
	db_read_bytes((db_addr_t)&l->l_addr, sizeof(laddr), (char *)&laddr);
	if (laddr == 0)
		return 0;
	return (vaddr_t)((char *)laddr - UAREA_PCB_OFFSET);
}

static const char *
getlwpnamebysp(uint64_t sp)
{
	static char c_name[MAXCOMLEN];
	lwp_t *lwp;
	struct proc *pp;
	char *lname;

	for (lwp = db_lwp_first(); lwp != NULL; lwp = db_lwp_next(lwp)) {
		uint64_t uarea = db_lwp_getuarea(lwp);
		if ((uarea <= sp) && (sp < (uarea + USPACE))) {
			db_read_bytes((db_addr_t)&lwp->l_name, sizeof(lname),
			    (char *)&lname);
			if (lname != NULL) {
				db_read_bytes((db_addr_t)lname, sizeof(c_name),
			    c_name);
				return c_name;
			}
			db_read_bytes((db_addr_t)&lwp->l_proc, sizeof(pp),
			    (char *)&pp);
			if (pp != NULL) {
				db_read_bytes((db_addr_t)&pp->p_comm,
				    sizeof(c_name), c_name);
				return c_name;
			}
			break;
		}
	}
	return "unknown";
}

#define TRACEFLAG_LOOKUPLWP	0x00000001
#define TRACEFLAG_USERSPACE	0x00000002

static void
pr_traceaddr(const char *prefix, uint64_t frame, uint64_t pc, int flags,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	db_expr_t offset;
	db_sym_t sym;
	const char *name;

	sym = db_search_symbol(pc, DB_STGY_ANY, &offset);
	if (sym != DB_SYM_NULL) {
		db_symbol_values(sym, &name, NULL);

		if (flags & TRACEFLAG_LOOKUPLWP) {
			(*pr)("%s %016lx %s %s() at %016lx ",
			    prefix, frame, getlwpnamebysp(frame), name, pc);
		} else {
			(*pr)("%s %016lx %s() at %016lx ",
			    prefix, frame, name, pc);
		}
		db_printsym(pc, DB_STGY_PROC, pr);
		(*pr)("\n");
	} else {
		if (flags & TRACEFLAG_LOOKUPLWP) {
			(*pr)("%s %016lx %s ?() at %016lx\n",
			    prefix, frame, getlwpnamebysp(frame), pc);
		} else {
			(*pr)("%s %016lx ?() at %016lx\n", prefix, frame, pc);
		}
	}
}

static __inline uint64_t
SignExtend(int bitwidth, uint64_t imm, unsigned int multiply)
{
	const uint64_t signbit = ((uint64_t)1 << (bitwidth - 1));
	const uint64_t immmax = signbit << 1;

	if (imm & signbit)
		imm -= immmax;
	return imm * multiply;
}

static __inline uint64_t
ZeroExtend(int bitwidth, uint64_t imm, unsigned int multiply)
{
	return imm * multiply;
}

/* rotate right. if n < 0, rotate left. */
static __inline uint64_t
rotate(int bitwidth, uint64_t v, int n)
{
	uint64_t result;

	n &= (bitwidth - 1);
	result = (((v << (bitwidth - n)) | (v >> n)));
	if (bitwidth < 64)
		result &= ((1ULL << bitwidth) - 1);
	return result;
}

static __inline uint64_t
DecodeBitMasks(uint64_t sf, uint64_t n, uint64_t imms, uint64_t immr)
{
	const int bitwidth = (sf == 0) ? 32 : 64;
	uint64_t result;
	int esize, len;

	len = fls64((n << 6) + (~imms & 0x3f)) - 1;
	esize = (1 << len);
	imms &= (esize - 1);
	immr &= (esize - 1);
	result = rotate(esize, (1ULL << (imms + 1)) - 1, immr);
	while (esize < bitwidth) {
		result |= (result << esize);
		esize <<= 1;
	}
	if (sf == 0)
		result &= ((1ULL << bitwidth) - 1);
	return result;
}

static int
analyze_func(db_addr_t func_entry, db_addr_t pc, db_addr_t sp,
    db_addr_t *lrp, vsize_t *stacksizep,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	vsize_t ssize = 0, lr_off = 0;
	db_addr_t lr = 0;
	uint64_t alloc_by_Xn_kvalue = 0;
	uint64_t alloc_by_Xn_kmask = 0;
	int alloc_by_Xn_reg = -1;
	bool found_lr_off = false;
	bool func_entry_autodetect = false;

#define MAX_BACKTRACK_ANALYZE_INSN	(1024 * 4)
	if (func_entry == 0) {
		if (pc > MAX_BACKTRACK_ANALYZE_INSN)
			func_entry = pc - MAX_BACKTRACK_ANALYZE_INSN;
		else
			func_entry = 4;
		func_entry_autodetect = true;
	};


	/*
	 * Locate the following instructions that allocates a stackframe.
	 * Only the following patterns are supported:
	 *
	 *  sub sp, sp, #ALLOCSIZE		-> ssize += ALLOCSIZE
	 *  sub sp, sp, #ALLOCSIZE, lsl #12	-> ssize += (ALLOCSIZE << 12)
	 *
	 *  mov xN, #ALLOCSIZE1
	 *  (movk xN, #ALLOCSIZE2, lsl #xx)
	 *  sub sp, sp, xN			-> ssize += ALLOCSIZE
	 *
	 *  stp x30, x??, [sp, #-ALLOCSIZE]!	-> ssize =+ ALLOCSIZE, lr_off=0
	 *  stp x??, x30, [sp, #-ALLOCSIZE]!	-> ssize =+ ALLOCSIZE, lr_off=8
	 *  stp x??, x??, [sp, #-ALLOCSIZE]!	-> ssize =+ ALLOCSIZE
	 *
	 *  str x30, [sp, #-ALLOCSIZE]!		-> ssize =+ ALLOCSIZE, lr_off=0
	 *
	 *  stp x30, x??, [sp, #LR_OFF]		-> lr_off = LR_OFF
	 *  stp x??, x30, [sp, #LR_OFF]		-> lr_off = LR_OFF+8
	 *  str x30, [sp, #LR_OFF]		-> lr_off = LR_OFF
	 */

/* #define BACKTRACE_ANALYZE_DEBUG */
#ifdef BACKTRACE_ANALYZE_DEBUG
#define TRACE_DEBUG(fmt, args...)	pr("BACKTRACE: " fmt, ## args)
#else
#define TRACE_DEBUG(args...)		__nothing
#endif

	TRACE_DEBUG("func_entry=%016lx\n", func_entry);
	TRACE_DEBUG("        pc=%016lx (+%#lx)\n", pc, pc - func_entry);
	TRACE_DEBUG("        sp=%016lx\n", sp);

	for (pc -= 4; pc >= func_entry; pc -= 4) {
		uint32_t insn;

		db_read_bytes(pc, sizeof(insn), (char *)&insn);
		if (insn == 0)
			break;
		LE32TOH(insn);

		TRACE_DEBUG("INSN: %016lx: %04x\n", pc, insn);

		/* "ret", "eret", or "paciasp" to detect function entry */
		if (func_entry_autodetect && (
		    insn == 0xd65f03e0 ||	/* "ret" */
		    insn == 0xd69f03e0 ||	/* "eret" */
		    insn == 0xd503233f))	/* "paciasp" */
			break;

		/* "sub sp,sp,#imm" or "sub sp,sp,#imm,lsl #12" */
		if ((insn & 0xff8003ff) == 0xd10003ff) {
			unsigned int sh = (insn >> 22) & 1;
			uint64_t imm12 =
			    ZeroExtend(12, (insn >> 10) & 0xfff, 1);
			if (sh)
				imm12 <<= 12;
			ssize += imm12;
			TRACE_DEBUG("sub sp,sp,#%lu\n", imm12);
			continue;
		}

		/* sub sp,sp,Xn */
		if ((insn & 0xffe0ffff) == 0xcb2063ff) {
			alloc_by_Xn_reg = (insn >> 16) & 0x1f;
			alloc_by_Xn_kvalue = 0;
			alloc_by_Xn_kmask = 0;
			TRACE_DEBUG("sub sp,sp,x%d\n", alloc_by_Xn_reg);
			continue;
		}
		if (alloc_by_Xn_reg >= 0) {
			/* movk xN,#ALLOCSIZE2,lsl #xx */
			if ((insn & 0xff80001f) ==
			    (0xf2800000 | alloc_by_Xn_reg)) {
				int hw = (insn >> 21) & 3;
				alloc_by_Xn_kvalue = ZeroExtend(16,
				    (insn >> 5) & 0xffff, 1) << (hw * 16);
				alloc_by_Xn_kmask = (0xffffULL << (hw * 16));
				TRACE_DEBUG("movk x%d,#%#lx,lsl #%d\n",
				    alloc_by_Xn_reg, alloc_by_Xn_kvalue,
				    hw * 16);
				continue;
			}

			/* (orr) mov xN,#ALLOCSIZE1 */
			if ((insn & 0xff8003ff) ==
			    (0xb20003e0 | alloc_by_Xn_reg)) {
				uint64_t n = (insn >> 22) & 1;
				uint64_t immr = (insn >> 16) & 0x3f;
				uint64_t imms = (insn >> 10) & 0x3f;
				uint64_t v = DecodeBitMasks(1, n, imms, immr);
				TRACE_DEBUG("(orr) mov x%d,#%#lx\n",
				    alloc_by_Xn_reg, v);
				ssize += v;
				alloc_by_Xn_reg = -1;
				continue;
			}

			/* (movz) mov xN,#ALLOCSIZE1 */
			if ((insn & 0xffe0001f) ==
			    (0xd2800000 | alloc_by_Xn_reg)) {
				uint64_t v =
				    ZeroExtend(16, (insn >> 5) & 0xffff, 1);
				TRACE_DEBUG("(movz) mov x%d,#%#lx\n",
				    alloc_by_Xn_reg, v);
				v &= ~alloc_by_Xn_kmask;
				v |= alloc_by_Xn_kvalue;
				ssize += v;
				alloc_by_Xn_reg = -1;
				continue;
			}
			/* (movn) mov xN,#ALLOCSIZE1 */
			if ((insn & 0xffe0001f) ==
			    (0x92800000 | alloc_by_Xn_reg)) {
				uint64_t v =
				    ~ZeroExtend(16, (insn >> 5) & 0xffff, 1);
				TRACE_DEBUG("(movn) mov x%d,#%#lx\n",
				    alloc_by_Xn_reg, v);
				v &= ~alloc_by_Xn_kmask;
				v |= alloc_by_Xn_kvalue;
				ssize += v;
				alloc_by_Xn_reg = -1;
				continue;
			}
		}

		/* stp x??,x??,[sp,#-imm7]! */
		if ((insn & 0xffe003e0) == 0xa9a003e0) {
			int64_t imm7 = SignExtend(7, (insn >> 15) & 0x7f, 8);
			uint64_t Rt2 = (insn >> 10) & 0x1f;
			uint64_t Rt1 = insn & 0x1f;
			if (Rt1 == 30) {
				TRACE_DEBUG("stp x30,Xn[sp,#%ld]!\n", imm7);
				lr_off = ssize;
				ssize += -imm7;
				found_lr_off = true;
			} else if (Rt2 == 30) {
				TRACE_DEBUG("stp Xn,x30,[sp,#%ld]!\n", imm7);
				lr_off = ssize + 8;
				ssize += -imm7;
				found_lr_off = true;
			} else {
				ssize += -imm7;
				TRACE_DEBUG("stp Xn,Xn,[sp,#%ld]!\n", imm7);
			}

			/*
			 * "stp x29,x30,[sp,#-n]!" is the code to create
			 * a frame pointer at the beginning of the function.
			 */
			if (func_entry_autodetect && Rt1 == 29 && Rt2 == 30)
				break;

			continue;
		}

		/* stp x??,x??,[sp,#imm7] */
		if ((insn & 0xffc003e0) == 0xa90003e0) {
			int64_t imm7 = SignExtend(7, (insn >> 15) & 0x7f, 8);
			uint64_t Rt2 = (insn >> 10) & 0x1f;
			uint64_t Rt1 = insn & 0x1f;
			if (Rt1 == 30) {
				lr_off = ssize + imm7;
				found_lr_off = true;
				TRACE_DEBUG("stp x30,X%lu[sp,#%ld]\n",
				    Rt2, imm7);
				TRACE_DEBUG("lr off = %lu = %#lx\n",
				    lr_off, lr_off);
			} else if (Rt2 == 30) {
				lr_off = ssize + imm7 + 8;
				found_lr_off = true;
				TRACE_DEBUG("stp X%lu,x30,[sp,#%ld]\n",
				    Rt1, imm7);
				TRACE_DEBUG("lr off = %lu = %#lx\n",
				    lr_off, lr_off);
			}
			continue;
		}

		/* str x30,[sp,#imm12] */
		if ((insn & 0xffc003ff) == 0xf90003fe) {
			uint64_t imm12 =
			    ZeroExtend(12, (insn >> 10) & 0xfff, 8);
			lr_off = ssize + imm12;
			found_lr_off = true;
			TRACE_DEBUG("str x30,[sp,#%lu]\n", imm12);
			TRACE_DEBUG("lr off = %lu = %#lx\n", lr_off, lr_off);
			continue;
		}

		/* str x30,[sp,#-imm9]! */
		if ((insn & 0xfff00fff) == 0xf8100ffe) {
			int64_t imm9 = SignExtend(9, (insn >> 12) & 0x1ff, 1);
			lr_off = ssize;
			ssize += -imm9;
			found_lr_off = true;
			TRACE_DEBUG("str x30,[sp,#%ld]!\n", imm9);
			TRACE_DEBUG("lr off = %lu = %#lx\n", lr_off, lr_off);
			continue;
		}
	}

	if (found_lr_off) {
		if (lr_off >= ssize) {
			pr("cannot locate return address\n");
			return -1;
		}
		db_read_bytes((db_addr_t)sp + lr_off, sizeof(lr), (char *)&lr);
		lr = aarch64_strip_pac(lr);
	}
	*stacksizep = ssize;
	*lrp = lr;

	TRACE_DEBUG("-----------\n");
	TRACE_DEBUG("       sp: %#lx\n", sp);
	TRACE_DEBUG("stacksize: %#06lx = %lu\n", ssize, ssize);
	TRACE_DEBUG("lr offset: %#06lx = %lu\n", lr_off, lr_off);
	TRACE_DEBUG("   new lr: %#lx\n", lr);
	TRACE_DEBUG("===========\n\n");

	return 0;
}

/*
 * Backtrace without framepointer ($fp).
 *
 * Examines the contents of a function and returns the stack size allocated
 * by the function and the stored $lr.
 *
 * This works well for code compiled with -fomit-frame-pointer.
 */
static void
db_sp_trace(struct trapframe *tf, db_addr_t fp, db_expr_t count, int flags,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	struct trapframe tf_buf;
	db_addr_t pc, sp, lr0;
	bool allow_leaf_function = false;

	if (tf == NULL) {
		/*
		 * In the case of "trace/s <frame-address>",
		 * the specified frame pointer address is considered
		 * a trapframe (or a switchframe) address.
		 */
		tf = (struct trapframe *)fp;
	}

	pr_frame(tf, pr);

	db_read_bytes((db_addr_t)tf, sizeof(tf_buf), (char *)&tf_buf);
	if (tf_buf.tf_sp == 0) {
		/* switchframe */
		lr0 = 0;
		pc = aarch64_strip_pac(tf_buf.tf_lr);
		sp = (uint64_t)(tf + 1);
	} else {
		/* trapframe */
		lr0 = aarch64_strip_pac(tf_buf.tf_lr);
		pc = tf_buf.tf_pc;
		sp = tf_buf.tf_sp;
		allow_leaf_function = true;
	}

	TRACE_DEBUG("pc =%016lx\n", pc);
	TRACE_DEBUG("sp =%016lx\n", sp);
	TRACE_DEBUG("lr0=%016lx\n", lr0);

	for (; (count > 0) && (sp != 0); count--) {
		if (((pc - 4) == (db_addr_t)el0_trap) ||
		    ((pc - 4) == (db_addr_t)el1_trap)) {

			pr_traceaddr("tf", sp, pc - 4, flags, pr);

			db_read_bytes((db_addr_t)sp, sizeof(tf_buf),
			    (char *)&tf_buf);
			if (tf_buf.tf_lr == 0)
				break;
			pr_frame((struct trapframe *)sp, pr);

			sp = tf_buf.tf_sp;
			pc = tf_buf.tf_pc;
			if (pc == 0)
				pc = aarch64_strip_pac(tf_buf.tf_lr);
			if (pc == 0)
				break;

			pr_traceaddr("sp", sp, pc, flags, pr);

		} else {
			db_sym_t sym;
			db_addr_t func_entry, lr;
			db_expr_t func_offset;
			vsize_t stacksize;

			pr_traceaddr("sp", sp, pc, flags, pr);

			if ((flags & TRACEFLAG_USERSPACE) == 0 &&
			    !IN_KERNEL_VM_ADDRESS(pc))
				break;

			sym = db_search_symbol(pc, DB_STGY_ANY, &func_offset);
			if (sym != 0) {
				func_entry = pc - func_offset;
				if (func_entry ==
				    (db_addr_t)cpu_switchto_softint) {
					/*
					 * In cpu_switchto_softint(), backtrace
					 * information for DDB is pushed onto
					 * the stack.
					 */
					db_read_bytes((db_addr_t)sp + 8,
					    sizeof(pc), (char *)&pc);
					db_read_bytes((db_addr_t)sp,
					    sizeof(sp), (char *)&sp);
					continue;
				}
			} else {
				func_entry = 0;	/* autodetect mode */
			}

			if (analyze_func(func_entry, pc, sp, &lr, &stacksize,
			    pr) != 0)
				break;

			if (allow_leaf_function) {
				if (lr == 0)
					lr = lr0;
				allow_leaf_function = false;
			} else {
				if (lr == 0)
					break;
			}

			sp += stacksize;
			pc = lr;
		}
	}
}

static void
db_fp_trace(struct trapframe *tf, db_addr_t fp, db_expr_t count, int flags,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	uint64_t lr;
	uint64_t lastlr, lastfp;

	if (tf != NULL) {
		pr_frame(tf, pr);
		lastfp = lastlr = lr = fp = 0;

		db_read_bytes((db_addr_t)&tf->tf_pc, sizeof(lr), (char *)&lr);
		db_read_bytes((db_addr_t)&tf->tf_reg[29], sizeof(fp), (char *)&fp);
		lr = aarch64_strip_pac(lr);

		pr_traceaddr("fp", fp, lr - 4, flags, pr);
	}

	for (; (count > 0) && (fp != 0); count--) {

		lastfp = fp;
		fp = lr = 0;
		/*
		 * normal stack frame
		 *  fp[0]  saved fp(x29) value
		 *  fp[1]  saved lr(x30) value
		 */
		db_read_bytes(lastfp + 0, sizeof(fp), (char *)&fp);
		db_read_bytes(lastfp + 8, sizeof(lr), (char *)&lr);
		lr = aarch64_strip_pac(lr);

		if (lr == 0 || ((flags & TRACEFLAG_USERSPACE) == 0 &&
		    IN_USER_VM_ADDRESS(lr)))
			break;

		if (((char *)(lr - 4) == (char *)el0_trap) ||
		    ((char *)(lr - 4) == (char *)el1_trap)) {

			tf = (struct trapframe *)fp;

			lastfp = (uint64_t)tf;
			lastlr = lr;
			lr = fp = 0;
			db_read_bytes((db_addr_t)&tf->tf_pc, sizeof(lr),
			    (char *)&lr);
			if (lr == 0) {
				/*
				 * The exception may have been from a
				 * jump to null, so the null pc we
				 * would return to is useless.  Try
				 * x[30] instead -- that will be the
				 * return address for the jump.
				 */
				db_read_bytes((db_addr_t)&tf->tf_reg[30],
				    sizeof(lr), (char *)&lr);
			}
			db_read_bytes((db_addr_t)&tf->tf_reg[29], sizeof(fp),
			    (char *)&fp);
			lr = aarch64_strip_pac(lr);

			pr_traceaddr("tf", (db_addr_t)tf, lastlr - 4, flags, pr);

			if (lr == 0)
				break;

			pr_frame(tf, pr);
			tf = NULL;

			if ((flags & TRACEFLAG_USERSPACE) == 0 &&
			    IN_USER_VM_ADDRESS(lr))
				break;

			pr_traceaddr("fp", fp, lr, flags, pr);
		} else {
			pr_traceaddr("fp", fp, lr - 4, flags, pr);
		}
	}
}

void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif, void (*pr)(const char *, ...) __printflike(1, 2))
{
	uint64_t fp;
	struct trapframe *tf = NULL;
	int flags = 0;
	bool trace_thread = false;
	bool trace_lwp = false;
	bool trace_sp = false;

	for (; *modif != '\0'; modif++) {
		switch (*modif) {
		case 'a':
			trace_lwp = true;
			trace_thread = false;
			break;
		case 'l':
			break;
		case 't':
			trace_thread = true;
			trace_lwp = false;
			break;
		case 's':
			trace_sp = true;
			break;
		case 'u':
			flags |= TRACEFLAG_USERSPACE;
			break;
		case 'x':
			flags |= TRACEFLAG_LOOKUPLWP;
			break;
		default:
			pr("usage: bt[/ulsx] [frame-address][,count]\n");
			pr("       bt/t[ulsx] [pid][,count]\n");
			pr("       bt/a[ulsx] [lwpaddr][,count]\n");
			pr("\n");
			pr("       /s      trace without framepointer\n");
			pr("       /x      reverse lookup lwp name from sp\n");
			return;
		}
	}

#if defined(_KERNEL)
	if (!have_addr) {
		if (trace_lwp) {
			addr = (db_expr_t)curlwp;
		} else if (trace_thread) {
			addr = curlwp->l_proc->p_pid;
		} else {
			tf = DDB_REGS;
		}
	}
#endif

	if (trace_thread) {
		proc_t *pp;

		if ((pp = db_proc_find((pid_t)addr)) == 0) {
			(*pr)("trace: pid %d: not found\n", (int)addr);
			return;
		}
		db_read_bytes((db_addr_t)pp + offsetof(proc_t, p_lwps.lh_first),
		    sizeof(addr), (char *)&addr);
		trace_thread = false;
		trace_lwp = true;
	}

#if 0
	/* "/a" is abbreviated? */
	if (!trace_lwp && is_lwp(addr))
		trace_lwp = true;
#endif

	if (trace_lwp) {
		struct lwp l;
		pid_t pid;

		db_read_bytes(addr, sizeof(l), (char *)&l);
		db_read_bytes((db_addr_t)l.l_proc + offsetof(proc_t, p_pid),
		    sizeof(pid), (char *)&pid);

#if defined(_KERNEL)
		if (addr == (db_expr_t)curlwp) {
			fp = (uint64_t)&DDB_REGS->tf_reg[29];	/* &reg[29]={fp,lr} */
			tf = DDB_REGS;
			(*pr)("trace: pid %d lid %d (curlwp) at tf %p\n",
			    pid, l.l_lid, tf);
		} else
#endif
		{
			struct pcb *pcb = lwp_getpcb(&l);

			db_read_bytes((db_addr_t)pcb +
			    offsetof(struct pcb, pcb_tf),
			    sizeof(tf), (char *)&tf);
			if (tf != 0) {
				db_read_bytes((db_addr_t)&tf->tf_reg[29],
				    sizeof(fp), (char *)&fp);
				(*pr)("trace: pid %d lid %d at tf %p (in pcb)\n",
				    pid, l.l_lid, tf);
			}
#if defined(MULTIPROCESSOR) && defined(_KERNEL)
			else if (l.l_stat == LSONPROC ||
			    (l.l_pflag & LP_RUNNING) != 0) {

				/* running lwp on other cpus */
				extern struct trapframe *db_readytoswitch[];
				u_int index;

				db_read_bytes((db_addr_t)l.l_cpu +
				    offsetof(struct cpu_info, ci_index),
				    sizeof(index), (char *)&index);
				tf = db_readytoswitch[index];

				(*pr)("trace: pid %d lid %d at tf %p (in kdb_trap)\n",
				    pid, l.l_lid, tf);
			}
#endif
			else {
				(*pr)("trace: no trapframe found for lwp: %p\n", (void *)addr);
			}
		}
	} else if (tf == NULL) {
		fp = addr;
		pr("trace fp %016lx\n", fp);
	} else {
		pr("trace tf %p\n", tf);
	}

	if (count > MAXBACKTRACE)
		count = MAXBACKTRACE;

	if (trace_sp) {
		/* trace $lr pushed to sp */
		db_sp_trace(tf, fp, count, flags, pr);
	} else {
		/* trace $fp linked list */
		db_fp_trace(tf, fp, count, flags, pr);
	}
}
