/*  *********************************************************************
    *  SB1250 Board Support Package
    *
    *  Global constants and macros		File: sb1250_defs.h
    *
    *  This file contains macros and definitions used by the other
    *  include files.
    *
    *  Author:  Mitch Lichtenberg (mitch@sibyte.com)
    *
    *********************************************************************
    *
    *  Copyright 2000,2001
    *  Broadcom Corporation. All rights reserved.
    *
    *  This software is furnished under license and may be used and
    *  copied only in accordance with the following terms and
    *  conditions.  Subject to these conditions, you may download,
    *  copy, install, use, modify and distribute modified or unmodified
    *  copies of this software in source and/or binary form.  No title
    *  or ownership is transferred hereby.
    *
    *  1) Any source code used, modified or distributed must reproduce
    *     and retain this copyright notice and list of conditions as
    *     they appear in the source file.
    *
    *  2) No right is granted to use any trade name, trademark, or
    *     logo of Broadcom Corporation. Neither the "Broadcom
    *     Corporation" name nor any trademark or logo of Broadcom
    *     Corporation may be used to endorse or promote products
    *     derived from this software without the prior written
    *     permission of Broadcom Corporation.
    *
    *  3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR
    *     IMPLIED WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED
    *     WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
    *     PURPOSE, OR NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT
    *     SHALL BROADCOM BE LIABLE FOR ANY DAMAGES WHATSOEVER, AND IN
    *     PARTICULAR, BROADCOM SHALL NOT BE LIABLE FOR DIRECT, INDIRECT,
    *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    *     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
    *     GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
    *     BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    *     OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
    *     TORT (INCLUDING NEGLIGENCE OR OTHERWISE), EVEN IF ADVISED OF
    *     THE POSSIBILITY OF SUCH DAMAGE.
    ********************************************************************* */


/*  *********************************************************************
    *  Naming schemes for constants in these files:
    *
    *  M_xxx		MASK constant (identifies bits in a register).
    *			For multi-bit fields, all bits in the field will
    *			be set.
    *
    *  K_xxx		"Code" constant (value for data in a multi-bit
    *			field).  The value is right justified.
    *
    *  V_xxx		"Value" constant.  This is the same as the
    *			corresponding "K_xxx" constant, except it is
    *			shifted to the correct position in the register.
    *
    *  S_xxx		SHIFT constant.  This is the number of bits that
    *			a field value (code) needs to be shifted
    *			(towards the left) to put the value in the right
    *			position for the register.
    *
    *  A_xxx		ADDRESS constant.  This will be a physical
    *			address.  Use the MIPS_PHYS_TO_KSEG1 macro to
    *			generate a K1SEG address.
    *
    *  R_xxx		RELATIVE offset constant.  This is an offset from
    *			an A_xxx constant (usually the first register in
    *			a group).
    *
    *  G_xxx(X)		GET value.  This macro obtains a multi-bit field
    *			from a register, masks it, and shifts it to
    *			the bottom of the register (retrieving a K_xxx
    *			value, for example).
    *
    *  V_xxx(X)		VALUE.  This macro computes the value of a
    *			K_xxx constant shifted to the correct position
    *			in the register.
    ********************************************************************* */




#ifndef _SB1250_DEFS_H
#define	_SB1250_DEFS_H

/*
 * Cast to 64-bit number.  Presumably the syntax is different in
 * assembly language.
 *
 * Note: you'll need to define uint32_t and uint64_t in your headers.
 */

#if !defined(__ASSEMBLER__)
#define	_SB_MAKE64(x) ((uint64_t)(x))
#define	_SB_MAKE32(x) ((uint32_t)(x))
#else
#define	_SB_MAKE64(x) (x)
#define	_SB_MAKE32(x) (x)
#endif


/*
 * Make a mask for 1 bit at position 'n'
 */

#define	_SB_MAKEMASK1(n) (_SB_MAKE64(1) << _SB_MAKE64(n))
#define	_SB_MAKEMASK1_32(n) (_SB_MAKE32(1) << _SB_MAKE32(n))

/*
 * Make a mask for 'v' bits at position 'n'
 */

#define	_SB_MAKEMASK(v,n) (_SB_MAKE64((_SB_MAKE64(1)<<(v))-1) << _SB_MAKE64(n))
#define	_SB_MAKEMASK_32(v,n) (_SB_MAKE32((_SB_MAKE32(1)<<(v))-1) << _SB_MAKE32(n))

/*
 * Make a value at 'v' at bit position 'n'
 */

#define	_SB_MAKEVALUE(v,n) (_SB_MAKE64(v) << _SB_MAKE64(n))
#define	_SB_MAKEVALUE_32(v,n) (_SB_MAKE32(v) << _SB_MAKE32(n))

#define	_SB_GETVALUE(v,n,m) ((_SB_MAKE64(v) & _SB_MAKE64(m)) >> _SB_MAKE64(n))
#define	_SB_GETVALUE_32(v,n,m) ((_SB_MAKE32(v) & _SB_MAKE32(m)) >> _SB_MAKE32(n))

/*
 * Macros to read/write on-chip registers
 * XXX should we do the MIPS_PHYS_TO_KSEG1 here?
 */


#if !defined(__ASSEMBLER__)
#define	SBWRITECSR(csr,val) *((volatile uint64_t *) MIPS_PHYS_TO_KSEG1(csr)) = (val)
#define	SBREADCSR(csr) (*((volatile uint64_t *) MIPS_PHYS_TO_KSEG1(csr)))
#endif /* __ASSEMBLER__ */

#endif
