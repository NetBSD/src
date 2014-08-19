/* $NetBSD: db_interface.c,v 1.1.4.2 2014/08/20 00:02:39 tls Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: db_interface.c,v 1.1.4.2 2014/08/20 00:02:39 tls Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <aarch64/db_machdep.h>
#include <aarch64/locore.h>

#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>

#include <dev/cons.h>

int db_active = 0;

void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{

	while (size > 0) {
		const uint8_t * const dp = (void *)addr;
		if (vtophys(addr) == VTOPHYS_FAILED) {
			db_printf("address %p is invalid\n", dp);
		}

		uintptr_t tmp = addr | (uintptr_t)data;
		if (size == 8 && (tmp & 7) == 0) {
			*(uint64_t *)data = *(const uint64_t *)dp;
			size -= 8;
			data += 8;
			continue;
		}

		if (size == 4 && (tmp & 3) == 0) {
			*(uint32_t *)data = *(const uint32_t *)dp;
			size -= 4;
			data += 4;
			continue;
		}

		if (size == 1 && (tmp & 1) == 0) {
			*(uint16_t *)data = *(const uint16_t *)dp;
			size -= 2;
			data += 2;
			continue;
		}

		*(uint8_t *)data = *(const uint8_t *)dp;
		size -= 1;
		data += 1;
	}
}

void
db_write_bytes(vaddr_t addr, size_t size, const char *data)
{

	while (size > 0) {
		uint8_t * const dp = (void *)addr;
		if (vtophysw(addr) == VTOPHYS_FAILED) {
			db_printf("address %p is invalid\n", dp);
		}

		uintptr_t tmp = addr | (uintptr_t)data;
		if (size == 8 && (tmp & 7) == 0) {
			*(uint64_t *)dp = *(const uint64_t *)data;
			size -= 8;
			data += 8;
			continue;
		}

		if (size == 4 && (tmp & 3) == 0) {
			*(uint32_t *)dp = *(const uint32_t *)data;
			size -= 4;
			data += 4;
			continue;
		}

		if (size == 1 && (tmp & 1) == 0) {
			*(uint16_t *)dp = *(const uint16_t *)data;
			size -= 2;
			data += 2;
			continue;
		}

		*(uint8_t *)dp = *(const uint8_t *)data;
		size -= 1;
		data += 1;
	}
}
