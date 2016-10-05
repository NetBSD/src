/*	$NetBSD: sbbuswatch.c,v 1.2.34.1 2016/10/05 20:55:32 skrll Exp $	*/
/*
 * Copyright (c) 2010, The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <mips/cpu.h>
#include <mips/locore.h>

#include <mips/sibyte/include/sb1250_int.h>
#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/dev/sbbuswatchvar.h>

#define READ_REG(rp)            mips3_ld((register_t)(rp))
#define WRITE_REG(rp, val)      mips3_sd((register_t)(rp), (val))

static void sibyte_bus_watch_intr(void *, uint32_t, vaddr_t);

void
sibyte_bus_watch_init(void)
{
	(void)READ_REG(MIPS_PHYS_TO_KSEG1(A_SCD_BUS_ERR_STATUS));
	WRITE_REG(MIPS_PHYS_TO_KSEG1(A_BUS_L2_ERRORS), 0);
	WRITE_REG(MIPS_PHYS_TO_KSEG1(A_BUS_MEM_IO_ERRORS), 0);

	(void)cpu_intr_establish(K_INT_BAD_ECC, IPL_DDB,
		sibyte_bus_watch_intr, (void *)K_INT_BAD_ECC);
	(void)cpu_intr_establish(K_INT_COR_ECC, IPL_DDB,
		sibyte_bus_watch_intr, (void *)K_INT_COR_ECC);
	(void)cpu_intr_establish(K_INT_IO_BUS, IPL_DDB,
		sibyte_bus_watch_intr, (void *)K_INT_IO_BUS);
}

int
sibyte_bus_watch_check(unsigned int cause)
{
	uint64_t err_ctl;
	uint64_t cache_err_i;
	uint64_t cache_err_d;
	uint64_t cache_err_dpa;
	uint64_t bus_err_dpa;
	uint32_t bus_err_status;
	uint32_t l2_errors;
	uint32_t mem_io_errors;

	bus_err_status = READ_REG(
		MIPS_PHYS_TO_KSEG1(A_SCD_BUS_ERR_STATUS));

	if (bus_err_status == 0)
		return 0;

	l2_errors = READ_REG(
		MIPS_PHYS_TO_KSEG1(A_BUS_L2_ERRORS));
	if (l2_errors != 0)
		WRITE_REG(MIPS_PHYS_TO_KSEG1(A_BUS_L2_ERRORS), 0);

	mem_io_errors = READ_REG(
		MIPS_PHYS_TO_KSEG1(A_BUS_MEM_IO_ERRORS));
	if (mem_io_errors != 0)
		WRITE_REG(MIPS_PHYS_TO_KSEG1(A_BUS_MEM_IO_ERRORS), 0);

	asm volatile("dmfc0 %0, $26, 0;" : "=r"(err_ctl));
	asm volatile("dmfc0 %0, $26, 1;" : "=r"(bus_err_dpa));
	asm volatile("dmfc0 %0, $27, 0;" : "=r"(cache_err_i));
	asm volatile("dmfc0 %0, $27, 1;" : "=r"(cache_err_d));
	asm volatile("dmfc0 %0, $27, 3;" : "=r"(cache_err_dpa));

	printf("bus_err_status=%#x\n", bus_err_status);
	printf("l2_errors=%#x\n", l2_errors);
	printf("mem_io_errors=%#x\n", mem_io_errors);
	printf("err_ctl=%#"PRIx64"\n", err_ctl);
	printf("bus_err_dpa=%#"PRIx64"\n", bus_err_dpa);
	printf("cache_err_i=%#"PRIx64"\n", cache_err_i);
	printf("cache_err_d=%#"PRIx64"\n", cache_err_d);
	printf("cache_err_dpa=%#"PRIx64"\n", cache_err_dpa);

	return -1;
}

static void
sibyte_bus_watch_intr(void *arg, uint32_t status, vaddr_t pc)
{
	printf("%s: %p\n", __func__, arg);
	(void)sibyte_bus_watch_check(0);
}
