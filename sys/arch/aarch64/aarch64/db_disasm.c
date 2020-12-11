/* $NetBSD: db_disasm.c,v 1.11 2020/12/11 18:03:33 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: db_disasm.c,v 1.11 2020/12/11 18:03:33 skrll Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd32.h"
#endif

#include <sys/param.h>
#include <machine/db_machdep.h>
#include <ddb/db_interface.h>
#include <ddb/db_sym.h>
#include <ddb/db_output.h>
#include <ddb/db_access.h>
#include <ddb/db_user.h>

#include <aarch64/machdep.h>
#include <arch/aarch64/aarch64/disasm.h>

#include <arm/cpufunc.h>

static uint32_t
db_disasm_readword(uintptr_t address)
{
	return db_get_value(address, sizeof(uint32_t), false);
}

static void
db_disasm_printaddr(uintptr_t address)
{
	db_printf("%lx <", address);
	db_printsym((db_addr_t)address, DB_STGY_ANY, db_printf);
	db_printf(">");
}

static const disasm_interface_t db_disasm_interface = {
	.di_readword = db_disasm_readword,
	.di_printaddr = db_disasm_printaddr,
	.di_printf = db_printf
};

db_addr_t
db_disasm(db_addr_t loc, bool altfmt)
{
	return disasm(&db_disasm_interface, loc);
}


static char *strdisasm_ptr;
static char strdisasm_buf[256];

static uint32_t
strdisasm_readword(uintptr_t address)
{
#ifdef _KERNEL
	/*
	 * if it cannot be read due to a EFAULT etc.,
	 * ignores the error and returns 0
	 */
	uint32_t word = 0;

	switch (aarch64_addressspace((vaddr_t)address)) {
	case AARCH64_ADDRSPACE_UPPER:
		kcopy((void*)address, &word, sizeof(word));
		break;
	case AARCH64_ADDRSPACE_LOWER:
		ufetch_32((uint32_t *)address, &word);
		break;
	default:
		break;
	}

	return word;
#else
	return *(uint32_t *)address;
#endif
}

static void __printflike(1, 2)
strdisasm_printf(const char *fmt, ...)
{
	va_list ap;
	int len;

	/* calculation spaces to append a string */
	len = strdisasm_buf + sizeof(strdisasm_buf) - strdisasm_ptr;

	va_start(ap, fmt);
	len = vsnprintf(strdisasm_ptr, len, fmt, ap);
	va_end(ap);

	strdisasm_ptr += len;
}

static void
strdisasm_printaddr(uintptr_t address)
{
	strdisasm_printf("0x%lx", address);
}

static const disasm_interface_t strdisasm_interface = {
	.di_readword = strdisasm_readword,
	.di_printaddr = strdisasm_printaddr,
	.di_printf = strdisasm_printf
};

const char *
strdisasm(vaddr_t pc, uint64_t spsr)
{
#ifdef COMPAT_NETBSD32
	if (spsr & SPSR_A32) {
		uint32_t insn = 0;
		int size;
		const char *arch = (spsr & SPSR_A32_T) ? "T32" : "A32";

		size = fetch_arm_insn(pc, spsr, &insn);
		if (size != 2)
			size = 4;
		snprintf(strdisasm_buf, sizeof(strdisasm_buf),
		    ".insn 0x%0*x (%s)", size * 2, insn, arch);
	} else
#endif /* COMPAT_NETBSD32 */
	{
		char *p;

		/* disasm an aarch64 instruction */
		strdisasm_ptr = strdisasm_buf;
		disasm(&strdisasm_interface, (db_addr_t)pc);

		/* replace tab to space, and chomp '\n' */
		for (p = strdisasm_buf; *p != '\0'; p++) {
			if (*p == '\t')
				*p = ' ';
		}
		if ((p > strdisasm_buf) && (p[-1] == '\n'))
			p[-1] = '\0';
	}

	return strdisasm_buf;
}
