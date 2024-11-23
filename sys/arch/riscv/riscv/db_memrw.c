/*	$NetBSD: db_memrw.c,v 1.1 2024/11/23 12:03:55 skrll Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>

__RCSID("$NetBSD: db_memrw.c,v 1.1 2024/11/23 12:03:55 skrll Exp $");

#include <sys/param.h>

#include <riscv/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_output.h>


void
db_read_bytes(db_addr_t addr, size_t len, char *data)
{
	const char *src = (char *)addr;
	int err;

	/* If asked to fetch from userspace, do it safely */
	if ((intptr_t)addr >= 0) {
		err = copyin(src, data, len);
		if (err) {
#ifdef DDB
			db_printf("address %p is invalid\n", src);
#endif
			memset(data, 0, len);
		}
		return;
	}

	while (len--) {
		*data++ = *src++;
	}
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, size_t len, const char *data)
{
	int err;

	/* If asked to fetch from userspace, do it safely */
	if ((intptr_t)addr >= 0) {
		err = copyout(data, (char *)addr, len);
		if (err) {
#ifdef DDB
			db_printf("address %p is invalid\n", (char *)addr);
#endif
		}
		return;
	}

	if (len == 8) {
		*(uint64_t *)addr = *(const uint64_t *) data;
	} else if (len == 4) {
		*(uint32_t *)addr = *(const uint32_t *) data;
	} else if (len == 2) {
		*(uint16_t *)addr = *(const uint16_t *) data;
	} else {
		KASSERT(len == 1);
		*(uint8_t *)addr = *(const uint8_t *) data;
	}
	__asm("fence rw,rw; fence.i");
}
