/*	$NetBSD: psl.h,v 1.7.12.1 2012/04/17 00:06:26 yamt Exp $	*/

/*	$OpenBSD: psl.h,v 1.6 1999/11/25 18:29:01 mickey Exp $	*/

/*
 * Copyright (c) 1999-2004 Michael Shalayeff
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
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _HPPA_PSL_H_
#define _HPPA_PSL_H_

/*
 * Reference:
 * 1. PA-RISC 1.1 Architecture and Instruction Set Manual
 *    Hewlett Packard, 3rd Edition, February 1994; Part Number 09740-90039
 */

/*
 * Processor Status Word Bit Positions (in PA-RISC bit order)
 */
#define	PSW_Y_POS	(0)
#define	PSW_Z_POS	(1)
#define	PSW_SS_POS	(3)	/* Reserved, Software-defined */
#define	PSW_W_POS	(4)
#define	PSW_E_POS	(5)
#define	PSW_S_POS	(6)
#define	PSW_T_POS	(7)
#define	PSW_H_POS	(8)
#define	PSW_L_POS	(9)
#define	PSW_N_POS	(10)
#define	PSW_X_POS	(11)
#define	PSW_B_POS	(12)
#define	PSW_C_POS	(13)
#define	PSW_V_POS	(14)
#define	PSW_M_POS	(15)
#define	PSW_CB_POS	(16)
#define	PSW_O_POS	(24)
#define	PSW_G_POS	(25)
#define	PSW_F_POS	(26)
#define	PSW_R_POS	(27)
#define	PSW_Q_POS	(28)
#define	PSW_P_POS	(29)
#define	PSW_D_POS	(30)
#define	PSW_I_POS	(31)

#define	PSW_BITS	"\020\001I\002D\003P\004Q\005R\006F\007G\010O"  \
			"\021M\022V\023C\024B\025X\026N\027L\030H" \
			"\031T\032S\033E\034W\037Z\040Y"

/*
 * Processor Status Word Bit Values
 */
#define	PSW_Y	(1 << (31-PSW_Y_POS))	/* Data Debug Trap Disable */
#define	PSW_Z	(1 << (31-PSW_Z_POS))	/* Instruction Debug Trap Disable */
#define	PSW_SS	(1 << (31-PSW_SS_POS))	/* Reserved; Software Single-Step */
#define	PSW_W	(1 << (31-PSW_W_POS))	/* 64bit address decode enable */
#define	PSW_E	(1 << (31-PSW_E_POS))	/* Little Endian Memory Access Enable */
#define	PSW_S	(1 << (31-PSW_S_POS))	/* Secure Interval Timer */
#define	PSW_T	(1 << (31-PSW_T_POS))	/* Taken Branch Trap Enable */
#define	PSW_H	(1 << (31-PSW_H_POS))	/* Higher-privilege Transfer Trap Enable */
#define	PSW_L	(1 << (31-PSW_L_POS))	/* Lower-privilege Transfer Trap Enable */
#define	PSW_N	(1 << (31-PSW_N_POS))	/* Nullify */
#define	PSW_X	(1 << (31-PSW_X_POS))	/* Data Memory Break Disable */
#define	PSW_B	(1 << (31-PSW_B_POS))	/* Taken Branch */
#define	PSW_C	(1 << (31-PSW_C_POS))	/* Instruction Address Translation Enable */
#define	PSW_V	(1 << (31-PSW_V_POS))	/* Divide Step Correction */
#define	PSW_M	(1 << (31-PSW_M_POS))	/* High-priority Machine Check Mask */
#define	PSW_CB	(1 << (31-PSW_CB_POS))	/* Carry/Borrow Bits */
#define	PSW_O	(1 << (31-PSW_O_POS))	/* Force strong ordering (2.0) */
#define	PSW_G	(1 << (31-PSW_G_POS))	/* Debug Trap Enable */
#define	PSW_F	(1 << (31-PSW_F_POS))	/* Perfomance Monitor Interrupt Unmask */
#define	PSW_R	(1 << (31-PSW_R_POS))	/* Recover Counter Enable */
#define	PSW_Q	(1 << (31-PSW_Q_POS))	/* Interrupt State Collection Enable */
#define	PSW_P	(1 << (31-PSW_P_POS))	/* Protection Identifier Validation Enable */
#define	PSW_D	(1 << (31-PSW_D_POS))	/* Data Address Translation Enable */
#define	PSW_I	(1 << (31-PSW_I_POS))	/* External Interrupt, Power Failure
					   Interrupt, and Low-Priority Machine
					   Check Interrupt unmask */

/*
 * Frequently Used PSW Values
 */
#define	RESET_PSW	(PSW_R | PSW_Q | PSW_P | PSW_D | PSW_I)
#define PSW_MBS		(PSW_C | PSW_Q | PSW_P | PSW_D | PSW_I)
#define PSW_MBZ		(PSW_Y | PSW_Z | PSW_S | PSW_X | PSW_M | PSW_R)

#endif  /* _HPPA_PSL_H_ */
