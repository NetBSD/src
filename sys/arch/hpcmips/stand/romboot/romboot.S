/*	$NetBSD: romboot.S,v 1.3.2.3 2002/03/16 15:57:59 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001, 2002 Takao Shinohara.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * simple boot loader for ROM
 *
 * supported platform: LASER5 L-Router(L-Board)
 */	
	
#define zero	$0
#define AT	$at
#define v0	$2
#define v1	$3
#define a0	$4
#define a1	$5
#define a2	$6
#define a3	$7
#define t0	$8
#define t1	$9
#define t2	$10
#define t3	$11
#define t4	$12
#define t5	$13
#define t6	$14
#define t7	$15
#define s0	$16
#define s1	$17
#define s2	$18
#define s3	$19
#define s4	$20
#define s5	$21
#define s6	$22
#define s7	$23
#define t8	$24
#define t9	$25
#define k0	$26
#define k1	$27
#define gp	$28
#define sp	$29
#define s8	$30
#define ra	$31

#define MIPS_KSEG0_START	0x80000000
#define MIPS_KSEG1_START	0xa0000000
#define KSEG1_KSEG0_DIFF	(MIPS_KSEG1_START - MIPS_KSEG0_START)
#define KERNEL_LOADADDR		0x80001000
#define MAX_KERNEL_SIZE		(1024*1024*6)	# 6MB
#define MAX_DCACHE_SIZE		(1024*32)	# 32KB

	.text
	.set	noreorder
	.globl	_start
_start:
	bal	1f			# ra = ROM address + 8
	nop
1:	subu	a0, ra, 8		# a0 = ROM address(kseg1)
	subu	a0, KSEG1_KSEG0_DIFF	# convert to kseg0 address
	move	s0, a0			# keep it in s0	
	la	t0, _etext
	la	t1, _ftext
	subu	t0, t1			# t0 = size of boot loader
	addu	a0, t0			# a0 = kernel address in ROM
	li	a1, KERNEL_LOADADDR
	li	t2, MAX_KERNEL_SIZE	# max kernel size = 6MB - boot
	subu	t2, t0
	addu	t2, a1			# kernel end address
	la	t9, 2f
	addu	t9, s0
	jr	t9			# jump to cached address
	nop
2:
	lw	v0, 0(a0)
	sw	v0, 0(a1)
	addu	a0, 4
	bltu	a1, t2, 2b
	addu	a1, 4			# BDSLOT

	/* purge data cache */
	li	v0, MAX_DCACHE_SIZE
3:	lw	zero, 0(a1)
	addu	a1, 4
	bnez	v0, 3b
	subu	v0, 4			# BDSLOT

	li	sp, KERNEL_LOADADDR	# initialize stack pointer
	li	a0, 1			# argc = 1
	addu	a1, sp, -16		# argv
	la	t0, argv0
	addu	t0, s0
	sw	t0, 0(a1)		# argv[0] = "netbsd"
	sw	zero, 4(a1)		# argv[1] = NULL
	la	a2, bootinfo
	addu	a2, s0
	move	a3, zero
	li	t9, KERNEL_LOADADDR
	jr	t9
	nop

argv0:	.asciiz	"netbsd"

bootinfo:
	.word	34		# len
	.word	0x13536135	# magic
	.word	0		# fb_addr
	.word	0, 0		# fb_line_bytes, fb_width, fb_height, fb_type
	.word	2		# BI_CNUSE_SERIAL
	.word	0x04104500	# VR4122
	.word	0x03810200	# LASER5 L-BOARD
/*
 * kernel binary image begins here.
 */
