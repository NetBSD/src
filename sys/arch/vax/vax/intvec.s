/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: intvec.s,v 1.2 1994/08/16 23:47:28 ragge Exp $
 */

 /* All bugs are subject to removal without further notice */
		


#include "vax/include/mtpr.h"
#include "vax/include/pte.h"
#include "uba.h"

		.globl	Interrupt_scb
	
Interrupt_scb:
                .long trp_0x00+1	# Unused.
_V_MACHINE_CHK: .long trp_0x04+1	# Machine Check.
_V_K_STK_INV:   .long trp_0x08+1	# Kernel Stack Invalid.
_V_POWER_FAIL:  .long trp_0x0C+1	# Power Failed.

_V_PRIV_INSTR:  .long trp_0x10+1	# Privileged/Reserved Instruction.
_V_CUSTOMER:    .long trp_0x14+1	# Customer Reserved Instruction.
_V_RES_OP:      .long trp_0x18+1	# Reserved Operand/Boot Vector(?)
_V_RES_ADD_MODE:.long trp_0x1C+1	# Reserved Address Mode.

_V_ACC_CNTL_VIO:.long trp_0x20		# Access Control Violation.
_V_TRANSL_INV:  .long trp_0x24		# Translation Invalid.
_V_TRACE_PEND:  .long trp_0x28+1	# Trace Pending.
_V_BREAKPOINT:  .long trp_0x2C+1	# Breakpoint Instruction.

_V_COMPAT:      .long trp_0x30+1	# Compatibility Exception.
_V_ARITHMETIC:  .long trp_0x34+1	# Arithmetic Fault.
                .long trp_0x38+1	# Unused.
                .long trp_0x3C+1	# Unused.

_V_CHMK:        .long trp_0x40		# Mode Change Trap.
_V_CHME:        .long trp_0x44+1	# Mode Change Trap.
_V_CHMS:	.long trp_0x48+1	# Mode Change Trap.
_V_CHMU:        .long trp_0x4C+1	# Mode Change Trap.

_V_SBI_SILO:    .long trp_0x50+1	# System Backplane Exception.
_V_CORR_READ:	.long trp_0x54+1	# Corrected Memory Read.
_V_SBI_ALERT:   .long trp_0x58+1	# System Backplane Alert.
_V_SBI_FAULT:   .long trp_0x5C+1	# System Backplane Fault.

_V_MEM_W_TOUT:  .long trp_0x60+1	# Memory Write Timeout
                .long trp_0x64+1	# Unused
                .long trp_0x68+1	# Unused
                .long trp_0x6C+1	# Unused

                .long trp_0x70+1        # Unused
                .long trp_0x74+1        # Unused
                .long trp_0x78+1        # Unused
                .long trp_0x7C+1        # Unused

                .long trp_0x80+1              # Unused
_V_SW_LVL1:     .long trp_0x84+1              # Software Lvl1 Interrupt
_V_SW_LVL2:     .long trp_0x88         	      # AST interrupt
_V_SW_LVL3:     .long trp_0x8C+1              # Software Lvl3 Interrupt

_V_SW_LVL4:     .long trp_0x90+1               # Software Lvl4 Interrupt
_V_SW_LVL5:     .long trp_0x94+1               # Software Lvl5 Interrupt
_V_SW_LVL6:     .long trp_0x98+1               # Software Lvl6 Interrupt
_V_SW_LVL7:     .long trp_0x9C+1               # Software Lvl7 Interrupt

_V_SW_LVL8:     .long trp_0xA0+1               # Software Lvl8 Interrupt
_V_SW_LVL9:     .long trp_0xA4+1               # Software Lvl9 Interrupt
_V_SW_LVL10:    .long trp_0xA8+1               # Software Lvl10 Interrupt
_V_SW_LVL11:    .long trp_0xAC+1               # Software Lvl11 Interrupt

_V_SW_LVL12:    .long trp_0xB0+1               # Software Lvl12 Interrupt
_V_SW_LVL13:    .long trp_0xB4+1               # Software Lvl13 Interrupt
_V_SW_LVL14:    .long trp_0xB8+1               # Software Lvl14 Interrupt
_V_SW_LVL15:    .long trp_0xBC+1               # Software Lvl15 Interrupt

_V_INTERVAL:    .long trp_0xC0+1               # Interval Timer
                .long trp_0xC4+1               # Unused
                .long trp_0xC8+1               # Unused
                .long trp_0xCC+1               # Unused

                .long trp_0xD0+1               # Unused
                .long trp_0xD4+1               # Unused
                .long trp_0xD8+1               # Unused
                .long trp_0xDC+1               # Unused

                .long trp_0xE0+1               # Unused
                .long trp_0xE4+1               # Unused
                .long trp_0xE8+1               # Unused
                .long trp_0xEC+1               # Unused

_V_CONSOLE_SR:  .long trp_0xF0+1               # Console Storage Recieve Interrupt
_V_CONSOLE_ST:  .long trp_0xF4+1               # Console Storage Transmit Interrupt
_V_CONSOLE_TR:  .long trp_0xF8+1               # Console Terminal Recieve Interrupt
_V_CONSOLE_TT:  .long trp_0xFC+1               # Console Terminal Transmit Interrupt
		.globl _V_DEVICE_VEC
_V_DEVICE_VEC:  .space 0x100

_UNIvec:	.globl _UNIvec
		# Unibus vector space
#if NUBA>0
	.long	_uba_00+1,_uba_01+1,_uba_02+1,_uba_03+1,_uba_04+1,_uba_05+1,_uba_06+1,_uba_07+1
	.long	_uba_08+1,_uba_09+1,_uba_0a+1,_uba_0b+1,_uba_0c+1,_uba_0d+1,_uba_0e+1,_uba_0f+1
	.long	_uba_10+1,_uba_11+1,_uba_12+1,_uba_13+1,_uba_14+1,_uba_15+1,_uba_16+1,_uba_17+1
	.long	_uba_18+1,_uba_19+1,_uba_1a+1,_uba_1b+1,_uba_1c+1,_uba_1d+1,_uba_1e+1,_uba_1f+1
	.long	_uba_20+1,_uba_21+1,_uba_22+1,_uba_23+1,_uba_24+1,_uba_25+1,_uba_26+1,_uba_27+1
	.long	_uba_28+1,_uba_29+1,_uba_2a+1,_uba_2b+1,_uba_2c+1,_uba_2d+1,_uba_2e+1,_uba_2f+1
	.long	_uba_30+1,_uba_31+1,_uba_32+1,_uba_33+1,_uba_34+1,_uba_35+1,_uba_36+1,_uba_37+1
	.long	_uba_38+1,_uba_39+1,_uba_3a+1,_uba_3b+1,_uba_3c+1,_uba_3d+1,_uba_3e+1,_uba_3f+1
	.long	_uba_40+1,_uba_41+1,_uba_42+1,_uba_43+1,_uba_44+1,_uba_45+1,_uba_46+1,_uba_47+1
	.long	_uba_48+1,_uba_49+1,_uba_4a+1,_uba_4b+1,_uba_4c+1,_uba_4d+1,_uba_4e+1,_uba_4f+1
	.long	_uba_50+1,_uba_51+1,_uba_52+1,_uba_53+1,_uba_54+1,_uba_55+1,_uba_56+1,_uba_57+1
	.long	_uba_58+1,_uba_59+1,_uba_5a+1,_uba_5b+1,_uba_5c+1,_uba_5d+1,_uba_5e+1,_uba_5f+1
	.long	_uba_60+1,_uba_61+1,_uba_62+1,_uba_63+1,_uba_64+1,_uba_65+1,_uba_66+1,_uba_67+1
	.long	_uba_68+1,_uba_69+1,_uba_6a+1,_uba_6b+1,_uba_6c+1,_uba_6d+1,_uba_6e+1,_uba_6f+1
	.long	_uba_70+1,_uba_71+1,_uba_72+1,_uba_73+1,_uba_74+1,_uba_75+1,_uba_76+1,_uba_77+1
	.long	_uba_78+1,_uba_79+1,_uba_7a+1,_uba_7b+1,_uba_7c+1,_uba_7d+1,_uba_7e+1,_uba_7f+1
#if NUBA>1
	.long	_uba_00+1,_uba_01+1,_uba_02+1,_uba_03+1,_uba_04+1,_uba_05+1,_uba_06+1,_uba_07+1
	.long	_uba_08+1,_uba_09+1,_uba_0a+1,_uba_0b+1,_uba_0c+1,_uba_0d+1,_uba_0e+1,_uba_0f+1
	.long	_uba_10+1,_uba_11+1,_uba_12+1,_uba_13+1,_uba_14+1,_uba_15+1,_uba_16+1,_uba_17+1
	.long	_uba_18+1,_uba_19+1,_uba_1a+1,_uba_1b+1,_uba_1c+1,_uba_1d+1,_uba_1e+1,_uba_1f+1
	.long	_uba_20+1,_uba_21+1,_uba_22+1,_uba_23+1,_uba_24+1,_uba_25+1,_uba_26+1,_uba_27+1
	.long	_uba_28+1,_uba_29+1,_uba_2a+1,_uba_2b+1,_uba_2c+1,_uba_2d+1,_uba_2e+1,_uba_2f+1
	.long	_uba_30+1,_uba_31+1,_uba_32+1,_uba_33+1,_uba_34+1,_uba_35+1,_uba_36+1,_uba_37+1
	.long	_uba_38+1,_uba_39+1,_uba_3a+1,_uba_3b+1,_uba_3c+1,_uba_3d+1,_uba_3e+1,_uba_3f+1
	.long	_uba_40+1,_uba_41+1,_uba_42+1,_uba_43+1,_uba_44+1,_uba_45+1,_uba_46+1,_uba_47+1
	.long	_uba_48+1,_uba_49+1,_uba_4a+1,_uba_4b+1,_uba_4c+1,_uba_4d+1,_uba_4e+1,_uba_4f+1
	.long	_uba_50+1,_uba_51+1,_uba_52+1,_uba_53+1,_uba_54+1,_uba_55+1,_uba_56+1,_uba_57+1
	.long	_uba_58+1,_uba_59+1,_uba_5a+1,_uba_5b+1,_uba_5c+1,_uba_5d+1,_uba_5e+1,_uba_5f+1
	.long	_uba_60+1,_uba_61+1,_uba_62+1,_uba_63+1,_uba_64+1,_uba_65+1,_uba_66+1,_uba_67+1
	.long	_uba_68+1,_uba_69+1,_uba_6a+1,_uba_6b+1,_uba_6c+1,_uba_6d+1,_uba_6e+1,_uba_6f+1
	.long	_uba_70+1,_uba_71+1,_uba_72+1,_uba_73+1,_uba_74+1,_uba_75+1,_uba_76+1,_uba_77+1
	.long	_uba_78+1,_uba_79+1,_uba_7a+1,_uba_7b+1,_uba_7c+1,_uba_7d+1,_uba_7e+1,_uba_7f+1
#if NUBA>2
	.long	_uba_00+1,_uba_01+1,_uba_02+1,_uba_03+1,_uba_04+1,_uba_05+1,_uba_06+1,_uba_07+1
	.long	_uba_08+1,_uba_09+1,_uba_0a+1,_uba_0b+1,_uba_0c+1,_uba_0d+1,_uba_0e+1,_uba_0f+1
	.long	_uba_10+1,_uba_11+1,_uba_12+1,_uba_13+1,_uba_14+1,_uba_15+1,_uba_16+1,_uba_17+1
	.long	_uba_18+1,_uba_19+1,_uba_1a+1,_uba_1b+1,_uba_1c+1,_uba_1d+1,_uba_1e+1,_uba_1f+1
	.long	_uba_20+1,_uba_21+1,_uba_22+1,_uba_23+1,_uba_24+1,_uba_25+1,_uba_26+1,_uba_27+1
	.long	_uba_28+1,_uba_29+1,_uba_2a+1,_uba_2b+1,_uba_2c+1,_uba_2d+1,_uba_2e+1,_uba_2f+1
	.long	_uba_30+1,_uba_31+1,_uba_32+1,_uba_33+1,_uba_34+1,_uba_35+1,_uba_36+1,_uba_37+1
	.long	_uba_38+1,_uba_39+1,_uba_3a+1,_uba_3b+1,_uba_3c+1,_uba_3d+1,_uba_3e+1,_uba_3f+1
	.long	_uba_40+1,_uba_41+1,_uba_42+1,_uba_43+1,_uba_44+1,_uba_45+1,_uba_46+1,_uba_47+1
	.long	_uba_48+1,_uba_49+1,_uba_4a+1,_uba_4b+1,_uba_4c+1,_uba_4d+1,_uba_4e+1,_uba_4f+1
	.long	_uba_50+1,_uba_51+1,_uba_52+1,_uba_53+1,_uba_54+1,_uba_55+1,_uba_56+1,_uba_57+1
	.long	_uba_58+1,_uba_59+1,_uba_5a+1,_uba_5b+1,_uba_5c+1,_uba_5d+1,_uba_5e+1,_uba_5f+1
	.long	_uba_60+1,_uba_61+1,_uba_62+1,_uba_63+1,_uba_64+1,_uba_65+1,_uba_66+1,_uba_67+1
	.long	_uba_68+1,_uba_69+1,_uba_6a+1,_uba_6b+1,_uba_6c+1,_uba_6d+1,_uba_6e+1,_uba_6f+1
	.long	_uba_70+1,_uba_71+1,_uba_72+1,_uba_73+1,_uba_74+1,_uba_75+1,_uba_76+1,_uba_77+1
	.long	_uba_78+1,_uba_79+1,_uba_7a+1,_uba_7b+1,_uba_7c+1,_uba_7d+1,_uba_7e+1,_uba_7f+1
#if NUBA>3
	.long	_uba_00+1,_uba_01+1,_uba_02+1,_uba_03+1,_uba_04+1,_uba_05+1,_uba_06+1,_uba_07+1
	.long	_uba_08+1,_uba_09+1,_uba_0a+1,_uba_0b+1,_uba_0c+1,_uba_0d+1,_uba_0e+1,_uba_0f+1
	.long	_uba_10+1,_uba_11+1,_uba_12+1,_uba_13+1,_uba_14+1,_uba_15+1,_uba_16+1,_uba_17+1
	.long	_uba_18+1,_uba_19+1,_uba_1a+1,_uba_1b+1,_uba_1c+1,_uba_1d+1,_uba_1e+1,_uba_1f+1
	.long	_uba_20+1,_uba_21+1,_uba_22+1,_uba_23+1,_uba_24+1,_uba_25+1,_uba_26+1,_uba_27+1
	.long	_uba_28+1,_uba_29+1,_uba_2a+1,_uba_2b+1,_uba_2c+1,_uba_2d+1,_uba_2e+1,_uba_2f+1
	.long	_uba_30+1,_uba_31+1,_uba_32+1,_uba_33+1,_uba_34+1,_uba_35+1,_uba_36+1,_uba_37+1
	.long	_uba_38+1,_uba_39+1,_uba_3a+1,_uba_3b+1,_uba_3c+1,_uba_3d+1,_uba_3e+1,_uba_3f+1
	.long	_uba_40+1,_uba_41+1,_uba_42+1,_uba_43+1,_uba_44+1,_uba_45+1,_uba_46+1,_uba_47+1
	.long	_uba_48+1,_uba_49+1,_uba_4a+1,_uba_4b+1,_uba_4c+1,_uba_4d+1,_uba_4e+1,_uba_4f+1
	.long	_uba_50+1,_uba_51+1,_uba_52+1,_uba_53+1,_uba_54+1,_uba_55+1,_uba_56+1,_uba_57+1
	.long	_uba_58+1,_uba_59+1,_uba_5a+1,_uba_5b+1,_uba_5c+1,_uba_5d+1,_uba_5e+1,_uba_5f+1
	.long	_uba_60+1,_uba_61+1,_uba_62+1,_uba_63+1,_uba_64+1,_uba_65+1,_uba_66+1,_uba_67+1
	.long	_uba_68+1,_uba_69+1,_uba_6a+1,_uba_6b+1,_uba_6c+1,_uba_6d+1,_uba_6e+1,_uba_6f+1
	.long	_uba_70+1,_uba_71+1,_uba_72+1,_uba_73+1,_uba_74+1,_uba_75+1,_uba_76+1,_uba_77+1
	.long	_uba_78+1,_uba_79+1,_uba_7a+1,_uba_7b+1,_uba_7c+1,_uba_7d+1,_uba_7e+1,_uba_7f+1
#if NUBA>4
#error "Number of bus adapters must be increased in intvec.s"
#endif
#endif
#endif
#endif
#endif


		.globl _eUNIvec
_eUNIvec:
		.align 2
trp_0x00:	pushal	msg_trp_0x00
		calls	$1,_conout
		halt

		.align 2
 #
 # trp_0x04 is the badaddress trap, also called when referencing
 # a invalid address (busserror)
 # _memtest (memtest in C) holds the address to continue execution
 # at when returning from a intentional test.
 #
trp_0x04:
		cmpl	$0,_memtest	# Ar we running the memory test?
		bneq	1f		# Yes.
                                        # Get number of extra 
                addl2   (sp)+,sp        # long:s on interrupt stack
                                        # Remove all the overhead

                pushl   r0
                pushl   r1
                movl    8(sp),r1
                pushl   r1              # The address
                pushl   (r1)            # The instruction               
                pushal  msg_trp_0x04
                calls   $1,_printf
                movq    (sp)+,r1
                movq    (sp)+,r0

                halt                    # halt for debugging
                rei

1:		# Ok, we got the last address
					# Get number of extra 
		addl2 	(sp)+,sp	# long:s on interrupt stack
				# Remove all the overhead

		movl	_memtest,(sp)	# Do not redo the address
					# that generated the trap

		mtpr	$0xF,$PR_MCESR	# clear the bus error bit
					# XXX What if there was another error?
		movl	$0,_memtest
		rei


		.align 2
trp_0x08:	pushal	msg_trp_0x08
		calls	$1,_conout
		halt
		rei

		.align 2
trp_0x0C:	pushal	msg_trp_0x0C
		calls	$1,_conout
		halt
		rei
		.align 2
trp_0x10:.globl	trp_0x10
		halt
		pushl	r0
		pushl	r1
		movl	8(sp),r1
		pushl	r1
		pushl	(r1)
		pushal	msg_trp_0x10
		calls	$1,_printf
		movq	(sp)+,r1
		movq	(sp)+,r0
		halt
		rei
		.align 2
trp_0x14:	pushal	msg_trp_0x14
		calls	$1,_conout
		halt
		rei

		.align 2
trp_0x18: #	halt
		pushl	r0		# Reserved operand
		pushl	r1
		movl	8(sp),r1
		pushl	r1		# The address
		pushl	(r1)		# The instruction		
		pushal	msg_trp_0x18
		calls	$1,_printf
		movq	(sp)+,r1
		movq	(sp)+,r0
		halt
		rei

		.align 2
trp_0x1C:	pushal	msg_trp_0x1C 	# Reserved address
		calls	$1,_conout
		halt
		rei
		.align 2

trp_0x20:.globl trp_0x20	# Access cntrl viol fault

        pushl   r0

        pushr   $0x7fff
        pushl   76(sp)
        pushl   76(sp)
        pushl   76(sp)
        pushl   76(sp)
        calls   $4,_access_v
        popr    $0x7fff
        movl    (sp)+,r0
        addl2   $8,sp
        rei

	.align 2
trp_0x24:.globl trp_0x24
#	halt
	pushl	r0		# We first check for a simulated
#	movl	8(sp),r0	# page reference
#	bicl2	$0x800001ff,r0
#	ashl	$-7,r0,r0
#	addl2	_Sysmap,r0	# Now we have calculated pte addr...

#	bbsc	$0x17,(r0),1f	# PG_SREF is bit 0x17 (simul ref)

	pushr	$0x7fff
	pushl	76(sp)
	pushl	76(sp)
	pushl	76(sp)
	pushl	76(sp)
	calls	$4,_ingen_v
	popr	$0x7fff
	movl	(sp)+,r0
	addl2	$8,sp
#	halt
	rei


1:	bisl2	$(PG_REF|PG_V),(r0)	# Set valid & ref bit
	movl	(sp),r0
	addl2	$12,sp			# pop params off stack
	rei

trp_0x28:	.align 2
		pushal	msg_trp_0x28
		calls	$1,_conout
		halt
		rei
		.align 2
trp_0x2C:	pushal	msg_trp_0x2C
		calls	$1,_conout
		halt
		rei
		.align 2
trp_0x30:	pushal	msg_trp_0x30
		calls	$1,_conout
		halt
		rei
		.align 2
trp_0x34:	pushal	msg_trp_0x34
		calls	$1,_conout
		halt
		rei
		.align 2
trp_0x38:	pushal	msg_trp_0x38
		calls	$1,_conout
		halt
		rei
		.align 2
trp_0x3C:	pushal	msg_trp_0x3C
		calls	$1,_conout
		halt
		rei

	.align 2		# Main system call 
	.globl	trp_0x40
trp_0x40:
#	halt
	pushl	r0
	pushl	r1
	pushl	ap
	pushl	sp
	calls	$2,_syscall
	movl	(sp)+,r1
	movl	(sp)+,r0
	addl2	$4,sp
	mtpr	$0x1f,$PR_IPL
#	halt
	rei


		.align 2
trp_0x44:	pushal	msg_trp_0x44
		calls	$1,_conout
		halt
		rei
		.align 2
trp_0x48:	pushal	msg_trp_0x48
		calls	$1,_conout
		halt
		rei
		.align 2
trp_0x4C:	pushal	msg_trp_0x4C
		calls	$1,_conout
		halt
		rei
		.align 2
trp_0x50:	pushal	msg_trp_0x50
		calls	$1,_conout
		halt

		.align 2
trp_0x54:	pushal	msg_trp_0x54
		calls	$1,_conout
		halt

		.align 2
trp_0x58:	pushal	msg_trp_0x58
 		calls	$1,_conout
		halt

		.align 2
trp_0x5C:	pushal	msg_trp_0x5C
		calls	$1,_conout
		halt

		.align 2
trp_0x60:	pushl   r0
                pushl   r1
                pushl   16(sp)
                pushl   16(sp)
                pushal  msg_trp_0x60
                calls   $3,_printf
                movl    (sp)+,r1
                movl    (sp)+,r0
                halt
                rei

		.align 2
trp_0x64:	pushal	msg_trp_0x64
		calls	$1,_conout
		halt

		.align 2
trp_0x68:	pushal	msg_trp_0x68
		calls	$1,_conout
		halt

		.align 2
trp_0x6C:	pushal	msg_trp_0x6C
		calls	$1,_conout
		halt

		.align 2
trp_0x70:	pushal	msg_trp_0x70
		calls	$1,_conout
		halt

		.align 2
trp_0x74:	pushal	msg_trp_0x74
		calls	$1,_conout
		halt

		.align 2
trp_0x78:	pushal	msg_trp_0x78
		calls	$1,_conout
		halt

		.align 2
trp_0x7C:	pushal	msg_trp_0x7C
		calls	$1,_conout
		halt

		.align 2
trp_0x80:	pushal	msg_trp_0x80
		calls	$1,_conout
		halt

		.align 2
trp_0x84:	pushal	msg_trp_0x84
		calls	$1,_conout
		halt

		.align 2
trp_0x88:	pushr	$0x7fff		# AST trap
		calls	$0,_astint
		popr	$0x7fff	
		rei

		.align 2
trp_0x8C:	pushal	msg_trp_0x8C
		calls	$1,_conout
		halt

		.align 2
trp_0x90:	pushal	msg_trp_0x90
		calls	$1,_conout
		halt

		.align 2
trp_0x94:	pushal	msg_trp_0x94
		calls	$1,_conout
		halt

		.align 2
trp_0x98:	pushal	msg_trp_0x98
		calls	$1,_conout
		halt

		.align 2
trp_0x9C:	pushal	msg_trp_0x9C
		calls	$1,_conout
		halt

	.align 2
trp_0xA0:
	pushr $0x7fff			# Software interrupt vector
#        pushl   sp
#        addl2   $8,(sp)
        calls $0,_softclock
        popr $0x7fff
        rei

		.align 2
trp_0xA4:	pushal	msg_trp_0xA4
		calls	$1,_conout
		halt

		.align 2
trp_0xA8:	pushal	msg_trp_0xA8
		calls	$1,_conout
		halt

		.align 2
trp_0xAC:	pushal	msg_trp_0xAC
		calls	$1,_conout
		halt

		.align 2
trp_0xB0:	pushal	msg_trp_0xB0
		calls	$1,_conout
		halt

		.align 2
trp_0xB4:	pushal	msg_trp_0xB4
		calls	$1,_conout
		halt

		.align 2
trp_0xB8:	pushal	msg_trp_0xB8
		calls	$1,_conout
		halt

		.align 2
trp_0xBC:	pushal	msg_trp_0xBC
		calls	$1,_conout
		halt
	.align 2
trp_0xC0:
		mtpr	$0xc1,$PR_ICCS		# Reset interrupt flag
		pushr $0x7fff
		pushl	sp
		addl2	$60,(sp)
#		halt
		calls $1,_hardclock
		popr $0x7fff
		rei

		.align 2
trp_0xC4:	pushal	msg_trp_0xC4
		calls	$1,_conout
		halt

		.align 2
trp_0xC8:	pushal	msg_trp_0xC8
		calls	$1,_conout
		halt

		.align 2
trp_0xCC:	pushal	msg_trp_0xCC
		calls	$1,_conout
		halt

		.align 2
trp_0xD0:	pushal	msg_trp_0xD0
		calls	$1,_conout
		halt

		.align 2
trp_0xD4:	pushal	msg_trp_0xD4
		calls	$1,_conout
		halt

		.align 2
trp_0xD8:	pushal	msg_trp_0xD8
		calls	$1,_conout
		halt

		.align 2
trp_0xDC:	pushal	msg_trp_0xDC
		calls	$1,_conout
		halt

		.align 2
trp_0xE0:	pushal	msg_trp_0xE0
		calls	$1,_conout
		halt

		.align 2
trp_0xE4:	pushal	msg_trp_0xE4
		calls	$1,_conout
		halt

		.align 2
trp_0xE8:	pushal	msg_trp_0xE8
		calls	$1,_conout
		halt

		.align 2
trp_0xEC:	pushal	msg_trp_0xEC
		calls	$1,_conout
		halt

		.align 2
trp_0xF0:	pushal	msg_trp_0xF0
		calls	$1,_conout
		halt

		.align 2
trp_0xF4:	pushal	msg_trp_0xF4
		calls	$1,_conout
		halt

		.align 2
trp_0xF8:	pushal	msg_trp_0xF8
		calls	$1,_conout
		halt

		.align 2
trp_0xFC:	pushal	msg_trp_0xFC
		calls	$1,_conout
		halt



msg_end_of_memtest:	.asciz	"Memory check, at addr: 0x%x, MCESR: 0x%x, MCESR cleared\n"

msg_trp_0x00:	.asciz	"\r\nTrap:	0x00	Unused.\r\n"
msg_trp_0x04:   .asciz  "\r\nTrap:      0x04    V_MACHINE_CHK\nTried to execute op: 0x%x\n       from address: 0x%x\n"
msg_trp_0x08:	.asciz	"\r\nTrap:	0x08	V_K_STK_INV\r\n"
msg_trp_0x0C:	.asciz	"\r\nTrap:	0x0C	V_POWER_FAIL\r\n"

msg_trp_0x10:	.asciz	"\r\nTrap:	0x10	V_PRIV_INSTR\nTried to execute op: 0x%x\n       from address: 0x%x\n"
msg_trp_0x14:	.asciz	"\r\nTrap:	0x14	V_CUSTOMER\n"
msg_trp_0x18:	.asciz	"\r\nTrap:	0x18	V_RES_OP\nTried to execute op: 0x%x\n       from address: 0x%x\n"
msg_trp_0x1C:	.asciz	"\r\nTrap:	0x1C	V_RES_ADD_MODE.\r\n"

msg_trp_0x20:	.asciz	"\r\nTrap:	0x20	V_ACC_CNTL_VIO\nTried to access virtual adress: 0x%x\n                  from address: 0x%x\n"
msg_trp_0x24:	.asciz	"\r\nTrap:	0x24	V_TRANSL_INV\nTried to access virtual adress: 0x%x\n                  from address: 0x%x\nObserve: YOU forgot to set the pte valid bit!\n"
msg_trp_0x28:	.asciz	"\r\nTrap:	0x28	V_TRACE_PEND\r\n"
msg_trp_0x2C:	.asciz	"\r\nTrap:	0x2C	V_BREAKPOINT\r\n"

msg_trp_0x30:	.asciz	"\r\nTrap:	0x30	V_COMPAT\r\n"
msg_trp_0x34:	.asciz	"\r\nTrap:	0x34	V_ARITHMETIC\r\n"
msg_trp_0x38:	.asciz	"\r\nTrap:	0x38	Unused.\r\n"
msg_trp_0x3C:	.asciz	"\r\nTrap:	0x3C	Unused.\r\n"

msg_trp_0x40:	.asciz	"\r\nTrap:	0x40	V_CHMK\r\n"
msg_trp_0x44:	.asciz	"\r\nTrap:	0x44	V_CHME\r\n"
msg_trp_0x48:	.asciz	"\r\nTrap:	0x48	V_CHMS.\r\n"
msg_trp_0x4C:	.asciz	"\r\nTrap:	0x4C	V_CHMU\r\n"

msg_trp_0x50:	.asciz	"\r\nTrap:	0x50	V_SBI_SILO\r\n"
msg_trp_0x54:	.asciz	"\r\nTrap:	0x54	V_CORR_READ\r\n"
msg_trp_0x58:	.asciz	"\r\nTrap:	0x58	V_SBI_ALERT\r\n"
msg_trp_0x5C:	.asciz	"\r\nTrap:	0x5C	V_SBI_FAULT\r\n"

msg_trp_0x60:	.asciz	"\r\nTrap:	0x60	V_MEM_W_TOUT\nTried to access virtual adress: 0x%x\n                  from address: 0x%x\n\n"
msg_trp_0x64:	.asciz	"\r\nTrap:	0x64	Unused.\r\n"
msg_trp_0x68:	.asciz	"\r\nTrap:	0x68	Unused.\r\n"
msg_trp_0x6C:	.asciz	"\r\nTrap:	0x6C	Unused.\r\n"

msg_trp_0x70:	.asciz	"\r\nTrap:	0x70	Unused.\r\n"
msg_trp_0x74:	.asciz	"\r\nTrap:	0x74	Unused.\r\n"
msg_trp_0x78:	.asciz	"\r\nTrap:	0x78	Unused.\r\n"
msg_trp_0x7C:	.asciz	"\r\nTrap:	0x7C	Unused.\r\n"

msg_trp_0x80:	.asciz	"\r\nTrap:	0x80	Unused.\r\n"
msg_trp_0x84:	.asciz	"\r\nTrap:	0x84	V_SW_LVL1\r\n"
msg_trp_0x88:	.asciz	"\r\nTrap:	0x88	V_SW_LVL2\r\n"
msg_trp_0x8C:	.asciz	"\r\nTrap:	0x8C	V_SW_LVL3\r\n"

msg_trp_0x90:	.asciz	"\r\nTrap:	0x90	V_SW_LVL4\r\n"
msg_trp_0x94:	.asciz	"\r\nTrap:	0x94	V_SW_LVL5\r\n"
msg_trp_0x98:	.asciz	"\r\nTrap:	0x98	V_SW_LVL6\r\n"
msg_trp_0x9C:	.asciz	"\r\nTrap:	0x9C	V_SW_LVL7\r\n"

msg_trp_0xA0:	.asciz	"\r\nTrap:	0xA0	V_SW_LVL8\r\n"
msg_trp_0xA4:	.asciz	"\r\nTrap:	0xA4	V_SW_LVL9\r\n"
msg_trp_0xA8:	.asciz	"\r\nTrap:	0xA8	V_SW_LVL10\r\n"
msg_trp_0xAC:	.asciz	"\r\nTrap:	0xAC	V_SW_LVL11\r\n"

msg_trp_0xB0:	.asciz	"\r\nTrap:	0xB0	V_SW_LVL12\r\n"
msg_trp_0xB4:	.asciz	"\r\nTrap:	0xB4	V_SW_LVL13\r\n"
msg_trp_0xB8:	.asciz	"\r\nTrap:	0xB8	V_SW_LVL14\r\n"
msg_trp_0xBC:	.asciz	"\r\nTrap:	0xBC	V_SW_LVL15\r\n"

msg_trp_0xC0:	.asciz	"\r\nTrap:	0xC0	V_INTERVAL\r\n"
msg_trp_0xC4:	.asciz	"\r\nTrap:	0xC4	Unused.\r\n"
msg_trp_0xC8:	.asciz	"\r\nTrap:	0xC8	Unused.\r\n"
msg_trp_0xCC:	.asciz	"\r\nTrap:	0xCC	Unused.\r\n"

msg_trp_0xD0:	.asciz	"\r\nTrap:	0xD0	Unused.\r\n"
msg_trp_0xD4:	.asciz	"\r\nTrap:	0xD4	Unused.\r\n"
msg_trp_0xD8:	.asciz	"\r\nTrap:	0xD8	Unused.\r\n"
msg_trp_0xDC:	.asciz	"\r\nTrap:	0xDC	Unused.\r\n"

msg_trp_0xE0:	.asciz	"\r\nTrap:	0xE0	Unused.\r\n"
msg_trp_0xE4:	.asciz	"\r\nTrap:	0xE4	Unused.\r\n"
msg_trp_0xE8:	.asciz	"\r\nTrap:	0xE8	Unused.\r\n"
msg_trp_0xEC:	.asciz	"\r\nTrap:	0xEC	Unused.\r\n"

msg_trp_0xF0:	.asciz	"\r\nTrap:	0xF0	V_CONSOLE_SR\r\n"
msg_trp_0xF4:	.asciz	"\r\nTrap:	0xF4	V_CONSOLE_ST\r\n"
msg_trp_0xF8:	.asciz	"\r\nTrap:	0xF8	V_CONSOLE_TR\r\n"
msg_trp_0xFC:	.asciz	"\r\nTrap:	0xFC	V_CONSOLE_TT\r\n"
