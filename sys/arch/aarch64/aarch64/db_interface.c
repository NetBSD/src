/* $NetBSD: db_interface.c,v 1.2 2018/04/01 04:35:03 ryo Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.2 2018/04/01 04:35:03 ryo Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <uvm/uvm.h>

#include <aarch64/db_machdep.h>
#include <aarch64/machdep.h>
#include <aarch64/pmap.h>

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

void
db_write_bytes(vaddr_t addr, size_t size, const char *data)
{
	vaddr_t lastpage = -1;
	char *dst;

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

db_addr_t
db_branch_taken(db_expr_t inst, db_addr_t pc, db_regs_t *regs)
{
	/* XXX */
	return pc + 4;
}

bool
db_inst_unconditional_flow_transfer(db_expr_t inst)
{
	/* XXX */
	return false;
}

