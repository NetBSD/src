/* $NetBSD: wiiu.h,v 1.3 2026/02/01 12:09:40 jmcneill Exp $ */

/*-
 * Copyright (c) 2025-2026 Jared McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Nintendo Wii U platform definitions.
 */

#ifndef _WIIU_H
#define _WIIU_H

#include <machine/wii.h>

#define WIIU_MEM1_BASE			0x00000000
#define WIIU_MEM1_SIZE			0x02000000	/* 32 MB */
#define WIIU_MEM0_BASE			0x08000000
#define WIIU_MEM0_SIZE			0x00300000	/* 3 MB */
#define WIIU_MEM2_BASE			0x10000000
#define WIIU_MEM2_SIZE			0x80000000	/* 2 GB */

#define WIIU_GFX_TV_BASE		0x17500000
#define WIIU_GFX_DRC_BASE		0x178c0000

#define WIIU_BUS_FREQ_HZ		248625000
#define WIIU_CPU_FREQ_HZ		(WIIU_BUS_FREQ_HZ * 5)
#define WIIU_TIMEBASE_FREQ_HZ		(WIIU_BUS_FREQ_HZ / 4)

#define WIIU_PI_BASE			0x0c000000

#define WIIU_GX2_BASE			0x0c200000
#define WIIU_GX2_SIZE			0x80000

#define WIIU_DSP_BASE			0x0c280000

/* Processor interface registers */
#define WIIU_PI_INTSR(n)		(WIIU_PI_BASE + 0x78 + (n) * 8)
#define WIIU_PI_INTMSK(n)		(WIIU_PI_BASE + 0x7c + (n) * 8)

/* Latte IRQs */
#define WIIU_PI_IRQ_MB_CPU(n)		(20 + (n))
#define WIIU_PI_IRQ_GPU7		23

/* Latte registers */
#define LT_PPCnINT1STS(n)		(HOLLYWOOD_PRIV_BASE + 0x440 + (n) * 0x10)
#define LT_PPCnINT2STS(n)		(HOLLYWOOD_PRIV_BASE + 0x444 + (n) * 0x10)
#define LT_PPCnINT1EN(n)		(HOLLYWOOD_PRIV_BASE + 0x448 + (n) * 0x10)
#define LT_PPCnINT2EN(n)		(HOLLYWOOD_PRIV_BASE + 0x44c + (n) * 0x10)
#define LT_IOPINT1STS			LT_PPCnINT1STS(3)
#define LT_IOPINT2STS			LT_PPCnINT2STS(3)
#define LT_IOPIRQINT1EN			LT_PPCnINT1EN(3)
#define LT_IOPIRQINT2EN			LT_PPCnINT2EN(3)
#define LT_CHIPREVID			(HOLLYWOOD_PRIV_BASE + 0x5a0)
#define  LT_CHIPREVID_MAGIC		__BITS(31, 16)
#define  LT_CHIPREVID_MAGIC_CAFE	0xCAFE
#define  LT_CHIPREVID_VERHI		__BITS(7, 4)
#define  LT_CHIPREVID_VERLO		__BITS(3, 0)
#define LT_PIMCOMPAT			(HOLLYWOOD_PRIV_BASE + 0x5b0)
#define  PPC_COMPAT			__BIT(5)
#define LT_GPUINDADDR			(HOLLYWOOD_PRIV_BASE + 0x620)
#define  LT_GPUINDADDR_REGSPACE_GPU	(0x3U << 30)
#define LT_GPUINDDATA			(HOLLYWOOD_PRIV_BASE + 0x624)

/* GPIOs */
#define WIIU_GPIO_POWER			0

/* Boot vector */
#define WIIU_BOOT_VECTOR		0x08100100

/* linux-loader command line protocol */
#define WIIU_LOADER_DATA_ADDR		0x89200000
#define WIIU_LOADER_MAGIC		0xcafefeca
struct wiiu_argv {
	uint32_t	magic;
	char		cmdline[256];
	uint32_t	initrd;
	uint32_t	initrd_len;
};

/* Declared in sys/arch/evbppc/nintendo/machdep.c */
extern bool wiiu_plat;
extern bool wiiu_native;

#endif /* !_WIIU_H */
