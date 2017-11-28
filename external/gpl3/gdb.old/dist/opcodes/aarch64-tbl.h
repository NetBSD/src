/* aarch64-tbl.h -- AArch64 opcode description table and instruction
   operand description table.
   Copyright (C) 2012-2016 Free Software Foundation, Inc.

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this file; see the file COPYING.  If not, write to the
   Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "aarch64-opc.h"

#ifndef VERIFIER
#error  VERIFIER must be defined.
#endif

/* Operand type.  */

#define OPND(x)	AARCH64_OPND_##x
#define OP0() {}
#define OP1(a) {OPND(a)}
#define OP2(a,b) {OPND(a), OPND(b)}
#define OP3(a,b,c) {OPND(a), OPND(b), OPND(c)}
#define OP4(a,b,c,d) {OPND(a), OPND(b), OPND(c), OPND(d)}
#define OP5(a,b,c,d,e) {OPND(a), OPND(b), OPND(c), OPND(d), OPND(e)}

#define QLF(x) AARCH64_OPND_QLF_##x
#define QLF1(a) {QLF(a)}
#define QLF2(a,b) {QLF(a), QLF(b)}
#define QLF3(a,b,c) {QLF(a), QLF(b), QLF(c)}
#define QLF4(a,b,c,d) {QLF(a), QLF(b), QLF(c), QLF(d)}
#define QLF5(a,b,c,d,e) {QLF(a), QLF(b), QLF(c), QLF(d), QLF(e)}

/* Qualifiers list.  */

/* e.g. MSR <systemreg>, <Xt>.  */
#define QL_SRC_X		\
{				\
  QLF2(NIL,X),			\
}

/* e.g. MRS <Xt>, <systemreg>.  */
#define QL_DST_X		\
{				\
  QLF2(X,NIL),			\
}

/* e.g. SYS #<op1>, <Cn>, <Cm>, #<op2>{, <Xt>}.  */
#define QL_SYS			\
{				\
  QLF5(NIL,NIL,NIL,NIL,X),	\
}

/* e.g. SYSL <Xt>, #<op1>, <Cn>, <Cm>, #<op2>.  */
#define QL_SYSL			\
{				\
  QLF5(X,NIL,NIL,NIL,NIL),	\
}

/* e.g. ADRP <Xd>, <label>.  */
#define QL_ADRP			\
{				\
  QLF2(X,NIL),			\
}

/* e.g. B.<cond> <label>.  */
#define QL_PCREL_NIL		\
{				\
  QLF1(NIL),			\
}

/* e.g. TBZ <Xt>, #<imm>, <label>.  */
#define QL_PCREL_14		\
{				\
  QLF3(X,imm_0_63,NIL),		\
}

/* e.g. BL <label>.  */
#define QL_PCREL_26		\
{				\
  QLF1(NIL),			\
}

/* e.g. LDRSW <Xt>, <label>.  */
#define QL_X_PCREL		\
{				\
  QLF2(X,NIL),			\
}

/* e.g. LDR <Wt>, <label>.  */
#define QL_R_PCREL		\
{				\
  QLF2(W,NIL),			\
  QLF2(X,NIL),			\
}

/* e.g. LDR <Dt>, <label>.  */
#define QL_FP_PCREL		\
{				\
  QLF2(S_S,NIL),		\
  QLF2(S_D,NIL),		\
  QLF2(S_Q,NIL),		\
}

/* e.g. PRFM <prfop>, <label>.  */
#define QL_PRFM_PCREL		\
{				\
  QLF2(NIL,NIL),		\
}

/* e.g. BR <Xn>.  */
#define QL_I1X			\
{				\
  QLF1(X),			\
}

/* e.g. RBIT <Wd>, <Wn>.  */
#define QL_I2SAME		\
{				\
  QLF2(W,W),			\
  QLF2(X,X),			\
}

/* e.g. CMN <Wn|WSP>, <Wm>{, <extend> {#<amount>}}.  */
#define QL_I2_EXT		\
{				\
  QLF2(W,W),			\
  QLF2(X,W),			\
  QLF2(X,X),			\
}

/* e.g. MOV <Wd|WSP>, <Wn|WSP>, at least one SP.  */
#define QL_I2SP			\
{				\
  QLF2(WSP,W),			\
  QLF2(W,WSP),			\
  QLF2(SP,X),			\
  QLF2(X,SP),			\
}

/* e.g. REV <Wd>, <Wn>.  */
#define QL_I2SAMEW		\
{				\
  QLF2(W,W),			\
}

/* e.g. REV32 <Xd>, <Xn>.  */
#define QL_I2SAMEX		\
{				\
  QLF2(X,X),			\
}

#define QL_I2SAMER		\
{				\
  QLF2(W,W),			\
  QLF2(X,X),			\
}

/* e.g. CRC32B <Wd>, <Wn>, <Wm>.  */
#define QL_I3SAMEW		\
{				\
  QLF3(W,W,W),			\
}

/* e.g. SMULH <Xd>, <Xn>, <Xm>.  */
#define QL_I3SAMEX		\
{				\
  QLF3(X,X,X),			\
}

/* e.g. CRC32X <Wd>, <Wn>, <Xm>.  */
#define QL_I3WWX		\
{				\
  QLF3(W,W,X),			\
}

/* e.g. UDIV <Xd>, <Xn>, <Xm>.  */
#define QL_I3SAMER		\
{				\
  QLF3(W,W,W),			\
  QLF3(X,X,X),			\
}

/* e.g. ADDS <Xd>, <Xn|SP>, <R><m>{, <extend> {#<amount>}}.  */
#define QL_I3_EXT		\
{				\
  QLF3(W,W,W),			\
  QLF3(X,X,W),			\
  QLF3(X,X,X),			\
}

/* e.g. MADD <Xd>, <Xn>, <Xm>, <Xa>.  */
#define QL_I4SAMER		\
{				\
  QLF4(W,W,W,W),		\
  QLF4(X,X,X,X),		\
}

/* e.g. SMADDL <Xd>, <Wn>, <Wm>, <Xa>.  */
#define QL_I3SAMEL		\
{				\
  QLF3(X,W,W),			\
}

/* e.g. SMADDL <Xd>, <Wn>, <Wm>, <Xa>.  */
#define QL_I4SAMEL		\
{				\
  QLF4(X,W,W,X),		\
}

/* e.g. CSINC <Xd>, <Xn>, <Xm>, <cond>.  */
#define QL_CSEL			\
{				\
  QLF4(W, W, W, NIL),		\
  QLF4(X, X, X, NIL),		\
}

/* e.g. CSET <Wd>, <cond>.  */
#define QL_DST_R			\
{				\
  QLF2(W, NIL),			\
  QLF2(X, NIL),			\
}

/* e.g. BFM <Wd>, <Wn>, #<immr>, #<imms>.  */
#define QL_BF			\
{				\
  QLF4(W,W,imm_0_31,imm_0_31),	\
  QLF4(X,X,imm_0_63,imm_0_63),	\
}

/* e.g. BFC <Wd>, #<immr>, #<imms>.  */
#define QL_BF1					\
{						\
  QLF3 (W, imm_0_31, imm_1_32),			\
  QLF3 (X, imm_0_63, imm_1_64),			\
}

/* e.g. UBFIZ <Wd>, <Wn>, #<lsb>, #<width>.  */
#define QL_BF2			\
{				\
  QLF4(W,W,imm_0_31,imm_1_32),	\
  QLF4(X,X,imm_0_63,imm_1_64),	\
}

/* e.g. SCVTF <Sd>, <Xn>, #<fbits>.  */
#define QL_FIX2FP		\
{				\
  QLF3(S_D,W,imm_1_32),		\
  QLF3(S_S,W,imm_1_32),		\
  QLF3(S_D,X,imm_1_64),		\
  QLF3(S_S,X,imm_1_64),		\
}

/* e.g. SCVTF <Hd>, <Xn>, #<fbits>.  */
#define QL_FIX2FP_H			\
{					\
  QLF3 (S_H, W, imm_1_32),		\
  QLF3 (S_H, X, imm_1_64),		\
}

/* e.g. FCVTZS <Wd>, <Dn>, #<fbits>.  */
#define QL_FP2FIX		\
{				\
  QLF3(W,S_D,imm_1_32),		\
  QLF3(W,S_S,imm_1_32),		\
  QLF3(X,S_D,imm_1_64),		\
  QLF3(X,S_S,imm_1_64),		\
}

/* e.g. FCVTZS <Wd>, <Hn>, #<fbits>.  */
#define QL_FP2FIX_H			\
{					\
  QLF3 (W, S_H, imm_1_32),		\
  QLF3 (X, S_H, imm_1_64),		\
}

/* e.g. SCVTF <Dd>, <Wn>.  */
#define QL_INT2FP		\
{				\
  QLF2(S_D,W),			\
  QLF2(S_S,W),			\
  QLF2(S_D,X),			\
  QLF2(S_S,X),			\
}

/* e.g. SCVTF <Hd>, <Wn>.  */
#define QL_INT2FP_H			\
{					\
  QLF2 (S_H, W),			\
  QLF2 (S_H, X),			\
}

/* e.g. FCVTNS <Xd>, <Dn>.  */
#define QL_FP2INT		\
{				\
  QLF2(W,S_D),			\
  QLF2(W,S_S),			\
  QLF2(X,S_D),			\
  QLF2(X,S_S),			\
}

/* e.g. FCVTNS <Hd>, <Wn>.  */
#define QL_FP2INT_H			\
{					\
  QLF2 (W, S_H),			\
  QLF2 (X, S_H),			\
}

/* e.g. FMOV <Xd>, <Vn>.D[1].  */
#define QL_XVD1			\
{				\
  QLF2(X,S_D),			\
}

/* e.g. FMOV <Vd>.D[1], <Xn>.  */
#define QL_VD1X			\
{				\
  QLF2(S_D,X),			\
}

/* e.g. EXTR <Xd>, <Xn>, <Xm>, #<lsb>.  */
#define QL_EXTR			\
{				\
  QLF4(W,W,W,imm_0_31),		\
  QLF4(X,X,X,imm_0_63),		\
}

/* e.g. LSL <Wd>, <Wn>, #<uimm>.  */
#define QL_SHIFT		\
{				\
  QLF3(W,W,imm_0_31),		\
  QLF3(X,X,imm_0_63),		\
}

/* e.g. UXTH <Xd>, <Wn>.  */
#define QL_EXT			\
{				\
  QLF2(W,W),			\
  QLF2(X,W),			\
}

/* e.g. UXTW <Xd>, <Wn>.  */
#define QL_EXT_W		\
{				\
  QLF2(X,W),			\
}

/* e.g. SQSHL <V><d>, <V><n>, #<shift>.  */
#define QL_SSHIFT		\
{				\
  QLF3(S_B , S_B , S_B ),	\
  QLF3(S_H , S_H , S_H ),	\
  QLF3(S_S , S_S , S_S ),	\
  QLF3(S_D , S_D , S_D )	\
}

/* e.g. SSHR <V><d>, <V><n>, #<shift>.  */
#define QL_SSHIFT_D		\
{				\
  QLF3(S_D , S_D , S_D )	\
}

/* e.g. UCVTF <Vd>.<T>, <Vn>.<T>, #<fbits>.  */
#define QL_SSHIFT_SD		\
{				\
  QLF3(S_S , S_S , S_S ),	\
  QLF3(S_D , S_D , S_D )	\
}

/* e.g. UCVTF <Vd>.<T>, <Vn>.<T>, #<fbits>.  */
#define QL_SSHIFT_H		\
{				\
  QLF3 (S_H, S_H, S_H)		\
}

/* e.g. SQSHRUN <Vb><d>, <Va><n>, #<shift>.  */
#define QL_SSHIFTN		\
{				\
  QLF3(S_B , S_H , S_B ),	\
  QLF3(S_H , S_S , S_H ),	\
  QLF3(S_S , S_D , S_S ),	\
}

/* e.g. SSHR <Vd>.<T>, <Vn>.<T>, #<shift>.
   The register operand variant qualifiers are deliberately used for the
   immediate operand to ease the operand encoding/decoding and qualifier
   sequence matching.  */
#define QL_VSHIFT		\
{				\
  QLF3(V_8B , V_8B , V_8B ),	\
  QLF3(V_16B, V_16B, V_16B),	\
  QLF3(V_4H , V_4H , V_4H ),	\
  QLF3(V_8H , V_8H , V_8H ),	\
  QLF3(V_2S , V_2S , V_2S ),	\
  QLF3(V_4S , V_4S , V_4S ),	\
  QLF3(V_2D , V_2D , V_2D )	\
}

/* e.g. SCVTF <Vd>.<T>, <Vn>.<T>, #<fbits>.  */
#define QL_VSHIFT_SD		\
{				\
  QLF3(V_2S , V_2S , V_2S ),	\
  QLF3(V_4S , V_4S , V_4S ),	\
  QLF3(V_2D , V_2D , V_2D )	\
}

/* e.g. SCVTF <Vd>.<T>, <Vn>.<T>, #<fbits>.  */
#define QL_VSHIFT_H		\
{				\
  QLF3 (V_4H, V_4H, V_4H),	\
  QLF3 (V_8H, V_8H, V_8H)	\
}

/* e.g. SHRN<Q> <Vd>.<Tb>, <Vn>.<Ta>, #<shift>.  */
#define QL_VSHIFTN		\
{				\
  QLF3(V_8B , V_8H , V_8B ),	\
  QLF3(V_4H , V_4S , V_4H ),	\
  QLF3(V_2S , V_2D , V_2S ),	\
}

/* e.g. SHRN<Q> <Vd>.<Tb>, <Vn>.<Ta>, #<shift>.  */
#define QL_VSHIFTN2		\
{				\
  QLF3(V_16B, V_8H, V_16B),	\
  QLF3(V_8H , V_4S , V_8H ),	\
  QLF3(V_4S , V_2D , V_4S ),	\
}

/* e.g. SSHLL<Q> <Vd>.<Ta>, <Vn>.<Tb>, #<shift>.
   the 3rd qualifier is used to help the encoding.  */
#define QL_VSHIFTL		\
{				\
  QLF3(V_8H , V_8B , V_8B ),	\
  QLF3(V_4S , V_4H , V_4H ),	\
  QLF3(V_2D , V_2S , V_2S ),	\
}

/* e.g. SSHLL<Q> <Vd>.<Ta>, <Vn>.<Tb>, #<shift>.  */
#define QL_VSHIFTL2		\
{				\
  QLF3(V_8H , V_16B, V_16B),	\
  QLF3(V_4S , V_8H , V_8H ),	\
  QLF3(V_2D , V_4S , V_4S ),	\
}

/* e.g. TBL.  */
#define QL_TABLE		\
{				\
  QLF3(V_8B , V_16B, V_8B ),	\
  QLF3(V_16B, V_16B, V_16B),	\
}

/* e.g. SHA1H.  */
#define QL_2SAMES		\
{				\
  QLF2(S_S, S_S),		\
}

/* e.g. ABS <V><d>, <V><n>.  */
#define QL_2SAMED		\
{				\
  QLF2(S_D, S_D),		\
}

/* e.g. CMGT <V><d>, <V><n>, #0.  */
#define QL_SISD_CMP_0		\
{				\
  QLF3(S_D, S_D, NIL),		\
}

/* e.g. FCMEQ <V><d>, <V><n>, #0.  */
#define QL_SISD_FCMP_0		\
{				\
  QLF3(S_S, S_S, NIL),		\
  QLF3(S_D, S_D, NIL),		\
}

/* e.g. FCMEQ <V><d>, <V><n>, #0.  */
#define QL_SISD_FCMP_H_0	\
{				\
  QLF3 (S_H, S_H, NIL),		\
}

/* e.g. FMAXNMP <V><d>, <Vn>.<T>.  */
#define QL_SISD_PAIR		\
{				\
  QLF2(S_S, V_2S),		\
  QLF2(S_D, V_2D),		\
}

/* e.g. FMAXNMP <V><d>, <Vn>.<T>.  */
#define QL_SISD_PAIR_H		\
{				\
  QLF2 (S_H, V_2H),		\
}

/* e.g. ADDP <V><d>, <Vn>.<T>.  */
#define QL_SISD_PAIR_D		\
{				\
  QLF2(S_D, V_2D),		\
}

/* e.g. DUP <V><d>, <Vn>.<T>[<index>].  */
#define QL_S_2SAME		\
{				\
  QLF2(S_B, S_B),		\
  QLF2(S_H, S_H),		\
  QLF2(S_S, S_S),		\
  QLF2(S_D, S_D),		\
}

/* e.g. FCVTNS <V><d>, <V><n>.  */
#define QL_S_2SAMESD		\
{				\
  QLF2(S_S, S_S),		\
  QLF2(S_D, S_D),		\
}

/* e.g. FCVTNS <V><d>, <V><n>.  */
#define QL_S_2SAMEH		\
{				\
  QLF2 (S_H, S_H),		\
}

/* e.g. SQXTN <Vb><d>, <Va><n>.  */
#define QL_SISD_NARROW		\
{				\
  QLF2(S_B, S_H),		\
  QLF2(S_H, S_S),		\
  QLF2(S_S, S_D),		\
}

/* e.g. FCVTXN <Vb><d>, <Va><n>.  */
#define QL_SISD_NARROW_S	\
{				\
  QLF2(S_S, S_D),		\
}

/* e.g. FCVT.  */
#define QL_FCVT			\
{				\
  QLF2(S_S, S_H),		\
  QLF2(S_S, S_D),		\
  QLF2(S_D, S_H),		\
  QLF2(S_D, S_S),		\
  QLF2(S_H, S_S),		\
  QLF2(S_H, S_D),		\
}

/* FMOV <Dd>, <Dn>.  */
#define QL_FP2			\
{				\
  QLF2(S_S, S_S),		\
  QLF2(S_D, S_D),		\
}

/* FMOV <Hd>, <Hn>.  */
#define QL_FP2_H		\
{				\
  QLF2 (S_H, S_H),		\
}

/* e.g. SQADD <V><d>, <V><n>, <V><m>.  */
#define QL_S_3SAME		\
{				\
  QLF3(S_B, S_B, S_B),		\
  QLF3(S_H, S_H, S_H),		\
  QLF3(S_S, S_S, S_S),		\
  QLF3(S_D, S_D, S_D),		\
}

/* e.g. CMGE <V><d>, <V><n>, <V><m>.  */
#define QL_S_3SAMED		\
{				\
  QLF3(S_D, S_D, S_D),		\
}

/* e.g. SQDMULH <V><d>, <V><n>, <V><m>.  */
#define QL_SISD_HS		\
{				\
  QLF3(S_H, S_H, S_H),		\
  QLF3(S_S, S_S, S_S),		\
}

/* e.g. SQDMLAL <Va><d>, <Vb><n>, <Vb><m>.  */
#define QL_SISDL_HS		\
{				\
  QLF3(S_S, S_H, S_H),		\
  QLF3(S_D, S_S, S_S),		\
}

/* FMUL <Sd>, <Sn>, <Sm>.  */
#define QL_FP3			\
{				\
  QLF3(S_S, S_S, S_S),		\
  QLF3(S_D, S_D, S_D),		\
}

/* FMUL <Hd>, <Hn>, <Hm>.  */
#define QL_FP3_H		\
{				\
  QLF3 (S_H, S_H, S_H),		\
}

/* FMADD <Dd>, <Dn>, <Dm>, <Da>.  */
#define QL_FP4			\
{				\
  QLF4(S_S, S_S, S_S, S_S),	\
  QLF4(S_D, S_D, S_D, S_D),	\
}

/* FMADD <Hd>, <Hn>, <Hm>, <Ha>.  */
#define QL_FP4_H		\
{				\
  QLF4 (S_H, S_H, S_H, S_H),	\
}

/* e.g. FCMP <Dn>, #0.0.  */
#define QL_DST_SD			\
{				\
  QLF2(S_S, NIL),		\
  QLF2(S_D, NIL),		\
}

/* e.g. FCMP <Hn>, #0.0.  */
#define QL_DST_H		\
{				\
  QLF2 (S_H, NIL),		\
}

/* FCSEL <Sd>, <Sn>, <Sm>, <cond>.  */
#define QL_FP_COND		\
{				\
  QLF4(S_S, S_S, S_S, NIL),	\
  QLF4(S_D, S_D, S_D, NIL),	\
}

/* FCSEL <Hd>, <Hn>, <Hm>, <cond>.  */
#define QL_FP_COND_H		\
{				\
  QLF4 (S_H, S_H, S_H, NIL),	\
}

/* e.g. CCMN <Xn>, <Xm>, #<nzcv>, <cond>.  */
#define QL_CCMP			\
{				\
  QLF4(W, W, NIL, NIL),		\
  QLF4(X, X, NIL, NIL),		\
}

/* e.g. CCMN <Xn>, #<imm>, #<nzcv>, <cond>,  */
#define QL_CCMP_IMM		\
{				\
  QLF4(W, NIL, NIL, NIL),	\
  QLF4(X, NIL, NIL, NIL),	\
}

/* e.g. FCCMP <Sn>, <Sm>, #<nzcv>, <cond>.  */
#define QL_FCCMP		\
{				\
  QLF4(S_S, S_S, NIL, NIL),	\
  QLF4(S_D, S_D, NIL, NIL),	\
}

/* e.g. FCCMP <Sn>, <Sm>, #<nzcv>, <cond>.  */
#define QL_FCCMP_H		\
{				\
  QLF4 (S_H, S_H, NIL, NIL),	\
}

/* e.g. DUP <Vd>.<T>, <Vn>.<Ts>[<index>].  */
#define QL_DUP_VX		\
{				\
  QLF2(V_8B , S_B ),		\
  QLF2(V_16B, S_B ),		\
  QLF2(V_4H , S_H ),		\
  QLF2(V_8H , S_H ),		\
  QLF2(V_2S , S_S ),		\
  QLF2(V_4S , S_S ),		\
  QLF2(V_2D , S_D ),		\
}

/* e.g. DUP <Vd>.<T>, <Wn>.  */
#define QL_DUP_VR		\
{				\
  QLF2(V_8B , W ),		\
  QLF2(V_16B, W ),		\
  QLF2(V_4H , W ),		\
  QLF2(V_8H , W ),		\
  QLF2(V_2S , W ),		\
  QLF2(V_4S , W ),		\
  QLF2(V_2D , X ),		\
}

/* e.g. INS <Vd>.<Ts>[<index>], <Wn>.  */
#define QL_INS_XR		\
{				\
  QLF2(S_H , W ),		\
  QLF2(S_S , W ),		\
  QLF2(S_D , X ),		\
  QLF2(S_B , W ),		\
}

/* e.g. SMOV <Wd>, <Vn>.<Ts>[<index>].  */
#define QL_SMOV			\
{				\
  QLF2(W , S_H),		\
  QLF2(X , S_H),		\
  QLF2(X , S_S),		\
  QLF2(W , S_B),		\
  QLF2(X , S_B),		\
}

/* e.g. UMOV <Wd>, <Vn>.<Ts>[<index>].  */
#define QL_UMOV			\
{				\
  QLF2(W , S_H),		\
  QLF2(W , S_S),		\
  QLF2(X , S_D),		\
  QLF2(W , S_B),		\
}

/* e.g. MOV <Wd>, <Vn>.<Ts>[<index>].  */
#define QL_MOV			\
{				\
  QLF2(W , S_S),		\
  QLF2(X , S_D),		\
}

/* e.g. SUQADD <Vd>.<T>, <Vn>.<T>.  */
#define QL_V2SAME		\
{				\
  QLF2(V_8B , V_8B ),		\
  QLF2(V_16B, V_16B),		\
  QLF2(V_4H , V_4H ),		\
  QLF2(V_8H , V_8H ),		\
  QLF2(V_2S , V_2S ),		\
  QLF2(V_4S , V_4S ),		\
  QLF2(V_2D , V_2D ),		\
}

/* e.g. URSQRTE <Vd>.<T>, <Vn>.<T>.  */
#define QL_V2SAMES		\
{				\
  QLF2(V_2S , V_2S ),		\
  QLF2(V_4S , V_4S ),		\
}

/* e.g. REV32 <Vd>.<T>, <Vn>.<T>.  */
#define QL_V2SAMEBH		\
{				\
  QLF2(V_8B , V_8B ),		\
  QLF2(V_16B, V_16B),		\
  QLF2(V_4H , V_4H ),		\
  QLF2(V_8H , V_8H ),		\
}

/* e.g. FRINTN <Vd>.<T>, <Vn>.<T>.  */
#define QL_V2SAMESD		\
{				\
  QLF2(V_2S , V_2S ),		\
  QLF2(V_4S , V_4S ),		\
  QLF2(V_2D , V_2D ),		\
}

/* e.g. REV64 <Vd>.<T>, <Vn>.<T>.  */
#define QL_V2SAMEBHS		\
{				\
  QLF2(V_8B , V_8B ),		\
  QLF2(V_16B, V_16B),		\
  QLF2(V_4H , V_4H ),		\
  QLF2(V_8H , V_8H ),		\
  QLF2(V_2S , V_2S ),		\
  QLF2(V_4S , V_4S ),		\
}

/* e.g. FCMGT <Vd>.<T>, <Vd>.<T>>, #0.0.  */
#define QL_V2SAMEH		\
{				\
  QLF2 (V_4H, V_4H),		\
  QLF2 (V_8H, V_8H),		\
}

/* e.g. REV16 <Vd>.<T>, <Vn>.<T>.  */
#define QL_V2SAMEB		\
{				\
  QLF2(V_8B , V_8B ),		\
  QLF2(V_16B, V_16B),		\
}

/* e.g. SADDLP <Vd>.<Ta>, <Vn>.<Tb>.  */
#define QL_V2PAIRWISELONGBHS		\
{				\
  QLF2(V_4H , V_8B ),		\
  QLF2(V_8H , V_16B),		\
  QLF2(V_2S , V_4H ),		\
  QLF2(V_4S , V_8H ),		\
  QLF2(V_1D , V_2S ),		\
  QLF2(V_2D , V_4S ),		\
}

/* e.g. SHLL<Q> <Vd>.<Ta>, <Vn>.<Tb>, #<shift>.  */
#define QL_V2LONGBHS		\
{				\
  QLF2(V_8H , V_8B ),		\
  QLF2(V_4S , V_4H ),		\
  QLF2(V_2D , V_2S ),		\
}

/* e.g. SHLL<Q> <Vd>.<Ta>, <Vn>.<Tb>, #<shift>.  */
#define QL_V2LONGBHS2		\
{				\
  QLF2(V_8H , V_16B),		\
  QLF2(V_4S , V_8H ),		\
  QLF2(V_2D , V_4S ),		\
}

/* */
#define QL_V3SAME		\
{				\
  QLF3(V_8B , V_8B , V_8B ),	\
  QLF3(V_16B, V_16B, V_16B),	\
  QLF3(V_4H , V_4H , V_4H ),	\
  QLF3(V_8H , V_8H , V_8H ),	\
  QLF3(V_2S , V_2S , V_2S ),	\
  QLF3(V_4S , V_4S , V_4S ),	\
  QLF3(V_2D , V_2D , V_2D )	\
}

/* e.g. SHADD.  */
#define QL_V3SAMEBHS		\
{				\
  QLF3(V_8B , V_8B , V_8B ),	\
  QLF3(V_16B, V_16B, V_16B),	\
  QLF3(V_4H , V_4H , V_4H ),	\
  QLF3(V_8H , V_8H , V_8H ),	\
  QLF3(V_2S , V_2S , V_2S ),	\
  QLF3(V_4S , V_4S , V_4S ),	\
}

/* e.g. FCVTXN<Q> <Vd>.<Tb>, <Vn>.<Ta>.  */
#define QL_V2NARRS		\
{				\
  QLF2(V_2S , V_2D ),		\
}

/* e.g. FCVTXN<Q> <Vd>.<Tb>, <Vn>.<Ta>.  */
#define QL_V2NARRS2		\
{				\
  QLF2(V_4S , V_2D ),		\
}

/* e.g. FCVTN<Q> <Vd>.<Tb>, <Vn>.<Ta>.  */
#define QL_V2NARRHS		\
{				\
  QLF2(V_4H , V_4S ),		\
  QLF2(V_2S , V_2D ),		\
}

/* e.g. FCVTN<Q> <Vd>.<Tb>, <Vn>.<Ta>.  */
#define QL_V2NARRHS2		\
{				\
  QLF2(V_8H , V_4S ),		\
  QLF2(V_4S , V_2D ),		\
}

/* e.g. FCVTL<Q> <Vd>.<Ta>, <Vn>.<Tb>.  */
#define QL_V2LONGHS		\
{				\
  QLF2(V_4S , V_4H ),		\
  QLF2(V_2D , V_2S ),		\
}

/* e.g. FCVTL<Q> <Vd>.<Ta>, <Vn>.<Tb>.  */
#define QL_V2LONGHS2		\
{				\
  QLF2(V_4S , V_8H ),		\
  QLF2(V_2D , V_4S ),		\
}

/* e.g. XTN<Q> <Vd>.<Tb>, <Vn>.<Ta>.  */
#define QL_V2NARRBHS		\
{				\
  QLF2(V_8B , V_8H ),		\
  QLF2(V_4H , V_4S ),		\
  QLF2(V_2S , V_2D ),		\
}

/* e.g. XTN<Q> <Vd>.<Tb>, <Vn>.<Ta>.  */
#define QL_V2NARRBHS2		\
{				\
  QLF2(V_16B, V_8H ),		\
  QLF2(V_8H , V_4S ),		\
  QLF2(V_4S , V_2D ),		\
}

/* e.g. ORR.  */
#define QL_V2SAMEB		\
{				\
  QLF2(V_8B , V_8B ),		\
  QLF2(V_16B, V_16B),		\
}

/* e.g. AESE.  */
#define QL_V2SAME16B		\
{				\
  QLF2(V_16B, V_16B),		\
}

/* e.g. SHA1SU1.  */
#define QL_V2SAME4S		\
{				\
  QLF2(V_4S, V_4S),		\
}

/* e.g. SHA1SU0.  */
#define QL_V3SAME4S		\
{				\
  QLF3(V_4S, V_4S, V_4S),	\
}

/* e.g. SHADD.  */
#define QL_V3SAMEB		\
{				\
  QLF3(V_8B , V_8B , V_8B ),	\
  QLF3(V_16B, V_16B, V_16B),	\
}

/* e.g. EXT <Vd>.<T>, <Vn>.<T>, <Vm>.<T>, #<index>.  */
#define QL_VEXT			\
{					\
  QLF4(V_8B , V_8B , V_8B , imm_0_7),	\
  QLF4(V_16B, V_16B, V_16B, imm_0_15),	\
}

/* e.g. .  */
#define QL_V3SAMEHS		\
{				\
  QLF3(V_4H , V_4H , V_4H ),	\
  QLF3(V_8H , V_8H , V_8H ),	\
  QLF3(V_2S , V_2S , V_2S ),	\
  QLF3(V_4S , V_4S , V_4S ),	\
}

/* */
#define QL_V3SAMESD		\
{				\
  QLF3(V_2S , V_2S , V_2S ),	\
  QLF3(V_4S , V_4S , V_4S ),	\
  QLF3(V_2D , V_2D , V_2D )	\
}

/* e.g. FMAXNM <Vd>.<T>, <Vn>.<T>, <Vm>.<T>.  */
#define QL_V3SAMEH		\
{				\
  QLF3 (V_4H , V_4H , V_4H ),	\
  QLF3 (V_8H , V_8H , V_8H ),	\
}

/* e.g. SQDMLAL<Q> <Vd>.<Ta>, <Vn>.<Tb>, <Vm>.<Tb>.  */
#define QL_V3LONGHS		\
{				\
  QLF3(V_4S , V_4H , V_4H ),	\
  QLF3(V_2D , V_2S , V_2S ),	\
}

/* e.g. SQDMLAL<Q> <Vd>.<Ta>, <Vn>.<Tb>, <Vm>.<Tb>.  */
#define QL_V3LONGHS2		\
{				\
  QLF3(V_4S , V_8H , V_8H ),	\
  QLF3(V_2D , V_4S , V_4S ),	\
}

/* e.g. SADDL<Q> <Vd>.<Ta>, <Vn>.<Tb>, <Vm>.<Tb>.  */
#define QL_V3LONGBHS		\
{				\
  QLF3(V_8H , V_8B , V_8B ),	\
  QLF3(V_4S , V_4H , V_4H ),	\
  QLF3(V_2D , V_2S , V_2S ),	\
}

/* e.g. SADDL<Q> <Vd>.<Ta>, <Vn>.<Tb>, <Vm>.<Tb>.  */
#define QL_V3LONGBHS2		\
{				\
  QLF3(V_8H , V_16B , V_16B ),	\
  QLF3(V_4S , V_8H , V_8H ),	\
  QLF3(V_2D , V_4S , V_4S ),	\
}

/* e.g. SADDW<Q> <Vd>.<Ta>, <Vn>.<Ta>, <Vm>.<Tb>.  */
#define QL_V3WIDEBHS		\
{				\
  QLF3(V_8H , V_8H , V_8B ),	\
  QLF3(V_4S , V_4S , V_4H ),	\
  QLF3(V_2D , V_2D , V_2S ),	\
}

/* e.g. SADDW<Q> <Vd>.<Ta>, <Vn>.<Ta>, <Vm>.<Tb>.  */
#define QL_V3WIDEBHS2		\
{				\
  QLF3(V_8H , V_8H , V_16B ),	\
  QLF3(V_4S , V_4S , V_8H ),	\
  QLF3(V_2D , V_2D , V_4S ),	\
}

/* e.g. ADDHN<Q> <Vd>.<Tb>, <Vn>.<Ta>, <Vm>.<Ta>.  */
#define QL_V3NARRBHS		\
{				\
  QLF3(V_8B , V_8H , V_8H ),	\
  QLF3(V_4H , V_4S , V_4S ),	\
  QLF3(V_2S , V_2D , V_2D ),	\
}

/* e.g. ADDHN<Q> <Vd>.<Tb>, <Vn>.<Ta>, <Vm>.<Ta>.  */
#define QL_V3NARRBHS2		\
{				\
  QLF3(V_16B , V_8H , V_8H ),	\
  QLF3(V_8H , V_4S , V_4S ),	\
  QLF3(V_4S , V_2D , V_2D ),	\
}

/* e.g. PMULL.  */
#define QL_V3LONGB		\
{				\
  QLF3(V_8H , V_8B , V_8B ),	\
}

/* e.g. PMULL crypto.  */
#define QL_V3LONGD		\
{				\
  QLF3(V_1Q , V_1D , V_1D ),	\
}

/* e.g. PMULL2.  */
#define QL_V3LONGB2		\
{				\
  QLF3(V_8H , V_16B, V_16B),	\
}

/* e.g. PMULL2 crypto.  */
#define QL_V3LONGD2		\
{				\
  QLF3(V_1Q , V_2D , V_2D ),	\
}

/* e.g. SHA1C.  */
#define QL_SHAUPT		\
{				\
  QLF3(S_Q, S_S, V_4S),		\
}

/* e.g. SHA256H2.  */
#define QL_SHA256UPT		\
{				\
  QLF3(S_Q, S_Q, V_4S),		\
}

/* e.g. LDXRB <Wt>, [<Xn|SP>{,#0}].  */
#define QL_W1_LDST_EXC		\
{				\
  QLF2(W, NIL),			\
}

/* e.g. LDXR <Xt>, [<Xn|SP>{,#0}].  */
#define QL_R1NIL		\
{				\
  QLF2(W, NIL),			\
  QLF2(X, NIL),			\
}

/* e.g. STXRB <Ws>, <Wt>, [<Xn|SP>{,#0}].  */
#define QL_W2_LDST_EXC		\
{				\
  QLF3(W, W, NIL),		\
}

/* e.g. STXR <Ws>, <Xt>, [<Xn|SP>{,#0}].  */
#define QL_R2_LDST_EXC		\
{				\
  QLF3(W, W, NIL),		\
  QLF3(W, X, NIL),		\
}

/* e.g. LDXP <Xt1>, <Xt2>, [<Xn|SP>{,#0}].  */
#define QL_R2NIL		\
{				\
  QLF3(W, W, NIL),		\
  QLF3(X, X, NIL),		\
}

/* e.g. CASP <Xt1>, <Xt1+1>, <Xt2>, <Xt2+1>, [<Xn|SP>{,#0}].  */
#define QL_R4NIL		\
{				\
  QLF5(W, W, W, W, NIL),	\
  QLF5(X, X, X, X, NIL),	\
}

/* e.g. STXP <Ws>, <Xt1>, <Xt2>, [<Xn|SP>{,#0}].  */
#define QL_R3_LDST_EXC		\
{				\
  QLF4(W, W, W, NIL),		\
  QLF4(W, X, X, NIL),		\
}

/* e.g. STR <Qt>, [<Xn|SP>, <R><m>{, <extend> {<amount>}}].  */
#define QL_LDST_FP		\
{				\
  QLF2(S_B, S_B),		\
  QLF2(S_H, S_H),		\
  QLF2(S_S, S_S),		\
  QLF2(S_D, S_D),		\
  QLF2(S_Q, S_Q),		\
}

/* e.g. STR <Xt>, [<Xn|SP>, <R><m>{, <extend> {<amount>}}].  */
#define QL_LDST_R		\
{				\
  QLF2(W, S_S),			\
  QLF2(X, S_D),			\
}

/* e.g. STRB <Wt>, [<Xn|SP>, <R><m>{, <extend> {<amount>}}].  */
#define QL_LDST_W8		\
{				\
  QLF2(W, S_B),			\
}

/* e.g. LDRSB <Wt>, [<Xn|SP>, <R><m>{, <extend> {<amount>}}].  */
#define QL_LDST_R8		\
{				\
  QLF2(W, S_B),			\
  QLF2(X, S_B),			\
}

/* e.g. STRH <Wt>, [<Xn|SP>, <R><m>{, <extend> {<amount>}}].  */
#define QL_LDST_W16		\
{				\
  QLF2(W, S_H),			\
}

/* e.g. LDRSW <Xt>, [<Xn|SP>, <R><m>{, <extend> {<amount>}}].  */
#define QL_LDST_X32		\
{				\
  QLF2(X, S_S),			\
}

/* e.g. LDRSH <Wt>, [<Xn|SP>, <R><m>{, <extend> {<amount>}}].  */
#define QL_LDST_R16		\
{				\
  QLF2(W, S_H),			\
  QLF2(X, S_H),			\
}

/* e.g. PRFM <prfop>, [<Xn|SP>, <R><m>{, <extend> {<amount>}}].  */
#define QL_LDST_PRFM		\
{				\
  QLF2(NIL, S_D),		\
}

/* e.g. LDPSW <Xt1>, <Xt2>, [<Xn|SP>{, #<imm>}].  */
#define QL_LDST_PAIR_X32	\
{				\
  QLF3(X, X, S_S),		\
}

/* e.g. STP <Wt1>, <Wt2>, [<Xn|SP>, #<imm>]!.  */
#define QL_LDST_PAIR_R		\
{				\
  QLF3(W, W, S_S),		\
  QLF3(X, X, S_D),		\
}

/* e.g. STNP <Qt1>, <Qt2>, [<Xn|SP>{, #<imm>}].  */
#define QL_LDST_PAIR_FP		\
{				\
  QLF3(S_S, S_S, S_S),		\
  QLF3(S_D, S_D, S_D),		\
  QLF3(S_Q, S_Q, S_Q),		\
}

/* e.g. LD3 {<Vt>.<T>, <Vt2>.<T>, <Vt3>.<T>}, [<Xn|SP>].  */
#define QL_SIMD_LDST		\
{				\
  QLF2(V_8B, NIL),		\
  QLF2(V_16B, NIL),		\
  QLF2(V_4H, NIL),		\
  QLF2(V_8H, NIL),		\
  QLF2(V_2S, NIL),		\
  QLF2(V_4S, NIL),		\
  QLF2(V_2D, NIL),		\
}

/* e.g. LD1 {<Vt>.<T>, <Vt2>.<T>, <Vt3>.<T>}, [<Xn|SP>].  */
#define QL_SIMD_LDST_ANY	\
{				\
  QLF2(V_8B, NIL),		\
  QLF2(V_16B, NIL),		\
  QLF2(V_4H, NIL),		\
  QLF2(V_8H, NIL),		\
  QLF2(V_2S, NIL),		\
  QLF2(V_4S, NIL),		\
  QLF2(V_1D, NIL),		\
  QLF2(V_2D, NIL),		\
}

/* e.g. LD4 {<Vt>.<T>, <Vt2a>.<T>, <Vt3a>.<T>, <Vt4a>.<T>}[<index>], [<Xn|SP>].  */
#define QL_SIMD_LDSTONE		\
{				\
  QLF2(S_B, NIL),		\
  QLF2(S_H, NIL),		\
  QLF2(S_S, NIL),		\
  QLF2(S_D, NIL),		\
}

/* e.g. ADDV <V><d>, <Vn>.<T>.  */
#define QL_XLANES		\
{				\
  QLF2(S_B, V_8B),		\
  QLF2(S_B, V_16B),		\
  QLF2(S_H, V_4H),		\
  QLF2(S_H, V_8H),		\
  QLF2(S_S, V_4S),		\
}

/* e.g. FMINV <V><d>, <Vn>.<T>.  */
#define QL_XLANES_FP		\
{				\
  QLF2(S_S, V_4S),		\
}

/* e.g. FMINV <V><d>, <Vn>.<T>.  */
#define QL_XLANES_FP_H		\
{				\
  QLF2 (S_H, V_4H),		\
  QLF2 (S_H, V_8H),		\
}

/* e.g. SADDLV <V><d>, <Vn>.<T>.  */
#define QL_XLANES_L		\
{				\
  QLF2(S_H, V_8B),		\
  QLF2(S_H, V_16B),		\
  QLF2(S_S, V_4H),		\
  QLF2(S_S, V_8H),		\
  QLF2(S_D, V_4S),		\
}

/* e.g. MUL <Vd>.<T>, <Vn>.<T>, <Vm>.<Ts>[<index>].  */
#define QL_ELEMENT		\
{				\
  QLF3(V_4H, V_4H, S_H),	\
  QLF3(V_8H, V_8H, S_H),	\
  QLF3(V_2S, V_2S, S_S),	\
  QLF3(V_4S, V_4S, S_S),	\
}

/* e.g. SMLAL <Vd>.<Ta>, <Vn>.<Tb>, <Vm>.<Ts>[<index>].  */
#define QL_ELEMENT_L		\
{				\
  QLF3(V_4S, V_4H, S_H),	\
  QLF3(V_2D, V_2S, S_S),	\
}

/* e.g. SMLAL2 <Vd>.<Ta>, <Vn>.<Tb>, <Vm>.<Ts>[<index>].  */
#define QL_ELEMENT_L2		\
{				\
  QLF3(V_4S, V_8H, S_H),	\
  QLF3(V_2D, V_4S, S_S),	\
}

/* e.g. FMLA <V><d>, <V><n>, <Vm>.<Ts>[<index>].  */
#define QL_ELEMENT_FP		\
{				\
  QLF3(V_2S, V_2S, S_S),	\
  QLF3(V_4S, V_4S, S_S),	\
  QLF3(V_2D, V_2D, S_D),	\
}

/* e.g. FMLA <V><d>, <V><n>, <Vm>.<Ts>[<index>].  */
#define QL_ELEMENT_FP_H		\
{				\
  QLF3 (V_4H, V_4H, S_H),	\
  QLF3 (V_8H, V_8H, S_H),	\
}

/* e.g. MOVI <Vd>.4S, #<imm8> {, LSL #<amount>}.  */
#define QL_SIMD_IMM_S0W		\
{				\
  QLF2(V_2S, LSL),		\
  QLF2(V_4S, LSL),		\
}

/* e.g. MOVI <Vd>.4S, #<imm8>, MSL #<amount>.  */
#define QL_SIMD_IMM_S1W		\
{				\
  QLF2(V_2S, MSL),		\
  QLF2(V_4S, MSL),		\
}

/* e.g. MOVI <Vd>.4H, #<imm8> {, LSL #<amount>}.  */
#define QL_SIMD_IMM_S0H		\
{				\
  QLF2(V_4H, LSL),		\
  QLF2(V_8H, LSL),		\
}

/* e.g. FMOV <Vd>.<T>, #<imm>.  */
#define QL_SIMD_IMM_S		\
{				\
  QLF2(V_2S, NIL),		\
  QLF2(V_4S, NIL),		\
}

/* e.g. MOVI <Vd>.8B, #<imm8> {, LSL #<amount>}.  */
#define QL_SIMD_IMM_B		\
{				\
  QLF2(V_8B, LSL),		\
  QLF2(V_16B, LSL),		\
}
/* e.g. MOVI <Dd>, #<imm>.  */
#define QL_SIMD_IMM_D		\
{				\
  QLF2(S_D, NIL),		\
}

/* e.g. FMOV <Vd>.<T>, #<imm>.  */
#define QL_SIMD_IMM_H		\
{				\
  QLF2 (V_4H, NIL),		\
  QLF2 (V_8H, NIL),		\
}

/* e.g. MOVI <Vd>.2D, #<imm>.  */
#define QL_SIMD_IMM_V2D		\
{				\
  QLF2(V_2D, NIL),		\
}

/* Opcode table.  */

static const aarch64_feature_set aarch64_feature_v8 =
  AARCH64_FEATURE (AARCH64_FEATURE_V8, 0);
static const aarch64_feature_set aarch64_feature_fp =
  AARCH64_FEATURE (AARCH64_FEATURE_FP, 0);
static const aarch64_feature_set aarch64_feature_simd =
  AARCH64_FEATURE (AARCH64_FEATURE_SIMD, 0);
static const aarch64_feature_set aarch64_feature_crypto =
  AARCH64_FEATURE (AARCH64_FEATURE_CRYPTO, 0);
static const aarch64_feature_set aarch64_feature_crc =
  AARCH64_FEATURE (AARCH64_FEATURE_CRC, 0);
static const aarch64_feature_set aarch64_feature_lse =
  AARCH64_FEATURE (AARCH64_FEATURE_LSE, 0);
static const aarch64_feature_set aarch64_feature_lor =
  AARCH64_FEATURE (AARCH64_FEATURE_LOR, 0);
static const aarch64_feature_set aarch64_feature_rdma =
  AARCH64_FEATURE (AARCH64_FEATURE_RDMA, 0);
static const aarch64_feature_set aarch64_feature_ras =
  AARCH64_FEATURE (AARCH64_FEATURE_RAS, 0);
static const aarch64_feature_set aarch64_feature_v8_2 =
  AARCH64_FEATURE (AARCH64_FEATURE_V8_2, 0);
static const aarch64_feature_set aarch64_feature_fp_f16 =
  AARCH64_FEATURE (AARCH64_FEATURE_F16 | AARCH64_FEATURE_FP, 0);
static const aarch64_feature_set aarch64_feature_simd_f16 =
  AARCH64_FEATURE (AARCH64_FEATURE_F16 | AARCH64_FEATURE_SIMD, 0);
static const aarch64_feature_set aarch64_feature_stat_profile =
  AARCH64_FEATURE (AARCH64_FEATURE_PROFILE, 0);

#define CORE		&aarch64_feature_v8
#define FP		&aarch64_feature_fp
#define SIMD		&aarch64_feature_simd
#define CRYPTO		&aarch64_feature_crypto
#define CRC		&aarch64_feature_crc
#define LSE		&aarch64_feature_lse
#define LOR		&aarch64_feature_lor
#define RDMA		&aarch64_feature_rdma
#define FP_F16		&aarch64_feature_fp_f16
#define SIMD_F16	&aarch64_feature_simd_f16
#define RAS		&aarch64_feature_ras
#define STAT_PROFILE	&aarch64_feature_stat_profile
#define ARMV8_2		&aarch64_feature_v8_2

#define CORE_INSN(NAME,OPCODE,MASK,CLASS,OPS,QUALS,FLAGS) \
  { NAME, OPCODE, MASK, CLASS, 0, CORE, OPS, QUALS, FLAGS, NULL }
#define __FP_INSN(NAME,OPCODE,MASK,CLASS,OPS,QUALS,FLAGS) \
  { NAME, OPCODE, MASK, CLASS, 0, FP, OPS, QUALS, FLAGS, NULL }
#define SIMD_INSN(NAME,OPCODE,MASK,CLASS,OPS,QUALS,FLAGS) \
  { NAME, OPCODE, MASK, CLASS, 0, SIMD, OPS, QUALS, FLAGS, NULL }
#define CRYP_INSN(NAME,OPCODE,MASK,CLASS,OPS,QUALS,FLAGS) \
  { NAME, OPCODE, MASK, CLASS, 0, CRYPTO, OPS, QUALS, FLAGS, NULL }
#define _CRC_INSN(NAME,OPCODE,MASK,CLASS,OPS,QUALS,FLAGS) \
  { NAME, OPCODE, MASK, CLASS, 0, CRC, OPS, QUALS, FLAGS, NULL }
#define _LSE_INSN(NAME,OPCODE,MASK,CLASS,OPS,QUALS,FLAGS) \
  { NAME, OPCODE, MASK, CLASS, 0, LSE, OPS, QUALS, FLAGS, NULL }
#define _LOR_INSN(NAME,OPCODE,MASK,CLASS,OPS,QUALS,FLAGS) \
  { NAME, OPCODE, MASK, CLASS, 0, LOR, OPS, QUALS, FLAGS, NULL }
#define RDMA_INSN(NAME,OPCODE,MASK,CLASS,OPS,QUALS,FLAGS) \
  { NAME, OPCODE, MASK, CLASS, 0, RDMA, OPS, QUALS, FLAGS, NULL }
#define FF16_INSN(NAME,OPCODE,MASK,CLASS,OPS,QUALS,FLAGS) \
  { NAME, OPCODE, MASK, CLASS, 0, FP_F16, OPS, QUALS, FLAGS, NULL }
#define SF16_INSN(NAME,OPCODE,MASK,CLASS,OPS,QUALS,FLAGS)		\
  { NAME, OPCODE, MASK, CLASS, 0, SIMD_F16, OPS, QUALS, FLAGS, NULL }

struct aarch64_opcode aarch64_opcode_table[] =
{
  /* Add/subtract (with carry).  */
  CORE_INSN ("adc",  0x1a000000, 0x7fe0fc00, addsub_carry, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF),
  CORE_INSN ("adcs", 0x3a000000, 0x7fe0fc00, addsub_carry, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF),
  CORE_INSN ("sbc",  0x5a000000, 0x7fe0fc00, addsub_carry, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_HAS_ALIAS | F_SF),
  CORE_INSN ("ngc",  0x5a0003e0, 0x7fe0ffe0, addsub_carry, OP2 (Rd, Rm),     QL_I2SAME,  F_ALIAS | F_SF),
  CORE_INSN ("sbcs", 0x7a000000, 0x7fe0fc00, addsub_carry, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_HAS_ALIAS | F_SF),
  CORE_INSN ("ngcs", 0x7a0003e0, 0x7fe0ffe0, addsub_carry, OP2 (Rd, Rm),     QL_I2SAME,  F_ALIAS | F_SF),
  /* Add/subtract (extended register).  */
  CORE_INSN ("add",  0x0b200000, 0x7fe00000, addsub_ext, OP3 (Rd_SP, Rn_SP, Rm_EXT), QL_I3_EXT, F_SF),
  CORE_INSN ("adds", 0x2b200000, 0x7fe00000, addsub_ext, OP3 (Rd, Rn_SP, Rm_EXT),    QL_I3_EXT, F_HAS_ALIAS | F_SF),
  CORE_INSN ("cmn",  0x2b20001f, 0x7fe0001f, addsub_ext, OP2 (Rn_SP, Rm_EXT),        QL_I2_EXT, F_ALIAS | F_SF),
  CORE_INSN ("sub",  0x4b200000, 0x7fe00000, addsub_ext, OP3 (Rd_SP, Rn_SP, Rm_EXT), QL_I3_EXT, F_SF),
  CORE_INSN ("subs", 0x6b200000, 0x7fe00000, addsub_ext, OP3 (Rd, Rn_SP, Rm_EXT),    QL_I3_EXT, F_HAS_ALIAS | F_SF),
  CORE_INSN ("cmp",  0x6b20001f, 0x7fe0001f, addsub_ext, OP2 (Rn_SP, Rm_EXT),        QL_I2_EXT, F_ALIAS | F_SF),
  /* Add/subtract (immediate).  */
  { "add",  0x11000000, 0x7f000000, addsub_imm, OP_ADD, CORE, OP3 (Rd_SP, Rn_SP, AIMM), QL_R2NIL, F_HAS_ALIAS | F_SF, NULL},
  CORE_INSN ("mov",  0x11000000, 0x7ffffc00, addsub_imm, OP2 (Rd_SP, Rn_SP),       QL_I2SP, F_ALIAS | F_SF),
  CORE_INSN ("adds", 0x31000000, 0x7f000000, addsub_imm, OP3 (Rd, Rn_SP, AIMM),    QL_R2NIL, F_HAS_ALIAS | F_SF),
  CORE_INSN ("cmn",  0x3100001f, 0x7f00001f, addsub_imm, OP2 (Rn_SP, AIMM),        QL_R1NIL, F_ALIAS | F_SF),
  CORE_INSN ("sub",  0x51000000, 0x7f000000, addsub_imm, OP3 (Rd_SP, Rn_SP, AIMM), QL_R2NIL, F_SF),
  CORE_INSN ("subs", 0x71000000, 0x7f000000, addsub_imm, OP3 (Rd, Rn_SP, AIMM),    QL_R2NIL, F_HAS_ALIAS | F_SF),
  CORE_INSN ("cmp",  0x7100001f, 0x7f00001f, addsub_imm, OP2 (Rn_SP, AIMM),        QL_R1NIL, F_ALIAS | F_SF),
  /* Add/subtract (shifted register).  */
  CORE_INSN ("add",  0x0b000000, 0x7f200000, addsub_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_SF),
  CORE_INSN ("adds", 0x2b000000, 0x7f200000, addsub_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_HAS_ALIAS | F_SF),
  CORE_INSN ("cmn",  0x2b00001f, 0x7f20001f, addsub_shift, OP2 (Rn, Rm_SFT),     QL_I2SAME,  F_ALIAS | F_SF),
  CORE_INSN ("sub",  0x4b000000, 0x7f200000, addsub_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_HAS_ALIAS | F_SF),
  CORE_INSN ("neg",  0x4b0003e0, 0x7f2003e0, addsub_shift, OP2 (Rd, Rm_SFT),     QL_I2SAME,  F_ALIAS | F_SF),
  CORE_INSN ("subs", 0x6b000000, 0x7f200000, addsub_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_HAS_ALIAS | F_SF),
  CORE_INSN ("cmp",  0x6b00001f, 0x7f20001f, addsub_shift, OP2 (Rn, Rm_SFT),     QL_I2SAME,  F_ALIAS | F_SF),
  CORE_INSN ("negs", 0x6b0003e0, 0x7f2003e0, addsub_shift, OP2 (Rd, Rm_SFT),     QL_I2SAME,  F_ALIAS | F_SF),
  /* AdvSIMD across lanes.  */
  SIMD_INSN ("saddlv", 0x0e303800, 0xbf3ffc00, asimdall, OP2 (Fd, Vn), QL_XLANES_L,    F_SIZEQ),
  SIMD_INSN ("smaxv",  0x0e30a800, 0xbf3ffc00, asimdall, OP2 (Fd, Vn), QL_XLANES,      F_SIZEQ),
  SIMD_INSN ("sminv",  0x0e31a800, 0xbf3ffc00, asimdall, OP2 (Fd, Vn), QL_XLANES,      F_SIZEQ),
  SIMD_INSN ("addv",   0x0e31b800, 0xbf3ffc00, asimdall, OP2 (Fd, Vn), QL_XLANES,      F_SIZEQ),
  SIMD_INSN ("uaddlv", 0x2e303800, 0xbf3ffc00, asimdall, OP2 (Fd, Vn), QL_XLANES_L,    F_SIZEQ),
  SIMD_INSN ("umaxv",  0x2e30a800, 0xbf3ffc00, asimdall, OP2 (Fd, Vn), QL_XLANES,      F_SIZEQ),
  SIMD_INSN ("uminv",  0x2e31a800, 0xbf3ffc00, asimdall, OP2 (Fd, Vn), QL_XLANES,      F_SIZEQ),
  SIMD_INSN ("fmaxnmv",0x2e30c800, 0xbfbffc00, asimdall, OP2 (Fd, Vn), QL_XLANES_FP,   F_SIZEQ),
  SF16_INSN ("fmaxnmv",0x0e30c800, 0xbffffc00, asimdall, OP2 (Fd, Vn), QL_XLANES_FP_H, F_SIZEQ),
  SIMD_INSN ("fmaxv",  0x2e30f800, 0xbfbffc00, asimdall, OP2 (Fd, Vn), QL_XLANES_FP,   F_SIZEQ),
  SF16_INSN ("fmaxv",  0x0e30f800, 0xbffffc00, asimdall, OP2 (Fd, Vn), QL_XLANES_FP_H, F_SIZEQ),
  SIMD_INSN ("fminnmv",0x2eb0c800, 0xbfbffc00, asimdall, OP2 (Fd, Vn), QL_XLANES_FP,   F_SIZEQ),
  SF16_INSN ("fminnmv",0x0eb0c800, 0xbffffc00, asimdall, OP2 (Fd, Vn), QL_XLANES_FP_H, F_SIZEQ),
  SIMD_INSN ("fminv",  0x2eb0f800, 0xbfbffc00, asimdall, OP2 (Fd, Vn), QL_XLANES_FP,   F_SIZEQ),
  SF16_INSN ("fminv",  0x0eb0f800, 0xbffffc00, asimdall, OP2 (Fd, Vn), QL_XLANES_FP_H, F_SIZEQ),
  /* AdvSIMD three different.  */
  SIMD_INSN ("saddl",   0x0e200000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("saddl2",  0x4e200000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("saddw",   0x0e201000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS,  F_SIZEQ),
  SIMD_INSN ("saddw2",  0x4e201000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS2, F_SIZEQ),
  SIMD_INSN ("ssubl",   0x0e202000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("ssubl2",  0x4e202000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("ssubw",   0x0e203000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS,  F_SIZEQ),
  SIMD_INSN ("ssubw2",  0x4e203000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS2, F_SIZEQ),
  SIMD_INSN ("addhn",   0x0e204000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3NARRBHS,  F_SIZEQ),
  SIMD_INSN ("addhn2",  0x4e204000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3NARRBHS2, F_SIZEQ),
  SIMD_INSN ("sabal",   0x0e205000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("sabal2",  0x4e205000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("subhn",   0x0e206000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3NARRBHS,  F_SIZEQ),
  SIMD_INSN ("subhn2",  0x4e206000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3NARRBHS2, F_SIZEQ),
  SIMD_INSN ("sabdl",   0x0e207000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("sabdl2",  0x4e207000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("smlal",   0x0e208000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("smlal2",  0x4e208000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("sqdmlal", 0x0e209000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGHS,   F_SIZEQ),
  SIMD_INSN ("sqdmlal2",0x4e209000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGHS2,  F_SIZEQ),
  SIMD_INSN ("smlsl",   0x0e20a000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("smlsl2",  0x4e20a000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("sqdmlsl", 0x0e20b000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGHS,   F_SIZEQ),
  SIMD_INSN ("sqdmlsl2",0x4e20b000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGHS2,  F_SIZEQ),
  SIMD_INSN ("smull",   0x0e20c000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("smull2",  0x4e20c000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("sqdmull", 0x0e20d000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGHS,   F_SIZEQ),
  SIMD_INSN ("sqdmull2",0x4e20d000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGHS2,  F_SIZEQ),
  SIMD_INSN ("pmull",   0x0e20e000, 0xffe0fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGB,    0),
  CRYP_INSN ("pmull",   0x0ee0e000, 0xffe0fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGD,    0),
  SIMD_INSN ("pmull2",  0x4e20e000, 0xffe0fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGB2,   0),
  CRYP_INSN ("pmull2",  0x4ee0e000, 0xffe0fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGD2,   0),
  SIMD_INSN ("uaddl",   0x2e200000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("uaddl2",  0x6e200000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("uaddw",   0x2e201000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS,  F_SIZEQ),
  SIMD_INSN ("uaddw2",  0x6e201000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS2, F_SIZEQ),
  SIMD_INSN ("usubl",   0x2e202000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("usubl2",  0x6e202000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("usubw",   0x2e203000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS,  F_SIZEQ),
  SIMD_INSN ("usubw2",  0x6e203000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3WIDEBHS2, F_SIZEQ),
  SIMD_INSN ("raddhn",  0x2e204000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3NARRBHS,  F_SIZEQ),
  SIMD_INSN ("raddhn2", 0x6e204000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3NARRBHS2, F_SIZEQ),
  SIMD_INSN ("uabal",   0x2e205000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("uabal2",  0x6e205000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("rsubhn",  0x2e206000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3NARRBHS,  F_SIZEQ),
  SIMD_INSN ("rsubhn2", 0x6e206000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3NARRBHS2, F_SIZEQ),
  SIMD_INSN ("uabdl",   0x2e207000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("uabdl2",  0x6e207000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("umlal",   0x2e208000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("umlal2",  0x6e208000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("umlsl",   0x2e20a000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("umlsl2",  0x6e20a000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  SIMD_INSN ("umull",   0x2e20c000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS,  F_SIZEQ),
  SIMD_INSN ("umull2",  0x6e20c000, 0xff20fc00, asimddiff, OP3 (Vd, Vn, Vm), QL_V3LONGBHS2, F_SIZEQ),
  /* AdvSIMD vector x indexed element.  */
  SIMD_INSN ("smlal",   0x0f002000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L,    F_SIZEQ),
  SIMD_INSN ("smlal2",  0x4f002000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L2,   F_SIZEQ),
  SIMD_INSN ("sqdmlal", 0x0f003000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L,    F_SIZEQ),
  SIMD_INSN ("sqdmlal2",0x4f003000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L2,   F_SIZEQ),
  SIMD_INSN ("smlsl",   0x0f006000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L,    F_SIZEQ),
  SIMD_INSN ("smlsl2",  0x4f006000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L2,   F_SIZEQ),
  SIMD_INSN ("sqdmlsl", 0x0f007000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L,    F_SIZEQ),
  SIMD_INSN ("sqdmlsl2",0x4f007000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L2,   F_SIZEQ),
  SIMD_INSN ("mul",     0x0f008000, 0xbf00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT,      F_SIZEQ),
  SIMD_INSN ("smull",   0x0f00a000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L,    F_SIZEQ),
  SIMD_INSN ("smull2",  0x4f00a000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L2,   F_SIZEQ),
  SIMD_INSN ("sqdmull", 0x0f00b000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L,    F_SIZEQ),
  SIMD_INSN ("sqdmull2",0x4f00b000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L2,   F_SIZEQ),
  SIMD_INSN ("sqdmulh", 0x0f00c000, 0xbf00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT,      F_SIZEQ),
  SIMD_INSN ("sqrdmulh",0x0f00d000, 0xbf00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT,      F_SIZEQ),
  SIMD_INSN ("fmla",    0x0f801000, 0xbf80f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_FP,   F_SIZEQ),
  SF16_INSN ("fmla",    0x0f001000, 0xbfe0fc00, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_FP_H, F_SIZEQ),
  SIMD_INSN ("fmls",    0x0f805000, 0xbf80f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_FP,   F_SIZEQ),
  SF16_INSN ("fmls",    0x0f005000, 0xbfe0fc00, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_FP_H, F_SIZEQ),
  SIMD_INSN ("fmul",    0x0f809000, 0xbf80f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_FP,   F_SIZEQ),
  SF16_INSN ("fmul",    0x0f009000, 0xbfe0fc00, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_FP_H, F_SIZEQ),
  SIMD_INSN ("mla",     0x2f000000, 0xbf00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT,      F_SIZEQ),
  SIMD_INSN ("umlal",   0x2f002000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L,    F_SIZEQ),
  SIMD_INSN ("umlal2",  0x6f002000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L2,   F_SIZEQ),
  SIMD_INSN ("mls",     0x2f004000, 0xbf00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT,      F_SIZEQ),
  SIMD_INSN ("umlsl",   0x2f006000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L,    F_SIZEQ),
  SIMD_INSN ("umlsl2",  0x6f006000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L2,   F_SIZEQ),
  SIMD_INSN ("umull",   0x2f00a000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L,    F_SIZEQ),
  SIMD_INSN ("umull2",  0x6f00a000, 0xff00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_L2,   F_SIZEQ),
  SIMD_INSN ("fmulx",   0x2f809000, 0xbf80f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_FP,   F_SIZEQ),
  SF16_INSN ("fmulx",   0x2f009000, 0xbfe0fc00, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT_FP_H, F_SIZEQ),
  RDMA_INSN ("sqrdmlah",0x2f00d000, 0xbf00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT,      F_SIZEQ),
  RDMA_INSN ("sqrdmlsh",0x2f00f000, 0xbf00f400, asimdelem, OP3 (Vd, Vn, Em), QL_ELEMENT,      F_SIZEQ),
  /* AdvSIMD EXT.  */
  SIMD_INSN ("ext",  0x2e000000, 0xbfe08400, asimdext, OP4 (Vd, Vn, Vm, IDX), QL_VEXT, F_SIZEQ),
  /* AdvSIMD modified immediate.  */
  SIMD_INSN ("movi", 0x0f000400, 0xbff89c00, asimdimm, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0W, F_SIZEQ),
  SIMD_INSN ("orr",  0x0f001400, 0xbff89c00, asimdimm, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0W, F_SIZEQ),
  SIMD_INSN ("movi", 0x0f008400, 0xbff8dc00, asimdimm, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0H, F_SIZEQ),
  SIMD_INSN ("orr",  0x0f009400, 0xbff8dc00, asimdimm, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0H, F_SIZEQ),
  SIMD_INSN ("movi", 0x0f00c400, 0xbff8ec00, asimdimm, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S1W, F_SIZEQ),
  SIMD_INSN ("movi", 0x0f00e400, 0xbff8fc00, asimdimm, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_B, F_SIZEQ),
  SIMD_INSN ("fmov", 0x0f00f400, 0xbff8fc00, asimdimm, OP2 (Vd, SIMD_FPIMM), QL_SIMD_IMM_S, F_SIZEQ),
  SF16_INSN ("fmov", 0x0f00fc00, 0xbff8fc00, asimdimm, OP2 (Vd, SIMD_FPIMM), QL_SIMD_IMM_H, F_SIZEQ),
  SIMD_INSN ("mvni", 0x2f000400, 0xbff89c00, asimdimm, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0W, F_SIZEQ),
  SIMD_INSN ("bic",  0x2f001400, 0xbff89c00, asimdimm, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0W, F_SIZEQ),
  SIMD_INSN ("mvni", 0x2f008400, 0xbff8dc00, asimdimm, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0H, F_SIZEQ),
  SIMD_INSN ("bic",  0x2f009400, 0xbff8dc00, asimdimm, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S0H, F_SIZEQ),
  SIMD_INSN ("mvni", 0x2f00c400, 0xbff8ec00, asimdimm, OP2 (Vd, SIMD_IMM_SFT), QL_SIMD_IMM_S1W, F_SIZEQ),
  SIMD_INSN ("movi", 0x2f00e400, 0xfff8fc00, asimdimm, OP2 (Sd, SIMD_IMM), QL_SIMD_IMM_D, F_SIZEQ),
  SIMD_INSN ("movi", 0x6f00e400, 0xfff8fc00, asimdimm, OP2 (Vd, SIMD_IMM), QL_SIMD_IMM_V2D, F_SIZEQ),
  SIMD_INSN ("fmov", 0x6f00f400, 0xfff8fc00, asimdimm, OP2 (Vd, SIMD_FPIMM), QL_SIMD_IMM_V2D, F_SIZEQ),
  /* AdvSIMD copy.  */
  SIMD_INSN ("dup", 0x0e000400, 0xbfe0fc00, asimdins, OP2 (Vd, En), QL_DUP_VX, F_T),
  SIMD_INSN ("dup", 0x0e000c00, 0xbfe0fc00, asimdins, OP2 (Vd, Rn), QL_DUP_VR, F_T),
  SIMD_INSN ("smov",0x0e002c00, 0xbfe0fc00, asimdins, OP2 (Rd, En), QL_SMOV, F_GPRSIZE_IN_Q),
  SIMD_INSN ("umov",0x0e003c00, 0xbfe0fc00, asimdins, OP2 (Rd, En), QL_UMOV, F_HAS_ALIAS | F_GPRSIZE_IN_Q),
  SIMD_INSN ("mov", 0x0e003c00, 0xbfe0fc00, asimdins, OP2 (Rd, En), QL_MOV, F_ALIAS | F_GPRSIZE_IN_Q),
  SIMD_INSN ("ins", 0x4e001c00, 0xffe0fc00, asimdins, OP2 (Ed, Rn), QL_INS_XR, F_HAS_ALIAS),
  SIMD_INSN ("mov", 0x4e001c00, 0xffe0fc00, asimdins, OP2 (Ed, Rn), QL_INS_XR, F_ALIAS),
  SIMD_INSN ("ins", 0x6e000400, 0xffe08400, asimdins, OP2 (Ed, En), QL_S_2SAME, F_HAS_ALIAS),
  SIMD_INSN ("mov", 0x6e000400, 0xffe08400, asimdins, OP2 (Ed, En), QL_S_2SAME, F_ALIAS),
  /* AdvSIMD two-reg misc.  */
  SIMD_INSN ("rev64", 0x0e200800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEBHS, F_SIZEQ),
  SIMD_INSN ("rev16", 0x0e201800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEB, F_SIZEQ),
  SIMD_INSN ("saddlp",0x0e202800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2PAIRWISELONGBHS, F_SIZEQ),
  SIMD_INSN ("suqadd",0x0e203800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ),
  SIMD_INSN ("cls",   0x0e204800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEBHS, F_SIZEQ),
  SIMD_INSN ("cnt",   0x0e205800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEB, F_SIZEQ),
  SIMD_INSN ("sadalp",0x0e206800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2PAIRWISELONGBHS, F_SIZEQ),
  SIMD_INSN ("sqabs", 0x0e207800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ),
  SIMD_INSN ("cmgt",  0x0e208800, 0xbf3ffc00, asimdmisc, OP3 (Vd, Vn, IMM0), QL_V2SAME, F_SIZEQ),
  SIMD_INSN ("cmeq",  0x0e209800, 0xbf3ffc00, asimdmisc, OP3 (Vd, Vn, IMM0), QL_V2SAME, F_SIZEQ),
  SIMD_INSN ("cmlt",  0x0e20a800, 0xbf3ffc00, asimdmisc, OP3 (Vd, Vn, IMM0), QL_V2SAME, F_SIZEQ),
  SIMD_INSN ("abs",   0x0e20b800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ),
  SIMD_INSN ("xtn",   0x0e212800, 0xff3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2NARRBHS, F_SIZEQ),
  SIMD_INSN ("xtn2",  0x4e212800, 0xff3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2NARRBHS2, F_SIZEQ),
  SIMD_INSN ("sqxtn", 0xe214800, 0xff3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2NARRBHS, F_SIZEQ),
  SIMD_INSN ("sqxtn2",0x4e214800, 0xff3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2NARRBHS2, F_SIZEQ),
  {"fcvtn",  0x0e216800, 0xffbffc00, asimdmisc, OP_FCVTN,  SIMD, OP2 (Vd, Vn), QL_V2NARRHS,  F_MISC, NULL},
  {"fcvtn2", 0x4e216800, 0xffbffc00, asimdmisc, OP_FCVTN2, SIMD, OP2 (Vd, Vn), QL_V2NARRHS2, F_MISC, NULL},
  {"fcvtl",  0x0e217800, 0xffbffc00, asimdmisc, OP_FCVTL,  SIMD, OP2 (Vd, Vn), QL_V2LONGHS,  F_MISC, NULL},
  {"fcvtl2", 0x4e217800, 0xffbffc00, asimdmisc, OP_FCVTL2, SIMD, OP2 (Vd, Vn), QL_V2LONGHS2, F_MISC, NULL},
  SIMD_INSN ("frintn", 0x0e218800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("frintn", 0x0e798800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("frintm", 0x0e219800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("frintm", 0x0e799800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcvtns", 0x0e21a800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcvtns", 0x0e79a800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcvtms", 0x0e21b800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcvtms", 0x0e79b800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcvtas", 0x0e21c800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcvtas", 0x0e79c800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("scvtf",  0x0e21d800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("scvtf",  0x0e79d800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcmgt",  0x0ea0c800, 0xbfbffc00, asimdmisc, OP3 (Vd, Vn, FPIMM0), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcmgt",  0x0ef8c800, 0xbffffc00, asimdmisc, OP3 (Vd, Vn, FPIMM0), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcmeq",  0x0ea0d800, 0xbfbffc00, asimdmisc, OP3 (Vd, Vn, FPIMM0), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcmeq",  0x0ef8d800, 0xbffffc00, asimdmisc, OP3 (Vd, Vn, FPIMM0), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcmlt",  0x0ea0e800, 0xbfbffc00, asimdmisc, OP3 (Vd, Vn, FPIMM0), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcmlt",  0x0ef8e800, 0xbffffc00, asimdmisc, OP3 (Vd, Vn, FPIMM0), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fabs",   0x0ea0f800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fabs",   0x0ef8f800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("frintp", 0x0ea18800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("frintp", 0x0ef98800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("frintz", 0x0ea19800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("frintz", 0x0ef99800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcvtps", 0x0ea1a800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcvtps", 0x0ef9a800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcvtzs", 0x0ea1b800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcvtzs", 0x0ef9b800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("urecpe", 0x0ea1c800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMES, F_SIZEQ),
  SIMD_INSN ("frecpe", 0x0ea1d800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("frecpe", 0x0ef9d800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("rev32",  0x2e200800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEBH, F_SIZEQ),
  SIMD_INSN ("uaddlp", 0x2e202800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2PAIRWISELONGBHS, F_SIZEQ),
  SIMD_INSN ("usqadd", 0x2e203800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ),
  SIMD_INSN ("clz",    0x2e204800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEBHS, F_SIZEQ),
  SIMD_INSN ("uadalp", 0x2e206800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2PAIRWISELONGBHS, F_SIZEQ),
  SIMD_INSN ("sqneg",  0x2e207800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ),
  SIMD_INSN ("cmge",   0x2e208800, 0xbf3ffc00, asimdmisc, OP3 (Vd, Vn, IMM0), QL_V2SAME, F_SIZEQ),
  SIMD_INSN ("cmle",   0x2e209800, 0xbf3ffc00, asimdmisc, OP3 (Vd, Vn, IMM0), QL_V2SAME, F_SIZEQ),
  SIMD_INSN ("neg",    0x2e20b800, 0xbf3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAME, F_SIZEQ),
  SIMD_INSN ("sqxtun", 0x2e212800, 0xff3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2NARRBHS, F_SIZEQ),
  SIMD_INSN ("sqxtun2",0x6e212800, 0xff3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2NARRBHS2, F_SIZEQ),
  SIMD_INSN ("shll",   0x2e213800, 0xff3ffc00, asimdmisc, OP3 (Vd, Vn, SHLL_IMM), QL_V2LONGBHS, F_SIZEQ),
  SIMD_INSN ("shll2",  0x6e213800, 0xff3ffc00, asimdmisc, OP3 (Vd, Vn, SHLL_IMM), QL_V2LONGBHS2, F_SIZEQ),
  SIMD_INSN ("uqxtn",  0x2e214800, 0xff3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2NARRBHS, F_SIZEQ),
  SIMD_INSN ("uqxtn2", 0x6e214800, 0xff3ffc00, asimdmisc, OP2 (Vd, Vn), QL_V2NARRBHS2, F_SIZEQ),
  SIMD_INSN ("fcvtxn", 0x2e616800, 0xfffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2NARRS, 0),
  SIMD_INSN ("fcvtxn2",0x6e616800, 0xfffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2NARRS2, 0),
  SIMD_INSN ("frinta", 0x2e218800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("frinta", 0x2e798800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("frintx", 0x2e219800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("frintx", 0x2e799800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcvtnu", 0x2e21a800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcvtnu", 0x2e79a800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcvtmu", 0x2e21b800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcvtmu", 0x2e79b800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcvtau", 0x2e21c800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcvtau", 0x2e79c800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("ucvtf",  0x2e21d800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("ucvtf",  0x2e79d800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("not",    0x2e205800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEB, F_SIZEQ | F_HAS_ALIAS),
  SIMD_INSN ("mvn",    0x2e205800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEB, F_SIZEQ | F_ALIAS),
  SIMD_INSN ("rbit",   0x2e605800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEB, F_SIZEQ),
  SIMD_INSN ("fcmge",  0x2ea0c800, 0xbfbffc00, asimdmisc, OP3 (Vd, Vn, FPIMM0), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcmge",  0x2ef8c800, 0xbffffc00, asimdmisc, OP3 (Vd, Vn, FPIMM0), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcmle",  0x2ea0d800, 0xbfbffc00, asimdmisc, OP3 (Vd, Vn, FPIMM0), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcmle",  0x2ef8d800, 0xbffffc00, asimdmisc, OP3 (Vd, Vn, FPIMM0), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fneg",   0x2ea0f800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fneg",   0x2ef8f800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("frinti", 0x2ea19800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("frinti", 0x2ef99800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcvtpu", 0x2ea1a800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcvtpu", 0x2ef9a800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fcvtzu", 0x2ea1b800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fcvtzu", 0x2ef9b800, 0xbffffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("ursqrte",0x2ea1c800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMES, F_SIZEQ),
  SIMD_INSN ("frsqrte",0x2ea1d800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("frsqrte",0x2ef9d800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  SIMD_INSN ("fsqrt",  0x2ea1f800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMESD, F_SIZEQ),
  SF16_INSN ("fsqrt",  0x2ef9f800, 0xbfbffc00, asimdmisc, OP2 (Vd, Vn), QL_V2SAMEH, F_SIZEQ),
  /* AdvSIMD ZIP/UZP/TRN.  */
  SIMD_INSN ("uzp1", 0xe001800, 0xbf20fc00, asimdperm, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("trn1", 0xe002800, 0xbf20fc00, asimdperm, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("zip1", 0xe003800, 0xbf20fc00, asimdperm, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("uzp2", 0xe005800, 0xbf20fc00, asimdperm, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("trn2", 0xe006800, 0xbf20fc00, asimdperm, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("zip2", 0xe007800, 0xbf20fc00, asimdperm, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  /* AdvSIMD three same.  */
  SIMD_INSN ("shadd", 0xe200400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("sqadd", 0xe200c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("srhadd", 0xe201400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("shsub", 0xe202400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("sqsub", 0xe202c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("cmgt", 0xe203400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("cmge", 0xe203c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("sshl", 0xe204400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("sqshl", 0xe204c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("srshl", 0xe205400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("sqrshl", 0xe205c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("smax", 0xe206400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("smin", 0xe206c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("sabd", 0xe207400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("saba", 0xe207c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("add", 0xe208400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("cmtst", 0xe208c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("mla", 0xe209400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("mul", 0xe209c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("smaxp", 0xe20a400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("sminp", 0xe20ac00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("sqdmulh", 0xe20b400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEHS, F_SIZEQ),
  SIMD_INSN ("addp", 0xe20bc00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("fmaxnm", 0xe20c400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fmaxnm", 0xe400400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fmla", 0xe20cc00, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fmla", 0xe400c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fadd", 0xe20d400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fadd", 0xe401400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fmulx", 0xe20dc00, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fmulx", 0xe401c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fcmeq", 0xe20e400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fcmeq", 0xe402400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fmax", 0xe20f400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fmax", 0xe403400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("frecps", 0xe20fc00, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("frecps", 0xe403c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("and", 0xe201c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ),
  SIMD_INSN ("bic", 0xe601c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ),
  SIMD_INSN ("fminnm", 0xea0c400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fminnm", 0xec00400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fmls", 0xea0cc00, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fmls", 0xec00c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fsub", 0xea0d400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fsub", 0xec01400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fmin", 0xea0f400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fmin", 0xec03400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("frsqrts", 0xea0fc00, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("frsqrts", 0xec03c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("orr", 0xea01c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_HAS_ALIAS | F_SIZEQ),
  {"mov", 0xea01c00, 0xbfe0fc00, asimdsame, OP_MOV_V, SIMD, OP2 (Vd, Vn), QL_V2SAMEB, F_ALIAS | F_CONV, NULL},
  SIMD_INSN ("orn", 0xee01c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ),
  SIMD_INSN ("uhadd", 0x2e200400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("uqadd", 0x2e200c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("urhadd", 0x2e201400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("uhsub", 0x2e202400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("uqsub", 0x2e202c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("cmhi", 0x2e203400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("cmhs", 0x2e203c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("ushl", 0x2e204400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("uqshl", 0x2e204c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("urshl", 0x2e205400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("uqrshl", 0x2e205c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("umax", 0x2e206400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("umin", 0x2e206c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("uabd", 0x2e207400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("uaba", 0x2e207c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("sub", 0x2e208400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("cmeq", 0x2e208c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAME, F_SIZEQ),
  SIMD_INSN ("mls", 0x2e209400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("pmul", 0x2e209c00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ),
  SIMD_INSN ("umaxp", 0x2e20a400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("uminp", 0x2e20ac00, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEBHS, F_SIZEQ),
  SIMD_INSN ("sqrdmulh", 0x2e20b400, 0xbf20fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEHS, F_SIZEQ),
  SIMD_INSN ("fmaxnmp", 0x2e20c400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fmaxnmp", 0x2e400400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("faddp", 0x2e20d400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("faddp", 0x2e401400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fmul", 0x2e20dc00, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fmul", 0x2e401c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fcmge", 0x2e20e400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fcmge", 0x2e402400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("facge", 0x2e20ec00, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("facge", 0x2e402c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fmaxp", 0x2e20f400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fmaxp", 0x2e403400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fdiv", 0x2e20fc00, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fdiv", 0x2e403c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("eor", 0x2e201c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ),
  SIMD_INSN ("bsl", 0x2e601c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ),
  SIMD_INSN ("fminnmp", 0x2ea0c400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fminnmp", 0x2ec00400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fabd", 0x2ea0d400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fabd", 0x2ec01400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fcmgt", 0x2ea0e400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fcmgt", 0x2ec02400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("facgt", 0x2ea0ec00, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("facgt", 0x2ec02c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("fminp", 0x2ea0f400, 0xbfa0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMESD, F_SIZEQ),
  SF16_INSN ("fminp", 0x2ec03400, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEH, F_SIZEQ),
  SIMD_INSN ("bit", 0x2ea01c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ),
  SIMD_INSN ("bif", 0x2ee01c00, 0xbfe0fc00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEB, F_SIZEQ),
  /* AdvSIMD three same extension.  */
  RDMA_INSN ("sqrdmlah",0x2e008400, 0xbf20fe00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEHS, F_SIZEQ),
  RDMA_INSN ("sqrdmlsh",0x2e008c00, 0xbf20fe00, asimdsame, OP3 (Vd, Vn, Vm), QL_V3SAMEHS, F_SIZEQ),
  /* AdvSIMD shift by immediate.  */
  SIMD_INSN ("sshr", 0xf000400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0),
  SIMD_INSN ("ssra", 0xf001400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0),
  SIMD_INSN ("srshr", 0xf002400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0),
  SIMD_INSN ("srsra", 0xf003400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0),
  SIMD_INSN ("shl", 0xf005400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFT, 0),
  SIMD_INSN ("sqshl", 0xf007400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFT, 0),
  SIMD_INSN ("shrn", 0xf008400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0),
  SIMD_INSN ("shrn2", 0x4f008400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0),
  SIMD_INSN ("rshrn", 0xf008c00, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0),
  SIMD_INSN ("rshrn2", 0x4f008c00, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0),
  SIMD_INSN ("sqshrn", 0xf009400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0),
  SIMD_INSN ("sqshrn2", 0x4f009400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0),
  SIMD_INSN ("sqrshrn", 0xf009c00, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0),
  SIMD_INSN ("sqrshrn2", 0x4f009c00, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0),
  SIMD_INSN ("sshll", 0xf00a400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFTL, F_HAS_ALIAS),
  {"sxtl", 0xf00a400, 0xff87fc00, asimdshf, OP_SXTL, SIMD, OP2 (Vd, Vn), QL_V2LONGBHS, F_ALIAS | F_CONV, NULL},
  SIMD_INSN ("sshll2", 0x4f00a400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFTL2, F_HAS_ALIAS),
  {"sxtl2", 0x4f00a400, 0xff87fc00, asimdshf, OP_SXTL2, SIMD, OP2 (Vd, Vn), QL_V2LONGBHS2, F_ALIAS | F_CONV, NULL},
  SIMD_INSN ("scvtf", 0xf00e400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_SD, 0),
  SF16_INSN ("scvtf", 0xf10e400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_H, 0),
  SIMD_INSN ("fcvtzs", 0xf00fc00, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_SD, 0),
  SF16_INSN ("fcvtzs", 0xf10fc00, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_H, 0),
  SIMD_INSN ("ushr", 0x2f000400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0),
  SIMD_INSN ("usra", 0x2f001400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0),
  SIMD_INSN ("urshr", 0x2f002400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0),
  SIMD_INSN ("ursra", 0x2f003400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0),
  SIMD_INSN ("sri", 0x2f004400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT, 0),
  SIMD_INSN ("sli", 0x2f005400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFT, 0),
  SIMD_INSN ("sqshlu", 0x2f006400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFT, 0),
  SIMD_INSN ("uqshl", 0x2f007400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFT, 0),
  SIMD_INSN ("sqshrun", 0x2f008400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0),
  SIMD_INSN ("sqshrun2", 0x6f008400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0),
  SIMD_INSN ("sqrshrun", 0x2f008c00, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0),
  SIMD_INSN ("sqrshrun2", 0x6f008c00, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0),
  SIMD_INSN ("uqshrn", 0x2f009400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0),
  SIMD_INSN ("uqshrn2", 0x6f009400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0),
  SIMD_INSN ("uqrshrn", 0x2f009c00, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN, 0),
  SIMD_INSN ("uqrshrn2", 0x6f009c00, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFTN2, 0),
  SIMD_INSN ("ushll", 0x2f00a400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFTL, F_HAS_ALIAS),
  {"uxtl", 0x2f00a400, 0xff87fc00, asimdshf, OP_UXTL, SIMD, OP2 (Vd, Vn), QL_V2LONGBHS, F_ALIAS | F_CONV, NULL},
  SIMD_INSN ("ushll2", 0x6f00a400, 0xff80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSL), QL_VSHIFTL2, F_HAS_ALIAS),
  {"uxtl2", 0x6f00a400, 0xff87fc00, asimdshf, OP_UXTL2, SIMD, OP2 (Vd, Vn), QL_V2LONGBHS2, F_ALIAS | F_CONV, NULL},
  SIMD_INSN ("ucvtf", 0x2f00e400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_SD, 0),
  SF16_INSN ("ucvtf", 0x2f10e400, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_H, 0),
  SIMD_INSN ("fcvtzu", 0x2f00fc00, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_SD, 0),
  SF16_INSN ("fcvtzu", 0x2f10fc00, 0xbf80fc00, asimdshf, OP3 (Vd, Vn, IMM_VLSR), QL_VSHIFT_H, 0),
  /* AdvSIMD TBL/TBX.  */
  SIMD_INSN ("tbl", 0xe000000, 0xbfe09c00, asimdtbl, OP3 (Vd, LVn, Vm), QL_TABLE, F_SIZEQ),
  SIMD_INSN ("tbx", 0xe001000, 0xbfe09c00, asimdtbl, OP3 (Vd, LVn, Vm), QL_TABLE, F_SIZEQ),
  /* AdvSIMD scalar three different.  */
  SIMD_INSN ("sqdmlal", 0x5e209000, 0xff20fc00, asisddiff, OP3 (Sd, Sn, Sm), QL_SISDL_HS, F_SSIZE),
  SIMD_INSN ("sqdmlsl", 0x5e20b000, 0xff20fc00, asisddiff, OP3 (Sd, Sn, Sm), QL_SISDL_HS, F_SSIZE),
  SIMD_INSN ("sqdmull", 0x5e20d000, 0xff20fc00, asisddiff, OP3 (Sd, Sn, Sm), QL_SISDL_HS, F_SSIZE),
  /* AdvSIMD scalar x indexed element.  */
  SIMD_INSN ("sqdmlal", 0x5f003000, 0xff00f400, asisdelem, OP3 (Sd, Sn, Em), QL_SISDL_HS, F_SSIZE),
  SIMD_INSN ("sqdmlsl", 0x5f007000, 0xff00f400, asisdelem, OP3 (Sd, Sn, Em), QL_SISDL_HS, F_SSIZE),
  SIMD_INSN ("sqdmull", 0x5f00b000, 0xff00f400, asisdelem, OP3 (Sd, Sn, Em), QL_SISDL_HS, F_SSIZE),
  SIMD_INSN ("sqdmulh", 0x5f00c000, 0xff00f400, asisdelem, OP3 (Sd, Sn, Em), QL_SISD_HS, F_SSIZE),
  SIMD_INSN ("sqrdmulh", 0x5f00d000, 0xff00f400, asisdelem, OP3 (Sd, Sn, Em), QL_SISD_HS, F_SSIZE),
  SIMD_INSN ("fmla", 0x5f801000, 0xff80f400, asisdelem, OP3 (Sd, Sn, Em), QL_FP3, F_SSIZE),
  SF16_INSN ("fmla", 0x5f001000, 0xffc0f400, asisdelem, OP3 (Sd, Sn, Em), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("fmls", 0x5f805000, 0xff80f400, asisdelem, OP3 (Sd, Sn, Em), QL_FP3, F_SSIZE),
  SF16_INSN ("fmls", 0x5f005000, 0xffc0f400, asisdelem, OP3 (Sd, Sn, Em), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("fmul", 0x5f809000, 0xff80f400, asisdelem, OP3 (Sd, Sn, Em), QL_FP3, F_SSIZE),
  SF16_INSN ("fmul", 0x5f009000, 0xffc0f400, asisdelem, OP3 (Sd, Sn, Em), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("fmulx", 0x7f809000, 0xff80f400, asisdelem, OP3 (Sd, Sn, Em), QL_FP3, F_SSIZE),
  SF16_INSN ("fmulx", 0x7f009000, 0xffc0f400, asisdelem, OP3 (Sd, Sn, Em), QL_FP3_H, F_SSIZE),
  RDMA_INSN ("sqrdmlah", 0x7f00d000, 0xff00f400, asisdelem, OP3 (Sd, Sn, Em), QL_SISD_HS, F_SSIZE),
  RDMA_INSN ("sqrdmlsh", 0x7f00f000, 0xff00f400, asisdelem, OP3 (Sd, Sn, Em), QL_SISD_HS, F_SSIZE),
  /* AdvSIMD load/store multiple structures.  */
  SIMD_INSN ("st4", 0xc000000, 0xbfff0000, asisdlse, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(4)),
  SIMD_INSN ("st1", 0xc000000, 0xbfff0000, asisdlse, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)),
  SIMD_INSN ("st2", 0xc000000, 0xbfff0000, asisdlse, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(2)),
  SIMD_INSN ("st3", 0xc000000, 0xbfff0000, asisdlse, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(3)),
  SIMD_INSN ("ld4", 0xc400000, 0xbfff0000, asisdlse, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(4)),
  SIMD_INSN ("ld1", 0xc400000, 0xbfff0000, asisdlse, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)),
  SIMD_INSN ("ld2", 0xc400000, 0xbfff0000, asisdlse, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(2)),
  SIMD_INSN ("ld3", 0xc400000, 0xbfff0000, asisdlse, OP2 (LVt, SIMD_ADDR_SIMPLE), QL_SIMD_LDST, F_SIZEQ | F_OD(3)),
  /* AdvSIMD load/store multiple structures (post-indexed).  */
  SIMD_INSN ("st4", 0xc800000, 0xbfe00000, asisdlsep, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(4)),
  SIMD_INSN ("st1", 0xc800000, 0xbfe00000, asisdlsep, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)),
  SIMD_INSN ("st2", 0xc800000, 0xbfe00000, asisdlsep, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(2)),
  SIMD_INSN ("st3", 0xc800000, 0xbfe00000, asisdlsep, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(3)),
  SIMD_INSN ("ld4", 0xcc00000, 0xbfe00000, asisdlsep, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(4)),
  SIMD_INSN ("ld1", 0xcc00000, 0xbfe00000, asisdlsep, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)),
  SIMD_INSN ("ld2", 0xcc00000, 0xbfe00000, asisdlsep, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(2)),
  SIMD_INSN ("ld3", 0xcc00000, 0xbfe00000, asisdlsep, OP2 (LVt, SIMD_ADDR_POST), QL_SIMD_LDST, F_SIZEQ | F_OD(3)),
  /* AdvSIMD load/store single structure.  */
  SIMD_INSN ("st1", 0xd000000, 0xbfff2000, asisdlso, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(1)),
  SIMD_INSN ("st3", 0xd002000, 0xbfff2000, asisdlso, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(3)),
  SIMD_INSN ("st2", 0xd200000, 0xbfff2000, asisdlso, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(2)),
  SIMD_INSN ("st4", 0xd202000, 0xbfff2000, asisdlso, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(4)),
  SIMD_INSN ("ld1", 0xd400000, 0xbfff2000, asisdlso, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(1)),
  SIMD_INSN ("ld3", 0xd402000, 0xbfff2000, asisdlso, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(3)),
  SIMD_INSN ("ld1r", 0xd40c000, 0xbfffe000, asisdlso, OP2 (LVt_AL, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)),
  SIMD_INSN ("ld3r", 0xd40e000, 0xbfffe000, asisdlso, OP2 (LVt_AL, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(3)),
  SIMD_INSN ("ld2", 0xd600000, 0xbfff2000, asisdlso, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(2)),
  SIMD_INSN ("ld4", 0xd602000, 0xbfff2000, asisdlso, OP2 (LEt, SIMD_ADDR_SIMPLE), QL_SIMD_LDSTONE, F_OD(4)),
  SIMD_INSN ("ld2r", 0xd60c000, 0xbfffe000, asisdlso, OP2 (LVt_AL, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(2)),
  SIMD_INSN ("ld4r", 0xd60e000, 0xbfffe000, asisdlso, OP2 (LVt_AL, SIMD_ADDR_SIMPLE), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(4)),
  /* AdvSIMD load/store single structure (post-indexed).  */
  SIMD_INSN ("st1", 0xd800000, 0xbfe02000, asisdlsop, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(1)),
  SIMD_INSN ("st3", 0xd802000, 0xbfe02000, asisdlsop, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(3)),
  SIMD_INSN ("st2", 0xda00000, 0xbfe02000, asisdlsop, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(2)),
  SIMD_INSN ("st4", 0xda02000, 0xbfe02000, asisdlsop, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(4)),
  SIMD_INSN ("ld1", 0xdc00000, 0xbfe02000, asisdlsop, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(1)),
  SIMD_INSN ("ld3", 0xdc02000, 0xbfe02000, asisdlsop, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(3)),
  SIMD_INSN ("ld1r", 0xdc0c000, 0xbfe0e000, asisdlsop, OP2 (LVt_AL, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(1)),
  SIMD_INSN ("ld3r", 0xdc0e000, 0xbfe0e000, asisdlsop, OP2 (LVt_AL, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(3)),
  SIMD_INSN ("ld2", 0xde00000, 0xbfe02000, asisdlsop, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(2)),
  SIMD_INSN ("ld4", 0xde02000, 0xbfe02000, asisdlsop, OP2 (LEt, SIMD_ADDR_POST), QL_SIMD_LDSTONE, F_OD(4)),
  SIMD_INSN ("ld2r", 0xde0c000, 0xbfe0e000, asisdlsop, OP2 (LVt_AL, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(2)),
  SIMD_INSN ("ld4r", 0xde0e000, 0xbfe0e000, asisdlsop, OP2 (LVt_AL, SIMD_ADDR_POST), QL_SIMD_LDST_ANY, F_SIZEQ | F_OD(4)),
  /* AdvSIMD scalar two-reg misc.  */
  SIMD_INSN ("suqadd", 0x5e203800, 0xff3ffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAME, F_SSIZE),
  SIMD_INSN ("sqabs", 0x5e207800, 0xff3ffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAME, F_SSIZE),
  SIMD_INSN ("cmgt", 0x5e208800, 0xff3ffc00, asisdmisc, OP3 (Sd, Sn, IMM0), QL_SISD_CMP_0, F_SSIZE),
  SIMD_INSN ("cmeq", 0x5e209800, 0xff3ffc00, asisdmisc, OP3 (Sd, Sn, IMM0), QL_SISD_CMP_0, F_SSIZE),
  SIMD_INSN ("cmlt", 0x5e20a800, 0xff3ffc00, asisdmisc, OP3 (Sd, Sn, IMM0), QL_SISD_CMP_0, F_SSIZE),
  SIMD_INSN ("abs", 0x5e20b800, 0xff3ffc00, asisdmisc, OP2 (Sd, Sn), QL_2SAMED, F_SSIZE),
  SIMD_INSN ("sqxtn", 0x5e214800, 0xff3ffc00, asisdmisc, OP2 (Sd, Sn), QL_SISD_NARROW, F_SSIZE),
  SIMD_INSN ("fcvtns", 0x5e21a800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("fcvtns", 0x5e79a800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("fcvtms", 0x5e21b800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("fcvtms", 0x5e79b800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("fcvtas", 0x5e21c800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("fcvtas", 0x5e79c800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("scvtf", 0x5e21d800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("scvtf", 0x5e79d800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("fcmgt", 0x5ea0c800, 0xffbffc00, asisdmisc, OP3 (Sd, Sn, FPIMM0), QL_SISD_FCMP_0, F_SSIZE),
  SF16_INSN ("fcmgt", 0x5ef8c800, 0xfffffc00, asisdmisc, OP3 (Sd, Sn, FPIMM0), QL_SISD_FCMP_H_0, F_SSIZE),
  SIMD_INSN ("fcmeq", 0x5ea0d800, 0xffbffc00, asisdmisc, OP3 (Sd, Sn, FPIMM0), QL_SISD_FCMP_0, F_SSIZE),
  SF16_INSN ("fcmeq", 0x5ef8d800, 0xfffffc00, asisdmisc, OP3 (Sd, Sn, FPIMM0), QL_SISD_FCMP_H_0, F_SSIZE),
  SIMD_INSN ("fcmlt", 0x5ea0e800, 0xffbffc00, asisdmisc, OP3 (Sd, Sn, FPIMM0), QL_SISD_FCMP_0, F_SSIZE),
  SF16_INSN ("fcmlt", 0x5ef8e800, 0xfffffc00, asisdmisc, OP3 (Sd, Sn, FPIMM0), QL_SISD_FCMP_H_0, F_SSIZE),
  SIMD_INSN ("fcvtps", 0x5ea1a800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("fcvtps", 0x5ef9a800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("fcvtzs", 0x5ea1b800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("fcvtzs", 0x5ef9b800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("frecpe", 0x5ea1d800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("frecpe", 0x5ef9d800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("frecpx", 0x5ea1f800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("frecpx", 0x5ef9f800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("usqadd", 0x7e203800, 0xff3ffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAME, F_SSIZE),
  SIMD_INSN ("sqneg", 0x7e207800, 0xff3ffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAME, F_SSIZE),
  SIMD_INSN ("cmge", 0x7e208800, 0xff3ffc00, asisdmisc, OP3 (Sd, Sn, IMM0), QL_SISD_CMP_0, F_SSIZE),
  SIMD_INSN ("cmle", 0x7e209800, 0xff3ffc00, asisdmisc, OP3 (Sd, Sn, IMM0), QL_SISD_CMP_0, F_SSIZE),
  SIMD_INSN ("neg", 0x7e20b800, 0xff3ffc00, asisdmisc, OP2 (Sd, Sn), QL_2SAMED, F_SSIZE),
  SIMD_INSN ("sqxtun", 0x7e212800, 0xff3ffc00, asisdmisc, OP2 (Sd, Sn), QL_SISD_NARROW, F_SSIZE),
  SIMD_INSN ("uqxtn", 0x7e214800, 0xff3ffc00, asisdmisc, OP2 (Sd, Sn), QL_SISD_NARROW, F_SSIZE),
  {"fcvtxn", 0x7e216800, 0xffbffc00, asisdmisc, OP_FCVTXN_S, SIMD, OP2 (Sd, Sn), QL_SISD_NARROW_S, F_MISC, NULL},
  SIMD_INSN ("fcvtnu", 0x7e21a800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("fcvtnu", 0x7e79a800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("fcvtmu", 0x7e21b800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("fcvtmu", 0x7e79b800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("fcvtau", 0x7e21c800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("fcvtau", 0x7e79c800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("ucvtf", 0x7e21d800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("ucvtf", 0x7e79d800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("fcmge", 0x7ea0c800, 0xffbffc00, asisdmisc, OP3 (Sd, Sn, FPIMM0), QL_SISD_FCMP_0, F_SSIZE),
  SF16_INSN ("fcmge", 0x7ef8c800, 0xfffffc00, asisdmisc, OP3 (Sd, Sn, FPIMM0), QL_SISD_FCMP_H_0, F_SSIZE),
  SIMD_INSN ("fcmle", 0x7ea0d800, 0xffbffc00, asisdmisc, OP3 (Sd, Sn, FPIMM0), QL_SISD_FCMP_0, F_SSIZE),
  SF16_INSN ("fcmle", 0x7ef8d800, 0xfffffc00, asisdmisc, OP3 (Sd, Sn, FPIMM0), QL_SISD_FCMP_H_0, F_SSIZE),
  SIMD_INSN ("fcvtpu", 0x7ea1a800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("fcvtpu", 0x7ef9a800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_SISD_FCMP_H_0, F_SSIZE),
  SIMD_INSN ("fcvtzu", 0x7ea1b800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("fcvtzu", 0x7ef9b800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  SIMD_INSN ("frsqrte", 0x7ea1d800, 0xffbffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMESD, F_SSIZE),
  SF16_INSN ("frsqrte", 0x7ef9d800, 0xfffffc00, asisdmisc, OP2 (Sd, Sn), QL_S_2SAMEH, F_SSIZE),
  /* AdvSIMD scalar copy.  */
  SIMD_INSN ("dup", 0x5e000400, 0xffe0fc00, asisdone, OP2 (Sd, En), QL_S_2SAME, F_HAS_ALIAS),
  SIMD_INSN ("mov", 0x5e000400, 0xffe0fc00, asisdone, OP2 (Sd, En), QL_S_2SAME, F_ALIAS),
  /* AdvSIMD scalar pairwise.  */
  SIMD_INSN ("addp", 0x5e31b800, 0xff3ffc00, asisdpair, OP2 (Sd, Vn), QL_SISD_PAIR_D, F_SIZEQ),
  SIMD_INSN ("fmaxnmp", 0x7e30c800, 0xffbffc00, asisdpair, OP2 (Sd, Vn), QL_SISD_PAIR, F_SIZEQ),
  SF16_INSN ("fmaxnmp", 0x5e30c800, 0xfffffc00, asisdpair, OP2 (Sd, Vn), QL_SISD_PAIR_H, F_SIZEQ),
  SIMD_INSN ("faddp", 0x7e30d800, 0xffbffc00, asisdpair, OP2 (Sd, Vn), QL_SISD_PAIR, F_SIZEQ),
  SF16_INSN ("faddp", 0x5e30d800, 0xfffffc00, asisdpair, OP2 (Sd, Vn), QL_SISD_PAIR_H, F_SIZEQ),
  SIMD_INSN ("fmaxp", 0x7e30f800, 0xffbffc00, asisdpair, OP2 (Sd, Vn), QL_SISD_PAIR, F_SIZEQ),
  SF16_INSN ("fmaxp", 0x5e30f800, 0xfffffc00, asisdpair, OP2 (Sd, Vn), QL_SISD_PAIR_H, F_SIZEQ),
  SIMD_INSN ("fminnmp", 0x7eb0c800, 0xffbffc00, asisdpair, OP2 (Sd, Vn), QL_SISD_PAIR, F_SIZEQ),
  SF16_INSN ("fminnmp", 0x5eb0c800, 0xfffffc00, asisdpair, OP2 (Sd, Vn), QL_SISD_PAIR_H, F_SIZEQ),
  SIMD_INSN ("fminp", 0x7eb0f800, 0xffbffc00, asisdpair, OP2 (Sd, Vn), QL_SISD_PAIR, F_SIZEQ),
  SF16_INSN ("fminp", 0x5eb0f800, 0xfffffc00, asisdpair, OP2 (Sd, Vn), QL_SISD_PAIR_H, F_SIZEQ),
  /* AdvSIMD scalar three same.  */
  SIMD_INSN ("sqadd", 0x5e200c00, 0xff20fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE),
  SIMD_INSN ("sqsub", 0x5e202c00, 0xff20fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE),
  SIMD_INSN ("sqshl", 0x5e204c00, 0xff20fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE),
  SIMD_INSN ("sqrshl", 0x5e205c00, 0xff20fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE),
  SIMD_INSN ("sqdmulh", 0x5e20b400, 0xff20fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_SISD_HS, F_SSIZE),
  SIMD_INSN ("fmulx", 0x5e20dc00, 0xffa0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE),
  SF16_INSN ("fmulx", 0x5e401c00, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("fcmeq", 0x5e20e400, 0xffa0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE),
  SF16_INSN ("fcmeq", 0x5e402400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("frecps", 0x5e20fc00, 0xffa0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE),
  SF16_INSN ("frecps", 0x5e403c00, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("frsqrts", 0x5ea0fc00, 0xffa0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE),
  SF16_INSN ("frsqrts", 0x5ec03c00, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("cmgt", 0x5ee03400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  SIMD_INSN ("cmge", 0x5ee03c00, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  SIMD_INSN ("sshl", 0x5ee04400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  SIMD_INSN ("srshl", 0x5ee05400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  SIMD_INSN ("add", 0x5ee08400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  SIMD_INSN ("cmtst", 0x5ee08c00, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  SIMD_INSN ("uqadd", 0x7e200c00, 0xff20fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE),
  SIMD_INSN ("uqsub", 0x7e202c00, 0xff20fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE),
  SIMD_INSN ("uqshl", 0x7e204c00, 0xff20fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE),
  SIMD_INSN ("uqrshl", 0x7e205c00, 0xff20fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAME, F_SSIZE),
  SIMD_INSN ("sqrdmulh", 0x7e20b400, 0xff20fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_SISD_HS, F_SSIZE),
  SIMD_INSN ("fcmge", 0x7e20e400, 0xffa0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE),
  SF16_INSN ("fcmge", 0x7e402400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("facge", 0x7e20ec00, 0xffa0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE),
  SF16_INSN ("facge", 0x7e402c00, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("fabd", 0x7ea0d400, 0xffa0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE),
  SF16_INSN ("fabd", 0x7ec01400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("fcmgt", 0x7ea0e400, 0xffa0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE),
  SF16_INSN ("fcmgt", 0x7ec02400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("facgt", 0x7ea0ec00, 0xffa0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3, F_SSIZE),
  SF16_INSN ("facgt", 0x7ec02c00, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_FP3_H, F_SSIZE),
  SIMD_INSN ("cmhi", 0x7ee03400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  SIMD_INSN ("cmhs", 0x7ee03c00, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  SIMD_INSN ("ushl", 0x7ee04400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  SIMD_INSN ("urshl", 0x7ee05400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  SIMD_INSN ("sub", 0x7ee08400, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  SIMD_INSN ("cmeq", 0x7ee08c00, 0xffe0fc00, asisdsame, OP3 (Sd, Sn, Sm), QL_S_3SAMED, F_SSIZE),
  /* AdvSIMDs scalar three same extension.  */
  RDMA_INSN ("sqrdmlah", 0x7e008400, 0xff20fc00, asimdsame, OP3 (Sd, Sn, Sm), QL_SISD_HS, F_SSIZE),
  RDMA_INSN ("sqrdmlsh", 0x7e008c00, 0xff20fc00, asimdsame, OP3 (Sd, Sn, Sm), QL_SISD_HS, F_SSIZE),
  /* AdvSIMD scalar shift by immediate.  */
  SIMD_INSN ("sshr", 0x5f000400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0),
  SIMD_INSN ("ssra", 0x5f001400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0),
  SIMD_INSN ("srshr", 0x5f002400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0),
  SIMD_INSN ("srsra", 0x5f003400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0),
  SIMD_INSN ("shl", 0x5f005400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSL), QL_SSHIFT_D, 0),
  SIMD_INSN ("sqshl", 0x5f007400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSL), QL_SSHIFT, 0),
  SIMD_INSN ("sqshrn", 0x5f009400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0),
  SIMD_INSN ("sqrshrn", 0x5f009c00, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0),
  SIMD_INSN ("scvtf", 0x5f00e400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_SD, 0),
  SF16_INSN ("scvtf", 0x5f10e400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_H, 0),
  SIMD_INSN ("fcvtzs", 0x5f00fc00, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_SD, 0),
  SF16_INSN ("fcvtzs", 0x5f10fc00, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_H, 0),
  SIMD_INSN ("ushr", 0x7f000400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0),
  SIMD_INSN ("usra", 0x7f001400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0),
  SIMD_INSN ("urshr", 0x7f002400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0),
  SIMD_INSN ("ursra", 0x7f003400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0),
  SIMD_INSN ("sri", 0x7f004400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_D, 0),
  SIMD_INSN ("sli", 0x7f005400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSL), QL_SSHIFT_D, 0),
  SIMD_INSN ("sqshlu", 0x7f006400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSL), QL_SSHIFT, 0),
  SIMD_INSN ("uqshl", 0x7f007400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSL), QL_SSHIFT, 0),
  SIMD_INSN ("sqshrun", 0x7f008400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0),
  SIMD_INSN ("sqrshrun", 0x7f008c00, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0),
  SIMD_INSN ("uqshrn", 0x7f009400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0),
  SIMD_INSN ("uqrshrn", 0x7f009c00, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFTN, 0),
  SIMD_INSN ("ucvtf", 0x7f00e400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_SD, 0),
  SF16_INSN ("ucvtf", 0x7f10e400, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_H, 0),
  SIMD_INSN ("fcvtzu", 0x7f00fc00, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_SD, 0),
  SF16_INSN ("fcvtzu", 0x7f10fc00, 0xff80fc00, asisdshf, OP3 (Sd, Sn, IMM_VLSR), QL_SSHIFT_H, 0),
  /* Bitfield.  */
  CORE_INSN ("sbfm", 0x13000000, 0x7f800000, bitfield, OP4 (Rd, Rn, IMMR, IMMS), QL_BF, F_HAS_ALIAS | F_SF | F_N),
  {"sbfiz", 0x13000000, 0x7f800000, bitfield, OP_SBFIZ, CORE, OP4 (Rd, Rn, IMM, WIDTH), QL_BF2, F_ALIAS | F_P1 | F_CONV, NULL},
  {"sbfx",  0x13000000, 0x7f800000, bitfield, OP_SBFX,  CORE, OP4 (Rd, Rn, IMM, WIDTH), QL_BF2, F_ALIAS | F_P1 | F_CONV, NULL},
  CORE_INSN ("sxtb", 0x13001c00, 0x7fbffc00, bitfield, OP2 (Rd, Rn), QL_EXT, F_ALIAS | F_P3 | F_SF | F_N),
  CORE_INSN ("sxth", 0x13003c00, 0x7fbffc00, bitfield, OP2 (Rd, Rn), QL_EXT, F_ALIAS | F_P3 | F_SF | F_N),
  CORE_INSN ("sxtw", 0x93407c00, 0xfffffc00, bitfield, OP2 (Rd, Rn), QL_EXT_W, F_ALIAS | F_P3),
  {"asr", 0x13000000, 0x7f800000, bitfield, OP_ASR_IMM, CORE, OP3 (Rd, Rn, IMM), QL_SHIFT, F_ALIAS | F_P2 | F_CONV, NULL},
  CORE_INSN ("bfm", 0x33000000, 0x7f800000, bitfield, OP4 (Rd, Rn, IMMR, IMMS), QL_BF, F_HAS_ALIAS | F_SF | F_N),
  {"bfi", 0x33000000, 0x7f800000, bitfield, OP_BFI, CORE, OP4 (Rd, Rn, IMM, WIDTH), QL_BF2, F_ALIAS | F_P1 | F_CONV, NULL},
  {"bfc", 0x330003e0, 0x7f8003e0, bitfield, OP_BFC, ARMV8_2, OP3 (Rd, IMM, WIDTH), QL_BF1, F_ALIAS | F_P2 | F_CONV, NULL},
  {"bfxil", 0x33000000, 0x7f800000, bitfield, OP_BFXIL, CORE, OP4 (Rd, Rn, IMM, WIDTH), QL_BF2, F_ALIAS | F_P1 | F_CONV, NULL},
  CORE_INSN ("ubfm", 0x53000000, 0x7f800000, bitfield, OP4 (Rd, Rn, IMMR, IMMS), QL_BF, F_HAS_ALIAS | F_SF | F_N),
  {"ubfiz", 0x53000000, 0x7f800000, bitfield, OP_UBFIZ, CORE, OP4 (Rd, Rn, IMM, WIDTH), QL_BF2, F_ALIAS | F_P1 | F_CONV, NULL},
  {"ubfx", 0x53000000, 0x7f800000, bitfield, OP_UBFX, CORE, OP4 (Rd, Rn, IMM, WIDTH), QL_BF2, F_ALIAS | F_P1 | F_CONV, NULL},
  {"uxtb", 0x53001c00, 0xfffffc00, bitfield, OP_UXTB, CORE, OP2 (Rd, Rn), QL_I2SAMEW, F_ALIAS | F_P3, NULL},
  {"uxth", 0x53003c00, 0xfffffc00, bitfield, OP_UXTH, CORE, OP2 (Rd, Rn), QL_I2SAMEW, F_ALIAS | F_P3, NULL},
  {"lsl", 0x53000000, 0x7f800000, bitfield, OP_LSL_IMM, CORE, OP3 (Rd, Rn, IMM), QL_SHIFT, F_ALIAS | F_P2 | F_CONV, NULL},
  {"lsr", 0x53000000, 0x7f800000, bitfield, OP_LSR_IMM, CORE, OP3 (Rd, Rn, IMM), QL_SHIFT, F_ALIAS | F_P2 | F_CONV, NULL},
  /* Unconditional branch (immediate).  */
  {"b", 0x14000000, 0xfc000000, branch_imm, OP_B, CORE, OP1 (ADDR_PCREL26), QL_PCREL_26, 0, NULL},
  {"bl", 0x94000000, 0xfc000000, branch_imm, OP_BL, CORE, OP1 (ADDR_PCREL26), QL_PCREL_26, 0, NULL},
  /* Unconditional branch (register).  */
  CORE_INSN ("br", 0xd61f0000, 0xfffffc1f, branch_reg, OP1 (Rn), QL_I1X, 0),
  CORE_INSN ("blr", 0xd63f0000, 0xfffffc1f, branch_reg, OP1 (Rn), QL_I1X, 0),
  CORE_INSN ("ret", 0xd65f0000, 0xfffffc1f, branch_reg, OP1 (Rn), QL_I1X, F_OPD0_OPT | F_DEFAULT (30)),
  CORE_INSN ("eret", 0xd69f03e0, 0xffffffff, branch_reg, OP0 (), {}, 0),
  CORE_INSN ("drps", 0xd6bf03e0, 0xffffffff, branch_reg, OP0 (), {}, 0),
  /* Compare & branch (immediate).  */
  CORE_INSN ("cbz", 0x34000000, 0x7f000000, compbranch, OP2 (Rt, ADDR_PCREL19), QL_R_PCREL, F_SF),
  CORE_INSN ("cbnz", 0x35000000, 0x7f000000, compbranch, OP2 (Rt, ADDR_PCREL19), QL_R_PCREL, F_SF),
  /* Conditional branch (immediate).  */
  CORE_INSN ("b.c", 0x54000000, 0xff000010, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_COND),
  /* Conditional compare (immediate).  */
  CORE_INSN ("ccmn", 0x3a400800, 0x7fe00c10, condcmp_imm, OP4 (Rn, CCMP_IMM, NZCV, COND), QL_CCMP_IMM, F_SF),
  CORE_INSN ("ccmp", 0x7a400800, 0x7fe00c10, condcmp_imm, OP4 (Rn, CCMP_IMM, NZCV, COND), QL_CCMP_IMM, F_SF),
  /* Conditional compare (register).  */
  CORE_INSN ("ccmn", 0x3a400000, 0x7fe00c10, condcmp_reg, OP4 (Rn, Rm, NZCV, COND), QL_CCMP, F_SF),
  CORE_INSN ("ccmp", 0x7a400000, 0x7fe00c10, condcmp_reg, OP4 (Rn, Rm, NZCV, COND), QL_CCMP, F_SF),
  /* Conditional select.  */
  CORE_INSN ("csel", 0x1a800000, 0x7fe00c00, condsel, OP4 (Rd, Rn, Rm, COND), QL_CSEL, F_SF),
  CORE_INSN ("csinc", 0x1a800400, 0x7fe00c00, condsel, OP4 (Rd, Rn, Rm, COND), QL_CSEL, F_HAS_ALIAS | F_SF),
  {"cinc", 0x1a800400, 0x7fe00c00, condsel, OP_CINC, CORE, OP3 (Rd, Rn, COND1), QL_CSEL, F_ALIAS | F_SF | F_CONV, NULL},
  {"cset", 0x1a9f07e0, 0x7fff0fe0, condsel, OP_CSET, CORE, OP2 (Rd, COND1), QL_DST_R, F_ALIAS | F_P1 | F_SF | F_CONV, NULL},
  CORE_INSN ("csinv", 0x5a800000, 0x7fe00c00, condsel, OP4 (Rd, Rn, Rm, COND), QL_CSEL, F_HAS_ALIAS | F_SF),
  {"cinv", 0x5a800000, 0x7fe00c00, condsel, OP_CINV, CORE, OP3 (Rd, Rn, COND1), QL_CSEL, F_ALIAS | F_SF | F_CONV, NULL},
  {"csetm", 0x5a9f03e0, 0x7fff0fe0, condsel, OP_CSETM, CORE, OP2 (Rd, COND1), QL_DST_R, F_ALIAS | F_P1 | F_SF | F_CONV, NULL},
  CORE_INSN ("csneg", 0x5a800400, 0x7fe00c00, condsel, OP4 (Rd, Rn, Rm, COND), QL_CSEL, F_HAS_ALIAS | F_SF),
  {"cneg", 0x5a800400, 0x7fe00c00, condsel, OP_CNEG, CORE, OP3 (Rd, Rn, COND1), QL_CSEL, F_ALIAS | F_SF | F_CONV, NULL},
  /* Crypto AES.  */
  CRYP_INSN ("aese",     0x4e284800, 0xfffffc00, cryptoaes, OP2 (Vd, Vn), QL_V2SAME16B, 0),
  CRYP_INSN ("aesd",     0x4e285800, 0xfffffc00, cryptoaes, OP2 (Vd, Vn), QL_V2SAME16B, 0),
  CRYP_INSN ("aesmc",    0x4e286800, 0xfffffc00, cryptoaes, OP2 (Vd, Vn), QL_V2SAME16B, 0),
  CRYP_INSN ("aesimc",   0x4e287800, 0xfffffc00, cryptoaes, OP2 (Vd, Vn), QL_V2SAME16B, 0),
  /* Crypto two-reg SHA.  */
  CRYP_INSN ("sha1h",    0x5e280800, 0xfffffc00, cryptosha2, OP2 (Fd, Fn), QL_2SAMES, 0),
  CRYP_INSN ("sha1su1",  0x5e281800, 0xfffffc00, cryptosha2, OP2 (Vd, Vn), QL_V2SAME4S, 0),
  CRYP_INSN ("sha256su0",0x5e282800, 0xfffffc00, cryptosha2, OP2 (Vd, Vn), QL_V2SAME4S, 0),
  /* Crypto three-reg SHA.  */
  CRYP_INSN ("sha1c",    0x5e000000, 0xffe0fc00, cryptosha3, OP3 (Fd, Fn, Vm), QL_SHAUPT, 0),
  CRYP_INSN ("sha1p",    0x5e001000, 0xffe0fc00, cryptosha3, OP3 (Fd, Fn, Vm), QL_SHAUPT, 0),
  CRYP_INSN ("sha1m",    0x5e002000, 0xffe0fc00, cryptosha3, OP3 (Fd, Fn, Vm), QL_SHAUPT, 0),
  CRYP_INSN ("sha1su0",  0x5e003000, 0xffe0fc00, cryptosha3, OP3 (Vd, Vn, Vm), QL_V3SAME4S, 0),
  CRYP_INSN ("sha256h",  0x5e004000, 0xffe0fc00, cryptosha3, OP3 (Fd, Fn, Vm), QL_SHA256UPT, 0),
  CRYP_INSN ("sha256h2", 0x5e005000, 0xffe0fc00, cryptosha3, OP3 (Fd, Fn, Vm), QL_SHA256UPT, 0),
  CRYP_INSN ("sha256su1",0x5e006000, 0xffe0fc00, cryptosha3, OP3 (Vd, Vn, Vm), QL_V3SAME4S, 0),
  /* Data-processing (1 source).  */
  CORE_INSN ("rbit",  0x5ac00000, 0x7ffffc00, dp_1src, OP2 (Rd, Rn), QL_I2SAME, F_SF),
  CORE_INSN ("rev16", 0x5ac00400, 0x7ffffc00, dp_1src, OP2 (Rd, Rn), QL_I2SAME, F_SF),
  CORE_INSN ("rev",   0x5ac00800, 0xfffffc00, dp_1src, OP2 (Rd, Rn), QL_I2SAMEW, 0),
  CORE_INSN ("rev",   0xdac00c00, 0xfffffc00, dp_1src, OP2 (Rd, Rn), QL_I2SAMEX, F_SF | F_HAS_ALIAS | F_P1),
  {"rev64", 0xdac00c00, 0xfffffc00, dp_1src, 0, ARMV8_2, OP2 (Rd, Rn), QL_I2SAMEX, F_SF | F_ALIAS, NULL},
  CORE_INSN ("clz",   0x5ac01000, 0x7ffffc00, dp_1src, OP2 (Rd, Rn), QL_I2SAME, F_SF),
  CORE_INSN ("cls",   0x5ac01400, 0x7ffffc00, dp_1src, OP2 (Rd, Rn), QL_I2SAME, F_SF),
  CORE_INSN ("rev32", 0xdac00800, 0xfffffc00, dp_1src, OP2 (Rd, Rn), QL_I2SAMEX, 0),
  /* Data-processing (2 source).  */
  CORE_INSN ("udiv",  0x1ac00800, 0x7fe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF),
  CORE_INSN ("sdiv",  0x1ac00c00, 0x7fe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF),
  CORE_INSN ("lslv",  0x1ac02000, 0x7fe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF | F_HAS_ALIAS),
  CORE_INSN ("lsl",   0x1ac02000, 0x7fe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF | F_ALIAS),
  CORE_INSN ("lsrv",  0x1ac02400, 0x7fe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF | F_HAS_ALIAS),
  CORE_INSN ("lsr",   0x1ac02400, 0x7fe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF | F_ALIAS),
  CORE_INSN ("asrv",  0x1ac02800, 0x7fe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF | F_HAS_ALIAS),
  CORE_INSN ("asr",   0x1ac02800, 0x7fe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF | F_ALIAS),
  CORE_INSN ("rorv",  0x1ac02c00, 0x7fe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF | F_HAS_ALIAS),
  CORE_INSN ("ror",   0x1ac02c00, 0x7fe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMER, F_SF | F_ALIAS),
  /* CRC instructions.  */
  _CRC_INSN ("crc32b", 0x1ac04000, 0xffe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMEW, 0),
  _CRC_INSN ("crc32h", 0x1ac04400, 0xffe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMEW, 0),
  _CRC_INSN ("crc32w", 0x1ac04800, 0xffe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMEW, 0),
  _CRC_INSN ("crc32x", 0x9ac04c00, 0xffe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3WWX,   0),
  _CRC_INSN ("crc32cb",0x1ac05000, 0xffe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMEW, 0),
  _CRC_INSN ("crc32ch",0x1ac05400, 0xffe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMEW, 0),
  _CRC_INSN ("crc32cw",0x1ac05800, 0xffe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3SAMEW, 0),
  _CRC_INSN ("crc32cx",0x9ac05c00, 0xffe0fc00, dp_2src, OP3 (Rd, Rn, Rm), QL_I3WWX,   0),
  /* Data-processing (3 source).  */
  CORE_INSN ("madd",  0x1b000000, 0x7fe08000, dp_3src, OP4 (Rd, Rn, Rm, Ra), QL_I4SAMER, F_HAS_ALIAS | F_SF),
  CORE_INSN ("mul",   0x1b007c00, 0x7fe0fc00, dp_3src, OP3 (Rd, Rn, Rm),     QL_I3SAMER, F_ALIAS | F_SF),
  CORE_INSN ("msub",  0x1b008000, 0x7fe08000, dp_3src, OP4 (Rd, Rn, Rm, Ra), QL_I4SAMER, F_HAS_ALIAS | F_SF),
  CORE_INSN ("mneg",  0x1b00fc00, 0x7fe0fc00, dp_3src, OP3 (Rd, Rn, Rm),     QL_I3SAMER, F_ALIAS | F_SF),
  CORE_INSN ("smaddl",0x9b200000, 0xffe08000, dp_3src, OP4 (Rd, Rn, Rm, Ra), QL_I4SAMEL, F_HAS_ALIAS),
  CORE_INSN ("smull", 0x9b207c00, 0xffe0fc00, dp_3src, OP3 (Rd, Rn, Rm),     QL_I3SAMEL, F_ALIAS),
  CORE_INSN ("smsubl",0x9b208000, 0xffe08000, dp_3src, OP4 (Rd, Rn, Rm, Ra), QL_I4SAMEL, F_HAS_ALIAS),
  CORE_INSN ("smnegl",0x9b20fc00, 0xffe0fc00, dp_3src, OP3 (Rd, Rn, Rm),     QL_I3SAMEL, F_ALIAS),
  CORE_INSN ("smulh", 0x9b407c00, 0xffe08000, dp_3src, OP3 (Rd, Rn, Rm),     QL_I3SAMEX, 0),
  CORE_INSN ("umaddl",0x9ba00000, 0xffe08000, dp_3src, OP4 (Rd, Rn, Rm, Ra), QL_I4SAMEL, F_HAS_ALIAS),
  CORE_INSN ("umull", 0x9ba07c00, 0xffe0fc00, dp_3src, OP3 (Rd, Rn, Rm),     QL_I3SAMEL, F_ALIAS),
  CORE_INSN ("umsubl",0x9ba08000, 0xffe08000, dp_3src, OP4 (Rd, Rn, Rm, Ra), QL_I4SAMEL, F_HAS_ALIAS),
  CORE_INSN ("umnegl",0x9ba0fc00, 0xffe0fc00, dp_3src, OP3 (Rd, Rn, Rm),     QL_I3SAMEL, F_ALIAS),
  CORE_INSN ("umulh", 0x9bc07c00, 0xffe08000, dp_3src, OP3 (Rd, Rn, Rm),     QL_I3SAMEX, 0),
  /* Excep'n generation.  */
  CORE_INSN ("svc",   0xd4000001, 0xffe0001f, exception, OP1 (EXCEPTION), {}, 0),
  CORE_INSN ("hvc",   0xd4000002, 0xffe0001f, exception, OP1 (EXCEPTION), {}, 0),
  CORE_INSN ("smc",   0xd4000003, 0xffe0001f, exception, OP1 (EXCEPTION), {}, 0),
  CORE_INSN ("brk",   0xd4200000, 0xffe0001f, exception, OP1 (EXCEPTION), {}, 0),
  CORE_INSN ("hlt",   0xd4400000, 0xffe0001f, exception, OP1 (EXCEPTION), {}, 0),
  CORE_INSN ("dcps1", 0xd4a00001, 0xffe0001f, exception, OP1 (EXCEPTION), {}, F_OPD0_OPT | F_DEFAULT (0)),
  CORE_INSN ("dcps2", 0xd4a00002, 0xffe0001f, exception, OP1 (EXCEPTION), {}, F_OPD0_OPT | F_DEFAULT (0)),
  CORE_INSN ("dcps3", 0xd4a00003, 0xffe0001f, exception, OP1 (EXCEPTION), {}, F_OPD0_OPT | F_DEFAULT (0)),
  /* Extract.  */
  CORE_INSN ("extr",  0x13800000, 0x7fa00000, extract, OP4 (Rd, Rn, Rm, IMMS), QL_EXTR, F_HAS_ALIAS | F_SF | F_N),
  {"ror", 0x13800000, 0x7fa00000, extract, OP_ROR_IMM, CORE, OP3 (Rd, Rm, IMMS), QL_SHIFT, F_ALIAS | F_CONV, NULL},
  /* Floating-point<->fixed-point conversions.  */
  __FP_INSN ("scvtf", 0x1e020000, 0x7f3f0000, float2fix, OP3 (Fd, Rn, FBITS), QL_FIX2FP, F_FPTYPE | F_SF),
  FF16_INSN ("scvtf", 0x1ec20000, 0x7f3f0000, float2fix, OP3 (Fd, Rn, FBITS), QL_FIX2FP_H, F_FPTYPE | F_SF),
  __FP_INSN ("ucvtf", 0x1e030000, 0x7f3f0000, float2fix, OP3 (Fd, Rn, FBITS), QL_FIX2FP, F_FPTYPE | F_SF),
  FF16_INSN ("ucvtf", 0x1ec30000, 0x7f3f0000, float2fix, OP3 (Fd, Rn, FBITS), QL_FIX2FP_H, F_FPTYPE | F_SF),
  __FP_INSN ("fcvtzs",0x1e180000, 0x7f3f0000, float2fix, OP3 (Rd, Fn, FBITS), QL_FP2FIX, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtzs",0x1ed80000, 0x7f3f0000, float2fix, OP3 (Rd, Fn, FBITS), QL_FP2FIX_H, F_FPTYPE | F_SF),
  __FP_INSN ("fcvtzu",0x1e190000, 0x7f3f0000, float2fix, OP3 (Rd, Fn, FBITS), QL_FP2FIX, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtzu",0x1ed90000, 0x7f3f0000, float2fix, OP3 (Rd, Fn, FBITS), QL_FP2FIX_H, F_FPTYPE | F_SF),
  /* Floating-point<->integer conversions.  */
  __FP_INSN ("fcvtns",0x1e200000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtns",0x1ee00000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT_H, F_FPTYPE | F_SF),
  __FP_INSN ("fcvtnu",0x1e210000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtnu",0x1ee10000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT_H, F_FPTYPE | F_SF),
  __FP_INSN ("scvtf", 0x1e220000, 0x7f3ffc00, float2int, OP2 (Fd, Rn), QL_INT2FP, F_FPTYPE | F_SF),
  FF16_INSN ("scvtf", 0x1ee20000, 0x7f3ffc00, float2int, OP2 (Fd, Rn), QL_INT2FP_H, F_FPTYPE | F_SF),
  __FP_INSN ("ucvtf", 0x1e230000, 0x7f3ffc00, float2int, OP2 (Fd, Rn), QL_INT2FP, F_FPTYPE | F_SF),
  FF16_INSN ("ucvtf", 0x1ee30000, 0x7f3ffc00, float2int, OP2 (Fd, Rn), QL_INT2FP_H, F_FPTYPE | F_SF),
  __FP_INSN ("fcvtas",0x1e240000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtas",0x1ee40000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT_H, F_FPTYPE | F_SF),
  __FP_INSN ("fcvtau",0x1e250000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtau",0x1ee50000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT_H, F_FPTYPE | F_SF),
  __FP_INSN ("fmov",  0x1e260000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF),
  FF16_INSN ("fmov",  0x1ee60000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT_H, F_FPTYPE | F_SF),
  __FP_INSN ("fmov",  0x1e270000, 0x7f3ffc00, float2int, OP2 (Fd, Rn), QL_INT2FP, F_FPTYPE | F_SF),
  FF16_INSN ("fmov",  0x1ee70000, 0x7f3ffc00, float2int, OP2 (Fd, Rn), QL_INT2FP_H, F_FPTYPE | F_SF),
  __FP_INSN ("fcvtps",0x1e280000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtps",0x1ee80000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT_H, F_FPTYPE | F_SF),
  __FP_INSN ("fcvtpu",0x1e290000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtpu",0x1ee90000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT_H, F_FPTYPE | F_SF),
  __FP_INSN ("fcvtms",0x1e300000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtms",0x1ef00000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT_H, F_FPTYPE | F_SF),
  __FP_INSN ("fcvtmu",0x1e310000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtmu",0x1ef10000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT_H, F_FPTYPE | F_SF),
  __FP_INSN ("fcvtzs",0x1e380000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtzs",0x1ef80000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT_H, F_FPTYPE | F_SF),
  __FP_INSN ("fcvtzu",0x1e390000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT, F_FPTYPE | F_SF),
  FF16_INSN ("fcvtzu",0x1ef90000, 0x7f3ffc00, float2int, OP2 (Rd, Fn), QL_FP2INT_H, F_FPTYPE | F_SF),
  __FP_INSN ("fmov",  0x9eae0000, 0xfffffc00, float2int, OP2 (Rd, VnD1), QL_XVD1, 0),
  __FP_INSN ("fmov",  0x9eaf0000, 0xfffffc00, float2int, OP2 (VdD1, Rn), QL_VD1X, 0),
  /* Floating-point conditional compare.  */
  __FP_INSN ("fccmp", 0x1e200400, 0xff200c10, floatccmp, OP4 (Fn, Fm, NZCV, COND), QL_FCCMP, F_FPTYPE),
  FF16_INSN ("fccmp", 0x1ee00400, 0xff200c10, floatccmp, OP4 (Fn, Fm, NZCV, COND), QL_FCCMP_H, F_FPTYPE),
  __FP_INSN ("fccmpe",0x1e200410, 0xff200c10, floatccmp, OP4 (Fn, Fm, NZCV, COND), QL_FCCMP, F_FPTYPE),
  FF16_INSN ("fccmpe",0x1ee00410, 0xff200c10, floatccmp, OP4 (Fn, Fm, NZCV, COND), QL_FCCMP_H, F_FPTYPE),
  /* Floating-point compare.  */
  __FP_INSN ("fcmp",  0x1e202000, 0xff20fc1f, floatcmp, OP2 (Fn, Fm),     QL_FP2,   F_FPTYPE),
  FF16_INSN ("fcmp",  0x1ee02000, 0xff20fc1f, floatcmp, OP2 (Fn, Fm),     QL_FP2_H, F_FPTYPE),
  __FP_INSN ("fcmpe", 0x1e202010, 0xff20fc1f, floatcmp, OP2 (Fn, Fm),     QL_FP2,   F_FPTYPE),
  FF16_INSN ("fcmpe", 0x1ee02010, 0xff20fc1f, floatcmp, OP2 (Fn, Fm),     QL_FP2_H, F_FPTYPE),
  __FP_INSN ("fcmp",  0x1e202008, 0xff20fc1f, floatcmp, OP2 (Fn, FPIMM0), QL_DST_SD,F_FPTYPE),
  FF16_INSN ("fcmp",  0x1ee02008, 0xff20fc1f, floatcmp, OP2 (Fn, FPIMM0), QL_FP2_H, F_FPTYPE),
  __FP_INSN ("fcmpe", 0x1e202018, 0xff20fc1f, floatcmp, OP2 (Fn, FPIMM0), QL_DST_SD,F_FPTYPE),
  FF16_INSN ("fcmpe", 0x1ee02018, 0xff20fc1f, floatcmp, OP2 (Fn, FPIMM0), QL_FP2_H, F_FPTYPE),
  /* Floating-point data-processing (1 source).  */
  __FP_INSN ("fmov",  0x1e204000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2,   F_FPTYPE),
  FF16_INSN ("fmov",  0x1ee04000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2_H, F_FPTYPE),
  __FP_INSN ("fabs",  0x1e20c000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2,   F_FPTYPE),
  FF16_INSN ("fabs",  0x1ee0c000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2_H, F_FPTYPE),
  __FP_INSN ("fneg",  0x1e214000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2,   F_FPTYPE),
  FF16_INSN ("fneg",  0x1ee14000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2_H, F_FPTYPE),
  __FP_INSN ("fsqrt", 0x1e21c000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2,   F_FPTYPE),
  FF16_INSN ("fsqrt", 0x1ee1c000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2_H, F_FPTYPE),
  {"fcvt", 0x1e224000, 0xff3e7c00, floatdp1, OP_FCVT, FP, OP2 (Fd, Fn), QL_FCVT, F_FPTYPE | F_MISC, NULL },
  __FP_INSN ("frintn",0x1e244000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2,   F_FPTYPE),
  FF16_INSN ("frintn",0x1ee44000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2_H, F_FPTYPE),
  __FP_INSN ("frintp",0x1e24c000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2,   F_FPTYPE),
  FF16_INSN ("frintp",0x1ee4c000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2_H, F_FPTYPE),
  __FP_INSN ("frintm",0x1e254000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2,   F_FPTYPE),
  FF16_INSN ("frintm",0x1ee54000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2_H, F_FPTYPE),
  __FP_INSN ("frintz",0x1e25c000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2,   F_FPTYPE),
  FF16_INSN ("frintz",0x1ee5c000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2_H, F_FPTYPE),
  __FP_INSN ("frinta",0x1e264000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2,   F_FPTYPE),
  FF16_INSN ("frinta",0x1ee64000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2_H, F_FPTYPE),
  __FP_INSN ("frintx",0x1e274000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2,   F_FPTYPE),
  FF16_INSN ("frintx",0x1ee74000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2_H, F_FPTYPE),
  __FP_INSN ("frinti",0x1e27c000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2,   F_FPTYPE),
  FF16_INSN ("frinti",0x1ee7c000, 0xff3ffc00, floatdp1, OP2 (Fd, Fn), QL_FP2_H, F_FPTYPE),
  /* Floating-point data-processing (2 source).  */
  __FP_INSN ("fmul",  0x1e200800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3,   F_FPTYPE),
  FF16_INSN ("fmul",  0x1ee00800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3_H, F_FPTYPE),
  __FP_INSN ("fdiv",  0x1e201800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3,   F_FPTYPE),
  FF16_INSN ("fdiv",  0x1ee01800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3_H, F_FPTYPE),
  __FP_INSN ("fadd",  0x1e202800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3,   F_FPTYPE),
  FF16_INSN ("fadd",  0x1ee02800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3_H, F_FPTYPE),
  __FP_INSN ("fsub",  0x1e203800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3,   F_FPTYPE),
  FF16_INSN ("fsub",  0x1ee03800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3_H, F_FPTYPE),
  __FP_INSN ("fmax",  0x1e204800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3,   F_FPTYPE),
  FF16_INSN ("fmax",  0x1ee04800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3_H, F_FPTYPE),
  __FP_INSN ("fmin",  0x1e205800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3,   F_FPTYPE),
  FF16_INSN ("fmin",  0x1ee05800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3_H, F_FPTYPE),
  __FP_INSN ("fmaxnm",0x1e206800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3,   F_FPTYPE),
  FF16_INSN ("fmaxnm",0x1ee06800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3_H, F_FPTYPE),
  __FP_INSN ("fminnm",0x1e207800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3,   F_FPTYPE),
  FF16_INSN ("fminnm",0x1ee07800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3_H, F_FPTYPE),
  __FP_INSN ("fnmul", 0x1e208800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3,   F_FPTYPE),
  FF16_INSN ("fnmul", 0x1ee08800, 0xff20fc00, floatdp2, OP3 (Fd, Fn, Fm), QL_FP3_H, F_FPTYPE),
  /* Floating-point data-processing (3 source).  */
  __FP_INSN ("fmadd", 0x1f000000, 0xff208000, floatdp3, OP4 (Fd, Fn, Fm, Fa), QL_FP4,   F_FPTYPE),
  FF16_INSN ("fmadd", 0x1fc00000, 0xff208000, floatdp3, OP4 (Fd, Fn, Fm, Fa), QL_FP4_H, F_FPTYPE),
  __FP_INSN ("fmsub", 0x1f008000, 0xff208000, floatdp3, OP4 (Fd, Fn, Fm, Fa), QL_FP4,   F_FPTYPE),
  FF16_INSN ("fmsub", 0x1fc08000, 0xff208000, floatdp3, OP4 (Fd, Fn, Fm, Fa), QL_FP4_H, F_FPTYPE),
  __FP_INSN ("fnmadd",0x1f200000, 0xff208000, floatdp3, OP4 (Fd, Fn, Fm, Fa), QL_FP4,   F_FPTYPE),
  FF16_INSN ("fnmadd",0x1fe00000, 0xff208000, floatdp3, OP4 (Fd, Fn, Fm, Fa), QL_FP4_H, F_FPTYPE),
  __FP_INSN ("fnmsub",0x1f208000, 0xff208000, floatdp3, OP4 (Fd, Fn, Fm, Fa), QL_FP4,   F_FPTYPE),
  FF16_INSN ("fnmsub",0x1fe08000, 0xff208000, floatdp3, OP4 (Fd, Fn, Fm, Fa), QL_FP4_H, F_FPTYPE),
  /* Floating-point immediate.  */
  __FP_INSN ("fmov", 0x1e201000, 0xff201fe0, floatimm, OP2 (Fd, FPIMM), QL_DST_SD, F_FPTYPE),
  FF16_INSN ("fmov", 0x1ee01000, 0xff201fe0, floatimm, OP2 (Fd, FPIMM), QL_DST_H, F_FPTYPE),
  /* Floating-point conditional select.  */
  __FP_INSN ("fcsel", 0x1e200c00, 0xff200c00, floatsel, OP4 (Fd, Fn, Fm, COND), QL_FP_COND, F_FPTYPE),
  FF16_INSN ("fcsel", 0x1ee00c00, 0xff200c00, floatsel, OP4 (Fd, Fn, Fm, COND), QL_FP_COND_H, F_FPTYPE),
  /* Load/store register (immediate indexed).  */
  CORE_INSN ("strb", 0x38000400, 0xffe00400, ldst_imm9, OP2 (Rt, ADDR_SIMM9), QL_LDST_W8, 0),
  CORE_INSN ("ldrb", 0x38400400, 0xffe00400, ldst_imm9, OP2 (Rt, ADDR_SIMM9), QL_LDST_W8, 0),
  CORE_INSN ("ldrsb", 0x38800400, 0xffa00400, ldst_imm9, OP2 (Rt, ADDR_SIMM9), QL_LDST_R8, F_LDS_SIZE),
  CORE_INSN ("str", 0x3c000400, 0x3f600400, ldst_imm9, OP2 (Ft, ADDR_SIMM9), QL_LDST_FP, 0),
  CORE_INSN ("ldr", 0x3c400400, 0x3f600400, ldst_imm9, OP2 (Ft, ADDR_SIMM9), QL_LDST_FP, 0),
  CORE_INSN ("strh", 0x78000400, 0xffe00400, ldst_imm9, OP2 (Rt, ADDR_SIMM9), QL_LDST_W16, 0),
  CORE_INSN ("ldrh", 0x78400400, 0xffe00400, ldst_imm9, OP2 (Rt, ADDR_SIMM9), QL_LDST_W16, 0),
  CORE_INSN ("ldrsh", 0x78800400, 0xffa00400, ldst_imm9, OP2 (Rt, ADDR_SIMM9), QL_LDST_R16, F_LDS_SIZE),
  CORE_INSN ("str", 0xb8000400, 0xbfe00400, ldst_imm9, OP2 (Rt, ADDR_SIMM9), QL_LDST_R, F_GPRSIZE_IN_Q),
  CORE_INSN ("ldr", 0xb8400400, 0xbfe00400, ldst_imm9, OP2 (Rt, ADDR_SIMM9), QL_LDST_R, F_GPRSIZE_IN_Q),
  CORE_INSN ("ldrsw", 0xb8800400, 0xffe00400, ldst_imm9, OP2 (Rt, ADDR_SIMM9), QL_LDST_X32, 0),
  /* Load/store register (unsigned immediate).  */
  {"strb", 0x39000000, 0xffc00000, ldst_pos, OP_STRB_POS, CORE, OP2 (Rt, ADDR_UIMM12), QL_LDST_W8, 0, NULL},
  {"ldrb", 0x39400000, 0xffc00000, ldst_pos, OP_LDRB_POS, CORE, OP2 (Rt, ADDR_UIMM12), QL_LDST_W8, 0, NULL},
  {"ldrsb", 0x39800000, 0xff800000, ldst_pos, OP_LDRSB_POS, CORE, OP2 (Rt, ADDR_UIMM12), QL_LDST_R8, F_LDS_SIZE, NULL},
  {"str", 0x3d000000, 0x3f400000, ldst_pos, OP_STRF_POS, CORE, OP2 (Ft, ADDR_UIMM12), QL_LDST_FP, 0, NULL},
  {"ldr", 0x3d400000, 0x3f400000, ldst_pos, OP_LDRF_POS, CORE, OP2 (Ft, ADDR_UIMM12), QL_LDST_FP, 0, NULL},
  {"strh", 0x79000000, 0xffc00000, ldst_pos, OP_STRH_POS, CORE, OP2 (Rt, ADDR_UIMM12), QL_LDST_W16, 0, NULL},
  {"ldrh", 0x79400000, 0xffc00000, ldst_pos, OP_LDRH_POS, CORE, OP2 (Rt, ADDR_UIMM12), QL_LDST_W16, 0, NULL},
  {"ldrsh", 0x79800000, 0xff800000, ldst_pos, OP_LDRSH_POS, CORE, OP2 (Rt, ADDR_UIMM12), QL_LDST_R16, F_LDS_SIZE, NULL},
  {"str", 0xb9000000, 0xbfc00000, ldst_pos, OP_STR_POS, CORE, OP2 (Rt, ADDR_UIMM12), QL_LDST_R, F_GPRSIZE_IN_Q, NULL},
  {"ldr", 0xb9400000, 0xbfc00000, ldst_pos, OP_LDR_POS, CORE, OP2 (Rt, ADDR_UIMM12), QL_LDST_R, F_GPRSIZE_IN_Q, NULL},
  {"ldrsw", 0xb9800000, 0xffc00000, ldst_pos, OP_LDRSW_POS, CORE, OP2 (Rt, ADDR_UIMM12), QL_LDST_X32, 0, NULL},
  {"prfm", 0xf9800000, 0xffc00000, ldst_pos, OP_PRFM_POS, CORE, OP2 (PRFOP, ADDR_UIMM12), QL_LDST_PRFM, 0, NULL},
  /* Load/store register (register offset).  */
  CORE_INSN ("strb", 0x38200800, 0xffe00c00, ldst_regoff, OP2 (Rt, ADDR_REGOFF), QL_LDST_W8, 0),
  CORE_INSN ("ldrb", 0x38600800, 0xffe00c00, ldst_regoff, OP2 (Rt, ADDR_REGOFF), QL_LDST_W8, 0),
  CORE_INSN ("ldrsb", 0x38a00800, 0xffa00c00, ldst_regoff, OP2 (Rt, ADDR_REGOFF), QL_LDST_R8, F_LDS_SIZE),
  CORE_INSN ("str", 0x3c200800, 0x3f600c00, ldst_regoff, OP2 (Ft, ADDR_REGOFF), QL_LDST_FP, 0),
  CORE_INSN ("ldr", 0x3c600800, 0x3f600c00, ldst_regoff, OP2 (Ft, ADDR_REGOFF), QL_LDST_FP, 0),
  CORE_INSN ("strh", 0x78200800, 0xffe00c00, ldst_regoff, OP2 (Rt, ADDR_REGOFF), QL_LDST_W16, 0),
  CORE_INSN ("ldrh", 0x78600800, 0xffe00c00, ldst_regoff, OP2 (Rt, ADDR_REGOFF), QL_LDST_W16, 0),
  CORE_INSN ("ldrsh", 0x78a00800, 0xffa00c00, ldst_regoff, OP2 (Rt, ADDR_REGOFF), QL_LDST_R16, F_LDS_SIZE),
  CORE_INSN ("str", 0xb8200800, 0xbfe00c00, ldst_regoff, OP2 (Rt, ADDR_REGOFF), QL_LDST_R, F_GPRSIZE_IN_Q),
  CORE_INSN ("ldr", 0xb8600800, 0xbfe00c00, ldst_regoff, OP2 (Rt, ADDR_REGOFF), QL_LDST_R, F_GPRSIZE_IN_Q),
  CORE_INSN ("ldrsw", 0xb8a00800, 0xffe00c00, ldst_regoff, OP2 (Rt, ADDR_REGOFF), QL_LDST_X32, 0),
  CORE_INSN ("prfm", 0xf8a00800, 0xffe00c00, ldst_regoff, OP2 (PRFOP, ADDR_REGOFF), QL_LDST_PRFM, 0),
  /* Load/store register (unprivileged).  */
  CORE_INSN ("sttrb", 0x38000800, 0xffe00c00, ldst_unpriv, OP2 (Rt, ADDR_SIMM9), QL_LDST_W8, 0),
  CORE_INSN ("ldtrb", 0x38400800, 0xffe00c00, ldst_unpriv, OP2 (Rt, ADDR_SIMM9), QL_LDST_W8, 0),
  CORE_INSN ("ldtrsb", 0x38800800, 0xffa00c00, ldst_unpriv, OP2 (Rt, ADDR_SIMM9), QL_LDST_R8, F_LDS_SIZE),
  CORE_INSN ("sttrh", 0x78000800, 0xffe00c00, ldst_unpriv, OP2 (Rt, ADDR_SIMM9), QL_LDST_W16, 0),
  CORE_INSN ("ldtrh", 0x78400800, 0xffe00c00, ldst_unpriv, OP2 (Rt, ADDR_SIMM9), QL_LDST_W16, 0),
  CORE_INSN ("ldtrsh", 0x78800800, 0xffa00c00, ldst_unpriv, OP2 (Rt, ADDR_SIMM9), QL_LDST_R16, F_LDS_SIZE),
  CORE_INSN ("sttr", 0xb8000800, 0xbfe00c00, ldst_unpriv, OP2 (Rt, ADDR_SIMM9), QL_LDST_R, F_GPRSIZE_IN_Q),
  CORE_INSN ("ldtr", 0xb8400800, 0xbfe00c00, ldst_unpriv, OP2 (Rt, ADDR_SIMM9), QL_LDST_R, F_GPRSIZE_IN_Q),
  CORE_INSN ("ldtrsw", 0xb8800800, 0xffe00c00, ldst_unpriv, OP2 (Rt, ADDR_SIMM9), QL_LDST_X32, 0),
  /* Load/store register (unscaled immediate).  */
  {"sturb", 0x38000000, 0xffe00c00, ldst_unscaled, OP_STURB, CORE, OP2 (Rt, ADDR_SIMM9), QL_LDST_W8, 0, NULL},
  {"ldurb", 0x38400000, 0xffe00c00, ldst_unscaled, OP_LDURB, CORE, OP2 (Rt, ADDR_SIMM9), QL_LDST_W8, 0, NULL},
  {"ldursb", 0x38800000, 0xffa00c00, ldst_unscaled, OP_LDURSB, CORE, OP2 (Rt, ADDR_SIMM9), QL_LDST_R8, F_LDS_SIZE, NULL},
  {"stur", 0x3c000000, 0x3f600c00, ldst_unscaled, OP_STURV, CORE, OP2 (Ft, ADDR_SIMM9), QL_LDST_FP, 0, NULL},
  {"ldur", 0x3c400000, 0x3f600c00, ldst_unscaled, OP_LDURV, CORE, OP2 (Ft, ADDR_SIMM9), QL_LDST_FP, 0, NULL},
  {"sturh", 0x78000000, 0xffe00c00, ldst_unscaled, OP_STURH, CORE, OP2 (Rt, ADDR_SIMM9), QL_LDST_W16, 0, NULL},
  {"ldurh", 0x78400000, 0xffe00c00, ldst_unscaled, OP_LDURH, CORE, OP2 (Rt, ADDR_SIMM9), QL_LDST_W16, 0, NULL},
  {"ldursh", 0x78800000, 0xffa00c00, ldst_unscaled, OP_LDURSH, CORE, OP2 (Rt, ADDR_SIMM9), QL_LDST_R16, F_LDS_SIZE, NULL},
  {"stur", 0xb8000000, 0xbfe00c00, ldst_unscaled, OP_STUR, CORE, OP2 (Rt, ADDR_SIMM9), QL_LDST_R, F_GPRSIZE_IN_Q, NULL},
  {"ldur", 0xb8400000, 0xbfe00c00, ldst_unscaled, OP_LDUR, CORE, OP2 (Rt, ADDR_SIMM9), QL_LDST_R, F_GPRSIZE_IN_Q, NULL},
  {"ldursw", 0xb8800000, 0xffe00c00, ldst_unscaled, OP_LDURSW, CORE, OP2 (Rt, ADDR_SIMM9), QL_LDST_X32, 0, NULL},
  {"prfum", 0xf8800000, 0xffe00c00, ldst_unscaled, OP_PRFUM, CORE, OP2 (PRFOP, ADDR_SIMM9), QL_LDST_PRFM, 0, NULL},
  /* Load/store exclusive.  */
  CORE_INSN ("stxrb", 0x8007c00, 0xffe08000, ldstexcl, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  CORE_INSN ("stlxrb", 0x800fc00, 0xffe08000, ldstexcl, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  CORE_INSN ("ldxrb", 0x85f7c00, 0xffe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  CORE_INSN ("ldaxrb", 0x85ffc00, 0xffe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  CORE_INSN ("stlrb", 0x89ffc00, 0xffe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  CORE_INSN ("ldarb", 0x8dffc00, 0xffeffc00, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  CORE_INSN ("stxrh", 0x48007c00, 0xffe08000, ldstexcl, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  CORE_INSN ("stlxrh", 0x4800fc00, 0xffe08000, ldstexcl, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  CORE_INSN ("ldxrh", 0x485f7c00, 0xffe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  CORE_INSN ("ldaxrh", 0x485ffc00, 0xffe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  CORE_INSN ("stlrh", 0x489ffc00, 0xffe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  CORE_INSN ("ldarh", 0x48dffc00, 0xffeffc00, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  CORE_INSN ("stxr", 0x88007c00, 0xbfe08000, ldstexcl, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2_LDST_EXC, F_GPRSIZE_IN_Q),
  CORE_INSN ("stlxr", 0x8800fc00, 0xbfe08000, ldstexcl, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2_LDST_EXC, F_GPRSIZE_IN_Q),
  CORE_INSN ("stxp", 0x88200000, 0xbfe08000, ldstexcl, OP4 (Rs, Rt, Rt2, ADDR_SIMPLE), QL_R3_LDST_EXC, F_GPRSIZE_IN_Q),
  CORE_INSN ("stlxp", 0x88208000, 0xbfe08000, ldstexcl, OP4 (Rs, Rt, Rt2, ADDR_SIMPLE), QL_R3_LDST_EXC, F_GPRSIZE_IN_Q),
  CORE_INSN ("ldxr", 0x885f7c00, 0xbfe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_R1NIL, F_GPRSIZE_IN_Q),
  CORE_INSN ("ldaxr", 0x885ffc00, 0xbfe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_R1NIL, F_GPRSIZE_IN_Q),
  CORE_INSN ("ldxp", 0x887f0000, 0xbfe08000, ldstexcl, OP3 (Rt, Rt2, ADDR_SIMPLE), QL_R2NIL, F_GPRSIZE_IN_Q),
  CORE_INSN ("ldaxp", 0x887f8000, 0xbfe08000, ldstexcl, OP3 (Rt, Rt2, ADDR_SIMPLE), QL_R2NIL, F_GPRSIZE_IN_Q),
  CORE_INSN ("stlr", 0x889ffc00, 0xbfe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_R1NIL, F_GPRSIZE_IN_Q),
  CORE_INSN ("ldar", 0x88dffc00, 0xbfeffc00, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_R1NIL, F_GPRSIZE_IN_Q),
  /* Limited Ordering Regions load/store instructions.  */
  _LOR_INSN ("ldlar",  0x88df7c00, 0xbfe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_R1NIL,       F_GPRSIZE_IN_Q),
  _LOR_INSN ("ldlarb", 0x08df7c00, 0xffe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  _LOR_INSN ("ldlarh", 0x48df7c00, 0xffe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  _LOR_INSN ("stllr",  0x889f7c00, 0xbfe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_R1NIL,       F_GPRSIZE_IN_Q),
  _LOR_INSN ("stllrb", 0x089f7c00, 0xffe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  _LOR_INSN ("stllrh", 0x489f7c00, 0xbfe08000, ldstexcl, OP2 (Rt, ADDR_SIMPLE), QL_W1_LDST_EXC, 0),
  /* Load/store no-allocate pair (offset).  */
  CORE_INSN ("stnp", 0x28000000, 0x7fc00000, ldstnapair_offs, OP3 (Rt, Rt2, ADDR_SIMM7), QL_LDST_PAIR_R, F_SF),
  CORE_INSN ("ldnp", 0x28400000, 0x7fc00000, ldstnapair_offs, OP3 (Rt, Rt2, ADDR_SIMM7), QL_LDST_PAIR_R, F_SF),
  CORE_INSN ("stnp", 0x2c000000, 0x3fc00000, ldstnapair_offs, OP3 (Ft, Ft2, ADDR_SIMM7), QL_LDST_PAIR_FP, 0),
  CORE_INSN ("ldnp", 0x2c400000, 0x3fc00000, ldstnapair_offs, OP3 (Ft, Ft2, ADDR_SIMM7), QL_LDST_PAIR_FP, 0),
  /* Load/store register pair (offset).  */
  CORE_INSN ("stp", 0x29000000, 0x7ec00000, ldstpair_off, OP3 (Rt, Rt2, ADDR_SIMM7), QL_LDST_PAIR_R, F_SF),
  CORE_INSN ("ldp", 0x29400000, 0x7ec00000, ldstpair_off, OP3 (Rt, Rt2, ADDR_SIMM7), QL_LDST_PAIR_R, F_SF),
  CORE_INSN ("stp", 0x2d000000, 0x3fc00000, ldstpair_off, OP3 (Ft, Ft2, ADDR_SIMM7), QL_LDST_PAIR_FP, 0),
  CORE_INSN ("ldp", 0x2d400000, 0x3fc00000, ldstpair_off, OP3 (Ft, Ft2, ADDR_SIMM7), QL_LDST_PAIR_FP, 0),
  {"ldpsw", 0x69400000, 0xffc00000, ldstpair_off, 0, CORE, OP3 (Rt, Rt2, ADDR_SIMM7), QL_LDST_PAIR_X32, 0, VERIFIER (ldpsw)},
  /* Load/store register pair (indexed).  */
  CORE_INSN ("stp", 0x28800000, 0x7ec00000, ldstpair_indexed, OP3 (Rt, Rt2, ADDR_SIMM7), QL_LDST_PAIR_R, F_SF),
  CORE_INSN ("ldp", 0x28c00000, 0x7ec00000, ldstpair_indexed, OP3 (Rt, Rt2, ADDR_SIMM7), QL_LDST_PAIR_R, F_SF),
  CORE_INSN ("stp", 0x2c800000, 0x3ec00000, ldstpair_indexed, OP3 (Ft, Ft2, ADDR_SIMM7), QL_LDST_PAIR_FP, 0),
  CORE_INSN ("ldp", 0x2cc00000, 0x3ec00000, ldstpair_indexed, OP3 (Ft, Ft2, ADDR_SIMM7), QL_LDST_PAIR_FP, 0),
  {"ldpsw", 0x68c00000, 0xfec00000, ldstpair_indexed, 0, CORE, OP3 (Rt, Rt2, ADDR_SIMM7), QL_LDST_PAIR_X32, 0, VERIFIER (ldpsw)},
  /* Load register (literal).  */
  {"ldr",   0x18000000, 0xbf000000, loadlit, OP_LDR_LIT,   CORE, OP2 (Rt, ADDR_PCREL19),    QL_R_PCREL, F_GPRSIZE_IN_Q, NULL},
  {"ldr",   0x1c000000, 0x3f000000, loadlit, OP_LDRV_LIT,  CORE, OP2 (Ft, ADDR_PCREL19),    QL_FP_PCREL, 0, NULL},
  {"ldrsw", 0x98000000, 0xff000000, loadlit, OP_LDRSW_LIT, CORE, OP2 (Rt, ADDR_PCREL19),    QL_X_PCREL, 0, NULL},
  {"prfm",  0xd8000000, 0xff000000, loadlit, OP_PRFM_LIT,  CORE, OP2 (PRFOP, ADDR_PCREL19), QL_PRFM_PCREL, 0, NULL},
  /* Logical (immediate).  */
  CORE_INSN ("and", 0x12000000, 0x7f800000, log_imm, OP3 (Rd_SP, Rn, LIMM), QL_R2NIL, F_HAS_ALIAS | F_SF),
  {"bic", 0x12000000, 0x7f800000, log_imm, OP_BIC, CORE, OP3 (Rd_SP, Rn, LIMM), QL_R2NIL, F_ALIAS | F_PSEUDO | F_SF, NULL},
  CORE_INSN ("orr", 0x32000000, 0x7f800000, log_imm, OP3 (Rd_SP, Rn, LIMM), QL_R2NIL, F_HAS_ALIAS | F_SF),
  {"mov", 0x320003e0, 0x7f8003e0, log_imm, OP_MOV_IMM_LOG, CORE, OP2 (Rd_SP, IMM_MOV), QL_R1NIL, F_ALIAS | F_P1 | F_SF | F_CONV, NULL},
  CORE_INSN ("eor", 0x52000000, 0x7f800000, log_imm, OP3 (Rd_SP, Rn, LIMM), QL_R2NIL, F_SF),
  CORE_INSN ("ands", 0x72000000, 0x7f800000, log_imm, OP3 (Rd, Rn, LIMM), QL_R2NIL, F_HAS_ALIAS | F_SF),
  CORE_INSN ("tst", 0x7200001f, 0x7f80001f, log_imm, OP2 (Rn, LIMM), QL_R1NIL, F_ALIAS | F_SF),
  /* Logical (shifted register).  */
  CORE_INSN ("and", 0xa000000, 0x7f200000, log_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_SF),
  CORE_INSN ("bic", 0xa200000, 0x7f200000, log_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_SF),
  CORE_INSN ("orr", 0x2a000000, 0x7f200000, log_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_HAS_ALIAS | F_SF),
  CORE_INSN ("mov", 0x2a0003e0, 0x7f2003e0, log_shift, OP2 (Rd, Rm_SFT), QL_I2SAMER, F_ALIAS | F_SF),
  {"uxtw", 0x2a0003e0, 0x7f2003e0, log_shift, OP_UXTW, CORE, OP2 (Rd, Rm), QL_I2SAMEW, F_ALIAS | F_PSEUDO, NULL},
  CORE_INSN ("orn", 0x2a200000, 0x7f200000, log_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_HAS_ALIAS | F_SF),
  CORE_INSN ("mvn", 0x2a2003e0, 0x7f2003e0, log_shift, OP2 (Rd, Rm_SFT), QL_I2SAMER, F_ALIAS | F_SF),
  CORE_INSN ("eor", 0x4a000000, 0x7f200000, log_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_SF),
  CORE_INSN ("eon", 0x4a200000, 0x7f200000, log_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_SF),
  CORE_INSN ("ands", 0x6a000000, 0x7f200000, log_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_HAS_ALIAS | F_SF),
  CORE_INSN ("tst", 0x6a00001f, 0x7f20001f, log_shift, OP2 (Rn, Rm_SFT), QL_I2SAMER, F_ALIAS | F_SF),
  CORE_INSN ("bics", 0x6a200000, 0x7f200000, log_shift, OP3 (Rd, Rn, Rm_SFT), QL_I3SAMER, F_SF),
  /* LSE extension (atomic).  */
  _LSE_INSN ("casb", 0x8a07c00, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("cash", 0x48a07c00, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("cas", 0x88a07c00, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("casab", 0x8e07c00, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("caslb", 0x8a0fc00, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("casalb", 0x8e0fc00, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("casah", 0x48e07c00, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("caslh", 0x48a0fc00, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("casalh", 0x48e0fc00, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("casa", 0x88e07c00, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("casl", 0x88a0fc00, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("casal", 0x88e0fc00, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("casp", 0x8207c00, 0xbfe0fc00, lse_atomic, OP5 (Rs, PAIRREG, Rt, PAIRREG, ADDR_SIMPLE), QL_R4NIL, F_LSE_SZ),
  _LSE_INSN ("caspa", 0x8607c00, 0xbfe0fc00, lse_atomic, OP5 (Rs, PAIRREG, Rt, PAIRREG, ADDR_SIMPLE), QL_R4NIL, F_LSE_SZ),
  _LSE_INSN ("caspl", 0x820fc00, 0xbfe0fc00, lse_atomic, OP5 (Rs, PAIRREG, Rt, PAIRREG, ADDR_SIMPLE), QL_R4NIL, F_LSE_SZ),
  _LSE_INSN ("caspal", 0x860fc00, 0xbfe0fc00, lse_atomic, OP5 (Rs, PAIRREG, Rt, PAIRREG, ADDR_SIMPLE), QL_R4NIL, F_LSE_SZ),
  _LSE_INSN ("swpb", 0x38208000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("swph", 0x78208000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("swp", 0xb8208000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("swpab", 0x38a08000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("swplb", 0x38608000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("swpalb", 0x38e08000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("swpah", 0x78a08000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("swplh", 0x78608000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("swpalh", 0x78e08000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("swpa", 0xb8a08000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("swpl", 0xb8608000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("swpal", 0xb8e08000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldaddb", 0x38200000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldaddh", 0x78200000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldadd", 0xb8200000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldaddab", 0x38a00000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldaddlb", 0x38600000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldaddalb", 0x38e00000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldaddah", 0x78a00000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldaddlh", 0x78600000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldaddalh", 0x78e00000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldadda", 0xb8a00000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldaddl", 0xb8600000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldaddal", 0xb8e00000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldclrb", 0x38201000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldclrh", 0x78201000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldclr", 0xb8201000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldclrab", 0x38a01000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldclrlb", 0x38601000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldclralb", 0x38e01000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldclrah", 0x78a01000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldclrlh", 0x78601000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldclralh", 0x78e01000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldclra", 0xb8a01000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldclrl", 0xb8601000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldclral", 0xb8e01000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldeorb", 0x38202000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldeorh", 0x78202000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldeor", 0xb8202000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldeorab", 0x38a02000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldeorlb", 0x38602000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldeoralb", 0x38e02000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldeorah", 0x78a02000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldeorlh", 0x78602000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldeoralh", 0x78e02000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldeora", 0xb8a02000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldeorl", 0xb8602000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldeoral", 0xb8e02000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldsetb", 0x38203000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldseth", 0x78203000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldset", 0xb8203000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldsetab", 0x38a03000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldsetlb", 0x38603000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldsetalb", 0x38e03000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldsetah", 0x78a03000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldsetlh", 0x78603000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldsetalh", 0x78e03000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldseta", 0xb8a03000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldsetl", 0xb8603000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldsetal", 0xb8e03000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldsmaxb", 0x38204000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldsmaxh", 0x78204000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldsmax", 0xb8204000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldsmaxab", 0x38a04000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldsmaxlb", 0x38604000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldsmaxalb", 0x38e04000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldsmaxah", 0x78a04000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldsmaxlh", 0x78604000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldsmaxalh", 0x78e04000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldsmaxa", 0xb8a04000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldsmaxl", 0xb8604000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldsmaxal", 0xb8e04000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldsminb", 0x38205000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldsminh", 0x78205000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldsmin", 0xb8205000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldsminab", 0x38a05000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldsminlb", 0x38605000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldsminalb", 0x38e05000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldsminah", 0x78a05000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldsminlh", 0x78605000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldsminalh", 0x78e05000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldsmina", 0xb8a05000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldsminl", 0xb8605000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldsminal", 0xb8e05000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldumaxb", 0x38206000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldumaxh", 0x78206000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldumax", 0xb8206000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldumaxab", 0x38a06000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldumaxlb", 0x38606000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldumaxalb", 0x38e06000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldumaxah", 0x78a06000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldumaxlh", 0x78606000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldumaxalh", 0x78e06000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldumaxa", 0xb8a06000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("ldumaxl", 0xb8606000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("ldumaxal", 0xb8e06000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("lduminb", 0x38207000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("lduminh", 0x78207000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("ldumin", 0xb8207000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("lduminab", 0x38a07000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("lduminlb", 0x38607000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("lduminalb", 0x38e07000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("lduminah", 0x78a07000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("lduminlh", 0x78607000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, F_HAS_ALIAS),
  _LSE_INSN ("lduminalh", 0x78e07000, 0xffe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_W2_LDST_EXC, 0),
  _LSE_INSN ("ldumina", 0xb8a07000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("lduminl", 0xb8607000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ | F_HAS_ALIAS),
  _LSE_INSN ("lduminal", 0xb8e07000, 0xbfe0fc00, lse_atomic, OP3 (Rs, Rt, ADDR_SIMPLE), QL_R2NIL, F_LSE_SZ),
  _LSE_INSN ("staddb", 0x3820001f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("staddh", 0x7820001f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stadd", 0xb820001f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("staddlb", 0x3860001f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("staddlh", 0x7860001f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("staddl", 0xb860001f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stclrb", 0x3820101f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stclrh", 0x7820101f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stclr", 0xb820101f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stclrlb", 0x3860101f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stclrlh", 0x7860101f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stclrl", 0xb860101f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("steorb", 0x3820201f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("steorh", 0x7820201f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("steor", 0xb820201f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("steorlb", 0x3860201f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("steorlh", 0x7860201f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("steorl", 0xb860201f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stsetb", 0x3820301f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stseth", 0x7820301f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stset", 0xb820301f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stsetlb", 0x3860301f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stsetlh", 0x7860301f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stsetl", 0xb860301f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stsmaxb", 0x3820401f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stsmaxh", 0x7820401f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stsmax", 0xb820401f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stsmaxlb", 0x3860401f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stsmaxlh", 0x7860401f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stsmaxl", 0xb860401f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stsminb", 0x3820501f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stsminh", 0x7820501f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stsmin", 0xb820501f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stsminlb", 0x3860501f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stsminlh", 0x7860501f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stsminl", 0xb860501f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stumaxb", 0x3820601f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stumaxh", 0x7820601f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stumax", 0xb820601f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stumaxlb", 0x3860601f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stumaxlh", 0x7860601f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stumaxl", 0xb860601f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stuminb", 0x3820701f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stuminh", 0x7820701f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stumin", 0xb820701f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  _LSE_INSN ("stuminlb", 0x3860701f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stuminlh", 0x7860701f, 0xffe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_W1_LDST_EXC, F_ALIAS),
  _LSE_INSN ("stuminl", 0xb860701f, 0xbfe0fc1f, lse_atomic, OP2 (Rs, ADDR_SIMPLE), QL_R1NIL, F_LSE_SZ | F_ALIAS),
  /* Move wide (immediate).  */
  {"movn", 0x12800000, 0x7f800000, movewide, OP_MOVN, CORE, OP2 (Rd, HALF), QL_DST_R, F_SF | F_HAS_ALIAS, NULL},
  {"mov",  0x12800000, 0x7f800000, movewide, OP_MOV_IMM_WIDEN, CORE, OP2 (Rd, IMM_MOV), QL_DST_R, F_SF | F_ALIAS | F_CONV, NULL},
  {"movz", 0x52800000, 0x7f800000, movewide, OP_MOVZ, CORE, OP2 (Rd, HALF), QL_DST_R, F_SF | F_HAS_ALIAS, NULL},
  {"mov",  0x52800000, 0x7f800000, movewide, OP_MOV_IMM_WIDE, CORE, OP2 (Rd, IMM_MOV), QL_DST_R, F_SF | F_ALIAS | F_CONV, NULL},
  {"movk", 0x72800000, 0x7f800000, movewide, OP_MOVK, CORE, OP2 (Rd, HALF), QL_DST_R, F_SF, NULL},
  /* PC-rel. addressing.  */
  CORE_INSN ("adr",  0x10000000, 0x9f000000, pcreladdr, OP2 (Rd, ADDR_PCREL21), QL_ADRP, 0),
  CORE_INSN ("adrp", 0x90000000, 0x9f000000, pcreladdr, OP2 (Rd, ADDR_ADRP), QL_ADRP, 0),
  /* System.  */
  CORE_INSN ("msr", 0xd500401f, 0xfff8f01f, ic_system, OP2 (PSTATEFIELD, UIMM4), {}, 0),
  CORE_INSN ("hint",0xd503201f, 0xfffff01f, ic_system, OP1 (UIMM7), {}, F_HAS_ALIAS),
  CORE_INSN ("nop", 0xd503201f, 0xffffffff, ic_system, OP0 (), {}, F_ALIAS),
  CORE_INSN ("yield", 0xd503203f, 0xffffffff, ic_system, OP0 (), {}, F_ALIAS),
  CORE_INSN ("wfe", 0xd503205f, 0xffffffff, ic_system, OP0 (), {}, F_ALIAS),
  CORE_INSN ("wfi", 0xd503207f, 0xffffffff, ic_system, OP0 (), {}, F_ALIAS),
  CORE_INSN ("sev", 0xd503209f, 0xffffffff, ic_system, OP0 (), {}, F_ALIAS),
  CORE_INSN ("sevl",0xd50320bf, 0xffffffff, ic_system, OP0 (), {}, F_ALIAS),
  {"esb", 0xd503221f, 0xffffffff, ic_system, 0, RAS, OP0 (), {}, F_ALIAS, NULL},
  {"psb", 0xd503223f, 0xffffffff, ic_system, 0, STAT_PROFILE, OP1 (BARRIER_PSB), {}, F_ALIAS, NULL},
  CORE_INSN ("clrex", 0xd503305f, 0xfffff0ff, ic_system, OP1 (UIMM4), {}, F_OPD0_OPT | F_DEFAULT (0xF)),
  CORE_INSN ("dsb", 0xd503309f, 0xfffff0ff, ic_system, OP1 (BARRIER), {}, 0),
  CORE_INSN ("dmb", 0xd50330bf, 0xfffff0ff, ic_system, OP1 (BARRIER), {}, 0),
  CORE_INSN ("isb", 0xd50330df, 0xfffff0ff, ic_system, OP1 (BARRIER_ISB), {}, F_OPD0_OPT | F_DEFAULT (0xF)),
  CORE_INSN ("sys", 0xd5080000, 0xfff80000, ic_system, OP5 (UIMM3_OP1, Cn, Cm, UIMM3_OP2, Rt), QL_SYS, F_HAS_ALIAS | F_OPD4_OPT | F_DEFAULT (0x1F)),
  CORE_INSN ("at",  0xd5080000, 0xfff80000, ic_system, OP2 (SYSREG_AT, Rt), QL_SRC_X, F_ALIAS),
  CORE_INSN ("dc",  0xd5080000, 0xfff80000, ic_system, OP2 (SYSREG_DC, Rt), QL_SRC_X, F_ALIAS),
  CORE_INSN ("ic",  0xd5080000, 0xfff80000, ic_system, OP2 (SYSREG_IC, Rt_SYS), QL_SRC_X, F_ALIAS | F_OPD1_OPT | F_DEFAULT (0x1F)),
  CORE_INSN ("tlbi",0xd5080000, 0xfff80000, ic_system, OP2 (SYSREG_TLBI, Rt_SYS), QL_SRC_X, F_ALIAS | F_OPD1_OPT | F_DEFAULT (0x1F)),
  CORE_INSN ("msr", 0xd5000000, 0xffe00000, ic_system, OP2 (SYSREG, Rt), QL_SRC_X, 0),
  CORE_INSN ("sysl",0xd5280000, 0xfff80000, ic_system, OP5 (Rt, UIMM3_OP1, Cn, Cm, UIMM3_OP2), QL_SYSL, 0),
  CORE_INSN ("mrs", 0xd5200000, 0xffe00000, ic_system, OP2 (Rt, SYSREG), QL_DST_X, 0),
  /* Test & branch (immediate).  */
  CORE_INSN ("tbz", 0x36000000, 0x7f000000, testbranch, OP3 (Rt, BIT_NUM, ADDR_PCREL14), QL_PCREL_14, 0),
  CORE_INSN ("tbnz",0x37000000, 0x7f000000, testbranch, OP3 (Rt, BIT_NUM, ADDR_PCREL14), QL_PCREL_14, 0),
  /* The old UAL conditional branch mnemonics (to aid portability).  */
  CORE_INSN ("beq", 0x54000000, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bne", 0x54000001, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bcs", 0x54000002, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bhs", 0x54000002, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bcc", 0x54000003, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("blo", 0x54000003, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bmi", 0x54000004, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bpl", 0x54000005, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bvs", 0x54000006, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bvc", 0x54000007, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bhi", 0x54000008, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bls", 0x54000009, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bge", 0x5400000a, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("blt", 0x5400000b, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("bgt", 0x5400000c, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),
  CORE_INSN ("ble", 0x5400000d, 0xff00001f, condbranch, OP1 (ADDR_PCREL19), QL_PCREL_NIL, F_ALIAS | F_PSEUDO),

  {0, 0, 0, 0, 0, 0, {}, {}, 0, NULL},
};

#ifdef AARCH64_OPERANDS
#undef AARCH64_OPERANDS
#endif

/* Macro-based operand decription; this will be fed into aarch64-gen for it
   to generate the structure aarch64_operands and the function
   aarch64_insert_operand and aarch64_extract_operand.

   These inserters and extracters in the description execute the conversion
   between the aarch64_opnd_info and value in the operand-related instruction
   field(s).  */

/* Y expects arguments (left to right) to be operand class, inserter/extractor
   name suffix, operand name, flags, related bitfield(s) and description.
   X only differs from Y by having the operand inserter and extractor names
   listed separately.  */

#define AARCH64_OPERANDS						\
    Y(INT_REG, regno, "Rd", 0, F(FLD_Rd), "an integer register")	\
    Y(INT_REG, regno, "Rn", 0, F(FLD_Rn), "an integer register")	\
    Y(INT_REG, regno, "Rm", 0, F(FLD_Rm), "an integer register")	\
    Y(INT_REG, regno, "Rt", 0, F(FLD_Rt), "an integer register")	\
    Y(INT_REG, regno, "Rt2", 0, F(FLD_Rt2), "an integer register")	\
    Y(INT_REG, regno, "Rs", 0, F(FLD_Rs), "an integer register")	\
    Y(INT_REG, regno, "Ra", 0, F(FLD_Ra), "an integer register")	\
    X(INT_REG, ins_regno, ext_regrt_sysins, "Rt_SYS", 0, F(FLD_Rt),	\
      "an integer register")						\
    Y(INT_REG, regno, "Rd_SP", OPD_F_MAYBE_SP, F(FLD_Rd),		\
      "an integer or stack pointer register")				\
    Y(INT_REG, regno, "Rn_SP", OPD_F_MAYBE_SP, F(FLD_Rn),		\
      "an integer or stack pointer register")				\
    X(INT_REG, 0, ext_regno_pair, "PAIRREG", 0, F(),			\
      "the second reg of a pair")					\
    Y(MODIFIED_REG, reg_extended, "Rm_EXT", 0, F(),			\
      "an integer register with optional extension")			\
    Y(MODIFIED_REG, reg_shifted, "Rm_SFT", 0, F(),			\
      "an integer register with optional shift")			\
    Y(FP_REG, regno, "Fd", 0, F(FLD_Rd), "a floating-point register")	\
    Y(FP_REG, regno, "Fn", 0, F(FLD_Rn), "a floating-point register")	\
    Y(FP_REG, regno, "Fm", 0, F(FLD_Rm), "a floating-point register")	\
    Y(FP_REG, regno, "Fa", 0, F(FLD_Ra), "a floating-point register")	\
    Y(FP_REG, ft, "Ft", 0, F(FLD_Rt), "a floating-point register")	\
    Y(FP_REG, regno, "Ft2", 0, F(FLD_Rt2), "a floating-point register")	\
    Y(SISD_REG, regno, "Sd", 0, F(FLD_Rd), "a SIMD scalar register")	\
    Y(SISD_REG, regno, "Sn", 0, F(FLD_Rn), "a SIMD scalar register")	\
    Y(SISD_REG, regno, "Sm", 0, F(FLD_Rm), "a SIMD scalar register")	\
    Y(SIMD_REG, regno, "Vd", 0, F(FLD_Rd), "a SIMD vector register")	\
    Y(SIMD_REG, regno, "Vn", 0, F(FLD_Rn), "a SIMD vector register")	\
    Y(SIMD_REG, regno, "Vm", 0, F(FLD_Rm), "a SIMD vector register")	\
    Y(FP_REG, regno, "VdD1", 0, F(FLD_Rd),				\
      "the top half of a 128-bit FP/SIMD register")			\
    Y(FP_REG, regno, "VnD1", 0, F(FLD_Rn),				\
      "the top half of a 128-bit FP/SIMD register")			\
    Y(SIMD_ELEMENT, reglane, "Ed", 0, F(FLD_Rd),			\
      "a SIMD vector element")						\
    Y(SIMD_ELEMENT, reglane, "En", 0, F(FLD_Rn),			\
      "a SIMD vector element")						\
    Y(SIMD_ELEMENT, reglane, "Em", 0, F(FLD_Rm),			\
      "a SIMD vector element")						\
    Y(SIMD_REGLIST, reglist, "LVn", 0, F(FLD_Rn),			\
      "a SIMD vector register list")					\
    Y(SIMD_REGLIST, ldst_reglist, "LVt", 0, F(),			\
      "a SIMD vector register list")					\
    Y(SIMD_REGLIST, ldst_reglist_r, "LVt_AL", 0, F(),			\
      "a SIMD vector register list")					\
    Y(SIMD_REGLIST, ldst_elemlist, "LEt", 0, F(),			\
      "a SIMD vector element list")					\
    Y(CP_REG, regno, "Cn", 0, F(FLD_CRn),				\
      "a 4-bit opcode field named for historical reasons C0 - C15")	\
    Y(CP_REG, regno, "Cm", 0, F(FLD_CRm),				\
      "a 4-bit opcode field named for historical reasons C0 - C15")	\
    Y(IMMEDIATE, imm, "IDX", 0, F(FLD_imm4),				\
      "an immediate as the index of the least significant byte")	\
    Y(IMMEDIATE, advsimd_imm_shift, "IMM_VLSL", 0, F(),			\
      "a left shift amount for an AdvSIMD register")			\
    Y(IMMEDIATE, advsimd_imm_shift, "IMM_VLSR", 0, F(),			\
      "a right shift amount for an AdvSIMD register")			\
    Y(IMMEDIATE, advsimd_imm_modified, "SIMD_IMM", 0, F(),		\
      "an immediate")							\
    Y(IMMEDIATE, advsimd_imm_modified, "SIMD_IMM_SFT", 0, F(),		\
      "an 8-bit unsigned immediate with optional shift")		\
    Y(IMMEDIATE, advsimd_imm_modified, "SIMD_FPIMM", 0, F(),		\
      "an 8-bit floating-point constant")				\
    X(IMMEDIATE, 0, ext_shll_imm, "SHLL_IMM", 0, F(),			\
      "an immediate shift amount of 8, 16 or 32")			\
    X(IMMEDIATE, 0, 0, "IMM0", 0, F(), "0")				\
    X(IMMEDIATE, 0, 0, "FPIMM0", 0, F(), "0.0")				\
    Y(IMMEDIATE, imm, "FPIMM", 0, F(FLD_imm8),				\
      "an 8-bit floating-point constant")				\
    Y(IMMEDIATE, imm, "IMMR", 0, F(FLD_immr),				\
      "the right rotate amount")					\
    Y(IMMEDIATE, imm, "IMMS", 0, F(FLD_imm6),				\
      "the leftmost bit number to be moved from the source")		\
    Y(IMMEDIATE, imm, "WIDTH", 0, F(FLD_imm6),				\
      "the width of the bit-field")					\
    Y(IMMEDIATE, imm, "IMM", 0, F(FLD_imm6), "an immediate")		\
    Y(IMMEDIATE, imm, "UIMM3_OP1", 0, F(FLD_op1),			\
      "a 3-bit unsigned immediate")					\
    Y(IMMEDIATE, imm, "UIMM3_OP2", 0, F(FLD_op2),			\
      "a 3-bit unsigned immediate")					\
    Y(IMMEDIATE, imm, "UIMM4", 0, F(FLD_CRm),				\
      "a 4-bit unsigned immediate")					\
    Y(IMMEDIATE, imm, "UIMM7", 0, F(FLD_CRm, FLD_op2),			\
      "a 7-bit unsigned immediate")					\
    Y(IMMEDIATE, imm, "BIT_NUM", 0, F(FLD_b5, FLD_b40),			\
      "the bit number to be tested")					\
    Y(IMMEDIATE, imm, "EXCEPTION", 0, F(FLD_imm16),			\
      "a 16-bit unsigned immediate")					\
    Y(IMMEDIATE, imm, "CCMP_IMM", 0, F(FLD_imm5),			\
      "a 5-bit unsigned immediate")					\
    Y(IMMEDIATE, imm, "NZCV", 0, F(FLD_nzcv),				\
      "a flag bit specifier giving an alternative value for each flag")	\
    Y(IMMEDIATE, limm, "LIMM", 0, F(FLD_N,FLD_immr,FLD_imms),		\
      "Logical immediate")						\
    Y(IMMEDIATE, aimm, "AIMM", 0, F(FLD_shift,FLD_imm12),		\
      "a 12-bit unsigned immediate with optional left shift of 12 bits")\
    Y(IMMEDIATE, imm_half, "HALF", 0, F(FLD_imm16),			\
      "a 16-bit immediate with optional left shift")			\
    Y(IMMEDIATE, fbits, "FBITS", 0, F(FLD_scale),			\
      "the number of bits after the binary point in the fixed-point value")\
    X(IMMEDIATE, 0, 0, "IMM_MOV", 0, F(), "an immediate")		\
    Y(COND, cond, "COND", 0, F(), "a condition")			\
    Y(COND, cond, "COND1", 0, F(),					\
      "one of the standard conditions, excluding AL and NV.")		\
    X(ADDRESS, 0, ext_imm, "ADDR_ADRP", OPD_F_SEXT, F(FLD_immhi, FLD_immlo),\
      "21-bit PC-relative address of a 4KB page")			\
    Y(ADDRESS, imm, "ADDR_PCREL14", OPD_F_SEXT | OPD_F_SHIFT_BY_2,	\
      F(FLD_imm14), "14-bit PC-relative address")			\
    Y(ADDRESS, imm, "ADDR_PCREL19", OPD_F_SEXT | OPD_F_SHIFT_BY_2,	\
      F(FLD_imm19), "19-bit PC-relative address")			\
    Y(ADDRESS, imm, "ADDR_PCREL21", OPD_F_SEXT, F(FLD_immhi,FLD_immlo),	\
      "21-bit PC-relative address")					\
    Y(ADDRESS, imm, "ADDR_PCREL26", OPD_F_SEXT | OPD_F_SHIFT_BY_2,	\
      F(FLD_imm26), "26-bit PC-relative address")			\
    Y(ADDRESS, addr_simple, "ADDR_SIMPLE", 0, F(),			\
      "an address with base register (no offset)")			\
    Y(ADDRESS, addr_regoff, "ADDR_REGOFF", 0, F(),			\
      "an address with register offset")				\
    Y(ADDRESS, addr_simm, "ADDR_SIMM7", 0, F(FLD_imm7,FLD_index2),	\
      "an address with 7-bit signed immediate offset")			\
    Y(ADDRESS, addr_simm, "ADDR_SIMM9", 0, F(FLD_imm9,FLD_index),	\
      "an address with 9-bit signed immediate offset")			\
    Y(ADDRESS, addr_simm, "ADDR_SIMM9_2", 0, F(FLD_imm9,FLD_index),	\
      "an address with 9-bit negative or unaligned immediate offset")	\
    Y(ADDRESS, addr_uimm12, "ADDR_UIMM12", 0, F(FLD_Rn,FLD_imm12),	\
      "an address with scaled, unsigned immediate offset")		\
    Y(ADDRESS, addr_simple, "SIMD_ADDR_SIMPLE", 0, F(),			\
      "an address with base register (no offset)")			\
    Y(ADDRESS, simd_addr_post, "SIMD_ADDR_POST", 0, F(),		\
      "a post-indexed address with immediate or register increment")	\
    Y(SYSTEM, sysreg, "SYSREG", 0, F(), "a system register")		\
    Y(SYSTEM, pstatefield, "PSTATEFIELD", 0, F(),			\
      "a PSTATE field name")						\
    Y(SYSTEM, sysins_op, "SYSREG_AT", 0, F(),				\
      "an address translation operation specifier")			\
    Y(SYSTEM, sysins_op, "SYSREG_DC", 0, F(),				\
      "a data cache maintenance operation specifier")			\
    Y(SYSTEM, sysins_op, "SYSREG_IC", 0, F(),				\
      "an instruction cache maintenance operation specifier")		\
    Y(SYSTEM, sysins_op, "SYSREG_TLBI", 0, F(),				\
      "a TBL invalidation operation specifier")				\
    Y(SYSTEM, barrier, "BARRIER", 0, F(),				\
      "a barrier option name")						\
    Y(SYSTEM, barrier, "BARRIER_ISB", 0, F(),				\
      "the ISB option name SY or an optional 4-bit unsigned immediate")	\
    Y(SYSTEM, prfop, "PRFOP", 0, F(),					\
      "a prefetch operation specifier")					\
    Y (SYSTEM, hint, "BARRIER_PSB", 0, F (),				\
      "the PSB option name CSYNC")
