/* $NetBSD: ac97reg.h,v 1.1.10.2 2006/04/22 11:37:41 simonb Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_MIPS_ALCHEMY_AC97REG_H
#define	_MIPS_ALCHEMY_AC97REG_H

/*  *********************************************************************
    *  Naming schemes for constants in these files:
    *
    *  M_xxx            MASK constant (identifies bits in a register).
    *                   For multi-bit fields, all bits in the field will
    *                   be set.
    *
    *  K_xxx            "Code" constant (value for data in a multi-bit
    *                   field).  The value is right justified.
    *
    *  V_xxx            "Value" constant.  This is the same as the
    *                   corresponding "K_xxx" constant, except it is
    *                   shifted to the correct position in the register.
    *
    *  S_xxx            SHIFT constant.  This is the number of bits that
    *                   a field value (code) needs to be shifted
    *                   (towards the left) to put the value in the right
    *                   position for the register.
    *
    *  A_xxx            ADDRESS constant.  This will be a physical
    *                   address.  Use the MIPS_PHYS_TO_KSEG1 macro to
    *                   generate a K1SEG address.
    *
    *  R_xxx            RELATIVE offset constant.  This is an offset from
    *                   an A_xxx constant (usually the first register in
    *                   a group).
    *
    *  G_xxx(X)         GET value.  This macro obtains a multi-bit field
    *                   from a register, masks it, and shifts it to
    *                   the bottom of the register (retrieving a K_xxx
    *                   value, for example).
    *
    *  V_xxx(X)         VALUE.  This macro computes the value of a
    *                   K_xxx constant shifted to the correct position
    *                   in the register.
    ********************************************************************* */

#if !defined(__ASSEMBLER__)
#define _MAKE64(x) ((uint64_t)(x))
#define _MAKE32(x) ((uint32_t)(x))
#else
#define _MAKE64(x) (x)  
#define _MAKE32(x) (x)
#endif 

/* Make a mask for 1 bit at position 'n' */
#define _MAKEMASK1_64(n) (_MAKE64(1) << _MAKE64(n))
#define _MAKEMASK1_32(n) (_MAKE32(1) << _MAKE32(n))

/* Make a mask for 'v' bits at position 'n' */
#define _MAKEMASK_64(v,n) (_MAKE64((_MAKE64(1)<<(v))-1) << _MAKE64(n))
#define _MAKEMASK_32(v,n) (_MAKE32((_MAKE32(1)<<(v))-1) << _MAKE32(n))

/* Make a value at 'v' at bit position 'n' */
#define _MAKEVALUE_64(v,n) (_MAKE64(v) << _MAKE64(n))
#define _MAKEVALUE_32(v,n) (_MAKE32(v) << _MAKE32(n))

#define _GETVALUE_64(v,n,m) ((_MAKE64(v) & _MAKE64(m)) >> _MAKE64(n))
#define _GETVALUE_32(v,n,m) ((_MAKE32(v) & _MAKE32(m)) >> _MAKE32(n))

/********************   AC97 Controller registers   *********************/

#define	AC97_CONFIG		0x00

#define	  M_AC97CFG_RS		  _MAKEMASK1_32(0)
#define	  M_AC97CFG_SN		  _MAKEMASK1_32(1)
#define	  M_AC97CFG_SG		  _MAKEMASK1_32(2)

#define	  S_AC97CFG_XS		  _MAKE32(12)
#define	  M_AC97CFG_XS		  _MAKEMASK_32(10)
#define	  V_AC97CFG_XS(x)	  _MAKEVALUE_32(x, S_AC97CFG_XS)
#define	  G_AC97CFG_XS(x)	  _GETVALUE_32(x, S_AC97CFG_XS, M_AC97CFG_XS)

#define	  S_AC97CFG_RC		  _MAKE32(12)
#define	  M_AC97CFG_RC		  _MAKEMASK_32(10)
#define	  V_AC97CFG_RC(x)	  _MAKEVALUE_32(x, S_AC97CFG_RC)
#define	  G_AC97CFG_RC(x)	  _GETVALUE_32(x, S_AC97CFG_RC, M_AC97CFG_RC)

#define	AC97_STATUS		0x04

#define	  M_AC97STAT_RF		  _MAKEMASK1_32(0)
#define	  M_AC97STAT_RE		  _MAKEMASK1_32(1)
#define	  M_AC97STAT_TF		  _MAKEMASK1_32(3)
#define	  M_AC97STAT_TE		  _MAKEMASK1_32(4)
#define	  M_AC97STAT_CP		  _MAKEMASK1_32(6)
#define	  M_AC97STAT_RD		  _MAKEMASK1_32(7)
#define	  M_AC97STAT_RO		  _MAKEMASK1_32(8)
#define	  M_AC97STAT_RU		  _MAKEMASK1_32(9)
#define	  M_AC97STAT_XO		  _MAKEMASK1_32(10)
#define	  M_AC97STAT_XU		  _MAKEMASK1_32(11)

#define	AC97_DATA		0x08

#define	  S_AC97DATA_DATA	  _MAKE32(0)
#define	  M_AC97DATA_DATA	  _MAKEMASK_32(16)
#define	  V_AC97DATA_DATA(x)	  _MAKEVALUE_32(x, S_AC97DATA_DATA)
#define	  G_AC97DATA_DATA(x)	  _GETVALUE_32(x, S_AC97DATA_DATA, M_AC97DATA_DATA)

#define	AC97_COMMAND		0x0c

#define	  S_AC97CMD_INDEX	  _MAKE32(0)
#define	  M_AC97CMD_INDEX	  _MAKEMASK_32(7)
#define	  V_AC97CMD_INDEX(x)	  _MAKEVALUE_32(x, S_AC97CMD_INDEX)
#define	  G_AC97CMD_INDEX(x)	  _GETVALUE_32(x, S_AC97CMD_INDEX, M_AC97CMD_INDEX)

#define	  M_AC97CMD_RW		  _MAKEMASK1_32(7)

#define	  S_AC97CMD_DATA	  _MAKE32(16)
#define	  M_AC97CMD_DATA	  _MAKEMASK_32(16)
#define	  V_AC97CMD_DATA(x)	  _MAKEVALUE_32(x, S_AC97CMD_DATA)
#define	  G_AC97CMD_DATA(x)	  _GETVALUE_32(x, S_AC97CMD_DATA, M_AC97CMD_DATA)

#define	AC97_COMMAND_RESPONSE	0x0c

#define	  S_AC97CMDRESP_DATA	  _MAKE32(0)
#define	  M_AC97CMDRESP_DATA	  _MAKEMASK_32(16)
#define	  V_AC97CMDRESP_DATA(x)	  _MAKEVALUE_32(x, S_AC97CMDRESP_DATA)
#define	  G_AC97CMDRESP_DATA(x)	  _GETVALUE_32(x, S_AC97CMDRESP_DATA, M_AC97CMDRESP_DATA)

#define	AC97_ENABLE		0x10

#define	  M_AC97EN_CE		  _MAKEMASK1_32(0)
#define	  M_AC97EN_D		  _MAKEMASK1_32(1)

#define	AC97_SIZE		0x14		/* size of register set */

#endif	/* _MIPS_ALCHEMY_AC97REG_H */
