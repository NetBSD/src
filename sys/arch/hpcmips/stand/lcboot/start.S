/* $NetBSD: start.S,v 1.2 2003/08/09 08:01:49 igy Exp $ */

/*
 * Copyright (c) 2003 Naoto Shimazaki.
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
 * THIS SOFTWARE IS PROVIDED BY NAOTO SHIMAZAKI AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE NAOTO OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NOTE:
 * This code assumes some trick described below:
 *
 *	- Located at 0x80000000 by linker
 *	- Placed at 0xbfc00000 (by ROM writer)
 *	- Executed at 0xbfc00000 by CPU
 *
 * So,
 *
 *	- You cannot use 'j' and 'jal'.  Instead, you must use 'b'.
 *	- If you want to jump to absolute address, you must load
 *	  the target address to a register and jump to it with
 *	  'jr' or 'jalr'.
 *	- You never be able to write any memory before
 *	  the bus configuration completed.
 *
 */

#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/param.h>
#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <mips/trap.h>

#include "extern.h"

	.text
	.set	noreorder
	.align	2

/*
 * macro ROMICE  - support for Kyoto-micro's PARTNER-ETII ROM-ICE
 *
 * PARTNER-ETII by Kyoto-microcomputer is a ROM based emulater.
 * This ICE initializes by itself the very early configurations of
 * the target CPU.  This macro skips that configurations.
 */
#ifndef ROMICE
	/*
	 * exception vector table
	 */
	.org	0x0000
reset_vector:
	b	start		/* MUST relative jump */
	nop

	.org	0x0200
tlb_vector:
	b	start
	nop

	.org	0x0280
xtlb_vector:
	b	start
	nop
	
	.org	0x0380
exception_vector:
	b	start
	nop
#endif

	.org	0x1000
	.globl	start
start:
#ifndef ROMICE
	/*
	 * setup CP0 CONFIG
	 * EP = 0, AD = 0, K0 = 2
	 */
	li	t1, 0x00125482
	mtc0	t1, $16
	
	/*
	 * setup CP0 STATUS
	 * CU0 = 0, RE = 0, DS:BEV = 0, IM = 0, KX = SX = UX = 0,
	 * KSU = 0, IE = 0, others = untouch
	 */
	mfc0	t1, $12
	li	t2, 0x00770006
	and	t1, t1, t2
	li	t2, 0x00400000
	or	t1, t1, t2
	mtc0	t1, $12

	mtc0	zero, $18		/* CP0 Watch Lo */
	mtc0	zero, $11		/* CP0 compare */

	/*
	 * setup LED
	 */
	li	t0, 0xab000248		/* LEDCNTREG */
	li	t1, 0x0001
	sh	t1, (t0)

	/*
	 * reset HALTimer
	 */
	li	t0, 0xab0000a2
	li	t1, 0x0004
	sh	t1, (t0)

	/*
	 * initialize VR4181 bus controller
	 */

	/*
	 * setup BCUCNTREG1
	 * ROMs = 10 (64Mbit), ROMWEN0 = 1, Rtype = 01 (flash)
	 * RSTOUT = 1 (inactive)
	 */
	li	t0, 0xaa000000		/* BCUCNTREG1 */
	li	t1, 0x8013
	sh	t1, (t0)

	/*
	 * setup BCURFCNTREG
	 * BRF = refresh cycle x 1/TClock
	 *     = 30.52usec x 32.768MHz
	 *     = 0x3e8	(1000 TClock)
	 */
	li	t0, 0xaa000010		/* BCURFCNTREG */
	li	t1, 0x03e8
	sh	t1, (t0)

	/*
	 * setup BCUSPEEDREG
	 * WPROM = 111 = 8.5TClock = 259ns
	 * WROMA = 1000 = 9.5TClock = 290ns
	 */
	li	t0, 0xaa00000c		/* BCUSPEEDREG */
	li	t1, 0x7008
	sh	t1, (t0)

	/*
	 * setup SDTIMINGREG
	 * BIT8 = 1 (always 1)
	 * TRAS = 01 = 5SDCLK	(forced under 66, 50, 33MHz bus clock)
	 * TRC  = 01 = 7SDCLK	(forced under 66, 50, 33MHz bus clock)
	 * TRP  = 10 = 3SDCLK	(forced under 66, 50, 33MHz bus clock)
	 * TRCP = 01 = 2SDCLK	(forced under 66, 50, 33MHz bus clock)
	 */
	li	t0, 0xaa00030c		/* SDTIMINGREG */
	li	t1, 0x0159
	sh	t1, (t0)

	/*
	 * To initialize 64Mbit SDRAM properly, we have to take
	 * following steps:
	 * 
	 * 	1. set MEMCFG_REG for 16Mbit SDRAM
	 * 	2. setup MODE_REG
	 * 	3. init SDRAM (setting MEMCFG_REG:Init to 1)
	 * 	4. set MEMCFG_REG for 64Mbit SDRAM
	 *
	 * confirm to VR4181 users manual 6.5.2 MEMCFG_REG (page 142).
	 * (the page number is for Japanese edition.  it might be 
	 *  at another page number for the English edition.)
	 */

	/*
	 * first, say MEMCFG_REG that SDRAM is 16Mbit
	 * Init = 0
	 * B1Config = 01	(16Mbit)
	 * Bstreftype = 1	(all raw refresh)
	 * BstRefr = 0		(not allow burst refresh)
	 * EDOAsym = 0		(asymetric)
	 * B0Config = 01	(16Mbit)
	 * EDO/SDRAM = 1	(SDRAM)
	 */
	li	t0, 0xaa000304		/* MEMCFG_REG <- 503 (16Mbit) */
	li	t1, 0x0503
	sh	t1, (t0)

	/*
	 * second, setup MODE_REG
	 * Bit11 = 0		(always 0)
	 * Bit10 = 0		(always 0)
	 * BR-SW = 0		(always 0)
	 * TE-Ven = 00		(always 00)
	 * LTMode = 011		(3clock CAS latency)
	 * WT = 0		(always 0)
	 * BL = 111		(always 111)
	 */
	li	t0, 0xaa000308		/* MODE_REG */
	li	t1, 0x0037
	sh	t1, (t0)

	/*
	 * third, kick SDRAM initialization
	 * Init = 1
	 * other = untouched
	 */
	li	t0, 0xaa000304		/* MEMCFG_REG:Init <- 1 */
	li	t1, 0x8503
	sh	t1, (t0)

	/*
	 * final, say MEMCFG_REG that SDRAM is 16Mbit
	 * Init = 0
	 * B1Config = 10	(64Mbit)
	 * Bstreftype = 1	(all raw refresh)
	 * BstRefr = 0		(not allow burst refresh)
	 * EDOAsym = 0		(asymetric)
	 * B0Config = 10	(64Mbit)
	 * EDO/SDRAM = 1	(SDRAM)
	 */
	li	t0, 0xaa000304		/* MEMCFG_REG */
	li	t1, 0x0905
	sh	t1, (t0)

	/*
	 * setup XISACTL
	 * EXTRESULT = 1	(1 is recommended)
	 * INTRESULT = 0	(0 is recommended)
	 * EXBUFEN = 0		(use SYSDIR and SYSEN)
	 * MEMWS = 00		(1.5 SYSCLK)
	 * IOWS = 10		(2.5 SYSCLK)
	 * SCLKDIV = 10		(PCLK/6)
	 */
	li	t0, 0xab0002c4		/* XISACTL */
	li	t1, 0x0422
	sh	t1, (t0)
	nop


	/*
	 *  enable cache
	 */
	mfc0	t0, $16
	li	t1, 0xfffffff8
	and	t0, t0, t1
	or	t0, t0, 0x00000003	/* K0 = 3 */
	mtc0	t0, $16			/* config */
	nop
	nop
	nop

	/*
	 * initialize cache
	 */
	mtc0	zero, $28		/* TagLo */

	lui	t0, 0x8000		/* vaddr */
	ori	t1, zero, 0x1000	/* cache size = 4KB */
cache_clear:
	.set	push
	.set	mips3
	cache	0x00, (t0)		/* Index_Invalidate */
	cache	0x09, (t0)		/* Index_Store_Tag */
	.set	pop
	addiu	t1, t1, -0x10
	bgtz	t1, cache_clear
	addiu	t0, t0, 0x10		/* increment of line size */


	/* LED3 ON */
	li	t0, 0xab000306
	li	t1, 0x0800
	sh	t1, (t0)

	li	t0, 0xab000308
	sh	zero, (t0)
	nop
	/* LED3 ON */

	/*
	 * now early bus configuration is done.
	 */


	/*
	 *  copy bootloader ROM to RAM
	 */
	li	t1, LCBOOT_ROMSTARTADDR
	la	t2, start
	la	t3, edata
1:
	lw	t0, (t1)
	nop
	sw	t0, (t2)
	addu	t2, t2, 4
	sltu	t0, t2, t3
	.set	push
	.set	noreorder
	.set	nomacro
	bne	t0, zero, 1b
	addu	t1, t1, 4
	.set	pop
	

	/* verify */
	li	t1, LCBOOT_ROMSTARTADDR
	la	t2, start
	la	t3, edata
1:
	lw	t0, (t1)
	lw	t4, (t2)
	addu	t2, t2, 4
	bne	t0, t4, 2f
	sltu	t0, t2, t3
	.set	push
	.set	noreorder
	.set	nomacro
	bne	t0, zero, 1b
	addu	t1, t1, 4
	.set	pop
	b	4f
	nop
2:
	/* panic. stop LED */
	li	t0, 0xab000248		/* LEDCNTREG */
	sh	zero, (t0)
3:
	b	3b
	nop
4:	
	/* verify done */
		

	/* LED4 ON */
	li	t0, 0xab000306
	li	t1, 0x8800
	sh	t1, (t0)

	li	t0, 0xab000308
	sh	zero, (t0)
	/* LED4 ON */

	/*
	 * now we've got a working RAM with cache.
	 */


#else /* !ROMICE */
	/*
	 * enable cache
	 */
	mfc0	t0, $16
	li	t1, 0xfffffff8
	and	t0, t0, t1
	or	t0, t0, 0x00000003	/* K0 = 3 */
	mtc0	t0, $16			/* config */
	nop
	nop
	nop
#endif /* !ROMICE */


	/*
	 * zero the bss
	 */
	la	t1, edata
	la	t2, end
	sw	zero, (t1)
1:
	addu	t1, t1, 4
	.set	push
	.set	mips3
	.set	noreorder
	.set	nomacro
	sltu	t0, t1, t2
	bnel	t0, zero, 1b
	sw	zero, (t1)		/* delay slot */
	.set	pop




#ifdef DEBUG_LED
	/* LED5 ON */
	li	t0, 0xab000302
	li	t1, 0x0002
	sh	t1, (t0)

	li	t0, 0xab00030a
	sh	zero, (t0)
	/* LED5 ON */
#endif	
	
#ifdef DEBUG_LED
	/* LED6 ON */
	li	t0, 0xab000300
	li	t1, 0x0020
	sh	t1, (t0)

	li	t0, 0xab00030a
	sh	zero, (t0)
	/* LED6 ON */
#endif



	/*
	 * call lcboot main()
	 */
	move	a0, zero		/* a0:	argc = 0 */
	move	a1, zero		/* a1 */
	move	a2, zero		/* a2 */
	move	a3, zero		/* a3 */
	move	k0, zero		/* k0 */
	move	k1, zero		/* k1 */
	la	gp, _C_LABEL(_gp)	/* global pointer */
	la	sp, start		/* stack pointer */
	la	v0, main
	jalr	v0
	nop

	.globl	start_netbsd
start_netbsd:
	/*
	 * all LED OFF
	 */
	li	t0, 0xab000248		/* LEDCNTREG */
	sh	zero, (t0)
	li	t1, 0xffff
	li	t0, 0xab000308
	sh	t1, (t0)
	li	t0, 0xab00030a
	sh	t1, (t0)

	/*
	 * initialize registers
	 */
	li	a0, 1			/* a0:	argc = 1 */
	la	a1, argv0		/* a1:	argv */
	la	a2, bootinfo		/* a2:	bootinfo */
	move	a3, zero		/* a3 */
	move	k0, zero		/* k0 */
	move	k1, zero		/* k1 */
	/* no need to set grobal pointer. it set in locore.S */
	la	sp, NETBSD_STARTADDR	/* stack pointer */
	/*
	 * call netbsd
	 */
	jr	sp
	nop


/*
 * arguments for mach_init()
 */
	.data
argv0:
	.word	argv0c
argv1:
	.word	0
argv0c:
	.asciiz	"netbsd"

bootinfo:
	.half	34		/* length */
	.half	0		/* reserved */
	.word	0x13536135	/* magic */
	.word	0		/* fb_addr */
	.half	0		/* fb_line_bytes */
	.half	0		/* fb_width */
	.half	0		/* fb_height */
	.half	0		/* fb_type */
	.half	2		/* BI_CNUSE_SERIAL */
	.half	0		/* padding */
	.word	0x04104400	/* PLATID_CPU_MIPS_VR_4181 */
	.word	0x03810100	/* PLATID_MACH_LASER5_L_CARD */
	.word	0		/* GMT */

/*
 * End of start.S
 */
