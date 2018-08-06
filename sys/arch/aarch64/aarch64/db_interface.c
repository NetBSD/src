/* $NetBSD: db_interface.c,v 1.5 2018/08/06 12:50:56 ryo Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.5 2018/08/06 12:50:56 ryo Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <uvm/uvm.h>
#include <uvm/uvm_prot.h>

#include <aarch64/db_machdep.h>
#include <aarch64/machdep.h>
#include <aarch64/pmap.h>
#include <aarch64/cpufunc.h>

#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>

#include <dev/cons.h>

void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{
	vaddr_t lastpage = -1;
	const char *src;

	for (src = (const char *)addr; size > 0;) {
		uintptr_t tmp;

		if ((lastpage != atop((vaddr_t)src)) &&
		    vtophys((vaddr_t)src) == VTOPHYS_FAILED) {
			db_printf("address %p is invalid\n", src);
			memset(data, 0, size);	/* stubs are filled by zero */
			return;
		}
		lastpage = atop((vaddr_t)src);

		tmp = (uintptr_t)src | (uintptr_t)data;
		if ((size >= 8) && ((tmp & 7) == 0)) {
			*(uint64_t *)data = *(const uint64_t *)src;
			src += 8;
			data += 8;
			size -= 8;
		} else if ((size >= 4) && ((tmp & 3) == 0)) {
			*(uint32_t *)data = *(const uint32_t *)src;
			src += 4;
			data += 4;
			size -= 4;
		} else if ((size >= 2) && ((tmp & 1) == 0)) {
			*(uint16_t *)data = *(const uint16_t *)src;
			src += 2;
			data += 2;
			size -= 2;
		} else {
			*data++ = *src++;
			size--;
		}
	}
}

static void
db_write_text(vaddr_t addr, size_t size, const char *data)
{
	pt_entry_t *ptep, pte;
	size_t s;

	/*
	 * consider page boundary, and
	 * it works even if kernel_text is mapped with L2 or L3.
	 */
	if (atop(addr) != atop(addr + size - 1)) {
		s = PAGE_SIZE - (addr & PAGE_MASK);
		db_write_text(addr, s, data);
		addr += s;
		size -= s;
		data += s;
	}
	while (size > 0) {
		ptep = kvtopte(addr);
		KASSERT(ptep != NULL);

		/* save pte */
		pte = *ptep;

		/* change to writable */
		pmap_kvattr(addr, VM_PROT_READ|VM_PROT_WRITE);
		aarch64_tlbi_all();

		s = size;
		if (size > PAGE_SIZE)
			s = PAGE_SIZE;

		memcpy((void *)addr, data, s);
		cpu_icache_sync_range(addr, size);

		/* restore pte */
		*ptep = pte;
		aarch64_tlbi_all();

		addr += s;
		size -= s;
		data += s;
	}
}

void
db_write_bytes(vaddr_t addr, size_t size, const char *data)
{
	vaddr_t kernstart, datastart;
	vaddr_t lastpage = -1;
	char *dst;

	/* if readonly page, require changing attribute to write */
	extern char __kernel_text[], __data_start[];
	kernstart = trunc_page((vaddr_t)__kernel_text);
	datastart = trunc_page((vaddr_t)__data_start);
	if (kernstart <= addr && addr < datastart) {
		size_t s;

		s = datastart - addr;
		if (s > size)
			s = size;
		db_write_text(addr, s, data);
		addr += s;
		size -= s;
		data += s;
	}

	/* XXX: need to check read only block/page */
	for (dst = (char *)addr; size > 0;) {
		uintptr_t tmp;

		if ((lastpage != atop((vaddr_t)dst)) &&
		    (vtophys((vaddr_t)dst) == VTOPHYS_FAILED)) {
			db_printf("address %p is invalid\n", dst);
			return;
		}
		lastpage = atop((vaddr_t)dst);

		tmp = (uintptr_t)dst | (uintptr_t)data;
		if ((size >= 8) && ((tmp & 7) == 0)) {
			*(uint64_t *)dst = *(const uint64_t *)data;
			dst += 8;
			data += 8;
			size -= 8;
		} else if ((size >= 4) && ((tmp & 3) == 0)) {
			*(uint32_t *)dst = *(const uint32_t *)data;
			dst += 4;
			data += 4;
			size -= 4;
		} else if ((size >= 2) && ((tmp & 1) == 0)) {
			*(uint16_t *)dst = *(const uint16_t *)data;
			dst += 2;
			data += 2;
			size -= 2;
		} else {
			*dst++ = *data++;
			size--;
		}
	}
}

/*
 * return register value of $X0..$X30, $SP or 0($XZR)
 */
static uint64_t
db_fetch_reg(unsigned int reg, db_regs_t *regs, bool use_sp)
{
	if (reg >= 32)
		panic("db_fetch_reg: botch");

	if (reg == 31) {
		/* $SP or $XZR */
		return use_sp ? regs->tf_sp : 0;
	}
	return regs->tf_reg[reg];
}

static inline uint64_t
SignExtend(int bitwidth, uint64_t imm, unsigned int multiply)
{
	const uint64_t signbit = ((uint64_t)1 << (bitwidth - 1));
	const uint64_t immmax = signbit << 1;

	if (imm & signbit)
		imm -= immmax;
	return imm * multiply;
}

db_addr_t
db_branch_taken(db_expr_t inst, db_addr_t pc, db_regs_t *regs)
{
#define INSN_FMT_RN(insn)		(((insn) >> 5) & 0x1f)
#define INSN_FMT_IMM26(insn)	((insn) & 0x03ffffff)
#define INSN_FMT_IMM19(insn)	(((insn) >> 5) & 0x7ffff)
#define INSN_FMT_IMM14(insn)	(((insn) >> 5) & 0x3fff)

	if (((inst & 0xfffffc1f) == 0xd65f0000) ||	/* ret xN */
	    ((inst & 0xfffffc1f) == 0xd63f0000) ||	/* blr xN */
	    ((inst & 0xfffffc1f) == 0xd61f0000)) {	/* br xN */
		return db_fetch_reg(INSN_FMT_RN(inst), regs, false);
	}

	if (((inst & 0xfc000000) == 0x94000000) ||	/* bl imm */
	    ((inst & 0xfc000000) == 0x14000000)) {	/* b imm */
		return SignExtend(26, INSN_FMT_IMM26(inst), 4) + pc;
	}

	if (((inst & 0xff000010) == 0x54000000) ||	/* b.cond */
	    ((inst & 0x7f000000) == 0x35000000) ||	/* cbnz */
	    ((inst & 0x7f000000) == 0x34000000)) {	/* cbz */
		return SignExtend(19, INSN_FMT_IMM19(inst), 4) + pc;
	}

	if (((inst & 0x7f000000) == 0x37000000) ||	/* tbnz */
	    ((inst & 0x7f000000) == 0x36000000)) {	/* tbz */
		return SignExtend(14, INSN_FMT_IMM14(inst), 4) + pc;
	}

	panic("branch_taken: botch");
}

bool
db_inst_unconditional_flow_transfer(db_expr_t inst)
{
	if (((inst & 0xfffffc1f) == 0xd65f0000) ||	/* ret xN */
	    ((inst & 0xfc000000) == 0x94000000) ||	/* bl */
	    ((inst & 0xfffffc1f) == 0xd63f0000) ||	/* blr */
	    ((inst & 0xfc000000) == 0x14000000) ||	/* b imm */
	    ((inst & 0xfffffc1f) == 0xd61f0000))	/* br */
		return true;

#define INSN_FMT_COND(insn)	((insn) & 0xf)
#define CONDITION_AL	14

	if (((inst & 0xff000010) == 0x54000000) &&	/* b.cond */
	    (INSN_FMT_COND(inst) == CONDITION_AL))	/* always? */
		return true;

	return false;
}
