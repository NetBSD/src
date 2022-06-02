/*	$NetBSD: fix_unaligned.c,v 1.2 2022/06/02 00:32:14 rin Exp $	*/

/*
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rin Okuyama.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Routines to fix unaligned memory access for userland process.
 *
 * Intended mainly for PPC_IBM403 at the moment:
 *
 * - Fetch and decode insn; 403 does not have DSISR.
 *
 * - Only for integer insn; unaligned floating-point load/store are taken
 *   care of by FPU emulator. (Support for FPU insn should be trivial.)
 *
 * Also note:
 *
 * - For invalid forms, behaviors are undefined and not documented in
 *   processor manuals. Here, we mimic what described in
 *   "AIX 7.2 Assembler language reference":
 *
 *   - For "u" variants, ra is not updated if ra == 0 (or rd for load).
 *
 *   - Fix for {l,st}mw is disabled by default.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fix_unaligned.c,v 1.2 2022/06/02 00:32:14 rin Exp $");

#include "opt_ddb.h"
#include "opt_ppcarch.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/evcnt.h>
#include <sys/siginfo.h>
#include <sys/systm.h>

#include <powerpc/frame.h>
#include <powerpc/instr.h>
#include <powerpc/trap.h>

#define	UA_EVCNT_ATTACH(name)						\
    static struct evcnt unaligned_ev_##name =				\
	EVCNT_INITIALIZER(EVCNT_TYPE_TRAP, NULL, "unaligned", #name);	\
	EVCNT_ATTACH_STATIC(unaligned_ev_##name)

#define	UA_EVCNT_INCR(name)	unaligned_ev_##name.ev_count++

UA_EVCNT_ATTACH(lwz);
UA_EVCNT_ATTACH(lwzu);
UA_EVCNT_ATTACH(stw);
UA_EVCNT_ATTACH(stwu);
UA_EVCNT_ATTACH(lhz);
UA_EVCNT_ATTACH(lhzu);
UA_EVCNT_ATTACH(lha);
UA_EVCNT_ATTACH(lhau);
UA_EVCNT_ATTACH(sth);
UA_EVCNT_ATTACH(sthu);

UA_EVCNT_ATTACH(lwzx);
UA_EVCNT_ATTACH(lwzux);
UA_EVCNT_ATTACH(stwx);
UA_EVCNT_ATTACH(stwux);
UA_EVCNT_ATTACH(lhzx);
UA_EVCNT_ATTACH(lhzux);
UA_EVCNT_ATTACH(lhax);
UA_EVCNT_ATTACH(lhaux);
UA_EVCNT_ATTACH(sthx);
UA_EVCNT_ATTACH(sthux);
UA_EVCNT_ATTACH(lwbrx);
UA_EVCNT_ATTACH(stwbrx);
UA_EVCNT_ATTACH(lhbrx);
UA_EVCNT_ATTACH(sthbrx);

UA_EVCNT_ATTACH(lmw);
UA_EVCNT_ATTACH(stmw);

UA_EVCNT_ATTACH(isi);
UA_EVCNT_ATTACH(dsi);
UA_EVCNT_ATTACH(unknown);
UA_EVCNT_ATTACH(invalid);

#if 0
#define	UNALIGNED_DEBUG 1
#define	FIX_UNALIGNED_LSTMW 1
#endif

#if defined(UNALIGNED_DEBUG)
int unaligned_debug = 1;
#elif defined(DEBUG)
int unaligned_debug = 0;
#endif

#if defined(UNALIGNED_DEBUG) || defined(DEBUG)
#define DPRINTF(fmt, args...)						\
    do {								\
	if (unaligned_debug)						\
		printf("%s: " fmt, __func__, ##args);			\
    } while (0)
#else
#define	DPRINTF(fmt, args...)	__nothing
#endif

#if defined(DDB) && (defined(UNALIGNED_DEBUG) || defined(DEBUG))
extern vaddr_t opc_disasm(vaddr_t, int);
#define	DISASM(tf, insn)						\
    do {								\
	if (unaligned_debug)						\
		opc_disasm((tf)->tf_srr0, (insn)->i_int);		\
    } while (0)
#else
#define	DISASM(tf, insn)	__nothing
#endif

static bool emul_unaligned(struct trapframe *, ksiginfo_t *,
    const union instr *);
static bool do_lst(struct trapframe *, const union instr *, int);
#ifdef FIX_UNALIGNED_LSTMW
static bool do_lstmw(struct trapframe *, const union instr *, int);
#endif

bool
fix_unaligned(struct trapframe *tf, ksiginfo_t *ksi)
{
	union instr insn;
	int ret;

	KSI_INIT_TRAP(ksi);

	ret = ufetch_32((uint32_t *)tf->tf_srr0, (uint32_t *)&insn.i_int);
	if (ret) {
		UA_EVCNT_INCR(isi);
		DPRINTF("EXC_ISI: ret: %d, srr0: 0x%08lx dear: 0x%08lx\n",
		    ret, tf->tf_srr0, tf->tf_dear);
		ksi->ksi_signo = SIGSEGV;
		ksi->ksi_trap = EXC_ISI;
		ksi->ksi_code = SEGV_MAPERR;
		ksi->ksi_addr = (void *)tf->tf_srr0;
		return true;
	}

	if (emul_unaligned(tf, ksi, &insn))
		return true;

	CTASSERT(sizeof(insn) == 4);	/* It was broken before... */
	tf->tf_srr0 += sizeof(insn);
	return false;
}

#define	UAF_STORE	0
#define	UAF_LOAD	__BIT(0)
#define	UAF_HALF	__BIT(1)
#define	UAF_ALGEBRA	__BIT(2)
#define	UAF_REVERSE	__BIT(3)
#define	UAF_UPDATE	__BIT(4)

static bool
emul_unaligned(struct trapframe *tf, ksiginfo_t *ksi, const union instr *insn)
{
	int flags;

	switch (insn->i_any.i_opcd) {
	case OPC_LWZ:
		UA_EVCNT_INCR(lwz);
		flags = UAF_LOAD;
		break;

	case OPC_LWZU:
		UA_EVCNT_INCR(lwzu);
		flags = UAF_LOAD | UAF_UPDATE;
		break;

	case OPC_STW:
		UA_EVCNT_INCR(stw);
		flags = UAF_STORE;
		break;

	case OPC_STWU:
		UA_EVCNT_INCR(stwu);
		flags = UAF_STORE | UAF_UPDATE;
		break;

	case OPC_LHZ:
		UA_EVCNT_INCR(lhz);
		flags = UAF_LOAD | UAF_HALF;
		break;

	case OPC_LHZU:
		UA_EVCNT_INCR(lhzu);
		flags = UAF_LOAD | UAF_HALF | UAF_UPDATE;
		break;

	case OPC_LHA:
		UA_EVCNT_INCR(lha);
		flags = UAF_LOAD | UAF_HALF | UAF_ALGEBRA;
		break;

	case OPC_LHAU:
		UA_EVCNT_INCR(lhau);
		flags = UAF_LOAD | UAF_HALF | UAF_ALGEBRA | UAF_UPDATE;
		break;

	case OPC_STH:
		UA_EVCNT_INCR(sth);
		flags = UAF_STORE | UAF_HALF;
		break;

	case OPC_STHU:
		UA_EVCNT_INCR(sthu);
		flags = UAF_STORE | UAF_HALF | UAF_UPDATE;
		break;

	case OPC_integer_31:
		switch (insn->i_x.i_xo) {
		case OPC31_LWZX:
			UA_EVCNT_INCR(lwzx);
			flags = UAF_LOAD;
			break;

		case OPC31_LWZUX:
			UA_EVCNT_INCR(lwzux);
			flags = UAF_LOAD | UAF_UPDATE;
			break;

		case OPC31_STWX:
			UA_EVCNT_INCR(stwx);
			flags = UAF_STORE;
			break;

		case OPC31_STWUX:
			UA_EVCNT_INCR(stwux);
			flags = UAF_STORE | UAF_UPDATE;
			break;

		case OPC31_LHZX:
			UA_EVCNT_INCR(lhzx);
			flags = UAF_LOAD | UAF_HALF;
			break;

		case OPC31_LHZUX:
			UA_EVCNT_INCR(lhzux);
			flags = UAF_LOAD | UAF_HALF | UAF_UPDATE;
			break;

		case OPC31_LHAX:
			UA_EVCNT_INCR(lhax);
			flags = UAF_LOAD | UAF_HALF | UAF_ALGEBRA;
			break;
			
		case OPC31_LHAUX:
			UA_EVCNT_INCR(lhaux);
			flags = UAF_LOAD | UAF_HALF | UAF_ALGEBRA | UAF_UPDATE;
			break;

		case OPC31_STHX:
			UA_EVCNT_INCR(sthx);
			flags = UAF_STORE | UAF_HALF;
			break;

		case OPC31_STHUX:
			UA_EVCNT_INCR(sthux);
			flags = UAF_STORE | UAF_HALF | UAF_UPDATE;
			break;

		case OPC31_LWBRX:
			UA_EVCNT_INCR(lwbrx);
			flags = UAF_LOAD | UAF_REVERSE;
			break;

		case OPC31_STWBRX:
			UA_EVCNT_INCR(stwbrx);
			flags = UAF_STORE | UAF_REVERSE;
			break;

		case OPC31_LHBRX:
			UA_EVCNT_INCR(lhbrx);
			flags = UAF_LOAD | UAF_HALF | UAF_REVERSE;
			break;

		case OPC31_STHBRX:
			UA_EVCNT_INCR(sthbrx);
			flags = UAF_STORE | UAF_HALF | UAF_REVERSE;
			break;

		default:
			UA_EVCNT_INCR(unknown);
			goto unknown;
		}
		break;

	case OPC_LMW:
		UA_EVCNT_INCR(lmw);
#ifdef FIX_UNALIGNED_LSTMW
		flags = UAF_LOAD;
		if (do_lstmw(tf, insn, flags))
			goto fault;
		return false;
#else
		goto unknown;
#endif

	case OPC_STMW:
		UA_EVCNT_INCR(stmw);
#ifdef FIX_UNALIGNED_LSTMW
		flags = UAF_STORE;
		if (do_lstmw(tf, insn, flags))
			goto fault;
		return false;
#else
		goto unknown;
#endif

	default:
		UA_EVCNT_INCR(unknown);
 unknown:
		DPRINTF("unknown: srr0: 0x%08lx dear: 0x%08lx "
		    "insn: 0x%08x (opcd: 0x%02x, xo: 0x%03x) ",
		    tf->tf_srr0, tf->tf_dear,
		    insn->i_int, insn->i_any.i_opcd, insn->i_x.i_xo);
		DISASM(tf, insn);
		ksi->ksi_signo = SIGBUS;
		ksi->ksi_trap = EXC_ALI;
		ksi->ksi_addr = (void *)tf->tf_dear;
		return true;
	}

	if (do_lst(tf, insn, flags)) {
#ifdef FIX_UNALIGNED_LSTMW
 fault:
#endif
		UA_EVCNT_INCR(dsi);
		ksi->ksi_signo = SIGSEGV;
		ksi->ksi_trap = EXC_DSI;
		ksi->ksi_code = SEGV_MAPERR;
		ksi->ksi_addr = (void *)tf->tf_dear;
		return true;
	}

	return false;
}

#define	SIGN_EXT(u16, algebra)						\
    ((u16) | (((algebra) && (u16) >= 0x8000) ? 0xffff0000 : 0))

/*
 * We support formats D and X, but don't care which;
 * fault address is in dear.
 */
static bool
do_lst(struct trapframe *tf, const union instr *insn, int flags)
{
	const bool load    = flags & UAF_LOAD,
		   half    = flags & UAF_HALF,
		   algebra = flags & UAF_ALGEBRA,
		   reverse = flags & UAF_REVERSE,
		   update  = flags & UAF_UPDATE;
	uint8_t * const dear = (uint8_t *)tf->tf_dear;
	uint32_t u32;
	uint16_t u16;
	int rs, ra, ret;

	rs = insn->i_d.i_rs;	/* same as i_[dx].i_r[sd] */

	if (load) {
		if (half)
			ret = copyin(dear, &u16, sizeof(u16));
		else
			ret = copyin(dear, &u32, sizeof(u32));
	} else {
		if (half) {
			u16 = (uint16_t)tf->tf_fixreg[rs];
			if (reverse)
				u16 = bswap16(u16);
			ret = copyout(&u16, dear, sizeof(u16));
		} else {
			u32 = tf->tf_fixreg[rs];
			if (reverse)
				u32 = bswap32(u32);
			ret = copyout(&u32, dear, sizeof(u32));
		}
	}

	if (ret)
		goto fault;

	if (load) {
		if (half)
			tf->tf_fixreg[rs] = reverse ?
			    bswap16(u16) : SIGN_EXT(u16, algebra);
		else
			tf->tf_fixreg[rs] = reverse ?
			    bswap32(u32) : u32;
	}

	if (update) {
		ra = insn->i_d.i_ra;	/* same as i_x.i_ra */
		/*
		 * XXX
		 * ra == 0 (or ra == rd for load) is invalid (undefined).
		 * Mimic what AIX 7.2 describes.
		 */
		if (ra == 0 || (load && ra == rs)) {
			UA_EVCNT_INCR(invalid);
			DPRINTF("invalid: rs: %d ra: %d "
			    "srr0: 0x%08lx dear: 0x%08x "
			    "insn: 0x%08x (opcd: 0x%02x xo: 0x%03x) ",
			    rs, ra, tf->tf_srr0, (uint32_t)dear,
			    insn->i_int, insn->i_any.i_opcd, insn->i_x.i_xo);
			DISASM(tf, insn);
			/* XXX discard */
		} else
			tf->tf_fixreg[ra] = (__register_t)dear;
	}

	return false;

 fault:
	DPRINTF("fault: ret: %d srr0: 0x%08lx dear: 0x%08x "
	    "insn: 0x%08x (opcd: 0x%02x xo: 0x%03x) ",
	    ret, tf->tf_srr0, (uint32_t)dear,
	    insn->i_int, insn->i_any.i_opcd, insn->i_x.i_xo);
	DISASM(tf, insn);
	return true;
}

#ifdef FIX_UNALIGNED_LSTMW
static bool
do_lstmw(struct trapframe *tf, const union instr *insn, int flags)
{
	const bool load = flags & UAF_LOAD;
	const size_t size = sizeof(tf->tf_fixreg[0]);
	uint8_t *ea;
	uint32_t u32;
	int rs, ra, r, ret;

	/*
	 * XXX
	 * Can we always assume ea == tf->tf_dear? (True for 403 although...)
	 */
	rs = insn->i_d.i_rs;
	ra = insn->i_d.i_ra;
	ea = (uint8_t *)(insn->i_d.i_d + (ra ? tf->tf_fixreg[ra] : 0));

	for (r = rs; r < 32; r++) {
		if (load)
			ret = copyin(ea, &u32, size);
		else
			ret = copyout(&tf->tf_fixreg[r], ea, size);

		if (ret)
			goto fault;

		if (load) {
			/*
			 * XXX
			 * r == ra is invalid (undefined); Mimic what
			 * AIX 7.2 describes for POWER processors.
			 */
			if (r == ra) {
				UA_EVCNT_INCR(invalid);
				DPRINTF("invalid: rs: %d ra: %d r: %d "
				    "srr0: 0x%08lx dear: 0x%08lx (ea: 0x%08x) "
				    "insn: 0x%08x (opcd: 0x%02x xo: 0x%03x) ",
				    rs, ra, r,
				    tf->tf_srr0, tf->tf_dear, (uint32_t)ea,
				    insn->i_int, insn->i_any.i_opcd,
				    insn->i_x.i_xo);
				DISASM(tf, insn);
				if (r == 0) {
					/* XXX load anyway */
					tf->tf_fixreg[r] = u32;
				} else {
					/* XXX discard */
				}
			} else
				tf->tf_fixreg[r] = u32;
		}

		ea += size;
	}

	return false;

 fault:
	DPRINTF("fault: ret: %d rs: %d r: %d "
	    "srr0: 0x%08lx dear: 0x%08lx (ea: 0x%08x) "
	    "insn: 0x%08x (opcd: 0x%02x xo: 0x%03x) ",
	    ret, rs, r, tf->tf_srr0, tf->tf_dear, (uint32_t)ea,
	    insn->i_int, insn->i_any.i_opcd, insn->i_x.i_xo);
	DISASM(tf, insn);
	return true;
}
#endif /* FIX_UNALIGNED_LSTMW */
