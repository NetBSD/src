/*	$NetBSD: pmppc.h,v 1.1 2002/05/30 20:02:04 augustss Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/* SDRAM */
#define PMPPC_SDRAM_BASE	0x00000000

/* Flash */
#define PMPPC_FLASH_BASE	0x70000000

#define PMPPC_IO_START		0x7fe00000

/* CS8900A ethernet */
#define PMPPC_CS_IO_BASE	0x7fe00000
#define PMPPC_CS_IO		0x7fe00c00
#define PMPPC_CS_MEM		0x7fe04000

/* time-of-day clock */
#define PMPPC_RTC		0x7ff00000
#define PMPPC_RTC_SIZE		0x00002000

/* board config regs */
#define PMPPC_CONFIG0		0x7ff40000
#define PMPPC_CONFIG1		0x7ff40001
#define PMPPC_LEDS		0x7ff40002
#define PMPPC_RESET		0x7ff40003
#define PMPPC_RESET_SEQ_STEP1		0xac
#define PMPPC_RESET_SEQ_STEP2		0x1d
#define PMPPC_INTR		0x7ff40004

/* ROM */
#define PMPPC_ROM_BASE		0x7ff80000

void setleds(int leds);

/* Interrupts */
#define PMPPC_I_BPMC_INTA	CPC_IB_EXT0 /* PCI INTA */
#define PMPPC_I_BPMC_INTB	CPC_IB_EXT1 /* PCI INTB */
#define PMPPC_I_BPMC_INTC	CPC_IB_EXT2 /* PCI INTC */
#define PMPPC_I_BPMC_INTD	CPC_IB_EXT3 /* PCI INTD */
#define PMPPC_I_ETH_INT		CPC_IB_EXT4 /* ethernet */
#define PMPPC_I_RTC_INT		CPC_IB_EXT5 /* rtc */


/* 
 * The variables below are extracted from the config register located
 * at PMPPC_CONFIG.
 */
struct {
	int a_boot_device;
#define A_BOOT_ROM 0
#define A_BOOT_FLASH 1
	int a_has_ecc;
	uint a_mem_size;		/* in bytes */
	uint a_l2_cache;
#define A_CACHE_PARITY 0
#define A_CACHE_NO_PARITY 1
#define A_CACHE_NONE 3
	uint a_bus_freq;		/* in hz */
	int a_is_monarch;
	int a_has_eth;
	int a_has_rtc;
	uint a_flash_size;		/* in bytes */
	uint a_flash_width;		/* in bits */
} a_config;
