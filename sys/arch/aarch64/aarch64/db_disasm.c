/* $NetBSD: db_disasm.c,v 1.3 2018/07/17 10:07:49 ryo Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: db_disasm.c,v 1.3 2018/07/17 10:07:49 ryo Exp $");

#include <sys/param.h>
#include <machine/db_machdep.h>
#include <ddb/db_interface.h>
#include <ddb/db_sym.h>
#include <ddb/db_output.h>
#include <ddb/db_access.h>

#include <arch/aarch64/aarch64/disasm.h>

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
	return *(uint32_t *)address;
}

static void
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
strdisasm(vaddr_t pc)
{
	char *p;

	strdisasm_ptr = strdisasm_buf;
	disasm(&strdisasm_interface, (db_addr_t)pc);

	/* replace tab to space, and chomp '\n' */
	for (p = strdisasm_buf; *p != '\0'; p++) {
		if (*p == '\t')
			*p = ' ';
	}
	if ((p > strdisasm_buf) && (p[-1] == '\n'))
		p[-1] = '\0';

	return strdisasm_buf;
}
